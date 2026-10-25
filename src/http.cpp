#include "http.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>

#include "exceptions.hpp"
#include "media.hpp"

namespace fs = std::filesystem;

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

// ---------------------------------------------------------------- helpers

std::string urlDecode(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '+') {
            out += ' ';
        } else if (c == '%' && i + 2 < text.size() && std::isxdigit(static_cast<unsigned char>(text[i + 1])) &&
                   std::isxdigit(static_cast<unsigned char>(text[i + 2]))) {
            out += static_cast<char>(std::stoi(text.substr(i + 1, 2), nullptr, 16));
            i += 2;
        } else {
            out += c;
        }
    }
    return out;
}

std::string mimeTypeFor(const std::string& path) {
    static const std::map<std::string, std::string> types = {
        {".html", "text/html; charset=utf-8"}, {".css", "text/css; charset=utf-8"},
        {".js", "text/javascript; charset=utf-8"}, {".json", "application/json"},
        {".svg", "image/svg+xml"}, {".png", "image/png"}, {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"}, {".webp", "image/webp"}, {".ico", "image/x-icon"},
        {".mp3", "audio/mpeg"}, {".m4a", "audio/mp4"}, {".aac", "audio/aac"},
        {".wav", "audio/wav"}, {".flac", "audio/flac"}, {".ogg", "audio/ogg"},
        {".oga", "audio/ogg"}, {".opus", "audio/ogg"}, {".webm", "audio/webm"},
        {".m3u", "audio/x-mpegurl"},
    };
    auto it = types.find(toLower(fs::path(path).extension().string()));
    return it == types.end() ? "application/octet-stream" : it->second;
}

std::string statusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 206: return "Partial Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 413: return "Payload Too Large";
        case 416: return "Range Not Satisfiable";
        case 422: return "Unprocessable Entity";
        case 500: return "Internal Server Error";
        default: return "Status";
    }
}

// ---------------------------------------------------------------- HttpRequest

std::string HttpRequest::header(const std::string& name) const {
    auto it = headers.find(toLower(name));
    return it == headers.end() ? "" : it->second;
}

std::string HttpRequest::queryParam(const std::string& name, const std::string& fallback) const {
    auto it = query.find(name);
    return it == query.end() ? fallback : it->second;
}

const std::string& HttpRequest::param(std::size_t index) const {
    if (index >= params.size()) throw BadRequestException("Missing route parameter");
    return params[index];
}

Json HttpRequest::json() const {
    if (body.empty()) return Json::object();
    Json value = Json::parse(body);
    if (!value.isObject()) throw BadRequestException("Request body must be a JSON object");
    return value;
}

// ---------------------------------------------------------------- HttpResponse

HttpResponse::HttpResponse(int status, std::string contentType, std::string body)
    : status_(status), body_(std::move(body)) {
    setHeader("Content-Type", contentType);
}

HttpResponse HttpResponse::json(const Json& value, int status) {
    HttpResponse r(status, "application/json; charset=utf-8", value.dump());
    r.setHeader("Cache-Control", "no-store");
    return r;
}

HttpResponse HttpResponse::text(const std::string& body, const std::string& contentType, int status) {
    return HttpResponse(status, contentType, body);
}

HttpResponse HttpResponse::file(const std::string& path, const std::string& rangeHeader) {
    std::error_code ec;
    if (!fs::is_regular_file(path, ec)) throw NotFoundException("File not found: " + path);
    const std::uintmax_t size = fs::file_size(path, ec);
    if (ec) throw NotFoundException("Cannot read " + path);

    HttpResponse r(200, mimeTypeFor(path), "");
    r.setHeader("Accept-Ranges", "bytes");
    r.filePath_ = path;
    r.fileOffset_ = 0;
    r.fileLength_ = size;

    if (rangeHeader.rfind("bytes=", 0) == 0 && size > 0) {
        std::string spec = rangeHeader.substr(6);
        auto dash = spec.find('-');
        auto comma = spec.find(',');
        if (dash != std::string::npos && comma == std::string::npos) {
            std::string a = spec.substr(0, dash);
            std::string b = spec.substr(dash + 1);
            std::uintmax_t start = 0;
            std::uintmax_t end = size - 1;
            bool valid = true;
            try {
                if (a.empty()) {
                    std::uintmax_t suffix = std::stoull(b);
                    start = suffix >= size ? 0 : size - suffix;
                } else {
                    start = std::stoull(a);
                    if (!b.empty()) end = std::min<std::uintmax_t>(std::stoull(b), size - 1);
                }
            } catch (const std::exception&) {
                valid = false;
            }
            if (!valid || start > end || start >= size) {
                HttpResponse bad(416, "text/plain; charset=utf-8", "Range not satisfiable");
                bad.setHeader("Content-Range", "bytes */" + std::to_string(size));
                return bad;
            }
            r.status_ = 206;
            r.fileOffset_ = start;
            r.fileLength_ = end - start + 1;
            r.setHeader("Content-Range", "bytes " + std::to_string(start) + "-" + std::to_string(end) +
                                             "/" + std::to_string(size));
        }
    }
    return r;
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    for (auto& h : headers_) {
        if (toLower(h.first) == toLower(name)) {
            h.second = value;
            return;
        }
    }
    headers_.emplace_back(name, value);
}

