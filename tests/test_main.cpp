// tests/test_main.cpp
// Bộ test đơn giản cho mô phỏng First-Fit / Best-Fit / Worst-Fit.
// Mã TC01..TC12 khớp với bảng kiểm thử trong docs/test_table.md.
//
// Chạy:  test_main [thu_muc_data] [so_luong_stress]
//        (mặc định: "data" và 10000)

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "core/allocator.hpp"
#include "core/csv_io.hpp"
#include "core/generator.hpp"
#include "core/validator.hpp"

using namespace bo_nho;

// =====================================================================
//  PHẦN 1: KHUNG TEST (3 macro là đủ)
// =====================================================================
static int g_checks = 0;  // tổng số phép kiểm tra
static int g_failed = 0;  // số phép kiểm tra sai

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(cond)) {                                                       \
            ++g_failed;                                                      \
            std::cerr << "  [FAIL] dong " << __LINE__ << ": " #cond "\n";    \
        }                                                                    \
    } while (0)

#define CHECK_EQ(a, b)                                                       \
    do {                                                                     \
        ++g_checks;                                                          \
        auto va = (a);                                                       \
        auto vb = (b);                                                       \
        if (!(va == vb)) {                                                   \
            ++g_failed;                                                      \
            std::cerr << "  [FAIL] dong " << __LINE__ << ": " #a " == " #b   \
                      << "\n    thuc te : " << va                            \
                      << "\n    mong doi: " << vb << "\n";                   \
        }                                                                    \
    } while (0)

#define CHECK_CONTAINS(hay, kim)                                             \
    do {                                                                     \
        ++g_checks;                                                          \
        std::string h_ = (hay);                                              \
        if (h_.find(kim) == std::string::npos) {                             \
            ++g_failed;                                                      \
            std::cerr << "  [FAIL] dong " << __LINE__ << ": khong thay \""   \
                      << kim << "\" trong: \"" << h_ << "\"\n";              \
        }                                                                    \
    } while (0)

// =====================================================================
//  PHẦN 2: HÀM HỖ TRỢ
// =====================================================================
static void writeText(const std::string& path, const std::string& text) {
    std::ofstream f(path, std::ios::binary);
    f << text;
}

// Thư mục tạm để ghi file CSV thử; tự xóa khi hết phạm vi.
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

// 5 hole mẫu (size 100, 500, 200, 300, 600), đặt cách nhau, không chồng lấn.
static std::vector<Hole> nam_hole() {
    return {{"H1", 0, 100, 0},    {"H2", 200, 500, 0},  {"H3", 800, 200, 0},
            {"H4", 1100, 300, 0}, {"H5", 1500, 600, 0}};
}

// 4 yêu cầu mẫu: 212, 417, 112, 426.
static std::vector<Request> bon_yeu_cau() {
    return {{1, "P1", 212, 0}, {2, "P2", 417, 0}, {3, "P3", 112, 0}, {4, "P4", 426, 0}};
}

// Một bước trong trace: block == "" nghĩa là cấp phát THẤT BẠI.
struct Buoc {
    const char* block;
    long long before, after;
};

// So kết quả simulate với bảng tính tay (expected) cho một thuật toán.
static void kiemTraThuatToan(Algorithm a, const std::vector<Buoc>& exp, int allocated,
                             int failed, long long totalFree) {
    SimulationResult r = simulate(nam_hole(), bon_yeu_cau(), a);
    CHECK_EQ(r.trace.size(), exp.size());
    for (size_t i = 0; i < exp.size() && i < r.trace.size(); i++) {
        const TraceRecord& t = r.trace[i];
        bool ok = exp[i].block[0] != '\0';
        CHECK_EQ(t.allocated, ok);
        CHECK_EQ(t.blockId, std::string(exp[i].block));
        if (ok) {
            CHECK_EQ(t.sizeBefore, exp[i].before);
            CHECK_EQ(t.sizeAfter, exp[i].after);
        }
    }
    CHECK_EQ(r.metrics.allocated, allocated);
    CHECK_EQ(r.metrics.failed, failed);
    CHECK_EQ(r.metrics.totalFree, totalFree);
}

static std::string readAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// Bỏ ký tự '\r' để so sánh không phụ thuộc Windows (CRLF) hay Linux (LF).
static std::string normalizeEol(std::string s) {
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
    return s;
}

// Bỏ cột cuối (time_us) của summary.csv vì thời gian chạy mỗi lần mỗi khác.
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

