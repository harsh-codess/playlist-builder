#include "media.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>

#include "exceptions.hpp"

int Media::liveCount_ = 0;

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

std::string formatDuration(int seconds) {
    if (seconds <= 0) return "--:--";
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    char buf[16];
    if (h > 0) std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
    else std::snprintf(buf, sizeof buf, "%d:%02d", m, s);
    return buf;
}

namespace {

std::string trim(const std::string& s) {
    auto begin = std::find_if_not(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return begin < end ? std::string(begin, end) : std::string();
}

}  // namespace

// ---------------------------------------------------------------- Media

Media::Media(std::string id, std::string title, std::string creator, int durationSeconds,
             std::string source)
    : id_(std::move(id)),
      title_(requireText(title, "Title")),
      creator_(requireText(creator, "Creator")),
      duration_(0),
      source_(requireText(source, "Source", 2048)),
      addedAt_(std::time(nullptr)) {
    if (id_.empty()) throw InvalidMediaException("Media id cannot be empty");
    setDuration(durationSeconds);
    ++liveCount_;
}

Media::Media(const Media& other)
    : id_(other.id_),
      title_(other.title_),
      creator_(other.creator_),
      duration_(other.duration_),
      source_(other.source_),
      favorite_(other.favorite_),
      plays_(other.plays_),
      addedAt_(other.addedAt_) {
    ++liveCount_;
}

Media::~Media() { --liveCount_; }

std::string Media::requireText(const std::string& value, const std::string& field,
                               std::size_t maxLength) {
    std::string cleaned = trim(value);
    if (cleaned.empty()) throw InvalidMediaException(field + " cannot be empty");
    if (cleaned.size() > maxLength) {
        throw InvalidMediaException(field + " is too long (max " + std::to_string(maxLength) +
                                    " characters)");
    }
    return cleaned;
}

std::string Media::optionalText(const std::string& value, const std::string& field,
                                std::size_t maxLength) {
    std::string cleaned = trim(value);
    if (cleaned.size() > maxLength) {
        throw InvalidMediaException(field + " is too long (max " + std::to_string(maxLength) +
                                    " characters)");
    }
    return cleaned;
}

bool Media::containsLowered(const std::string& haystack, const std::string& loweredNeedle) {
    return toLower(haystack).find(loweredNeedle) != std::string::npos;
}

void Media::setDuration(int seconds) {
    if (seconds < 0) throw InvalidMediaException("Duration cannot be negative");
    if (seconds > kMaxDurationSeconds) throw InvalidMediaException("Duration cannot exceed 24 hours");
    duration_ = seconds;
}

void Media::setPlays(int plays) {
    if (plays < 0) throw InvalidMediaException("Play count cannot be negative");
    plays_ = plays;
}

std::string Media::m3uLabel() const { return creator_ + " - " + title_; }

bool Media::matches(const std::string& loweredQuery) const {
    return loweredQuery.empty() || containsLowered(title_, loweredQuery) ||
           containsLowered(creator_, loweredQuery);
}

bool Media::isRemote() const {
    return source_.rfind("http://", 0) == 0 || source_.rfind("https://", 0) == 0;
}

bool Media::isPlayable() const {
    if (isRemote()) return true;
    std::error_code ec;
    return std::filesystem::is_regular_file(source_, ec);
}

Json Media::toJson() const {
    Json out = Json::object();
    out["id"] = id_;
    out["type"] = type();
    out["typeLabel"] = typeLabel();
    out["title"] = title_;
    out["creator"] = creator_;
    out["duration"] = duration_;
    out["source"] = source_;
    out["favorite"] = favorite_;
    out["plays"] = plays_;
    out["addedAt"] = static_cast<long long>(addedAt_);
    writeFields(out);
    return out;
}

Json Media::toViewJson() const {
    Json out = toJson();
    out["subtitle"] = subtitle();
    out["label"] = m3uLabel();
    out["playable"] = isPlayable();
    out["streamUrl"] = isRemote() ? source_ : "/stream/" + id_;
    return out;
}

std::string Media::toM3uEntry() const {
    int seconds = duration_ > 0 ? duration_ : -1;
    std::string location = source_;
    if (!isRemote()) {
        std::error_code ec;
        auto absolute = std::filesystem::absolute(source_, ec);
        if (!ec) location = absolute.lexically_normal().string();
    }
    return "#EXTINF:" + std::to_string(seconds) + "," + m3uLabel() + "\n" + location + "\n";
}

bool Media::operator<(const Media& other) const { return toLower(title_) < toLower(other.title_); }

std::ostream& operator<<(std::ostream& out, const Media& media) {
    return out << "[" << media.typeLabel() << "] " << media.title() << " - " << media.subtitle()
               << " (" << formatDuration(media.duration()) << ")";
}

// ---------------------------------------------------------------- Song

Song::Song(std::string id, std::string title, std::string artist, int durationSeconds,
           std::string source, std::string album, std::string genre, int year)
    : Media(std::move(id), std::move(title), std::move(artist), durationSeconds, std::move(source)),
      album_(optionalText(album, "Album")),
      genre_(optionalText(genre, "Genre", 40)),
      year_(year) {
    if (year_ != 0 && (year_ < 1900 || year_ > 2100)) {
        throw InvalidMediaException("Year must be between 1900 and 2100");
    }
}

std::string Song::subtitle() const {
    std::string text = creator();
    if (!album_.empty()) text += " \u00b7 " + album_;
    if (year_ != 0) text += " (" + std::to_string(year_) + ")";
    return text;
}

std::string Song::m3uLabel() const { return creator() + " - " + title(); }

bool Song::matches(const std::string& loweredQuery) const {
    return Media::matches(loweredQuery) || containsLowered(album_, loweredQuery) ||
           containsLowered(genre_, loweredQuery);
}

void Song::writeFields(Json& out) const {
    out["album"] = album_;
    out["genre"] = genre_;
    out["year"] = year_;
}

// ---------------------------------------------------------------- PodcastEpisode

PodcastEpisode::PodcastEpisode(std::string id, std::string title, std::string host,
                               int durationSeconds, std::string source, std::string show,
                               int episode)
    : Media(std::move(id), std::move(title), std::move(host), durationSeconds, std::move(source)),
      show_(requireText(show, "Show")),
      episode_(episode) {
    if (episode_ < 1) throw InvalidMediaException("Episode number must be 1 or more");
}

std::string PodcastEpisode::subtitle() const {
    return show_ + " \u00b7 Episode " + std::to_string(episode_);
}

std::string PodcastEpisode::m3uLabel() const {
    return show_ + " - Ep. " + std::to_string(episode_) + ": " + title();
}

bool PodcastEpisode::matches(const std::string& loweredQuery) const {
    return Media::matches(loweredQuery) || containsLowered(show_, loweredQuery);
}

void PodcastEpisode::writeFields(Json& out) const {
    out["show"] = show_;
    out["episode"] = episode_;
}

// ---------------------------------------------------------------- Audiobook

Audiobook::Audiobook(std::string id, std::string title, std::string author, int durationSeconds,
                     std::string source, std::string narrator, int chapter)
    : Media(std::move(id), std::move(title), std::move(author), durationSeconds, std::move(source)),
      narrator_(optionalText(narrator, "Narrator")),
      chapter_(chapter) {
    if (chapter_ < 1) throw InvalidMediaException("Chapter number must be 1 or more");
}

std::string Audiobook::subtitle() const {
    std::string text = creator() + " \u00b7 Chapter " + std::to_string(chapter_);
    if (!narrator_.empty()) text += " \u00b7 read by " + narrator_;
    return text;
}

std::string Audiobook::m3uLabel() const {
    return creator() + " - " + title() + ", Chapter " + std::to_string(chapter_);
}

bool Audiobook::matches(const std::string& loweredQuery) const {
    return Media::matches(loweredQuery) || containsLowered(narrator_, loweredQuery);
}

void Audiobook::writeFields(Json& out) const {
    out["narrator"] = narrator_;
    out["chapter"] = chapter_;
}

// ---------------------------------------------------------------- MediaFactory

std::shared_ptr<Media> MediaFactory::create(const std::string& id, const Json& data) {
    if (!data.isObject()) throw BadRequestException("Media must be a JSON object");

    std::string type = toLower(data.getString("type"));
    std::string title = data.getString("title");
    std::string creator = data.getString("creator");
    long long duration = data.getInt("duration", 0);
    std::string source = data.getString("source");

    if (duration < 0 || duration > Media::kMaxDurationSeconds) {
        throw InvalidMediaException(duration < 0 ? "Duration cannot be negative"
                                                 : "Duration cannot exceed 24 hours");
    }
    int seconds = static_cast<int>(duration);

    std::shared_ptr<Media> media;
    if (type == "song") {
        media = std::make_shared<Song>(id, title, creator, seconds, source, data.getString("album"),
                                       data.getString("genre"),
                                       static_cast<int>(data.getInt("year", 0)));
    } else if (type == "podcast") {
        std::string show = data.getString("show");
        media = std::make_shared<PodcastEpisode>(id, title, creator, seconds, source,
                                                 show.empty() ? creator : show,
                                                 static_cast<int>(data.getInt("episode", 1)));
    } else if (type == "audiobook") {
        media = std::make_shared<Audiobook>(id, title, creator, seconds, source,
                                            data.getString("narrator"),
                                            static_cast<int>(data.getInt("chapter", 1)));
    } else if (type.empty()) {
        throw InvalidMediaException("Media type is required (song, podcast or audiobook)");
    } else {
        throw InvalidMediaException("Unknown media type \"" + type +
                                    "\" (expected song, podcast or audiobook)");
    }

    if (data.has("favorite")) media->setFavorite(data.getBool("favorite"));
    if (data.has("plays")) media->setPlays(static_cast<int>(data.getInt("plays")));
    if (data.has("addedAt")) media->setAddedAt(static_cast<std::time_t>(data.getInt("addedAt")));
    return media;
}
