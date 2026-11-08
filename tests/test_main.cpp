#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/allocator.hpp"
#include "core/csv_io.hpp"
#include "core/generator.hpp"
#include "core/validator.hpp"

using namespace bo_nho;

// =====================================================================================
//  PHẦN 1: BỘ KHUNG TEST
//    CHECK(điều_kiện)   : điều_kiện phải đúng, sai thì báo [FAIL] kèm số dòng.
//    CHECK_EQ(a, b)     : a phải bằng b, sai thì in cả giá trị thực tế lẫn giá trị mong đợi.
//    CHECK_CONTAINS(s,k): chuỗi s phải chứa chuỗi con k (dùng để kiểm tra thông báo lỗi).
// =====================================================================================
static int g_checks = 0;  // tổng số phép kiểm tra đã chạy
static int g_failed = 0;  // số phép kiểm tra bị sai

#define CHECK(cond)                                                                   \
    do {                                                                              \
        ++g_checks;                                                                   \
        if (!(cond)) {                                                                \
            ++g_failed;                                                               \
            std::cerr << "  [FAIL] dong " << __LINE__ << ": " #cond "\n";             \
        }                                                                             \
    } while (0)

#define CHECK_EQ(a, b)                                                                \
    do {                                                                              \
        ++g_checks;                                                                   \
        auto va = (a);                                                                \
        auto vb = (b);                                                                \
        if (!(va == vb)) {                                                            \
            ++g_failed;                                                               \
            std::cerr << "  [FAIL] dong " << __LINE__ << ": " #a " == " #b            \
                      << "\n    thuc te : " << va << "\n    mong doi: " << vb << "\n"; \
        }                                                                             \
    } while (0)

#define CHECK_CONTAINS(hay, kim)                                                      \
    do {                                                                              \
        ++g_checks;                                                                   \
        std::string h_ = (hay);                                                       \
        if (h_.find(kim) == std::string::npos) {                                      \
            ++g_failed;                                                               \
            std::cerr << "  [FAIL] dong " << __LINE__ << ": khong thay \"" << kim     \
                      << "\" trong: \"" << h_ << "\"\n";                             \
        }                                                                             \
    } while (0)

// --- Hàm hỗ trợ đọc/ghi file khi test ---
static std::string readAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void writeText(const std::string& path, const std::string& text) {
    std::ofstream f(path, std::ios::binary);
    f << text;
}

// Bỏ ký tự '\r' để so sánh file không phụ thuộc Windows (CRLF) hay Linux (LF).
static std::string normalizeEol(std::string s) {
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
    return s;
}