// Chạy cả 3 thuật toán giống main.cpp: gộp trace, mỗi thuật toán một dòng summary.
struct KetQua {
    std::vector<TraceRecord> trace;
    std::vector<Metrics> summary;
};

static KetQua chayBaThuatToan(const std::vector<Hole>& h, const std::vector<Request>& q) {
    KetQua kq;
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(h, q, a);
        kq.trace.insert(kq.trace.end(), r.trace.begin(), r.trace.end());
        kq.summary.push_back(r.metrics);
    }
    return kq;
}

// Có lỗi nào ở đúng dòng `dong` mà nội dung chứa `chua` không?
static bool coLoi(const std::vector<LoiDuLieu>& ds, int dong, const std::string& chua) {
    for (const auto& l : ds)
        if (l.dong == dong && l.noiDung.find(chua) != std::string::npos) return true;
    return false;
}

// =====================================================================
//  PHẦN 3: UNIT TEST THUẬT TOÁN
// =====================================================================

// Hàm chọn hole: trả về chỉ số hole được chọn, -1 nếu không có hole đủ lớn.
static void test_chonHole() {
    std::vector<Hole> h = nam_hole();  // size: 100 500 200 300 600

    CHECK_EQ(FirstFit(h, 212), 1);   // hole đầu tiên đủ lớn (500)
    CHECK_EQ(BestFit(h, 212), 3);    // hole nhỏ nhất đủ lớn (300)
    CHECK_EQ(WorstFit(h, 212), 4);   // hole lớn nhất (600)

    CHECK_EQ(FirstFit(h, 100), 0);   // vừa khít vẫn được chọn
    CHECK_EQ(BestFit(h, 100), 0);

    CHECK_EQ(FirstFit(h, 601), -1);  // không hole nào đủ
    CHECK_EQ(BestFit(h, 601), -1);
    CHECK_EQ(WorstFit(h, 601), -1);

    std::vector<Hole> rong;          // danh sách hole rỗng
    CHECK_EQ(FirstFit(rong, 1), -1);
    CHECK_EQ(BestFit(rong, 1), -1);
    CHECK_EQ(WorstFit(rong, 1), -1);
}

// TC06: nhiều hole cùng kích thước -> chọn hole có địa chỉ nhỏ nhất.
static void test_hoaDongGiaTri() {
    std::vector<Hole> h = {{"H1", 0, 300, 0}, {"H2", 400, 300, 0}};
    CHECK_EQ(FirstFit(h, 100), 0);
    CHECK_EQ(BestFit(h, 100), 0);
    CHECK_EQ(WorstFit(h, 100), 0);
}

// TC06 (qua simulate): dữ liệu vào bị đảo thứ tự vẫn chọn hole địa chỉ nhỏ hơn.
static void test_hoaDongKichThuocTrongSimulate() {
    std::vector<Hole> nguoc = {{"H2", 500, 300, 0}, {"H1", 0, 300, 0}};
    std::vector<Request> q = {{1, "P1", 100, 0}};
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(nguoc, q, a);
        CHECK_EQ(r.trace[0].blockId, std::string("H1"));
        CHECK_EQ(r.trace[0].allocStart, 0LL);
    }
}

// Ví dụ giáo trình: 3 thuật toán cho 3 kết quả khác nhau (đã tính tay).
static void test_viDuGiaoTrinh() {
    kiemTraThuatToan(Algorithm::FirstFit,
                     {{"H2", 500, 288}, {"H5", 600, 183}, {"H2", 288, 176}, {"", 0, 0}},
                     3, 1, 959);
    kiemTraThuatToan(Algorithm::BestFit,
                     {{"H4", 300, 88}, {"H2", 500, 83}, {"H3", 200, 88}, {"H5", 600, 174}},
                     4, 0, 533);
    kiemTraThuatToan(Algorithm::WorstFit,
                     {{"H5", 600, 388}, {"H2", 500, 83}, {"H5", 388, 276}, {"", 0, 0}},
                     3, 1, 959);
}

// TC04: cấp vừa khít -> hole dùng hết phải bị xóa.
static void test_vuaKhit() {
    std::vector<Hole> h = {{"H1", 50, 200, 0}};
    std::vector<Request> q = {{1, "P1", 200, 0}};
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(h, q, a);
        CHECK(r.trace[0].allocated);
        CHECK_EQ(r.trace[0].sizeAfter, 0LL);
        CHECK(r.finalHoles.empty());
        CHECK_EQ(r.metrics.totalFree, 0LL);
    }
}

