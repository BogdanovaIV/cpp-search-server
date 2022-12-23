#include"remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::map<std::string, std::set<int>> document_id_words;

    for (const int documet_id : search_server) {
        auto words = search_server.GetWordFrequencies(documet_id);
        std::string str;
        for (auto word : words) {
            str += word.first;
        }
        document_id_words[str].insert(documet_id);
    }
 
    for (auto [str, documents_id] : document_id_words) {
        if (documents_id.size() > 1) {
             for (auto document_id = documents_id.rbegin(); document_id != next(documents_id.rend(),-1); ++document_id) {
                 std::cout << "Found duplicate document id "s << *document_id << std::endl;
                 search_server.RemoveDocument(*document_id);
             }
        }
    }

}
