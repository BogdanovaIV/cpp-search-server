#include "search_server.h"
#include <cmath>
#include <execution>

void SearchServer::AddDocument(int document_id, const std::string_view & document, DocumentStatus status, const std::vector<int>&ratings) {
    if (document_id <= INVALID_DOCUMENT_ID) {
        throw std::invalid_argument("ID can't be a negative number");
    }
    else if (documents_.count(document_id)) {
        throw std::invalid_argument("ID already added");
    }
    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const auto& word : words) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word is'nt valid (documaent)");
        }
        std::string str = static_cast<std::string>(word);
        std::string_view str_view = str;
        word_to_document_freqs_[std::move(str)][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][str_view] += inv_word_count;
    }
    
    documents_.emplace(document_id, SearchServer::DocumentData{ComputeAverageRating(ratings), status});
    documents_input_.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view & raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(
        std::execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const {
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}

int SearchServer::GetDocumentId(int index) const {
    return documents_input_.at(index);
}

bool SearchServer::IsStopWord(const std::string_view& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view & text) const {
    std::vector<std::string_view> words;
    for (const auto& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word.substr(0, word.size()));
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const {
    return SearchServer::ParseQuery(std::execution::seq, text);
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsValidWord(const std::string_view& word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    SearchServer::RemoveDocument(std::execution::seq, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query, int document_id) const {
    return SearchServer::MatchDocument(std::execution::seq, raw_query, document_id);
}

std::vector<int>::iterator  SearchServer::begin() {
    return documents_input_.begin();
}

std::vector<int>::iterator SearchServer::end() {
    return documents_input_.end();
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}