// TC05: yêu cầu thất bại vẫn ghi vào trace và chương trình chạy tiếp.
static void test_thatBaiVaTiepTuc() {
    std::vector<Hole> h = {{"H1", 0, 100, 0}};
    std::vector<Request> q = {{1, "P1", 150, 0}, {2, "P2", 50, 0}, {3, "P3", 60, 0}};
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(h, q, a);
        CHECK_EQ(r.trace.size(), (size_t)3);
        CHECK(!r.trace[0].allocated);  // 150 > 100
        CHECK(r.trace[1].allocated);   // yêu cầu sau vẫn được xử lý
        CHECK(!r.trace[2].allocated);  // hole chỉ còn 50 < 60
        CHECK_EQ(r.metrics.allocated, 1);
        CHECK_EQ(r.metrics.failed, 2);
    }
}

// Phân mảnh ngoài: tổng trống đủ nhưng không có hole liên tục đủ lớn.
static void test_phanManhNgoai() {
    std::vector<Hole> h = {{"H1", 0, 100, 0}, {"H2", 110, 200, 0}, {"H3", 320, 300, 0}};
    std::vector<Request> q = {{1, "P1", 400, 0}};  // tổng trống 600 >= 400
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(h, q, a);
        CHECK(!r.trace[0].allocated);
        CHECK_EQ(r.metrics.totalFree, 600LL);
        CHECK_EQ(r.metrics.largestFree, 300LL);
    }
}

// TC07: không có yêu cầu nào -> mọi hole giữ nguyên, không crash.
static void tc07_yeuCauRong() {
    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(nam_hole(), {}, a);
        CHECK(r.trace.empty());
        CHECK_EQ(r.metrics.allocated, 0);
        CHECK_EQ(r.metrics.failed, 0);
        CHECK_EQ(r.metrics.totalFree, 1700LL);  // 100+500+200+300+600
        CHECK_EQ(r.finalHoles.size(), (size_t)5);
    }
    // Không có hole nào: mọi yêu cầu đều thất bại
    SimulationResult n = simulate({}, bon_yeu_cau(), Algorithm::FirstFit);
    CHECK_EQ(n.metrics.failed, 4);

    // Ghi trace rỗng: file chỉ có đúng dòng tiêu đề
    TempDir tmp("test_tmp_rong");
    std::string err;
    CHECK(writeTrace(tmp.file("trace.csv"), {}, err));
    CHECK_EQ(normalizeEol(readAll(tmp.file("trace.csv"))),
             std::string("step,algorithm,process_id,request_size,block_id,alloc_start,"
                         "block_size_before,block_size_after,status\n"));
}

// TC12: chạy 2 lần cho kết quả giống hệt (từng byte của trace.csv / summary.csv);
// dữ liệu đầu vào không bị sửa.
static void tc12_taiLap() {
    std::vector<Hole> h = nam_hole();
    std::vector<Request> q = bon_yeu_cau();
    KetQua a = chayBaThuatToan(h, q);
    KetQua b = chayBaThuatToan(h, q);

    TempDir tmp("test_tmp_taiLap");
    std::string err;
    CHECK(writeTrace(tmp.file("trace1.csv"), a.trace, err));
    CHECK(writeTrace(tmp.file("trace2.csv"), b.trace, err));
    CHECK(writeSummary(tmp.file("sum1.csv"), a.summary, err));
    CHECK(writeSummary(tmp.file("sum2.csv"), b.summary, err));
    CHECK_EQ(readAll(tmp.file("trace1.csv")), readAll(tmp.file("trace2.csv")));
    CHECK_EQ(boCotThoiGian(readAll(tmp.file("sum1.csv"))),
             boCotThoiGian(readAll(tmp.file("sum2.csv"))));

    CHECK_EQ(h.size(), (size_t)5);  // simulate không làm đổi danh sách hole gốc
    CHECK_EQ(h[1].size, 500LL);
}

// =====================================================================
//  PHẦN 4: TEST ĐỌC CSV + KIỂM TRA DỮ LIỆU (validator)
// =====================================================================
static std::string g_dataDir = "data";

