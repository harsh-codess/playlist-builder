#pragma once

#include <memory>
#include <string>
#include <vector>

#include "json.hpp"
#include "media.hpp"

// An ordered list of shared Media items. The same item may appear twice.
class Playlist {
public:
    Playlist(std::string id, std::string name, std::string description = "");
    Playlist(const Playlist& other);
    Playlist& operator=(const Playlist& other) = default;

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& description() const { return description_; }
    const std::vector<std::shared_ptr<Media>>& tracks() const { return tracks_; }

    void setId(std::string id);
    void rename(const std::string& name);
    void setDescription(const std::string& description);

    void add(std::shared_ptr<Media> media);
    void removeAt(std::size_t index);
    std::size_t removeMedia(const std::string& mediaId);
    void replaceMedia(const std::string& mediaId, const std::shared_ptr<Media>& media);
    void move(std::size_t from, std::size_t to);
    void shuffle();

    std::size_t size() const { return tracks_.size(); }
    bool empty() const { return tracks_.empty(); }
    long long totalDuration() const;

    std::string toM3u() const;
    std::string fileName() const;
    Json summaryJson() const;
    Json detailJson() const;

    Playlist& operator+=(std::shared_ptr<Media> media);
    const Media& operator[](std::size_t index) const;

    static constexpr std::size_t kMaxTracks = 1000;

private:
    void checkIndex(std::size_t index, const char* what) const;

    std::string id_;
    std::string name_;
    std::string description_;
    std::vector<std::shared_ptr<Media>> tracks_;
};
