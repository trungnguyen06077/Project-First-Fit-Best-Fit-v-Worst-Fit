#include "csv_io.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace bo_nho {
namespace {

std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

void removeBOM(std::string& s) {
    if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB &&
        (unsigned char)s[2] == 0xBF)
        s.erase(0, 3);
}

std::vector<std::string> splitRow(const std::string& row) {
    std::vector<std::string> out;
    std::string item;
    std::stringstream ss(row);
    while (std::getline(ss, item, ',')) out.push_back(trim(item));
    if (!row.empty() && row.back() == ',') out.push_back("");
    return out;
}

bool parseLL(const std::string& s, long long& v) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        v = std::stoll(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

int findCol(const std::vector<std::string>& headers, std::initializer_list<const char*> names) {
    for (size_t i = 0; i < headers.size(); ++i)
        for (const char* n : names)
            if (headers[i] == n) return (int)i;
    return -1;
}

std::string lineErr(const std::string& file, int line, const std::string& msg) {
    return file + ", dong " + std::to_string(line) + ": " + msg;
}

}  // namespace

bool readBlocks(const std::string& filename, std::vector<Hole>& out, std::string& error) {
    out.clear();
    std::ifstream file(filename);
    if (!file.is_open()) { error = "Khong the mo file: " + filename; return false; }

    std::string header;
    if (!std::getline(file, header)) { error = "File rong hoac thieu dong tieu de: " + filename; return false; }
    removeBOM(header);
    auto headers = splitRow(trim(header));

    int iId = findCol(headers, {"block_id", "id"});
    int iStart = findCol(headers, {"start"});
    int iSize = findCol(headers, {"size"});
    if (iId < 0 || iStart < 0 || iSize < 0) {
        error = filename + ": thieu cot bat buoc (block_id, start, size)";
        return false;
    }
    size_t need = (size_t)std::max({iId, iStart, iSize}) + 1;

    std::string line;
    int lineNum = 1;
    while (std::getline(file, line)) {
        ++lineNum;
        line = trim(line);
        if (line.empty()) continue;
        auto row = splitRow(line);
        if (row.size() < need) { error = lineErr(filename, lineNum, "thieu truong du lieu"); return false; }
        Hole h;
        h.id = row[iId];
        h.line = lineNum;
        if (!parseLL(row[iStart], h.start) || !parseLL(row[iSize], h.size)) {
            error = lineErr(filename, lineNum, "du lieu sai dinh dang so");
            return false;
        }
        out.push_back(h);
    }
    return true;
}

bool readRequests(const std::string& filename, std::vector<Request>& out, std::string& error) {
    out.clear();
    std::ifstream file(filename);
    if (!file.is_open()) { error = "Khong the mo file: " + filename; return false; }

    std::string header;
    if (!std::getline(file, header)) { error = "File rong hoac thieu dong tieu de: " + filename; return false; }
    removeBOM(header);
    auto headers = splitRow(trim(header));

    int iOrder = findCol(headers, {"order"});
    int iPid = findCol(headers, {"process_id"});
    int iSize = findCol(headers, {"size"});
    if (iOrder < 0 || iPid < 0 || iSize < 0) {
        error = filename + ": thieu cot bat buoc (order, process_id, size)";
        return false;
    }
    size_t need = (size_t)std::max({iOrder, iPid, iSize}) + 1;

    std::string line;
    int lineNum = 1;
    while (std::getline(file, line)) {
        ++lineNum;
        line = trim(line);
        if (line.empty()) continue;
        auto row = splitRow(line);
        if (row.size() < need) { error = lineErr(filename, lineNum, "thieu truong du lieu"); return false; }
        Request r;
        long long order = 0;
        r.processId = row[iPid];
        r.line = lineNum;
        if (!parseLL(row[iOrder], order) || !parseLL(row[iSize], r.size)) {
            error = lineErr(filename, lineNum, "du lieu sai dinh dang so");
            return false;
        }
        r.order = (int)order;
        out.push_back(r);
    }
    return true;
}

bool writeTrace(const std::string& filename, const std::vector<TraceRecord>& records, std::string& error) {
    std::ofstream file(filename);
    if (!file.is_open()) { error = "Khong the tao hoac ghi file: " + filename; return false; }

    file << "step,algorithm,process_id,request_size,block_id,alloc_start,"
            "block_size_before,block_size_after,status\n";
    for (const auto& r : records) {
        file << r.step << ',' << toString(r.algorithm) << ',' << r.processId << ',' << r.requestSize << ',';
        if (r.allocated)
            file << r.blockId << ',' << r.allocStart << ',' << r.sizeBefore << ',' << r.sizeAfter << ",ALLOCATED\n";
        else
            file << ",,,,FAILED\n";
    }
    return file.good();
}

bool writeSummary(const std::string& filename, const std::vector<Metrics>& records, std::string& error) {
    std::ofstream file(filename);
    if (!file.is_open()) { error = "Khong the tao hoac ghi file: " + filename; return false; }

    file << "algorithm,allocated,failed,total_free,largest_free,time_us\n";
    file << std::fixed << std::setprecision(2);
    for (const auto& m : records) {
        file << toString(m.algorithm) << ',' << m.allocated << ',' << m.failed << ',' << m.totalFree << ','
             << m.largestFree << ',' << m.timeUs << '\n';
    }
    return file.good();
}

}  // namespace bo_nho
