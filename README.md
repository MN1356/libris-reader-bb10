# Libris - Native E-Book Reader for BlackBerry 10

Libris is a high-performance native e-book reader built for **BlackBerry 10 Cascades (C++ / Qt 4.8 / QML)**. It delivers a fluid 60fps reading experience, native Cascades gestures, real-time Active Frame cover display, and hardware keyboard shortcuts tailored for devices such as the BlackBerry Passport, Classic, Q10, Q5, Z10, and Z30.

---

## 1. Key Features

* **Multi-Format Document Support:**
  * Reads **EPUB**, **MOBI**, **CBZ**, **FB2**, **TXT**, **PDF**, and **DJVU** documents seamlessly (integrated Libris PDF/DjVu Core engine).
  * Self-contained, lightweight architecture powered by `miniz` (in-memory ZIP extraction), `pugixml` (fast XML parsing), and native PalmDOC/HUFF decompression.

* **Pure Cascades QML Experience:**
  * Fully compliant with BlackBerry 10 Human Interface Guidelines (HIG).
  * Native Left-Swipe Drawer sidebar combined with TabbedPane navigation: Recent, Library, Table of Contents, Bookmarks & Notes, and File Browser.
  * System `FilePicker` integration alongside an in-app file browser to load books from internal storage or external SD Card.

* **Real-Time Active Frame (Application Cover):**
  * Displays the book cover, title, current chapter, and reading progress (%) in real time when minimized to the home screen.

* **Display & Battery Optimization:**
  * **AMOLED Pure Black (`#000000`):** Maximizes battery life on Super AMOLED displays (Q10, Z30).
  * 4 reading themes: AMOLED Black, Dark, Sepia, and Light.
  * Customizable typography: font size, line spacing, margins, and status footer (battery, clock, progress).

* **Reading & Study Tools:**
  * **Text-to-Speech (TTS) with Sleep Timer:** Native BlackBerry 10 speech synthesis (and Google Cloud/Gemini TTS) with automatic sentence tracking, page turning, and built-in Sleep Timer (15m, 30m, 45m, 60m, or at end of chapter).
  * **Footnote & Annotation Popup:** Instant footnote inspection in EPUB books via a floating card dialog without losing your reading position.
  * **Markdown Export:** Export book highlights, annotations, and bookmarks to structured `.md` files for Obsidian, Notion, or personal archives.
  * **Comic & Manga Reader (CBZ):** Optimized memory footprint via on-demand disk cache (eliminates Base64 RAM bloat) and Japanese Manga RTL (Right-to-Left) reading mode with inverted touch tap zones and swipe gestures.
  * **Two-Column Reading Mode:** Native CSS multi-column book layout optimized for wide screens like the BlackBerry Passport (1440x1440).
  * **Dictionary & Quick Translation:** Offline StarDict/BGL dictionary lookup and instant text translation.
  * **TOC, Bookmarks & Highlights:** Hierarchical table of contents, color-coded highlights, personal notes, and bookmarks.
  * **OPDS Catalog:** Browse and download books directly from online digital libraries and public OPDS feeds.
  * **Reading Statistics & Streaks:** Tracks daily and cumulative reading time, completed books, and reading habits with a streak calendar.

* **Hardware Keyboard Shortcuts (Passport, Classic, Q10):**
  * `Space`: Cuộn / lật tiếp trang (Page down).
  * `N` / `P`: Đến chương tiếp theo / chương trước (Next / Prev Chapter).
  * `J` / `K`: Cuộn xuống / cuộn lên theo dòng.
  * `T` / `B`: Về đầu sách / Đến cuối sách (Top / Bottom).
  * `M` / `H`: Đánh dấu trang (Bookmark toggle) / Mở danh sách trích dẫn.
  * `F` / `/`: Mở tìm kiếm từ khóa trong nội dung.
  * `D`: Chuyển đổi nhanh giao diện Theme (Đen AMOLED / Tối / Sepia / Sáng).
  * `A`: Bật / tắt đọc giọng nói (Toggle TTS Voice Reader).
  * `C`: Đổi chế độ chuyển trang (Paged lật trang vs Scroll cuộn liên tục).
  * `Z` / `+` / `=`: Phóng to cỡ chữ (Font size up).
  * `W` / `-` / `_`: Thu nhỏ cỡ chữ (Font size down).
  * `S`: Mở nhanh Cài đặt đọc sách.
  * `G`: Mở Mục lục sách (TOC).
  * `L`: Mở Từ điển & Tra cứu nhanh.
  * `I`: Xem chi tiết thông tin sách hiện tại.
  * `R` / `X`: Xoay trang PDF (Rotate 90°) / Chuyển chế độ PDF Reflow văn bản.
  * `1` - `9`: Nhảy nhanh đến 10% – 90% tiến độ chương hiện tại.
  * Hỗ trợ trượt cảm ứng trên bàn phím vật lý (Touch-enabled capacitive keyboard) của BlackBerry Passport.

