#include "csv_io.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

static void removeBOM(std::string& str) {
    if (str.size() >= 3 && 
        (unsigned char)str[0] == 0xEF && 
        (unsigned char)str[1] == 0xBB && 
        (unsigned char)str[2] == 0xBF) {
        str = str.substr(3);
    }
}

static std::vector<std::string> splitCSVRow(const std::string& row) {
    std::vector<std::string> result;
    std::stringstream ss(row);
    std::string item;
    while (std::getline(ss, item, ',')) {
        result.push_back(trim(item));
    }
    if (!row.empty() && row.back() == ',') {
        result.push_back("");
    }
    return result;
}

std::vector<Hole> readBlocks(const std::string& filename, std::string& error_msg) {
    std::vector<Hole> holes;
    std::ifstream file(filename);
    if (!file.is_open()) {
        error_msg = "Không thể mở file: " + filename;
        return holes;
    }

    std::string header_line;
    if (!std::getline(file, header_line)) {
        error_msg = "File rỗng hoặc thiếu dòng tiêu đề: " + filename;
        return holes;
    }

    removeBOM(header_line);
    std::vector<std::string> headers = splitCSVRow(header_line);

    int idx_id = -1, idx_start = -1, idx_size = -1;
    for (size_t i = 0; i < headers.size(); ++i) {
        if (headers[i] == "block_id") idx_id = i;
        else if (headers[i] == "start") idx_start = i;
        else if (headers[i] == "size") idx_size = i;
    }

    if (idx_id == -1 || idx_start == -1 || idx_size == -1) {
        error_msg = "File CSV thiếu hoặc sai tên cột bắt buộc (block_id, start, size)";
        return holes;
    }

    std::string line;
    int line_num = 1;
    while (std::getline(file, line)) {
        line_num++;
        line = trim(line);
        if (line.empty()) continue; 

        std::vector<std::string> row = splitCSVRow(line);
        if (row.size() < std::max({idx_id, idx_start, idx_size}) + 1) {
            error_msg = "Dòng " + std::to_string(line_num) + " bị thiếu trường dữ liệu.";
            return holes;
        }

        try {
            Hole h;
            h.block_id = row[idx_id];
            h.start = std::stoi(row[idx_start]);
            h.size = std::stoi(row[idx_size]);
            holes.push_back(h);
        } catch (...) {
            error_msg = "Dòng " + std::to_string(line_num) + " chứa dữ liệu sai định dạng số.";
            return holes;
        }
    }
    return holes;
}

std::vector<Request> readRequests(const std::string& filename, std::string& error_msg) {
    std::vector<Request> requests;
    std::ifstream file(filename);
    if (!file.is_open()) {
        error_msg = "Không thể mở file: " + filename;
        return requests;
    }

    std::string header_line;
    if (!std::getline(file, header_line)) {
        error_msg = "File rỗng hoặc thiếu dòng tiêu đề: " + filename;
        return requests;
    }

    removeBOM(header_line);
    std::vector<std::string> headers = splitCSVRow(header_line);

    int idx_order = -1, idx_pid = -1, idx_size = -1;
    for (size_t i = 0; i < headers.size(); ++i) {
        if (headers[i] == "order") idx_order = i;
        else if (headers[i] == "process_id") idx_pid = i;
        else if (headers[i] == "size") idx_size = i;
    }

    if (idx_order == -1 || idx_pid == -1 || idx_size == -1) {
        error_msg = "File CSV thiếu hoặc sai tên cột bắt buộc (order, process_id, size)";
        return requests;
    }

    std::string line;
    int line_num = 1;
    while (std::getline(file, line)) {
        line_num++;
        line = trim(line);
        if (line.empty()) continue;

        std::vector<std::string> row = splitCSVRow(line);
        if (row.size() < std::max({idx_order, idx_pid, idx_size}) + 1) {
            error_msg = "Dòng " + std::to_string(line_num) + " bị thiếu trường dữ liệu.";
            return requests;
        }

        try {
            Request r;
            r.order = std::stoi(row[idx_order]);
            r.process_id = row[idx_pid];
            r.size = std::stoi(row[idx_size]);
            requests.push_back(r);
        } catch (...) {
            error_msg = "Dòng " + std::to_string(line_num) + " chứa dữ liệu sai định dạng số.";
            return requests;
        }
    }
    return requests;
}

bool writeTrace(const std::string& filename, const std::vector<TraceRecord>& records, std::string& error_msg) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        error_msg = "Không thể tạo hoặc ghi file: " + filename;
        return false;
    }

    file << "step,algorithm,process_id,request_size,block_id,alloc_start,block_size_before,block_size_after,status\n";

    for (const auto& r : records) {
        file << r.step << ","
             << r.algorithm << ","
             << r.process_id << ","
             << r.request_size << ","
             << r.block_id << ","       
             << r.alloc_start << ","    
             << r.block_size_before << ","
             << r.block_size_after << ","
             << r.status << "\n";
    }
    return true;
}

bool writeSummary(const std::string& filename, const std::vector<SummaryRecord>& records, std::string& error_msg) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        error_msg = "Không thể tạo hoặc ghi file: " + filename;
        return false;
    }

    file << "algorithm,allocated,failed,total_free,largest_free,time_us\n";

    for (const auto& r : records) {
        file << r.algorithm << ","
             << r.allocated << ","
             << r.failed << ","
             << r.total_free << ","
             << r.largest_free << ","
             << r.time_us << "\n";
    }
    return true;
}
