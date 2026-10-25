#include "scanner.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "exceptions.hpp"
#include "media.hpp"

namespace fs = std::filesystem;

std::string expandHome(const std::string& path) {
    if (path == "~" || path.rfind("~/", 0) == 0) {
        const char* home = std::getenv("HOME");
        if (home) return std::string(home) + path.substr(1);
    }
    return path;
}

FolderScanner::FolderScanner()
    : FolderScanner({".mp3", ".m4a", ".aac", ".wav", ".flac", ".ogg", ".oga", ".opus", ".webm"}) {}

FolderScanner::FolderScanner(std::set<std::string> extensions) : extensions_(std::move(extensions)) {
    if (extensions_.empty()) throw BadRequestException("Scanner needs at least one file extension");
}

bool FolderScanner::isAudioFile(const std::string& path) const {
    return extensions_.count(toLower(fs::path(path).extension().string())) > 0;
}

int FolderScanner::wavDurationSeconds(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return 0;
    char riff[12];
    if (!in.read(riff, 12) || std::string(riff, 4) != "RIFF" || std::string(riff + 8, 4) != "WAVE") return 0;

    std::uint32_t byteRate = 0;
    while (in) {
        char id[4];
        std::uint32_t size = 0;
        if (!in.read(id, 4) || !in.read(reinterpret_cast<char*>(&size), 4)) break;
        std::string chunk(id, 4);
        if (chunk == "fmt " && size >= 16) {
            char fmt[16];
            in.read(fmt, 16);
            byteRate = static_cast<std::uint8_t>(fmt[8]) | (static_cast<std::uint8_t>(fmt[9]) << 8) |
                       (static_cast<std::uint8_t>(fmt[10]) << 16) |
                       (static_cast<std::uint32_t>(static_cast<std::uint8_t>(fmt[11])) << 24);
            in.seekg(size - 16 + (size & 1), std::ios::cur);
        } else if (chunk == "data") {
            return byteRate ? static_cast<int>((size + byteRate / 2) / byteRate) : 0;
        } else {
            in.seekg(size + (size & 1), std::ios::cur);
        }
    }
    return 0;
}

std::vector<Json> FolderScanner::scan(const std::string& folder) const {
    const std::string root = expandHome(folder);
    if (root.empty()) throw BadRequestException("Folder path cannot be empty");

    std::error_code ec;
    if (!fs::exists(root, ec)) throw NotFoundException("Folder not found: " + root);
    if (!fs::is_directory(root, ec)) throw BadRequestException(root + " is not a folder");

    std::vector<fs::path> files;
    auto options = fs::directory_options::skip_permission_denied;
    for (auto it = fs::recursive_directory_iterator(root, options, ec);
         !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (it->is_regular_file(ec) && isAudioFile(it->path().string())) {
            files.push_back(it->path());
            if (files.size() >= kMaxFiles) break;
        }
    }
    std::sort(files.begin(), files.end());

    std::vector<Json> bodies;
    bodies.reserve(files.size());
    for (const auto& file : files) {
        std::string stem = file.stem().string();
        std::string folderName = file.parent_path().filename().string();
        std::string artist = folderName.empty() ? "Unknown Artist" : folderName;
        std::string title = stem;

        auto dash = stem.find(" - ");
        if (dash != std::string::npos && dash > 0 && dash + 3 < stem.size()) {
            artist = stem.substr(0, dash);
            title = stem.substr(dash + 3);
        }

        Json body = Json::object();
        body["type"] = "song";
        body["title"] = title;
        body["creator"] = artist;
        body["album"] = folderName;
        body["source"] = fs::absolute(file).string();
        body["duration"] = toLower(file.extension().string()) == ".wav" ? wavDurationSeconds(file.string()) : 0;
        bodies.push_back(body);
    }
    return bodies;
}
