#pragma once

#include <string>
#include <vector>

#include "library.hpp"
#include "playlist.hpp"
#include "registry.hpp"

// Saves and restores the whole library + playlists as one JSON file.
class Storage {
public:
    explicit Storage(std::string path);

    const std::string& path() const { return path_; }
    bool exists() const;

    // Returns warnings for entries that were skipped because they were invalid.
    std::vector<std::string> load(Library& library, Registry<Playlist>& playlists) const;
    void save(const Library& library, const Registry<Playlist>& playlists) const;

private:
    std::string path_;
};