---

## 2. Project Structure

```text
BB10-Book-reader/
├── bar-descriptor.xml         # BAR package manifest (metadata, permissions, MIME types, icons)
├── Libris.pro                 # QMake project file (cascades10, Qt4, Libris Core)
├── build.sh                   # Automated build script using containerized BBNDK
├── deploy.sh                  # One-step device deployment script
├── src/                       # Native C++ source code
│   ├── main.cpp               # Application entry point
│   ├── ApplicationUI.hpp/cpp  # QML scene initialization, Active Frame, event handling
│   ├── BookManager.hpp/cpp    # Library manager, progress tracking, stats, bookmarks
│   ├── BookWorkers.hpp/cpp    # Background worker threads for scanning and search
│   ├── EpubParser.hpp/cpp     # EPUB parser (ZIP, OPF, NCX, HTML)
│   ├── CbzParser.hpp/cpp      # Comic Book (CBZ) archive parser with ComicInfo metadata
│   ├── MobiParser.hpp/cpp     # Mobipocket / Kindle (MOBI) parser with PalmDOC/HUFF engine
│   ├── Fb2Parser.hpp/cpp      # FictionBook 2 (FB2) parser
│   ├── TxtParser.hpp/cpp      # Plain text (TXT) parser with chapter detection
│   ├── PdfParser.hpp/cpp      # PDF & DjVu parser and rendering controller
│   ├── TtsManager.hpp/cpp     # Native Text-to-Speech (TTS) manager
│   ├── DictManager.hpp/cpp    # Offline dictionary and translation manager
│   ├── OpdsManager.hpp/cpp    # OPDS catalog client and download manager
│   ├── AppTranslator.hpp/cpp  # Localization / multi-language engine
│   ├── pdfcore/               # Libris PDF / DJVU rendering backend engine
│   ├── miniz.c / miniz.h      # High-performance ZIP decompression library
│   └── pugixml.cpp / .hpp     # Lightweight XML/OPF parsing library
├── assets/                    # Cascades QML UI assets
│   ├── main.qml               # Root TabbedPane and drawer navigation
│   ├── RecentPage.qml         # Recent books & quick-resume dashboard
│   ├── LibraryPage.qml        # Bookshelf view (grid / list modes)
│   ├── BrowserPage.qml        # Hierarchical file browser for internal & SD card storage
│   ├── ReaderPage.qml         # Main reading surface (WebView + keyboard controls)
│   ├── TocPage.qml            # Table of contents navigation
│   ├── HighlightPage.qml      # Bookmarks, highlights, and annotations manager
│   ├── ReadingStatsSheet.qml  # Reading statistics and streak calendar
│   ├── OpdsSheet.qml          # Online OPDS catalog browser
│   ├── DictTranslateSheet.qml # Dictionary lookup and translation sheet
│   ├── Cover.qml              # Active Frame Cover UI
│   ├── settings/              # Settings pages (display, library, TTS, about)
│   └── icons/                 # Application icon assets
└── build/                     # Build artifacts and BAR packages
    ├── device/Libris-unsigned.bar
    └── simulator/Libris-simulator-unsigned.bar
```

---

## 3. Building from Source

The project builds via the `accupara/bbndk:latest` container (BBNDK 10.3.1.995 / tools 10.3.1.12) using `podman` or `docker`:

### Build for Physical Device (ARMv7 - Passport, Classic, Q10, Z30...):
```bash
./build.sh device
```
*Output artifact:* `build/device/Libris-unsigned.bar`

### Build for Simulator (x86):
```bash
./build.sh simulator
```
*Output artifact:* `build/simulator/Libris-simulator-unsigned.bar`

---

## 4. Installation on BlackBerry 10

### Step 1: Enable Development Mode on Device
1. Open **Settings** -> **Security and Privacy** -> **Development Mode**.
2. Switch Development Mode to **ON** and set a device password if prompted.
3. Note the device IP address (e.g., `192.168.1.15` on Wi-Fi or `169.254.0.1` via USB).

### Step 2: Deploy to Device

**Option 1: Using `deploy.sh` (Recommended)**
```bash
./deploy.sh <DEVICE_IP>
# Example via USB connection:
./deploy.sh 169.254.0.1
```
The script automatically builds the BAR package if needed, terminates older instances, installs, and launches the app.

**Option 2: Using Sachesi or DBBT (GUI)**
1. Connect your BlackBerry 10 device to your computer via USB or Wi-Fi.
2. Open Sachesi or DBBT and navigate to the **Install** tab.
3. Drag and drop `build/device/Libris-unsigned.bar` into the window to install.

**Option 3: Manual Command-Line Deployment (`blackberry-deploy`)**
```bash
podman run --rm --net=host -v "$PWD/build/device:/output:ro,z" docker.io/accupara/bbndk:latest \
    blackberry-deploy -installApp -launchApp -device <DEVICE_IP> -password <DEVICE_PASSWORD> /output/Libris-unsigned.bar
```
