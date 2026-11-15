import bb.cascades 1.3
import bb.cascades.pickers 1.0
import bb.system 1.2

Page {
    id: libraryPage

    actionBarVisibility: ChromeVisibility.Visible

    // =========================================================================
    // NATIVE BLACKBERRY 10 TOP SWIPE-DOWN APPLICATION MENU (Matching Image 1)
    // =========================================================================
    Menu.definition: MenuDefinition {
        id: appMenu
        objectName: "appMenu"

        helpAction: HelpActionItem {
            title: qsTr("Giới thiệu") + Retranslate.onLanguageChanged
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
                title: qsTr("Tiến Độ") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_chart.png"
                onTriggered: {
                    app.openReadingStats();
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

    // =========================================================================
    // NATIVE ACTION BAR (Bottom bar matching Image 2 with Open action)
    // =========================================================================
    actions: [
        ActionItem {
            title: qsTr("Mở Tệp") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_open.png"
            ActionBar.placement: ActionBarPlacement.OnBar
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
            ActionBar.placement: ActionBarPlacement.OnBar
            shortcuts: [
                Shortcut { key: "K" }
            ]
            onTriggered: {
                opdsSheet.open();
            }
        },
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
            ActionBar.placement: ActionBarPlacement.InOverflow
            shortcuts: [
                Shortcut { key: "V" }
            ]
            onTriggered: {
                bookManager.toggleLibraryViewMode();
            }
        },
        ActionItem {
            title: qsTr("Quét lại") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_reflow.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            onTriggered: {
                bookManager.scanLibrary();
            }
        },
        ActionItem {
            title: qsTr("Thoát") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_exit.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            shortcuts: [
                Shortcut { key: "Q" }
            ]
            onTriggered: {
                app.promptExit();
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

    titleBar: TitleBar {
        title: qsTr("Tủ Sách") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
        acceptAction: ActionItem {
            title: ""
            imageSource: "asset:///icons/ic_search.png"
            onTriggered: {
                libraryPage.isSearching = !libraryPage.isSearching;
                if (!libraryPage.isSearching) {
                    bookManager.setSearchQuery("");
                }
            }
        }
    }

    Container {
        id: rootContainer
        layout: DockLayout {}
        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")

        // Main Page Content Container
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill

            // =========================================================================
            // INLINE LIBRARY SEARCH BAR (Toggled by Search icon)
            // =========================================================================
            Container {
                id: searchBarContainer
                visible: libraryPage.isSearching
                horizontalAlignment: HorizontalAlignment.Fill
                background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F5F5F5")
                topPadding: 8
                bottomPadding: 8
                leftPadding: 12
                rightPadding: 12

                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }

                ImageView {
                    verticalAlignment: VerticalAlignment.Center
                    preferredWidth: 32
                    preferredHeight: 32
                    imageSource: "asset:///icons/ic_search.png"
                }

                TextField {
                    id: searchInputField
                    leftMargin: 8
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1.0
                    }
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

                Button {
                    leftMargin: 8
                    verticalAlignment: VerticalAlignment.Center
                    text: qsTr("Đóng") + Retranslate.onLanguageChanged
                    preferredWidth: 105
                    onClicked: {
                        searchInputField.text = "";
                        bookManager.setSearchQuery("");
                        libraryPage.isSearching = false;
                    }
                }
            }

            // =========================================================================
            // KỆ SÁCH (BOOKSHELF VIEW)
            // =========================================================================
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 1.0
                }

                // =============================================================
                // 1. BOOKSHELF TOOLBAR (Count, View Mode, Filter & Sort)
                // =============================================================
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                    topPadding: 12
                    bottomPadding: 12
                    leftPadding: 16
                    rightPadding: 16

                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    Label {
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                        text: bookManager.searchQuery !== "" ?
                              ((qsTr("Kết quả") + ": " + bookManager.bookCount) + Retranslate.onLanguageChanged) :
                              ((qsTr("Tủ Sách") + " • " + bookManager.bookCount +
                                (bookManager.categoryFilter === "favorites" ? (qsTr(" • Yêu thích") + Retranslate.onLanguageChanged) :
                                 bookManager.formatFilter !== "all" ? (" • " + bookManager.formatFilter.toUpperCase()) : "") +
                                (bookManager.groupBy === "author" ? (qsTr(" • Theo Tác giả") + Retranslate.onLanguageChanged) :
                                 bookManager.groupBy === "series" ? (qsTr(" • Theo Series") + Retranslate.onLanguageChanged) : "")) + Retranslate.onLanguageChanged)
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                    }

                    // View Mode Switcher (Grid / List)
                    Container {
                        id: viewModeToggleBtn
                        verticalAlignment: VerticalAlignment.Center
                        preferredWidth: 104
                        preferredHeight: 72
                        minWidth: 88
                        minHeight: 64
                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        layout: DockLayout {}

                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    var nextMode = bookManager.libraryViewMode === "mosaic" ? "detailed" : "mosaic";
                                    bookManager.setLibraryViewMode(nextMode);
                                }
                            }
                        ]

                        ImageView {
                            horizontalAlignment: HorizontalAlignment.Center
                            verticalAlignment: VerticalAlignment.Center
                            preferredWidth: 52
                            preferredHeight: 52
                            imageSource: bookManager.libraryViewMode === "mosaic" ? "asset:///icons/ic_grid.png" : "asset:///icons/ic_list.png"
                        }
                    }
                }

                // Bookshelf Main Content Area (Mosaic or Detailed)
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1.0
                    }
                    layout: DockLayout {}

                    // Empty state 1: Tủ sách đang trống
                    Container {
                        visible: bookManager.bookCount === 0 && bookManager.categoryFilter === "all" && bookManager.formatFilter === "all" && bookManager.searchQuery === ""
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center
                        leftPadding: 40
                        rightPadding: 40

                        ImageView {
                            horizontalAlignment: HorizontalAlignment.Center
                            preferredWidth: 240
                            preferredHeight: 240
                            opacity: 0.35
                            imageSource: "asset:///icons/ic_book.png"
                            scalingMethod: ScalingMethod.AspectFit
                        }

                        Label {
                            topMargin: 24
                            text: qsTr("Tủ sách đang trống") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.BigText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            horizontalAlignment: HorizontalAlignment.Center
                        }

                        Label {
                            topMargin: 10
                            text: qsTr("Tủ sách của bạn đang trống. Hãy mở tệp hoặc duyệt thư mục để bắt đầu đọc sách!") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                            multiline: true
                            horizontalAlignment: HorizontalAlignment.Center
                        }
                    }

                    // Empty state 2: Filtered out
                    Container {
                        visible: bookManager.bookCount === 0 && (bookManager.categoryFilter !== "all" || bookManager.formatFilter !== "all" || bookManager.searchQuery !== "")
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center
                        leftPadding: 40
                        rightPadding: 40

                        ImageView {
                            horizontalAlignment: HorizontalAlignment.Center
                            preferredWidth: 120
                            preferredHeight: 120
                            opacity: 0.25
                            imageSource: bookManager.categoryFilter === "favorites" ? "asset:///icons/ic_star.png" : "asset:///icons/ic_book.png"
                            scalingMethod: ScalingMethod.AspectFit
                        }

                        Label {
                            topMargin: 16
                            text: bookManager.categoryFilter === "favorites" ?
                                  (qsTr("Chưa có sách yêu thích") + Retranslate.onLanguageChanged) :
                                  (qsTr("Không tìm thấy sách phù hợp") + Retranslate.onLanguageChanged)
                            textStyle.base: SystemDefaults.TextStyles.BigText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            horizontalAlignment: HorizontalAlignment.Center
                        }

                        Label {
                            topMargin: 8
                            visible: bookManager.categoryFilter === "favorites"
                            text: qsTr("Hãy bấm vào biểu tượng ngôi sao để thêm sách vào mục Yêu Thích.") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                            multiline: true
                            horizontalAlignment: HorizontalAlignment.Center
                        }

                        Button {
                            topMargin: 16
                            text: qsTr("Đặt lại bộ lọc & tìm kiếm") + Retranslate.onLanguageChanged
                            horizontalAlignment: HorizontalAlignment.Center
                            onClicked: {
                                bookManager.resetFilters();
                                searchInputField.text = "";
                            }
                        }
                    }

                    // MOSAIC VIEW (Grid Layout with Covers)
                    ListView {
                        id: mosaicListView
                        visible: bookManager.bookCount > 0 && bookManager.libraryViewMode === "mosaic"
                        horizontalAlignment: HorizontalAlignment.Fill
                        verticalAlignment: VerticalAlignment.Fill
                        dataModel: bookManager.model

                        layout: GridListLayout {
                            columnCount: bookManager.screenWidth > 1000 ? 4 : 3
                            headerMode: bookManager.groupBy !== "none" ? ListHeaderMode.Sticky : ListHeaderMode.None
                            cellAspectRatio: 0.62
                            horizontalCellSpacing: 8
                            verticalCellSpacing: 10
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
                                                    libraryPage.openBookWithLoading(path);
                                                }
                                            }

                                            ActionItem {
                                                title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
                                                imageSource: "asset:///icons/ic_info.png"
                                                onTriggered: {
                                                    libraryPage.showBookDetail(ListItemData);
                                                }
                                            }

                                            ActionItem {
                                                title: (ListItemData.isFavorite ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
                                                imageSource: ListItemData.isFavorite ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
                                                onTriggered: {
                                                    var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                    var newState = bookManager.toggleFavorite(path);
                                                    libraryPage.showToast(newState ? (qsTr("Đã thêm vào yêu thích: ") + ListItemData.title) : (qsTr("Đã bỏ yêu thích: ") + ListItemData.title));
                                                }
                                            }

                                            ActionItem {
                                                title: (ListItemData.isReadingList ? qsTr("Bỏ khỏi DS đọc") : qsTr("Thêm vào danh sách đọc")) + Retranslate.onLanguageChanged
                                                imageSource: "asset:///icons/ic_book.png"
                                                onTriggered: {
                                                    var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                    var newState = bookManager.toggleReadingList(path);
                                                    libraryPage.showToast(newState ? (qsTr("Đã thêm vào DS đọc: ") + ListItemData.title) : (qsTr("Đã bỏ khỏi DS đọc: ") + ListItemData.title));
                                                }
                                            }

                                            ActionItem {
                                                title: (ListItemData.progress >= 100 ? qsTr("Đánh dấu chưa đọc") : qsTr("Đánh dấu đã đọc xong")) + Retranslate.onLanguageChanged
                                                imageSource: "asset:///icons/ic_chart.png"
                                                onTriggered: {
                                                    var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                    var isCompleted = bookManager.toggleBookCompleted(path);
                                                    libraryPage.showToast(isCompleted ? (qsTr("Đã đánh dấu đã đọc xong: ") + ListItemData.title) : (qsTr("Đã đánh dấu chưa đọc: ") + ListItemData.title));
                                                }
                                            }

                                            DeleteActionItem {
                                                title: qsTr("Xóa khỏi thư viện") + Retranslate.onLanguageChanged
                                                onTriggered: {
                                                    var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                    bookManager.removeBookFromLibrary(path);
                                                    libraryPage.showToast(qsTr("Đã xóa khỏi thư viện: ") + ListItemData.title);
                                                }
                                            }
                                        }
                                    ]

                                    // Outer frame/border to clearly define card in dark & light mode
                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                        topPadding: 1
                                        bottomPadding: 1
                                        leftPadding: 1
                                        rightPadding: 1
                                        layout: DockLayout {}

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            verticalAlignment: VerticalAlignment.Fill
                                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                                            layout: DockLayout {}

                                            // Real Cover
                                            ImageView {
                                                visible: ListItemData.cover !== undefined && ListItemData.cover !== ""
                                                horizontalAlignment: HorizontalAlignment.Fill
                                                verticalAlignment: VerticalAlignment.Fill
                                                imageSource: (ListItemData.cover !== undefined && ListItemData.cover !== "") ? ("file://" + ListItemData.cover) : ""
                                                scalingMethod: ScalingMethod.AspectFill
                                            }

                                            // Real Cover Overlay: Favorite / Reading List & Finished badges positioned to the top left
                                            Container {
                                                visible: (ListItemData.cover !== undefined && ListItemData.cover !== "") && (ListItemData.isFavorite || ListItemData.isReadingList || ListItemData.progress >= 100)
                                                horizontalAlignment: HorizontalAlignment.Left
                                                verticalAlignment: VerticalAlignment.Top
                                                topMargin: 4
                                                leftMargin: 4

                                                layout: StackLayout {
                                                    orientation: LayoutOrientation.LeftToRight
                                                }

                                                Container {
                                                    visible: false
                                                    background: Color.create("#CC000000")
                                                    leftPadding: 6
                                                    rightPadding: 6
                                                    topPadding: 2
                                                    bottomPadding: 2
                                                    verticalAlignment: VerticalAlignment.Center

                                                    layout: StackLayout {
                                                        orientation: LayoutOrientation.LeftToRight
                                                    }

                                                    Label {
                                                        visible: ListItemData.isFavorite === true
                                                        text: ""
                                                        textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                        textStyle.fontWeight: FontWeight.Bold
                                                        textStyle.color: Color.create("#FCC419")
                                                    }

                                                    Label {
                                                        visible: ListItemData.isReadingList === true
                                                        leftMargin: ListItemData.isFavorite ? 4 : 0
                                                        text: ""
                                                        textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                        textStyle.fontWeight: FontWeight.Bold
                                                        textStyle.color: Color.create("#10B981")
                                                    }
                                                }

                                                // Finished badge
                                                Container {
                                                    visible: ListItemData.progress >= 100
                                                    leftMargin: (ListItemData.isFavorite || ListItemData.isReadingList) ? 4 : 0
                                                    background: Color.create("#10B981")
                                                    leftPadding: 6
                                                    rightPadding: 6
                                                    topPadding: 2
                                                    bottomPadding: 2
                                                    verticalAlignment: VerticalAlignment.Center

                                                    Label {
                                                        text: qsTr("Hoàn thành") + Retranslate.onLanguageChanged
                                                        textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                        textStyle.fontWeight: FontWeight.Bold
                                                        textStyle.color: Color.White
                                                    }
                                                }
                                            }

                                            // Real Cover Overlay: Format badge positioned to the far right
                                            Container {
                                                visible: ListItemData.cover !== undefined && ListItemData.cover !== ""
                                                horizontalAlignment: HorizontalAlignment.Right
                                                verticalAlignment: VerticalAlignment.Top
                                                topMargin: 4
                                                rightMargin: 4
                                                background: Color.create("#99000000")
                                                leftPadding: 6
                                                rightPadding: 6
                                                topPadding: 2
                                                bottomPadding: 2

                                                Label {
                                                    text: ListItemData.formatBadge
                                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                    textStyle.fontWeight: FontWeight.Bold
                                                    textStyle.color: ListItemData.format === "pdf" ? Color.create("#FF6B6B") :
                                                                    ListItemData.format === "epub" ? Color.create("#51CF66") :
                                                                    ListItemData.format === "fb2" ? Color.create("#CC5DE8") :
                                                                    ListItemData.format === "djvu" ? Color.create("#339AF0") :
                                                                    Color.create("#FCC419")
                                                }
                                            }

                                            // Real Cover Overlay: Progress bar and % on the right
                                            Container {
                                                visible: (ListItemData.cover !== undefined && ListItemData.cover !== "") && ListItemData.progress > 0
                                                horizontalAlignment: HorizontalAlignment.Fill
                                                verticalAlignment: VerticalAlignment.Bottom
                                                background: Color.create("#B3000000")
                                                leftPadding: 6
                                                rightPadding: 6
                                                topPadding: 3
                                                bottomPadding: 3

                                                layout: StackLayout {
                                                    orientation: LayoutOrientation.LeftToRight
                                                }

                                                Container {
                                                    verticalAlignment: VerticalAlignment.Center
                                                    layoutProperties: StackLayoutProperties {
                                                        spaceQuota: 1.0
                                                    }
                                                    preferredHeight: 3
                                                    background: Color.create("#40FFFFFF")
                                                    layout: StackLayout {
                                                        orientation: LayoutOrientation.LeftToRight
                                                    }
                                                    Container {
                                                        verticalAlignment: VerticalAlignment.Fill
                                                        background: ListItemData.progress >= 100 ? Color.create("#059669") : Color.create("#10B981")
                                                        layoutProperties: StackLayoutProperties {
                                                            spaceQuota: Math.max(0.001, (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                        }
                                                    }
                                                    Container {
                                                        verticalAlignment: VerticalAlignment.Fill
                                                        background: Color.Transparent
                                                        layoutProperties: StackLayoutProperties {
                                                            spaceQuota: Math.max(0.001, 100 - (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                        }
                                                    }
                                                }

                                                Label {
                                                    leftMargin: 4
                                                    verticalAlignment: VerticalAlignment.Center
                                                    text: ListItemData.progress + "%"
                                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                    textStyle.fontWeight: FontWeight.Bold
                                                    textStyle.color: ListItemData.progress >= 100 ? Color.create("#51CF66") : Color.White
                                                }
                                            }

                                            // Book card fallback
                                            Container {
                                                visible: ListItemData.cover === undefined || ListItemData.cover === ""
                                                horizontalAlignment: HorizontalAlignment.Fill
                                                verticalAlignment: VerticalAlignment.Fill
                                                background: bookManager.appTheme === "dark" ? Color.create("#141414") : Color.create("#F5F5F5")
                                                leftPadding: 8
                                                rightPadding: 8
                                                topPadding: 8
                                                bottomPadding: 8

                                                layout: StackLayout {
                                                    orientation: LayoutOrientation.TopToBottom
                                                }

                                                // Header row: Badges (Favorite & Reading list) on left, Format badge on right
                                                Container {
                                                    horizontalAlignment: HorizontalAlignment.Fill
                                                    layout: StackLayout {
                                                        orientation: LayoutOrientation.LeftToRight
                                                    }

                                                    Container {
                                                        visible: false
                                                        background: Color.create("#30000000")
                                                        leftPadding: 6
                                                        rightPadding: 6
                                                        topPadding: 2
                                                        bottomPadding: 2
                                                        verticalAlignment: VerticalAlignment.Center

                                                        layout: StackLayout {
                                                            orientation: LayoutOrientation.LeftToRight
                                                        }

                                                        Label {
                                                            visible: ListItemData.isFavorite === true
                                                            text: ""
                                                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                            textStyle.fontWeight: FontWeight.Bold
                                                            textStyle.color: Color.create("#FCC419")
                                                        }

                                                        Label {
                                                            visible: ListItemData.isReadingList === true
                                                            leftMargin: ListItemData.isFavorite ? 4 : 0
                                                            text: ""
                                                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                            textStyle.fontWeight: FontWeight.Bold
                                                            textStyle.color: Color.create("#10B981")
                                                        }
                                                    }

                                                    // Finished badge
                                                    Container {
                                                        visible: ListItemData.progress >= 100
                                                        leftMargin: (ListItemData.isFavorite || ListItemData.isReadingList) ? 4 : 0
                                                        background: Color.create("#10B981")
                                                        leftPadding: 6
                                                        rightPadding: 6
                                                        topPadding: 2
                                                        bottomPadding: 2
                                                        verticalAlignment: VerticalAlignment.Center

                                                        Label {
                                                            text: qsTr("Hoàn thành") + Retranslate.onLanguageChanged
                                                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                            textStyle.fontWeight: FontWeight.Bold
                                                            textStyle.color: Color.White
                                                        }
                                                    }

                                                    Container {
                                                        layoutProperties: StackLayoutProperties {
                                                            spaceQuota: 1.0
                                                        }
                                                    }

                                                    // Định dạng sách để hết sang bên phải của thẻ sách
                                                    Container {
                                                        background: Color.create("#30000000")
                                                        leftPadding: 6
                                                        rightPadding: 6
                                                        topPadding: 2
                                                        bottomPadding: 2
                                                        horizontalAlignment: HorizontalAlignment.Right

                                                        Label {
                                                            text: ListItemData.formatBadge
                                                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                            textStyle.fontWeight: FontWeight.Bold
                                                            textStyle.color: ListItemData.format === "pdf" ? Color.create("#FF6B6B") :
                                                                            ListItemData.format === "epub" ? Color.create("#51CF66") :
                                                                            ListItemData.format === "fb2" ? Color.create("#CC5DE8") :
                                                                            ListItemData.format === "djvu" ? Color.create("#339AF0") :
                                                                            Color.create("#FCC419")
                                                        }
                                                    }
                                                }

                                                // Khu vực giữa: Tên sách (lớn nhất) > Tên tác giả (lớn hơn dòng dưới)
                                                Container {
                                                    verticalAlignment: VerticalAlignment.Center
                                                    horizontalAlignment: HorizontalAlignment.Fill
                                                    topMargin: 4
                                                    layoutProperties: StackLayoutProperties {
                                                        spaceQuota: 1.0
                                                    }

                                                    // Tên sách: Cỡ chữ lớn nhất trong thẻ (không in đậm)
                                                    Label {
                                                        horizontalAlignment: HorizontalAlignment.Center
                                                        text: ListItemData.title
                                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                                        multiline: true
                                                        autoSize.maxLineCount: 3
                                                    }

                                                    // Tên tác giả: Cỡ chữ vừa (lớn hơn định dạng/dung lượng/trạng thái/ngày)
                                                    Label {
                                                        topMargin: 2
                                                        horizontalAlignment: HorizontalAlignment.Center
                                                        text: ListItemData.author
                                                        textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                        textStyle.fontWeight: FontWeight.Medium
                                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                                        multiline: true
                                                        autoSize.maxLineCount: 1
                                                    }
                                                }

                                                // Dung lượng và trạng thái và ngày modified: Cỡ chữ nhỏ nhất
                                                Label {
                                                    horizontalAlignment: HorizontalAlignment.Center
                                                    text: ListItemData.fileSize + " • " + (ListItemData.progress >= 100 ? (qsTr("Hoàn thành") + Retranslate.onLanguageChanged) : (ListItemData.progress > 0 ? ((qsTr("Đã đọc ") + Retranslate.onLanguageChanged) + ListItemData.progress + "%") : (qsTr("Chưa đọc") + Retranslate.onLanguageChanged)))
                                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                    textStyle.color: ListItemData.progress >= 100 ? Color.create("#10B981") : (bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#777777"))
                                                }

                                                // Thanh tiến độ & cái progress % để sang bên phải cạnh với thanh tiến độ
                                                Container {
                                                    topMargin: 4
                                                    horizontalAlignment: HorizontalAlignment.Fill
                                                    layout: StackLayout {
                                                        orientation: LayoutOrientation.LeftToRight
                                                    }

                                                    // Thanh tiến độ
                                                    Container {
                                                        verticalAlignment: VerticalAlignment.Center
                                                        layoutProperties: StackLayoutProperties {
                                                            spaceQuota: 1.0
                                                        }
                                                        preferredHeight: 3
                                                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                                        layout: StackLayout {
                                                            orientation: LayoutOrientation.LeftToRight
                                                        }

                                                        Container {
                                                            verticalAlignment: VerticalAlignment.Fill
                                                            background: ListItemData.progress >= 100 ? Color.create("#059669") : Color.create("#10B981")
                                                            layoutProperties: StackLayoutProperties {
                                                                spaceQuota: Math.max(0.001, (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                            }
                                                        }
                                                        Container {
                                                            verticalAlignment: VerticalAlignment.Fill
                                                            background: Color.Transparent
                                                            layoutProperties: StackLayoutProperties {
                                                                spaceQuota: Math.max(0.001, 100 - (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                            }
                                                        }
                                                    }

                                                    // Progress % để sang bên phải cạnh với thanh tiến độ
                                                    Label {
                                                        leftMargin: 4
                                                        verticalAlignment: VerticalAlignment.Center
                                                        text: (ListItemData.progress > 0 ? ListItemData.progress : 0) + "%"
                                                        textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                        textStyle.fontWeight: FontWeight.Bold
                                                        textStyle.color: ListItemData.progress >= 100 ? Color.create("#51CF66") : (bookManager.appTheme === "dark" ? Color.White : Color.Black)
                                                    }
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

                    // DETAILED VIEW (List Layout)
                    ListView {
                        id: detailedListView
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
                                                    libraryPage.openBookWithLoading(path);
                                                }
                                            }

                                            ActionItem {
                                                title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
                                                imageSource: "asset:///icons/ic_info.png"
                                                onTriggered: {
                                                    libraryPage.showBookDetail(ListItemData);
                                                }
                                            }

                                            ActionItem {
                                                title: (ListItemData.isFavorite ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
                                                imageSource: ListItemData.isFavorite ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
                                                onTriggered: {
                                                    var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                    var newState = bookManager.toggleFavorite(path);
                                                    libraryPage.showToast(newState ? (qsTr("Đã thêm vào yêu thích: ") + ListItemData.title) : (qsTr("Đã bỏ yêu thích: ") + ListItemData.title));
                                                }
                                            }

                                            ActionItem {
                                                title: (ListItemData.isReadingList ? qsTr("Bỏ khỏi DS đọc") : qsTr("Thêm vào danh sách đọc")) + Retranslate.onLanguageChanged
                                                imageSource: "asset:///icons/ic_book.png"
                                                onTriggered: {
                                                    var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                    var newState = bookManager.toggleReadingList(path);
                                                    libraryPage.showToast(newState ? (qsTr("Đã thêm vào DS đọc: ") + ListItemData.title) : (qsTr("Đã bỏ khỏi DS đọc: ") + ListItemData.title));
                                                }
                                            }

                                            ActionItem {
                                                title: (ListItemData.progress >= 100 ? qsTr("Đánh dấu chưa đọc") : qsTr("Đánh dấu đã đọc xong")) + Retranslate.onLanguageChanged
                                                imageSource: "asset:///icons/ic_chart.png"
                                                onTriggered: {
                                                    var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                    var isCompleted = bookManager.toggleBookCompleted(path);
                                                    libraryPage.showToast(isCompleted ? (qsTr("Đã đánh dấu đã đọc xong: ") + ListItemData.title) : (qsTr("Đã đánh dấu chưa đọc: ") + ListItemData.title));
                                                }
                                            }

                                            DeleteActionItem {
                                                title: qsTr("Xóa khỏi thư viện") + Retranslate.onLanguageChanged
                                                onTriggered: {
                                                    var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                    bookManager.removeBookFromLibrary(path);
                                                    libraryPage.showToast(qsTr("Đã xóa khỏi thư viện: ") + ListItemData.title);
                                                }
                                            }
                                        }
                                    ]

                                    // Outer Card Frame with outline border matching illustration
                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        topMargin: 4
                                        bottomMargin: 4
                                        leftMargin: 8
                                        rightMargin: 8
                                        preferredHeight: 180
                                        minHeight: 180
                                        maxHeight: 180
                                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                        topPadding: 1
                                        bottomPadding: 1
                                        leftPadding: 1
                                        rightPadding: 1

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            verticalAlignment: VerticalAlignment.Fill
                                            preferredHeight: 178
                                            maxHeight: 178
                                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                                            topPadding: 8
                                            bottomPadding: 8
                                            leftPadding: 10
                                            rightPadding: 10
                                            layout: StackLayout {
                                                orientation: LayoutOrientation.LeftToRight
                                            }

                                            // Cover thumbnail (Big cover with border matching illustration)
                                            Container {
                                                preferredWidth: 114
                                                preferredHeight: 160
                                                minWidth: 114
                                                maxWidth: 114
                                                minHeight: 160
                                                maxHeight: 160
                                                verticalAlignment: VerticalAlignment.Center
                                                background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                                topPadding: 1
                                                bottomPadding: 1
                                                leftPadding: 1
                                                rightPadding: 1
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
                                                    background: bookManager.appTheme === "dark" ? Color.create("#141414") : Color.create("#F5F5F5")
                                                    layout: DockLayout {}

                                                    Label {
                                                        horizontalAlignment: HorizontalAlignment.Center
                                                        verticalAlignment: VerticalAlignment.Center
                                                        text: ListItemData.formatBadge
                                                        textStyle.base: SystemDefaults.TextStyles.MediumText
                                                        textStyle.fontWeight: FontWeight.Bold
                                                        textStyle.color: ListItemData.format === "pdf" ? Color.create("#FF6B6B") :
                                                                        ListItemData.format === "epub" ? Color.create("#51CF66") :
                                                                        ListItemData.format === "fb2" ? Color.create("#CC5DE8") :
                                                                        ListItemData.format === "djvu" ? Color.create("#339AF0") :
                                                                        Color.create("#FCC419")
                                                    }
                                                }

                                                // Top-Left Badges (Favorite / Reading list)
                                                Container {
                                                    visible: false
                                                    horizontalAlignment: HorizontalAlignment.Left
                                                    verticalAlignment: VerticalAlignment.Top
                                                    topMargin: 2
                                                    leftMargin: 2
                                                    background: Color.create("#CC000000")
                                                    leftPadding: 4
                                                    rightPadding: 4
                                                    topPadding: 2
                                                    bottomPadding: 2

                                                    layout: StackLayout {
                                                        orientation: LayoutOrientation.LeftToRight
                                                    }

                                                    Label {
                                                        visible: ListItemData.isFavorite === true
                                                        text: ""
                                                        textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                        textStyle.fontWeight: FontWeight.Bold
                                                        textStyle.color: Color.create("#FCC419")
                                                    }

                                                    Label {
                                                        visible: ListItemData.isReadingList === true
                                                        leftMargin: ListItemData.isFavorite ? 4 : 0
                                                        text: ""
                                                        textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                        textStyle.fontWeight: FontWeight.Bold
                                                        textStyle.color: Color.create("#10B981")
                                                    }
                                                }
                                            }

                                            // Details column
                                            Container {
                                                leftMargin: 12
                                                verticalAlignment: VerticalAlignment.Fill
                                                layoutProperties: StackLayoutProperties {
                                                    spaceQuota: 1.0
                                                }

                                                // Top row: Tên sách (Name) bên trái + Định dạng (epub, pdf,...) bên phải
                                                Container {
                                                    horizontalAlignment: HorizontalAlignment.Fill
                                                    layout: StackLayout {
                                                        orientation: LayoutOrientation.LeftToRight
                                                    }

                                                    // Tên sách: Name
                                                    Label {
                                                        verticalAlignment: VerticalAlignment.Top
                                                        layoutProperties: StackLayoutProperties {
                                                            spaceQuota: 1.0
                                                        }
                                                        text: ListItemData.title
                                                        textStyle.base: SystemDefaults.TextStyles.TitleText
                                                        textStyle.fontWeight: FontWeight.Bold
                                                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                                        multiline: true
                                                        autoSize.maxLineCount: 2
                                                    }

                                                    // Finished badge
                                                    Container {
                                                        visible: ListItemData.progress >= 100
                                                        leftMargin: 8
                                                        verticalAlignment: VerticalAlignment.Top
                                                        background: Color.create("#10B981")
                                                        leftPadding: 8
                                                        rightPadding: 8
                                                        topPadding: 3
                                                        bottomPadding: 3

                                                        Label {
                                                            text: qsTr("Hoàn thành") + Retranslate.onLanguageChanged
                                                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                            textStyle.fontWeight: FontWeight.Bold
                                                            textStyle.color: Color.White
                                                        }
                                                    }

                                                    // Định dạng sách: epub, pdf,...
                                                    Container {
                                                        leftMargin: 8
                                                        verticalAlignment: VerticalAlignment.Top
                                                        background: ListItemData.format === "pdf" ? Color.create("#40FF6B6B") :
                                                                    ListItemData.format === "epub" ? Color.create("#4051CF66") :
                                                                    ListItemData.format === "fb2" ? Color.create("#40CC5DE8") :
                                                                    ListItemData.format === "djvu" ? Color.create("#40339AF0") :
                                                                    Color.create("#40FCC419")
                                                        leftPadding: 8
                                                        rightPadding: 8
                                                        topPadding: 3
                                                        bottomPadding: 3

                                                        Label {
                                                            text: ListItemData.formatBadge
                                                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                            textStyle.fontWeight: FontWeight.Bold
                                                            textStyle.color: ListItemData.format === "pdf" ? Color.create("#FF6B6B") :
                                                                            ListItemData.format === "epub" ? Color.create("#51CF66") :
                                                                            ListItemData.format === "fb2" ? Color.create("#CC5DE8") :
                                                                            ListItemData.format === "djvu" ? Color.create("#339AF0") :
                                                                            Color.create("#FCC419")
                                                        }
                                                    }
                                                }

                                                // Tác giả: Author
                                                Label {
                                                    topMargin: 2
                                                    text: ListItemData.author !== "" ? ListItemData.author : (qsTr("Tác giả không rõ") + Retranslate.onLanguageChanged)
                                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                                    multiline: true
                                                    autoSize.maxLineCount: 1
                                                }

                                                // Bộ sách: Series
                                                Label {
                                                    visible: ListItemData.series ? true : false
                                                    topMargin: 1
                                                    text: (ListItemData.series ? ListItemData.series : "") + (ListItemData.seriesIndex > 0 ? (" #" + ListItemData.seriesIndex) : "")
                                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                                    multiline: false
                                                    autoSize.maxLineCount: 1
                                                }

                                                // Spacer đẩy thanh tiến độ xuống sát đáy
                                                Container {
                                                    layoutProperties: StackLayoutProperties {
                                                        spaceQuota: 1.0
                                                    }
                                                }

                                                // Thanh tiến độ & phần trăm tiến độ (a %)
                                                Container {
                                                    horizontalAlignment: HorizontalAlignment.Fill
                                                    layout: StackLayout {
                                                        orientation: LayoutOrientation.LeftToRight
                                                    }

                                                    // Thanh tiến độ có viền bao quanh như minh họa
                                                    Container {
                                                        verticalAlignment: VerticalAlignment.Center
                                                        layoutProperties: StackLayoutProperties {
                                                            spaceQuota: 1.0
                                                        }
                                                        preferredHeight: 12
                                                        minHeight: 12
                                                        maxHeight: 12
                                                        background: bookManager.appTheme === "dark" ? Color.create("#374151") : Color.create("#94A3B8")
                                                        topPadding: 1
                                                        bottomPadding: 1
                                                        leftPadding: 1
                                                        rightPadding: 1

                                                        Container {
                                                            horizontalAlignment: HorizontalAlignment.Fill
                                                            verticalAlignment: VerticalAlignment.Fill
                                                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                                            layout: StackLayout {
                                                                orientation: LayoutOrientation.LeftToRight
                                                            }

                                                            Container {
                                                                verticalAlignment: VerticalAlignment.Fill
                                                                background: ListItemData.progress >= 100 ? Color.create("#059669") : Color.create("#10B981")
                                                                layoutProperties: StackLayoutProperties {
                                                                    spaceQuota: Math.max(0.001, (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                                }
                                                            }
                                                            Container {
                                                                verticalAlignment: VerticalAlignment.Fill
                                                                background: Color.Transparent
                                                                layoutProperties: StackLayoutProperties {
                                                                    spaceQuota: Math.max(0.001, 100 - (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                                }
                                                            }
                                                        }
                                                    }

                                                    // a %
                                                    Label {
                                                        leftMargin: 10
                                                        verticalAlignment: VerticalAlignment.Center
                                                        text: (ListItemData.progress > 0 ? ListItemData.progress : 0) + " %"
                                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                                        textStyle.fontWeight: FontWeight.Bold
                                                        textStyle.color: ListItemData.progress >= 100 ? Color.create("#51CF66") : (ListItemData.progress > 0 ? (bookManager.appTheme === "dark" ? Color.White : Color.Black) : (bookManager.appTheme === "dark" ? Color.create("#667788") : Color.create("#94A3B8")))
                                                    }
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

        }

        // =========================================================================
        // BOOK LOADING OVERLAY (Waiting for book to render)
        // Tap anywhere on overlay or dialog to dismiss/cancel!
        // =========================================================================
        Container {
            id: bookLoadingOverlay
            visible: bookManager.isLoadingBook
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: bookManager.appTheme === "dark" ? Color.create("#D0000000") : Color.create("#A0000000")
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
            onTouch: {
                if (event.isUp()) {
                    bookManager.stopWatchdog();
                    bookManager.setIsLoadingBook(false);
                }
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: 420
                background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                leftPadding: 24
                rightPadding: 24
                topPadding: 24
                bottomPadding: 24

                gestureHandlers: [
                    TapHandler {
                        onTapped: {
                            bookManager.stopWatchdog();
                            bookManager.setIsLoadingBook(false);
                        }
                    }
                ]
                onTouch: {
                    if (event.isUp()) {
                        bookManager.stopWatchdog();
                        bookManager.setIsLoadingBook(false);
                    }
                }

                ImageView {
                    horizontalAlignment: HorizontalAlignment.Center
                    preferredWidth: 64
                    preferredHeight: 64
                    imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_app_dark.png" : "asset:///icons/ic_app_light.png"
                    scalingMethod: ScalingMethod.AspectFit
                }

                ActivityIndicator {
                    id: bookLoadingIndicator
                    topMargin: 16
                    horizontalAlignment: HorizontalAlignment.Center
                    visible: bookLoadingOverlay.visible
                    running: bookLoadingOverlay.visible
                    preferredWidth: 50
                    preferredHeight: 50
                    onVisibleChanged: {
                        if (visible) start(); else stop();
                    }
                    onRunningChanged: {
                        if (running) start(); else stop();
                    }
                }

                Label {
                    topMargin: 14
                    horizontalAlignment: HorizontalAlignment.Center
                    text: qsTr("Đang mở sách...") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                }

                Label {
                    topMargin: 4
                    horizontalAlignment: HorizontalAlignment.Center
                    text: qsTr("Vui lòng chờ trong giây lát...") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                }

                Label {
                    topMargin: 10
                    horizontalAlignment: HorizontalAlignment.Center
                    text: qsTr("Chạm màn hình để hủy") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                }

                Button {
                    topMargin: 12
                    horizontalAlignment: HorizontalAlignment.Center
                    preferredWidth: 140
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
        OpdsSheet {
            id: opdsSheet
        }
    ]

    onCreationCompleted: {
        bookManager.setIsLoadingBook(false);
        bookManager.pageRenderTimeout.connect(function() {
            bookManager.setIsLoadingBook(false);
        });
    }
}