// Thư mục tạm cho file test ghi ra. Dùng đường dẫn TƯƠNG ĐỐI; tự xóa khi hết phạm vi.
struct TempDir {
    std::string path;
    explicit TempDir(const std::string& name) : path(name) {
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
    std::string file(const std::string& name) const { return path + "/" + name; }
};

// =====================================================================================
//  PHẦN 2: UNIT TEST THUẬT TOÁN (allocator)
// =====================================================================================
// ---------------------------------------------------------------- dữ liệu mẫu
// 5 hole (kích thước 100, 500, 200, 300, 600) đặt cách nhau, không chồng lấn.
static std::vector<Hole> nam_hole() {
    return {{"H1", 0, 100, 0},   {"H2", 200, 500, 0},  {"H3", 800, 200, 0},
            {"H4", 1100, 300, 0}, {"H5", 1500, 600, 0}};
}

static std::vector<Request> bon_yeu_cau() {
    return {{1, "P1", 212, 0}, {2, "P2", 417, 0}, {3, "P3", 112, 0}, {4, "P4", 426, 0}};
}

static std::vector<Hole> tu_kich_thuoc(const std::vector<long long>& sizes) {
    std::vector<Hole> v;
    long long start = 0;
    for (size_t i = 0; i < sizes.size(); i++) {
        v.push_back({"H" + std::to_string(i + 1), start, sizes[i], 0});
        start += sizes[i] + 10;
    }
    return v;
}

// Kết quả mong đợi của một dòng trace. block == "" nghĩa là FAILED.
struct Expect {
    const char* block;
    long long start, before, after;
};

static void kiemTraTrace(const SimulationResult& r, const std::vector<Expect>& exp) {
    CHECK_EQ(r.trace.size(), exp.size());
    for (size_t i = 0; i < exp.size() && i < r.trace.size(); i++) {
        const TraceRecord& t = r.trace[i];
        const Expect& e = exp[i];
        bool ok = e.block[0] != '\0';
        CHECK_EQ(t.step, (int)i + 1);
        CHECK_EQ(t.allocated, ok);
        CHECK_EQ(t.blockId, std::string(e.block));
        if (ok) {
            CHECK_EQ(t.allocStart, e.start);
            CHECK_EQ(t.sizeBefore, e.before);
            CHECK_EQ(t.sizeAfter, e.after);
        }
    }
}

static bool giongTrace(const SimulationResult& a, const SimulationResult& b) {
    if (a.trace.size() != b.trace.size()) return false;
    for (size_t i = 0; i < a.trace.size(); i++) {
        const TraceRecord &x = a.trace[i], &y = b.trace[i];
        if (x.step != y.step || x.processId != y.processId || x.blockId != y.blockId ||
            x.allocStart != y.allocStart || x.sizeBefore != y.sizeBefore ||
            x.sizeAfter != y.sizeAfter || x.allocated != y.allocated)
            return false;
    }
    return true;
}

// ---------------------------------------------------------------- test: hàm chọn hole
static void test_chonHole() {
    std::vector<Hole> h = nam_hole();  // size: 100 500 200 300 600

    // First-Fit: hole đầu tiên đủ lớn
    CHECK_EQ(FirstFit(h, 212), 1);
    CHECK_EQ(FirstFit(h, 100), 0);  // vừa khít cũng được chọn
    CHECK_EQ(FirstFit(h, 600), 4);
    CHECK_EQ(FirstFit(h, 601), -1);

    // Best-Fit: hole nhỏ nhất vẫn đủ lớn
    CHECK_EQ(BestFit(h, 212), 3);  // 300 là nhỏ nhất trong {500,300,600}
    CHECK_EQ(BestFit(h, 100), 0);
    CHECK_EQ(BestFit(h, 601), -1);

    // Worst-Fit: hole lớn nhất
    CHECK_EQ(WorstFit(h, 212), 4);
    CHECK_EQ(WorstFit(h, 100), 4);
    CHECK_EQ(WorstFit(h, 600), 4);  // vừa khít nhưng là hole duy nhất đủ
    CHECK_EQ(WorstFit(h, 601), -1);

    // Danh sách rỗng
    std::vector<Hole> rong;
    CHECK_EQ(FirstFit(rong, 1), -1);
    CHECK_EQ(BestFit(rong, 1), -1);
    CHECK_EQ(WorstFit(rong, 1), -1);
}

static void test_hoaDongGiaTri() {
    // Nhiều hole cùng kích thước -> chọn hole có địa chỉ nhỏ nhất (index nhỏ nhất)
    std::vector<Hole> h = tu_kich_thuoc({300, 200, 300, 200});
    CHECK_EQ(BestFit(h, 150), 1);   // hai hole 200, lấy cái đứng trước
    CHECK_EQ(WorstFit(h, 150), 0);  // hai hole 300, lấy cái đứng trước
    CHECK_EQ(FirstFit(h, 250), 0);

    // Best-Fit ưu tiên hole vừa khít (dư = 0)
    std::vector<Hole> g = tu_kich_thuoc({300, 212, 250});
    CHECK_EQ(BestFit(g, 212), 1);
}


static void test_viDuGiaoTrinh() {
    {  // First-Fit
        SimulationResult r = simulate(nam_hole(), bon_yeu_cau(), Algorithm::FirstFit);
        kiemTraTrace(r, {{"H2", 200, 500, 288},
                         {"H5", 1500, 600, 183},
                         {"H2", 412, 288, 176},
                         {"", 0, 0, 0}});
        CHECK_EQ(r.metrics.allocated, 3);
        CHECK_EQ(r.metrics.failed, 1);
        CHECK_EQ(r.metrics.totalFree, 959LL);
        CHECK_EQ(r.metrics.largestFree, 300LL);
    }
    {  // Best-Fit
        SimulationResult r = simulate(nam_hole(), bon_yeu_cau(), Algorithm::BestFit);
        kiemTraTrace(r, {{"H4", 1100, 300, 88},
                         {"H2", 200, 500, 83},
                         {"H3", 800, 200, 88},
                         {"H5", 1500, 600, 174}});
        CHECK_EQ(r.metrics.allocated, 4);
        CHECK_EQ(r.metrics.failed, 0);
        CHECK_EQ(r.metrics.totalFree, 533LL);
        CHECK_EQ(r.metrics.largestFree, 174LL);
    }
    {  // Worst-Fit
        SimulationResult r = simulate(nam_hole(), bon_yeu_cau(), Algorithm::WorstFit);
        kiemTraTrace(r, {{"H5", 1500, 600, 388},
                         {"H2", 200, 500, 83},
                         {"H5", 1712, 388, 276},
                         {"", 0, 0, 0}});
        CHECK_EQ(r.metrics.allocated, 3);
        CHECK_EQ(r.metrics.failed, 1);
        CHECK_EQ(r.metrics.totalFree, 959LL);
        CHECK_EQ(r.metrics.largestFree, 300LL);
    }
}

// ---------------------------------------------------------------- test: hành vi của simulate
static void test_vuaKhit() {
    std::vector<Hole> h = {{"H1", 50, 200, 0}};
    std::vector<Request> q = {{1, "P1", 200, 0}};
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(h, q, a);
        CHECK_EQ(r.trace.size(), (size_t)1);
        CHECK(r.trace[0].allocated);
        CHECK_EQ(r.trace[0].sizeAfter, 0LL);
        CHECK(r.finalHoles.empty());  // hole dùng hết bị xóa
        CHECK_EQ(r.metrics.totalFree, 0LL);
        CHECK_EQ(r.metrics.largestFree, 0LL);
    }
}

static void test_holeCoLai() {
    // Cấp từ ĐẦU hole: start dịch lên, size giảm
    std::vector<Hole> h = {{"H1", 1000, 500, 0}};
    std::vector<Request> q = {{1, "P1", 100, 0}, {2, "P2", 100, 0}};
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(h, q, a);
        CHECK_EQ(r.trace[0].allocStart, 1000LL);
        CHECK_EQ(r.trace[1].allocStart, 1100LL);
        CHECK_EQ(r.finalHoles.size(), (size_t)1);
        CHECK_EQ(r.finalHoles[0].start, 1200LL);
        CHECK_EQ(r.finalHoles[0].size, 300LL);
        CHECK_EQ(r.allocations.size(), (size_t)2);
    }
}

static void test_thatBaiVaTiepTuc() {
    std::vector<Hole> h = {{"H1", 0, 100, 0}};
    std::vector<Request> q = {{1, "P1", 150, 0}, {2, "P2", 50, 0}, {3, "P3", 60, 0}};
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(h, q, a);
        CHECK_EQ(r.trace.size(), (size_t)3);  // thất bại vẫn được ghi trace
        CHECK(!r.trace[0].allocated);
        CHECK(r.trace[0].blockId.empty());
        CHECK(r.trace[1].allocated);  // yêu cầu sau vẫn được xử lý bình thường
        CHECK(!r.trace[2].allocated);  // hole chỉ còn 50 < 60
        CHECK_EQ(r.trace[2].step, 3);
        CHECK_EQ(r.metrics.allocated, 1);
        CHECK_EQ(r.metrics.failed, 2);
        CHECK_EQ(r.allocations.size(), (size_t)1);  // allocations chỉ chứa lần thành công
    }
}

