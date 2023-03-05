#include"request_queue.h"
#include<iterator>
#include<execution>
#include<list>


std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    
    std::vector<std::vector<Document>> result(queries.size());
    
    std::transform(std::execution::par, queries.cbegin(), queries.cend(), result.begin(), [&search_server](const std::string& raw_query) {return search_server.FindTopDocuments(raw_query); });

    return result;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> result_vector = ProcessQueries(search_server, queries);
    std::list<Document> result;
    for(auto& vec_documents : result_vector){
        for (auto& doc : vec_documents) {
            result.push_back(std::move(doc));
        }
    }
    return result;
}