#pragma once
#include "search_server.h"
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        : search_server_(search_server) {
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::string request;
        bool answer_empty;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int count_request_in_day_ = 0;
    int answer_empty_ = 0;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> documents = search_server_.FindTopDocuments(raw_query, document_predicate);
    if (count_request_in_day_ == min_in_day_) {
        if (requests_.front().answer_empty) {
            --answer_empty_;
            requests_.pop_front();
        }
    }
    else {
        ++count_request_in_day_;
    }
    requests_.push_back({ raw_query, documents.empty() });

    if (documents.empty()) {
        ++answer_empty_;
    }
    return documents;
}