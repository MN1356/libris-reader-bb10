# Libris — Native E-Book Reader for BlackBerry 10

Libris is a native C++ / Qt 4.8 / Cascades e-book reader built for **BlackBerry 10** devices (Passport, Classic, Q10, Q5, Z10, Z30). It combines hardware-accelerated rendering, an integrated PDF/DjVu core, offline & online Text-to-Speech, wireless book transfer, and full physical keyboard navigation.

* **Repository:** https://github.com/MN1356/libris-reader-bb10
* **Current Version:** `2.3.1` (`Build 275`)
* **License:** GNU General Public License v2.0 or later ([LICENSE](LICENSE))

---

## 1. Key Features

### Multi-Format Reading Engine
* **Reflowable Books:** **EPUB**, **MOBI / AZW / AZW3 / KF8 / PRC** (PalmDOC & HUFF/CDIC), **FB2**, **TXT**, **Markdown** (`.md`, `.markdown`), and **HTML** (`.html`, `.htm`).
* **Fixed-Layout Documents & Comics:** **PDF** and **DJVU** via `libnsrreadercore.so` (`NSRPopplerDocument` / `NSRDjVuDocument`), plus **CBZ** comic archives with on-demand disk image caching and Manga Right-to-Left (RTL) reading mode.
* **Vietnamese Legacy Encoding Support:** Automatic detection for **UTF-8**, **UTF-16**, **TCVN3**, **Windows-1258**, and **VNI** text files.

### Embedded-OCR PDF Reading, Selection & TTS
* **Continuous Stream, Zoom & Reflow:** Scroll smoothly across rendered pages, pinch-to-zoom, crop white margins (`isPdfAutoCrop`), rotate 90°, invert colors for night reading, or switch to **OCR Text Reflow** mode.
* **OCR Text Selection & Study Actions:** Long-press any PDF page (`ActionSet`) or tap **Chọn chữ (OCR)** to open [PdfOcrTextSheet.qml](assets/PdfOcrTextSheet.qml) — select text with native BB10 WebKit selection handles to **Copy**, **Look Up**, **Translate**, **Highlight**, or **Read aloud (TTS)**.
* **Live OCR TTS Tracking:** Speaks embedded OCR text directly in both visual PDF mode and Reflow mode, displaying full spoken sentences (`BodyText`), page/sentence progress (`Trang X/Y • Câu i/n`), and live sentence highlighting inside the OCR viewer.

### Library Management, Transfer & Study Tools
* **Fast Non-Blocking Scanner & Custom Scan Folders:** Rapid filesystem discovery across internal storage and external microSD cards (`sdcard` / `sdcard2`) with asynchronous background cover extraction (`CoverExtractWorker`), plus custom scan-folder selection in **Settings -> Tủ sách & Quản lý -> Chọn thư mục quét...** ([ScanFoldersSheet.qml](assets/settings/ScanFoldersSheet.qml)).
* **In-App File Management:** Custom shelves, grouping (author/series), favorites, reading list, metadata editor, and safe in-app file deletion with confirmation dialogs across Library, File Browser, and Book Detail views.
* **Wi-Fi Book Drop & Web Clipper:** Built-in LAN web portal (`WiFiDropServer`) secured by a per-session **4-digit access PIN** lock gate (`#pinGate`), supporting drag-and-drop book uploads, downloading books/notes/backups, and **Web Clipper** (saved as HTML under `/accounts/1000/shared/books/Clip_<timestamp>.html` and auto-indexed).
* **Backup & Persistence:** Full `.librisbackup` archive export/import, automatic persistent state sync, and Markdown highlight/bookmark exports.
* **TTS Hybrid Engine:** Offline **eSpeak-NG** synthesis alongside **Google Translate TTS** and **Gemini TTS**, complete with Sleep Timer and sentence tracking.
* **Dictionary & OPDS:** Offline dictionary lookup, online translation, and OPDS digital library catalog browsing with verified TLS (`assets/certs/cacert.pem`).
* **Cascades UI & BB10 Hardware Support:** AMOLED Pure Black (`#000000`), Dark, Sepia, and Light themes; Two-Column mode for wide screens (Passport `1440x1440`); Bionic Reading; Focus Mode; real-time **Active Frame** (`Cover.qml`); Streak & Reading Stats; and **Passport capacitive touch keyboard** scrolling.

