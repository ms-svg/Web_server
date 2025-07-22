#include "../../include/indexer/FileIndexer.hpp"
#include <filesystem>
#include <fstream>
#include <regex>
#include <iostream>
#include <string>
#include <unordered_map>
namespace fs = std::filesystem;

std::string stripHtmlTags(const std::string& input) {
    // Remove tags like <div>, <script>...</script>, etc.
    return std::regex_replace(input, std::regex("<[^>]*>"), " ");
}

void FileIndexer::buildIndex(const std::string& basePath) {
    for (const auto& entry : fs::recursive_directory_iterator(basePath)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            std::cout<<filename << std::endl; // Debug: Print filename being indexed
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
    std::cout << "🔍 Searching for: " << query << "\n";
    std::vector<std::string> results;
    if (query.empty()) {
        std::cout << "Empty query, returning no results.\n";
        return results; // Return empty if query is empty
    }
    // Search through the indexed files
    for (const auto& [path, content] : index) {
        // Convert both content and query to lowercase for case-insensitive search
        std::string lower_content = content;
        std::transform(lower_content.begin(), lower_content.end(), lower_content.begin(), ::tolower);
        std::string lower_query = query;
        std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
        // Check if the content contains the query
        if (lower_content.find(lower_query) != std::string::npos) {
            std::string path_only = fs::path(path).filename().string(); // Get only the filename
            results.push_back(path_only);
        }
        // Debug: Print the content being checked
        std::cout << "Checking file: " << path << "\n";
        std::cout << "Content: " << lower_content << "\n";
        std::cout << "Query: " << lower_query << "\n";
    }
    if (results.empty()) {
        std::cout << "No results found for query: " << query << "\n";
    } else {
        std::cout << "Found " << results.size() << " results for query: " << query << "\n";
    }
    // print results for debugging
    for (const auto& result : results) {
        std::cout << "Result: " << result << "\n";
    }
    return results;
}
