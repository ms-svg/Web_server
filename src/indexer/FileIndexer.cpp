#include "indexer/FileIndexer.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

void FileIndexer::buildIndex(const std::string& basePath) {
    for (const auto& entry : fs::recursive_directory_iterator(basePath)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension();
            if (ext == ".txt" || ext == ".json") {
                std::ifstream file(entry.path());
                std::string content((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
                index[entry.path().string()] = content;
            }
        }
    }
}

std::vector<std::string> FileIndexer::search(const std::string& query) {
    std::vector<std::string> results;
    for (const auto& [path, content] : index) {
        if (content.find(query) != std::string::npos) {
            results.push_back(path);
        }
    }
    return results;
}