// TC01-03: đọc file mẫu trong data/input rồi chạy cả 3 thuật toán.
static void tc01_03_duLieuMau() {
    std::vector<Hole> holes;
    std::vector<Request> reqs;
    std::string err;
    CHECK(readBlocks(g_dataDir + "/input/blocks_sample.csv", holes, err));
    CHECK(readRequests(g_dataDir + "/input/requests_sample.csv", reqs, err));
    CHECK_EQ(holes.size(), (size_t)5);
    CHECK_EQ(reqs.size(), (size_t)4);
    CHECK(kiemTraDanhSachHole(holes).empty());
    CHECK(kiemTraDanhSachYeuCau(reqs).empty());

    SimulationResult ff = simulate(holes, reqs, Algorithm::FirstFit);
    SimulationResult bf = simulate(holes, reqs, Algorithm::BestFit);
    SimulationResult wf = simulate(holes, reqs, Algorithm::WorstFit);
    CHECK_EQ(ff.metrics.allocated, 3);  // P4 thất bại
    CHECK_EQ(ff.metrics.failed, 1);
    CHECK_EQ(bf.metrics.allocated, 4);  // Best-Fit cấp được cả 4
    CHECK_EQ(bf.metrics.failed, 0);
    CHECK_EQ(wf.metrics.allocated, 3);  // P4 thất bại
    CHECK_EQ(wf.metrics.failed, 1);
}

// TC08: file thiếu cột / không tồn tại / rỗng / thiếu trường.
static void tc08_thieuCotVaFileLoi() {
    TempDir tmp("test_tmp_loi");
    std::vector<Hole> holes;
    std::vector<Request> reqs;
    std::string err;

    writeText(tmp.file("thieu_size.csv"), "block_id,start\nB1,0\n");
    CHECK(!readBlocks(tmp.file("thieu_size.csv"), holes, err));
    CHECK_CONTAINS(err, "thieu cot bat buoc");

    writeText(tmp.file("thieu_pid.csv"), "order,size\n1,100\n");
    CHECK(!readRequests(tmp.file("thieu_pid.csv"), reqs, err));
    CHECK_CONTAINS(err, "thieu cot bat buoc");

    CHECK(!readBlocks(tmp.file("khong_co.csv"), holes, err));
    CHECK_CONTAINS(err, "Khong the mo file");

    writeText(tmp.file("rong.csv"), "");
    CHECK(!readBlocks(tmp.file("rong.csv"), holes, err));
    CHECK_CONTAINS(err, "File rong");

    writeText(tmp.file("thieu_truong.csv"), "block_id,start,size\nB1,0,100\nB2,200\n");
    CHECK(!readBlocks(tmp.file("thieu_truong.csv"), holes, err));
    CHECK_CONTAINS(err, "dong 3");
    CHECK_CONTAINS(err, "thieu truong du lieu");
}

// TC09: size không phải số / âm / bằng 0.
static void tc09_sizeSai() {
    TempDir tmp("test_tmp_size");
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

    // size âm / bằng 0 -> đọc được nhưng validator phải bắt, đúng dòng
    writeText(tmp.file("am.csv"), "block_id,start,size\nB1,0,100\nB2,200,-5\nB3,400,0\n");
    CHECK(readBlocks(tmp.file("am.csv"), holes, err));
    auto loi = kiemTraDanhSachHole(holes);
    CHECK(coLoi(loi, 3, "size phai > 0"));
    CHECK(coLoi(loi, 4, "size phai > 0"));
    CHECK(!coLoi(loi, 2, "size"));  // dòng đúng thì không bị báo

    writeText(tmp.file("req_am.csv"), "order,process_id,size\n1,P1,-1\n0,P2,10\n");
    CHECK(readRequests(tmp.file("req_am.csv"), reqs, err));
    auto loiReq = kiemTraDanhSachYeuCau(reqs);
    CHECK(coLoi(loiReq, 2, "size phai > 0"));
    CHECK(coLoi(loiReq, 3, "order phai >= 1"));
}

// TC10: trùng block_id / order, hole chồng lấn địa chỉ.
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

    // Chồng lấn: [0,100) và [50,150)
    writeText(tmp.file("chong.csv"), "block_id,start,size\nB1,0,100\nB2,50,100\n");
    CHECK(readBlocks(tmp.file("chong.csv"), holes, err));
    CHECK(coLoi(kiemTraDanhSachHole(holes), 3, "chong lan"));

    // Hai hole liền kề [0,100) và [100,200) là HỢP LỆ
    writeText(tmp.file("lien_ke.csv"), "block_id,start,size\nB1,0,100\nB2,100,100\n");
    CHECK(readBlocks(tmp.file("lien_ke.csv"), holes, err));
    CHECK(kiemTraDanhSachHole(holes).empty());
}

