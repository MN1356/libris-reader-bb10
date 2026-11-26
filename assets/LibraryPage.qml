import bb.cascades 1.3
import bb.cascades.pickers 1.0
import bb.system 1.2

Page {
    id: libraryPage

    actionBarVisibility: ChromeVisibility.Visible

    Menu.definition: MenuDefinition {
        id: appMenu
        objectName: "appMenu"

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
            title: qsTr("Bộ lọc") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_filter.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                filterSortSheet.open();
            }
        },
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
            title: qsTr("Quản lý Kệ sách") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_library.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                shelfManagerSheet.openForManage();
            }
        },
        ActionItem {
            title: qsTr("Mở Tệp") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_open.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            shortcuts: [
                Shortcut { key: "O" },
                Shortcut { key: "E" }
            ]
            onTriggered: {
                app.openFilePicker();
            }
        },
        ActionItem {
            title: qsTr("Kho sách OPDS") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_download.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            shortcuts: [
                Shortcut { key: "K" }
            ]
            onTriggered: {
                app.openOpds();
            }
        },
        ActionItem {
            title: qsTr("Truyền sách Wi-Fi") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_wifi.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            shortcuts: [
                Shortcut { key: "W" }
            ]
            onTriggered: {
                app.openWifiDrop();
            }
        },
        ActionItem {
            title: qsTr("Quét lại") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_refresh.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            onTriggered: {
                bookManager.scanLibrary();
                libraryPage.showToast(qsTr("Đang nạp và quét thư viện sách...") + Retranslate.onLanguageChanged);
            }
        }
    ]

    property bool isSearching: false

    onIsSearchingChanged: {
        if (isSearching) {
            searchInputField.requestFocus();
        }
    }

    function openBookWithLoading(filePath) {
        if (!filePath) return;
        app.openBook(filePath);
    }

    function showToast(msg) {
        if (!msg) return;
        libraryToast.body = msg;
        libraryToast.show();
    }

    function showBookDetail(item) {
        if (!item) return;
        bookDetailSheet.openWithBook(item);
    }

    function openShelfAssign(filePath, title) {
        shelfManagerSheet.openForBook(filePath, title);
    }

    titleBar: TitleBar {
        title: qsTr("Tủ Sách") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
        acceptAction: ActionItem {
            title: (libraryPage.isSearching ? qsTr("Đóng") : qsTr("Tìm")) + Retranslate.onLanguageChanged
            onTriggered: {
                libraryPage.isSearching = !libraryPage.isSearching;
                if (!libraryPage.isSearching) {
                    searchInputField.text = "";
                    bookManager.setSearchQuery("");
                }
            }
        }
    }

    Container {
        id: rootContainer
        layout: DockLayout {}
        background: Color.create(bookManager.uiBg)

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            // 1. INLINE LIBRARY SEARCH BAR
            Container {
                id: searchBarContainer
                visible: libraryPage.isSearching
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
                    id: searchInputField
                    leftMargin: bookManager.dp(10)
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    hintText: qsTr("Tìm theo tên sách hoặc tác giả...") + Retranslate.onLanguageChanged
                    input.submitKey: SubmitKey.Search
                    clearButtonVisible: true
                    text: bookManager.searchQuery
                    onTextChanging: {
                        bookManager.setSearchQuery(text);
                    }
                    input.onSubmitted: {
                        searchInputField.loseFocus();
                    }
                }
            }

            Container {
                visible: libraryPage.isSearching
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: 1
                background: Color.create(bookManager.uiHairline)
            }

            // 2. CATEGORY FILTER BAR + COMPACT STATUS STRIP
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(bookManager.uiCard)
                topPadding: bookManager.dp(10)
                bottomPadding: bookManager.dp(10)
                leftPadding: bookManager.dp(14)
                rightPadding: bookManager.dp(14)
                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                SegmentedControl {
                    id: shelfCategoryControl
                    property bool syncingFilter: false
                    horizontalAlignment: HorizontalAlignment.Fill

                    function syncFromManager() {
                        syncingFilter = true;
                        var cf = bookManager.categoryFilter;
                        if (cf === "reading") selectedIndex = 1;
                        else if (cf === "favorites") selectedIndex = 2;
                        else if (cf === "all_scanned") selectedIndex = 3;
                        else selectedIndex = 0;
                        syncingFilter = false;
                    }

                    onCreationCompleted: {
                        syncFromManager();
                        bookManager.categoryFilterChanged.connect(syncFromManager);
                    }

                    Option {
                        text: qsTr("Tủ sách") + Retranslate.onLanguageChanged
                        value: "all"
                        selected: true
                    }
                    Option {
                        text: qsTr("Đang đọc") + Retranslate.onLanguageChanged
                        value: "reading"
                    }
                    Option {
                        text: qsTr("Yêu thích") + Retranslate.onLanguageChanged
                        value: "favorites"
                    }
                    Option {
                        text: qsTr("Kho máy") + Retranslate.onLanguageChanged
                        value: "all_scanned"
                    }

                    onSelectedValueChanged: {
                        if (!syncingFilter && selectedValue && selectedValue !== bookManager.categoryFilter) {
                            bookManager.setCategoryFilter(selectedValue);
                        }
                    }
                }

                Container {
                    topMargin: bookManager.dp(8)
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    Label {
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        text: {
                            if (bookManager.searchQuery !== "") {
                                return (qsTr("Kết quả") + ": " + bookManager.bookCount) + Retranslate.onLanguageChanged;
                            }
                            var cf = bookManager.categoryFilter;
                            var cfLabel = "";
                            if (cf.indexOf("shelf:") === 0) cfLabel = " • " + qsTr("Kệ") + ": " + cf.substring(6);
                            else if (cf === "completed") cfLabel = " • " + qsTr("Hoàn thành");
                            else if (cf === "readingList") cfLabel = " • " + qsTr("DS đọc");
                            else if (cf === "unread") cfLabel = " • " + qsTr("Chưa đọc");
                            var unit = bookManager.language === "en" ? (bookManager.bookCount === 1 ? "book" : "books") : qsTr("cuốn");
                            return (bookManager.bookCount + " " + unit + cfLabel +
                                (bookManager.formatFilter !== "all" ? (" • " + bookManager.formatFilter.toUpperCase()) : "") +
                                (bookManager.groupBy === "author" ? (qsTr(" • Theo Tác giả") + Retranslate.onLanguageChanged) :
                                 bookManager.groupBy === "series" ? (qsTr(" • Theo Series") + Retranslate.onLanguageChanged) : "")) + Retranslate.onLanguageChanged;
                        }
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiMuted)
                        autoSize.maxLineCount: 1
                    }

                    Container {
                        visible: (bookManager.categoryFilter !== "all" && bookManager.categoryFilter !== "") || bookManager.formatFilter !== "all" || bookManager.searchQuery !== ""
                        leftMargin: bookManager.dp(10)
                        verticalAlignment: VerticalAlignment.Center
                        background: Color.create(bookManager.uiSurface)
                        leftPadding: bookManager.dp(10)
                        rightPadding: bookManager.dp(10)
                        topPadding: bookManager.dp(4)
                        bottomPadding: bookManager.dp(4)

                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    bookManager.resetFilters();
                                    searchInputField.text = "";
                                }
                            }
                        ]

                        Label {
                            text: qsTr("Đặt lại") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create(bookManager.uiAccent)
                        }
                    }
                }
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: 1
                background: Color.create(bookManager.uiHairline)
            }

            // 3. BOOKSHELF MAIN AREA
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                layout: DockLayout {}

                // Empty State 1: Tủ sách trống
                Container {
                    visible: bookManager.bookCount === 0 && (bookManager.categoryFilter === "all" || bookManager.categoryFilter === "") && bookManager.formatFilter === "all" && bookManager.searchQuery === ""
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    leftPadding: bookManager.dp(28)
                    rightPadding: bookManager.dp(28)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    ImageView {
                        horizontalAlignment: HorizontalAlignment.Center
                        preferredWidth: bookManager.dp(76)
                        preferredHeight: bookManager.dp(76)
                        opacity: 0.38
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_book.png" : "asset:///icons/ic_book_dark.png"
                        scalingMethod: ScalingMethod.AspectFit
                    }

                    Label {
                        topMargin: bookManager.dp(14)
                        text: qsTr("Tủ sách đang trống") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.textAlign: TextAlign.Center
                        textStyle.color: Color.create(bookManager.uiInk)
                        horizontalAlignment: HorizontalAlignment.Center
                        multiline: true
                    }

                    Label {
                        topMargin: bookManager.dp(6)
                        text: qsTr("Tủ sách của bạn đang trống. Hãy mở tệp hoặc duyệt thư mục để bắt đầu đọc sách!") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.textAlign: TextAlign.Center
                        textStyle.color: Color.create(bookManager.uiMuted)
                        multiline: true
                        horizontalAlignment: HorizontalAlignment.Center
                    }
                }

                // Empty State 2: Không khớp bộ lọc
                Container {
                    visible: bookManager.bookCount === 0 && !((bookManager.categoryFilter === "all" || bookManager.categoryFilter === "") && bookManager.formatFilter === "all" && bookManager.searchQuery === "")
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    leftPadding: bookManager.dp(28)
                    rightPadding: bookManager.dp(28)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    ImageView {
                        horizontalAlignment: HorizontalAlignment.Center
                        preferredWidth: bookManager.dp(76)
                        preferredHeight: bookManager.dp(76)
                        opacity: 0.35
                        imageSource: bookManager.categoryFilter === "favorites" ?
                                     (bookManager.appTheme === "dark" ? "asset:///icons/ic_star.png" : "asset:///icons/ic_star_dark.png") :
                                     (bookManager.appTheme === "dark" ? "asset:///icons/ic_book.png" : "asset:///icons/ic_book_dark.png")
                        scalingMethod: ScalingMethod.AspectFit
                    }

                    Label {
                        topMargin: bookManager.dp(14)
                        text: bookManager.categoryFilter === "favorites" ?
                              (qsTr("Chưa có sách yêu thích") + Retranslate.onLanguageChanged) :
                              (qsTr("Không tìm thấy sách phù hợp") + Retranslate.onLanguageChanged)
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.textAlign: TextAlign.Center
                        textStyle.color: Color.create(bookManager.uiInk)
                        horizontalAlignment: HorizontalAlignment.Center
                        multiline: true
                    }

                    Label {
                        topMargin: bookManager.dp(6)
                        visible: bookManager.categoryFilter === "favorites"
                        text: qsTr("Hãy bấm vào biểu tượng ngôi sao để thêm sách vào mục Yêu Thích.") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.textAlign: TextAlign.Center
                        textStyle.color: Color.create(bookManager.uiMuted)
                        multiline: true
                        horizontalAlignment: HorizontalAlignment.Center
                    }

                    Button {
                        topMargin: bookManager.dp(16)
                        appearance: ControlAppearance.Primary
                        text: qsTr("Đặt lại bộ lọc & tìm kiếm") + Retranslate.onLanguageChanged
                        horizontalAlignment: HorizontalAlignment.Center
                        onClicked: {
                            bookManager.resetFilters();
                            searchInputField.text = "";
                        }
                    }
                }

                // MOSAIC GRID VIEW
                ListView {
                    id: mosaicListView
                    function fwd_openBookWithLoading() { return libraryPage.openBookWithLoading.apply(libraryPage, arguments); }
                    function fwd_showBookDetail() { return libraryPage.showBookDetail.apply(libraryPage, arguments); }
                    function fwd_showToast() { return libraryPage.showToast.apply(libraryPage, arguments); }
                    function fwd_openShelfAssign() { return libraryPage.openShelfAssign.apply(libraryPage, arguments); }

                    visible: bookManager.bookCount > 0 && bookManager.libraryViewMode === "mosaic"
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    dataModel: bookManager.model

                    layout: GridListLayout {
                        columnCount: bookManager.isHiRes ? 4 : 3
                        headerMode: bookManager.groupBy !== "none" ? ListHeaderMode.Sticky : ListHeaderMode.None
                        cellAspectRatio: 0.62
                        horizontalCellSpacing: bookManager.dp(10)
                        verticalCellSpacing: bookManager.dp(12)
                    }

                    listItemComponents: [
                        ListItemComponent {
                            type: "header"
                            Header {
                                title: ListItemData ? ListItemData : ""
                            }
                        },
                        ListItemComponent {
                            type: "item"
                            CustomListItem {
                                id: ctxCell427
                                dividerVisible: false
                                highlightAppearance: HighlightAppearance.Frame

                                contextActions: [
                                    ActionSet {
                                        title: ListItemData.title ? ListItemData.title : ""
                                        subtitle: ListItemData.author ? ListItemData.author : ""

                                        ActionItem {
                                            title: qsTr("Đọc sách") + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_open.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                ctxCell427.ListItem.view.fwd_openBookWithLoading(path);
                                            }
                                        }
                                        ActionItem {
                                            title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_info.png"
                                            onTriggered: { ctxCell427.ListItem.view.fwd_showBookDetail(ListItemData); }
                                        }
                                        ActionItem {
                                            title: (ListItemData.isFavorite ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
                                            imageSource: ListItemData.isFavorite ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                var newState = bookManager.toggleFavorite(path);
                                                ctxCell427.ListItem.view.fwd_showToast(newState ? (qsTr("Đã thêm vào yêu thích: ") + ListItemData.title) : (qsTr("Đã bỏ yêu thích: ") + ListItemData.title));
                                            }
                                        }
                                        ActionItem {
                                            title: (ListItemData.isReadingList ? qsTr("Bỏ khỏi DS đọc") : qsTr("Thêm vào danh sách đọc")) + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_book.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                var newState = bookManager.toggleReadingList(path);
                                                ctxCell427.ListItem.view.fwd_showToast(newState ? (qsTr("Đã thêm vào DS đọc: ") + ListItemData.title) : (qsTr("Đã bỏ khỏi DS đọc: ") + ListItemData.title));
                                            }
                                        }
                                        ActionItem {
                                            title: qsTr("Thêm vào kệ...") + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_library.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                ctxCell427.ListItem.view.fwd_openShelfAssign(path, ListItemData.title);
                                            }
                                        }
                                        ActionItem {
                                            title: (ListItemData.progress >= 100 ? qsTr("Đánh dấu chưa đọc") : qsTr("Đánh dấu đã đọc xong")) + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_done.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                var isCompleted = bookManager.toggleBookCompleted(path);
                                                ctxCell427.ListItem.view.fwd_showToast(isCompleted ? (qsTr("Đã đánh dấu đã đọc xong: ") + ListItemData.title) : (qsTr("Đã đánh dấu chưa đọc: ") + ListItemData.title));
                                            }
                                        }
                                        DeleteActionItem {
                                            title: qsTr("Xóa khỏi thư viện") + Retranslate.onLanguageChanged
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                bookManager.removeBookFromLibrary(path);
                                                ctxCell427.ListItem.view.fwd_showToast(qsTr("Đã xóa khỏi thư viện: ") + ListItemData.title);
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

                                    // Real Cover
                                    ImageView {
                                        visible: ListItemData.cover !== undefined && ListItemData.cover !== ""
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        imageSource: (ListItemData.cover !== undefined && ListItemData.cover !== "") ? ("file://" + ListItemData.cover) : ""
                                        scalingMethod: ScalingMethod.AspectFill
                                    }

                                    // Real Cover Top Left: Finished Badge
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

                                    // Real Cover Top Right: Frosted Format Pill
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
                                            text: ListItemData.formatBadge
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: Color.create("#F5EFE6")
                                        }
                                    }

                                    // Real Cover Bottom: Slim Progress Scrim
                                    Container {
                                        visible: (ListItemData.cover !== undefined && ListItemData.cover !== "") && ListItemData.progress > 0
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Bottom
                                        background: Color.create("#BF0E0C0A")
                                        leftPadding: bookManager.dp(6)
                                        rightPadding: bookManager.dp(6)
                                        topPadding: bookManager.dp(3)
                                        bottomPadding: bookManager.dp(3)
                                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                        Container {
                                            verticalAlignment: VerticalAlignment.Center
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            preferredHeight: bookManager.dp(3)
                                            background: Color.create("#40FFFFFF")
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Container {
                                                verticalAlignment: VerticalAlignment.Fill
                                                background: Color.create("#D4A359")
                                                layoutProperties: StackLayoutProperties {
                                                    spaceQuota: Math.max(0.001, (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                }
                                            }
                                            Container {
                                                verticalAlignment: VerticalAlignment.Fill
                                                background: Color.Transparent
                                                visible: ListItemData.progress < 100
                                                layoutProperties: StackLayoutProperties {
                                                    spaceQuota: Math.max(0.001, 100 - (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                }
                                            }
                                        }

                                        Label {
                                            leftMargin: bookManager.dp(5)
                                            verticalAlignment: VerticalAlignment.Center
                                            text: ListItemData.progress + "%"
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: Color.create("#F5EFE6")
                                        }
                                    }

                                    // Coverless Book Typographic Card (No duplicate progress badges)
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
                                                horizontalAlignment: HorizontalAlignment.Right

                                                Label {
                                                    text: ListItemData.formatBadge
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
                                                text: ListItemData.title
                                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.textAlign: TextAlign.Center
                                                textStyle.color: Color.create(bookManager.uiInk)
                                                multiline: true
                                                autoSize.maxLineCount: 3
                                            }

                                            Label {
                                                topMargin: bookManager.dp(3)
                                                horizontalAlignment: HorizontalAlignment.Center
                                                text: ListItemData.author
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.textAlign: TextAlign.Center
                                                textStyle.color: Color.create(bookManager.uiMuted)
                                                multiline: true
                                                autoSize.maxLineCount: 1
                                            }
                                        }

                                        Label {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: ListItemData.fileSize ? ListItemData.fileSize : ""
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: Color.create(bookManager.uiMuted)
                                        }

                                        Container {
                                            visible: ListItemData.progress > 0
                                            topMargin: bookManager.dp(5)
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Container {
                                                verticalAlignment: VerticalAlignment.Center
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                preferredHeight: bookManager.dp(3)
                                                background: Color.create(bookManager.uiSurface)
                                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                                Container {
                                                    verticalAlignment: VerticalAlignment.Fill
                                                    background: Color.create("#D4A359")
                                                    layoutProperties: StackLayoutProperties {
                                                        spaceQuota: Math.max(0.001, (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                    }
                                                }
                                                Container {
                                                    verticalAlignment: VerticalAlignment.Fill
                                                    background: Color.Transparent
                                                    visible: ListItemData.progress < 100
                                                    layoutProperties: StackLayoutProperties {
                                                        spaceQuota: Math.max(0.001, 100 - (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                    }
                                                }
                                            }

                                            Label {
                                                leftMargin: bookManager.dp(5)
                                                verticalAlignment: VerticalAlignment.Center
                                                text: ListItemData.progress + "%"
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: Color.create(bookManager.uiAccent)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    ]

                    onTriggered: {
                        if (bookManager.groupBy !== "none" && indexPath.length < 2) return;
                        var chosenItem = dataModel.data(indexPath);
                        if (!chosenItem) return;
                        var path = chosenItem.filePath ? chosenItem.filePath : chosenItem.path;
                        if (path) {
                            openBookWithLoading(path);
                        }
                    }
                }

                // DETAILED LIST VIEW (Full-Bleed Native Rows)
                ListView {
                    id: detailedListView
                    function fwd_openBookWithLoading() { return libraryPage.openBookWithLoading.apply(libraryPage, arguments); }
                    function fwd_showBookDetail() { return libraryPage.showBookDetail.apply(libraryPage, arguments); }
                    function fwd_showToast() { return libraryPage.showToast.apply(libraryPage, arguments); }
                    function fwd_openShelfAssign() { return libraryPage.openShelfAssign.apply(libraryPage, arguments); }

                    visible: bookManager.bookCount > 0 && bookManager.libraryViewMode === "detailed"
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    dataModel: bookManager.model

                    layout: StackListLayout {
                        headerMode: bookManager.groupBy !== "none" ? ListHeaderMode.Sticky : ListHeaderMode.None
                    }

                    listItemComponents: [
                        ListItemComponent {
                            type: "header"
                            Header {
                                title: ListItemData ? ListItemData : ""
                            }
                        },
                        ListItemComponent {
                            type: "item"
                            CustomListItem {
                                id: ctxCell819
                                dividerVisible: true
                                highlightAppearance: HighlightAppearance.Frame

                                contextActions: [
                                    ActionSet {
                                        title: ListItemData.title ? ListItemData.title : ""
                                        subtitle: ListItemData.author ? ListItemData.author : ""

                                        ActionItem {
                                            title: qsTr("Đọc sách") + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_open.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                ctxCell819.ListItem.view.fwd_openBookWithLoading(path);
                                            }
                                        }
                                        ActionItem {
                                            title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_info.png"
                                            onTriggered: { ctxCell819.ListItem.view.fwd_showBookDetail(ListItemData); }
                                        }
                                        ActionItem {
                                            title: (ListItemData.isFavorite ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
                                            imageSource: ListItemData.isFavorite ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                var newState = bookManager.toggleFavorite(path);
                                                ctxCell819.ListItem.view.fwd_showToast(newState ? (qsTr("Đã thêm vào yêu thích: ") + ListItemData.title) : (qsTr("Đã bỏ yêu thích: ") + ListItemData.title));
                                            }
                                        }
                                        ActionItem {
                                            title: (ListItemData.isReadingList ? qsTr("Bỏ khỏi DS đọc") : qsTr("Thêm vào danh sách đọc")) + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_book.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                var newState = bookManager.toggleReadingList(path);
                                                ctxCell819.ListItem.view.fwd_showToast(newState ? (qsTr("Đã thêm vào DS đọc: ") + ListItemData.title) : (qsTr("Đã bỏ khỏi DS đọc: ") + ListItemData.title));
                                            }
                                        }
                                        ActionItem {
                                            title: qsTr("Thêm vào kệ...") + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_library.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                ctxCell819.ListItem.view.fwd_openShelfAssign(path, ListItemData.title);
                                            }
                                        }
                                        ActionItem {
                                            title: (ListItemData.progress >= 100 ? qsTr("Đánh dấu chưa đọc") : qsTr("Đánh dấu đã đọc xong")) + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_done.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                var isCompleted = bookManager.toggleBookCompleted(path);
                                                ctxCell819.ListItem.view.fwd_showToast(isCompleted ? (qsTr("Đã đánh dấu đã đọc xong: ") + ListItemData.title) : (qsTr("Đã đánh dấu chưa đọc: ") + ListItemData.title));
                                            }
                                        }
                                        DeleteActionItem {
                                            title: qsTr("Xóa khỏi thư viện") + Retranslate.onLanguageChanged
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                bookManager.removeBookFromLibrary(path);
                                                ctxCell819.ListItem.view.fwd_showToast(qsTr("Đã xóa khỏi thư viện: ") + ListItemData.title);
                                            }
                                        }
                                    }
                                ]

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topPadding: bookManager.dp(12)
                                    bottomPadding: bookManager.dp(12)
                                    leftPadding: bookManager.dp(14)
                                    rightPadding: bookManager.dp(14)
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    // Cover Thumbnail
                                    Container {
                                        preferredWidth: bookManager.dp(72)
                                        preferredHeight: bookManager.dp(104)
                                        minWidth: bookManager.dp(72)
                                        maxWidth: bookManager.dp(72)
                                        minHeight: bookManager.dp(104)
                                        maxHeight: bookManager.dp(104)
                                        verticalAlignment: VerticalAlignment.Center
                                        background: Color.create(bookManager.uiSurface)
                                        layout: DockLayout {}

                                        ImageView {
                                            visible: ListItemData.cover !== undefined && ListItemData.cover !== ""
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            verticalAlignment: VerticalAlignment.Fill
                                            imageSource: (ListItemData.cover !== undefined && ListItemData.cover !== "") ? ("file://" + ListItemData.cover) : ""
                                            scalingMethod: ScalingMethod.AspectFill
                                        }

                                        Container {
                                            visible: ListItemData.cover === undefined || ListItemData.cover === ""
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            verticalAlignment: VerticalAlignment.Fill
                                            background: Color.create(bookManager.uiCard)
                                            layout: DockLayout {}

                                            Label {
                                                horizontalAlignment: HorizontalAlignment.Center
                                                verticalAlignment: VerticalAlignment.Center
                                                text: ListItemData.formatBadge
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: Color.create(bookManager.uiAccent)
                                            }
                                        }
                                    }

                                    // Details column
                                    Container {
                                        leftMargin: bookManager.dp(14)
                                        verticalAlignment: VerticalAlignment.Center
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Label {
                                                verticalAlignment: VerticalAlignment.Top
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                text: ListItemData.title
                                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: Color.create(bookManager.uiInk)
                                                multiline: true
                                                autoSize.maxLineCount: 2
                                            }

                                            Container {
                                                visible: ListItemData.progress >= 100
                                                leftMargin: bookManager.dp(8)
                                                verticalAlignment: VerticalAlignment.Top
                                                background: Color.create("#D4A359")
                                                leftPadding: bookManager.dp(8)
                                                rightPadding: bookManager.dp(8)
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
                                                leftMargin: bookManager.dp(6)
                                                verticalAlignment: VerticalAlignment.Top
                                                background: Color.create(bookManager.uiSurface)
                                                leftPadding: bookManager.dp(8)
                                                rightPadding: bookManager.dp(8)
                                                topPadding: bookManager.dp(2)
                                                bottomPadding: bookManager.dp(2)

                                                Label {
                                                    text: ListItemData.formatBadge
                                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                                    textStyle.fontWeight: FontWeight.Bold
                                                    textStyle.color: Color.create(bookManager.uiInk)
                                                }
                                            }
                                        }

                                        Label {
                                            topMargin: bookManager.dp(2)
                                            text: (ListItemData.author !== "" ? ListItemData.author : (qsTr("Tác giả không rõ") + Retranslate.onLanguageChanged)) + (ListItemData.fileSize ? (" • " + ListItemData.fileSize) : "")
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: Color.create(bookManager.uiMuted)
                                            multiline: true
                                            autoSize.maxLineCount: 1
                                        }

                                        Label {
                                            visible: ListItemData.series ? true : false
                                            topMargin: bookManager.dp(2)
                                            text: (ListItemData.series ? ListItemData.series : "") + (ListItemData.seriesIndex > 0 ? (" #" + ListItemData.seriesIndex) : "")
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: Color.create(bookManager.uiAccent)
                                            autoSize.maxLineCount: 1
                                        }

                                        Container {
                                            topMargin: bookManager.dp(8)
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Container {
                                                verticalAlignment: VerticalAlignment.Center
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                preferredHeight: bookManager.dp(4)
                                                minHeight: bookManager.dp(4)
                                                background: Color.create(bookManager.uiSurface)
                                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                                Container {
                                                    visible: (ListItemData.progress !== undefined && ListItemData.progress > 0)
                                                    verticalAlignment: VerticalAlignment.Fill
                                                    background: Color.create("#D4A359")
                                                    layoutProperties: StackLayoutProperties {
                                                        spaceQuota: Math.max(0.001, (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                    }
                                                }
                                                Container {
                                                    visible: (ListItemData.progress === undefined || ListItemData.progress < 100)
                                                    verticalAlignment: VerticalAlignment.Fill
                                                    background: Color.Transparent
                                                    layoutProperties: StackLayoutProperties {
                                                        spaceQuota: Math.max(0.001, 100 - (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                    }
                                                }
                                            }

                                            Label {
                                                leftMargin: bookManager.dp(10)
                                                verticalAlignment: VerticalAlignment.Center
                                                text: (ListItemData.progress > 0 ? ListItemData.progress : 0) + "%"
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: ListItemData.progress >= 100 ? Color.create(bookManager.uiAccent) : (ListItemData.progress > 0 ? Color.create(bookManager.uiInk) : Color.create(bookManager.uiMuted))
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    ]

                    onTriggered: {
                        if (bookManager.groupBy !== "none" && indexPath.length < 2) return;
                        var chosenItem = dataModel.data(indexPath);
                        if (!chosenItem) return;
                        var path = chosenItem.filePath ? chosenItem.filePath : chosenItem.path;
                        if (path) {
                            openBookWithLoading(path);
                        }
                    }
                }
            }
        }

        // BOOK LOADING OVERLAY
        Container {
            id: bookLoadingOverlay
            visible: bookManager.isLoadingBook
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: bookManager.appTheme === "dark" ? Color.create("#CC0E0C0A") : Color.create("#991C1916")
            touchPropagationMode: TouchPropagationMode.Full
            layout: DockLayout {}

            gestureHandlers: [
                TapHandler {
                    onTapped: {
                        bookManager.stopWatchdog();
                        bookManager.setIsLoadingBook(false);
                    }
                }
            ]

            Container {
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: Math.min(bookManager.screenWidth > 0 ? bookManager.screenWidth - bookManager.dp(48) : 400, bookManager.dp(360))
                background: Color.create(bookManager.uiCard)
                leftPadding: bookManager.dp(24)
                rightPadding: bookManager.dp(24)
                topPadding: bookManager.dp(24)
                bottomPadding: bookManager.dp(24)
                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                ActivityIndicator {
                    id: bookLoadingIndicator
                    horizontalAlignment: HorizontalAlignment.Center
                    visible: bookLoadingOverlay.visible
                    running: bookLoadingOverlay.visible
                    preferredWidth: bookManager.dp(48)
                    preferredHeight: bookManager.dp(48)
                    onVisibleChanged: { if (visible) start(); else stop(); }
                    onRunningChanged: { if (running) start(); else stop(); }
                }

                Label {
                    topMargin: bookManager.dp(14)
                    horizontalAlignment: HorizontalAlignment.Center
                    text: qsTr("Đang mở sách...") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.textAlign: TextAlign.Center
                    textStyle.color: Color.create(bookManager.uiInk)
                }

                Label {
                    topMargin: bookManager.dp(4)
                    horizontalAlignment: HorizontalAlignment.Center
                    text: qsTr("Vui lòng chờ trong giây lát...") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.textAlign: TextAlign.Center
                    textStyle.color: Color.create(bookManager.uiMuted)
                }

                Button {
                    topMargin: bookManager.dp(16)
                    horizontalAlignment: HorizontalAlignment.Center
                    text: qsTr("Hủy") + Retranslate.onLanguageChanged
                    onClicked: {
                        bookManager.stopWatchdog();
                        bookManager.setIsLoadingBook(false);
                    }
                }
            }
        }
    }

    attachedObjects: [
        BookDetailSheet {
            id: bookDetailSheet
            onRequestOpenBook: {
                libraryPage.openBookWithLoading(filePath);
            }
        },
        SystemToast {
            id: libraryToast
            body: ""
        },
        FilterSortSheet {
            id: filterSortSheet
        },
        ShelfManagerSheet {
            id: shelfManagerSheet
        }
    ]

    onCreationCompleted: {
        bookManager.setIsLoadingBook(false);
        bookManager.pageRenderTimeout.connect(function() {
            bookManager.setIsLoadingBook(false);
        });
    }
}
