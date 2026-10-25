#include "app.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>

#include "exceptions.hpp"
#include "seeder.hpp"

namespace fs = std::filesystem;

namespace {

std::size_t parseIndex(const std::string& text, const char* what) {
    try {
        std::size_t used = 0;
        long long value = std::stoll(text, &used);
        if (used != text.size() || value < 0) throw std::invalid_argument("bad");
        return static_cast<std::size_t>(value);
    } catch (const std::exception&) {
        throw BadRequestException(std::string(what) + " must be a non-negative whole number");
    }
}

std::size_t jsonIndex(const Json& body, const std::string& key) {
    if (!body.has(key)) throw BadRequestException("Field \"" + key + "\" is required");
    long long value = body.getInt(key);
    if (value < 0) throw BadRequestException("Field \"" + key + "\" cannot be negative");
    return static_cast<std::size_t>(value);
}

Json okJson() {
    Json out = Json::object();
    out["ok"] = true;
    return out;
}

}  // namespace

PlaylistApp::PlaylistApp(const std::string& dataFile, std::string publicDir, std::string samplesDir)
    : playlists_("p", "playlist"), storage_(dataFile), publicDir_(std::move(publicDir)) {
    if (storage_.exists()) {
        for (const auto& warning : storage_.load(library_, playlists_)) std::cerr << "warning: " << warning << '\n';
    } else {
        std::cout << "First run: creating demo library and sample audio in " << samplesDir << std::endl;
        Seeder(samplesDir).seed(library_, playlists_);
        persist();
    }
    registerRoutes();
}

void PlaylistApp::persist() { storage_.save(library_, playlists_); }

std::shared_ptr<Media> PlaylistApp::buildMedia(const std::string& id, const Json& body) const {
    Json data = body;
    data["source"] = expandHome(body.getString("source"));
    auto media = MediaFactory::create(id, data);
    if (!media->isRemote() && !media->isPlayable()) {
        throw InvalidMediaException("File not found: " + media->source());
    }
    return media;
}

Json PlaylistApp::playlistsJson() const {
    Json list = Json::array();
    playlists_.forEach([&](const Playlist& p) { list.push(p.summaryJson()); });
    Json out = Json::object();
    out["playlists"] = list;
    return out;
}

HttpResponse PlaylistApp::errorResponse(const std::string& type, const std::string& message, int status) {
    Json err = Json::object();
    err["type"] = type;
    err["message"] = message;
    err["status"] = status;
    Json out = Json::object();
    out["error"] = err;
    return HttpResponse::json(out, status);
}

HttpResponse PlaylistApp::serveStatic(const std::string& path) const {
    std::string relative = path == "/" ? "index.html" : path.substr(1);
    std::error_code ec;
    fs::path root = fs::weakly_canonical(publicDir_, ec);
    fs::path target = fs::weakly_canonical(root / relative, ec);
    auto mismatch = std::mismatch(root.begin(), root.end(), target.begin(), target.end());
    if (ec || mismatch.first != root.end() || !fs::is_regular_file(target, ec)) {
        throw NotFoundException("No page at " + path);
    }
    HttpResponse response = HttpResponse::file(target.string(), "");
    response.setHeader("Cache-Control", "no-cache");
    return response;
}

