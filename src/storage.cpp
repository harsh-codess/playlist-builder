#include "storage.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "exceptions.hpp"

namespace fs = std::filesystem;

Storage::Storage(std::string path) : path_(std::move(path)) {
    if (path_.empty()) throw StorageException("Storage path cannot be empty");
}

bool Storage::exists() const {
    std::error_code ec;
    return fs::is_regular_file(path_, ec);
}

std::vector<std::string> Storage::load(Library& library, Registry<Playlist>& playlists) const {
    std::ifstream in(path_);
    if (!in) throw StorageException("Cannot open " + path_ + " for reading");
    std::stringstream buffer;
    buffer << in.rdbuf();

    Json root;
    try {
        root = Json::parse(buffer.str());
    } catch (const AppException& e) {
        throw StorageException(path_ + " is corrupted: " + e.what());
    }

    std::vector<std::string> warnings;
    library.clear();
    playlists.clear();

    const Json& media = root.get("media");
    if (media.isArray()) {
        for (const Json& item : media.asArray()) {
            try {
                library.add(MediaFactory::create(item.getString("id"), item));
            } catch (const AppException& e) {
                warnings.push_back("Skipped media \"" + item.getString("title", "?") + "\": " + e.what());
            }
        }
    }

    const Json& lists = root.get("playlists");
    if (lists.isArray()) {
        for (const Json& item : lists.asArray()) {
            try {
                Playlist playlist(item.getString("id"), item.getString("name"),
                                  item.getString("description"));
                const Json& tracks = item.get("tracks");
                if (tracks.isArray()) {
                    for (const Json& trackId : tracks.asArray()) {
                        if (trackId.isString() && library.contains(trackId.asString())) {
                            playlist += library.get(trackId.asString());
                        }
                    }
                }
                playlists.add(playlist);
            } catch (const AppException& e) {
                warnings.push_back("Skipped playlist \"" + item.getString("name", "?") + "\": " + e.what());
            }
        }
    }

    library.setNextId(root.getInt("nextMediaId", static_cast<long long>(library.size()) + 1));
    playlists.setNextId(root.getInt("nextPlaylistId", static_cast<long long>(playlists.size()) + 1));
    return warnings;
}

void Storage::save(const Library& library, const Registry<Playlist>& playlists) const {
    Json root = Json::object();
    root["version"] = 1;
    root["nextMediaId"] = library.peekNextId();
    root["nextPlaylistId"] = playlists.peekNextId();

    Json media = Json::array();
    for (const auto& item : library.all()) media.push(item->toJson());
    root["media"] = media;

    Json lists = Json::array();
    playlists.forEach([&](const Playlist& p) {
        Json entry = Json::object();
        entry["id"] = p.id();
        entry["name"] = p.name();
        entry["description"] = p.description();
        Json ids = Json::array();
        for (const auto& track : p.tracks()) ids.push(track->id());
        entry["tracks"] = ids;
        lists.push(entry);
    });
    root["playlists"] = lists;

    std::error_code ec;
    fs::path target(path_);
    if (target.has_parent_path()) fs::create_directories(target.parent_path(), ec);

    // Write to a temp file and rename so a crash never leaves a half-written save.
    const std::string temp = path_ + ".tmp";
    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out) throw StorageException("Cannot write " + temp);
        out << root.dump(2) << '\n';
        if (!out) throw StorageException("Failed while writing " + temp);
    }
    fs::rename(temp, target, ec);
    if (ec) throw StorageException("Cannot replace " + path_ + ": " + ec.message());
}
