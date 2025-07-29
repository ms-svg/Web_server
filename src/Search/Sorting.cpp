#include "Sorting.hpp"
#include <unordered_set>
#include <algorithm>
#include <iostream>

std::vector<std::string> common_word = {"is","a","the","and","to","of","in","that","it","for","on","with","as","was","by","this","an","be","at","from"};
int querylength = 0;
Sorting::Sorting(
    const std::unordered_map<std::string, std::map<std::string, int>>& index,
    const std::map<std::string, int>& lengths,
    int totalDocs
) : invertedIndex(index), documentLengths(lengths), totalDocuments(totalDocs) {
    int totalLength = 0;
    for (const auto& [doc, len] : documentLengths) {
        totalLength += len;
    }
    averageDocumentLength = totalDocuments > 0 ? static_cast<double>(totalLength) / totalDocuments : 0.0;
}

double Sorting::computeIDF(const std::string& term) {
    auto it = invertedIndex.find(term);
    if (it == invertedIndex.end()) return 0.0;

    int df = it->second.size();
    return std::log((totalDocuments - df + 0.5) / (df + 0.5) + 1.0);  // BM25-style IDF
}

std::vector<DocumentScore> Sorting::rankByTFIDF(const std::vector<std::string>& queryTokens) {
    std::map<std::string, double> docScores;

    querylength = queryTokens.size();
    int queryMatchCount = querylength;
    std::unordered_set<std::string> uniqueTerms(queryTokens.begin(), queryTokens.end());
    for (const std::string& term : uniqueTerms) {
        if (std::find(common_word.begin(), common_word.end(), term) != common_word.end()) {
            continue;  // Skip common words
        }
        double idf = computeIDF(term);
        
        if (idf == 0.0) {
            std::cout << "Term '" << term << "' has zero IDF, skipping." << std::endl;
            queryMatchCount--;
            continue;
        }

        auto postings = invertedIndex.find(term);
        if (postings == invertedIndex.end()) continue;

        for (const auto& [doc, tf] : postings->second) {
            double tfidf = tf * idf;
            docScores[doc] += tfidf;
        }
    }

    std::vector<DocumentScore> results;
    for (const auto& [doc, score] : docScores) {
        results.push_back({doc, score});
    }

    std::sort(results.begin(), results.end());
    return results;
}

std::vector<DocumentScore> Sorting::rankByBM25(const std::vector<std::string>& queryTokens) {
    const double k1 = 1.5;
    const double b = 0.75;

    std::map<std::string, double> docScores;

    std::unordered_set<std::string> uniqueTerms(queryTokens.begin(), queryTokens.end());
    for (const std::string& term : uniqueTerms) {
        std::cout << "Searching for term: " << term << std::endl;

        auto it = invertedIndex.find(term);
        if (it == invertedIndex.end()) continue;

        double idf = computeIDF(term);

        for (const auto& [doc, tf] : it->second) {
            int dl = documentLengths.at(doc);
            double denom = tf + k1 * (1 - b + b * ((double)dl / averageDocumentLength));
            double score = idf * ((tf * (k1 + 1)) / denom);
            docScores[doc] += score;
        }
    }

    std::vector<DocumentScore> results;
    for (const auto& [doc, score] : docScores) {
        results.push_back({doc, score});
    }

    std::sort(results.begin(), results.end());
    return results;
}