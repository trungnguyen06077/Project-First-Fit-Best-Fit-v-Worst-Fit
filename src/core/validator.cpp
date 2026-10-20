#include "validator.hpp"

#include <algorithm>
#include <set>
#include <utility>

namespace {

const long long GIA_TRI_TOI_DA = 1000000000;

void themLoi(std::vector<LoiDuLieu>& danhSachLoi, int dong, const std::string& noiDung) {
    LoiDuLieu loi;
    loi.dong = dong;
    loi.noiDung = noiDung;
    danhSachLoi.push_back(loi);
}

}

std::vector<LoiDuLieu> kiemTraDanhSachHole(const std::vector<Hole>& danhSachHole) {
    std::vector<LoiDuLieu> danhSachLoi;
    std::set<std::string> cacIdDaGap;

    for (size_t i = 0; i < danhSachHole.size(); i++) {
        int dong = static_cast<int>(i) + 2;

        if (danhSachHole[i].id.empty()) {
            themLoi(danhSachLoi, dong, "block_id rong");
        } else if (cacIdDaGap.count(danhSachHole[i].id) > 0) {
            themLoi(danhSachLoi, dong, "block_id bi trung: " + danhSachHole[i].id);
        } else {
            cacIdDaGap.insert(danhSachHole[i].id);
        }

        if (danhSachHole[i].start < 0) {
            themLoi(danhSachLoi, dong, "start phai >= 0");
        } else if (danhSachHole[i].start > GIA_TRI_TOI_DA) {
            themLoi(danhSachLoi, dong, "start qua lon");
        }

        if (danhSachHole[i].size <= 0) {
            themLoi(danhSachLoi, dong, "size phai > 0");
        } else if (danhSachHole[i].size > GIA_TRI_TOI_DA) {
            themLoi(danhSachLoi, dong, "size qua lon");
        }
    }

    std::vector<std::pair<long long, int> > danhSachDaSapXep;
    for (size_t i = 0; i < danhSachHole.size(); i++) {
        if (danhSachHole[i].start >= 0 && danhSachHole[i].size > 0) {
            danhSachDaSapXep.push_back(std::make_pair(static_cast<long long>(danhSachHole[i].start), static_cast<int>(i)));
        }
    }
    std::sort(danhSachDaSapXep.begin(), danhSachDaSapXep.end());

    for (size_t k = 1; k < danhSachDaSapXep.size(); k++) {
        int a = danhSachDaSapXep[k - 1].second;
        int b = danhSachDaSapXep[k].second;
        long long diemCuoiCuaA = static_cast<long long>(danhSachHole[a].start) + danhSachHole[a].size;
        if (diemCuoiCuaA > danhSachHole[b].start) {
            int dongA = a + 2;
            int dongB = b + 2;
            int dongBaoLoi = std::max(dongA, dongB);
            int dongConLai = std::min(dongA, dongB);
            themLoi(danhSachLoi, dongBaoLoi, "hole chong lan dia chi voi dong " + std::to_string(dongConLai));
        }
    }

    return danhSachLoi;
}

std::vector<LoiDuLieu> kiemTraDanhSachYeuCau(const std::vector<Request>& danhSachYeuCau) {
    std::vector<LoiDuLieu> danhSachLoi;
    std::set<long long> cacOrderDaGap;

    for (size_t i = 0; i < danhSachYeuCau.size(); i++) {
        int dong = static_cast<int>(i) + 2;

        if (danhSachYeuCau[i].order < 1) {
            themLoi(danhSachLoi, dong, "order phai >= 1");
        } else if (cacOrderDaGap.count(danhSachYeuCau[i].order) > 0) {
            themLoi(danhSachLoi, dong, "order bi trung: " + std::to_string(danhSachYeuCau[i].order));
        } else {
            cacOrderDaGap.insert(danhSachYeuCau[i].order);
        }

        if (danhSachYeuCau[i].process_id.empty()) {
            themLoi(danhSachLoi, dong, "process_id rong");
        }

        if (danhSachYeuCau[i].size <= 0) {
            themLoi(danhSachLoi, dong, "size phai > 0");
        } else if (danhSachYeuCau[i].size > GIA_TRI_TOI_DA) {
            themLoi(danhSachLoi, dong, "size qua lon");
        }
    }

    return danhSachLoi;
}

std::string taoThongBaoLoi(const std::string& tenFile, const std::vector<LoiDuLieu>& danhSachLoi) {
    std::string vanBan;
    for (size_t i = 0; i < danhSachLoi.size(); i++) {
        vanBan += tenFile + ", dong " + std::to_string(danhSachLoi[i].dong) + ": " + danhSachLoi[i].noiDung + "\n";
    }
    return vanBan;
}
