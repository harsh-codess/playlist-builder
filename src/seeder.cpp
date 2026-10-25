#include "seeder.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "exceptions.hpp"
#include "scanner.hpp"

namespace fs = std::filesystem;

namespace {

constexpr double kPi = 3.14159265358979323846;

double noteFrequency(int midi) { return 440.0 * std::pow(2.0, (midi - 69) / 12.0); }

void writeLE(std::ofstream& out, std::uint32_t value, int bytes) {
    for (int i = 0; i < bytes; ++i) out.put(static_cast<char>((value >> (8 * i)) & 0xFF));
}

}  // namespace

SampleGenerator::SampleGenerator(std::string folder, int sampleRate)
    : folder_(std::move(folder)), sampleRate_(sampleRate) {
    if (sampleRate_ < 8000) throw BadRequestException("Sample rate must be at least 8000 Hz");
}

void SampleGenerator::writeWav(const std::string& path, const std::vector<short>& samples,
                               int sampleRate) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw StorageException("Cannot write sample file " + path);
    const std::uint32_t dataBytes = static_cast<std::uint32_t>(samples.size() * sizeof(short));
    out.write("RIFF", 4);
    writeLE(out, 36 + dataBytes, 4);
    out.write("WAVEfmt ", 8);
    writeLE(out, 16, 4);
    writeLE(out, 1, 2);                                  // PCM
    writeLE(out, 1, 2);                                  // mono
    writeLE(out, static_cast<std::uint32_t>(sampleRate), 4);
    writeLE(out, static_cast<std::uint32_t>(sampleRate * 2), 4);
    writeLE(out, 2, 2);
    writeLE(out, 16, 2);
    out.write("data", 4);
    writeLE(out, dataBytes, 4);
    for (short s : samples) writeLE(out, static_cast<std::uint16_t>(s), 2);
}

std::string SampleGenerator::generate(const Spec& spec) const {
    fs::create_directories(folder_);
    const std::string path = (fs::path(folder_) / spec.fileName).string();
    if (fs::exists(path)) return path;

    const std::size_t total = static_cast<std::size_t>(spec.seconds) * sampleRate_;
    std::vector<double> mix(total, 0.0);
    const double beat = 60.0 / spec.bpm;
    const double bar = beat * 4;
    const int third = spec.minor ? 3 : 4;

    auto addTone = [&](double start, double length, double freq, double amp, double attack) {
        auto first = static_cast<std::size_t>(start * sampleRate_);
        auto count = static_cast<std::size_t>(length * sampleRate_);
        for (std::size_t i = 0; i < count && first + i < total; ++i) {
            double t = static_cast<double>(i) / sampleRate_;
            double env = std::min(1.0, t / attack) * std::exp(-2.2 * t / length);
            double wave = std::sin(2 * kPi * freq * t) + 0.25 * std::sin(4 * kPi * freq * t);
            mix[first + i] += amp * env * wave;
        }
    };

    for (double barStart = 0; barStart < spec.seconds; barStart += bar) {
        int root = spec.chordRoots[static_cast<std::size_t>(barStart / bar) % spec.chordRoots.size()];
        const int chord[3] = {root, root + third, root + 7};

        for (int n : chord) addTone(barStart, bar * 1.1, noteFrequency(n - 12), 0.07, 0.4);
        if (spec.ambient) continue;

        addTone(barStart, beat * 2, noteFrequency(root - 24), 0.18, 0.01);
        addTone(barStart + beat * 2, beat * 2, noteFrequency(root - 24), 0.14, 0.01);
        static const int pattern[8] = {0, 1, 2, 1, 0, 2, 1, 2};
        for (int step = 0; step < 8; ++step) {
            int note = chord[pattern[step]] + (step >= 4 ? 12 : 0);
            addTone(barStart + step * beat / 2, beat * 0.9, noteFrequency(note + 12), 0.11, 0.005);
        }
    }

    const double fade = 1.5 * sampleRate_;
    double peak = 1e-9;
    for (double v : mix) peak = std::max(peak, std::fabs(v));
    std::vector<short> samples(total);
    for (std::size_t i = 0; i < total; ++i) {
        double gain = std::min({1.0, i / fade, (total - i) / fade});
        samples[i] = static_cast<short>(std::lround(mix[i] / peak * 0.8 * gain * 32767));
    }
    writeWav(path, samples, sampleRate_);
    return path;
}

