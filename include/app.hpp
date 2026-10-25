#pragma once

#include <mutex>
#include <string>

#include "http.hpp"
#include "library.hpp"
#include "playlist.hpp"
#include "registry.hpp"
#include "scanner.hpp"
#include "storage.hpp"

// Wires the domain classes to HTTP routes. Every AppException thrown below
// becomes a JSON error response with the exception's own status and type.
class PlaylistApp {
public:
    PlaylistApp(const std::string& dataFile, std::string publicDir, std::string samplesDir);

    HttpResponse handle(HttpRequest& request);

    const Library& library() const { return library_; }
    std::size_t playlistCount() const { return playlists_.size(); }

private:
    void registerRoutes();
    void persist();

    std::shared_ptr<Media> buildMedia(const std::string& id, const Json& body) const;
    Json playlistsJson() const;
    HttpResponse serveStatic(const std::string& path) const;
    static HttpResponse errorResponse(const std::string& type, const std::string& message, int status);

    Library library_;
    Registry<Playlist> playlists_;
    Storage storage_;
    FolderScanner scanner_;
    Router router_;
    std::string publicDir_;
    std::mutex mutex_;
};
