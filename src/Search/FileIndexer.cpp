#include "../../include/Search/FileIndexer.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;

void FileIndexer::buildIndex(const std::string& directoryPath) {
    for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            std::cout << "Indexing file: " << entry.path() << std::endl;
            indexFile(entry.path().string());
        }
    }
}

void FileIndexer::indexFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    std::vector<std::string> tokens = tokenize(content);
    for (const std::string& token : tokens) {
        invertedIndex[token][filePath]++;
    }

    documentLengths[filePath] = tokens.size();
    totalDocuments++;
}

const std::unordered_map<std::string, std::map<std::string, int>>& FileIndexer::getInvertedIndex() const {
    return invertedIndex;
}

const std::map<std::string, int>& FileIndexer::getDocumentLengths() const {
    return documentLengths;
}

int FileIndexer::getTotalDocuments() const {
    return totalDocuments;
}

std::vector<std::string> FileIndexer::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string token;
    
    for (char c : text) {
        if (std::isalnum(c)) {
            token += std::tolower(c);
        } else if (!token.empty()) {
            tokens.push_back(token);
            token.clear();
        }
    }

    if (!token.empty()) {
        tokens.push_back(token);
    } 
    
    return tokens;
}