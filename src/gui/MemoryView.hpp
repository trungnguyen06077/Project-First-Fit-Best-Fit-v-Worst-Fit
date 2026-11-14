#pragma once

#include <string>
#include <vector>

#include <QColor>
#include <QPaintEvent>
#include <QWidget>

#include "core/models.hpp"

class MemoryView : public QWidget {
    Q_OBJECT

public:
    explicit MemoryView(QWidget* cha = nullptr);

    void datDuLieu(const std::vector<bo_nho::Hole>& danhSachHoleMoi, const std::vector<bo_nho::Allocation>& danhSachCapPhatMoi);
    void xoaDuLieu();

protected:
    void paintEvent(QPaintEvent* suKien) override;

private:
    QColor layMauTheoTienTrinh(const std::string& process_id) const;
    long long tinhTongDiaChi() const;

    std::vector<bo_nho::Hole> danhSachHole;
    std::vector<bo_nho::Allocation> danhSachCapPhat;
};
