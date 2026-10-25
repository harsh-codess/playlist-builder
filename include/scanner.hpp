#pragma once

#include <set>
#include <string>
#include <vector>

#include "json.hpp"

// Walks a folder and describes each audio file as a Song request body.
class FolderScanner {
public:
    FolderScanner();
    explicit FolderScanner(std::set<std::string> extensions);

    std::vector<Json> scan(const std::string& folder) const;
    bool isAudioFile(const std::string& path) const;

    static int wavDurationSeconds(const std::string& path);
    static constexpr std::size_t kMaxFiles = 2000;

private:
    std::set<std::string> extensions_;
};

std::string expandHome(const std::string& path);