static void test_phanManhNgoai() {
    // Tổng trống 600 >= 400 nhưng không hole nào liên tục >= 400 -> thất bại
    std::vector<Hole> h = tu_kich_thuoc({100, 200, 300});
    std::vector<Request> q = {{1, "P1", 400, 0}};
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(h, q, a);
        CHECK(!r.trace[0].allocated);
        CHECK_EQ(r.metrics.totalFree, 600LL);
        CHECK_EQ(r.metrics.largestFree, 300LL);
    }
}

static void test_banSaoDocLap() {
    std::vector<Hole> h = nam_hole();
    std::vector<Request> q = bon_yeu_cau();
    std::vector<Hole> goc = h;

    SimulationResult lan1 = simulate(h, q, Algorithm::FirstFit);
    simulate(h, q, Algorithm::BestFit);
    simulate(h, q, Algorithm::WorstFit);

    // Danh sách hole đầu vào không bị thay đổi sau khi mô phỏng
    CHECK_EQ(h.size(), goc.size());
    for (size_t i = 0; i < h.size() && i < goc.size(); i++) {
        CHECK_EQ(h[i].id, goc[i].id);
        CHECK_EQ(h[i].start, goc[i].start);
        CHECK_EQ(h[i].size, goc[i].size);
    }
    // Chạy lại First-Fit cho đúng kết quả cũ (deterministic)
    CHECK(giongTrace(lan1, simulate(h, q, Algorithm::FirstFit)));
}

static void test_dauVaoKhongSapXep() {
    std::vector<Hole> xuoi = nam_hole();
    std::vector<Hole> nguoc = xuoi;
    std::reverse(nguoc.begin(), nguoc.end());
    std::vector<Request> qx = bon_yeu_cau();
    std::vector<Request> qn = qx;
    std::reverse(qn.begin(), qn.end());

    for (Algorithm a : kAllAlgorithms)
        CHECK(giongTrace(simulate(xuoi, qx, a), simulate(nguoc, qn, a)));
}

static void test_hoaDongKichThuocTrongSimulate() {
    // Hai hole cùng size 300 tại địa chỉ 0 và 500; đưa vào theo thứ tự ngược
    std::vector<Hole> h = {{"H2", 500, 300, 0}, {"H1", 0, 300, 0}};
    std::vector<Request> q = {{1, "P1", 100, 0}};
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(h, q, a);
        CHECK_EQ(r.trace[0].blockId, std::string("H1"));  // địa chỉ nhỏ hơn thắng
        CHECK_EQ(r.trace[0].allocStart, 0LL);
    }
}

static void test_metrics() {
    SimulationResult r = simulate(nam_hole(), bon_yeu_cau(), Algorithm::BestFit);
    CHECK(r.metrics.timeUs >= 0.0);
    CHECK(r.metrics.algorithm == Algorithm::BestFit);
    CHECK(r.algorithm == Algorithm::BestFit);
    for (const TraceRecord& t : r.trace) CHECK(t.algorithm == Algorithm::BestFit);

    // Không có yêu cầu nào: mọi hole giữ nguyên
    SimulationResult e = simulate(nam_hole(), {}, Algorithm::FirstFit);
    CHECK(e.trace.empty());
    CHECK_EQ(e.metrics.totalFree, 1700LL);
    CHECK_EQ(e.metrics.largestFree, 600LL);
    CHECK_EQ(e.finalHoles.size(), (size_t)5);

    // Không có hole nào: tất cả thất bại
    SimulationResult n = simulate({}, bon_yeu_cau(), Algorithm::WorstFit);
    CHECK_EQ(n.metrics.failed, 4);
    CHECK_EQ(n.metrics.totalFree, 0LL);
}

// ---------------------------------------------------------------- stress test
struct Doan {
    long long a, b;  // [a, b)
};

static bool khongChongLan(std::vector<Doan> v) {
    std::sort(v.begin(), v.end(), [](const Doan& x, const Doan& y) { return x.a < y.a; });
    for (size_t k = 1; k < v.size(); k++)
        if (v[k - 1].b > v[k].a) return false;
    return true;
}

static const Hole* timHole(const std::vector<Hole>& hs, const std::string& id) {
    for (const Hole& h : hs)
        if (h.id == id) return &h;
    return nullptr;
}

