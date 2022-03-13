#include "remove_duplicates.h"

bool map_compare(const std::map<std::string, double>& a, const std::map<std::string, double>& b) {
    if (a.size() != b.size()){
        return false;
    }
    for (auto [word,_] : a) {
        if (b.count(word) == 0) {
            return false;
        }
    }
    return true;
}

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> ids_to_delete;
    for (const int document_id : search_server) {
        auto it = find_if(search_server.begin(), search_server.end(), 
        [search_server, document_id](const int a)
        { return (a != document_id && map_compare(search_server.GetWordFrequencies(document_id), search_server.GetWordFrequencies(a)));});
        if (it != search_server.end()) {
            if (*it > document_id) {
                ids_to_delete.insert(*it);
            }
            else {
                ids_to_delete.insert(document_id);
            }
        }
    }
    for (const int document_id : ids_to_delete){
        search_server.RemoveDocument(document_id);
        std::cout << "Found duplicate document id "<< document_id << std::endl;
    }
}