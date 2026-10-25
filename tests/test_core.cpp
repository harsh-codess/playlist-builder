// Unit tests for the domain classes. Run with `make test`.
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "exceptions.hpp"
#include "json.hpp"
#include "library.hpp"
#include "playlist.hpp"
#include "registry.hpp"
#include "scanner.hpp"
#include "seeder.hpp"
#include "storage.hpp"

namespace fs = std::filesystem;

namespace {

struct TestCase {
    std::string id;
    std::string name;
    std::function<void()> body;
};

int failures = 0;

void check(bool condition, const std::string& what) {
    if (!condition) throw std::runtime_error("check failed: " + what);
}

template <typename ExceptionT>
std::string expectThrow(const std::function<void()>& fn) {
    try {
        fn();
    } catch (const ExceptionT& e) {
        return e.what();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("wrong exception: ") + e.what());
    }
    throw std::runtime_error("expected an exception, none thrown");
}

const std::string kTmp = (fs::temp_directory_path() / "playlist-builder-tests").string();

std::string makeWav(const std::string& name, int seconds) {
    SampleGenerator gen(kTmp, 8000);
    return gen.generate({name, seconds, 120, {48, 53}, false, true});
}

std::shared_ptr<Media> song(const std::string& id, const std::string& title, const std::string& artist,
                            int duration = 180) {
    return std::make_shared<Song>(id, title, artist, duration, "/music/" + id + ".mp3", "Album", "Pop", 2024);
}

}  // namespace

