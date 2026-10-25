#include "playlist.hpp"

#include <algorithm>
#include <cctype>
#include <numeric>
#include <random>

#include "exceptions.hpp"

namespace {

std::string cleanName(const std::string& raw) {
    auto begin = std::find_if_not(raw.begin(), raw.end(), [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(raw.rbegin(), raw.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    std::string name = begin < end ? std::string(begin, end) : std::string();
    if (name.empty()) throw BadRequestException("Playlist name cannot be empty");
    if (name.size() > 60) throw BadRequestException("Playlist name is too long (max 60 characters)");
    return name;
}

}  // namespace

Playlist::Playlist(std::string id, std::string name, std::string description)
    : id_(std::move(id)), name_(cleanName(name)) {
    if (id_.empty()) throw BadRequestException("Playlist id cannot be empty");
    setDescription(description);
}

Playlist::Playlist(const Playlist& other)
    : id_(other.id_),
      name_(other.name_),
      description_(other.description_),
      tracks_(other.tracks_) {}

void Playlist::setId(std::string id) {
    if (id.empty()) throw BadRequestException("Playlist id cannot be empty");
    id_ = std::move(id);
}

void Playlist::rename(const std::string& name) { name_ = cleanName(name); }

void Playlist::setDescription(const std::string& description) {
    if (description.size() > 240) {
        throw BadRequestException("Description is too long (max 240 characters)");
    }
    description_ = description;
}

void Playlist::add(std::shared_ptr<Media> media) {
    if (!media) throw NotFoundException("Cannot add a missing media item");
    if (tracks_.size() >= kMaxTracks) {
        throw BadRequestException("Playlist is full (max " + std::to_string(kMaxTracks) + " tracks)");
    }
    tracks_.push_back(std::move(media));
}

void Playlist::checkIndex(std::size_t index, const char* what) const {
    if (index >= tracks_.size()) {
        throw BadRequestException(std::string(what) + " position " + std::to_string(index) +
                                  " is out of range (playlist has " +
                                  std::to_string(tracks_.size()) + " tracks)");
    }
}

void Playlist::removeAt(std::size_t index) {
    checkIndex(index, "Track");
    tracks_.erase(tracks_.begin() + static_cast<std::ptrdiff_t>(index));
}

std::size_t Playlist::removeMedia(const std::string& mediaId) {
    auto before = tracks_.size();
    tracks_.erase(std::remove_if(tracks_.begin(), tracks_.end(),
                                 [&](const auto& m) { return m->id() == mediaId; }),
                  tracks_.end());
    return before - tracks_.size();
}

void Playlist::replaceMedia(const std::string& mediaId, const std::shared_ptr<Media>& media) {
    std::replace_if(tracks_.begin(), tracks_.end(),
                    [&](const auto& m) { return m->id() == mediaId; }, media);
}

void Playlist::move(std::size_t from, std::size_t to) {
    checkIndex(from, "Source");
    checkIndex(to, "Target");
    auto first = tracks_.begin();
    if (from < to) {
        std::rotate(first + static_cast<std::ptrdiff_t>(from),
                    first + static_cast<std::ptrdiff_t>(from) + 1,
                    first + static_cast<std::ptrdiff_t>(to) + 1);
    } else if (from > to) {
        std::rotate(first + static_cast<std::ptrdiff_t>(to),
                    first + static_cast<std::ptrdiff_t>(from),
                    first + static_cast<std::ptrdiff_t>(from) + 1);
    }
}

void Playlist::shuffle() {
    static std::mt19937 rng(std::random_device{}());
    std::shuffle(tracks_.begin(), tracks_.end(), rng);
}

long long Playlist::totalDuration() const {
    return std::accumulate(tracks_.begin(), tracks_.end(), 0LL,
                           [](long long sum, const auto& m) { return sum + m->duration(); });
}

std::string Playlist::toM3u() const {
    if (tracks_.empty()) throw EmptyPlaylistException(name_);
    std::string out = "#EXTM3U\n#PLAYLIST:" + name_ + "\n";
    for (const auto& media : tracks_) out += media->toM3uEntry();
    return out;
}

std::string Playlist::fileName() const {
    std::string safe;
    for (unsigned char c : name_) {
        if (std::isalnum(c) || c == '-' || c == '_') safe += static_cast<char>(c);
        else if (c == ' ' && !safe.empty() && safe.back() != '-') safe += '-';
    }
    while (!safe.empty() && safe.back() == '-') safe.pop_back();
    return (safe.empty() ? "playlist" : safe) + ".m3u";
}

Json Playlist::summaryJson() const {
    Json out = Json::object();
    out["id"] = id_;
    out["name"] = name_;
    out["description"] = description_;
    out["count"] = static_cast<long long>(tracks_.size());
    out["duration"] = totalDuration();
    Json ids = Json::array();
    for (const auto& m : tracks_) ids.push(m->id());
    out["trackIds"] = ids;
    return out;
}

Json Playlist::detailJson() const {
    Json out = summaryJson();
    Json list = Json::array();
    for (const auto& m : tracks_) list.push(m->toViewJson());
    out["tracks"] = list;
    return out;
}

Playlist& Playlist::operator+=(std::shared_ptr<Media> media) {
    add(std::move(media));
    return *this;
}

const Media& Playlist::operator[](std::size_t index) const {
    checkIndex(index, "Track");
    return *tracks_[index];
}