---

## 2. Hardware Keyboard Shortcuts (Passport, Classic, Q10)

* `Space`: Next page / scroll down
* `N` / `P`: Next / Previous chapter (or PDF page)
* `J` / `K`: Scroll down / up line by line
* `T` / `B`: Jump to Top / Bottom of chapter
* `M` / `H`: Toggle Bookmark / Open Highlights list
* `F` / `/`: Search text inside current book
* `D`: Cycle theme (AMOLED / Dark / Sepia / Light)
* `A`: Toggle Text-to-Speech (TTS)
* `C`: Toggle Paged vs. Continuous Scroll mode
* `Z` / `+` / `=`: Increase font size
* `W` / `-` / `_`: Decrease font size
* `S`: Open Reader Settings
* `G`: Open Table of Contents (TOC)
* `L`: Open Dictionary & Translation
* `I`: Open current Book Info
* `R` / `X`: Rotate PDF 90° / Toggle PDF OCR Reflow
* `1` – `9`: Jump to 10% – 90% of current chapter

---

## 3. Project Structure

```text
BB10 Book reader/
├── bar-descriptor.xml                 # BlackBerry 10 BAR manifest (app ID, permissions, invoke filters)
├── Libris.pro                         # QMake configuration (Cascades 10, Qt 4.8, eSpeak-NG, nsrreadercore)
├── build.sh                           # Containerized BBNDK build & BAR packager
├── deploy.sh                          # USB / Wi-Fi deploy & launch helper via blackberry-deploy
├── LICENSE                            # GPL v2 license and third-party notices
├── src/                               # Native C++ backend
│   ├── main.cpp                       # Entry point, logging, UTF-8 codec & TLS CA setup
│   ├── ApplicationUI.hpp/cpp          # Cascades lifecycle, Active Frame, keyboard/trackpad handlers
│   ├── BookManager.hpp/cpp            # Library, shelves, file deletion, scan folders & reader state
│   ├── BookWorkers.hpp/cpp            # ScanWorker, CoverExtractWorker, SearchWorker threads
│   ├── EpubParser.hpp/cpp             # EPUB parser (OPF, NCX, spine, covers)
│   ├── MobiParser.hpp/cpp             # MOBI / AZW3 / KF8 / PRC parser (PalmDOC / HUFF)
│   ├── Fb2Parser.hpp/cpp              # FictionBook 2 parser
│   ├── CbzParser.hpp/cpp              # CBZ comic archive parser
│   ├── TxtParser.hpp/cpp              # TXT / Markdown / HTML parser + Vietnamese legacy encodings
│   ├── PdfParser.hpp/cpp              # PDF & DjVu controller, streaming worker & OCR text extraction
│   ├── TtsManager.hpp/cpp             # Hybrid TTS orchestrator (eSpeak-NG, Google, Gemini)
│   ├── EspeakEngine.hpp/cpp           # Native C++ wrapper around vendored eSpeak-NG
│   ├── DictManager.hpp/cpp            # Dictionary lookup & online translation
│   ├── OpdsManager.hpp/cpp            # OPDS feed client & downloader
│   ├── WiFiDropServer.hpp/cpp         # HTTP server with PIN auth gate, upload & Web Clipper
│   ├── AppTranslator.hpp/cpp          # Dynamic Vietnamese / English localization
│   ├── pdfcore/                       # Interfaces & simulator stub for NSR Reader Core
│   ├── espeak-ng/                     # Vendored eSpeak-NG C synthesizer + QNX compatibility shims
│   ├── miniz.c / miniz.h              # Embedded ZIP / inflate library
│   └── pugixml.cpp / .hpp             # Embedded XML DOM parser
├── assets/                            # Cascades QML UI, bundled runtime data & certs
│   ├── main.qml                       # Root TabbedPane & navigation drawer
│   ├── RecentPage.qml                 # Dashboard & reading history
│   ├── LibraryPage.qml                # Bookshelf (grid / list, custom shelves)
│   ├── BrowserPage.qml                # Storage & SD card file browser
│   ├── ReaderPage.qml                 # Reader viewport, PDF stream/zoom, TTS panel & shortcuts
│   ├── PdfOcrTextSheet.qml            # Selectable PDF OCR text sheet with live TTS highlight
│   ├── WiFiDropSheet.qml              # Wireless Drop server UI & access PIN display
│   ├── BookDetailSheet.qml            # Metadata editor, reviews & file deletion
│   ├── DictTranslateSheet.qml         # Dictionary & translation modal
│   ├── OpdsSheet.qml                  # OPDS catalog browser
│   ├── ReadingStatsSheet.qml          # Reading analytics & streak calendar
│   ├── Cover.qml                      # Active Frame cover widget
│   ├── settings/                      # Settings pages & ScanFoldersSheet.qml
│   ├── lib/libnsrreadercore.so        # Precompiled ARMv7 PDF/DjVu core shared library
│   ├── espeak-ng-data/                # Bundled eSpeak-NG language & voice dictionaries
│   └── certs/cacert.pem               # Root CA bundle for TLS verification
└── build/                             # Generated binaries & .bar packages
    ├── device/Libris-unsigned.bar
    └── simulator/Libris-simulator-unsigned.bar
```