// ---------------------------------------------------------------- Router

std::vector<std::string> Router::split(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) parts.push_back(urlDecode(current));
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty()) parts.push_back(urlDecode(current));
    return parts;
}

void Router::add(const std::string& method, const std::string& pattern, Handler handler) {
    routes_.push_back({method, split(pattern), std::move(handler)});
}

bool Router::match(const Route& route, const std::vector<std::string>& parts,
                   std::vector<std::string>& params) {
    if (route.segments.size() != parts.size()) return false;
    params.clear();
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const std::string& seg = route.segments[i];
        if (!seg.empty() && seg[0] == ':') params.push_back(parts[i]);
        else if (seg != parts[i]) return false;
    }
    return true;
}

std::optional<HttpResponse> Router::dispatch(HttpRequest& request) const {
    const auto parts = split(request.path);
    const std::string method = request.method == "HEAD" ? "GET" : request.method;
    bool pathMatched = false;
    std::vector<std::string> params;
    for (const auto& route : routes_) {
        if (!match(route, parts, params)) continue;
        pathMatched = true;
        if (route.method != method) continue;
        request.params = params;
        return route.handler(request);
    }
    if (pathMatched) {
        throw MethodNotAllowedException(request.method + " is not allowed on " + request.path);
    }
    return std::nullopt;
}

// ---------------------------------------------------------------- HttpServer

HttpServer::HttpServer(std::string host, int port, Handler handler)
    : host_(std::move(host)), port_(port), handler_(std::move(handler)) {
    if (port_ <= 0 || port_ > 65535) throw BadRequestException("Port must be between 1 and 65535");
}

HttpServer::~HttpServer() {
    if (listenFd_ >= 0) ::close(listenFd_);
}

void HttpServer::run() {
    listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) throw StorageException(std::string("socket() failed: ") + std::strerror(errno));

    int yes = 1;
    ::setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port_));
    if (::inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) != 1) {
        throw BadRequestException("Invalid host address " + host_);
    }
    if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) {
        throw StorageException("Cannot listen on " + host_ + ":" + std::to_string(port_) + " (" +
                               std::strerror(errno) + "). Is another server already running?");
    }
    if (::listen(listenFd_, 64) < 0) throw StorageException(std::string("listen() failed: ") + std::strerror(errno));

    while (true) {
        int client = ::accept(listenFd_, nullptr, nullptr);
        if (client < 0) {
            if (errno == EINTR) continue;
            std::cerr << "accept() failed: " << std::strerror(errno) << '\n';
            continue;
        }
#ifdef SO_NOSIGPIPE
        ::setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof yes);
#endif
        timeval timeout{15, 0};
        ::setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
        std::thread([this, client] { serveConnection(client); }).detach();
    }
}

