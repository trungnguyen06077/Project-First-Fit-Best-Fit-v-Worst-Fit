#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "core/allocator.hpp"
#include "core/csv_io.hpp"
#include "core/generator.hpp"
#include "core/models.hpp"
#include "core/validator.hpp"

using namespace bo_nho;

static int generate(int argc, char** argv) {
    std::string dir = argc > 2 ? argv[2] : ".";
    int nHole = argc > 3 ? std::atoi(argv[3]) : 10;
    int nReq = argc > 4 ? std::atoi(argv[4]) : 10;
    unsigned seed = argc > 5 ? (unsigned)std::atoi(argv[5]) : 42u;
    auto holes = generateHoles(nHole, 50, 300, seed);
    auto reqs = generateRequests(nReq, 20, 200, seed + 1);
    if (!writeHolesToCSV(dir + "/holes.csv", holes) || !writeRequestsToCSV(dir + "/requests.csv", reqs)) {
        std::cerr << "Khong ghi duoc file vao thu muc: " << dir << "\n";
        return 1;
    }
    std::cout << "Da tao " << dir << "/holes.csv va " << dir << "/requests.csv\n";
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "--gen") return generate(argc, argv);
    if (argc < 3) {
        std::cerr << "Cach dung:\n  " << argv[0] << " <holes.csv> <requests.csv> [thu_muc_ra]\n  " << argv[0]
                  << " --gen [thu_muc_ra] [so_hole] [so_req] [seed]\n";
        return 1;
    }
    std::string holesFile = argv[1], reqFile = argv[2], outDir = argc > 3 ? argv[3] : ".";

    std::vector<Hole> holes;
    std::vector<Request> reqs;
    std::string err;
    if (!readBlocks(holesFile, holes, err) || !readRequests(reqFile, reqs, err)) {
        std::cerr << err << "\n";
        return 1;
    }

    auto e1 = kiemTraDanhSachHole(holes);
    auto e2 = kiemTraDanhSachYeuCau(reqs);
    if (!e1.empty() || !e2.empty()) {
        std::cerr << taoThongBaoLoi(holesFile, e1) << taoThongBaoLoi(reqFile, e2);
        return 1;
    }

    std::vector<TraceRecord> allTrace;
    std::vector<Metrics> summary;
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(holes, reqs, a);
        allTrace.insert(allTrace.end(), r.trace.begin(), r.trace.end());
        summary.push_back(r.metrics);
        std::cout << displayName(a) << ": allocated=" << r.metrics.allocated << " failed=" << r.metrics.failed
                  << " totalFree=" << r.metrics.totalFree << " largestFree=" << r.metrics.largestFree
                  << " time=" << r.metrics.timeUs << "us\n";
    }

    if (!writeTrace(outDir + "/trace.csv", allTrace, err) || !writeSummary(outDir + "/summary.csv", summary, err)) {
        std::cerr << err << "\n";
        return 1;
    }
    std::cout << "Da ghi " << outDir << "/trace.csv va " << outDir << "/summary.csv\n";
    return 0;
}
