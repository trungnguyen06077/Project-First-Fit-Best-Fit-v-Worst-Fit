#include "MemoryView.hpp"

#include <QFontMetrics>
#include <QPainter>
#include <QRect>
#include <QString>

MemoryView::MemoryView(QWidget* cha) : QWidget(cha) {
    setMinimumHeight(140);
}

void MemoryView::datDuLieu(const std::vector<bo_nho::Hole>& danhSachHoleMoi, const std::vector<bo_nho::Allocation>& danhSachCapPhatMoi) {
    danhSachHole = danhSachHoleMoi;
    danhSachCapPhat = danhSachCapPhatMoi;
    update();
}

void MemoryView::xoaDuLieu() {
    danhSachHole.clear();
    danhSachCapPhat.clear();
    update();
}

QColor MemoryView::layMauTheoTienTrinh(const std::string& process_id) const {
    int tong = 0;
    for (size_t i = 0; i < process_id.size(); i++) {
        tong = tong * 31 + process_id[i];
    }
    int mau = ((tong % 360) + 360) % 360;
    return QColor::fromHsv(mau, 140, 230);
}

long long MemoryView::tinhTongDiaChi() const {
    long long tong = 0;
    for (size_t i = 0; i < danhSachHole.size(); i++) {
        long long diemCuoi = danhSachHole[i].start + danhSachHole[i].size;
        if (diemCuoi > tong) {
            tong = diemCuoi;
        }
    }
    for (size_t i = 0; i < danhSachCapPhat.size(); i++) {
        long long diemCuoi = danhSachCapPhat[i].start + danhSachCapPhat[i].size;
        if (diemCuoi > tong) {
            tong = diemCuoi;
        }
    }
    return tong;
}

void MemoryView::paintEvent(QPaintEvent* suKien) {
    Q_UNUSED(suKien);

    QPainter veTranh(this);
    veTranh.fillRect(rect(), Qt::white);

    long long tongDiaChi = tinhTongDiaChi();
    if (tongDiaChi <= 0) {
        veTranh.setPen(Qt::gray);
        veTranh.drawText(rect(), Qt::AlignCenter, "Chua co du lieu");
        return;
    }

    int le = 20;
    int trai = le;
    int rongThanh = width() - 2 * le;
    int tren = height() / 2 - 30;
    int caoThanh = 60;

    if (rongThanh <= 0) {
        return;
    }

    veTranh.fillRect(trai, tren, rongThanh, caoThanh, QColor(190, 190, 190));

    for (size_t i = 0; i < danhSachHole.size(); i++) {
        int x = trai + static_cast<int>(static_cast<double>(danhSachHole[i].start) * rongThanh / tongDiaChi);
        int rong = static_cast<int>(static_cast<double>(danhSachHole[i].size) * rongThanh / tongDiaChi);
        if (rong < 1) {
            rong = 1;
        }
        QRect khung(x, tren, rong, caoThanh);
        veTranh.fillRect(khung, QColor(120, 200, 120));
        veTranh.setPen(Qt::black);
        veTranh.drawRect(khung);

        veTranh.save();
        veTranh.setClipRect(khung);
        veTranh.drawText(khung, Qt::AlignCenter, QString::fromStdString(danhSachHole[i].id));
        veTranh.restore();
    }

    for (size_t i = 0; i < danhSachCapPhat.size(); i++) {
        int x = trai + static_cast<int>(static_cast<double>(danhSachCapPhat[i].start) * rongThanh / tongDiaChi);
        int rong = static_cast<int>(static_cast<double>(danhSachCapPhat[i].size) * rongThanh / tongDiaChi);
        if (rong < 1) {
            rong = 1;
        }
        QRect khung(x, tren, rong, caoThanh);
        veTranh.fillRect(khung, layMauTheoTienTrinh(danhSachCapPhat[i].processId));
        veTranh.setPen(Qt::black);
        veTranh.drawRect(khung);

        veTranh.save();
        veTranh.setClipRect(khung);
        veTranh.drawText(khung, Qt::AlignCenter, QString::fromStdString(danhSachCapPhat[i].processId));
        veTranh.restore();
    }

    veTranh.setPen(Qt::black);
    veTranh.drawRect(trai, tren, rongThanh, caoThanh);
    veTranh.drawText(trai, tren + caoThanh + 18, "0");
    QString nhanCuoi = QString::number(tongDiaChi);
    int rongNhan = veTranh.fontMetrics().horizontalAdvance(nhanCuoi);
    veTranh.drawText(trai + rongThanh - rongNhan, tren + caoThanh + 18, nhanCuoi);

    int yChuThich = tren - 12;
    veTranh.fillRect(trai, yChuThich - 10, 12, 12, QColor(120, 200, 120));
    veTranh.drawText(trai + 18, yChuThich, "Hole trong");
    veTranh.fillRect(trai + 110, yChuThich - 10, 12, 12, QColor(190, 190, 190));
    veTranh.drawText(trai + 128, yChuThich, "Ngoai hole");
    veTranh.fillRect(trai + 220, yChuThich - 10, 12, 12, QColor(230, 150, 100));
    veTranh.drawText(trai + 238, yChuThich, "Da cap phat (ten tien trinh)");
}
