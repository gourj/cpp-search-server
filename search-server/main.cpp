#include <iostream>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
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
        rating_status_.emplace(document_id, DocumentData {ComputeAverageRating(ratings), status});
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

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Function>
void RunTestImpl(const Function& f, const string& f_str) {
    f();        
    cerr << f_str << " OK"s << endl;
    
}

#define RUN_TEST(func)  RunTestImpl((func), #func)

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1ull);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}
//Тест проверяет, что документы содержащие минус слова исключены из выдачи
void TestExcludeMinusWordsFromSearch() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2 ,3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(21, "cat at the city"s, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat -in"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1ull, "Quantity of added documents is incorrect"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, 21, "Document`s ID is incorrect"s);
    }
}
//Тест проверяет соответствие слов запроса словам в документах
void TestDocumentMatching() {
    SearchServer server;
    server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(2, "dog at the table"s, DocumentStatus::ACTUAL, {4, 5, 6});
    const auto doc1 = server.MatchDocument("cat city", 1);
    const auto doc2 = server.MatchDocument("dog -table", 2);
    ASSERT_EQUAL_HINT(get<0>(doc1).size(), 2ull, "Quantity of matched words is incorrect"s);
    ASSERT_EQUAL(get<0>(doc1)[0], "cat"s);
    ASSERT_EQUAL(get<0>(doc1)[1], "city"s);
    ASSERT_HINT(get<0>(doc2).empty(), "Documnents containing minus words must be excluded"s);
}
//Тест проверяет корректность сортировки по разным параметром и правильность их вычисления
void TestDocumentSortingByRelevanceAndRatingCalculation() {
    SearchServer server;
    server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, {-1, -2, 3});
    server.AddDocument(2, "dog at the table"s, DocumentStatus::ACTUAL, {4, 2, 3});
    server.AddDocument(3, "parrot at the table in a cage"s, DocumentStatus::ACTUAL, {2, 5, 3});
    const vector<Document> search_results = server.FindTopDocuments("parrot in the city"s);
    
    //Relevance testing
    ASSERT_EQUAL_HINT(search_results[0].id, 1, "Document ID is incorrect"s);
    ASSERT_EQUAL_HINT(search_results[1].id, 3, "Document ID is incorrect"s);
    ASSERT_EQUAL_HINT(search_results[2].id, 2, "Document ID is incorrect"s);
    ASSERT_HINT(search_results[0].relevance > search_results[1].relevance, "Sorting by relevance is incorrect"s);
    ASSERT_HINT(search_results[0].relevance > search_results[2].relevance, "Sorting by relevance is incorrect"s);
    ASSERT_HINT(search_results[1].relevance > search_results[2].relevance, "Sorting by relevance is incorrect"s);
    
    //Relevance calculation testing
    ASSERT_HINT((search_results[0].relevance - 0.376019349) < 1e-6, "Relevance value calculation is incorrect"s);
    ASSERT_HINT((search_results[1].relevance - 0.2148682) < 1e-6, "Relevance value calculation is incorrect"s);
    ASSERT_HINT((search_results[2].relevance - 0.0) < 1e-6, "Relevance valeu calculation is incorrect"s);
    
    //Rating testing
    ASSERT_EQUAL_HINT(search_results[0].rating, 0, "Rating value is incorrect"s);
    ASSERT_EQUAL_HINT(search_results[2].rating, 3, "Rating value is incorrect"s);
    ASSERT_EQUAL_HINT(search_results[1].rating, 3, "Rating value is incorrect"s);
}
//Тест проверяет работу фильтра выдачи
void TestSearchResultsFiltration() {
    SearchServer server;
    server.AddDocument(1, "white cat and fancy collar"s,        DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(2, "fluffy cat fluffy tale"s,       DocumentStatus::IRRELEVANT, {7, 2, 7});
    server.AddDocument(3, "groomed dog expressive eyes"s, DocumentStatus::REMOVED, {5, -12, 2, 1});
    server.AddDocument(4, "groomed starling eugene"s,         DocumentStatus::BANNED, {9});

    const vector<Document> filter_by_id = server.FindTopDocuments("fluffy groomed cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
    ASSERT_EQUAL_HINT(filter_by_id.size(), 2ull, "Filtratoin by ID via predicate function has failed"s);

    const vector<Document> filter_by_rating = server.FindTopDocuments("fluffy groomed cat"s, [](int document_id, DocumentStatus status, int rating) { return rating > 4; });
    ASSERT_EQUAL_HINT(filter_by_rating.size(), 2ull, "Filtration by rating via predicate function has failed"s);
    ASSERT_EQUAL_HINT(filter_by_rating[0].id, 2, "Filtration by rating via predicate function has failed"s);
    ASSERT_EQUAL_HINT(filter_by_rating[1].id, 4, "Filtration by rating via predicate function has failed"s);

    const vector<Document> filter_by_status1 = server.FindTopDocuments("fluffy groomed cat"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
    const vector<Document> filter_by_status2 = server.FindTopDocuments("fluffy groomed cat"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::IRRELEVANT; });
    const vector<Document> filter_by_status3 = server.FindTopDocuments("fluffy groomed cat"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::REMOVED; });
    const vector<Document> filter_by_status4 = server.FindTopDocuments("fluffy groomed cat"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
    ASSERT_HINT(filter_by_status1.size() == 1 && filter_by_status1[0].id == 1, "Filtration by DocumentStatus::ACTUAL via predicate function has failed"s);
    ASSERT_HINT(filter_by_status2.size() == 1 && filter_by_status2[0].id == 2, "Filtration by DocumentStatus::IRRELEVANT via predicate function has failed"s);
    ASSERT_HINT(filter_by_status3.size() == 1 && filter_by_status3[0].id == 3, "Filtration by DocumentStatus::REMOVED via predicate function has failed"s);
    ASSERT_HINT(filter_by_status4.size() == 1 && filter_by_status4[0].id == 4, "Filtration by DocumentStatus::BANNED via predicate function has failed"s);

    const vector<Document> filter_by_status5 = server.FindTopDocuments("fluffy groomed cat"s, DocumentStatus::ACTUAL);
    ASSERT_HINT(filter_by_status5.size() == 1 && filter_by_status5[0].id == 1, "Filtration by document status as second argument has failed"s);

}

//Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromSearch);
    RUN_TEST(TestDocumentMatching);
    RUN_TEST(TestDocumentSortingByRelevanceAndRatingCalculation);
    RUN_TEST(TestSearchResultsFiltration);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}