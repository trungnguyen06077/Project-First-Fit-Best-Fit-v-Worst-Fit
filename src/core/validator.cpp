#include "validator.hpp"

#include <algorithm>
#include <set>
#include <utility>

namespace bo_nho {
namespace {

const long long GIA_TRI_TOI_DA = 1000000000;

void themLoi(std::vector<LoiDuLieu>& ds, int dong, const std::string& noiDung) {
    LoiDuLieu loi;
    loi.dong = dong;
    loi.noiDung = noiDung;
    ds.push_back(loi);
}

// Dùng số dòng thật trong file nếu có, nếu không thì suy ra từ vị trí (dòng 1 là header).
int dongCuaHole(const std::vector<Hole>& ds, size_t i) { return ds[i].line > 0 ? ds[i].line : (int)i + 2; }
int dongCuaYeuCau(const std::vector<Request>& ds, size_t i) { return ds[i].line > 0 ? ds[i].line : (int)i + 2; }

}  // namespace

std::vector<LoiDuLieu> kiemTraDanhSachHole(const std::vector<Hole>& ds) {
    std::vector<LoiDuLieu> loi;
    std::set<std::string> daGap;

    for (size_t i = 0; i < ds.size(); i++) {
        int dong = dongCuaHole(ds, i);

        if (ds[i].id.empty()) {
            themLoi(loi, dong, "block_id rong");
        } else if (daGap.count(ds[i].id) > 0) {
            themLoi(loi, dong, "block_id bi trung: " + ds[i].id);
        } else {
            daGap.insert(ds[i].id);
        }

        if (ds[i].start < 0) themLoi(loi, dong, "start phai >= 0");
        else if (ds[i].start > GIA_TRI_TOI_DA) themLoi(loi, dong, "start qua lon");

        if (ds[i].size <= 0) themLoi(loi, dong, "size phai > 0");
        else if (ds[i].size > GIA_TRI_TOI_DA) themLoi(loi, dong, "size qua lon");
    }

    std::vector<std::pair<long long, int> > sapXep;
    for (size_t i = 0; i < ds.size(); i++)
        if (ds[i].start >= 0 && ds[i].size > 0) sapXep.push_back({ds[i].start, (int)i});
    std::sort(sapXep.begin(), sapXep.end());

    for (size_t k = 1; k < sapXep.size(); k++) {
        size_t a = (size_t)sapXep[k - 1].second;
        size_t b = (size_t)sapXep[k].second;
        if (ds[a].start + ds[a].size > ds[b].start) {
            int dongA = dongCuaHole(ds, a);
            int dongB = dongCuaHole(ds, b);
            themLoi(loi, std::max(dongA, dongB),
                    "hole chong lan dia chi voi dong " + std::to_string(std::min(dongA, dongB)));
        }
    }
    return loi;
}

std::vector<LoiDuLieu> kiemTraDanhSachYeuCau(const std::vector<Request>& ds) {
    std::vector<LoiDuLieu> loi;
    std::set<int> daGap;

    for (size_t i = 0; i < ds.size(); i++) {
        int dong = dongCuaYeuCau(ds, i);

        if (ds[i].order < 1) {
            themLoi(loi, dong, "order phai >= 1");
        } else if (daGap.count(ds[i].order) > 0) {
            themLoi(loi, dong, "order bi trung: " + std::to_string(ds[i].order));
        } else {
            daGap.insert(ds[i].order);
        }

        if (ds[i].processId.empty()) themLoi(loi, dong, "process_id rong");

        if (ds[i].size <= 0) themLoi(loi, dong, "size phai > 0");
        else if (ds[i].size > GIA_TRI_TOI_DA) themLoi(loi, dong, "size qua lon");
    }
    return loi;
}

std::string taoThongBaoLoi(const std::string& tenFile, const std::vector<LoiDuLieu>& ds) {
    std::string s;
    for (const auto& l : ds) s += tenFile + ", dong " + std::to_string(l.dong) + ": " + l.noiDung + "\n";
    return s;
}

}  // namespace bo_nho