static void test_stressBatBienBoNho() {
    for (unsigned seed = 1; seed <= 300; seed++) {
        std::mt19937 rng(seed);

        std::vector<Hole> holes;
        int nh = 1 + (int)(rng() % 20);
        long long cursor = rng() % 50, tongBanDau = 0;
        for (int i = 0; i < nh; i++) {
            long long size = 10 + (long long)(rng() % 300);
            holes.push_back({"H" + std::to_string(i + 1), cursor, size, 0});
            tongBanDau += size;
            cursor += size + (long long)(rng() % 40);  // khoảng hở 0 -> hai hole liền kề
        }
        std::vector<Request> reqs;
        int nr = 1 + (int)(rng() % 30);
        for (int i = 0; i < nr; i++)
            reqs.push_back({i + 1, "P" + std::to_string(i + 1), 5 + (long long)(rng() % 250), 0});

        // Xáo trộn đầu vào để kiểm tra luôn bước sắp xếp trong simulate
        std::vector<Hole> holesTron = holes;
        std::vector<Request> reqsTron = reqs;
        std::shuffle(holesTron.begin(), holesTron.end(), rng);
        std::shuffle(reqsTron.begin(), reqsTron.end(), rng);

        for (Algorithm algo : kAllAlgorithms) {
            SimulationResult r = simulate(holesTron, reqsTron, algo);

            // 1. Mỗi yêu cầu đúng một dòng trace, step liên tục
            CHECK_EQ(r.trace.size(), reqs.size());
            for (size_t i = 0; i < r.trace.size(); i++) {
                CHECK_EQ(r.trace[i].step, (int)i + 1);
                CHECK_EQ(r.trace[i].processId, reqs[i].processId);  // đã sort theo order
            }
            CHECK_EQ(r.metrics.allocated + r.metrics.failed, nr);

            // 2. Trace thành công: sizeBefore - requestSize == sizeAfter >= 0
            size_t k = 0;
            int soThanhCong = 0;
            for (const TraceRecord& t : r.trace) {
                if (!t.allocated) continue;
                soThanhCong++;
                CHECK_EQ(t.sizeBefore - t.requestSize, t.sizeAfter);
                CHECK(t.sizeAfter >= 0);
                CHECK(k < r.allocations.size());
                if (k < r.allocations.size()) {
                    CHECK_EQ(r.allocations[k].start, t.allocStart);
                    CHECK_EQ(r.allocations[k].size, t.requestSize);
                    CHECK_EQ(r.allocations[k].processId, t.processId);
                }
                k++;
            }
            CHECK_EQ(soThanhCong, r.metrics.allocated);
            CHECK_EQ(r.allocations.size(), (size_t)r.metrics.allocated);

            // 3. Bảo toàn bộ nhớ: tổng ban đầu = còn trống + đã cấp
            long long daCap = 0;
            for (const Allocation& a : r.allocations) daCap += a.size;
            CHECK_EQ(tongBanDau, r.metrics.totalFree + daCap);

            // 4. Mỗi vùng cấp / hole còn lại nằm gọn trong hole gốc cùng id
            std::vector<Doan> tatCa;
            for (const Allocation& a : r.allocations) {
                const Hole* g = timHole(holes, a.blockId);
                CHECK(g != nullptr);
                if (g) CHECK(a.start >= g->start && a.start + a.size <= g->start + g->size);
                tatCa.push_back({a.start, a.start + a.size});
            }
            long long lonNhat = 0;
            for (const Hole& h : r.finalHoles) {
                const Hole* g = timHole(holes, h.id);
                CHECK(g != nullptr);
                if (g) CHECK(h.start >= g->start && h.start + h.size <= g->start + g->size);
                CHECK(h.size > 0);  // hole size 0 phải bị xóa
                lonNhat = std::max(lonNhat, h.size);
                tatCa.push_back({h.start, h.start + h.size});
            }
            CHECK_EQ(r.metrics.largestFree, lonNhat);

            // 5. Vùng đã cấp và hole còn lại không chồng lấn nhau
            CHECK(khongChongLan(tatCa));
        }
    }
}

// =====================================================================================
//  PHẦN 3: INTEGRATION TEST (nạp CSV -> kiểm tra -> chạy 3 thuật toán -> xuất CSV -> so expected)
//  Đường dẫn thư mục data lấy từ đối số dòng lệnh (mặc định "data").
// =====================================================================================
static std::string g_dataDir = "data";

static std::string fileVao(const char* ten) { return g_dataDir + "/input/" + ten; }
static std::string fileExpected(const char* ten) { return g_dataDir + "/output/expected/" + ten; }

// ------------------------------------------------------------------ hàm hỗ trợ
struct DuLieu {
    std::vector<Hole> holes;
    std::vector<Request> reqs;
};

static DuLieu docDuLieuMau() {
    DuLieu d;
    std::string err;
    bool a = readBlocks(fileVao("blocks_sample.csv"), d.holes, err);
    if (!a) std::cerr << "  " << err << "\n";
    bool b = readRequests(fileVao("requests_sample.csv"), d.reqs, err);
    if (!b) std::cerr << "  " << err << "\n";
    CHECK(a);
    CHECK(b);
    return d;
}

// Chạy cả ba thuật toán giống main.cpp: gộp trace, mỗi thuật toán một dòng summary.
struct KetQua {
    std::vector<TraceRecord> trace;
    std::vector<Metrics> summary;
};

static KetQua chayBaThuatToan(const DuLieu& d) {
    KetQua kq;
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(d.holes, d.reqs, a);
        kq.trace.insert(kq.trace.end(), r.trace.begin(), r.trace.end());
        kq.summary.push_back(r.metrics);
    }
    return kq;
}

// Bỏ cột cuối (time_us) của summary.csv: thời gian chạy mỗi lần mỗi khác nên không so sánh được.
static std::string boCotThoiGian(const std::string& csv) {
    std::string out, line;
    std::string s = normalizeEol(csv);
    for (size_t i = 0; i <= s.size(); i++) {
        if (i == s.size() || s[i] == '\n') {
            if (!line.empty()) out += line.substr(0, line.rfind(',')) + "\n";
            line.clear();
        } else {
            line += s[i];
        }
    }
    return out;
}

// Có lỗi nào ở đúng dòng `dong` mà nội dung chứa `chua` không?
static bool coLoi(const std::vector<LoiDuLieu>& ds, int dong, const std::string& chua) {
    for (const auto& l : ds)
        if (l.dong == dong && l.noiDung.find(chua) != std::string::npos) return true;
    return false;
}

static void kiemTraBuoc(const SimulationResult& r, size_t i, const char* block, long long start, long long truoc,
                        long long sau) {
    CHECK(i < r.trace.size());
    if (i >= r.trace.size()) return;
    const TraceRecord& t = r.trace[i];
    bool ok = block[0] != '\0';
    CHECK_EQ(t.allocated, ok);
    CHECK_EQ(t.blockId, std::string(block));
    if (ok) {
        CHECK_EQ(t.allocStart, start);
        CHECK_EQ(t.sizeBefore, truoc);
        CHECK_EQ(t.sizeAfter, sau);
    }
}

