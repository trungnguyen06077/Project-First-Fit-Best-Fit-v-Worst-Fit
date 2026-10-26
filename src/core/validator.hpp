#pragma once
#include <string>
#include <vector>

#include "models.hpp"

namespace bo_nho {

struct LoiDuLieu {
    int dong = 0;
    std::string noiDung;
};

std::vector<LoiDuLieu> kiemTraDanhSachHole(const std::vector<Hole>& danhSachHole);
std::vector<LoiDuLieu> kiemTraDanhSachYeuCau(const std::vector<Request>& danhSachYeuCau);
std::string taoThongBaoLoi(const std::string& tenFile, const std::vector<LoiDuLieu>& danhSachLoi);

}
