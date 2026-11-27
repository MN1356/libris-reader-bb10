import bb.cascades 1.3
import bb.system 1.2

Page {
    id: browserPage

    actionBarVisibility: ChromeVisibility.Visible

    property bool isSearchAndFilterVisible: false
    property string pendingDeletePath: ""
    property string pendingDeleteTitle: ""

    onIsSearchAndFilterVisibleChanged: {
        if (isSearchAndFilterVisible) {
            browserSearchInputField.requestFocus();
        }
    }

    function showToast(msg) {
        if (!msg) return;
        browserToast.body = msg;
        browserToast.show();
    }

    function showBookDetail(item) {
        if (!item) return;
        bookDetailSheet.openWithBook(item);
    }

    function confirmDeleteFile(filePath, title) {
        if (!filePath) return;
        pendingDeletePath = filePath;
        pendingDeleteTitle = title ? title : filePath;
        browserDeleteDialog.body = qsTr("Tệp \"%1\" sẽ bị xóa vĩnh viễn khỏi bộ nhớ máy. Bạn có chắc chắn?").replace("%1", pendingDeleteTitle) + Retranslate.onLanguageChanged;
        browserDeleteDialog.show();
    }

    function formatDisplayPath(raw) {
        if (!raw || raw === "root") return qsTr("Bộ nhớ thiết bị");
        if (raw.indexOf("/accounts/1000/removable/sdcard") === 0) {
            var rest = raw.replace(new RegExp("^/accounts/1000/removable/sdcard[0-9]*/?"), "");
            return rest.length > 0 ? (qsTr("Thẻ SD") + " / " + rest) : qsTr("Thẻ SD");
        }
        if (raw.indexOf("/accounts/1000/shared") === 0) {
            var srest = raw.replace(new RegExp("^/accounts/1000/shared/?"), "");
            return srest.length > 0 ? (qsTr("Bộ nhớ trong") + " / " + srest) : qsTr("Bộ nhớ trong");
        }
        return raw;
    }

    titleBar: TitleBar {
        title: (bookManager.canGoUp ? qsTr(bookManager.currentBrowserTitle) : qsTr("Duyệt Thư Mục")) + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
        dismissAction: bookManager.canGoUp ? upAction : null
        acceptAction: ActionItem {
            title: (browserPage.isSearchAndFilterVisible ? qsTr("Đóng") : qsTr("Tìm")) + Retranslate.onLanguageChanged
            onTriggered: {
                browserPage.isSearchAndFilterVisible = !browserPage.isSearchAndFilterVisible;
                if (!browserPage.isSearchAndFilterVisible) {
                    bookManager.clearBrowserFilter();
                    browserSearchInputField.text = "";
                }
            }
        }
    }

    Menu.definition: MenuDefinition {
        helpAction: HelpActionItem {
            title: qsTr("Giới Thiệu") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_info.png"
            onTriggered: {
                app.showAbout();
            }
        }
        settingsAction: SettingsActionItem {
            title: qsTr("Cài Đặt") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_settings.png"
            onTriggered: {
                app.openSettings();
            }
        }
        actions: [
            ActionItem {
                title: qsTr("Trang chủ") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_home.png"
                onTriggered: {
                    app.switchToTab("home");
                }
            },
            ActionItem {
                title: qsTr("Thoát") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_exit.png"
                onTriggered: {
                    app.promptExit();
                }
            }
        ]
    }

    actions: [
        ActionItem {
            title: bookManager.libraryViewMode === "mosaic" ?
                   (qsTr("Danh sách") + Retranslate.onLanguageChanged) :
                   (qsTr("Dạng lưới") + Retranslate.onLanguageChanged)
            imageSource: bookManager.libraryViewMode === "mosaic" ? "asset:///icons/ic_list.png" : "asset:///icons/ic_grid.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            shortcuts: [
                Shortcut { key: "V" }
            ]
            onTriggered: {
                bookManager.toggleLibraryViewMode();
            }
        },
        ActionItem {
            title: qsTr("Làm Mới") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_refresh.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            shortcuts: [
                Shortcut { key: "R" }
            ]
            onTriggered: {
                bookManager.refreshBrowser();
            }
        },
        ActionItem {
            title: qsTr("Mở Tệp") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_open_file.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            shortcuts: [
                Shortcut { key: "O" }
            ]
            onTriggered: {
                app.openFilePicker();
            }
        },
        ActionItem {
            title: qsTr("Tủ Sách") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_library.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            onTriggered: {
                app.switchToTab("shelf");
            }
        },
        ActionItem {
            title: qsTr("Quét lại") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_refresh.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            onTriggered: {
                bookManager.scanLibrary();
                browserPage.showToast(qsTr("Đang nạp và quét thư viện sách...") + Retranslate.onLanguageChanged);
            }
        }
    ]

    Container {
        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: Color.create(bookManager.uiBg)

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: 1
            background: Color.create(bookManager.uiHairline)
        }

        // 1. COLLAPSIBLE LIVE SEARCH BAR
        Container {
            id: searchAndFilterContainer
            visible: browserPage.isSearchAndFilterVisible
            horizontalAlignment: HorizontalAlignment.Fill
            background: Color.create(bookManager.uiCard)
            topPadding: bookManager.dp(10)
            bottomPadding: bookManager.dp(10)
            leftPadding: bookManager.dp(14)
            rightPadding: bookManager.dp(14)
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            ImageView {
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: bookManager.dp(24)
                preferredHeight: bookManager.dp(24)
                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_search.png" : "asset:///icons/ic_search_dark.png"
            }

            TextField {
                id: browserSearchInputField
                leftMargin: bookManager.dp(10)
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                hintText: qsTr("Tìm theo tên file, tên sách, tác giả...") + Retranslate.onLanguageChanged
                input.submitKey: SubmitKey.Search
                clearButtonVisible: true
                text: bookManager.browserSearchQuery
                onTextChanging: {
                    bookManager.setBrowserSearchQuery(text);
                }
                input.onSubmitted: {
                    browserSearchInputField.loseFocus();
                    bookManager.setBrowserSearchQuery(text);
                }
            }
        }

        Container {
            visible: browserPage.isSearchAndFilterVisible
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: 1
            background: Color.create(bookManager.uiHairline)
        }

        // 2. QUICK STORAGE SWITCHER + READABLE BREADCRUMB
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            background: Color.create(bookManager.uiCard)
            topPadding: bookManager.dp(10)
            bottomPadding: bookManager.dp(10)
            leftPadding: bookManager.dp(14)
            rightPadding: bookManager.dp(14)
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            SegmentedControl {
                id: quickStorageControl
                property bool syncingStorage: false
                horizontalAlignment: HorizontalAlignment.Fill

                function syncFromPath() {
                    syncingStorage = true;
                    var p = bookManager.currentBrowserPath;
                    if (p === "root" || p === "") selectedIndex = 0;
                    else if (p.indexOf("removable") >= 0 || p.indexOf("sdcard") >= 0) selectedIndex = 2;
                    else selectedIndex = 1;
                    syncingStorage = false;
                }

                onCreationCompleted: {
                    syncFromPath();
                    bookManager.currentBrowserPathChanged.connect(syncFromPath);
                }

                Option { text: qsTr("Tất cả") + Retranslate.onLanguageChanged; value: "root"; selected: true }
                Option { text: qsTr("Trong máy") + Retranslate.onLanguageChanged; value: "__INTERNAL__" }
                Option { text: qsTr("Thẻ SD") + Retranslate.onLanguageChanged; value: "__SDCARD__" }

                onSelectedValueChanged: {
                    if (!syncingStorage && selectedValue) {
                        bookManager.openBrowserFolder(selectedValue);
                    }
                }
            }

            Container {
                topMargin: bookManager.dp(8)
                horizontalAlignment: HorizontalAlignment.Fill
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                ImageView {
                    verticalAlignment: VerticalAlignment.Center
                    preferredWidth: bookManager.dp(20)
                    preferredHeight: bookManager.dp(20)
                    rightMargin: bookManager.dp(8)
                    opacity: 0.75
                    imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_folder.png" : "asset:///icons/ic_folder_dark.png"
                }

                Label {
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    text: browserPage.formatDisplayPath(bookManager.currentBrowserPath) + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: Color.create(bookManager.uiMuted)
                    multiline: true
                    autoSize.maxLineCount: 2
                }

                Label {
                    leftMargin: bookManager.dp(10)
                    verticalAlignment: VerticalAlignment.Center
                    text: bookManager.browserItemCount + " " + (qsTr("mục") + Retranslate.onLanguageChanged)
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create(bookManager.uiAccent)
                }
            }
        }

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: 1
            background: Color.create(bookManager.uiHairline)
        }

        // 3. BROWSER CONTENT AREA (Empty / Detailed / Mosaic)
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
            layout: DockLayout {}

            // Empty State
            Container {
                visible: bookManager.browserItemCount === 0
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                leftPadding: bookManager.dp(28)
                rightPadding: bookManager.dp(28)
                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                ImageView {
                    horizontalAlignment: HorizontalAlignment.Center
                    preferredWidth: bookManager.dp(72)
                    preferredHeight: bookManager.dp(72)
                    imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_folder.png" : "asset:///icons/ic_folder_dark.png"
                    opacity: 0.45
                    scalingMethod: ScalingMethod.AspectFit
                }

                Label {
                    topMargin: bookManager.dp(14)
                    horizontalAlignment: HorizontalAlignment.Center
                    text: (bookManager.browserSearchQuery !== "" || bookManager.browserFormatFilter !== "all") ?
                          (qsTr("Không tìm thấy tệp hoặc thư mục phù hợp") + Retranslate.onLanguageChanged) :
                          (qsTr("Thư mục trống hoặc không có sách phù hợp") + Retranslate.onLanguageChanged)
                    textStyle.base: SystemDefaults.TextStyles.TitleText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.textAlign: TextAlign.Center
                    textStyle.color: Color.create(bookManager.uiInk)
                    multiline: true
                }

                Button {
                    visible: (bookManager.browserSearchQuery !== "" || bookManager.browserFormatFilter !== "all")
                    topMargin: bookManager.dp(16)
                    appearance: ControlAppearance.Primary
                    horizontalAlignment: HorizontalAlignment.Center
                    text: qsTr("Xóa bộ lọc & tìm kiếm") + Retranslate.onLanguageChanged
                    onClicked: {
                        bookManager.clearBrowserFilter();
                        browserSearchInputField.text = "";
                    }
                }
            }

            // 4A. DETAILED LIST VIEW
            ListView {
                id: fileBrowserDetailedListView
                function fwd_showBookDetail() { return browserPage.showBookDetail.apply(browserPage, arguments); }
                function fwd_showToast() { return browserPage.showToast.apply(browserPage, arguments); }
                function fwd_confirmDeleteFile() { return browserPage.confirmDeleteFile.apply(browserPage, arguments); }

                visible: bookManager.browserItemCount > 0 && bookManager.libraryViewMode === "detailed"
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                dataModel: bookManager.fileBrowserModel

                layout: StackListLayout {
                    headerMode: ListHeaderMode.None
                }

                listItemComponents: [
                    ListItemComponent {
                        type: "item"
                        CustomListItem {
                            id: ctxCell284
                            dividerVisible: true
                            highlightAppearance: HighlightAppearance.Frame

                            contextActions: [
                                ActionSet {
                                    title: ListItemData.title ? ListItemData.title : (ListItemData.name ? ListItemData.name : "")
                                    subtitle: ListItemData.author ? ListItemData.author : (ListItemData.subtext ? ListItemData.subtext : "")

                                    ActionItem {
                                        title: (ListItemData.itemType === "folder" || ListItemData.isDir) ? (qsTr("Mở thư mục") + Retranslate.onLanguageChanged) : (qsTr("Đọc sách") + Retranslate.onLanguageChanged)
                                        imageSource: "asset:///icons/ic_open.png"
                                        onTriggered: {
                                            if (ListItemData.path === "__UP__") bookManager.browserGoUp();
                                            else if (ListItemData.itemType === "folder" || ListItemData.isDir) bookManager.openBrowserFolder(ListItemData.path);
                                            else app.openBook(ListItemData.filePath ? ListItemData.filePath : ListItemData.path);
                                        }
                                    }

                                    ActionItem {
                                        title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_info.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: { ctxCell284.ListItem.view.fwd_showBookDetail(ListItemData); }
                                    }

                                    ActionItem {
                                        title: (ListItemData.isFavorite ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
                                        imageSource: ListItemData.isFavorite ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var newState = bookManager.toggleFavorite(path);
                                            ctxCell284.ListItem.view.fwd_showToast(newState ? (qsTr("Đã thêm vào yêu thích: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)) : (qsTr("Đã bỏ yêu thích: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)));
                                        }
                                    }

                                    ActionItem {
                                        title: (ListItemData.isReadingList ? qsTr("Bỏ khỏi DS đọc") : qsTr("Thêm vào danh sách đọc")) + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_book.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var newState = bookManager.toggleReadingList(path);
                                            ctxCell284.ListItem.view.fwd_showToast(newState ? (qsTr("Đã thêm vào DS đọc: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)) : (qsTr("Đã bỏ khỏi DS đọc: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)));
                                        }
                                    }

                                    ActionItem {
                                        title: (ListItemData.progress >= 100 ? qsTr("Đánh dấu chưa đọc") : qsTr("Đánh dấu đã đọc xong")) + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_done.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var isCompleted = bookManager.toggleBookCompleted(path);
                                            ctxCell284.ListItem.view.fwd_showToast(isCompleted ? (qsTr("Đã đánh dấu đã đọc xong: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)) : (qsTr("Đã đánh dấu chưa đọc: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)));
                                        }
                                    }

                                    DeleteActionItem {
                                        title: qsTr("Xóa file khỏi máy") + Retranslate.onLanguageChanged
                                        enabled: ListItemData.path !== "__UP__" && !ListItemData.isDir && ListItemData.itemType !== "folder"
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var label = ListItemData.title ? ListItemData.title : ListItemData.name;
                                            ctxCell284.ListItem.view.fwd_confirmDeleteFile(path, label);
                                        }
                                    }
                                }
                            ]

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                minHeight: (ListItemData.itemType === "folder" || ListItemData.isDir) ? bookManager.dp(76) : bookManager.dp(140)
                                topPadding: (ListItemData.itemType === "folder" || ListItemData.isDir) ? bookManager.dp(10) : bookManager.dp(6)
                                bottomPadding: (ListItemData.itemType === "folder" || ListItemData.isDir) ? bookManager.dp(10) : bookManager.dp(6)
                                leftPadding: bookManager.dp(14)
                                rightPadding: bookManager.dp(14)
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                // Left Icon / Thumbnail
                                Container {
                                    preferredWidth: (ListItemData.itemType === "folder" || ListItemData.isDir) ? bookManager.dp(56) : bookManager.dp(88)
                                    preferredHeight: (ListItemData.itemType === "folder" || ListItemData.isDir) ? bookManager.dp(56) : bookManager.dp(128)
                                    minWidth: (ListItemData.itemType === "folder" || ListItemData.isDir) ? bookManager.dp(56) : bookManager.dp(88)
                                    maxWidth: (ListItemData.itemType === "folder" || ListItemData.isDir) ? bookManager.dp(56) : bookManager.dp(88)
                                    minHeight: (ListItemData.itemType === "folder" || ListItemData.isDir) ? bookManager.dp(56) : bookManager.dp(128)
                                    maxHeight: (ListItemData.itemType === "folder" || ListItemData.isDir) ? bookManager.dp(56) : bookManager.dp(128)
                                    verticalAlignment: VerticalAlignment.Center
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}

                                    // Folder Icon
                                    ImageView {
                                        visible: ListItemData.itemType === "folder" || ListItemData.isDir
                                        horizontalAlignment: HorizontalAlignment.Center
                                        verticalAlignment: VerticalAlignment.Center
                                        preferredWidth: bookManager.dp(32)
                                        preferredHeight: bookManager.dp(32)
                                        imageSource: {
                                            if (bookManager.appTheme === "dark") {
                                                if (ListItemData.path === "__UP__" || ListItemData.folderType === "up") return "asset:///icons/ic_previous.png";
                                                if (ListItemData.folderType === "sdcard") return "asset:///icons/ic_sdcard.png";
                                                if (ListItemData.folderType === "internal") return "asset:///icons/ic_storage.png";
                                                return ListItemData.icon ? ListItemData.icon : "asset:///icons/ic_folder.png";
                                            }
                                            if (ListItemData.path === "__UP__" || ListItemData.folderType === "up") return "asset:///icons/ic_previous_dark.png";
                                            if (ListItemData.folderType === "sdcard") return "asset:///icons/ic_sdcard_dark.png";
                                            if (ListItemData.folderType === "internal") return "asset:///icons/ic_storage_dark.png";
                                            return "asset:///icons/ic_folder_dark.png";
                                        }
                                        scalingMethod: ScalingMethod.AspectFit
                                    }

                                    // Book Cover (real)
                                    ImageView {
                                        visible: ListItemData.itemType === "book" && !ListItemData.isDir && ListItemData.cover !== undefined && ListItemData.cover !== ""
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        imageSource: (ListItemData.cover !== undefined && ListItemData.cover !== "") ? ("file://" + ListItemData.cover) : ""
                                        scalingMethod: ScalingMethod.AspectFill
                                    }

                                    // Coverless Book Placeholder Icon (avoids duplicating format pill)
                                    ImageView {
                                        visible: ListItemData.itemType === "book" && !ListItemData.isDir && (ListItemData.cover === undefined || ListItemData.cover === "")
                                        horizontalAlignment: HorizontalAlignment.Center
                                        verticalAlignment: VerticalAlignment.Center
                                        preferredWidth: bookManager.dp(32)
                                        preferredHeight: bookManager.dp(32)
                                        opacity: 0.55
                                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_book.png" : "asset:///icons/ic_book_dark.png"
                                        scalingMethod: ScalingMethod.AspectFit
                                    }
                                }

                                // Info Column
                                Container {
                                    leftMargin: bookManager.dp(14)
                                    rightMargin: bookManager.dp(8)
                                    verticalAlignment: VerticalAlignment.Center
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

                                    Label {
                                        text: ListItemData.name ? ListItemData.name : (ListItemData.title ? ListItemData.title : "")
                                        textStyle.base: SystemDefaults.TextStyles.TitleText
                                        textStyle.fontWeight: (ListItemData.itemType === "folder" || ListItemData.isDir) ? FontWeight.Bold : FontWeight.W300
                                        textStyle.color: Color.create(bookManager.uiInk)
                                        multiline: true
                                        autoSize.maxLineCount: 2
                                    }

                                    Label {
                                        topMargin: bookManager.dp(2)
                                        text: (ListItemData.itemType === "folder" || ListItemData.isDir)
                                            ? (ListItemData.subtext ? ListItemData.subtext : "")
                                            : ((ListItemData.author && ListItemData.author !== "") ? ListItemData.author : (qsTr("Tác giả không rõ") + Retranslate.onLanguageChanged))
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        autoSize.maxLineCount: 1
                                    }

                                    Label {
                                        visible: !(ListItemData.itemType === "folder" || ListItemData.isDir) && (ListItemData.pageInfoStr !== undefined && ListItemData.pageInfoStr !== "")
                                        topMargin: bookManager.dp(2)
                                        text: ListItemData.pageInfoStr ? ListItemData.pageInfoStr : ""
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiAccent)
                                        autoSize.maxLineCount: 1
                                    }

                                    Container {
                                        visible: ListItemData.itemType === "book" && !ListItemData.isDir && ListItemData.progress !== undefined && ListItemData.progress > 0
                                        topMargin: bookManager.dp(6)
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        preferredHeight: bookManager.dp(3)
                                        background: Color.create(bookManager.uiSurface)
                                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                        Container {
                                            verticalAlignment: VerticalAlignment.Fill
                                            background: Color.create("#D4A359")
                                            layoutProperties: StackLayoutProperties {
                                                spaceQuota: Math.max(0.001, ListItemData.progress)
                                            }
                                        }
                                        Container {
                                            verticalAlignment: VerticalAlignment.Fill
                                            visible: ListItemData.progress < 100
                                            background: Color.Transparent
                                            layoutProperties: StackLayoutProperties {
                                                spaceQuota: Math.max(0.001, 100 - ListItemData.progress)
                                            }
                                        }
                                    }
                                }

                                // Monochrome Format / Status Pill + File Size
                                Container {
                                    visible: ListItemData.itemType === "book" && !ListItemData.isDir && ListItemData.format !== undefined && ListItemData.format !== ""
                                    verticalAlignment: VerticalAlignment.Center
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Right
                                        background: ListItemData.progress >= 100 ? Color.create("#D4A359") : Color.create(bookManager.uiSurface)
                                        topPadding: bookManager.dp(4)
                                        bottomPadding: bookManager.dp(4)
                                        leftPadding: bookManager.dp(10)
                                        rightPadding: bookManager.dp(10)

                                        Label {
                                            text: ListItemData.progress >= 100 ?
                                                  (qsTr("Xong") + Retranslate.onLanguageChanged) :
                                                  (ListItemData.format ? ListItemData.format.toUpperCase() : "")
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: ListItemData.progress >= 100 ? Color.create("#0D0B09") : Color.create(bookManager.uiInk)
                                        }
                                    }

                                    Label {
                                        visible: ListItemData.fileSize !== undefined && ListItemData.fileSize !== ""
                                        topMargin: bookManager.dp(4)
                                        horizontalAlignment: HorizontalAlignment.Right
                                        text: ListItemData.fileSize ? ListItemData.fileSize : ""
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                    }
                                }
                            }
                        }
                    }
                ]

                onTriggered: {
                    var item = dataModel.data(indexPath);
                    if (item) {
                        if (item.path === "__UP__") {
                            bookManager.browserGoUp();
                        } else if (item.itemType === "folder" || item.isDir) {
                            bookManager.openBrowserFolder(item.path);
                        } else {
                            var path = item.filePath ? item.filePath : item.path;
                            app.openBook(path);
                        }
                    }
                }
            }

            // 4B. MOSAIC GRID VIEW (functions placed INSIDE ListView so context menu works)
            ListView {
                id: fileBrowserMosaicListView
                function fwd_showBookDetail() { return browserPage.showBookDetail.apply(browserPage, arguments); }
                function fwd_showToast() { return browserPage.showToast.apply(browserPage, arguments); }
                function fwd_confirmDeleteFile() { return browserPage.confirmDeleteFile.apply(browserPage, arguments); }

                visible: bookManager.browserItemCount > 0 && bookManager.libraryViewMode === "mosaic"
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                dataModel: bookManager.fileBrowserModel

                layout: GridListLayout {
                    columnCount: bookManager.isHiRes ? 4 : 3
                    headerMode: ListHeaderMode.None
                    cellAspectRatio: 0.62
                    horizontalCellSpacing: bookManager.dp(10)
                    verticalCellSpacing: bookManager.dp(12)
                }

                listItemComponents: [
                    ListItemComponent {
                        type: "item"
                        CustomListItem {
                            id: ctxCell563
                            dividerVisible: false
                            highlightAppearance: HighlightAppearance.Frame

                            contextActions: [
                                ActionSet {
                                    title: ListItemData.title ? ListItemData.title : (ListItemData.name ? ListItemData.name : "")
                                    subtitle: ListItemData.author ? ListItemData.author : (ListItemData.subtext ? ListItemData.subtext : "")

                                    ActionItem {
                                        title: (ListItemData.itemType === "folder" || ListItemData.isDir) ? (qsTr("Mở thư mục") + Retranslate.onLanguageChanged) : (qsTr("Đọc sách") + Retranslate.onLanguageChanged)
                                        imageSource: "asset:///icons/ic_open.png"
                                        onTriggered: {
                                            if (ListItemData.path === "__UP__") bookManager.browserGoUp();
                                            else if (ListItemData.itemType === "folder" || ListItemData.isDir) bookManager.openBrowserFolder(ListItemData.path);
                                            else app.openBook(ListItemData.filePath ? ListItemData.filePath : ListItemData.path);
                                        }
                                    }

                                    ActionItem {
                                        title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_info.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: { ctxCell563.ListItem.view.fwd_showBookDetail(ListItemData); }
                                    }

                                    ActionItem {
                                        title: (ListItemData.isFavorite ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
                                        imageSource: ListItemData.isFavorite ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var newState = bookManager.toggleFavorite(path);
                                            ctxCell563.ListItem.view.fwd_showToast(newState ? (qsTr("Đã thêm vào yêu thích: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)) : (qsTr("Đã bỏ yêu thích: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)));
                                        }
                                    }

                                    ActionItem {
                                        title: (ListItemData.isReadingList ? qsTr("Bỏ khỏi DS đọc") : qsTr("Thêm vào danh sách đọc")) + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_book.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var newState = bookManager.toggleReadingList(path);
                                            ctxCell563.ListItem.view.fwd_showToast(newState ? (qsTr("Đã thêm vào DS đọc: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)) : (qsTr("Đã bỏ khỏi DS đọc: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)));
                                        }
                                    }

                                    ActionItem {
                                        title: (ListItemData.progress >= 100 ? qsTr("Đánh dấu chưa đọc") : qsTr("Đánh dấu đã đọc xong")) + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_done.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var isCompleted = bookManager.toggleBookCompleted(path);
                                            ctxCell563.ListItem.view.fwd_showToast(isCompleted ? (qsTr("Đã đánh dấu đã đọc xong: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)) : (qsTr("Đã đánh dấu chưa đọc: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)));
                                        }
                                    }

                                    DeleteActionItem {
                                        title: qsTr("Xóa file khỏi máy") + Retranslate.onLanguageChanged
                                        enabled: ListItemData.path !== "__UP__" && !ListItemData.isDir && ListItemData.itemType !== "folder"
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var label = ListItemData.title ? ListItemData.title : ListItemData.name;
                                            ctxCell563.ListItem.view.fwd_confirmDeleteFile(path, label);
                                        }
                                    }
                                }
                            ]

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                topMargin: bookManager.dp(4)
                                leftMargin: bookManager.dp(4)
                                rightMargin: bookManager.dp(4)
                                background: Color.create(bookManager.uiCard)
                                layout: DockLayout {}

                                // 1. Folder Card
                                Container {
                                    visible: ListItemData.itemType === "folder" || ListItemData.isDir
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.create(bookManager.uiCard)
                                    layout: DockLayout {}

                                    // 1A. Folder with preview cover
                                    Container {
                                        visible: ListItemData.path !== "__UP__" && ListItemData.cover !== undefined && ListItemData.cover !== ""
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        layout: DockLayout {}

                                        ImageView {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            verticalAlignment: VerticalAlignment.Fill
                                            imageSource: (ListItemData.cover !== undefined && ListItemData.cover !== "") ? ("file://" + ListItemData.cover) : ""
                                            scalingMethod: ScalingMethod.AspectFill
                                        }

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Left
                                            verticalAlignment: VerticalAlignment.Top
                                            topMargin: bookManager.dp(4)
                                            leftMargin: bookManager.dp(4)
                                            background: Color.create("#CC141210")
                                            leftPadding: bookManager.dp(6)
                                            rightPadding: bookManager.dp(6)
                                            topPadding: bookManager.dp(3)
                                            bottomPadding: bookManager.dp(3)
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            ImageView {
                                                preferredWidth: bookManager.dp(14)
                                                preferredHeight: bookManager.dp(14)
                                                verticalAlignment: VerticalAlignment.Center
                                                imageSource: "asset:///icons/ic_folder.png"
                                            }

                                            Label {
                                                leftMargin: bookManager.dp(4)
                                                text: qsTr("Thư mục") + Retranslate.onLanguageChanged
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: Color.create("#F5EFE6")
                                            }
                                        }

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            verticalAlignment: VerticalAlignment.Bottom
                                            background: Color.create("#D90E0C0A")
                                            topPadding: bookManager.dp(6)
                                            bottomPadding: bookManager.dp(6)
                                            leftPadding: bookManager.dp(8)
                                            rightPadding: bookManager.dp(8)

                                            Label {
                                                horizontalAlignment: HorizontalAlignment.Fill
                                                text: ListItemData.name ? ListItemData.name : (ListItemData.title ? ListItemData.title : "")
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: Color.create("#F5EFE6")
                                                multiline: true
                                                autoSize.maxLineCount: 2
                                            }
                                        }
                                    }

                                    // 1B. Plain Folder
                                    Container {
                                        visible: ListItemData.path !== "__UP__" && (ListItemData.cover === undefined || ListItemData.cover === "")
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.create(bookManager.uiCard)
                                        topPadding: bookManager.dp(14)
                                        bottomPadding: bookManager.dp(10)
                                        leftPadding: bookManager.dp(8)
                                        rightPadding: bookManager.dp(8)
                                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            verticalAlignment: VerticalAlignment.Center
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            layout: DockLayout {}

                                            ImageView {
                                                horizontalAlignment: HorizontalAlignment.Center
                                                verticalAlignment: VerticalAlignment.Center
                                                preferredWidth: bookManager.dp(58)
                                                preferredHeight: bookManager.dp(58)
                                                opacity: 0.55
                                                imageSource: {
                                                    if (bookManager.appTheme === "dark") {
                                                        if (ListItemData.folderType === "sdcard") return "asset:///icons/ic_sdcard.png";
                                                        if (ListItemData.folderType === "internal") return "asset:///icons/ic_storage.png";
                                                        return ListItemData.icon ? ListItemData.icon : "asset:///icons/ic_folder.png";
                                                    }
                                                    if (ListItemData.folderType === "sdcard") return "asset:///icons/ic_sdcard_dark.png";
                                                    if (ListItemData.folderType === "internal") return "asset:///icons/ic_storage_dark.png";
                                                    return "asset:///icons/ic_folder_dark.png";
                                                }
                                                scalingMethod: ScalingMethod.AspectFit
                                            }
                                        }

                                        Label {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: ListItemData.name ? ListItemData.name : (ListItemData.title ? ListItemData.title : "")
                                            textStyle.base: SystemDefaults.TextStyles.BodyText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.textAlign: TextAlign.Center
                                            textStyle.color: Color.create(bookManager.uiInk)
                                            multiline: true
                                            autoSize.maxLineCount: 2
                                        }

                                        Label {
                                            topMargin: bookManager.dp(2)
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: ListItemData.subtext ? ListItemData.subtext : (qsTr("Thư mục") + Retranslate.onLanguageChanged)
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: Color.create(bookManager.uiMuted)
                                            autoSize.maxLineCount: 1
                                        }
                                    }

                                    // 1C. Parent directory (..)
                                    Container {
                                        visible: ListItemData.path === "__UP__"
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.create(bookManager.uiSurface)
                                        topPadding: bookManager.dp(14)
                                        bottomPadding: bookManager.dp(10)
                                        leftPadding: bookManager.dp(8)
                                        rightPadding: bookManager.dp(8)
                                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            verticalAlignment: VerticalAlignment.Center
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            layout: DockLayout {}

                                            ImageView {
                                                horizontalAlignment: HorizontalAlignment.Center
                                                verticalAlignment: VerticalAlignment.Center
                                                preferredWidth: bookManager.dp(46)
                                                preferredHeight: bookManager.dp(46)
                                                opacity: 0.65
                                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_previous.png" : "asset:///icons/ic_previous_dark.png"
                                                scalingMethod: ScalingMethod.AspectFit
                                            }
                                        }

                                        Label {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: ListItemData.name ? ListItemData.name : (".. (" + (qsTr("Lùi lại") + Retranslate.onLanguageChanged) + ")")
                                            textStyle.base: SystemDefaults.TextStyles.BodyText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.textAlign: TextAlign.Center
                                            textStyle.color: Color.create(bookManager.uiInk)
                                            multiline: true
                                            autoSize.maxLineCount: 2
                                        }

                                        Label {
                                            topMargin: bookManager.dp(2)
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: ListItemData.subtext ? ListItemData.subtext : (qsTr("Trở về thư mục trước") + Retranslate.onLanguageChanged)
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: Color.create(bookManager.uiMuted)
                                            autoSize.maxLineCount: 1
                                        }
                                    }
                                }

                                // 2. Book Card
                                Container {
                                    visible: ListItemData.itemType === "book" && !ListItemData.isDir
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.create(bookManager.uiCard)
                                    layout: DockLayout {}

                                    ImageView {
                                        visible: ListItemData.cover !== undefined && ListItemData.cover !== ""
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        imageSource: (ListItemData.cover !== undefined && ListItemData.cover !== "") ? ("file://" + ListItemData.cover) : ""
                                        scalingMethod: ScalingMethod.AspectFill
                                    }

                                    Container {
                                        visible: (ListItemData.cover !== undefined && ListItemData.cover !== "") && ListItemData.progress >= 100
                                        horizontalAlignment: HorizontalAlignment.Left
                                        verticalAlignment: VerticalAlignment.Top
                                        topMargin: bookManager.dp(4)
                                        leftMargin: bookManager.dp(4)
                                        background: Color.create("#D4A359")
                                        leftPadding: bookManager.dp(6)
                                        rightPadding: bookManager.dp(6)
                                        topPadding: bookManager.dp(2)
                                        bottomPadding: bookManager.dp(2)

                                        Label {
                                            text: qsTr("Hoàn thành") + Retranslate.onLanguageChanged
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: Color.create("#0D0B09")
                                        }
                                    }

                                    Container {
                                        visible: ListItemData.cover !== undefined && ListItemData.cover !== ""
                                        horizontalAlignment: HorizontalAlignment.Right
                                        verticalAlignment: VerticalAlignment.Top
                                        topMargin: bookManager.dp(4)
                                        rightMargin: bookManager.dp(4)
                                        background: Color.create("#CC141210")
                                        leftPadding: bookManager.dp(6)
                                        rightPadding: bookManager.dp(6)
                                        topPadding: bookManager.dp(2)
                                        bottomPadding: bookManager.dp(2)

                                        Label {
                                            text: ListItemData.formatBadge ? ListItemData.formatBadge : (ListItemData.format ? ListItemData.format.toUpperCase() : "")
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: Color.create("#F5EFE6")
                                        }
                                    }

                                    Container {
                                        visible: (ListItemData.cover !== undefined && ListItemData.cover !== "") && (ListItemData.progress > 0 || ListItemData.totalPages > 0)
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Bottom
                                        background: Color.create("#BF0E0C0A")
                                        leftPadding: bookManager.dp(6)
                                        rightPadding: bookManager.dp(6)
                                        topPadding: bookManager.dp(3)
                                        bottomPadding: bookManager.dp(3)
                                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Label {
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                text: ListItemData.pageInfoStr ? ListItemData.pageInfoStr : (ListItemData.totalPages > 0 ? (ListItemData.totalPages + (bookManager.language === "en" ? " p" : " tr")) : "")
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.color: Color.create("#F5EFE6")
                                                autoSize.maxLineCount: 1
                                            }

                                            Label {
                                                text: (ListItemData.progress > 0 ? ListItemData.progress : 0) + "%"
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: Color.create("#E6C07B")
                                            }
                                        }

                                        Container {
                                            topMargin: bookManager.dp(2)
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            preferredHeight: bookManager.dp(3)
                                            background: Color.create("#40FFFFFF")
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Container {
                                                verticalAlignment: VerticalAlignment.Fill
                                                background: Color.create("#D4A359")
                                                layoutProperties: StackLayoutProperties {
                                                    spaceQuota: Math.max(0.001, ListItemData.progress !== undefined ? ListItemData.progress : 0)
                                                }
                                            }
                                            Container {
                                                verticalAlignment: VerticalAlignment.Fill
                                                visible: ListItemData.progress < 100
                                                background: Color.Transparent
                                                layoutProperties: StackLayoutProperties {
                                                    spaceQuota: Math.max(0.001, 100 - (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                }
                                            }
                                        }
                                    }

                                    // Coverless Book Card
                                    Container {
                                        visible: ListItemData.cover === undefined || ListItemData.cover === ""
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.create(bookManager.uiCard)
                                        leftPadding: bookManager.dp(10)
                                        rightPadding: bookManager.dp(10)
                                        topPadding: bookManager.dp(10)
                                        bottomPadding: bookManager.dp(10)
                                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Container {
                                                visible: ListItemData.progress >= 100
                                                background: Color.create("#D4A359")
                                                leftPadding: bookManager.dp(6)
                                                rightPadding: bookManager.dp(6)
                                                topPadding: bookManager.dp(2)
                                                bottomPadding: bookManager.dp(2)
                                                verticalAlignment: VerticalAlignment.Center

                                                Label {
                                                    text: qsTr("Hoàn thành") + Retranslate.onLanguageChanged
                                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                                    textStyle.fontWeight: FontWeight.Bold
                                                    textStyle.color: Color.create("#0D0B09")
                                                }
                                            }

                                            Container {
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            }

                                            Container {
                                                background: Color.create(bookManager.uiSurface)
                                                leftPadding: bookManager.dp(6)
                                                rightPadding: bookManager.dp(6)
                                                topPadding: bookManager.dp(2)
                                                bottomPadding: bookManager.dp(2)
                                                verticalAlignment: VerticalAlignment.Center

                                                Label {
                                                    text: ListItemData.formatBadge ? ListItemData.formatBadge : (ListItemData.format ? ListItemData.format.toUpperCase() : "")
                                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                                    textStyle.fontWeight: FontWeight.Bold
                                                    textStyle.color: Color.create(bookManager.uiInk)
                                                }
                                            }
                                        }

                                        Container {
                                            verticalAlignment: VerticalAlignment.Center
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            topMargin: bookManager.dp(6)
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

                                            Label {
                                                horizontalAlignment: HorizontalAlignment.Center
                                                text: ListItemData.title ? ListItemData.title : (ListItemData.name ? ListItemData.name : "")
                                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                                textStyle.fontWeight: FontWeight.Normal
                                                textStyle.textAlign: TextAlign.Center
                                                textStyle.color: Color.create(bookManager.uiInk)
                                                multiline: true
                                                autoSize.maxLineCount: 3
                                            }

                                            Label {
                                                topMargin: bookManager.dp(3)
                                                horizontalAlignment: HorizontalAlignment.Center
                                                text: ListItemData.author ? ListItemData.author : ""
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.textAlign: TextAlign.Center
                                                textStyle.color: Color.create(bookManager.uiMuted)
                                                multiline: true
                                                autoSize.maxLineCount: 1
                                            }
                                        }

                                        Label {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: (ListItemData.fileSize ? ListItemData.fileSize : "") + (ListItemData.progress > 0 ? (" • " + ListItemData.progress + "%") : "")
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: ListItemData.progress >= 100 ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiMuted)
                                        }

                                        Container {
                                            visible: ListItemData.progress > 0
                                            topMargin: bookManager.dp(4)
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            preferredHeight: bookManager.dp(3)
                                            background: Color.create(bookManager.uiSurface)
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Container {
                                                verticalAlignment: VerticalAlignment.Fill
                                                background: Color.create("#D4A359")
                                                layoutProperties: StackLayoutProperties {
                                                    spaceQuota: Math.max(0.001, ListItemData.progress !== undefined ? ListItemData.progress : 0)
                                                }
                                            }
                                            Container {
                                                verticalAlignment: VerticalAlignment.Fill
                                                visible: ListItemData.progress < 100
                                                background: Color.Transparent
                                                layoutProperties: StackLayoutProperties {
                                                    spaceQuota: Math.max(0.001, 100 - (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                ]

                onTriggered: {
                    var item = dataModel.data(indexPath);
                    if (item) {
                        if (item.path === "__UP__") {
                            bookManager.browserGoUp();
                        } else if (item.itemType === "folder" || item.isDir) {
                            bookManager.openBrowserFolder(item.path);
                        } else {
                            var path = item.filePath ? item.filePath : item.path;
                            app.openBook(path);
                        }
                    }
                }
            }
        }
    }

    attachedObjects: [
        ActionItem {
            id: upAction
            title: qsTr("Lùi lại") + Retranslate.onLanguageChanged
            enabled: bookManager.canGoUp
            onTriggered: {
                bookManager.browserGoUp();
            }
        },
        BookDetailSheet {
            id: bookDetailSheet
            onRequestOpenBook: {
                app.openBook(filePath);
            }
        },
        SystemToast {
            id: browserToast
            body: ""
        },
        SystemDialog {
            id: browserDeleteDialog
            title: qsTr("Xóa file khỏi máy?") + Retranslate.onLanguageChanged
            confirmButton.label: qsTr("Xóa vĩnh viễn") + Retranslate.onLanguageChanged
            cancelButton.label: qsTr("Hủy") + Retranslate.onLanguageChanged
            onFinished: {
                if (result === SystemUiResult.ConfirmButtonSelection && browserPage.pendingDeletePath !== "") {
                    var ok = bookManager.deleteBookFile(browserPage.pendingDeletePath);
                    browserPage.showToast(ok ? (qsTr("Đã xóa file: ") + browserPage.pendingDeleteTitle) : qsTr("Không thể xóa file khỏi bộ nhớ."));
                    browserPage.pendingDeletePath = "";
                    browserPage.pendingDeleteTitle = "";
                }
            }
        }
    ]
}