// ------------------------------------------------------------------ TC01 - TC03: dữ liệu mẫu
static void tc01_03_duLieuMau() {
    DuLieu d = docDuLieuMau();
    CHECK_EQ(d.holes.size(), (size_t)5);
    CHECK_EQ(d.reqs.size(), (size_t)4);
    CHECK(kiemTraDanhSachHole(d.holes).empty());
    CHECK(kiemTraDanhSachYeuCau(d.reqs).empty());

    // TC01 - First-Fit: P4 FAILED, 3 cấp phát (bảng mục 9 README)
    SimulationResult ff = simulate(d.holes, d.reqs, Algorithm::FirstFit);
    CHECK_EQ(ff.metrics.allocated, 3);
    CHECK_EQ(ff.metrics.failed, 1);
    kiemTraBuoc(ff, 0, "B2", 150, 500, 288);
    kiemTraBuoc(ff, 1, "B5", 1300, 600, 183);
    kiemTraBuoc(ff, 2, "B2", 362, 288, 176);
    kiemTraBuoc(ff, 3, "", 0, 0, 0);

    // TC02 - Best-Fit: cả 4 ALLOCATED
    SimulationResult bf = simulate(d.holes, d.reqs, Algorithm::BestFit);
    CHECK_EQ(bf.metrics.allocated, 4);
    CHECK_EQ(bf.metrics.failed, 0);
    kiemTraBuoc(bf, 0, "B4", 950, 300, 88);
    kiemTraBuoc(bf, 1, "B2", 150, 500, 83);
    kiemTraBuoc(bf, 2, "B3", 700, 200, 88);
    kiemTraBuoc(bf, 3, "B5", 1300, 600, 174);

    // TC03 - Worst-Fit: P4 FAILED, 3 cấp phát
    SimulationResult wf = simulate(d.holes, d.reqs, Algorithm::WorstFit);
    CHECK_EQ(wf.metrics.allocated, 3);
    CHECK_EQ(wf.metrics.failed, 1);
    kiemTraBuoc(wf, 0, "B5", 1300, 600, 388);
    kiemTraBuoc(wf, 1, "B2", 150, 500, 83);
    kiemTraBuoc(wf, 2, "B5", 1512, 388, 276);
    kiemTraBuoc(wf, 3, "", 0, 0, 0);
}

// Toàn bộ đường ống: ghi trace.csv / summary.csv rồi so với file expected tính tay.
static void doiChieuExpected() {
    DuLieu d = docDuLieuMau();
    KetQua kq = chayBaThuatToan(d);

    TempDir tmp("test_tmp_expected");
    std::string err;
    CHECK(writeTrace(tmp.file("trace.csv"), kq.trace, err));
    CHECK(writeSummary(tmp.file("summary.csv"), kq.summary, err));

    std::string moTrace = normalizeEol(readAll(fileExpected("trace_expected.csv")));
    std::string moSummary = normalizeEol(readAll(fileExpected("summary_expected.csv")));
    CHECK(!moTrace.empty());  // file expected thiếu/rỗng thì test không được "đạt" một cách vô nghĩa
    CHECK(!moSummary.empty());

    CHECK_EQ(normalizeEol(readAll(tmp.file("trace.csv"))), moTrace);
    CHECK_EQ(boCotThoiGian(readAll(tmp.file("summary.csv"))), moSummary);
}

// ------------------------------------------------------------------ TC12: tái lập kết quả
static void tc12_taiLap() {
    DuLieu d = docDuLieuMau();
    TempDir tmp("test_tmp_taiLap");
    std::string err;
    KetQua a = chayBaThuatToan(d);
    KetQua b = chayBaThuatToan(d);
    CHECK(writeTrace(tmp.file("trace1.csv"), a.trace, err));
    CHECK(writeTrace(tmp.file("trace2.csv"), b.trace, err));
    CHECK(writeSummary(tmp.file("sum1.csv"), a.summary, err));
    CHECK(writeSummary(tmp.file("sum2.csv"), b.summary, err));
    CHECK_EQ(readAll(tmp.file("trace1.csv")), readAll(tmp.file("trace2.csv")));
    CHECK_EQ(boCotThoiGian(readAll(tmp.file("sum1.csv"))), boCotThoiGian(readAll(tmp.file("sum2.csv"))));
}

// ------------------------------------------------------------------ TC07: danh sách rỗng
static void tc07_yeuCauRong() {
    DuLieu d = docDuLieuMau();
    TempDir tmp("test_tmp_rong");
    writeText(tmp.file("req_rong.csv"), "order,process_id,size\n");
    std::vector<Request> reqs;
    std::string err;
    CHECK(readRequests(tmp.file("req_rong.csv"), reqs, err));
    CHECK(reqs.empty());
    CHECK(kiemTraDanhSachYeuCau(reqs).empty());

    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(d.holes, reqs, a);
        CHECK(r.trace.empty());
        CHECK_EQ(r.metrics.allocated, 0);
        CHECK_EQ(r.metrics.failed, 0);
        CHECK_EQ(r.metrics.totalFree, 1700LL);  // 100+500+200+300+600, không hole nào bị đụng tới
        CHECK_EQ(r.finalHoles.size(), d.holes.size());
    }
    // Ghi trace rỗng: file chỉ có đúng dòng tiêu đề
    CHECK(writeTrace(tmp.file("trace.csv"), {}, err));
    CHECK_EQ(normalizeEol(readAll(tmp.file("trace.csv"))),
             std::string("step,algorithm,process_id,request_size,block_id,alloc_start,"
                         "block_size_before,block_size_after,status\n"));
}

// ------------------------------------------------------------------ TC08: file lỗi, thiếu cột
static void tc08_thieuCotVaFileLoi() {
    TempDir tmp("test_tmp_thieuCot");
    std::vector<Hole> holes;
    std::vector<Request> reqs;
    std::string err;

    writeText(tmp.file("thieu_size.csv"), "block_id,start\nB1,0\n");
    CHECK(!readBlocks(tmp.file("thieu_size.csv"), holes, err));
    CHECK_CONTAINS(err, "thieu cot bat buoc");

    writeText(tmp.file("thieu_pid.csv"), "order,size\n1,100\n");
    CHECK(!readRequests(tmp.file("thieu_pid.csv"), reqs, err));
    CHECK_CONTAINS(err, "thieu cot bat buoc");

    // File không tồn tại
    CHECK(!readBlocks(tmp.file("khong_co.csv"), holes, err));
    CHECK_CONTAINS(err, "Khong the mo file");
    CHECK(!readRequests(tmp.file("khong_co.csv"), reqs, err));
    CHECK_CONTAINS(err, "Khong the mo file");

    // File rỗng hoàn toàn (0 byte)
    writeText(tmp.file("rong.csv"), "");
    CHECK(!readBlocks(tmp.file("rong.csv"), holes, err));
    CHECK_CONTAINS(err, "File rong");

    // Dòng thiếu trường: báo đúng số dòng
    writeText(tmp.file("thieu_truong.csv"), "block_id,start,size\nB1,0,100\nB2,200\n");
    CHECK(!readBlocks(tmp.file("thieu_truong.csv"), holes, err));
    CHECK_CONTAINS(err, "dong 3");
    CHECK_CONTAINS(err, "thieu truong du lieu");
}