bool HttpServer::readRequest(int client, HttpRequest& request, std::string& error) {
    std::string data;
    char buffer[8192];
    std::size_t headerEnd = std::string::npos;
    while (headerEnd == std::string::npos) {
        ssize_t n = ::recv(client, buffer, sizeof buffer, 0);
        if (n <= 0) return false;
        data.append(buffer, static_cast<std::size_t>(n));
        headerEnd = data.find("\r\n\r\n");
        if (headerEnd == std::string::npos && data.size() > 64 * 1024) {
            error = "Request headers too large";
            return false;
        }
    }

    std::string head = data.substr(0, headerEnd);
    std::size_t lineEnd = head.find("\r\n");
    std::string requestLine = head.substr(0, lineEnd);
    auto sp1 = requestLine.find(' ');
    auto sp2 = requestLine.find(' ', sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) {
        error = "Malformed request line";
        return false;
    }
    request.method = requestLine.substr(0, sp1);
    std::string target = requestLine.substr(sp1 + 1, sp2 - sp1 - 1);

    auto qmark = target.find('?');
    request.path = urlDecode(target.substr(0, qmark));
    if (qmark != std::string::npos) {
        std::string qs = target.substr(qmark + 1);
        std::size_t start = 0;
        while (start <= qs.size()) {
            std::size_t amp = qs.find('&', start);
            std::string pair = qs.substr(start, amp == std::string::npos ? std::string::npos : amp - start);
            if (!pair.empty()) {
                auto eq = pair.find('=');
                request.query[urlDecode(pair.substr(0, eq))] =
                    eq == std::string::npos ? "" : urlDecode(pair.substr(eq + 1));
            }
            if (amp == std::string::npos) break;
            start = amp + 1;
        }
    }

    std::size_t pos = lineEnd == std::string::npos ? head.size() : lineEnd + 2;
    while (pos < head.size()) {
        std::size_t end = head.find("\r\n", pos);
        if (end == std::string::npos) end = head.size();
        std::string line = head.substr(pos, end - pos);
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string value = line.substr(colon + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            request.headers[toLower(line.substr(0, colon))] = value;
        }
        pos = end + 2;
    }

    std::size_t length = 0;
    std::string lengthHeader = request.header("content-length");
    if (!lengthHeader.empty()) {
        try {
            length = std::stoul(lengthHeader);
        } catch (const std::exception&) {
            error = "Invalid Content-Length";
            return false;
        }
    }
    if (length > kMaxBodyBytes) {
        error = "Request body too large";
        return false;
    }
    request.body = data.substr(headerEnd + 4);
    while (request.body.size() < length) {
        ssize_t n = ::recv(client, buffer, sizeof buffer, 0);
        if (n <= 0) return false;
        request.body.append(buffer, static_cast<std::size_t>(n));
    }
    request.body.resize(length);
    return true;
}

namespace {

bool sendAll(int client, const char* data, std::size_t size) {
    while (size > 0) {
        ssize_t n = ::send(client, data, size, MSG_NOSIGNAL);
        if (n <= 0) return false;
        data += n;
        size -= static_cast<std::size_t>(n);
    }
    return true;
}

}  // namespace

void HttpServer::sendResponse(int client, const HttpResponse& response, bool headOnly) {
    std::uintmax_t length = response.isFile() ? response.fileLength() : response.body().size();
    std::string head = "HTTP/1.1 " + std::to_string(response.status()) + " " + statusText(response.status()) + "\r\n";
    for (const auto& h : response.headers()) head += h.first + ": " + h.second + "\r\n";
    head += "Content-Length: " + std::to_string(length) + "\r\n";
    head += "Connection: close\r\n\r\n";
    if (!sendAll(client, head.data(), head.size()) || headOnly) return;

    if (!response.isFile()) {
        sendAll(client, response.body().data(), response.body().size());
        return;
    }
    std::ifstream in(response.filePath(), std::ios::binary);
    in.seekg(static_cast<std::streamoff>(response.fileOffset()));
    std::vector<char> chunk(64 * 1024);
    std::uintmax_t remaining = response.fileLength();
    while (remaining > 0 && in) {
        auto want = static_cast<std::streamsize>(std::min<std::uintmax_t>(chunk.size(), remaining));
        in.read(chunk.data(), want);
        std::streamsize got = in.gcount();
        if (got <= 0 || !sendAll(client, chunk.data(), static_cast<std::size_t>(got))) break;
        remaining -= static_cast<std::uintmax_t>(got);
    }
}

void HttpServer::serveConnection(int client) const {
    HttpRequest request;
    std::string error;
    if (readRequest(client, request, error)) {
        HttpResponse response;
        try {
            response = handler_(request);
        } catch (const std::exception& e) {
            response = HttpResponse::text(std::string("Internal error: ") + e.what(),
                                          "text/plain; charset=utf-8", 500);
        }
        sendResponse(client, response, request.method == "HEAD");
    } else if (!error.empty()) {
        Json body = Json::object();
        Json err = Json::object();
        err["type"] = "BadRequestException";
        err["message"] = error;
        err["status"] = 400;
        body["error"] = err;
        sendResponse(client, HttpResponse::json(body, error.find("too large") != std::string::npos ? 413 : 400), false);
    }
    ::close(client);
}
