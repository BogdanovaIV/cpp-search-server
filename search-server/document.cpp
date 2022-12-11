#include "document.h"
#include <string>

using namespace std::string_literals;

void PrintDocument(const Document& document) {
    
    std::cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << std::endl;
}

std::ostream& operator<<(std::ostream& os, const Document& doc) {

    os << "{ "s
        << "document_id = "s << doc.id << ", "s
        << "relevance = "s << doc.relevance << ", "s
        << "rating = "s << doc.rating << " }"s;

    return os;
}