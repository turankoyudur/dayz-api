#include <cstring>
#include <string>
#include <mutex>
#include <random>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "http_server.h"

namespace fs = std::filesystem;

static std::mutex g_mu;
static std::string g_apiKey;
static std::string g_bindIp = "127.0.0.1";
static int g_bindPort = 8192;
static fs::path g_dataDir;
static MiniHttpServer g_server;
static bool g_serverStarted = false;

static std::string readFile(const fs::path& p)
{
    std::ifstream f(p, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static bool writeFileAtomic(const fs::path& p, const std::string& content)
{
    try
    {
        fs::create_directories(p.parent_path());
        fs::path tmp = p;
        tmp += ".tmp";
        {
            std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
            if (!f) return false;
            f.write(content.data(), (std::streamsize)content.size());
        }
        std::error_code ec;
        fs::rename(tmp, p, ec);
        if (ec)
        {
            // windows: rename over existing can fail; try remove then rename
            fs::remove(p, ec);
            fs::rename(tmp, p, ec);
        }
        if (ec)
        {
            fs::remove(tmp);
            return false;
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

static std::string randHex(size_t n)
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(n);
    for (size_t i = 0; i < n; i++) out.push_back(hex[(size_t)(rng() % 16)]);
    return out;
}

static std::string jsonEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s)
    {
        switch (c)
        {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

// Very small JSON body helpers (not a full JSON parser; expects simple flat objects)
static bool bodyHasKey(const std::string& body, const std::string& key)
{
    std::string pat = "\"" + key + "\"";
    return body.find(pat) != std::string::npos;
}

static std::string bodyGetString(const std::string& body, const std::string& key, const std::string& def = "")
{
    std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == std::string::npos) return def;
    size_t colon = body.find(':', k + pat.size());
    if (colon == std::string::npos) return def;
    size_t i = colon + 1;
    while (i < body.size() && (body[i] == ' ' || body[i] == '\t' || body[i] == '\n' || body[i] == '\r')) i++;
    if (i >= body.size() || body[i] != '"') return def;
    i++;
    size_t j = i;
    while (j < body.size() && body[j] != '"')
    {
        if (body[j] == '\\' && j + 1 < body.size()) j += 2;
        else j++;
    }
    if (j >= body.size()) return def;
    return body.substr(i, j - i);
}

static double bodyGetNumber(const std::string& body, const std::string& key, double def = 0.0)
{
    std::string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == std::string::npos) return def;
    size_t colon = body.find(':', k + pat.size());
    if (colon == std::string::npos) return def;
    size_t i = colon + 1;
    while (i < body.size() && (body[i] == ' ' || body[i] == '\t' || body[i] == '\n' || body[i] == '\r')) i++;
    size_t j = i;
    while (j < body.size() && (std::isdigit((unsigned char)body[j]) || body[j]=='-' || body[j]=='+' || body[j]=='.' || body[j]=='e' || body[j]=='E')) j++;
    if (j == i) return def;
    try { return std::stod(body.substr(i, j - i)); } catch (...) { return def; }
}

static std::vector<std::string> splitPath(const std::string& path)
{
    std::vector<std::string> parts;
    std::string cur;
    for (char c : path)
    {
        if (c == '/')
        {
            if (!cur.empty()) { parts.push_back(cur); cur.clear(); }
        }
        else cur.push_back(c);
    }
    if (!cur.empty()) parts.push_back(cur);
    return parts;
}

static fs::path statePath() { return g_dataDir / "state.json"; }
static fs::path commandsPath() { return g_dataDir / "commands.json"; }
static fs::path responsesPath() { return g_dataDir / "responses.json"; }

static bool appendCommandObject(const std::string& cmdObj)
{
    std::lock_guard<std::mutex> lk(g_mu);

    std::string existing = readFile(commandsPath());
    std::string out;

    if (existing.empty())
    {
        out = std::string("{\"commands\":[") + cmdObj + "]}";
        return writeFileAtomic(commandsPath(), out);
    }

    // naive insert before the last ]}
    size_t pos = existing.rfind("]}");
    if (pos == std::string::npos)
    {
        out = std::string("{\"commands\":[") + cmdObj + "]}";
        return writeFileAtomic(commandsPath(), out);
    }

    // if array is empty -> ...[ ]}
    size_t arrOpen = existing.find("[", existing.find("\"commands\""));
    size_t arrClose = existing.rfind("]");
    bool emptyArr = (arrOpen != std::string::npos && arrClose != std::string::npos && arrClose > arrOpen && existing.substr(arrOpen+1, arrClose-arrOpen-1).find_first_not_of(" \t\r\n") == std::string::npos);

    out = existing;
    if (emptyArr)
    {
        out.insert(arrClose, cmdObj);
    }
    else
    {
        out.insert(arrClose, std::string(",") + cmdObj);
    }

    return writeFileAtomic(commandsPath(), out);
}

static HttpResponse json(int status, const std::string& body)
{
    HttpResponse r;
    r.status = status;
    r.body = body;
    return r;
}

static bool checkKey(const HttpRequest& req)
{
    if (g_apiKey.empty()) return true;
    std::string got = req.header("x-api-key");
    return got == g_apiKey;
}

static HttpResponse handle(const HttpRequest& req)
{
    if (!checkKey(req))
        return json(401, "{\"ok\":false,\"error\":\"unauthorized\"}");

    auto parts = splitPath(req.path);

    if (req.method == "GET" && req.path == "/v1/health")
        return json(200, "{\"ok\":true}");

    if (req.method == "GET" && req.path == "/v1/state")
    {
        std::string s = readFile(statePath());
        if (s.empty()) return json(404, "{\"ok\":false,\"error\":\"no state yet\"}");
        HttpResponse r; r.status = 200; r.body = s; return r;
    }

    if (req.method == "GET" && req.path == "/v1/responses")
    {
        std::string s = readFile(responsesPath());
        if (s.empty()) s = "{\"responses\":[]}";
        HttpResponse r; r.status = 200; r.body = s; return r;
    }

    if (req.method == "GET" && req.path == "/v1/status")
    {
        bool hasState = fs::exists(statePath());
        bool hasResp = fs::exists(responsesPath());
        bool hasCmd = fs::exists(commandsPath());

        std::ostringstream ss;
        ss << "{\"ok\":true";
        ss << ",\"hasState\":" << (hasState?"true":"false");
        ss << ",\"hasResponses\":" << (hasResp?"true":"false");
        ss << ",\"hasCommands\":" << (hasCmd?"true":"false");
        ss << ",\"bindIp\":\"" << jsonEscape(g_bindIp) << "\"";
        ss << ",\"bindPort\":" << g_bindPort;
        ss << "}";
        return json(200, ss.str());
    }

    // POST /v1/players/{uid}/...
    if (req.method == "POST" && parts.size() >= 4 && parts[0] == "v1" && parts[1] == "players")
    {
        std::string uid = parts[2];
        std::string action = parts[3];
        std::string id = randHex(16);
        long long ts = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        std::ostringstream cmd;
        cmd << "{";
        cmd << "\"id\":\"" << id << "\",";
        cmd << "\"uid\":\"" << jsonEscape(uid) << "\",";

        if (action == "teleport")
        {
            double x = bodyGetNumber(req.body, "x", 0.0);
            double y = bodyGetNumber(req.body, "y", 0.0);
            double z = bodyGetNumber(req.body, "z", 0.0);

            cmd << "\"type\":\"teleport\",";
            cmd << "\"x\":" << x << ",\"y\":" << y << ",\"z\":" << z << ",";
            cmd << "\"ts\":" << ts;
            cmd << "}";

            if (!appendCommandObject(cmd.str()))
                return json(500, "{\"ok\":false,\"error\":\"failed to write command\"}");

            std::ostringstream out;
            out << "{\"ok\":true,\"accepted\":true,\"id\":\"" << id << "\"}";
            return json(200, out.str());
        }

        if (action == "inventory" && parts.size() >= 5)
        {
            std::string invAction = parts[4];
            if (invAction == "add")
            {
                std::string type = bodyGetString(req.body, "type", "");
                int qty = (int)bodyGetNumber(req.body, "quantity", 1);
                double health = bodyGetNumber(req.body, "health", 0.0);

                cmd << "\"type\":\"inv_add\",";
                cmd << "\"itemType\":\"" << jsonEscape(type) << "\",";
                cmd << "\"quantity\":" << qty << ",";
                cmd << "\"health\":" << health << ",";
                cmd << "\"ts\":" << ts;
                cmd << "}";

                if (!appendCommandObject(cmd.str()))
                    return json(500, "{\"ok\":false,\"error\":\"failed to write command\"}");

                std::ostringstream out;
                out << "{\"ok\":true,\"accepted\":true,\"id\":\"" << id << "\"}";
                return json(200, out.str());
            }

            if (invAction == "remove")
            {
                std::string type = bodyGetString(req.body, "type", "");
                int count = (int)bodyGetNumber(req.body, "count", 1);

                cmd << "\"type\":\"inv_remove\",";
                cmd << "\"itemType\":\"" << jsonEscape(type) << "\",";
                cmd << "\"quantity\":" << count << ",";
                cmd << "\"ts\":" << ts;
                cmd << "}";

                if (!appendCommandObject(cmd.str()))
                    return json(500, "{\"ok\":false,\"error\":\"failed to write command\"}");

                std::ostringstream out;
                out << "{\"ok\":true,\"accepted\":true,\"id\":\"" << id << "\"}";
                return json(200, out.str());
            }

            if (invAction == "setQuantity")
            {
                std::string type = bodyGetString(req.body, "type", "");
                int qty = (int)bodyGetNumber(req.body, "quantity", 1);

                cmd << "\"type\":\"inv_setqty\",";
                cmd << "\"itemType\":\"" << jsonEscape(type) << "\",";
                cmd << "\"quantity\":" << qty << ",";
                cmd << "\"ts\":" << ts;
                cmd << "}";

                if (!appendCommandObject(cmd.str()))
                    return json(500, "{\"ok\":false,\"error\":\"failed to write command\"}");

                std::ostringstream out;
                out << "{\"ok\":true,\"accepted\":true,\"id\":\"" << id << "\"}";
                return json(200, out.str());
            }

            return json(404, "{\"ok\":false,\"error\":\"unknown inventory action\"}");
        }

        if (action == "stats")
        {
            bool hasHealth = bodyHasKey(req.body, "health");
            bool hasBlood  = bodyHasKey(req.body, "blood");
            bool hasWater  = bodyHasKey(req.body, "water");
            bool hasEnergy = bodyHasKey(req.body, "energy");

            double h = bodyGetNumber(req.body, "health", 0.0);
            double b = bodyGetNumber(req.body, "blood", 0.0);
            double w = bodyGetNumber(req.body, "water", 0.0);
            double e = bodyGetNumber(req.body, "energy", 0.0);

            cmd << "\"type\":\"stats_set\",";
            cmd << "\"setHealth\":" << h << ",\"setBlood\":" << b << ",\"setWater\":" << w << ",\"setEnergy\":" << e << ",";
            cmd << "\"setHealthEnabled\":" << (hasHealth?"true":"false") << ",";
            cmd << "\"setBloodEnabled\":"  << (hasBlood?"true":"false")  << ",";
            cmd << "\"setWaterEnabled\":"  << (hasWater?"true":"false")  << ",";
            cmd << "\"setEnergyEnabled\":" << (hasEnergy?"true":"false") << ",";
            cmd << "\"ts\":" << ts;
            cmd << "}";

            if (!appendCommandObject(cmd.str()))
                return json(500, "{\"ok\":false,\"error\":\"failed to write command\"}");

            std::ostringstream out;
            out << "{\"ok\":true,\"accepted\":true,\"id\":\"" << id << "\"}";
            return json(200, out.str());
        }

        return json(404, "{\"ok\":false,\"error\":\"unknown action\"}");
    }

    return json(404, "{\"ok\":false,\"error\":\"not found\"}");
}

static bool parseInitJson(const std::string& j, std::string& profilePath, std::string& dataDir, std::string& apiKey, std::string& bindIp, int& bindPort)
{
    // naive: looks for "key":"value" or "key":number
    auto getStr = [&](const std::string& key)->std::string{
        std::string pat = "\"" + key + "\"";
        size_t k = j.find(pat);
        if (k == std::string::npos) return "";
        size_t colon = j.find(':', k + pat.size());
        if (colon == std::string::npos) return "";
        size_t i = colon + 1;
        while (i < j.size() && std::isspace((unsigned char)j[i])) i++;
        if (i >= j.size() || j[i] != '"') return "";
        i++;
        size_t e = j.find('"', i);
        if (e == std::string::npos) return "";
        return j.substr(i, e - i);
    };

    auto getInt = [&](const std::string& key, int def)->int{
        std::string pat = "\"" + key + "\"";
        size_t k = j.find(pat);
        if (k == std::string::npos) return def;
        size_t colon = j.find(':', k + pat.size());
        if (colon == std::string::npos) return def;
        size_t i = colon + 1;
        while (i < j.size() && std::isspace((unsigned char)j[i])) i++;
        size_t e = i;
        while (e < j.size() && (std::isdigit((unsigned char)j[e]) || j[e]=='-' || j[e]=='+')) e++;
        if (e == i) return def;
        try { return std::stoi(j.substr(i, e-i)); } catch (...) { return def; }
    };

    profilePath = getStr("profilePath");
    dataDir = getStr("dataDir");
    apiKey = getStr("apiKey");
    bindIp = getStr("bindIp");
    bindPort = getInt("bindPort", 8192);

    if (profilePath.empty() || dataDir.empty()) return false;
    if (bindIp.empty()) bindIp = "127.0.0.1";
    return true;
}

static void setOutput(char* output, int outputSize, const std::string& s)
{
    if (outputSize <= 0) return;
    std::string o = s;
    if ((int)o.size() >= outputSize) o.resize(outputSize - 1);
    std::memset(output, 0, (size_t)outputSize);
    std::memcpy(output, o.c_str(), o.size());
}

#ifdef _WIN32
  #define DLL_EXPORT __declspec(dllexport)
#else
  #define DLL_EXPORT __attribute__((visibility("default")))
#endif

extern "C"
{
    DLL_EXPORT void RVExtensionVersion(char* output, int outputSize)
    {
        setOutput(output, outputSize, "ApiBridgeExtension v1.0");
    }

    DLL_EXPORT void RVExtension(char* output, int outputSize, const char* function)
    {
        std::string fn = function ? function : "";
        if (fn.rfind("init ", 0) == 0)
        {
            std::string jsonInit = fn.substr(5);
            std::string profilePath, dataDir, apiKey, bindIp;
            int bindPort = 8192;

            if (!parseInitJson(jsonInit, profilePath, dataDir, apiKey, bindIp, bindPort))
            {
                setOutput(output, outputSize, "ERR bad init json");
                return;
            }

            std::lock_guard<std::mutex> lk(g_mu);

            g_apiKey = apiKey;
            g_bindIp = bindIp;
            g_bindPort = bindPort;

            g_dataDir = fs::path(profilePath) / dataDir;
            fs::create_directories(g_dataDir);

            if (!g_serverStarted)
            {
                g_server.start(g_bindIp, g_bindPort, handle);
                g_serverStarted = true;
                setOutput(output, outputSize, "OK started");
            }
            else
            {
                setOutput(output, outputSize, "OK already started");
            }
            return;
        }

        if (fn == "ping")
        {
            setOutput(output, outputSize, "PONG");
            return;
        }

        if (fn == "shutdown")
        {
            std::lock_guard<std::mutex> lk(g_mu);
            g_server.stop();
            g_serverStarted = false;
            setOutput(output, outputSize, "OK stopped");
            return;
        }

        setOutput(output, outputSize, "ERR unknown");
    }
}
