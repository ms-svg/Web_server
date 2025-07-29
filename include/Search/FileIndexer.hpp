#ifndef FILE_INDEXER_HPP
#define FILE_INDEXER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <map>

class FileIndexer {
public:
    void buildIndex(const std::string& directoryPath);
    void indexFile(const std::string& filePath);
    const std::unordered_map<std::string, std::map<std::string, int>>& getInvertedIndex() const;
    const std::map<std::string, int>& getDocumentLengths() const;
    int getTotalDocuments() const;
    std::vector<std::string> tokenize(const std::string& text);

private:
    std::unordered_map<std::string, std::map<std::string, int>> invertedIndex;
    std::map<std::string, int> documentLengths;
    int totalDocuments = 0;

};

#endif // FILE_INDEXER_HPP