// ------------------------------------------------------------------ TC09: size sai
static void tc09_sizeSai() {
    TempDir tmp("test_tmp_sizeSai");
    std::vector<Hole> holes;
    std::vector<Request> reqs;
    std::string err;

    // size không phải số -> lỗi ngay lúc đọc, báo đúng dòng
    writeText(tmp.file("abc.csv"), "block_id,start,size\nB1,0,100\nB2,200,abc\n");
    CHECK(!readBlocks(tmp.file("abc.csv"), holes, err));
    CHECK_CONTAINS(err, "dong 3");
    CHECK_CONTAINS(err, "sai dinh dang so");

    // số thực cũng không hợp lệ (size phải là số nguyên)
    writeText(tmp.file("thuc.csv"), "block_id,start,size\nB1,0,10.5\n");
    CHECK(!readBlocks(tmp.file("thuc.csv"), holes, err));
    CHECK_CONTAINS(err, "sai dinh dang so");

    writeText(tmp.file("req_abc.csv"), "order,process_id,size\n1,P1,50\n2,P2,xyz\n");
    CHECK(!readRequests(tmp.file("req_abc.csv"), reqs, err));
    CHECK_CONTAINS(err, "dong 3");
    CHECK_CONTAINS(err, "sai dinh dang so");

    // size âm / bằng 0 -> đọc được nhưng validator phải bắt, đúng dòng
    writeText(tmp.file("am.csv"), "block_id,start,size\nB1,0,100\nB2,200,-5\nB3,400,0\n");
    CHECK(readBlocks(tmp.file("am.csv"), holes, err));
    auto loi = kiemTraDanhSachHole(holes);
    CHECK(coLoi(loi, 3, "size phai > 0"));
    CHECK(coLoi(loi, 4, "size phai > 0"));
    CHECK(!coLoi(loi, 2, "size"));  // dòng hợp lệ thì không bị báo

    writeText(tmp.file("req_am.csv"), "order,process_id,size\n1,P1,-1\n0,P2,10\n");
    CHECK(readRequests(tmp.file("req_am.csv"), reqs, err));
    auto loiReq = kiemTraDanhSachYeuCau(reqs);
    CHECK(coLoi(loiReq, 2, "size phai > 0"));
    CHECK(coLoi(loiReq, 3, "order phai >= 1"));

    // thông báo gộp có đủ tên file, dòng, nguyên nhân (đúng yêu cầu mục 8.5 README)
    std::string tb = taoThongBaoLoi("am.csv", loi);
    CHECK_CONTAINS(tb, "am.csv, dong 3: size phai > 0");
}

// ------------------------------------------------------------------ TC10: trùng ID / order / chồng lấn
static void tc10_trungVaChongLan() {
    TempDir tmp("test_tmp_trung");
    std::vector<Hole> holes;
    std::vector<Request> reqs;
    std::string err;

    writeText(tmp.file("trung_id.csv"), "block_id,start,size\nB1,0,100\nB2,200,100\nB1,400,100\n");
    CHECK(readBlocks(tmp.file("trung_id.csv"), holes, err));
    CHECK(coLoi(kiemTraDanhSachHole(holes), 4, "block_id bi trung: B1"));

    writeText(tmp.file("trung_order.csv"), "order,process_id,size\n1,P1,10\n2,P2,10\n2,P3,10\n");
    CHECK(readRequests(tmp.file("trung_order.csv"), reqs, err));
    CHECK(coLoi(kiemTraDanhSachYeuCau(reqs), 4, "order bi trung: 2"));

    writeText(tmp.file("id_rong.csv"), "block_id,start,size\n,0,100\n");
    CHECK(readBlocks(tmp.file("id_rong.csv"), holes, err));
    CHECK(coLoi(kiemTraDanhSachHole(holes), 2, "block_id rong"));

    // Chồng lấn địa chỉ: [0,100) và [50,150)
    writeText(tmp.file("chong.csv"), "block_id,start,size\nB1,0,100\nB2,50,100\n");
    CHECK(readBlocks(tmp.file("chong.csv"), holes, err));
    CHECK(coLoi(kiemTraDanhSachHole(holes), 3, "chong lan"));

    // Hai hole LIỀN KỀ ([0,100) và [100,200)) là hợp lệ, không phải chồng lấn
    writeText(tmp.file("lien_ke.csv"), "block_id,start,size\nB1,0,100\nB2,100,100\n");
    CHECK(readBlocks(tmp.file("lien_ke.csv"), holes, err));
    CHECK(kiemTraDanhSachHole(holes).empty());

    // Chồng lấn vẫn bị phát hiện khi các dòng không xếp theo địa chỉ
    writeText(tmp.file("chong_dao.csv"), "block_id,start,size\nB1,500,100\nB2,0,100\nB3,550,10\n");
    CHECK(readBlocks(tmp.file("chong_dao.csv"), holes, err));
    CHECK(coLoi(kiemTraDanhSachHole(holes), 4, "chong lan"));
}

