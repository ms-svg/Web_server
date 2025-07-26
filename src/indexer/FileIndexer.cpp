#include "../../include/indexer/FileIndexer.hpp"
#include <filesystem>
#include <fstream>
#include <regex>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
namespace fs = std::filesystem;

std::string stripHtmlTags(const std::string& input) {
    return std::regex_replace(input, std::regex("<[^>]*>"), " ");
}

void FileIndexer::buildIndex(const std::string& basePath) {
    for (const auto& entry : fs::recursive_directory_iterator(basePath)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            std::cout<<filename << std::endl; // filename being indexed.
            // Only index text-based files
            std::string ext = entry.path().extension();
            if (ext == ".txt" || ext == ".json" || ext == ".html" || ext == ".htm") {
                std::ifstream file(entry.path());
                std::string content((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
                
                // Strip HTML tags for .html or .htm files
                if (ext == ".html" || ext == ".htm") {
                    content = stripHtmlTags(content);
                }

                this->index[entry.path().string()] = content;
            }
        }
    }
}

std::vector<std::string> FileIndexer::search(const std::string& query) {
    std::cout << "Searching for: " << query << "\n";
    std::vector<std::string> results;
    if (query.empty()) {
        std::cout << "Empty query, returning no results.\n";
        return results;
    }

    std::istringstream qss(query);
    std::unordered_set<std::string> queryWords;
    std::string word;
    while (qss >> word) {
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        queryWords.insert(word);
    }

    for (const auto& [path, content] : index) {
        std::istringstream css(content);
        std::unordered_set<std::string> contentWords;
        std::string cword;
        while (css >> cword) {
            cword.erase(std::remove_if(cword.begin(), cword.end(), ::ispunct), cword.end());
            std::transform(cword.begin(), cword.end(), cword.begin(), ::tolower);
            contentWords.insert(cword);
        }

        bool match = true;
        for (const auto& qw : queryWords) {
            if (contentWords.find(qw) == contentWords.end()) {
                match = false;
                break;
            }
        }

        if (match) {
            std::string path_only = fs::path(path).filename().string();
            std::cout <<"Found your query = "<< queryWords << " in file path " << path_only << std::endl; 
            results.push_back(path_only);
        }
    }

    return results;
}