---

## 4. Building & Deploying

The project cross-compiles inside the containerized BlackBerry 10 NDK (`docker.io/accupara/bbndk:latest` — BBNDK `10.3.1.995`) via `podman` or `docker`.

### Build
```bash
# Physical BB10 device (ARMv7)
./build.sh device

# BB10 Simulator (x86)
./build.sh simulator
```
Generated packages:
* `build/device/Libris-unsigned.bar`
* `build/simulator/Libris-simulator-unsigned.bar`

### Deploy to BlackBerry 10
1. On your device, enable **Settings -> Security and Privacy -> Development Mode** and note the password and IP (`169.254.0.1` over USB, or your LAN Wi-Fi IP).
2. Deploy and launch in one command:
   ```bash
   ./deploy.sh 169.254.0.1
   ```
   Alternatively, sideload `build/device/Libris-unsigned.bar` with Sachesi, DBBT, or `blackberry-deploy`.

---

## 5. Credits & Acknowledgments

Libris is made possible by the contributions of its author and the following open-source projects and communities:

* **Author & Maintainer:**
  * **NhatLee (`MN1356`)** — Application architecture, Cascades UI/UX, multi-format library engine, Wi-Fi Book Drop, OCR reader workflows, TTS integration, and BlackBerry 10 hardware optimizations ([GitHub](https://github.com/MN1356/libris-reader-bb10)).

* **Open-Source Libraries & Engines:**
  * **NSR Reader Core (`src/pdfcore/`, `assets/lib/libnsrreadercore.so`):** Native BlackBerry 10 PDF and DjVu rendering backend (`NSRReaderCore`, `NSRPopplerDocument`, `NSRDjVuDocument`), built atop **Poppler** and **DjVuLibre** (**GPL**).
  * **eSpeak-NG (`src/espeak-ng/`, `assets/espeak-ng-data/`):** Open-source compact multilingual software speech synthesizer originally created by **Jonathan Duddington** and maintained by the **eSpeak NG** contributors (**GPL-3.0+**).
  * **pugixml v1.16 (`src/pugixml.hpp`, `src/pugixml.cpp`, `src/pugiconfig.hpp`):** Light-weight, simple, and fast XML parser by **Arseny Kapoulkine** ([pugixml.org](https://pugixml.org/), **MIT License**).
  * **miniz (`src/miniz.c`, `src/miniz.h`):** Single-file, public-domain/MIT-style Zlib-compatible compression library by **Rich Geldreich**, **Tenacious Software LLC**, **RAD Game Tools**, and **Valve Software** (**MIT License**).
  * **Mozilla / curl CA Certificate Bundle (`assets/certs/cacert.pem`):** Trusted root certificates for TLS verification (**MPL-2.0**).
  * **BlackBerry 10 Cascades SDK & `accupara/bbndk`:** Qt 4.8 / Cascades 10 APIs and containerized BB10 NDK `10.3.1.995` build toolchain.
