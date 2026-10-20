#ifndef GENERATOR_HPP
#define GENERATOR_HPP
#include <vector>
#include <string>
#include "models.hpp"

namespace core {

    std::vector<Hole> generateHoles(
        int num_holes,
        int min_size,
        int max_size,
        unsigned int seed
    );

    std::vector<Request> generateRequests(
        int num_requests,
        int min_size,
        int max_size,
        unsigned int seed
    );

    bool writeHolesToCSV(
        const std::string& filepath,
        const std::vector<Hole>& holes
    );

    bool writeRequestsToCSV(
        const std::string& filepath,
        const std::vector<Request>& requests
    );

}

#endif
