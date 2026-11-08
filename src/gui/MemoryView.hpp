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

    void datDuLieu(const std::vector<Hole>& danhSachHoleMoi, const std::vector<Allocation>& danhSachCapPhatMoi);
    void xoaDuLieu();

protected:
    void paintEvent(QPaintEvent* suKien) override;

private:
    QColor layMauTheoTienTrinh(const std::string& process_id) const;
    long long tinhTongDiaChi() const;

    std::vector<Hole> danhSachHole;
    std::vector<Allocation> danhSachCapPhat;
};
