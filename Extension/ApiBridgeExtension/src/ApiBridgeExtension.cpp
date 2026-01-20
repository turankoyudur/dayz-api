// ApiBridgeExtension.cpp
// Minimal DayZ extension that exposes a tiny HTTP REST API.
// Build on Windows: cmake .. -G "Visual Studio 17 2022" -A x64 && cmake --build . --config Release

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

static std::atomic_bool g_running{false};
static std::thread g_thread;
static std::mutex g_mu;

static std::string g_profilePath;
static std::string g_bindIp = "127.0.0.1";
static int g_port = 8192;
static std::string g_apiKey;

static std::string join_path(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    char last = a.back();
    if (last == '\\' || last == '/') return a + b;
    return a + "\\" + b;
}

static std::string read_file(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static bool write_file(const std::string& p, const std::string& data) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(data.data(), (std::streamsize)data.size());
    return true;
}

static std::string json_escape(const std::string& s) {
    std::string o;
    o.reserve(s.size()+8);
    for (char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if ((unsigned char)c < 0x20) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                    o += buf;
                } else {
                    o += c;
                }
        }
    }
    return o;
}

static std::string make_id() {
    using namespace std::chrono;
    auto ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    std::ostringstream ss;
    ss << ms;
    return ss.str();
}

static bool header_has_api_key(const std::string& headers) {
    if (g_apiKey.empty()) return true;
    std::string h = headers;
    // naive lowercase compare for x-api-key
    for (auto& ch : h) ch = (char)tolower((unsigned char)ch);
    std::string needle = "x-api-key:";
    size_t pos = h.find(needle);
    if (pos == std::string::npos) return false;
    size_t end = h.find('\n', pos);
    std::string line = h.substr(pos, end == std::string::npos ? h.size()-pos : end-pos);
    // original headers are case sensitive; extract from original using same slice
    size_t pos2 = headers.find("x-api-key:");
    if (pos2 == std::string::npos) pos2 = headers.find("X-Api-Key:");
    if (pos2 == std::string::npos) pos2 = headers.find("X-API-KEY:");
    if (pos2 == std::string::npos) return false;
    size_t end2 = headers.find('\n', pos2);
    std::string orig = headers.substr(pos2, end2 == std::string::npos ? headers.size()-pos2 : end2-pos2);
    size_t colon = orig.find(':');
    std::string value = (colon==std::string::npos) ? "" : orig.substr(colon+1);
    // trim spaces
    while (!value.empty() && (value.front()==' ' || value.front()=='\t')) value.erase(value.begin());
    while (!value.empty() && (value.back()=='\r' || value.back()==' ' || value.back()=='\t')) value.pop_back();
    return value == g_apiKey;
}

static std::string http_response(int code, const std::string& body, const std::string& contentType="application/json") {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << code << " " << (code==200?"OK":(code==401?"Unauthorized":"Error")) << "\r\n";
    ss << "Content-Type: " << contentType << "\r\n";
    ss << "Content-Length: " << body.size() << "\r\n";
    ss << "Connection: close\r\n\r\n";
    ss << body;
    return ss.str();
}

static std::string state_path() {
    return join_path(join_path(g_profilePath, "ApiBridge"), "state.json");
}
static std::string responses_path() {
    return join_path(join_path(g_profilePath, "ApiBridge"), "responses.json");
}
static std::string commands_path() {
    return join_path(join_path(g_profilePath, "ApiBridge"), "commands.json");
}

static bool append_command(const std::string& cmdObjJson) {
    std::lock_guard<std::mutex> lk(g_mu);
    std::string cur = read_file(commands_path());
    if (cur.empty()) {
        std::string doc = std::string("{\"commands\":[") + cmdObjJson + "]}";
        return write_file(commands_path(), doc);
    }
    // find last ']' of commands array
    size_t close = cur.rfind(']');
    if (close == std::string::npos) {
        std::string doc = std::string("{\"commands\":[") + cmdObjJson + "]}";
        return write_file(commands_path(), doc);
    }
    // check if array is empty
    size_t open = cur.find('[');
    bool empty = (open != std::string::npos && close == open+1);
    std::string insert = (empty ? cmdObjJson : (std::string(",") + cmdObjJson));
    cur.insert(close, insert);
    return write_file(commands_path(), cur);
}

