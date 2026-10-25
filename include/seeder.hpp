#pragma once

#include <string>
#include <vector>

#include "library.hpp"
#include "playlist.hpp"
#include "registry.hpp"

// Synthesises short WAV clips so a fresh install has something to play.
class SampleGenerator {
public:
    struct Spec {
        std::string fileName;
        int seconds;
        int bpm;
        std::vector<int> chordRoots;  // MIDI note numbers, one chord per bar
        bool minor;
        bool ambient;                 // podcasts/audiobooks: soft pad only
    };

    explicit SampleGenerator(std::string folder, int sampleRate = 22050);

    std::string generate(const Spec& spec) const;

private:
    static void writeWav(const std::string& path, const std::vector<short>& samples, int sampleRate);

    std::string folder_;
    int sampleRate_;
};

// Fills an empty library with demo songs, podcasts, audiobooks and playlists.
class Seeder {
public:
    explicit Seeder(std::string samplesFolder);
    void seed(Library& library, Registry<Playlist>& playlists) const;

private:
    std::string samplesFolder_;
};