// ------------------------------------------------------------------ Định dạng file: BOM, CRLF, khoảng trắng
static void csv_bomCrlfKhoangTrang() {
    TempDir tmp("test_tmp_bom");
    std::vector<Hole> holes;
    std::string err;

    // File lưu từ Excel/Notepad trên Windows: có BOM UTF-8, xuống dòng CRLF, thừa khoảng trắng
    writeText(tmp.file("windows.csv"),
              "\xEF\xBB\xBF" "block_id,start,size\r\nB1,0,100\r\n B2 , 150 , 500 \r\n\r\n");
    CHECK(readBlocks(tmp.file("windows.csv"), holes, err));
    CHECK_EQ(holes.size(), (size_t)2);
    if (holes.size() == 2) {
        CHECK_EQ(holes[1].id, std::string("B2"));
        CHECK_EQ(holes[1].start, 150LL);
        CHECK_EQ(holes[1].size, 500LL);
        CHECK_EQ(holes[1].line, 3);
    }

    // Tên cột "id" được chấp nhận thay cho "block_id"; cột đảo thứ tự vẫn đọc đúng
    writeText(tmp.file("doi_cot.csv"), "size,id,start\n100,B9,40\n");
    CHECK(readBlocks(tmp.file("doi_cot.csv"), holes, err));
    CHECK_EQ(holes.size(), (size_t)1);
    if (holes.size() == 1) {
        CHECK_EQ(holes[0].id, std::string("B9"));
        CHECK_EQ(holes[0].start, 40LL);
        CHECK_EQ(holes[0].size, 100LL);
    }
}

// ------------------------------------------------------------------ Ghi file thất bại không được crash
static void csv_ghiFileThatBai() {
    std::string err;
    CHECK(!writeTrace("thu_muc_khong_ton_tai/trace.csv", {}, err));
    CHECK_CONTAINS(err, "Khong the tao");
    CHECK(!writeSummary("thu_muc_khong_ton_tai/summary.csv", {}, err));
    CHECK_CONTAINS(err, "Khong the tao");
}

// ------------------------------------------------------------------ Generator
static void generator_taiLapVaHopLe() {
    auto a = generateHoles(50, 50, 300, 42);
    auto b = generateHoles(50, 50, 300, 42);
    auto c = generateHoles(50, 50, 300, 43);
    CHECK_EQ(a.size(), (size_t)50);
    bool giong = a.size() == b.size(), khac = false;
    for (size_t i = 0; giong && i < a.size(); i++)
        if (a[i].id != b[i].id || a[i].start != b[i].start || a[i].size != b[i].size) giong = false;
    for (size_t i = 0; i < a.size() && i < c.size(); i++)
        if (a[i].size != c[i].size) khac = true;
    CHECK(giong);  // cùng seed -> cùng dữ liệu
    CHECK(khac);   // khác seed -> dữ liệu khác
    for (const Hole& h : a) CHECK(h.size >= 50 && h.size <= 300);
    CHECK(kiemTraDanhSachHole(a).empty());  // dữ liệu sinh ra phải qua validator

    auto r = generateRequests(50, 20, 200, 7);
    CHECK_EQ(r.size(), (size_t)50);
    for (size_t i = 0; i < r.size(); i++) {
        CHECK_EQ(r[i].order, (int)i + 1);
        CHECK(r[i].size >= 20 && r[i].size <= 200);
    }
    CHECK(kiemTraDanhSachYeuCau(r).empty());

    // Tham số không hợp lệ -> danh sách rỗng, không crash
    CHECK(generateHoles(0, 50, 300, 1).empty());
    CHECK(generateHoles(5, 0, 300, 1).empty());
    CHECK(generateHoles(5, 300, 50, 1).empty());
    CHECK(generateRequests(-1, 20, 200, 1).empty());
    CHECK(generateRequests(5, 200, 20, 1).empty());
}

static void generator_ghiRoiDocLai() {
    TempDir tmp("test_tmp_gen");
    auto holes = generateHoles(30, 50, 300, 5);
    auto reqs = generateRequests(30, 20, 200, 6);
    CHECK(writeHolesToCSV(tmp.file("h.csv"), holes));
    CHECK(writeRequestsToCSV(tmp.file("r.csv"), reqs));

    std::vector<Hole> h2;
    std::vector<Request> r2;
    std::string err;
    CHECK(readBlocks(tmp.file("h.csv"), h2, err));
    CHECK(readRequests(tmp.file("r.csv"), r2, err));
    CHECK_EQ(h2.size(), holes.size());
    CHECK_EQ(r2.size(), reqs.size());
    for (size_t i = 0; i < h2.size() && i < holes.size(); i++) {
        CHECK_EQ(h2[i].id, holes[i].id);
        CHECK_EQ(h2[i].start, holes[i].start);
        CHECK_EQ(h2[i].size, holes[i].size);
    }
    for (size_t i = 0; i < r2.size() && i < reqs.size(); i++) {
        CHECK_EQ(r2[i].order, reqs[i].order);
        CHECK_EQ(r2[i].processId, reqs[i].processId);
        CHECK_EQ(r2[i].size, reqs[i].size);
    }
    CHECK(!writeHolesToCSV("thu_muc_khong_ton_tai/h.csv", holes));
}

// =====================================================================================
//  PHẦN 4: STRESS TEST DỮ LIỆU LỚN (TC11)
//  Không có đáp án tính tay cho hàng chục nghìn yêu cầu, nên ta kiểm tra các BẤT BIẾN
//  (những điều luôn phải đúng): bảo toàn bộ nhớ, không chồng lấn, mỗi yêu cầu một dòng trace...
//  Chi phí O(n^2): n tăng 10 lần thì thời gian tăng ~100 lần, nên mặc định dùng n = 10.000.
// =====================================================================================
static int g_soLuongStress = 10000;

