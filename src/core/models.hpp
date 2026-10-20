#pragma once
#include <array>
#include <string>
#include <vector>

namespace bo_nho {

enum class Algorithm { FirstFit, BestFit, WorstFit };

inline constexpr std::array<Algorithm, 3> kAllAlgorithms = {
    Algorithm::FirstFit, Algorithm::BestFit, Algorithm::WorstFit};

// Tên dùng trong CSV (FIRST_FIT, ...)
inline const char* toString(Algorithm a) {
    switch (a) {
        case Algorithm::FirstFit: return "FIRST_FIT";
        case Algorithm::BestFit:  return "BEST_FIT";
        case Algorithm::WorstFit: return "WORST_FIT";
    }
    return "?";
}

// Tên hiển thị trên giao diện
inline const char* displayName(Algorithm a) {
    switch (a) {
        case Algorithm::FirstFit: return "First-Fit";
        case Algorithm::BestFit:  return "Best-Fit";
        case Algorithm::WorstFit: return "Worst-Fit";
    }
    return "?";
}

// Một khối nhớ trống (hole)
struct Hole {
    std::string id;
    long long start = 0;  // địa chỉ bắt đầu (KB)
    long long size = 0;   // kích thước (KB)
    int line = 0;         // số dòng trong file CSV (0 nếu không đọc từ file)
};

// Một yêu cầu cấp phát
struct Request {
    int order = 0;            // thứ tự xử lý
    std::string processId;
    long long size = 0;
    int line = 0;
};

// Một dòng trace (kết quả của 1 yêu cầu)
struct TraceRecord {
    int step = 0;
    Algorithm algorithm = Algorithm::FirstFit;
    std::string processId;
    long long requestSize = 0;
    std::string blockId;        // rỗng nếu thất bại
    long long allocStart = 0;   // địa chỉ được cấp phát
    long long sizeBefore = 0;   // kích thước hole trước khi cấp phát
    long long sizeAfter = 0;    // phần dư sau khi cấp phát
    bool allocated = false;
};

// Một vùng đã được cấp phát (dùng để vẽ bộ nhớ)
struct Allocation {
    std::string processId;
    std::string blockId;
    long long start = 0;
    long long size = 0;
};

struct Metrics {
    Algorithm algorithm = Algorithm::FirstFit;
    int allocated = 0;
    int failed = 0;
    long long totalFree = 0;    // tổng dung lượng hole còn lại
    long long largestFree = 0;  // hole lớn nhất còn lại
    double timeUs = 0.0;        // thời gian chạy (micro giây)
};

struct SimulationResult {
    Algorithm algorithm = Algorithm::FirstFit;
    std::vector<TraceRecord> trace;
    std::vector<Allocation> allocations;
    std::vector<Hole> finalHoles;
    Metrics metrics;
};

}  // namespace bo_nho
