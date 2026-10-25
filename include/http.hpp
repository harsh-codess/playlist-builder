#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "json.hpp"

struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> query;
    std::map<std::string, std::string> headers;  // keys lower-cased
    std::string body;
    std::vector<std::string> params;              // values captured by ":name" route segments

    std::string header(const std::string& name) const;
    std::string queryParam(const std::string& name, const std::string& fallback = "") const;
    const std::string& param(std::size_t index) const;
    Json json() const;
};

class HttpResponse {
public:
    HttpResponse() = default;
    HttpResponse(int status, std::string contentType, std::string body);

    static HttpResponse json(const Json& value, int status = 200);
    static HttpResponse text(const std::string& body, const std::string& contentType = "text/plain; charset=utf-8",
                             int status = 200);
    // Streams a file from disk, honouring an optional "Range: bytes=a-b" header.
    static HttpResponse file(const std::string& path, const std::string& rangeHeader);

    void setHeader(const std::string& name, const std::string& value);

    int status() const { return status_; }
    const std::string& body() const { return body_; }
    const std::vector<std::pair<std::string, std::string>>& headers() const { return headers_; }
    bool isFile() const { return !filePath_.empty(); }
    const std::string& filePath() const { return filePath_; }
    std::uintmax_t fileOffset() const { return fileOffset_; }
    std::uintmax_t fileLength() const { return fileLength_; }

private:
    int status_ = 200;
    std::vector<std::pair<std::string, std::string>> headers_;
    std::string body_;
    std::string filePath_;
    std::uintmax_t fileOffset_ = 0;
    std::uintmax_t fileLength_ = 0;
};

using Handler = std::function<HttpResponse(const HttpRequest&)>;

class Router {
public:
    void add(const std::string& method, const std::string& pattern, Handler handler);
    // Returns nothing when no route pattern matches the path. Throws
    // MethodNotAllowedException when a pattern matches but the method does not.
    std::optional<HttpResponse> dispatch(HttpRequest& request) const;

private:
    struct Route {
        std::string method;
        std::vector<std::string> segments;
        Handler handler;
    };
    static std::vector<std::string> split(const std::string& path);
    static bool match(const Route& route, const std::vector<std::string>& parts,
                      std::vector<std::string>& params);

    std::vector<Route> routes_;
};

class HttpServer {
public:
    HttpServer(std::string host, int port, Handler handler);
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;
    ~HttpServer();

    void run();

    static constexpr std::size_t kMaxBodyBytes = 1 << 20;

private:
    void serveConnection(int client) const;
    static bool readRequest(int client, HttpRequest& request, std::string& error);
    static void sendResponse(int client, const HttpResponse& response, bool headOnly);

    std::string host_;
    int port_;
    Handler handler_;
    int listenFd_ = -1;
};

std::string urlDecode(const std::string& text);
std::string mimeTypeFor(const std::string& path);
std::string statusText(int status);
