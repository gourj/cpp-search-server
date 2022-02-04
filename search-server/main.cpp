#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <stdexcept>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6; //точность сравнения релевантностей документов

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } 
        else {
            word += c;  
        }          
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

struct Document {
    
    Document() {
        id = 0;
        relevance = 0.0;
        rating = 0;
    }
    
    Document(int a, double b, int c) {
        id = a;
        relevance = b;
        rating = c;
    }

    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};


class SearchServer {
public:

    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    SearchServer() = default;

    explicit SearchServer(const string& stop_words) {
        SearchServer(SplitIntoWords(stop_words));
    }

    template <typename StringCollection>
    explicit SearchServer(const StringCollection& stop_words) {
         for (const string& word : stop_words) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Invalid characters"s);
            }
            if (word.empty() || stop_words_.count(word) == 1 ) {
                continue;
            }
            stop_words_.insert(word);
        }
    }
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Invalid characters"s);
            }
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0 || count(doc_ids_.begin(), doc_ids_.end(), document_id) > 0) {
            throw invalid_argument("Invalid id"s);
        }
        doc_ids_.push_back(document_id);
        const vector<string> words = SplitIntoWordsNoStop(document);
        for (const string& word : words) {
            documents_[word][document_id] += (1.0 / words.size());
        }
        rating_status_.emplace(document_id, DocumentData {ComputeAverageRating(ratings), status});
    }

    int GetDocumentId(int index) const {
        if (index < 0 || index >= GetDocumentCount()) {
            throw out_of_range("Invalid index"s);
        }
        else {
            return doc_ids_[index];
        }
    }

    template <typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query, Predicate predicate) const {
        Query query_words = ParseQuery(raw_query);                
        auto matched_documents = FindAllDocuments(query_words, predicate);
        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return (lhs.relevance > rhs.relevance) || (abs(lhs.relevance - rhs.relevance) < EPSILON && lhs.rating > rhs.rating);
            });                
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus request_status) const {
        return FindTopDocuments(raw_query,
                                [request_status](int document_id, DocumentStatus status, int rating) { 
                                    return status == request_status; });
    } 

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return rating_status_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        vector<string> words;
        Query query_words = ParseQuery(raw_query);
        for(const string& word : query_words.minus_words) {
            if(documents_.count(word)) {
                if (documents_.at(word).count(document_id)) {
                    return tuple(words, rating_status_.at(document_id).status);
                }
            }
        }
        for (const string& word : query_words.plus_words) {           
            if (documents_.count(word)) {
                if (documents_.at(word).count(document_id)) {
                    words.push_back(word);
                }
            }
        }
        return tuple(words, rating_status_.at(document_id).status);
    }

private:
    
    map<string, map<int, double>> documents_;
    vector<int> doc_ids_;
    set<string> stop_words_;    
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    map<int, DocumentData> rating_status_;
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    bool IsMinusWord(const string& word) const {
        if (word[0] == '-') {
            if (word.size() == 1) {
                throw invalid_argument("No text after '-'"s);
            } 
            else if (word[1] == '-') {
                throw invalid_argument("More than 1 '-'"s);
            } 
            else {
                return true;
            } 
        }
        else {
            return false;
        }
        
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (IsValidWord(word)) {
                if (!IsStopWord(word)) {
                    words.push_back(word);
                }
            }
            else {
                throw invalid_argument("Invalid characters"s);
            }
        }
        return words;
    }

    static bool IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
                return c >= '\0' && c < ' ';
            }
        );
    }

    Query ParseQuery(const string& query) const {
        Query result;
        for (const string& word : SplitIntoWordsNoStop(query)) {
            if (IsMinusWord(word) && !IsStopWord(word.substr(1))) {  
                result.minus_words.insert(word.substr(1));
            }
            else {
                result.plus_words.insert(word);
            }            
        }
        return result;
    }
    
    double CalculateIDF(const string& word) const {
        double idf = 0.0;
        if (documents_.count(word)) {
            idf = log((GetDocumentCount() * 1.0) / documents_.at(word).size());
        }
        return idf;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        return (accumulate(ratings.begin(), ratings.end(), 0)) / (static_cast<int>(ratings.size()));
    }

    template <typename Predicate>
    vector<Document> FindAllDocuments(const Query& query_words, Predicate predicate) const {
        vector<Document> matched_documents;
        map<int, double> document_to_relevance;
        for (const auto& word : query_words.plus_words) {            
            if (documents_.count(word) > 0) {
                for (const auto& [document_id, term_freq] : documents_.at(word)) {                    
                    DocumentData rs = rating_status_.at(document_id);                 
                    if (predicate(document_id, rs.status, rs.rating)) {
                        document_to_relevance[document_id] += term_freq * CalculateIDF(word);
                    }
                }
            }
        }
        for (const auto& word : query_words.minus_words) {                
            if (documents_.count(word) > 0) {
                for (const auto& [document_id, term_freq] : documents_.at(word)) {
                    document_to_relevance.erase(document_id);
                }
            }
        }
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance, rating_status_.at(document_id).rating});
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
                 const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const exception& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const exception& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const exception& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}

int main() {
    SearchServer search_server("и в на"s);

    AddDocument(search_server, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, -1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
    AddDocument(search_server, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 1, 1});

    FindTopDocuments(search_server, "пушистый -пёс"s);
    FindTopDocuments(search_server, "пушистый --кот"s);
    FindTopDocuments(search_server, "пушистый -"s);

    MatchDocuments(search_server, "пушистый пёс"s);
    MatchDocuments(search_server, "модный -кот"s);
    MatchDocuments(search_server, "модный --пёс"s);
    MatchDocuments(search_server, "пушистый - хвост"s);
} 