import bb.cascades 1.3
import bb.cascades.pickers 1.0
import bb.system 1.2

TabbedPane {
    id: rootTabbedPane

    // =========================================================================
    // NATIVE CASCADES LEFT-SWIPE DRAWER SIDEBAR (5 Fixed Tabs for Libris)
    // =========================================================================
    showTabsOnActionBar: false
    peekEnabled: !bookManager.isReaderActive
    activeTab: recentTab

    Menu.definition: MenuDefinition {
        id: rootAppMenu
        helpAction: HelpActionItem {
            title: qsTr("Giới Thiệu") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_info.png"
            onTriggered: {
                rootTabbedPane.showAbout();
            }
        }
        settingsAction: SettingsActionItem {
            title: qsTr("Cài Đặt") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_settings.png"
            onTriggered: {
                rootTabbedPane.openSettings();
            }
        }
        actions: [
            ActionItem {
                title: qsTr("Thoát") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_exit.png"
                onTriggered: {
                    rootTabbedPane.promptExit();
                }
            }
        ]
    }

    property alias settingsSheet: settingsSheet
    property alias aboutSheet: aboutSheet
    property alias readingStatsSheet: readingStatsSheet
    property alias opdsSheet: opdsSheet
    property alias wifiDropSheet: wifiDropSheet
    property alias filePicker: filePicker
    property alias navPane: navPane
    property alias recentTab: recentTab
    property alias shelfTab: shelfTab
    property alias highlightTab: highlightTab
    property alias browserTab: browserTab
    property alias importTab: importTab

    property string pendingExternalPath: ""

    function openSettings() {
        settingsSheet.open();
    }

    function showAbout() {
        aboutSheet.open();
    }

    function openReadingStats() {
        readingStatsSheet.open();
    }

    function openFilePicker() {
        filePicker.open();
    }

    function openWifiDrop() {
        wifiDropSheet.open();
    }

    function openOpds() {
        opdsSheet.open();
    }

    function promptExit() {
        exitConfirmDialog.show();
    }

    function switchToTab(tabName) {
        if (tabName === "stats" || tabName === "progress") {
            rootTabbedPane.openReadingStats();
        } else if (tabName === "reading" || tabName === "recent" || tabName === "home") {
            rootTabbedPane.activeTab = recentTab;
        } else if (tabName === "highlight" || tabName === "highlights" || tabName === "bookmark" || tabName === "bookmarks") {
            rootTabbedPane.activeTab = highlightTab;
        } else if (tabName === "shelf" || tabName === "library") {
            bookManager.setMainViewMode("shelf");
            rootTabbedPane.activeTab = shelfTab;
        } else if (tabName === "browser") {
            rootTabbedPane.activeTab = browserTab;
        } else if (tabName === "import" || tabName === "wifi" || tabName === "opds") {
            rootTabbedPane.activeTab = importTab;
        }
        rootTabbedPane.resetSidebarState();
    }

    function openBookWithLoading(filePath) {
        console.log("[main.qml] openBookWithLoading called with:", filePath);
        if (!filePath) return;

        rootTabbedPane.activeTab = recentTab;
        rootTabbedPane.resetSidebarState();

        if (navPane.count() > 1) {
            if (bookManager.currentFilePath === filePath) {
                console.log("[main.qml] Book is already currently open:", filePath);
                return;
            }
            pendingExternalPath = filePath;
            navPane.pop();
            return;
        }

        bookManager.setIsLoadingBook(true);
        bookManager.startWatchdog(2500);

        if (bookManager.openBook(filePath)) {
            var reader = readerDefinition.createObject();
            if (reader) {
                reader.navPane = navPane;
                navPane.push(reader);
                if (typeof reader.initFocus === "function") {
                    reader.initFocus();
                }
                bookManager.setIsLoadingBook(false);
                bookManager.setIsReaderActive(true);
            } else {
                var errDetail = "";
                if (typeof readerDefinition.errorMessage === "function") {
                    errDetail = readerDefinition.errorMessage();
                } else if (typeof readerDefinition.errorString === "function") {
                    errDetail = readerDefinition.errorString();
                }
                console.log("[ERROR] Failed to instantiate readerDefinition! Error:", errDetail);
                bookManager.setIsLoadingBook(false);
                errorToast.body = (errDetail && errDetail.length > 0) ?
                    (qsTr("Không thể khởi tạo giao diện đọc sách:\n") + errDetail + Retranslate.onLanguageChanged) :
                    (qsTr("Không thể khởi tạo giao diện đọc sách") + Retranslate.onLanguageChanged);
                errorToast.show();
            }
        } else {
            console.log("[ERROR] bookManager.openBook returned false for:", filePath);
            bookManager.setIsLoadingBook(false);
            errorToast.body = qsTr("Không thể mở sách. File có thể bị hỏng hoặc không được hỗ trợ.") + Retranslate.onLanguageChanged;
            errorToast.show();
        }
    }

    onCreationCompleted: {
        console.log("[main.qml] onCreationCompleted: connecting app signals");
        app.openBookRequested.connect(rootTabbedPane.openBookWithLoading);
        app.switchToTabRequested.connect(rootTabbedPane.switchToTab);
        app.showAboutRequested.connect(rootTabbedPane.showAbout);
        app.openSettingsRequested.connect(rootTabbedPane.openSettings);
        app.openReadingStatsRequested.connect(rootTabbedPane.openReadingStats);
        app.openFilePickerRequested.connect(rootTabbedPane.openFilePicker);
        app.openWifiDropRequested.connect(rootTabbedPane.openWifiDrop);
        app.openOpdsRequested.connect(rootTabbedPane.openOpds);
        app.promptExitRequested.connect(rootTabbedPane.promptExit);
        bookManager.promptExitRequested.connect(rootTabbedPane.promptExit);
    }

    function openBookExternal(filePath) {
        console.log("[main.qml] openBookExternal called with:", filePath);
        openBookWithLoading(filePath);
    }

    onActiveTabChanged: {
        console.log("[main.qml] Active tab changed:", activeTab.title);
        rootTabbedPane.resetSidebarState();
        if (activeTab === recentTab) {
            bookManager.refreshStats();
            var isReading = (navPane && navPane.count() > 1 && navPane.top !== recentPage);
            bookManager.setIsReaderActive(isReading);
        } else {
            bookManager.setIsReaderActive(false);
            if (activeTab === shelfTab) {
                bookManager.setMainViewMode("shelf");
            } else if (activeTab === highlightTab) {
                if (highlightPage && highlightPage.reloadHighlights) {
                    highlightPage.reloadHighlights();
                }
            }
        }
    }

    // =========================================================================
    // TAB 1: ĐANG ĐỌC (HOME) - Editorial Dashboard & Continue Reading
    // =========================================================================
    Tab {
        id: recentTab
        title: qsTr("Trang chủ") + Retranslate.onLanguageChanged
        imageSource: "asset:///icons/ic_recent.png"

        NavigationPane {
            id: navPane
            backButtonsVisible: false
            peekEnabled: false

            function openSettings() { rootTabbedPane.openSettings(); }
            function showAbout() { rootTabbedPane.showAbout(); }
            function openFilePicker() { rootTabbedPane.openFilePicker(); }

            RecentPage {
                id: recentPage
            }

            onTopChanged: {
                bookManager.setIsLoadingBook(false);
                var isReaderOnTop = (navPane.count() > 1 && navPane.top !== recentPage && rootTabbedPane.activeTab === recentTab);
                bookManager.setIsReaderActive(isReaderOnTop);
            }

            onPushTransitionEnded: {
                if (typeof page.initFocus === "function") {
                    page.initFocus();
                }
            }

            onPopTransitionEnded: {
                if (typeof page.cleanup === "function") {
                    page.cleanup();
                }
                bookManager.saveCurrentProgress();
                bookManager.setIsReaderActive(false);
                bookManager.stopWatchdog();
                bookManager.setIsLoadingBook(false);
                page.destroy();
                bookManager.refreshStats();
                if (pendingExternalPath !== "") {
                    var nextPath = pendingExternalPath;
                    pendingExternalPath = "";
                    rootTabbedPane.openBookWithLoading(nextPath);
                }
            }
        }
    }

    // =========================================================================
    // TAB 2: THƯ VIỆN (LIBRARY)
    // =========================================================================
    Tab {
        id: shelfTab
        title: qsTr("Tủ Sách") + Retranslate.onLanguageChanged
        imageSource: "asset:///icons/ic_library.png"

        LibraryPage {
            id: libraryPage
        }
    }

    // =========================================================================
    // TAB 3: GHI CHÚ & DẤU TRANG (NOTES & BOOKMARKS)
    // =========================================================================
    Tab {
        id: highlightTab
        title: qsTr("Ghi chú") + Retranslate.onLanguageChanged
        imageSource: "asset:///icons/ic_bookmarks.png"

        HighlightPage {
            id: highlightPage
        }
    }

    // =========================================================================
    // TAB 4: THƯ MỤC (FILES)
    // =========================================================================
    Tab {
        id: browserTab
        title: qsTr("Thư mục") + Retranslate.onLanguageChanged
        imageSource: "asset:///icons/ic_folder.png"

        BrowserPage {
            id: browserPage
        }
    }

    // =========================================================================
    // TAB 5: NHẬP SÁCH (IMPORT HUB: Wi-Fi Drop, OPDS, FilePicker, Scan)
    // =========================================================================
    Tab {
        id: importTab
        title: qsTr("Nhập sách") + Retranslate.onLanguageChanged
        imageSource: "asset:///icons/ic_import.png"

        ImportPage {
            id: importPage
        }
    }

    // =========================================================================
    // ATTACHED OBJECTS & DIALOGS
    // =========================================================================
    attachedObjects: [
        SystemToast {
            id: blockedToast
            position: SystemUiPosition.MiddleCenter
        },
        SystemToast {
            id: errorToast
            position: SystemUiPosition.MiddleCenter
        },
        SystemDialog {
            id: exitConfirmDialog
            title: qsTr("Thoát Libris") + Retranslate.onLanguageChanged
            body: qsTr("Bạn có muốn lưu tiến độ đọc và thoát hoàn toàn ứng dụng không?") + Retranslate.onLanguageChanged
            confirmButton.label: qsTr("Thoát") + Retranslate.onLanguageChanged
            cancelButton.label: qsTr("Hủy") + Retranslate.onLanguageChanged
            onFinished: {
                if (value === SystemUiResult.ConfirmButtonSelection) {
                    app.exitApp();
                }
            }
        },
        FilePicker {
            id: filePicker
            type: FileType.Other
            title: qsTr("Chọn sách để mở") + Retranslate.onLanguageChanged
            filter: ["*.epub", "*.txt", "*.fb2", "*.pdf", "*.djvu", "*.djv", "*.cbz", "*.mobi", "*.azw", "*.azw3", "*.kf8", "*.prc"]
            directories: [
                "/accounts/1000/removable/sdcard2",
                "/accounts/1000/removable/sdcard2/Book",
                "/accounts/1000/removable/sdcard2/books",
                "/accounts/1000/removable/sdcard2/documents",
                "/accounts/1000/removable/sdcard2/download",
                "/accounts/1000/removable/sdcard",
                "/accounts/1000/removable/sdcard/Book",
                "/accounts/1000/shared/documents",
                "/accounts/1000/shared/books",
                "/accounts/1000/shared/Book",
                "/accounts/1000/shared/downloads"
            ]
            onFileSelected: {
                if (selectedFiles.length > 0) {
                    rootTabbedPane.openBookWithLoading(selectedFiles[0]);
                }
            }
        },
        SettingsSheet {
            id: settingsSheet
        },
        AboutSheet {
            id: aboutSheet
        },
        ReadingStatsSheet {
            id: readingStatsSheet
        },
        OpdsSheet {
            id: opdsSheet
        },
        WiFiDropSheet {
            id: wifiDropSheet
        },
        ComponentDefinition {
            id: readerDefinition
            source: "asset:///ReaderPage.qml"
        }
    ]
}