// Toàn bộ đường ống: đọc CSV -> chạy 3 thuật toán -> ghi trace.csv, summary.csv
// -> so với data/output/expected/ (kết quả tính tay).
static void doiChieuExpected() {
    std::vector<Hole> holes;
    std::vector<Request> reqs;
    std::string err;
    CHECK(readBlocks(g_dataDir + "/input/blocks_sample.csv", holes, err));
    CHECK(readRequests(g_dataDir + "/input/requests_sample.csv", reqs, err));
    KetQua kq = chayBaThuatToan(holes, reqs);

    TempDir tmp("test_tmp_expected");
    CHECK(writeTrace(tmp.file("trace.csv"), kq.trace, err));
    CHECK(writeSummary(tmp.file("summary.csv"), kq.summary, err));

    std::string moTrace = normalizeEol(readAll(g_dataDir + "/output/expected/trace_expected.csv"));
    std::string moSummary = normalizeEol(readAll(g_dataDir + "/output/expected/summary_expected.csv"));
    CHECK(!moTrace.empty());    // thiếu file expected thì test KHÔNG được coi là đạt
    CHECK(!moSummary.empty());

    CHECK_EQ(normalizeEol(readAll(tmp.file("trace.csv"))), moTrace);
    CHECK_EQ(boCotThoiGian(readAll(tmp.file("summary.csv"))), moSummary);
}

// File CSV lưu từ Excel/Notepad trên Windows: có BOM, xuống dòng CRLF, thừa khoảng trắng.
static void csv_bomCrlfKhoangTrang() {
    TempDir tmp("test_tmp_bom");
    std::vector<Hole> holes;
    std::string err;

    writeText(tmp.file("windows.csv"),
              "\xEF\xBB\xBF" "block_id,start,size\r\nB1,0,100\r\n B2 , 150 , 500 \r\n\r\n");
    CHECK(readBlocks(tmp.file("windows.csv"), holes, err));
    CHECK_EQ(holes.size(), (size_t)2);
    if (holes.size() == 2) {
        CHECK_EQ(holes[1].id, std::string("B2"));
        CHECK_EQ(holes[1].start, 150LL);
        CHECK_EQ(holes[1].size, 500LL);
    }

    // Cột đảo thứ tự, và tên cột "id" được chấp nhận thay cho "block_id"
    writeText(tmp.file("doi_cot.csv"), "size,id,start\n100,B9,40\n");
    CHECK(readBlocks(tmp.file("doi_cot.csv"), holes, err));
    CHECK_EQ(holes.size(), (size_t)1);
    if (holes.size() == 1) {
        CHECK_EQ(holes[0].id, std::string("B9"));
        CHECK_EQ(holes[0].start, 40LL);
        CHECK_EQ(holes[0].size, 100LL);
    }
}

// Generator: cùng seed -> cùng dữ liệu; dữ liệu sinh ra phải hợp lệ; tham số sai không crash.
static void generator_taiLap() {
    auto a = generateHoles(50, 50, 300, 42);
    auto b = generateHoles(50, 50, 300, 42);
    auto c = generateHoles(50, 50, 300, 43);
    CHECK_EQ(a.size(), (size_t)50);

    bool giong = a.size() == b.size(), khac = false;
    for (size_t i = 0; giong && i < a.size(); i++)
        if (a[i].id != b[i].id || a[i].start != b[i].start || a[i].size != b[i].size) giong = false;
    for (size_t i = 0; i < a.size() && i < c.size(); i++)
        if (a[i].size != c[i].size) khac = true;
    CHECK(giong);  // cùng seed -> giống hệt
    CHECK(khac);   // khác seed -> khác
    for (const Hole& h : a) CHECK(h.size >= 50 && h.size <= 300);
    CHECK(kiemTraDanhSachHole(a).empty());

    auto r = generateRequests(50, 20, 200, 7);
    CHECK_EQ(r.size(), (size_t)50);
    for (size_t i = 0; i < r.size(); i++) CHECK_EQ(r[i].order, (int)i + 1);
    CHECK(kiemTraDanhSachYeuCau(r).empty());

    // Tham số không hợp lệ -> danh sách rỗng
    CHECK(generateHoles(0, 50, 300, 1).empty());
    CHECK(generateHoles(5, 300, 50, 1).empty());
    CHECK(generateRequests(-1, 20, 200, 1).empty());
}