static void handle_get(const std::string& path, std::string& outBody, int& outCode, std::string& outType) {
    if (path == "/v1/health" || path == "/health") {
        outCode = 200;
        outBody = "{\"ok\":true}";
        return;
    }
    if (path == "/v1/status") {
        outCode = 200;
        std::ostringstream ss;
        ss << "{\"ok\":true,\"bind\":\"" << json_escape(g_bindIp) << "\",\"port\":" << g_port << "}";
        outBody = ss.str();
        return;
    }
    if (path == "/v1/state") {
        std::string data = read_file(state_path());
        outCode = data.empty() ? 404 : 200;
        outType = "application/json";
        outBody = data.empty() ? "{}" : data;
        return;
    }
    if (path == "/v1/responses") {
        std::string data = read_file(responses_path());
        outCode = data.empty() ? 404 : 200;
        outType = "application/json";
        outBody = data.empty() ? "{}" : data;
        return;
    }
    outCode = 404;
    outBody = "{\"error\":\"not_found\"}";
}

static bool parse_json_field(const std::string& body, const std::string& key, std::string& val) {
    // ultra naive: "key" : "value"
    std::string needle = "\"" + key + "\"";
    size_t p = body.find(needle);
    if (p == std::string::npos) return false;
    p = body.find(':', p);
    if (p == std::string::npos) return false;
    p++;
    while (p < body.size() && (body[p] == ' ' || body[p] == '\t' || body[p]=='\n' || body[p]=='\r')) p++;
    if (p >= body.size()) return false;
    if (body[p] == '"') {
        size_t e = body.find('"', p+1);
        if (e == std::string::npos) return false;
        val = body.substr(p+1, e-(p+1));
        return true;
    }
    // number
    size_t e = p;
    while (e < body.size() && (isdigit((unsigned char)body[e]) || body[e]=='.' || body[e]=='-')) e++;
    val = body.substr(p, e-p);
    return true;
}

static void handle_post(const std::string& path, const std::string& body, std::string& outBody, int& outCode) {
    // routes:
    // /v1/players/<uid>/teleport
    // /v1/players/<uid>/inventory/add
    // /v1/players/<uid>/inventory/remove
    // /v1/players/<uid>/inventory/setQuantity
    // /v1/players/<uid>/stats

    auto starts_with = [](const std::string& s, const std::string& p){ return s.rfind(p,0)==0; };
    if (!starts_with(path, "/v1/players/")) {
        outCode = 404;
        outBody = "{\"error\":\"not_found\"}";
        return;
    }

    std::string rest = path.substr(std::string("/v1/players/").size());
    size_t slash = rest.find('/');
    if (slash == std::string::npos) {
        outCode = 404;
        outBody = "{\"error\":\"bad_path\"}";
        return;
    }

    std::string uid = rest.substr(0, slash);
    std::string action = rest.substr(slash); // begins with /

    std::string id = make_id();
    std::ostringstream cmd;

    if (action == "/teleport") {
        std::string sx, sy, sz;
        parse_json_field(body, "x", sx);
        parse_json_field(body, "y", sy);
        parse_json_field(body, "z", sz);
        cmd << "{\"id\":\"" << id << "\",\"type\":\"teleport\",\"uid\":\"" << json_escape(uid) << "\",\"x\":" << (sx.empty()?"0":sx) << ",\"y\":" << (sy.empty()?"0":sy) << ",\"z\":" << (sz.empty()?"0":sz) << "}";
    } else if (action == "/inventory/add") {
        std::string it, q;
        parse_json_field(body, "type", it);
        parse_json_field(body, "quantity", q);
        cmd << "{\"id\":\"" << id << "\",\"type\":\"inv_add\",\"uid\":\"" << json_escape(uid) << "\",\"itemType\":\"" << json_escape(it) << "\",\"quantity\":" << (q.empty()?"0":q) << "}";
    } else if (action == "/inventory/remove") {
        std::string it, c;
        parse_json_field(body, "type", it);
        parse_json_field(body, "count", c);
        cmd << "{\"id\":\"" << id << "\",\"type\":\"inv_remove\",\"uid\":\"" << json_escape(uid) << "\",\"itemType\":\"" << json_escape(it) << "\",\"count\":" << (c.empty()?"1":c) << "}";
    } else if (action == "/inventory/setQuantity") {
        std::string it, q;
        parse_json_field(body, "type", it);
        parse_json_field(body, "quantity", q);
        cmd << "{\"id\":\"" << id << "\",\"type\":\"inv_setqty\",\"uid\":\"" << json_escape(uid) << "\",\"itemType\":\"" << json_escape(it) << "\",\"quantity\":" << (q.empty()?"0":q) << "}";
    } else if (action == "/stats") {
        std::string h,b,w,e;
        parse_json_field(body, "health", h);
        parse_json_field(body, "blood", b);
        parse_json_field(body, "water", w);
        parse_json_field(body, "energy", e);
        cmd << "{\"id\":\"" << id << "\",\"type\":\"stats_set\",\"uid\":\"" << json_escape(uid) << "\"";
        if (!h.empty()) cmd << ",\"health\":" << h;
        if (!b.empty()) cmd << ",\"blood\":" << b;
        if (!w.empty()) cmd << ",\"water\":" << w;
        if (!e.empty()) cmd << ",\"energy\":" << e;
        cmd << "}";
    } else {
        outCode = 404;
        outBody = "{\"error\":\"unknown_action\"}";
        return;
    }

    if (!append_command(cmd.str())) {
        outCode = 500;
        outBody = "{\"ok\":false,\"error\":\"write_failed\"}";
        return;
    }

    outCode = 200;
    outBody = std::string("{\"ok\":true,\"accepted\":true,\"id\":\"") + id + "\"}";
}

