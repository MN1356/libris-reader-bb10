# BerryReader - Native E-Book Reader for BlackBerry 10

Ứng dụng đọc sách điện tử (E-Book Reader) chuẩn **Native BlackBerry 10 Cascades (C++ / Qt 4.8 / QML)**, đem lại giao diện mượt mà 60fps, hỗ trợ đầy đủ cử chỉ Cascades, Active Frame trực tiếp ngoài màn hình chính, và phím tắt phần cứng cho các dòng máy BlackBerry Passport, Classic, Q10, Q5, Z10, Z30.

---

## 1. Tính năng nổi bật

* **Giao diện Cascades QML nguyên bản:**
  * Đạt chuẩn BlackBerry 10 Human Interface Guidelines (HIG).
  * Hiệu ứng chuyển động mượt mà 60fps, vuốt cạnh để Peek / chuyển trang.
  * Tích hợp **`FilePicker`** hệ thống để duyệt mở sách `.epub` từ bộ nhớ trong hoặc thẻ nhớ ngoài (SD Card).
* **Active Frame thực thụ (Application Cover):**
  * Khi vuốt từ cạnh dưới để thu nhỏ ứng dụng ra màn hình Home, Active Frame hiển thị bìa sách, tên sách, tên chương và % tiến độ đọc theo thời gian thực.
* **Tối ưu hiển thị & Pin AMOLED:**
  * **AMOLED Pure Black (`#000000`):** Tiết kiệm pin tối đa cho màn hình Super AMOLED trên Q10 và Z30.
  * Hỗ trợ 3 Theme: Dark, Sepia, và Light.
  * Tùy biến cỡ chữ trực tiếp ngay trên thanh công cụ.
* **Hỗ trợ phím tắt phần cứng (Passport, Classic, Q10):**
  * `Phím Space`: Lật trang / sang chương tiếp theo.
  * `Phím N` / `Phím P`: Sang chương sau / lùi chương trước.
  * `Phím T` / `Phím B`: Về đầu sách (*Top*) / đến cuối sách (*Bottom*).
* **Lõi đọc sách độc lập & Siêu nhẹ:**
  * Tích hợp bộ giải nén zip `miniz` và bộ phân tích XML `pugixml` trực tiếp trong mã nguồn C++.
  * Tự động quét sách trong các thư mục `/accounts/1000/shared/documents` và `/accounts/1000/shared/books`.

---

## 2. Cấu trúc thư mục

```text
BB10-Book-reader/
├── bar-descriptor.xml         # Cấu hình gói BAR (quyền, icon, app ID)
├── BerryReader.pro            # Cấu hình QMake (cascades10, QtDeclarative, QtXml...)
├── build.sh                   # Script tự động build bằng container BBNDK
├── src/                       # Mã nguồn C++ Native
│   ├── main.cpp               # Điểm khởi động ứng dụng
│   ├── ApplicationUI.hpp/cpp  # Khởi tạo QML Scene và Active Frame
│   ├── BookManager.hpp/cpp    # Quản lý tủ sách, tiến độ, bookmarks, font
│   ├── EpubParser.hpp/cpp     # Lõi bóc tách file EPUB (ZIP, OPF, NCX, HTML)
│   ├── miniz.c / miniz.h      # Thư viện giải nén zip trong bộ nhớ
│   └── pugixml.cpp / .hpp     # Thư viện phân tích cú pháp XML/OPF
├── assets/                    # Giao diện Cascades QML
│   ├── main.qml               # NavigationPane chính
│   ├── LibraryPage.qml        # Màn hình Tủ sách (kèm FilePicker)
│   ├── ReaderPage.qml         # Màn hình đọc sách (WebView + phím tắt)
│   ├── Cover.qml              # Active Frame ngoài Homescreen
│   └── icons/                 # Các icon giao diện
└── build/                     # Thư mục chứa gói BAR và binary sau khi build
    ├── device/BerryReader-unsigned.bar
    └── simulator/BerryReader-simulator-unsigned.bar
```

---

## 3. Cách biên dịch (Build)

Dự án sử dụng container `accupara/bbndk` (chứa BBNDK 10.3.1.995 / tools 10.3.1.12) qua `podman` hoặc `docker`:

### Build cho thiết bị thật (ARMv7 - Passport, Classic, Q10, Z30...):
```bash
./build.sh device
```
*Kết quả xuất ra:* `build/device/BerryReader-unsigned.bar`

### Build cho máy ảo (x86 Simulator):
```bash
./build.sh simulator
```
*Kết quả xuất ra:* `build/simulator/BerryReader-simulator-unsigned.bar`

---

## 4. Cách cài đặt lên BlackBerry 10

1. **Bật Development Mode trên điện thoại:**
   * Vào `Cài đặt (Settings)` -> `Bảo mật và riêng tư (Security and Privacy)` -> `Chế độ phát triển (Development Mode)`.
   * Bật công tắc sang **ON** và đặt mật khẩu thiết bị (nếu chưa có).
   * Ghi nhớ địa chỉ IP của máy (ví dụ `192.168.1.15` qua Wi-Fi hoặc `169.254.0.1` qua cáp USB).

2. **Cài đặt bằng Sachesi / DBBT (Khuyên dùng trên Windows/Mac/Linux):**
   * Kết nối BB10 với máy tính qua cáp USB hoặc Wi-Fi.
   * Mở phần mềm Sachesi / DBBT, chuyển sang tab **Install**.
   * Kéo thả file `build/device/BerryReader-unsigned.bar` vào để cài đặt.

3. **Cài đặt bằng dòng lệnh qua `blackberry-deploy`:**
   ```bash
   podman run --rm -v "$PWD/build/device:/output:ro,z" docker.io/accupara/bbndk:latest \
       blackberry-deploy -installApp -device <IP_THIET_BI> -password <MAT_KHAU> /output/BerryReader-unsigned.bar
   ```
