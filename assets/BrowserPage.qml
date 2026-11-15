import bb.cascades 1.3
import bb.system 1.2

Page {
    id: browserPage

    actionBarVisibility: ChromeVisibility.Visible

    property bool isSearchAndFilterVisible: false

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

    titleBar: TitleBar {
        title: (bookManager.canGoUp ? qsTr(bookManager.currentBrowserTitle) : qsTr("Duyệt Thư Mục")) + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
        dismissAction: bookManager.canGoUp ? upAction : null
    }

    Menu.definition: MenuDefinition {
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
            id: searchAction
            title: qsTr("Tìm kiếm") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_search.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            shortcuts: [
                Shortcut { key: "S" }
            ]
            onTriggered: {
                browserPage.isSearchAndFilterVisible = !browserPage.isSearchAndFilterVisible;
                if (!browserPage.isSearchAndFilterVisible) {
                    bookManager.clearBrowserFilter();
                    browserSearchInputField.text = "";
                }
            }
        },
        ActionItem {
            title: bookManager.libraryViewMode === "mosaic" ?
                   (qsTr("Chi tiết") + Retranslate.onLanguageChanged) :
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
            imageSource: "asset:///icons/ic_reflow.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            shortcuts: [
                Shortcut { key: "R" }
            ]
            onTriggered: {
                bookManager.refreshBrowser();
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

    Container {
        layout: DockLayout {}
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill

            // =================================================================
            // 1. COLLAPSIBLE SEARCH TOOLBAR
            // =================================================================
            Container {
                id: searchAndFilterContainer
                visible: browserPage.isSearchAndFilterVisible
                horizontalAlignment: HorizontalAlignment.Fill
                background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F5F5F5")
                topPadding: 10
                bottomPadding: 10
                leftPadding: 14
                rightPadding: 14

                // Search Field + Submit Icon Button
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    TextField {
                        id: browserSearchInputField
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

                    Button {
                        leftMargin: 8
                        verticalAlignment: VerticalAlignment.Center
                        preferredWidth: 68
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_search.png" : "asset:///icons/ic_search_dark.png"
                        onClicked: {
                            browserSearchInputField.loseFocus();
                            bookManager.setBrowserSearchQuery(browserSearchInputField.text);
                        }
                    }
                }
            }

            // =================================================================
            // 2. CURRENT PATH BREADCRUMB BANNER
            // =================================================================
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: bookManager.appTheme === "dark" ? Color.create("#141414") : Color.create("#F5F5F5")
                topPadding: 8
                bottomPadding: 8
                leftPadding: 16
                rightPadding: 16

                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }

                ImageView {
                    verticalAlignment: VerticalAlignment.Center
                    preferredWidth: 26
                    preferredHeight: 26
                    rightMargin: 8
                    imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_folder_dark.png" : "asset:///icons/ic_folder.png"
                }

                Label {
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    text: bookManager.currentBrowserPath === "root" ? (qsTr("Bộ nhớ thiết bị") + Retranslate.onLanguageChanged) : bookManager.currentBrowserPath
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                    multiline: true
                    autoSize.maxLineCount: 2
                }

                Label {
                    leftMargin: 8
                    verticalAlignment: VerticalAlignment.Center
                    text: bookManager.browserItemCount + " " + (qsTr("mục") + Retranslate.onLanguageChanged)
                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                }
            }

            // =================================================================
            // 3. EMPTY STATE
            // =================================================================
            Container {
                visible: bookManager.browserItemCount === 0
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                topPadding: 40
                leftPadding: 24
                rightPadding: 24

                ImageView {
                    horizontalAlignment: HorizontalAlignment.Center
                    preferredWidth: 72
                    preferredHeight: 72
                    imageSource: "asset:///icons/ic_folder.png"
                    bottomMargin: 16
                    opacity: 0.6
                }

                Label {
                    horizontalAlignment: HorizontalAlignment.Center
                    text: (bookManager.browserSearchQuery !== "" || bookManager.browserFormatFilter !== "all") ?
                          (qsTr("Không tìm thấy tệp hoặc thư mục phù hợp") + Retranslate.onLanguageChanged) :
                          (qsTr("Thư mục trống hoặc không có sách phù hợp") + Retranslate.onLanguageChanged)
                    textStyle.base: SystemDefaults.TextStyles.TitleText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                }

                Button {
                    visible: (bookManager.browserSearchQuery !== "" || bookManager.browserFormatFilter !== "all")
                    topMargin: 16
                    horizontalAlignment: HorizontalAlignment.Center
                    text: qsTr("Xóa bộ lọc & tìm kiếm") + Retranslate.onLanguageChanged
                    onClicked: {
                        bookManager.clearBrowserFilter();
                        browserSearchInputField.text = "";
                    }
                }
            }

            // =================================================================
            // 4A. DETAILED LIST VIEW (StackListLayout)
            // =================================================================
            ListView {
                id: fileBrowserDetailedListView
                visible: bookManager.browserItemCount > 0 && bookManager.libraryViewMode === "detailed"
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

                dataModel: bookManager.fileBrowserModel

                layout: StackListLayout {
                    headerMode: ListHeaderMode.None
                }

                listItemComponents: [
                    ListItemComponent {
                        type: "item"
                        CustomListItem {
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
                                            if (ListItemData.path === "__UP__") {
                                                bookManager.browserGoUp();
                                            } else if (ListItemData.itemType === "folder" || ListItemData.isDir) {
                                                bookManager.openBrowserFolder(ListItemData.path);
                                            } else {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                app.openBook(path);
                                            }
                                        }
                                    }

                                    ActionItem {
                                        title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_info.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            browserPage.showBookDetail(ListItemData);
                                        }
                                    }

                                    ActionItem {
                                        title: (ListItemData.isFavorite ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
                                        imageSource: ListItemData.isFavorite ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var newState = bookManager.toggleFavorite(path);
                                            browserPage.showToast(newState ? (qsTr("Đã thêm vào yêu thích: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)) : (qsTr("Đã bỏ yêu thích: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)));
                                        }
                                    }

                                    ActionItem {
                                        title: (ListItemData.isReadingList ? qsTr("Bỏ khỏi DS đọc") : qsTr("Thêm vào danh sách đọc")) + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_book.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var newState = bookManager.toggleReadingList(path);
                                            browserPage.showToast(newState ? (qsTr("Đã thêm vào DS đọc: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)) : (qsTr("Đã bỏ khỏi DS đọc: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)));
                                        }
                                    }

                                    ActionItem {
                                        title: (ListItemData.progress >= 100 ? qsTr("Đánh dấu chưa đọc") : qsTr("Đánh dấu đã đọc xong")) + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_chart.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var isCompleted = bookManager.toggleBookCompleted(path);
                                            browserPage.showToast(isCompleted ? (qsTr("Đã đánh dấu đã đọc xong: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)) : (qsTr("Đã đánh dấu chưa đọc: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)));
                                        }
                                    }
                                }
                            ]

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                minHeight: 114
                                preferredHeight: 118
                                topPadding: 12
                                bottomPadding: 12
                                leftPadding: 16
                                rightPadding: 16

                                layout: StackLayout {
                                    orientation: LayoutOrientation.LeftToRight
                                }

                                // Icon / Thumbnail Box (84x106)
                                Container {
                                    preferredWidth: 84
                                    preferredHeight: 106
                                    minWidth: 84
                                    maxWidth: 84
                                    minHeight: 106
                                    maxHeight: 106
                                    verticalAlignment: VerticalAlignment.Center
                                    layout: DockLayout {}

                                    // 1. Folder / Storage Icon Container
                                    Container {
                                        visible: ListItemData.itemType === "folder" || ListItemData.isDir
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: ListItemData.path === "__UP__" ?
                                                    (bookManager.appTheme === "dark" ? Color.create("#1A1A1A") : Color.create("#E5E5E5")) :
                                                    (ListItemData.folderType === "sdcard" ?
                                                        (bookManager.appTheme === "dark" ? Color.create("#141414") : Color.create("#F5F5F5")) :
                                                     ListItemData.folderType === "internal" ?
                                                        (bookManager.appTheme === "dark" ? Color.create("#141414") : Color.create("#F5F5F5")) :
                                                        (bookManager.appTheme === "dark" ? Color.create("#141414") : Color.create("#F5F5F5")))
                                        layout: DockLayout {}

                                        ImageView {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            verticalAlignment: VerticalAlignment.Center
                                            preferredWidth: 62
                                            preferredHeight: 62
                                            imageSource: {
                                                if (bookManager.appTheme === "dark") {
                                                    return ListItemData.icon ? ListItemData.icon : "asset:///icons/ic_folder.png";
                                                }
                                                if (ListItemData.folderType === "sdcard") return "asset:///icons/ic_sdcard_dark.png";
                                                if (ListItemData.folderType === "internal") return "asset:///icons/ic_storage_dark.png";
                                                return "asset:///icons/ic_folder_dark.png";
                                            }
                                            scalingMethod: ScalingMethod.AspectFit
                                        }
                                    }

                                    // 2. Book Cover Thumbnail
                                    Container {
                                        visible: ListItemData.itemType === "book" && !ListItemData.isDir
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        layout: DockLayout {}

                                        // Real Cover Image
                                        ImageView {
                                            visible: ListItemData.cover !== undefined && ListItemData.cover !== ""
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            verticalAlignment: VerticalAlignment.Fill
                                            imageSource: (ListItemData.cover !== undefined && ListItemData.cover !== "") ? ("file://" + ListItemData.cover) : ""
                                            scalingMethod: ScalingMethod.AspectFit
                                        }

                                        // Fallback Format Badge Box
                                        Container {
                                            visible: ListItemData.cover === undefined || ListItemData.cover === ""
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            verticalAlignment: VerticalAlignment.Fill
                                            background: bookManager.appTheme === "dark" ? Color.create("#141414") : Color.create("#F5F5F5")
                                            layout: DockLayout {}

                                            Label {
                                                horizontalAlignment: HorizontalAlignment.Center
                                                verticalAlignment: VerticalAlignment.Center
                                                text: ListItemData.formatBadge ? ListItemData.formatBadge : (ListItemData.format ? ListItemData.format.toUpperCase() : "BOOK")
                                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: ListItemData.format === "pdf" ? Color.create("#FF6B6B") :
                                                                ListItemData.format === "epub" ? Color.create("#51CF66") :
                                                                ListItemData.format === "fb2" ? Color.create("#CC5DE8") :
                                                                ListItemData.format === "djvu" ? Color.create("#339AF0") :
                                                                Color.create("#10B981")
                                            }
                                        }
                                    }
                                }

                                // Info Container
                                Container {
                                    leftMargin: 16
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center

                                    Label {
                                        text: ListItemData.title ? ListItemData.title : (ListItemData.name ? ListItemData.name : "")
                                        textStyle.base: SystemDefaults.TextStyles.TitleText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                        multiline: true
                                        autoSize.maxLineCount: 2
                                    }

                                    Label {
                                        topMargin: 4
                                        text: (ListItemData.pageInfoStr ? (ListItemData.pageInfoStr + " • ") : (ListItemData.totalPages > 0 ? (ListItemData.totalPages + (bookManager.language === "en" ? (ListItemData.format === "epub" || ListItemData.format === "fb2" ? " chapters • " : " pages • ") : (ListItemData.format === "epub" || ListItemData.format === "fb2" ? " chương • " : " trang • "))) : "")) + (ListItemData.subtext ? ListItemData.subtext : (ListItemData.fileSize ? ListItemData.fileSize : ""))
                                        textStyle.base: SystemDefaults.TextStyles.BodyText
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                        autoSize.maxLineCount: 1
                                    }

                                    // Mini progress bar if book with progress
                                    Container {
                                        visible: ListItemData.itemType === "book" && ListItemData.progress > 0
                                        topMargin: 4
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        preferredHeight: 3
                                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                        layout: StackLayout {
                                            orientation: LayoutOrientation.LeftToRight
                                        }

                                        Container {
                                            preferredHeight: 3
                                            background: ListItemData.progress >= 100 ? Color.create("#059669") : Color.create("#10B981")
                                            layoutProperties: StackLayoutProperties {
                                                spaceQuota: Math.max(1, ListItemData.progress)
                                            }
                                        }
                                        Container {
                                            preferredHeight: 3
                                            background: Color.Transparent
                                            layoutProperties: StackLayoutProperties {
                                                spaceQuota: Math.max(0.001, 100 - ListItemData.progress)
                                            }
                                        }
                                    }
                                }

                                // Format badge if book
                                Container {
                                    visible: ListItemData.itemType === "book" && !ListItemData.isDir && ListItemData.format !== undefined && ListItemData.format !== ""
                                    verticalAlignment: VerticalAlignment.Center
                                    background: ListItemData.progress >= 100 ? Color.create("#16A34A") : (bookManager.appTheme === "dark" ? Color.create("#232D3B") : Color.create("#E2E8F0"))
                                    topPadding: 4
                                    bottomPadding: 4
                                    leftPadding: 10
                                    rightPadding: 10

                                    Label {
                                        text: ListItemData.progress >= 100 ?
                                              (qsTr("Xong") + Retranslate.onLanguageChanged) :
                                              (ListItemData.format ? ListItemData.format.toUpperCase() : "")
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: ListItemData.progress >= 100 ? Color.White : (bookManager.appTheme === "dark" ? Color.White : Color.Black)
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

            // =================================================================
            // 4B. MOSAIC GRID VIEW (GridListLayout with Real Covers)
            // =================================================================
            ListView {
                id: fileBrowserMosaicListView
                visible: bookManager.browserItemCount > 0 && bookManager.libraryViewMode === "mosaic"
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

                dataModel: bookManager.fileBrowserModel

                layout: GridListLayout {
                    columnCount: bookManager.screenWidth > 1000 ? 4 : 3
                    headerMode: ListHeaderMode.None
                    cellAspectRatio: 0.62
                    horizontalCellSpacing: 8
                    verticalCellSpacing: 10
                }

                listItemComponents: [
                    ListItemComponent {
                        type: "item"
                        CustomListItem {
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
                                            if (ListItemData.path === "__UP__") {
                                                bookManager.browserGoUp();
                                            } else if (ListItemData.itemType === "folder" || ListItemData.isDir) {
                                                bookManager.openBrowserFolder(ListItemData.path);
                                            } else {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                app.openBook(path);
                                            }
                                        }
                                    }

                                    ActionItem {
                                        title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_info.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            browserPage.showBookDetail(ListItemData);
                                        }
                                    }

                                    ActionItem {
                                        title: (ListItemData.isFavorite ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
                                        imageSource: ListItemData.isFavorite ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var newState = bookManager.toggleFavorite(path);
                                            browserPage.showToast(newState ? (qsTr("Đã thêm vào yêu thích: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)) : (qsTr("Đã bỏ yêu thích: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)));
                                        }
                                    }

                                    ActionItem {
                                        title: (ListItemData.isReadingList ? qsTr("Bỏ khỏi DS đọc") : qsTr("Thêm vào danh sách đọc")) + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_book.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var newState = bookManager.toggleReadingList(path);
                                            browserPage.showToast(newState ? (qsTr("Đã thêm vào DS đọc: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)) : (qsTr("Đã bỏ khỏi DS đọc: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)));
                                        }
                                    }

                                    ActionItem {
                                        title: (ListItemData.progress >= 100 ? qsTr("Đánh dấu chưa đọc") : qsTr("Đánh dấu đã đọc xong")) + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_chart.png"
                                        enabled: ListItemData.itemType === "book" && !ListItemData.isDir
                                        onTriggered: {
                                            var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                            var isCompleted = bookManager.toggleBookCompleted(path);
                                            browserPage.showToast(isCompleted ? (qsTr("Đã đánh dấu đã đọc xong: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)) : (qsTr("Đã đánh dấu chưa đọc: ") + (ListItemData.title ? ListItemData.title : ListItemData.name)));
                                        }
                                    }
                                }
                            ]

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                topPadding: 1
                                bottomPadding: 1
                                leftPadding: 1
                                rightPadding: 1
                                layout: DockLayout {}

                                // 1. Folder Card
                                Container {
                                    visible: ListItemData.itemType === "folder" || ListItemData.isDir
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                                    layout: DockLayout {}

                                    // -------------------------------------------------------------
                                    // 1A. FOLDER WITH COVER IMAGE FROM INSIDE BOOKS
                                    // -------------------------------------------------------------
                                    Container {
                                        visible: ListItemData.path !== "__UP__" && ListItemData.cover !== undefined && ListItemData.cover !== ""
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        layout: DockLayout {}

                                        // Real Book Cover Image from inside folder
                                        ImageView {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            verticalAlignment: VerticalAlignment.Fill
                                            imageSource: (ListItemData.cover !== undefined && ListItemData.cover !== "") ? ("file://" + ListItemData.cover) : ""
                                            scalingMethod: ScalingMethod.AspectFill
                                        }

                                        // Folder Indicator Badge on top-left of cover
                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Left
                                            verticalAlignment: VerticalAlignment.Top
                                            topMargin: 4
                                            leftMargin: 4
                                            background: Color.create("#CC000000")
                                            leftPadding: 6
                                            rightPadding: 6
                                            topPadding: 3
                                            bottomPadding: 3
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            ImageView {
                                                preferredWidth: 16
                                                preferredHeight: 16
                                                verticalAlignment: VerticalAlignment.Center
                                                imageSource: "asset:///icons/ic_folder.png"
                                            }

                                            Label {
                                                leftMargin: 4
                                                text: qsTr("Thư mục") + Retranslate.onLanguageChanged
                                                textStyle.base: SystemDefaults.TextStyles.XXSmallText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: Color.White
                                            }
                                        }

                                        // Translucent Bottom Overlay with Folder Title
                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            verticalAlignment: VerticalAlignment.Bottom
                                            background: Color.create("#D9000000")
                                            topPadding: 6
                                            bottomPadding: 6
                                            leftPadding: 8
                                            rightPadding: 8

                                            Label {
                                                horizontalAlignment: HorizontalAlignment.Fill
                                                text: ListItemData.name ? ListItemData.name : (ListItemData.title ? ListItemData.title : "")
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: Color.White
                                                multiline: true
                                                autoSize.maxLineCount: 2
                                            }
                                        }
                                    }

                                    // -------------------------------------------------------------
                                    // 1B. FOLDER WITHOUT COVER (Large Folder Icon in center)
                                    // -------------------------------------------------------------
                                    Container {
                                        visible: ListItemData.path !== "__UP__" && (ListItemData.cover === undefined || ListItemData.cover === "")
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: bookManager.appTheme === "dark" ? Color.create("#141414") : Color.create("#F5F5F5")
                                        topPadding: 16
                                        bottomPadding: 12
                                        leftPadding: 8
                                        rightPadding: 8

                                        layout: StackLayout {
                                            orientation: LayoutOrientation.TopToBottom
                                        }

                                        // Large Folder Icon Centered
                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            verticalAlignment: VerticalAlignment.Center
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            layout: DockLayout {}

                                            ImageView {
                                                horizontalAlignment: HorizontalAlignment.Center
                                                verticalAlignment: VerticalAlignment.Center
                                                preferredWidth: 84
                                                preferredHeight: 84
                                                opacity: 0.45
                                                imageSource: ListItemData.icon ? ListItemData.icon : "asset:///icons/ic_folder.png"
                                                scalingMethod: ScalingMethod.AspectFit
                                            }
                                        }

                                        // Folder Title
                                        Label {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: ListItemData.name ? ListItemData.name : (ListItemData.title ? ListItemData.title : "")
                                            textStyle.base: SystemDefaults.TextStyles.BodyText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                            multiline: true
                                            autoSize.maxLineCount: 2
                                        }

                                        Label {
                                            topMargin: 2
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: ListItemData.subtext ? ListItemData.subtext : (qsTr("Thư mục") + Retranslate.onLanguageChanged)
                                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                            autoSize.maxLineCount: 1
                                        }
                                    }

                                    // -------------------------------------------------------------
                                    // 1C. PARENT FOLDER ENTRY: .. (Back)
                                    // -------------------------------------------------------------
                                    Container {
                                        visible: ListItemData.path === "__UP__"
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: bookManager.appTheme === "dark" ? Color.create("#141414") : Color.create("#F5F5F5")
                                        topPadding: 16
                                        bottomPadding: 12
                                        leftPadding: 8
                                        rightPadding: 8

                                        layout: StackLayout {
                                            orientation: LayoutOrientation.TopToBottom
                                        }

                                        // Large Back Arrow Centered
                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            verticalAlignment: VerticalAlignment.Center
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            layout: DockLayout {}

                                            ImageView {
                                                horizontalAlignment: HorizontalAlignment.Center
                                                verticalAlignment: VerticalAlignment.Center
                                                preferredWidth: 64
                                                preferredHeight: 64
                                                opacity: 0.5
                                                imageSource: "asset:///icons/ic_previous.png"
                                                scalingMethod: ScalingMethod.AspectFit
                                            }
                                        }

                                        Label {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: ListItemData.name ? ListItemData.name : ".. (" + (qsTr("Lùi lại") + Retranslate.onLanguageChanged) + ")"
                                            textStyle.base: SystemDefaults.TextStyles.BodyText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                            multiline: true
                                            autoSize.maxLineCount: 2
                                        }

                                        Label {
                                            topMargin: 2
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: ListItemData.subtext ? ListItemData.subtext : (qsTr("Trở về thư mục trước") + Retranslate.onLanguageChanged)
                                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                            autoSize.maxLineCount: 1
                                        }
                                    }
                                }

                                // 2. Book Card
                                Container {
                                    visible: ListItemData.itemType === "book" && !ListItemData.isDir
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                                    layout: DockLayout {}

                                    // Real Cover Image
                                    ImageView {
                                        visible: ListItemData.cover !== undefined && ListItemData.cover !== ""
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        imageSource: (ListItemData.cover !== undefined && ListItemData.cover !== "") ? ("file://" + ListItemData.cover) : ""
                                        scalingMethod: ScalingMethod.AspectFill
                                    }

                                    // Finished badge on Real Cover
                                    Container {
                                        visible: (ListItemData.cover !== undefined && ListItemData.cover !== "") && ListItemData.progress >= 100
                                        horizontalAlignment: HorizontalAlignment.Left
                                        verticalAlignment: VerticalAlignment.Top
                                        topMargin: 4
                                        leftMargin: 4
                                        background: Color.create("#16A34A")
                                        leftPadding: 6
                                        rightPadding: 6
                                        topPadding: 2
                                        bottomPadding: 2

                                        Label {
                                            text: qsTr("Hoàn thành") + Retranslate.onLanguageChanged
                                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: Color.White
                                        }
                                    }

                                    // Format Badge Top Right on Real Cover
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
                                            text: ListItemData.formatBadge ? ListItemData.formatBadge : (ListItemData.format ? ListItemData.format.toUpperCase() : "")
                                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: ListItemData.format === "pdf" ? Color.create("#FF6B6B") :
                                                            ListItemData.format === "epub" ? Color.create("#51CF66") :
                                                            ListItemData.format === "fb2" ? Color.create("#CC5DE8") :
                                                            ListItemData.format === "djvu" ? Color.create("#339AF0") :
                                                            Color.create("#FCC419")
                                        }
                                    }

                                    // Bottom Overlay with Progress
                                    Container {
                                        visible: (ListItemData.cover !== undefined && ListItemData.cover !== "") && (ListItemData.progress > 0 || ListItemData.totalPages > 0)
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Bottom
                                        background: Color.create("#CC000000")
                                        leftPadding: 6
                                        rightPadding: 6
                                        topPadding: 3
                                        bottomPadding: 3

                                        layout: StackLayout {
                                            orientation: LayoutOrientation.TopToBottom
                                        }

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            layout: StackLayout {
                                                orientation: LayoutOrientation.LeftToRight
                                            }

                                            Label {
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                text: ListItemData.pageInfoStr ? ListItemData.pageInfoStr : (ListItemData.totalPages > 0 ? (ListItemData.totalPages + " tr") : "")
                                                textStyle.base: SystemDefaults.TextStyles.XXSmallText
                                                textStyle.color: Color.create("#D4D4D8")
                                                autoSize.maxLineCount: 1
                                            }

                                            Label {
                                                text: (ListItemData.progress > 0 ? ListItemData.progress : 0) + "%"
                                                textStyle.base: SystemDefaults.TextStyles.XXSmallText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: ListItemData.progress >= 100 ? Color.create("#51CF66") : Color.White
                                            }
                                        }

                                        Container {
                                            topMargin: 2
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            preferredHeight: 3
                                            background: Color.create("#40FFFFFF")
                                            layout: StackLayout {
                                                orientation: LayoutOrientation.LeftToRight
                                            }

                                            Container {
                                                preferredHeight: 3
                                                background: ListItemData.progress >= 100 ? Color.create("#059669") : Color.create("#10B981")
                                                layoutProperties: StackLayoutProperties {
                                                    spaceQuota: Math.max(1, ListItemData.progress !== undefined ? ListItemData.progress : 0)
                                                }
                                            }

                                            Container {
                                                preferredHeight: 3
                                                background: Color.Transparent
                                                layoutProperties: StackLayoutProperties {
                                                    spaceQuota: Math.max(0.001, 100 - (ListItemData.progress !== undefined ? ListItemData.progress : 0))
                                                }
                                            }
                                        }
                                    }

                                    // Fallback Book Card when Cover is Empty
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

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            layout: StackLayout {
                                                orientation: LayoutOrientation.LeftToRight
                                            }

                                            // Finished badge
                                            Container {
                                                visible: ListItemData.progress >= 100
                                                background: Color.create("#16A34A")
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
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            }

                                            Container {
                                                background: Color.create("#30000000")
                                                leftPadding: 6
                                                rightPadding: 6
                                                topPadding: 2
                                                bottomPadding: 2
                                                verticalAlignment: VerticalAlignment.Center

                                                Label {
                                                    text: ListItemData.formatBadge ? ListItemData.formatBadge : (ListItemData.format ? ListItemData.format.toUpperCase() : "")
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

                                        Container {
                                            verticalAlignment: VerticalAlignment.Center
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            topMargin: 4
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

                                            Label {
                                                horizontalAlignment: HorizontalAlignment.Center
                                                text: ListItemData.title ? ListItemData.title : (ListItemData.name ? ListItemData.name : "")
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                                multiline: true
                                                autoSize.maxLineCount: 3
                                            }

                                            Label {
                                                topMargin: 2
                                                horizontalAlignment: HorizontalAlignment.Center
                                                text: ListItemData.author ? ListItemData.author : ""
                                                textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                textStyle.fontWeight: FontWeight.Medium
                                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                                multiline: true
                                                autoSize.maxLineCount: 1
                                            }
                                        }

                                        Label {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: (ListItemData.fileSize ? ListItemData.fileSize : "") + (ListItemData.progress >= 100 ? (" • " + (qsTr("Hoàn thành") + Retranslate.onLanguageChanged)) : (ListItemData.progress > 0 ? (" • " + ListItemData.progress + "%") : ""))
                                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                                            textStyle.color: ListItemData.progress >= 100 ? Color.create("#51CF66") : (bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666"))
                                        }

                                        // Mini progress bar in fallback
                                        Container {
                                            visible: ListItemData.progress > 0
                                            topMargin: 4
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            preferredHeight: 3
                                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                            layout: StackLayout {
                                                orientation: LayoutOrientation.LeftToRight
                                            }

                                            Container {
                                                preferredHeight: 3
                                                background: ListItemData.progress >= 100 ? Color.create("#059669") : Color.create("#10B981")
                                                layoutProperties: StackLayoutProperties {
                                                    spaceQuota: Math.max(1, ListItemData.progress !== undefined ? ListItemData.progress : 0)
                                                }
                                            }

                                            Container {
                                                preferredHeight: 3
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
            imageSource: "asset:///icons/ic_previous.png"
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
        }
    ]
}
