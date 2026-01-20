#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <sstream>
#include <cctype>

#ifdef _WIN32
  #define NOMINMAX
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  using socket_t = SOCKET;
  static constexpr socket_t INVALID_SOCKET_T = INVALID_SOCKET;
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  using socket_t = int;
  static constexpr socket_t INVALID_SOCKET_T = -1;
#endif

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string query;
    std::string body;
    std::vector<std::pair<std::string,std::string>> headers;

    std::string header(const std::string& key) const
    {
        for (const auto& kv : headers)
        {
            if (kv.first == key) return kv.second;
        }
        return "";
    }
};

struct HttpResponse
{
    int status = 200;
    std::string contentType = "application/json; charset=utf-8";
    std::string body;
};

// Minimal single-threaded HTTP/1.1 server (close-connection), good enough for LAN + local usage.
class MiniHttpServer
{
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    MiniHttpServer() = default;
    ~MiniHttpServer() { stop(); }

    bool start(const std::string& bindIp, int port, Handler handler)
    {
        if (running_) return true;
        handler_ = std::move(handler);
        running_ = true;
        worker_ = std::thread([this, bindIp, port](){ this->run(bindIp, port); });
        return true;
    }

    void stop()
    {
        running_ = false;
        if (listenSock_ != INVALID_SOCKET_T)
        {
#ifdef _WIN32
            closesocket(listenSock_);
#else
            close(listenSock_);
#endif
            listenSock_ = INVALID_SOCKET_T;
        }
        if (worker_.joinable()) worker_.join();
#ifdef _WIN32
        if (wsaInited_) WSACleanup();
        wsaInited_ = false;
#endif
    }

private:
    std::atomic<bool> running_{false};
    std::thread worker_;
    socket_t listenSock_ = INVALID_SOCKET_T;
    Handler handler_;

#ifdef _WIN32
    bool wsaInited_ = false;
#endif

    static std::string toLower(std::string s)
    {
        for (char& c : s) c = (char)std::tolower((unsigned char)c);
        return s;
    }

    static void trim(std::string& s)
    {
        while (!s.empty() && (s.back()=='\r' || s.back()=='\n' || s.back()==' ' || s.back()=='\t')) s.pop_back();
        size_t i = 0;
        while (i < s.size() && (s[i]==' ' || s[i]=='\t')) i++;
        if (i>0) s = s.substr(i);
    }

    static bool recvAll(socket_t s, std::string& out, size_t want)
    {
        while (out.size() < want)
        {
            char buf[4096];
            int need = (int)std::min<size_t>(sizeof(buf), want - out.size());
#ifdef _WIN32
            int n = recv(s, buf, need, 0);
#else
            int n = (int)recv(s, buf, need, 0);
#endif
            if (n <= 0) return false;
            out.append(buf, buf + n);
        }
        return true;
    }

    static bool recvUntilHeaders(socket_t s, std::string& out)
    {
        // read until \r\n\r\n or cap
        while (out.find("\r\n\r\n") == std::string::npos)
        {
            char buf[4096];
#ifdef _WIN32
            int n = recv(s, buf, (int)sizeof(buf), 0);
#else
            int n = (int)recv(s, buf, sizeof(buf), 0);
#endif
            if (n <= 0) return false;
            out.append(buf, buf + n);
            if (out.size() > 1024 * 1024) return false; // 1MB cap
        }
        return true;
    }

    static HttpRequest parseRequest(const std::string& raw)
    {
        HttpRequest req;

        size_t headerEnd = raw.find("\r\n\r\n");
        std::string head = raw.substr(0, headerEnd);
        std::string body = raw.substr(headerEnd + 4);

        std::istringstream iss(head);
        std::string line;
        if (std::getline(iss, line))
        {
            trim(line);
            std::istringstream rls(line);
            rls >> req.method;
            std::string fullPath;
            rls >> fullPath;

            auto qpos = fullPath.find('?');
            if (qpos != std::string::npos)
            {
                req.path = fullPath.substr(0, qpos);
                req.query = fullPath.substr(qpos + 1);
            }
            else
            {
                req.path = fullPath;
                req.query = "";
            }
        }

        while (std::getline(iss, line))
        {
            trim(line);
            if (line.empty()) continue;
            auto cpos = line.find(':');
            if (cpos == std::string::npos) continue;
            std::string k = line.substr(0, cpos);
            std::string v = line.substr(cpos + 1);
            trim(k); trim(v);
            k = toLower(k);
            req.headers.push_back({k, v});
        }

        req.body = body;
        return req;
    }