Seeder::Seeder(std::string samplesFolder) : samplesFolder_(std::move(samplesFolder)) {}

void Seeder::seed(Library& library, Registry<Playlist>& playlists) const {
    SampleGenerator songs(samplesFolder_, 22050);
    SampleGenerator voice(samplesFolder_, 16000);

    struct Item {
        const char* type;
        const char* title;
        const char* creator;
        const char* extraA;   // album / show / narrator
        const char* extraB;   // genre
        int number;           // year / episode / chapter
        SampleGenerator::Spec spec;
    };

    const std::vector<Item> items = {
        {"song", "Odyssey", "Roary", "Horizons", "Electronic", 2024,
         {"odyssey.wav", 36, 104, {57, 53, 48, 55}, true, false}},
        {"song", "Violet Hour", "Kaia Moon", "Neon Bloom", "Synthwave", 2023,
         {"violet-hour.wav", 40, 96, {50, 46, 53, 48}, true, false}},
        {"song", "Towards Home", "Shawn Williams", "Long Roads", "Ambient", 2022,
         {"towards-home.wav", 44, 84, {48, 55, 57, 53}, false, false}},
        {"song", "Golden Static", "Chris Vale", "Afterlight", "R&B", 2021,
         {"golden-static.wav", 38, 90, {52, 48, 55, 50}, true, false}},
        {"song", "Icon", "Roary", "Horizons", "Electronic", 2024,
         {"icon.wav", 34, 112, {45, 41, 48, 43}, true, false}},
        {"song", "The First Day", "Lumen Fields", "Aurora", "Post-rock", 2020,
         {"the-first-day.wav", 42, 88, {53, 48, 50, 46}, false, false}},
        {"song", "Starfall", "Nova Lane", "Starfall", "Indie", 2025,
         {"starfall.wav", 39, 100, {55, 50, 52, 48}, false, false}},
        {"song", "Skin", "Ivy Laine", "Petals", "Electronic", 2025,
         {"skin.wav", 37, 98, {47, 43, 50, 45}, true, false}},
        {"podcast", "Why Virtual Functions Matter", "Asha Rao", "Compile Time", "", 12,
         {"compile-time-12.wav", 48, 70, {48, 53}, false, true}},
        {"podcast", "Smart Pointers Without Fear", "Asha Rao", "Compile Time", "", 13,
         {"compile-time-13.wav", 46, 70, {50, 55}, false, true}},
        {"audiobook", "The Quiet Algorithm", "Meera Iyer", "Rohan Sen", "", 1,
         {"quiet-algorithm-01.wav", 50, 60, {45, 52}, true, true}},
        {"audiobook", "The Quiet Algorithm", "Meera Iyer", "Rohan Sen", "", 2,
         {"quiet-algorithm-02.wav", 50, 60, {43, 50}, true, true}},
    };

    std::vector<std::shared_ptr<Media>> created;
    std::time_t now = std::time(nullptr);
    for (std::size_t i = 0; i < items.size(); ++i) {
        const Item& item = items[i];
        bool isSong = std::string(item.type) == "song";
        std::string path = (isSong ? songs : voice).generate(item.spec);

        Json body = Json::object();
        body["type"] = item.type;
        body["title"] = item.title;
        body["creator"] = item.creator;
        body["source"] = path;
        body["duration"] = FolderScanner::wavDurationSeconds(path);
        if (isSong) {
            body["album"] = item.extraA;
            body["genre"] = item.extraB;
            body["year"] = item.number;
        } else if (std::string(item.type) == "podcast") {
            body["show"] = item.extraA;
            body["episode"] = item.number;
        } else {
            body["narrator"] = item.extraA;
            body["chapter"] = item.number;
        }

        auto media = MediaFactory::create(library.nextId(), body);
        media->setAddedAt(now - static_cast<std::time_t>((items.size() - i) * 60));
        if (i == 0 || i == 6 || i == 8) media->setFavorite(true);
        library.add(media);
        created.push_back(media);
    }

    Playlist lateNight(playlists.nextId(), "Late Night Coding", "Synths and a podcast for long lab sessions");
    for (int index : {0, 1, 8, 6, 4}) lateNight += created[static_cast<std::size_t>(index)];
    playlists.add(lateNight);

    Playlist focus(playlists.nextId(), "Focus Flow", "Calm tracks and a chapter to read along with");
    for (int index : {2, 5, 10, 7}) focus += created[static_cast<std::size_t>(index)];
    playlists.add(focus);
}
