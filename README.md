# Berry Leaf - Native E-Book Reader for BlackBerry 10

Berry Leaf is a high-performance native e-book reader built for **BlackBerry 10 Cascades (C++ / Qt 4.8 / QML)**. It delivers a fluid 60fps reading experience, native Cascades gestures, real-time Active Frame cover display, and hardware keyboard shortcuts tailored for devices such as the BlackBerry Passport, Classic, Q10, Q5, Z10, and Z30.

---

## 1. Key Features

* **Multi-Format Document Support:**
  * Reads **EPUB**, **FB2**, **TXT**, **PDF**, and **DJVU** documents seamlessly (integrated NSRReaderCore engine).
  * Self-contained, lightweight architecture powered by `miniz` (in-memory ZIP extraction) and `pugixml` (fast XML parsing).

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
  * **Text-to-Speech (TTS):** Native BlackBerry 10 speech synthesis with automatic page-turning.
  * **Dictionary & Quick Translation:** Offline StarDict/BGL dictionary lookup and instant text translation.
  * **TOC, Bookmarks & Highlights:** Hierarchical table of contents, color-coded highlights, personal notes, and bookmarks.
  * **OPDS Catalog:** Browse and download books directly from online digital libraries and public OPDS feeds.
  * **Reading Statistics & Streaks:** Tracks daily and cumulative reading time, completed books, and reading habits with a streak calendar.

* **Hardware Keyboard Shortcuts (Passport, Classic, Q10):**
  * `Space`: Page down / advance reading.
  * `N` / `P`: Next / previous chapter.
  * `T` / `B`: Jump to Top / Bottom.
  * Touch-enabled physical keyboard scrolling on BlackBerry Passport.

---

## 2. Project Structure

```text
BB10-Book-reader/
├── bar-descriptor.xml         # BAR package manifest (metadata, permissions, MIME types, icons)
├── BerryLeaf.pro              # QMake project file (cascades10, Qt4, nsrreadercore)
├── build.sh                   # Automated build script using containerized BBNDK
├── deploy.sh                  # One-step device deployment script
├── src/                       # Native C++ source code
│   ├── main.cpp               # Application entry point
│   ├── ApplicationUI.hpp/cpp  # QML scene initialization, Active Frame, event handling
│   ├── BookManager.hpp/cpp    # Library manager, progress tracking, stats, bookmarks
│   ├── BookWorkers.hpp/cpp    # Background worker threads for scanning and search
│   ├── EpubParser.hpp/cpp     # EPUB parser (ZIP, OPF, NCX, HTML)
│   ├── Fb2Parser.hpp/cpp      # FictionBook 2 (FB2) parser
│   ├── TxtParser.hpp/cpp      # Plain text (TXT) parser with chapter detection
│   ├── TtsManager.hpp/cpp     # Native Text-to-Speech (TTS) manager
│   ├── DictManager.hpp/cpp    # Offline dictionary and translation manager
│   ├── OpdsManager.hpp/cpp    # OPDS catalog client and download manager
│   ├── AppTranslator.hpp/cpp  # Localization / multi-language engine
│   ├── nsrreadercore/         # PDF / DJVU rendering library headers
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
    ├── device/BerryLeaf-unsigned.bar
    └── simulator/BerryLeaf-simulator-unsigned.bar
```

---

## 3. Building from Source

The project builds via the `accupara/bbndk:latest` container (BBNDK 10.3.1.995 / tools 10.3.1.12) using `podman` or `docker`:

### Build for Physical Device (ARMv7 - Passport, Classic, Q10, Z30...):
```bash
./build.sh device
```
*Output artifact:* `build/device/BerryLeaf-unsigned.bar`

### Build for Simulator (x86):
```bash
./build.sh simulator
```
*Output artifact:* `build/simulator/BerryLeaf-simulator-unsigned.bar`

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
3. Drag and drop `build/device/BerryLeaf-unsigned.bar` into the window to install.

**Option 3: Manual Command-Line Deployment (`blackberry-deploy`)**
```bash
podman run --rm --net=host -v "$PWD/build/device:/output:ro,z" docker.io/accupara/bbndk:latest \
    blackberry-deploy -installApp -launchApp -device <DEVICE_IP> -password <DEVICE_PASSWORD> /output/BerryLeaf-unsigned.bar
```
