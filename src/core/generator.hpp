#pragma once
#include <string>
#include <vector>

#include "models.hpp"

namespace bo_nho {

std::vector<Hole> generateHoles(int numHoles, int minSize, int maxSize, unsigned int seed);
std::vector<Request> generateRequests(int numRequests, int minSize, int maxSize, unsigned int seed);

bool writeHolesToCSV(const std::string& filepath, const std::vector<Hole>& holes);
bool writeRequestsToCSV(const std::string& filepath, const std::vector<Request>& requests);

} 
