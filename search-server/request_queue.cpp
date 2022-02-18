#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server)
    , no_results_requests_(0)
    , current_time_(0) {
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus request_status) {
    return RequestQueue::AddFindRequest(raw_query,
                            [request_status](int document_id, DocumentStatus status, int rating) { 
                                return status == request_status; });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
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