int main() {
    fs::remove_all(kTmp);
    fs::create_directories(kTmp);

    std::vector<TestCase> tests = {
        {"TC01", "Song constructor stores validated fields", [] {
             Song s("m1", "  Odyssey  ", "Roary", 176, "a.wav", "Horizons", "Electronic", 2024);
             check(s.title() == "Odyssey", "title is trimmed");
             check(s.type() == "song" && s.year() == 2024, "type and year");
         }},
        {"TC02", "Empty title is rejected by the constructor", [] {
             auto msg = expectThrow<InvalidMediaException>([] { Song("m1", "   ", "A", 10, "a.wav"); });
             check(msg == "Title cannot be empty", msg);
         }},
        {"TC03", "Negative duration is rejected", [] {
             expectThrow<InvalidMediaException>([] { Song("m1", "T", "A", -5, "a.wav"); });
         }},
        {"TC04", "Song year outside 1900-2100 is rejected", [] {
             expectThrow<InvalidMediaException>([] { Song("m1", "T", "A", 10, "a.wav", "", "", 1800); });
         }},
        {"TC05", "Podcast episode number must be at least 1", [] {
             expectThrow<InvalidMediaException>([] { PodcastEpisode("m1", "T", "Host", 10, "a.wav", "Show", 0); });
         }},
        {"TC06", "Virtual subtitle() differs per derived class", [] {
             std::vector<std::shared_ptr<Media>> items = {
                 std::make_shared<Song>("m1", "Skin", "Ivy", 100, "a", "Petals", "", 2025),
                 std::make_shared<PodcastEpisode>("m2", "Pointers", "Asha", 100, "b", "Compile Time", 12),
                 std::make_shared<Audiobook>("m3", "Quiet", "Meera", 100, "c", "Rohan", 2)};
             check(items[0]->subtitle() == "Ivy \u00b7 Petals (2025)", items[0]->subtitle());
             check(items[1]->subtitle() == "Compile Time \u00b7 Episode 12", items[1]->subtitle());
             check(items[2]->subtitle() == "Meera \u00b7 Chapter 2 \u00b7 read by Rohan", items[2]->subtitle());
         }},
        {"TC07", "MediaFactory builds the right subclass", [] {
             Json body = Json::parse(R"({"type":"audiobook","title":"Quiet","creator":"Meera","duration":60,"source":"x","chapter":3})");
             auto m = MediaFactory::create("m9", body);
             check(dynamic_cast<Audiobook*>(m.get()) != nullptr, "is Audiobook");
             check(m->typeLabel() == "Audiobook", "label");
         }},
        {"TC08", "MediaFactory rejects unknown type", [] {
             auto msg = expectThrow<InvalidMediaException>([] {
                 MediaFactory::create("m1", Json::parse(R"({"type":"video","title":"T","creator":"C","source":"s"})"));
             });
             check(msg.find("Unknown media type") != std::string::npos, msg);
         }},
        {"TC09", "Library search matches album through Song::matches", [] {
             Library lib;
             lib.add(std::make_shared<Song>("m1", "Track", "Band", 100, "a", "Moonlight Album", "", 0));
             lib.add(song("m2", "Other", "Someone"));
             auto hits = lib.search("moonlight");
             check(hits.size() == 1 && hits[0]->id() == "m1", "one hit");
         }},
        {"TC10", "Library filter and sort by title", [] {
             Library lib;
             lib.add(song("m1", "Zeta", "A"));
             lib.add(song("m2", "alpha", "B"));
             lib.add(std::make_shared<PodcastEpisode>("m3", "Beta", "C", 50, "p.mp3", "Show", 1));
             auto songs = lib.search("", MediaFilter::Songs, SortKey::Title);
             check(songs.size() == 2 && songs[0]->title() == "alpha", "sorted songs");
         }},
        {"TC11", "Duplicate id and duplicate source are rejected", [] {
             Library lib;
             lib.add(song("m1", "A", "B"));
             expectThrow<DuplicateException>([&] { lib.add(song("m1", "C", "D")); });
             expectThrow<DuplicateException>([&] {
                 lib.add(std::make_shared<Song>("m2", "Copy", "B", 10, "/music/m1.mp3"));
             });
         }},
        {"TC12", "Missing media id throws NotFoundException", [] {
             Library lib;
             expectThrow<NotFoundException>([&] { lib.get("m404"); });
         }},
        {"TC13", "Playlist += operator, total duration and [] access", [] {
             Playlist p("p1", "Mix");
             p += song("m1", "A", "X", 100);
             p += song("m2", "B", "Y", 50);
             check(p.size() == 2 && p.totalDuration() == 150, "size and duration");
             check(p[1].title() == "B", "operator[]");
         }},
        {"TC14", "Playlist move() reorders with std::rotate", [] {
             Playlist p("p1", "Mix");
             for (auto id : {"m1", "m2", "m3", "m4"}) p += song(id, id, "X");
             p.move(0, 2);
             check(p[0].id() == "m2" && p[2].id() == "m1", "forward move");
             p.move(3, 0);
             check(p[0].id() == "m4" && p[3].id() == "m1", "backward move");
         }},
        {"TC15", "Out-of-range move throws BadRequestException", [] {
             Playlist p("p1", "Mix");
             p += song("m1", "A", "X");
             expectThrow<BadRequestException>([&] { p.move(0, 5); });
         }},
        {"TC16", "Exporting an empty playlist throws EmptyPlaylistException", [] {
             Playlist p("p1", "Empty");
             auto msg = expectThrow<EmptyPlaylistException>([&] { p.toM3u(); });
             check(msg.find("Empty") != std::string::npos, msg);
         }},
        {"TC17", "M3U export uses each type's own label", [] {
             Playlist p("p1", "Mixed");
             p += std::make_shared<Song>("m1", "Skin", "Ivy", 100, "https://x/skin.mp3");
             p += std::make_shared<PodcastEpisode>("m2", "Pointers", "Asha", 200, "https://x/ep.mp3", "Compile Time", 12);
             std::string m3u = p.toM3u();
             check(m3u.rfind("#EXTM3U\n", 0) == 0, "header");
             check(m3u.find("#EXTINF:100,Ivy - Skin\nhttps://x/skin.mp3") != std::string::npos, "song line");
             check(m3u.find("#EXTINF:200,Compile Time - Ep. 12: Pointers") != std::string::npos, "podcast line");
         }},
        {"TC18", "Copy constructor shares tracks but is independent", [] {
             Playlist a("p1", "Original");
             a += song("m1", "A", "X");
             Playlist b(a);
             b.rename("Copy");
             b += song("m2", "B", "Y");
             check(a.size() == 1 && b.size() == 2 && a.name() == "Original", "independent copy");
             check(&a[0] == &b[0], "shares the same Media object");
         }},
        {"TC19", "Removing media drops every occurrence from a playlist", [] {
             Playlist p("p1", "Mix");
             auto m = song("m1", "A", "X");
             p += m;
             p += song("m2", "B", "Y");
             p += m;
             check(p.removeMedia("m1") == 2 && p.size() == 1, "two removed");
         }},
        {"TC20", "Blank playlist name is rejected", [] {
             expectThrow<BadRequestException>([] { Playlist("p1", "   "); });
         }},
        {"TC21", "JSON parser round-trips and rejects bad input", [] {
             Json j = Json::parse(R"({"a":[1,2,{"b":"x\"y"}],"c":true,"d":null,"e":"\u00e9"})");
             check(j.get("a").size() == 3 && j.get("c").asBool(), "parsed");
             check(Json::parse(j.dump()).dump() == j.dump(), "round trip");
             expectThrow<BadRequestException>([] { Json::parse("{\"a\":}"); });
         }},
        {"TC22", "Storage save/load restores library and playlists", [] {
             Library lib;
             Registry<Playlist> lists("p", "playlist");
             lib.add(song(lib.nextId(), "A", "X", 120));
             lib.add(std::make_shared<Audiobook>(lib.nextId(), "B", "Y", 300, "/b.mp3", "N", 4));
             lib.get("m1")->setFavorite(true);
             Playlist p(lists.nextId(), "Saved");
             p += lib.get("m2");
             p += lib.get("m1");
             lists.add(p);
             Storage store(kTmp + "/library.json");
             store.save(lib, lists);

             Library lib2;
             Registry<Playlist> lists2("p", "playlist");
             auto warnings = store.load(lib2, lists2);
             check(warnings.empty() && lib2.size() == 2, "media restored");
             check(lib2.get("m1")->favorite(), "favorite restored");
             check(lists2.get("p1")[0].type() == "audiobook", "playlist order restored");
             check(lib2.nextId() == "m3", "id counter restored");
         }},
        {"TC23", "Corrupted save file raises StorageException", [] {
             std::string path = kTmp + "/broken.json";
             { std::ofstream(path) << "{ not json"; }
             Library lib;
             Registry<Playlist> lists("p", "playlist");
             expectThrow<StorageException>([&] { Storage(path).load(lib, lists); });
         }},
        {"TC24", "Folder scanner reads 'Artist - Title' and WAV duration", [] {
             fs::create_directories(kTmp + "/scan/Album One");
             std::string wav = makeWav("tone.wav", 3);
             fs::copy_file(wav, kTmp + "/scan/Album One/Nova Lane - Starfall.wav");
             { std::ofstream(kTmp + "/scan/notes.txt") << "not audio"; }
             auto found = FolderScanner().scan(kTmp + "/scan");
             check(found.size() == 1, "only audio files");
             check(found[0].getString("creator") == "Nova Lane" && found[0].getString("title") == "Starfall", "parsed name");
             check(found[0].getInt("duration") == 3, "wav duration");
         }},
        {"TC25", "Scanning a missing folder throws NotFoundException", [] {
             expectThrow<NotFoundException>([] { FolderScanner().scan(kTmp + "/does-not-exist"); });
         }},
        {"TC26", "Polymorphic objects are destroyed through base pointers", [] {
             int before = Media::liveCount();
             {
                 std::vector<std::unique_ptr<Media>> owned;
                 owned.push_back(std::make_unique<Song>("m1", "A", "B", 1, "a"));
                 owned.push_back(std::make_unique<Audiobook>("m2", "A", "B", 1, "b", "", 1));
                 check(Media::liveCount() == before + 2, "constructed");
             }
             check(Media::liveCount() == before, "virtual destructor ran");
         }},
        {"TC27", "operator<< prints a readable line", [] {
             std::ostringstream out;
             out << Song("m1", "Skin", "Ivy", 125, "a", "Petals", "", 0);
             check(out.str() == "[Song] Skin - Ivy \u00b7 Petals (2:05)", out.str());
         }},
    };

    std::printf("%-6s %-62s %s\n", "ID", "Test case", "Result");
    std::printf("%s\n", std::string(78, '-').c_str());
    for (const auto& test : tests) {
        std::string result = "PASS";
        try {
            test.body();
        } catch (const std::exception& e) {
            result = std::string("FAIL (") + e.what() + ")";
            ++failures;
        }
        std::printf("%-6s %-62s %s\n", test.id.c_str(), test.name.c_str(), result.c_str());
    }
    std::printf("%s\n%zu tests, %d failed\n", std::string(78, '-').c_str(), tests.size(), failures);
    fs::remove_all(kTmp);
    return failures == 0 ? 0 : 1;
}
