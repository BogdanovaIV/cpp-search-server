#include "search_server.h"
#include <cmath>

void SearchServer::AddDocument(int document_id, const std::string & document, DocumentStatus status, const std::vector<int>&ratings) {
    if (document_id <= INVALID_DOCUMENT_ID) {
        throw std::invalid_argument("ID can't be a negative number");
    }
    else if (documents_.count(document_id)) {
        throw std::invalid_argument("ID already added");
    }
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word is'nt valid (documaent)");
        }
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, SearchServer::DocumentData{ComputeAverageRating(ratings), status});
    documents_input_.push_back(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string & raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string & raw_query, int document_id) const {
    const Query query = SearchServer::ParseQuery(raw_query);
    std::vector < std::string > matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

int SearchServer::GetDocumentId(int index) const {
    return documents_input_.at(index);
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string & text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        const QueryWord query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (!IsValidWord(query_word.data)) {
                throw std::invalid_argument("Word is'nt valid (find)");
            }
            if (query_word.is_minus) {
                if (query_word.data.empty() || query_word.data[0] == '-') {
                    throw std::invalid_argument("Word have ""-"" in begin or empty (find)");
                }
                query.minus_words.insert(query_word.data);
            }
            else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsValidWord(const std::string& word) {
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

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {

    if (document_to_word_freqs_.count(document_id)) {
        
        auto it = find(documents_input_.begin(), documents_input_.end(), document_id);
        documents_input_.erase(it, it+1);

        for (auto str : document_to_word_freqs_.at(document_id)) {
            auto documents = word_to_document_freqs_.at(str.first);
            documents.erase(documents.find(document_id));
        }

        documents_.erase(documents_.find(document_id));

        document_to_word_freqs_.erase(document_to_word_freqs_.find(document_id));
    }
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

