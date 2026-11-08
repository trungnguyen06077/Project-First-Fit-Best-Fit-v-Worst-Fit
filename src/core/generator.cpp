#include "generator.hpp"

#include <fstream>
#include <random>

namespace bo_nho {

std::vector<Hole> generateHoles(int numHoles, int minSize, int maxSize, unsigned int seed) {
    std::vector<Hole> holes;
    if (numHoles <= 0 || minSize <= 0 || maxSize < minSize) return holes;

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(minSize, maxSize);

    long long currentStart = 0;
    for (int i = 0; i < numHoles; ++i) {
        Hole h;
        h.id = "B" + std::to_string(i + 1);
        h.start = currentStart;
        h.size = dist(rng);
        holes.push_back(h);
        currentStart += h.size;
    }
    return holes;
}

std::vector<Request> generateRequests(int numRequests, int minSize, int maxSize, unsigned int seed) {
    std::vector<Request> requests;
    if (numRequests <= 0 || minSize <= 0 || maxSize < minSize) return requests;

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(minSize, maxSize);

    for (int i = 0; i < numRequests; ++i) {
        Request r;
        r.order = i + 1;
        r.processId = "P" + std::to_string(i + 1);
        r.size = dist(rng);
        requests.push_back(r);
    }
    return requests;
}

bool writeHolesToCSV(const std::string& filepath, const std::vector<Hole>& holes) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    file << "block_id,start,size\n";
    for (const auto& h : holes) file << h.id << ',' << h.start << ',' << h.size << '\n';
    return file.good();
}

bool writeRequestsToCSV(const std::string& filepath, const std::vector<Request>& requests) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    file << "order,process_id,size\n";
    for (const auto& r : requests) file << r.order << ',' << r.processId << ',' << r.size << '\n';
    return file.good();
}

}  // namespace bo_nho