HttpResponse PlaylistApp::handle(HttpRequest& request) {
    const bool isApi = request.path.rfind("/api/", 0) == 0 || request.path.rfind("/stream/", 0) == 0;
    try {
        if (!isApi) {
            if (request.method != "GET" && request.method != "HEAD") {
                throw MethodNotAllowedException(request.method + " is not allowed on " + request.path);
            }
            return serveStatic(request.path);
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (auto response = router_.dispatch(request)) return *response;
        throw NotFoundException("No API route for " + request.method + " " + request.path);
    } catch (const AppException& e) {
        if (!isApi) {
            return HttpResponse::text(std::string(e.what()), "text/plain; charset=utf-8", e.status());
        }
        return errorResponse(e.type(), e.what(), e.status());
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return errorResponse("InternalError", e.what(), 500);
    }
}

void PlaylistApp::registerRoutes() {
    // ---------------------------------------------------------- media
    router_.add("GET", "/api/media", [this](const HttpRequest& req) {
        auto items = library_.search(req.queryParam("q"), parseFilter(req.queryParam("type")),
                                     parseSortKey(req.queryParam("sort")));
        Json list = Json::array();
        for (const auto& m : items) list.push(m->toViewJson());
        Json out = Json::object();
        out["items"] = list;
        return HttpResponse::json(out);
    });

    router_.add("GET", "/api/media/:id", [this](const HttpRequest& req) {
        return HttpResponse::json(library_.get(req.param(0))->toViewJson());
    });

    router_.add("POST", "/api/media", [this](const HttpRequest& req) {
        auto media = buildMedia(library_.nextId(), req.json());
        library_.add(media);
        persist();
        return HttpResponse::json(media->toViewJson(), 201);
    });

    router_.add("PUT", "/api/media/:id", [this](const HttpRequest& req) {
        auto old = library_.get(req.param(0));
        Json body = req.json();
        auto media = buildMedia(old->id(), body);
        media->setFavorite(old->favorite());
        media->setPlays(old->plays());
        media->setAddedAt(old->addedAt());
        library_.replace(old->id(), media);
        playlists_.forEach([&](Playlist& p) { p.replaceMedia(old->id(), media); });
        persist();
        return HttpResponse::json(media->toViewJson());
    });

    router_.add("DELETE", "/api/media/:id", [this](const HttpRequest& req) {
        const std::string id = req.param(0);
        library_.remove(id);
        playlists_.forEach([&](Playlist& p) { p.removeMedia(id); });
        persist();
        return HttpResponse::json(okJson());
    });

    router_.add("POST", "/api/media/:id/favorite", [this](const HttpRequest& req) {
        auto media = library_.get(req.param(0));
        media->toggleFavorite();
        persist();
        return HttpResponse::json(media->toViewJson());
    });

    router_.add("POST", "/api/media/:id/play", [this](const HttpRequest& req) {
        auto media = library_.get(req.param(0));
        media->recordPlay();
        persist();
        return HttpResponse::json(media->toViewJson());
    });

    router_.add("POST", "/api/media/:id/duration", [this](const HttpRequest& req) {
        auto media = library_.get(req.param(0));
        Json body = req.json();
        if (!body.has("duration")) throw BadRequestException("Field \"duration\" is required");
        media->setDuration(static_cast<int>(body.getInt("duration")));
        persist();
        return HttpResponse::json(media->toViewJson());
    });

    router_.add("POST", "/api/scan", [this](const HttpRequest& req) {
        Json body = req.json();
        auto found = scanner_.scan(body.getString("folder"));
        Json added = Json::array();
        long long skipped = 0;
        for (const Json& item : found) {
            try {
                auto media = MediaFactory::create(library_.nextId(), item);
                library_.add(media);
                added.push(media->toViewJson());
            } catch (const DuplicateException&) {
                ++skipped;
            } catch (const InvalidMediaException&) {
                ++skipped;
            }
        }
        if (added.size() > 0) persist();
        Json out = Json::object();
        out["added"] = static_cast<long long>(added.size());
        out["skipped"] = skipped;
        out["items"] = added;
        return HttpResponse::json(out);
    });

    router_.add("GET", "/api/stats", [this](const HttpRequest&) {
        LibraryStats s = library_.stats();
        Json out = Json::object();
        out["total"] = static_cast<long long>(s.total);
        out["songs"] = static_cast<long long>(s.byType["song"]);
        out["podcasts"] = static_cast<long long>(s.byType["podcast"]);
        out["audiobooks"] = static_cast<long long>(s.byType["audiobook"]);
        out["favorites"] = static_cast<long long>(s.favorites);
        out["totalDuration"] = s.totalDuration;
        out["playlists"] = static_cast<long long>(playlists_.size());
        out["liveMediaObjects"] = Media::liveCount();
        return HttpResponse::json(out);
    });

    router_.add("GET", "/stream/:id", [this](const HttpRequest& req) {
        auto media = library_.get(req.param(0));
        if (media->isRemote()) throw BadRequestException("\"" + media->title() + "\" streams from its own URL");
        if (!media->isPlayable()) throw NotFoundException("Audio file is missing: " + media->source());
        return HttpResponse::file(media->source(), req.header("range"));
    });

    // ---------------------------------------------------------- playlists
    router_.add("GET", "/api/playlists", [this](const HttpRequest&) {
        return HttpResponse::json(playlistsJson());
    });

    router_.add("POST", "/api/playlists", [this](const HttpRequest& req) {
        Json body = req.json();
        Playlist& p = playlists_.add(Playlist(playlists_.nextId(), body.getString("name"),
                                              body.getString("description")));
        persist();
        return HttpResponse::json(p.summaryJson(), 201);
    });

    router_.add("GET", "/api/playlists/:id", [this](const HttpRequest& req) {
        return HttpResponse::json(playlists_.get(req.param(0)).detailJson());
    });

    router_.add("PUT", "/api/playlists/:id", [this](const HttpRequest& req) {
        Playlist& p = playlists_.get(req.param(0));
        Json body = req.json();
        if (body.has("name")) p.rename(body.getString("name"));
        if (body.has("description")) p.setDescription(body.getString("description"));
        persist();
        return HttpResponse::json(p.detailJson());
    });

    router_.add("DELETE", "/api/playlists/:id", [this](const HttpRequest& req) {
        playlists_.remove(req.param(0));
        persist();
        return HttpResponse::json(okJson());
    });

    router_.add("POST", "/api/playlists/:id/duplicate", [this](const HttpRequest& req) {
        const Playlist& original = playlists_.get(req.param(0));
        Playlist copy(original);
        copy.setId(playlists_.nextId());
        std::string name = req.json().getString("name");
        copy.rename(name.empty() ? original.name() + " (copy)" : name);
        Playlist& stored = playlists_.add(copy);
        persist();
        return HttpResponse::json(stored.detailJson(), 201);
    });

    router_.add("POST", "/api/playlists/:id/tracks", [this](const HttpRequest& req) {
        Playlist& p = playlists_.get(req.param(0));
        Json body = req.json();
        std::string mediaId = body.getString("mediaId");
        if (mediaId.empty()) throw BadRequestException("Field \"mediaId\" is required");
        p += library_.get(mediaId);
        persist();
        return HttpResponse::json(p.detailJson());
    });

    router_.add("DELETE", "/api/playlists/:id/tracks/:index", [this](const HttpRequest& req) {
        Playlist& p = playlists_.get(req.param(0));
        p.removeAt(parseIndex(req.param(1), "Track position"));
        persist();
        return HttpResponse::json(p.detailJson());
    });

    router_.add("POST", "/api/playlists/:id/move", [this](const HttpRequest& req) {
        Playlist& p = playlists_.get(req.param(0));
        Json body = req.json();
        p.move(jsonIndex(body, "from"), jsonIndex(body, "to"));
        persist();
        return HttpResponse::json(p.detailJson());
    });

    router_.add("POST", "/api/playlists/:id/shuffle", [this](const HttpRequest& req) {
        Playlist& p = playlists_.get(req.param(0));
        p.shuffle();
        persist();
        return HttpResponse::json(p.detailJson());
    });

    router_.add("GET", "/api/playlists/:id/export.m3u", [this](const HttpRequest& req) {
        const Playlist& p = playlists_.get(req.param(0));
        HttpResponse response = HttpResponse::text(p.toM3u(), "audio/x-mpegurl; charset=utf-8");
        response.setHeader("Content-Disposition", "attachment; filename=\"" + p.fileName() + "\"");
        return response;
    });
}
