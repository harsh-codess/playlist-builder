#pragma once

#include <ctime>
#include <memory>
#include <ostream>
#include <string>

#include "json.hpp"

// Abstract base for everything the library can hold. Derived classes supply
// their own type name, display subtitle, playlist label and search rules.
class Media {
public:
    Media(std::string id, std::string title, std::string creator, int durationSeconds,
          std::string source);
    Media(const Media& other);
    Media& operator=(const Media&) = delete;
    virtual ~Media();

    virtual std::string type() const = 0;
    virtual std::string typeLabel() const = 0;
    virtual std::string subtitle() const = 0;
    virtual std::string m3uLabel() const;
    virtual bool matches(const std::string& loweredQuery) const;

    Json toJson() const;
    Json toViewJson() const;
    std::string toM3uEntry() const;

    bool isRemote() const;
    bool isPlayable() const;

    const std::string& id() const { return id_; }
    const std::string& title() const { return title_; }
    const std::string& creator() const { return creator_; }
    int duration() const { return duration_; }
    const std::string& source() const { return source_; }
    bool favorite() const { return favorite_; }
    int plays() const { return plays_; }
    std::time_t addedAt() const { return addedAt_; }

    void setDuration(int seconds);
    void setFavorite(bool value) { favorite_ = value; }
    void toggleFavorite() { favorite_ = !favorite_; }
    void setPlays(int plays);
    void recordPlay() { ++plays_; }
    void setAddedAt(std::time_t when) { addedAt_ = when; }

    bool operator<(const Media& other) const;
    friend std::ostream& operator<<(std::ostream& out, const Media& media);

    static int liveCount() { return liveCount_; }
    static constexpr int kMaxDurationSeconds = 24 * 60 * 60;

protected:
    virtual void writeFields(Json& out) const = 0;

    static std::string requireText(const std::string& value, const std::string& field,
                                   std::size_t maxLength = 160);
    static std::string optionalText(const std::string& value, const std::string& field,
                                    std::size_t maxLength = 160);
    static bool containsLowered(const std::string& haystack, const std::string& loweredNeedle);

private:
    std::string id_;
    std::string title_;
    std::string creator_;
    int duration_;
    std::string source_;
    bool favorite_ = false;
    int plays_ = 0;
    std::time_t addedAt_;

    static int liveCount_;
};

class Song : public Media {
public:
    Song(std::string id, std::string title, std::string artist, int durationSeconds,
         std::string source, std::string album = "", std::string genre = "", int year = 0);

    std::string type() const override { return "song"; }
    std::string typeLabel() const override { return "Song"; }
    std::string subtitle() const override;
    std::string m3uLabel() const override;
    bool matches(const std::string& loweredQuery) const override;

    const std::string& album() const { return album_; }
    const std::string& genre() const { return genre_; }
    int year() const { return year_; }

protected:
    void writeFields(Json& out) const override;

private:
    std::string album_;
    std::string genre_;
    int year_;
};

class PodcastEpisode : public Media {
public:
    PodcastEpisode(std::string id, std::string title, std::string host, int durationSeconds,
                   std::string source, std::string show, int episode);

    std::string type() const override { return "podcast"; }
    std::string typeLabel() const override { return "Podcast"; }
    std::string subtitle() const override;
    std::string m3uLabel() const override;
    bool matches(const std::string& loweredQuery) const override;

    const std::string& show() const { return show_; }
    int episode() const { return episode_; }

protected:
    void writeFields(Json& out) const override;

private:
    std::string show_;
    int episode_;
};

class Audiobook : public Media {
public:
    Audiobook(std::string id, std::string title, std::string author, int durationSeconds,
              std::string source, std::string narrator, int chapter);

    std::string type() const override { return "audiobook"; }
    std::string typeLabel() const override { return "Audiobook"; }
    std::string subtitle() const override;
    std::string m3uLabel() const override;
    bool matches(const std::string& loweredQuery) const override;

    const std::string& narrator() const { return narrator_; }
    int chapter() const { return chapter_; }

protected:
    void writeFields(Json& out) const override;

private:
    std::string narrator_;
    int chapter_;
};

// Builds the right Media subclass from a JSON body (API request or save file).
class MediaFactory {
public:
    static std::shared_ptr<Media> create(const std::string& id, const Json& data);
};

std::string toLower(std::string text);
std::string formatDuration(int seconds);
