#pragma once
#include <string>
#include <vector>

#include "models.hpp"

namespace bo_nho {

bool readBlocks(const std::string& filename, std::vector<Hole>& out, std::string& error);

bool readRequests(const std::string& filename, std::vector<Request>& out, std::string& error);

bool writeTrace(const std::string& filename, const std::vector<TraceRecord>& records, std::string& error);


bool writeSummary(const std::string& filename, const std::vector<Metrics>& records, std::string& error);

}
