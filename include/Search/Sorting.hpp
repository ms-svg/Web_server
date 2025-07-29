#ifndef SORTING_HPP
#define SORTING_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <cmath>

struct DocumentScore {
    std::string document;
    double score;

    bool operator<(const DocumentScore& other) const {
        return score > other.score;  // for descending sort
    }
};

class Sorting {
public:
    Sorting(
        const std::unordered_map<std::string, std::map<std::string, int>>& invertedIndex,
        const std::map<std::string, int>& documentLengths,
        int totalDocs
    );

    std::vector<DocumentScore> rankByTFIDF(const std::vector<std::string>& queryTokens);
    std::vector<DocumentScore> rankByBM25(const std::vector<std::string>& queryTokens);

private:
    const std::unordered_map<std::string, std::map<std::string, int>>& invertedIndex;
    const std::map<std::string, int>& documentLengths;
    int totalDocuments;
    double averageDocumentLength;

    double computeIDF(const std::string& term);
};

#endif 