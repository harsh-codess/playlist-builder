#include "library.hpp"

#include <algorithm>
#include <numeric>

#include "exceptions.hpp"

MediaFilter parseFilter(const std::string& text) {
    static const std::map<std::string, MediaFilter> names = {
        {"", MediaFilter::All},           {"all", MediaFilter::All},
        {"song", MediaFilter::Songs},     {"songs", MediaFilter::Songs},
        {"podcast", MediaFilter::Podcasts}, {"podcasts", MediaFilter::Podcasts},
        {"audiobook", MediaFilter::Audiobooks}, {"audiobooks", MediaFilter::Audiobooks},
        {"favorites", MediaFilter::Favorites}, {"favourites", MediaFilter::Favorites},
    };
    auto it = names.find(toLower(text));
    if (it == names.end()) throw BadRequestException("Unknown filter \"" + text + "\"");
    return it->second;
}

SortKey parseSortKey(const std::string& text) {
    static const std::map<std::string, SortKey> names = {
        {"", SortKey::Recent},          {"recent", SortKey::Recent},
        {"title", SortKey::Title},      {"creator", SortKey::Creator},
        {"duration", SortKey::Duration}, {"plays", SortKey::Plays},
    };
    auto it = names.find(toLower(text));
    if (it == names.end()) throw BadRequestException("Unknown sort order \"" + text + "\"");
    return it->second;
}

std::string Library::nextId() const {
    long long n = nextId_;
    while (items_.count("m" + std::to_string(n))) ++n;
    return "m" + std::to_string(n);
}

void Library::reserveId(const std::string& id) {
    if (id.size() < 2 || id[0] != 'm') return;
    try {
        std::size_t used = 0;
        long long n = std::stoll(id.substr(1), &used);
        if (used == id.size() - 1 && n >= nextId_) nextId_ = n + 1;
    } catch (const std::exception&) {
    }
}

void Library::setNextId(long long next) { nextId_ = std::max<long long>(1, next); }

void Library::add(std::shared_ptr<Media> media) {
    if (!media) throw InvalidMediaException("Cannot add an empty media item");
    if (items_.count(media->id())) {
        throw DuplicateException("A media item with id " + media->id() + " already exists");
    }
    if (hasSource(media->source())) {
        throw DuplicateException("\"" + media->source() + "\" is already in the library");
    }
    reserveId(media->id());
    order_.push_back(media->id());
    items_.emplace(media->id(), std::move(media));
}

void Library::replace(const std::string& id, std::shared_ptr<Media> media) {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundException("No media item with id " + id);
    if (!media || media->id() != id) throw InvalidMediaException("Replacement must keep the same id");
    bool sourceTaken = std::any_of(items_.begin(), items_.end(), [&](const auto& entry) {
        return entry.first != id && entry.second->source() == media->source();
    });
    if (sourceTaken) {
        throw DuplicateException("\"" + media->source() + "\" is already in the library");
    }
    it->second = std::move(media);
}

void Library::remove(const std::string& id) {
    if (!items_.erase(id)) throw NotFoundException("No media item with id " + id);
    order_.erase(std::remove(order_.begin(), order_.end(), id), order_.end());
}

void Library::clear() {
    items_.clear();
    order_.clear();
    nextId_ = 1;
}

std::shared_ptr<Media> Library::get(const std::string& id) const {
    auto it = items_.find(id);
    if (it == items_.end()) throw NotFoundException("No media item with id " + id);
    return it->second;
}

bool Library::contains(const std::string& id) const { return items_.count(id) > 0; }

bool Library::hasSource(const std::string& source) const {
    return std::any_of(items_.begin(), items_.end(),
                       [&](const auto& entry) { return entry.second->source() == source; });
}

std::vector<std::shared_ptr<Media>> Library::all() const {
    std::vector<std::shared_ptr<Media>> result;
    result.reserve(order_.size());
    std::transform(order_.begin(), order_.end(), std::back_inserter(result),
                   [this](const std::string& id) { return items_.at(id); });
    return result;
}

std::vector<std::shared_ptr<Media>> Library::search(const std::string& query, MediaFilter filter,
                                                    SortKey sort) const {
    const std::string needle = toLower(query);
    auto keep = [&](const std::shared_ptr<Media>& m) {
        switch (filter) {
            case MediaFilter::Songs: if (m->type() != "song") return false; break;
            case MediaFilter::Podcasts: if (m->type() != "podcast") return false; break;
            case MediaFilter::Audiobooks: if (m->type() != "audiobook") return false; break;
            case MediaFilter::Favorites: if (!m->favorite()) return false; break;
            case MediaFilter::All: break;
        }
        return m->matches(needle);
    };

    std::vector<std::shared_ptr<Media>> everything = all();
    std::vector<std::shared_ptr<Media>> result;
    std::copy_if(everything.begin(), everything.end(), std::back_inserter(result), keep);

    switch (sort) {
        case SortKey::Recent:
            std::reverse(result.begin(), result.end());
            break;
        case SortKey::Title:
            std::stable_sort(result.begin(), result.end(),
                             [](const auto& a, const auto& b) { return *a < *b; });
            break;
        case SortKey::Creator:
            std::stable_sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
                return toLower(a->creator()) < toLower(b->creator());
            });
            break;
        case SortKey::Duration:
            std::stable_sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
                return a->duration() > b->duration();
            });
            break;
        case SortKey::Plays:
            std::stable_sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
                return a->plays() > b->plays();
            });
            break;
    }
    return result;
}

LibraryStats Library::stats() const {
    LibraryStats s;
    s.total = items_.size();
    s.byType = {{"song", 0}, {"podcast", 0}, {"audiobook", 0}};
    for (const auto& entry : items_) {
        ++s.byType[entry.second->type()];
        if (entry.second->favorite()) ++s.favorites;
    }
    s.totalDuration = std::accumulate(items_.begin(), items_.end(), 0LL,
                                      [](long long sum, const auto& entry) {
                                          return sum + entry.second->duration();
                                      });
    return s;
}
