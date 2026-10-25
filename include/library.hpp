#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "media.hpp"

enum class MediaFilter { All, Songs, Podcasts, Audiobooks, Favorites };
enum class SortKey { Recent, Title, Creator, Duration, Plays };

MediaFilter parseFilter(const std::string& text);
SortKey parseSortKey(const std::string& text);

struct LibraryStats {
    std::size_t total = 0;
    std::size_t favorites = 0;
    long long totalDuration = 0;
    std::map<std::string, std::size_t> byType;
};

// Owns every Media item. Lookup is O(1) through the hash map; the vector keeps
// the order in which items were added.
class Library {
public:
    Library() = default;

    // Ids are only consumed by add(), so rejected items never leave gaps.
    std::string nextId() const;
    void setNextId(long long next);
    long long peekNextId() const { return nextId_; }

    void add(std::shared_ptr<Media> media);
    void replace(const std::string& id, std::shared_ptr<Media> media);
    void remove(const std::string& id);
    void clear();

    std::shared_ptr<Media> get(const std::string& id) const;
    bool contains(const std::string& id) const;
    bool hasSource(const std::string& source) const;

    std::vector<std::shared_ptr<Media>> all() const;
    std::vector<std::shared_ptr<Media>> search(const std::string& query,
                                                MediaFilter filter = MediaFilter::All,
                                                SortKey sort = SortKey::Recent) const;
    LibraryStats stats() const;
    std::size_t size() const { return items_.size(); }

private:
    void reserveId(const std::string& id);

    std::unordered_map<std::string, std::shared_ptr<Media>> items_;
    std::vector<std::string> order_;
    long long nextId_ = 1;
};
