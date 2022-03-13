#pragma once

#include "search_server.h"

#include <iostream>

bool map_compare(const std::map<std::string, double>& a, const std::map<std::string, double>& b);
void RemoveDuplicates(SearchServer& search_server);