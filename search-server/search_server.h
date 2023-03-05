#pragma once
#include "string_processing.h"
#include <stdexcept>
#include <map>
#include <algorithm>
#include <numeric>
#include "document.h"
#include <iterator>
#include <execution>
#include <string_view>
#include "concurrent_map.h"


using namespace std::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double epsilon = 1e-6;

class SearchServer {
public:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string_view& raw_query) const;

    template <typename Execution, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(Execution _Exec, const std::string_view& raw_query, DocumentPredicate document_predicate) const;

    template <typename Execution>
    std::vector<Document> FindTopDocuments(Execution _Exec, const std::string_view& raw_query, DocumentStatus status) const;

    template <typename Execution>
    std::vector<Document> FindTopDocuments(Execution _Exec, const std::string_view& raw_query) const;


    size_t GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query, int document_id) const;

    template <typename Execution>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(Execution _Exec, const std::string_view& raw_query, int document_id) const;

    int GetDocumentId(int index) const;

    std::vector<int>::iterator begin();

    std::vector<int>::iterator end() ;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    
    void RemoveDocument(int document_id);

    template <typename Execution>
    void RemoveDocument(Execution&& _Exec, int document_id);
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::vector<int> documents_input_;
 
    bool IsStopWord(const std::string_view& word) const;

    static bool IsValidWord(const std::string_view& word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view& text) const;

    template <typename Execution>
    Query ParseQuery(Execution _Exec, const std::string_view& text) const;
 

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;
    
    template <typename Execution, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(Execution&& _Exec, const Query& query,
        DocumentPredicate document_predicate) const;

};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), SearchServer::IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    return FindAllDocuments(std::execution::seq, query, document_predicate);
}

template <typename Execution, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(Execution&& _Exec, const Query& query,
    DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> cm_document_to_relevance(8);
    std::for_each(_Exec, query.plus_words.begin(), query.plus_words.end(),
        [&](const std::string_view& word) {
            if (word_to_document_freqs_.count(static_cast<std::string>(word)) != 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(static_cast<std::string>(word));
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(static_cast<std::string>(word))) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        cm_document_to_relevance[document_id] += term_freq * inverse_document_freq;
                    }
                }
            }
        });

    std::for_each(_Exec, query.minus_words.begin(), query.minus_words.end(),
        [&](const std::string_view& word) {
            if (word_to_document_freqs_.count(static_cast<std::string>(word)) != 0) {
                for (const auto [document_id, _] : word_to_document_freqs_.at(static_cast<std::string>(word))) {
                    cm_document_to_relevance.erase(document_id);
                }
            }
        });

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : cm_document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}


template <typename Execution>
void SearchServer::RemoveDocument(Execution&& _Exec, int document_id) {
    auto it_input = std::find(documents_input_.begin(), documents_input_.end(), document_id);
    if (it_input != documents_input_.end()) {

        documents_input_.erase(it_input, it_input + 1);

        auto& words = document_to_word_freqs_.at(document_id);
        std::vector<std::string_view> vector_words(words.size());
        std::transform(_Exec,
            words.begin(), words.end(),
            vector_words.begin(),
            [](auto& word) {
                return word.first; });
        std::for_each(_Exec,
            vector_words.begin(), vector_words.end(),
            [&](auto& word) {
                auto& documents = word_to_document_freqs_.at(static_cast<std::string>(word));
        documents.erase(document_id);
            }
        );
        documents_.erase(document_id);
        document_to_word_freqs_.erase(document_id);
    }
}

template <typename Execution>
SearchServer::Query SearchServer::ParseQuery(Execution _Exec, const std::string_view& text) const {
    Query query;
    for (const auto& word : SplitIntoWords(text)) {
        const QueryWord query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (!IsValidWord(query_word.data)) {
                throw std::invalid_argument("Word is'nt valid (find)");
            }
            if (query_word.is_minus) {
                if (query_word.data.empty() || query_word.data[0] == '-') {
                    throw std::invalid_argument("Word have ""-"" in begin or empty (find)");
                }
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    if constexpr (std::is_same_v
        <Execution,
        std::execution::sequenced_policy>) {

        std::sort(query.minus_words.begin(), query.minus_words.end());
        auto it_minus = std::unique(query.minus_words.begin(), query.minus_words.end());
        query.minus_words.erase(it_minus, query.minus_words.end());

        std::sort(query.plus_words.begin(), query.plus_words.end());
        auto it_plus = std::unique(query.plus_words.begin(), query.plus_words.end());
        query.plus_words.erase(it_plus, query.plus_words.end());
    }
    return query;

}

template <typename Execution>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(Execution _Exec, const std::string_view& raw_query, int document_id) const {
    const Query query = SearchServer::ParseQuery(_Exec, raw_query);
    std::vector < std::string_view > matched_words;
    if (std::find(documents_input_.begin(), documents_input_.end(), document_id) == documents_input_.end()) {
        throw std::out_of_range("Out of range"s);
    }
    if constexpr (std::is_same_v
        <Execution,
        std::execution::parallel_policy>) {
        if (std::any_of(_Exec, query.minus_words.begin(), query.minus_words.end(), [&](const std::string_view& word) {
            return word_to_document_freqs_.at(static_cast<std::string>(word)).count(document_id);
            })) {
            return { matched_words, documents_.at(document_id).status };
        }
 
        matched_words.resize(query.plus_words.size());
        auto it = std::copy_if(_Exec, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&](const std::string_view& word) {
            return word_to_document_freqs_.at(static_cast<std::string>(word)).count(document_id);
            });
        matched_words.erase(it, matched_words.end());
        std::sort(_Exec,matched_words.begin(), matched_words.end());
        auto it_unique = std::unique(_Exec, matched_words.begin(), matched_words.end());
        matched_words.erase(it_unique, matched_words.end());
 
    }
    else {
        for (const std::string_view& word : query.minus_words) {
             if (word_to_document_freqs_.at(static_cast<std::string>(word)).count(document_id)) {
                return { matched_words, documents_.at(document_id).status };
            }
        }

        for (const std::string_view& word : query.plus_words) {
            if (word_to_document_freqs_.count(static_cast<std::string>(word)) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(static_cast<std::string>(word)).count(document_id)) {
                matched_words.push_back(word);
            }
        }

         }
    return { matched_words, documents_.at(document_id).status };
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const {

    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename Execution, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(Execution _Exec, const std::string_view& raw_query, DocumentPredicate document_predicate) const {

    const Query query = ParseQuery(raw_query);
    std::vector<Document> result = FindAllDocuments(_Exec, query, document_predicate);

    std::sort(_Exec, result.begin(), result.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < epsilon) {
                return lhs.rating > rhs.rating;
            }

    return lhs.relevance > rhs.relevance;

        });
    if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
        result.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return result;
}


template <typename Execution>
std::vector<Document> SearchServer::FindTopDocuments(Execution _Exec, const std::string_view& raw_query) const {
    return SearchServer::FindTopDocuments(_Exec, raw_query, DocumentStatus::ACTUAL);
}

template <typename Execution>
std::vector<Document> SearchServer::FindTopDocuments(Execution _Exec, const std::string_view& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(
        _Exec, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);