// Các bất biến phải luôn đúng, bất kể dữ liệu lớn hay nhỏ.
static void kiemTraBatBien(const std::vector<Hole>& holes, const std::vector<Request>& reqs,
                    const SimulationResult& r) {
    const int n = (int)reqs.size();

    // Mỗi yêu cầu có đúng một dòng trace; allocated + failed = tổng số yêu cầu
    CHECK_EQ(r.trace.size(), reqs.size());
    CHECK_EQ(r.metrics.allocated + r.metrics.failed, n);
    CHECK_EQ(r.allocations.size(), (size_t)r.metrics.allocated);

    // Bảo toàn bộ nhớ: tổng hole ban đầu = còn trống + đã cấp
    long long tongBanDau = 0, daCap = 0;
    for (const Hole& h : holes) tongBanDau += h.size;
    for (const Allocation& a : r.allocations) daCap += a.size;
    CHECK_EQ(tongBanDau, r.metrics.totalFree + daCap);

    // Vùng đã cấp và hole còn lại nằm trong hole gốc cùng mã, và không chồng lấn nhau
    std::unordered_map<std::string, const Hole*> goc;
    for (const Hole& h : holes) goc[h.id] = &h;

    std::vector<Doan> tatCa;
    tatCa.reserve(r.allocations.size() + r.finalHoles.size());
    long long lonNhat = 0;
    int viPham = 0;
    for (const Allocation& a : r.allocations) {
        auto it = goc.find(a.blockId);
        if (it == goc.end() || a.start < it->second->start ||
            a.start + a.size > it->second->start + it->second->size)
            ++viPham;
        tatCa.push_back({a.start, a.start + a.size});
    }
    for (const Hole& h : r.finalHoles) {
        auto it = goc.find(h.id);
        if (it == goc.end() || h.size <= 0 || h.start < it->second->start ||
            h.start + h.size > it->second->start + it->second->size)
            ++viPham;
        lonNhat = std::max(lonNhat, h.size);
        tatCa.push_back({h.start, h.start + h.size});
    }
    CHECK_EQ(viPham, 0);
    CHECK_EQ(r.metrics.largestFree, lonNhat);
    CHECK(khongChongLan(tatCa));
}

static void tc11_duLieuLon() {
    const int n = g_soLuongStress;
    std::cout << "    (TC11 dung " << n << " hole + " << n << " yeu cau)\n";
    std::vector<Hole> holes = generateHoles(n, 50, 300, 42);
    std::vector<Request> reqs = generateRequests(n, 20, 200, 43);
    CHECK_EQ(holes.size(), (size_t)n);
    CHECK_EQ(reqs.size(), (size_t)n);

    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(holes, reqs, a);
        std::cout << "    " << displayName(a) << ": " << r.metrics.timeUs / 1000.0
                  << " ms, allocated=" << r.metrics.allocated << " failed=" << r.metrics.failed << "\n";
        kiemTraBatBien(holes, reqs, r);
    }

    // Tái lập: chạy lại trên một phần dữ liệu, kết quả phải giống hệt
    int nho = std::min(n, 2000);
    std::vector<Hole> h2 = generateHoles(nho, 50, 300, 42);
    std::vector<Request> q2 = generateRequests(nho, 20, 200, 43);
    for (Algorithm a : kAllAlgorithms) CHECK(giongTrace(simulate(h2, q2, a), simulate(h2, q2, a)));
}

// =====================================================================================
//  PHẦN 5: MAIN - danh sách test được chạy, theo mã trong bảng kiểm thử (docs/test_table.md)
// =====================================================================================
int main(int argc, char** argv) {
    if (argc > 1) g_dataDir = argv[1];
    if (argc > 2 && std::atoi(argv[2]) > 0) g_soLuongStress = std::atoi(argv[2]);

    struct Test {
        const char* ten;
        void (*ham)();
    };
    const Test ds[] = {
        // --- Integration: dữ liệu mẫu ---
        {"TC01-03 du lieu mau: ca 3 thuat toan khop bang tinh tay (README muc 9)", tc01_03_duLieuMau},
        {"doi chieu trace.csv / summary.csv voi data/output/expected", doiChieuExpected},
        {"TC12 tai lap: chay 2 lan cho ket qua giong het nhau", tc12_taiLap},
        // --- Unit: thuật toán ---
        {"chon hole (First/Best/Worst)", test_chonHole},
        {"TC06 hoa dong gia tri -> dia chi nho nhat", test_hoaDongGiaTri},
        {"vi du giao trinh (tinh tay, dia chi khac)", test_viDuGiaoTrinh},
        {"TC04 cap vua khit -> xoa hole", test_vuaKhit},
        {"hole co lai, start dich len", test_holeCoLai},
        {"TC05 that bai van ghi trace, tiep tuc", test_thatBaiVaTiepTuc},
        {"phan manh ngoai", test_phanManhNgoai},
        {"ban sao doc lap", test_banSaoDocLap},
        {"dau vao khong sap xep", test_dauVaoKhongSapXep},
        {"hoa dong gia tri trong simulate", test_hoaDongKichThuocTrongSimulate},
        {"metrics + truong hop bien", test_metrics},
        {"ngau nhien 300 bo du lieu nho (bat bien bo nho)", test_stressBatBienBoNho},
        // --- Integration: CSV / validator / generator ---
        {"TC07 danh sach yeu cau rong", tc07_yeuCauRong},
        {"TC08 thieu cot / file khong ton tai / file rong / thieu truong", tc08_thieuCotVaFileLoi},
        {"TC09 size khong phai so / am / bang 0", tc09_sizeSai},
        {"TC10 block_id va order trung, hole chong lan", tc10_trungVaChongLan},
        {"CSV kieu Windows: BOM, CRLF, khoang trang, doi thu tu cot", csv_bomCrlfKhoangTrang},
        {"ghi file that bai khong crash", csv_ghiFileThatBai},
        {"generator: tai lap, hop le, tham so sai", generator_taiLapVaHopLe},
        {"generator: ghi CSV roi doc lai", generator_ghiRoiDocLai},
        // --- Stress ---
        {"TC11 du lieu lon", tc11_duLieuLon},
    };

    int testSai = 0;
    for (const Test& t : ds) {
        int truoc = g_failed;
        t.ham();
        bool ok = g_failed == truoc;
        if (!ok) ++testSai;
        std::cout << (ok ? "[ OK ] " : "[FAIL] ") << t.ten << "\n";
    }
    std::cout << "\n" << g_checks << " phep kiem tra, " << g_failed << " that bai, " << testSai << "/"
              << (sizeof(ds) / sizeof(ds[0])) << " test sai\n";
    return g_failed == 0 ? 0 : 1;
}
