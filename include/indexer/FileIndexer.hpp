#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class FileIndexer {
public:
    void buildIndex(const std::string& basePath);
    std::vector<std::string> search(const std::string& query);
    std::unordered_map<std::string, std::string> index;
};
