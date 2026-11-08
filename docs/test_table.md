# Bảng kiểm thử

## A. Kiểm thử tự động (phần lõi) - file `tests/test_main.cpp`

Chạy toàn bộ: `ctest --test-dir build -C Release --output-on-failure`
Chạy trực tiếp (từ thư mục gốc dự án): `build/test_main data` (Windows/MSVC: `build\Release\test_main.exe data`)
TC11 đúng cỡ README: `build/test_main data 100000`

| Mã | Mô tả | Input | Kết quả mong đợi | Kết quả thực tế | Đạt | Hàm test trong `test_main.cpp` |
|---|---|---|---|---|---|---|
| TC01 | Dữ liệu mẫu, First-Fit | `data/input/*_sample.csv` | P4 FAILED, 3 cấp phát | allocated=3, failed=1; từng bước khớp bảng mục 9 README | ☑ | `tc01_03_duLieuMau` |
| TC02 | Dữ liệu mẫu, Best-Fit | sample | Cả 4 ALLOCATED | allocated=4, failed=0; totalFree=533, largestFree=174 | ☑ | `tc01_03_duLieuMau` |
| TC03 | Dữ liệu mẫu, Worst-Fit | sample | P4 FAILED, 3 cấp phát | allocated=3, failed=1; từng bước khớp bảng mục 9 | ☑ | `tc01_03_duLieuMau` |
| TC04 | Yêu cầu bằng đúng kích thước hole | hole 200, yêu cầu 200 | Hole bị xóa, phần dư 0 | `sizeAfter=0`, `finalHoles` rỗng, `totalFree=0` | ☑ | `test_vuaKhit` |
| TC05 | Yêu cầu lớn hơn mọi hole | yêu cầu > max | FAILED, không crash | `allocated=false`, `blockId` rỗng, các yêu cầu sau vẫn được xử lý | ☑ | `test_thatBaiVaTiepTuc`, `test_chonHole` |
| TC06 | Nhiều hole cùng kích thước | 2 hole bằng nhau | Chọn hole địa chỉ nhỏ hơn | Chọn đúng hole địa chỉ nhỏ hơn, kể cả khi dữ liệu vào bị đảo thứ tự | ☑ | `test_hoaDongGiaTri`, `test_hoaDongKichThuocTrongSimulate` |
| TC07 | Danh sách yêu cầu rỗng | file chỉ có header | Kết quả rỗng, không crash | Trace rỗng, `totalFree` không đổi (1700), file trace chỉ có dòng tiêu đề | ☑ | `tc07_yeuCauRong` |
| TC08 | CSV thiếu cột | bỏ cột `size` / `process_id` | Báo lỗi rõ ràng | Báo `thieu cot bat buoc`; cũng bắt file không tồn tại, file rỗng, dòng thiếu trường (kèm số dòng) | ☑ | `tc08_thieuCotVaFileLoi` |
| TC09 | `size` không phải số / âm | `abc`, `10.5`, `-5`, `0` | Báo lỗi đúng dòng | `abc`, `10.5` → `sai dinh dang so`; `-5`, `0` → `size phai > 0`; đúng số dòng | ☑ | `tc09_sizeSai` |
| TC10 | `block_id` trùng | 2 dòng cùng ID | Báo lỗi trùng ID | Báo `block_id bi trung`, `order bi trung`, `chong lan` đúng dòng; hole liền kề được chấp nhận | ☑ | `tc10_trungVaChongLan` |
| TC11 | Dữ liệu lớn (stress) | 100.000 hole, 100.000 yêu cầu | Chạy xong, kết quả nhất quán | Chạy xong, 0 lỗi: bảo toàn bộ nhớ, không chồng lấn, mỗi yêu cầu đúng 1 dòng trace. Thời gian mô phỏng: First-Fit ≈ 8 s, Best-Fit ≈ 39 s, Worst-Fit ≈ 37 s | ☑ | `tc11_duLieuLon` |
| TC12 | Tái lập kết quả | chạy 2 lần cùng input | Hai output giống hệt nhau | `trace.csv` giống từng byte; `summary.csv` giống (trừ cột `time_us`) | ☑ | `tc12_taiLap` |

### Ghi chú

- Thời gian ở TC11 đo bằng gcc -O2 trên máy của người soạn bảng; máy khác, trình biên dịch khác hoặc build Debug cho số khác. **Cần chạy lại trên máy nhóm và cập nhật số trước khi nộp.**
- Chi phí là O(n²) (mỗi yêu cầu duyệt O(n) hole), nên tăng n 10 lần thì thời gian tăng khoảng 100 lần. `ctest` mặc định dùng n = 10.000 (khoảng 2 giây).
- `summary_expected.csv` không có cột `time_us` vì thời gian chạy mỗi lần mỗi khác; test so sánh các cột còn lại.
- `data/output/expected/trace_expected.csv` được tính tay từ bảng mục 9 README, không sao chép từ output của chương trình.

## B. Kiểm thử giao diện GUI (làm tay khi GUI xong)

Dùng dữ liệu mẫu `data/input/blocks_sample.csv` và `requests_sample.csv`. Chụp ảnh từng bước vào `docs/screenshots/`.

| Mã | Thao tác | Kết quả mong đợi | Kết quả thực tế | Đạt |
|---|---|---|---|---|
| GUI01 | Mở GUI, nạp 2 file mẫu | Hiện 5 hole và 4 yêu cầu, không lỗi | `<điền>` | ☐ |
| GUI02 | Chọn First-Fit, bấm Run | P4 FAILED, 3 cấp phát, giống bảng mục 9 | `<điền>` | ☐ |
| GUI03 | Chọn Best-Fit, bấm Run | Cả 4 ALLOCATED, giống bảng mục 9 | `<điền>` | ☐ |
| GUI04 | Chọn Worst-Fit, bấm Run | P4 FAILED, 3 cấp phát, giống bảng mục 9 | `<điền>` | ☐ |
| GUI05 | Bấm Step nhiều lần | Mỗi lần bấm xử lý đúng 1 yêu cầu, hình bộ nhớ đổi theo | `<điền>` | ☐ |
| GUI06 | Chọn "So sánh cả ba" | Bảng so sánh 3 thuật toán đúng số liệu mục 9 | `<điền>` | ☐ |
| GUI07 | Bấm Export CSV | Ghi ra `data/output/`; trace khớp `expected/` | `<điền>` | ☐ |
| GUI08 | Nạp file CSV bị lỗi (thiếu cột, size = abc) | Hiện thông báo nêu rõ file, dòng, nguyên nhân; GUI không crash | `<điền>` | ☐ |
| GUI09 | Nạp file yêu cầu chỉ có header | Không crash, kết quả rỗng | `<điền>` | ☐ |
| GUI10 | Chạy lại 2 lần cùng dữ liệu | Hai kết quả giống nhau | `<điền>` | ☐ |
