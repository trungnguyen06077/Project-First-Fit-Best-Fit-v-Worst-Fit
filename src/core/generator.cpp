#include "generator.hpp"
#include <random>
#include <fstream>

namespace core {
    std::vector<Hole> generateHoles(
        int num_holes,
        int min_size,
        int max_size,
        unsigned int seed
    ) {
        std::vector<Hole> holes;

        if (num_holes <= 0 || min_size <= 0 || max_size < min_size) {
            return holes;
        }

        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> dist(min_size, max_size);

        int current_start = 0;

        for (int i = 0; i < num_holes; ++i) {
            int size = dist(rng);

            Hole hole;
            hole.id = i + 1;
            hole.start = current_start;
            hole.size = size;

            holes.push_back(hole);
            current_start += size;
        }

        return holes;
    }

    std::vector<Request> generateRequests(
        int num_requests,
        int min_size,
        int max_size,
        unsigned int seed
    ) {
        std::vector<Request> requests;

        if (num_requests <= 0 || min_size <= 0 || max_size < min_size) {
            return requests;
        }

        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> dist(min_size, max_size);

        for (int i = 0; i < num_requests; ++i) {
            Request request;
            request.order = i + 1;
            request.process_id = i + 1;
            request.size = dist(rng);

            requests.push_back(request);
        }

        return requests;
    }

    bool writeHolesToCSV(
        const std::string& filepath,
        const std::vector<Hole>& holes
    ) {
        std::ofstream file(filepath);

        if (!file.is_open()) {
            return false;
        }

        file << "id,start,size\n";

        for (const auto& hole : holes) {
            file << hole.id << ","
                << hole.start << ","
                << hole.size << "\n";
        }

        return file.good();
    }

    bool writeRequestsToCSV(
        const std::string& filepath,
        const std::vector<Request>& requests
    ) {
        std::ofstream file(filepath);

        if (!file.is_open()) {
            return false;
        }

        file << "order,process_id,size\n";

        for (const auto& request : requests) {
            file << request.order << ","
                << request.process_id << ","
                << request.size << "\n";
        }

        return file.good();
    }
}
