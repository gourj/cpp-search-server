#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

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
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus{
        ACTUAL,
        IRRELEVANT,
        BANNED,
        REMOVED
    };


class SearchServer {
public:

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        for (const string& word : words) {
            documents_[word][document_id] += (1.0 / words.size());
        }
        document_rating_[document_id] = ComputeAverageRating(ratings);
        document_status_[document_id] = status;
        document_count_++;
    }

    template <typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query, Predicate predicate) const {
        const Query query_words = ParseQuery(raw_query);
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus request_status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query,
                                [request_status](int document_id, DocumentStatus status, int rating) { 
                                    return status == request_status; });
    } 

    int GetDocumentCount() {
        return document_count_;
    }

      tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        vector<string> words;
        Query query_words = ParseQuery(raw_query);
        for(const string& word : query_words.minus_words) {
            if(documents_.count(word)){
                if (documents_.at(word).count(document_id)) {
                    return tuple(words, document_status_.at(document_id));
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
        return tuple(words, document_status_.at(document_id));
    }

private:
    
    map<string, map<int, double>> documents_;
    set<string> stop_words_;    
    int document_count_ = 0;
    map<int, int> document_rating_;
    map<int, DocumentStatus> document_status_;

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    bool IsMinusWord(const string& word) const {
        return word[0] == '-';
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (IsMinusWord(word) && !IsStopWord(word.substr(1))) {  
                query_words.minus_words.insert(word.substr(1));
            } 
            else {
                query_words.plus_words.insert(word);  
            }          
        }
        return query_words;
    }
    
    double CalculateIDF(const string& word) const {
        double idf = 0.0;
        if (documents_.count(word)) {
            idf = log((document_count_ * 1.0) / documents_.at(word).size());
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
            if (documents_.count(word)) {
                for (const auto& [document_id, term_freq] : documents_.at(word)) {                    
                //Если я правильно понял, вы предлагаете объеденить данные о рейтинге и статусе в один контейнер?
                //И в условии цикла распаковать его, вытащить из него id, статус, рейтинг и сразу передать в функцию-предикат?                 
                    if (predicate(document_id, document_status_.at(document_id), document_rating_.at(document_id))) {
                        document_to_relevance[document_id] += term_freq * CalculateIDF(word);
                    }
                }
            }
        }
        for (const auto& word : query_words.minus_words) {                
            if (documents_.count(word)){
                for (const auto& [document_id, term_freq] : documents_.at(word)) {
                    document_to_relevance.erase(document_id);
                }
            }
        }
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance, document_rating_.at(document_id)});
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
}