#pragma once

#include "document.h"
#include "search_server.h"

#include <iostream>
#include <vector>
#include <string>

std::string ReadLine();
int ReadLineWithNumber();

void PrintDocument(const Document& document);
void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);