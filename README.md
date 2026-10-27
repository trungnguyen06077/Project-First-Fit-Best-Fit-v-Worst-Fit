# Mô phỏng cấp phát bộ nhớ: First-Fit, Best-Fit, Worst-Fit

> **Bài tập lập trình giải thuật hệ điều hành** – Cơ chế Memory Management (cấp phát vùng nhớ liên tục)
> Ứng dụng **desktop GUI viết bằng C++** mô phỏng ba chiến lược: **First-Fit**, **Best-Fit**, **Worst-Fit**.

---

## 1. Mục tiêu

- Xây dựng ứng dụng desktop có GUI mô phỏng cơ chế cấp phát vùng nhớ của hệ điều hành.
- **Giải thích được mô hình**: bộ nhớ gồm các hole (khối trống) và các yêu cầu cấp phát; mỗi thuật toán có một quy tắc chọn hole riêng.
- **Chứng minh kết quả**: đối chiếu với pseudocode và kết quả tính tay trên dữ liệu có thể tái lập.
- **Đo hiệu năng** của ba thuật toán và **trình bày sản phẩm** có thể tái lập (cùng input thì luôn cùng output).

## 2. Quy mô và phân vai

Nhóm **5 sinh viên**, thực hiện trong khoảng **8 tuần**.

| MSSV | Họ tên | Vai trò | Phụ trách chính |
|---|---|---|---|
| `068207013575` | `Nguyễn Văn Trung` | Nhóm Trưởng | Phân việc,READme.md, models.hpp |
| `089207010161` | `Trần Lê Phước Gia Hưng` | Thành viên | csv_io.hpp/.cpp |
| `089207017978` | `Nguyễn Thế Vinh` | Thành viên | validator.hpp/.cpp |
| `079207020882` | `Trịnh Hoàng Hải` | Thành viên  | generator.hpp/cpp |
| `068207002805` | `Sú Tường Phúc Minh` | Thành viên | allocator.hpp/.cpp |

| Thông tin | Nội dung |
|---|---|
| Nhóm / Lớp | `OS_2627105_08` – `012012500105` |
| Giảng viên | `Mai Ngọc Châu` |


### Kế hoạch 8 tuần

| Tuần | Nội dung | Xong |
|---|---|---|
| 1 | Phân tích yêu cầu, phân vai, tạo repo, chọn thư viện GUI | ✓ |
| 2 | Pseudocode/flowchart, định nghĩa định dạng CSV, kế hoạch kiểm thử |✓|
| 3–4 | Cài đặt lõi thuật toán (`src/core`)  | ✓ |
| 5 | GUI, đọc/ghi CSV, kiểm tra dữ liệu, xử lý lỗi + unit test| ☐ |
| 6 | Kiểm thử tích hợp, edge case, đo hiệu năng | ☐ |
| 7 | Hoàn thiện, review chéo, sửa lỗi | ☐ |
| 8 | Báo cáo, quay video demo, trình bày | ☐ |

---

## 3. Nhiệm vụ

1. **Phân tích yêu cầu**
2. **Mô tả thuật toán**
3. **Xây dựng ứng dụng**
5. **Viết báo cáo**

---

## 4. Phân tích bài toán

Cho danh sách **hole** (khối nhớ trống, có địa chỉ bắt đầu và kích thước) và danh sách **yêu cầu cấp phát** theo thứ tự đến. Với mỗi yêu cầu, hệ thống chọn một hole đủ lớn theo chiến lược:

| Thuật toán | Quy tắc chọn hole | Ưu điểm | Nhược điểm |
|---|---|---|---|
| **First-Fit** | Hole **đầu tiên theo địa chỉ** có `size >= yêu cầu` | Nhanh, đơn giản | Phân mảnh dồn ở đầu bộ nhớ |
| **Best-Fit** | Hole **nhỏ nhất** thỏa `size >= yêu cầu` | Giữ lại hole lớn | Duyệt toàn bộ, sinh nhiều mảnh vụn |
| **Worst-Fit** | Hole **lớn nhất** thỏa `size >= yêu cầu` | Phần dư còn đủ lớn | Phá hole lớn, thường kém hiệu quả |

**Quy ước để kết quả tái lập được (deterministic):**

- Duyệt hole theo **địa chỉ bắt đầu tăng dần**.
- Khi có nhiều hole cùng kích thước (Best/Worst-Fit), chọn hole có **địa chỉ nhỏ nhất**.
- Cấp phát từ **đầu hole**: `alloc_start = hole.start`; phần dư `hole.start += size`, `hole.size -= size`. Nếu `hole.size == 0` thì xóa hole.
- Không có hole phù hợp thì ghi `FAILED`; không dừng chương trình, tiếp tục yêu cầu kế tiếp.
- Mỗi thuật toán chạy trên **bản sao độc lập** của danh sách hole ban đầu.

## 5. Mô tả thuật toán (pseudocode)

```text
FIRST_FIT(holes, size):
    for h in holes (địa chỉ tăng dần):
        if h.size >= size: return h
    return NONE

BEST_FIT(holes, size):
    best = NONE
    for h in holes:
        if h.size >= size and (best == NONE or h.size < best.size):
            best = h              # dấu < giữ hole có địa chỉ nhỏ nhất khi bằng nhau
    return best

WORST_FIT(holes, size):
    worst = NONE
    for h in holes:
        if h.size >= size and (worst == NONE or h.size > worst.size):
            worst = h
    return worst

SIMULATE(holes, requests, strategy):
    for each r in requests (theo thứ tự):
        h = strategy(holes, r.size)
        if h == NONE:
            ghi trace(r, FAILED)
        else:
            ghi trace(r, ALLOCATED, h.id, alloc_start = h.start)
            h.start += r.size;  h.size -= r.size
            if h.size == 0: xóa h khỏi holes
```

---

## 6. Yêu cầu kỹ thuật

| Yêu cầu |  |
|---|---|
| Ngôn ngữ **C++** |  
| **GUI chạy trên Windows**, không phải console, không phải Web App |  
| Xử lý lỗi, **không crash** với input hợp lệ | 
| **Tách phần thuật toán khỏi giao diện** | 

### Công cụ cần có

- Windows 10/11
- Visual Studio 2022 (MSVC) hoặc MinGW-w64
- CMake ≥ 3.21
- Qt 6.x (Qt Widgets)
- Git

### Cấu trúc thư mục

```text
memory-allocation-sim/
├── README.md
├── CMakeLists.txt
├── src/
│   ├── core/                  # Thuật toán + CSV (KHÔNG dùng Qt)
│   │   ├── models.hpp         # Hole, Request, TraceRecord, Metrics
│   │   ├── allocator.hpp/.cpp # First-Fit, Best-Fit, Worst-Fit
│   │   ├── generator.hpp/.cpp
│   │   ├── csv_io.hpp/.cpp    # Đọc / ghi CSV
│   │   └── validator.hpp/.cpp # Kiểm tra dữ liệu đầu vào
│   └── gui/                   # Giao diện Qt
│       ├── main.cpp
│       └── MainWindow.hpp/.cpp
├── tests/                     # Unit / integration / stress test
├── data/
│   ├── input/                 # CSV mẫu
│   │   ├── blocks_sample.csv
│   │   └── requests_sample.csv
│   └── output/                # Trace/kết quả xuất ra (CSV)
│       └── expected/          # Kết quả mong đợi để đối chiếu
└── docs/
    ├── flowchart_*.png
    ├── screenshots/
    ├── test_table.md          # Bảng kiểm thử
    ├── performance.md
    └── report.pdf
```

---