static void server_loop() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        return;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)g_port);
    inet_pton(AF_INET, g_bindIp.c_str(), &addr.sin_addr);

    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    if (bind(listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(listenSock);
        WSACleanup();
        return;
    }
    if (listen(listenSock, 16) == SOCKET_ERROR) {
        closesocket(listenSock);
        WSACleanup();
        return;
    }

    while (g_running.load()) {
        SOCKET client = accept(listenSock, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        std::string req;
        char buf[4096];
        int n;
        while ((n = recv(client, buf, sizeof(buf), 0)) > 0) {
            req.append(buf, buf+n);
            if (req.find("\r\n\r\n") != std::string::npos) break;
        }

        // parse request line
        size_t lineEnd = req.find("\r\n");
        std::string first = (lineEnd==std::string::npos)?req:req.substr(0,lineEnd);
        std::istringstream ls(first);
        std::string method, path, ver;
        ls >> method >> path >> ver;

        // headers + body
        size_t headerEnd = req.find("\r\n\r\n");
        std::string headers = (headerEnd==std::string::npos)?"":req.substr(0, headerEnd);
        std::string body = (headerEnd==std::string::npos)?"":req.substr(headerEnd+4);

        if (!header_has_api_key(headers)) {
            std::string resp = http_response(401, "{\"ok\":false,\"error\":\"bad_api_key\"}");
            send(client, resp.data(), (int)resp.size(), 0);
            closesocket(client);
            continue;
        }

        int code = 200;
        std::string respBody;
        std::string type = "application/json";

        if (method == "GET") {
            handle_get(path, respBody, code, type);
        } else if (method == "POST") {
            handle_post(path, body, respBody, code);
        } else {
            code = 405;
            respBody = "{\"error\":\"method_not_allowed\"}";
        }

        std::string resp = http_response(code, respBody, type);
        send(client, resp.data(), (int)resp.size(), 0);
        closesocket(client);
    }

    closesocket(listenSock);
    WSACleanup();
#endif
}

static std::vector<std::string> split_pipe(const std::string& s) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c: s) {
        if (c == '|') { parts.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    parts.push_back(cur);
    return parts;
}

extern "C" {

__declspec(dllexport) void RVExtensionVersion(char* output, int outputSize) {
    const char* v = "ApiBridgeExtension 1.0";
    std::snprintf(output, outputSize, "%s", v);
}

__declspec(dllexport) void RVExtension(char* output, int outputSize, const char* function) {
    std::string fn = function ? function : "";

    if (fn.rfind("init|", 0) == 0) {
        auto parts = split_pipe(fn);
        // parts[0] = init, [1]=profilePath, [2]=bind, [3]=port, [4]=key
        if (parts.size() >= 5) {
            std::lock_guard<std::mutex> lk(g_mu);
            g_profilePath = parts[1];
            g_bindIp = parts[2];
            g_port = std::atoi(parts[3].c_str());
            g_apiKey = parts[4];
        }
        if (!g_running.exchange(true)) {
            g_thread = std::thread(server_loop);
        }
        std::snprintf(output, outputSize, "ok");
        return;
    }

    if (fn == "shutdown") {
        if (g_running.exchange(false)) {
            // connect once to unblock accept (best effort)
#ifdef _WIN32
            SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (s != INVALID_SOCKET) {
                sockaddr_in addr{};
                addr.sin_family = AF_INET;
                addr.sin_port = htons((u_short)g_port);
                inet_pton(AF_INET, g_bindIp.c_str(), &addr.sin_addr);
                connect(s, (sockaddr*)&addr, sizeof(addr));
                closesocket(s);
            }
#endif
            if (g_thread.joinable()) g_thread.join();
        }
        std::snprintf(output, outputSize, "ok");
        return;
    }

    // default
    std::snprintf(output, outputSize, "unknown");
}

}
