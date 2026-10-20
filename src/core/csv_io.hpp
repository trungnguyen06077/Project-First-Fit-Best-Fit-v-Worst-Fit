#ifndef CSV_IO_HPP
#define CSV_IO_HPP

#include <string>
#include <vector>

struct Hole {
    std::string block_id;
    int start;
    int size;
};

struct Request {
    int order;
    std::string process_id;
    int size;
};

struct TraceRecord {
    int step;
    std::string algorithm;
    std::string process_id;
    int request_size;
    std::string block_id;       
    std::string alloc_start;    
    int block_size_before;
    int block_size_after;
    std::string status;         
};

struct SummaryRecord {
    std::string algorithm;
    int allocated;
    int failed;
    int total_free;
    int largest_free;
    long long time_us;
};

std::vector<Hole> readBlocks(const std::string& filename, std::string& error_msg);

std::vector<Request> readRequests(const std::string& filename, std::string& error_msg);

bool writeTrace(const std::string& filename, const std::vector<TraceRecord>& records, std::string& error_msg);

bool writeSummary(const std::string& filename, const std::vector<SummaryRecord>& records, std::string& error_msg);

#endif 
