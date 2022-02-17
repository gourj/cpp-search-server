#include "document.h"

Document::Document(int id, double relevance, int rating) {
    Document::id = id;
    Document::relevance = relevance;
    Document::rating = rating;
}

std::ostream& operator<<(std::ostream& out, const Document& document) {
    using namespace std;
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}