// =====================================================================
//  PHẦN 5: STRESS TEST (TC11) - dữ liệu lớn, kiểm tra bất biến
//  Không có đáp án tính tay cho hàng nghìn yêu cầu, nên chỉ kiểm tra
//  những điều luôn phải đúng.
// =====================================================================
static int g_soLuongStress = 10000;

static void tc11_duLieuLon() {
    const int n = g_soLuongStress;
    std::cout << "    (TC11 dung " << n << " hole + " << n << " yeu cau)\n";
    std::vector<Hole> holes = generateHoles(n, 50, 300, 42);
    std::vector<Request> reqs = generateRequests(n, 20, 200, 43);
    CHECK_EQ(holes.size(), (size_t)n);
    CHECK_EQ(reqs.size(), (size_t)n);

    long long tongBanDau = 0;
    for (const Hole& h : holes) tongBanDau += h.size;

    for (Algorithm a : kAllAlgorithms) {
        SimulationResult r = simulate(holes, reqs, a);
        std::cout << "    " << displayName(a) << ": " << r.metrics.timeUs / 1000.0
                  << " ms, allocated=" << r.metrics.allocated
                  << " failed=" << r.metrics.failed << "\n";

        // 1. Mỗi yêu cầu đúng một dòng trace; thành công + thất bại = tổng
        CHECK_EQ(r.trace.size(), reqs.size());
        CHECK_EQ(r.metrics.allocated + r.metrics.failed, n);

        // 2. Bảo toàn bộ nhớ: tổng ban đầu = còn trống + đã cấp
        long long daCap = 0;
        for (const Allocation& x : r.allocations) daCap += x.size;
        CHECK_EQ(tongBanDau, r.metrics.totalFree + daCap);

        // 3. Hole còn lại luôn có size > 0 (hole size 0 phải bị xóa)
        int holeXau = 0;
        for (const Hole& h : r.finalHoles)
            if (h.size <= 0) ++holeXau;
        CHECK_EQ(holeXau, 0);
    }
}

// =====================================================================
//  PHẦN 6: MAIN
// =====================================================================
int main(int argc, char** argv) {
    if (argc > 1) g_dataDir = argv[1];
    if (argc > 2 && std::atoi(argv[2]) > 0) g_soLuongStress = std::atoi(argv[2]);

    struct Test {
        const char* ten;
        void (*ham)();
    };
    const Test ds[] = {
        {"chon hole (First/Best/Worst, vua khit, khong du)", test_chonHole},
        {"TC01-03 du lieu mau tu file CSV", tc01_03_duLieuMau},
        {"vi du giao trinh: 3 thuat toan cho 3 ket qua khac nhau", test_viDuGiaoTrinh},
        {"TC04 cap vua khit -> xoa hole", test_vuaKhit},
        {"TC05 that bai van ghi trace, tiep tuc", test_thatBaiVaTiepTuc},
        {"TC06 hoa dong gia tri -> dia chi nho nhat", test_hoaDongGiaTri},
        {"TC06 hoa dong gia tri trong simulate (dau vao bi dao)", test_hoaDongKichThuocTrongSimulate},
        {"TC07 danh sach rong", tc07_yeuCauRong},
        {"TC08 file loi: thieu cot / khong ton tai / rong", tc08_thieuCotVaFileLoi},
        {"TC09 size khong phai so / am / bang 0", tc09_sizeSai},
        {"TC10 trung ID / order, chong lan dia chi", tc10_trungVaChongLan},
        {"doi chieu trace.csv / summary.csv voi data/output/expected", doiChieuExpected},
        {"CSV kieu Windows: BOM, CRLF, khoang trang, doi thu tu cot", csv_bomCrlfKhoangTrang},
        {"generator: tai lap, hop le, tham so sai", generator_taiLap},
        {"phan manh ngoai", test_phanManhNgoai},
        {"TC11 du lieu lon (stress)", tc11_duLieuLon},
        {"TC12 tai lap ket qua", tc12_taiLap},
    };

    int testSai = 0;
    for (const Test& t : ds) {
        int truoc = g_failed;
        t.ham();
        bool ok = g_failed == truoc;
        if (!ok) ++testSai;
        std::cout << (ok ? "[ OK ] " : "[FAIL] ") << t.ten << "\n";
    }
    std::cout << "\n" << g_checks << " phep kiem tra, " << g_failed << " that bai, "
              << testSai << "/" << (sizeof(ds) / sizeof(ds[0])) << " test sai\n";
    return g_failed == 0 ? 0 : 1;
}