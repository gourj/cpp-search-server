#include "document.h"

Document::Document(int _id, double _relevance, int _rating) {
    id = _id;
    relevance = _relevance;
    rating = _rating;
}

std::ostream& operator<<(std::ostream& out, const Document& document) {
    using namespace std;
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}