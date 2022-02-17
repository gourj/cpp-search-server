#include "request_queue.h"
    
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        const auto result = RequestQueue::search_server_.FindTopDocuments(raw_query, status);
        RequestQueue::AddRequest(result.size());
        return result;
    }
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
        const auto result = RequestQueue::search_server_.FindTopDocuments(raw_query);
        RequestQueue::AddRequest(result.size());
        return result;
    }
    int RequestQueue::GetNoResultRequests() const {
        return RequestQueue::no_results_requests_;
    }

    void RequestQueue::AddRequest(int results_num) {
        ++RequestQueue::current_time_;
        while (!RequestQueue::requests_.empty() && RequestQueue::min_in_day_ <= RequestQueue::current_time_ - RequestQueue::requests_.front().timestamp) {
            if (0 == RequestQueue::requests_.front().results) {
                --RequestQueue::no_results_requests_;
            }
            RequestQueue::requests_.pop_front();
        }
        RequestQueue::requests_.push_back({current_time_, results_num});
        if (0 == results_num) {
            ++RequestQueue::no_results_requests_;
        }
    }