    static std::string makeResponse(const HttpResponse& r)
    {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << r.status << " " << (r.status==200?"OK":"ERR") << "\r\n";
        oss << "Content-Type: " << r.contentType << "\r\n";
        oss << "Content-Length: " << r.body.size() << "\r\n";
        oss << "Connection: close\r\n";
        oss << "\r\n";
        oss << r.body;
        return oss.str();
    }

    void run(const std::string& bindIp, int port)
    {
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
        {
            running_ = false;
            return;
        }
        wsaInited_ = true;
#endif

        listenSock_ = (socket_t)socket(AF_INET, SOCK_STREAM, 0);
        if (listenSock_ == INVALID_SOCKET_T)
        {
            running_ = false;
            return;
        }

        int yes = 1;
#ifdef _WIN32
        setsockopt(listenSock_, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
#else
        setsockopt(listenSock_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons((uint16_t)port);
#ifdef _WIN32
        inet_pton(AF_INET, bindIp.c_str(), &addr.sin_addr);
#else
        addr.sin_addr.s_addr = inet_addr(bindIp.c_str());
#endif

        if (bind(listenSock_, (sockaddr*)&addr, sizeof(addr)) < 0)
        {
            running_ = false;
            return;
        }

        if (listen(listenSock_, 16) < 0)
        {
            running_ = false;
            return;
        }

        while (running_)
        {
            sockaddr_in client{};
#ifdef _WIN32
            int clen = sizeof(client);
            socket_t s = accept(listenSock_, (sockaddr*)&client, &clen);
#else
            socklen_t clen = sizeof(client);
            socket_t s = accept(listenSock_, (sockaddr*)&client, &clen);
#endif
            if (s == INVALID_SOCKET_T)
            {
                if (!running_) break;
                continue;
            }

            // handle inline (single-thread). If you want more throughput, spawn a thread here.
            std::string raw;
            if (!recvUntilHeaders(s, raw))
            {
#ifdef _WIN32
                closesocket(s);
#else
                close(s);
#endif
                continue;
            }

            // content-length?
            size_t headerEnd = raw.find("\r\n\r\n");
            size_t bodyHave = raw.size() - (headerEnd + 4);
            size_t contentLen = 0;
            {
                std::string head = raw.substr(0, headerEnd);
                std::istringstream iss(head);
                std::string line;
                while (std::getline(iss, line))
                {
                    trim(line);
                    auto cpos = line.find(':');
                    if (cpos == std::string::npos) continue;
                    std::string k = line.substr(0, cpos);
                    std::string v = line.substr(cpos + 1);
                    trim(k); trim(v);
                    k = toLower(k);
                    if (k == "content-length")
                    {
                        contentLen = (size_t)std::stoul(v);
                    }
                }
            }

            if (contentLen > bodyHave)
            {
                size_t want = (headerEnd + 4) + contentLen;
                if (!recvAll(s, raw, want))
                {
#ifdef _WIN32
                    closesocket(s);
#else
                    close(s);
#endif
                    continue;
                }
            }

            HttpRequest req = parseRequest(raw);
            HttpResponse resp;
            if (handler_) resp = handler_(req);
            else { resp.status = 500; resp.body = "{\"ok\":false,\"error\":\"no handler\"}"; }

            std::string out = makeResponse(resp);
#ifdef _WIN32
            send(s, out.c_str(), (int)out.size(), 0);
            closesocket(s);
#else
            send(s, out.c_str(), out.size(), 0);
            close(s);
#endif
        }
    }
};
