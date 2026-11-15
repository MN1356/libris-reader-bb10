import bb.cascades 1.3
import bb.system 1.2

Page {
    id: recentPage

    property bool isSearching: false

    onIsSearchingChanged: {
        if (isSearching) {
            recentSearchInputField.requestFocus();
        }
    }

    function showBookDetail(item) {
        if (!item) return;
        bookDetailSheet.openWithBook(item);
    }

    actionBarVisibility: ChromeVisibility.Visible

    titleBar: TitleBar {
        title: qsTr("BerryLeaf") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
        acceptAction: ActionItem {
            title: ""
            imageSource: recentPage.isSearching ? "asset:///icons/ic_clear.png" : "asset:///icons/ic_search.png"
            onTriggered: {
                recentPage.isSearching = !recentPage.isSearching;
                if (!recentPage.isSearching) {
                    bookManager.setRecentSearchQuery("");
                    recentSearchInputField.text = "";
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
                title: qsTr("Thư Viện") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_library.png"
                onTriggered: {
                    app.switchToTab("shelf");
                }
            },
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
            title: qsTr("Thư Viện") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_library.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                app.switchToTab("shelf");
            }
        },
        ActionItem {
            title: qsTr("Tiến Độ") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_chart.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                app.openReadingStats();
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
        layout: StackLayout {
            orientation: LayoutOrientation.TopToBottom
        }
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")

        // =============================================================
        // INLINE RECENT SEARCH BAR (Toggled by Search icon on TitleBar)
        // =============================================================
        Container {
            id: searchBarContainer
            visible: recentPage.isSearching
            horizontalAlignment: HorizontalAlignment.Fill
            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F5F5F5")
            topPadding: 10
            bottomPadding: 10
            leftPadding: 14
            rightPadding: 14

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            TextField {
                id: recentSearchInputField
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 1.0
                }
                hintText: qsTr("Tìm theo tên sách hoặc tác giả...") + Retranslate.onLanguageChanged
                input.submitKey: SubmitKey.Search
                clearButtonVisible: true
                text: bookManager.recentSearchQuery
                onTextChanging: {
                    bookManager.setRecentSearchQuery(text);
                }
                input.onSubmitted: {
                    recentSearchInputField.loseFocus();
                    bookManager.setRecentSearchQuery(text);
                }
            }

            Button {
                leftMargin: 8
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: 68
                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_search.png" : "asset:///icons/ic_search_dark.png"
                onClicked: {
                    recentSearchInputField.loseFocus();
                    bookManager.setRecentSearchQuery(recentSearchInputField.text);
                }
            }
        }

        ScrollView {
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1.0
            }
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            scrollViewProperties.scrollMode: ScrollMode.Vertical

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: 14
                bottomPadding: 24
                leftPadding: 14
                rightPadding: 14

                layout: StackLayout {
                    orientation: LayoutOrientation.TopToBottom
                }

                // =============================================================
                // 1. HIGHLIGHT: CONTINUE READING (Tiếp Tục Đọc Sách Đang Đọc Dở)
                // Pinned directly below the Title Bar with sleek neutral outline
                // =============================================================
                Container {
                    visible: bookManager.hasLastBook && (!recentPage.isSearching || bookManager.recentSearchQuery === "" ||
                             bookManager.lastBookTitle.toLowerCase().indexOf(bookManager.recentSearchQuery.toLowerCase()) >= 0 ||
                             bookManager.lastBookAuthor.toLowerCase().indexOf(bookManager.recentSearchQuery.toLowerCase()) >= 0)
                    horizontalAlignment: HorizontalAlignment.Fill
                    bottomMargin: 16
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
                        topPadding: 12
                        bottomPadding: 12
                        leftPadding: 12
                        rightPadding: 12

                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }

                        contextActions: [
                            ActionSet {
                                title: bookManager.lastBookTitle
                                subtitle: bookManager.lastBookAuthor

                                ActionItem {
                                    title: qsTr("Đọc sách") + Retranslate.onLanguageChanged
                                    imageSource: "asset:///icons/ic_open.png"
                                    onTriggered: {
                                        app.openBook(bookManager.lastBookPath);
                                    }
                                }

                                ActionItem {
                                    title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
                                    imageSource: "asset:///icons/ic_info.png"
                                    onTriggered: {
                                        var lastBook = {
                                            "title": bookManager.lastBookTitle,
                                            "author": bookManager.lastBookAuthor,
                                            "cover": bookManager.lastBookCover,
                                            "format": bookManager.lastBookFormat,
                                            "filePath": bookManager.lastBookPath,
                                            "progress": bookManager.lastBookProgress
                                        };
                                        recentPage.showBookDetail(lastBook);
                                    }
                                }

                                ActionItem {
                                    title: (bookManager.isFavorite(bookManager.lastBookPath) ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
                                    imageSource: bookManager.isFavorite(bookManager.lastBookPath) ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
                                    onTriggered: {
                                        var newState = bookManager.toggleFavorite(bookManager.lastBookPath);
                                        recentToast.body = newState ? (qsTr("Đã thêm vào yêu thích: ") + bookManager.lastBookTitle) : (qsTr("Đã bỏ yêu thích: ") + bookManager.lastBookTitle);
                                        recentToast.show();
                                    }
                                }

                                ActionItem {
                                    title: (bookManager.isReadingList(bookManager.lastBookPath) ? qsTr("Bỏ khỏi DS đọc") : qsTr("Thêm vào danh sách đọc")) + Retranslate.onLanguageChanged
                                    imageSource: "asset:///icons/ic_book.png"
                                    onTriggered: {
                                        var newState = bookManager.toggleReadingList(bookManager.lastBookPath);
                                        recentToast.body = newState ? (qsTr("Đã thêm vào DS đọc: ") + bookManager.lastBookTitle) : (qsTr("Đã bỏ khỏi DS đọc: ") + bookManager.lastBookTitle);
                                        recentToast.show();
                                    }
                                }

                                ActionItem {
                                    title: (bookManager.lastBookProgress >= 100 ? qsTr("Đánh dấu chưa đọc") : qsTr("Đánh dấu đã đọc xong")) + Retranslate.onLanguageChanged
                                    imageSource: "asset:///icons/ic_chart.png"
                                    onTriggered: {
                                        var isCompleted = bookManager.toggleBookCompleted(bookManager.lastBookPath);
                                        recentToast.body = isCompleted ? (qsTr("Đã đánh dấu đã đọc xong: ") + bookManager.lastBookTitle) : (qsTr("Đã đánh dấu chưa đọc: ") + bookManager.lastBookTitle);
                                        recentToast.show();
                                    }
                                }

                                DeleteActionItem {
                                    title: qsTr("Xóa khỏi gần đây") + Retranslate.onLanguageChanged
                                    onTriggered: {
                                        bookManager.removeFromRecent(bookManager.lastBookPath);
                                        recentToast.body = qsTr("Đã xóa khỏi gần đây: ") + bookManager.lastBookTitle;
                                        recentToast.show();
                                    }
                                }
                            }
                        ]

                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    app.openBook(bookManager.lastBookPath);
                                }
                            }
                        ]

                        // Book Cover Thumbnail
                        Container {
                            preferredWidth: 150
                            preferredHeight: 215
                            minWidth: 150
                            maxWidth: 150
                            minHeight: 215
                            maxHeight: 215
                            verticalAlignment: VerticalAlignment.Center
                            horizontalAlignment: HorizontalAlignment.Center
                            layout: DockLayout {}
                            background: bookManager.appTheme === "dark" ? Color.create("#181818") : Color.create("#F5F5F5")

                            ImageView {
                                visible: bookManager.lastBookCover !== ""
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                imageSource: bookManager.lastBookCover !== "" ? ("file://" + bookManager.lastBookCover) : ""
                                scalingMethod: ScalingMethod.AspectFill
                            }

                            ImageView {
                                visible: bookManager.lastBookCover === ""
                                horizontalAlignment: HorizontalAlignment.Center
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: 56
                                preferredHeight: 56
                                opacity: 0.35
                                imageSource: "asset:///icons/ic_book.png"
                            }
                        }

                        // Book Info & Progress
                        Container {
                            leftMargin: 14
                            horizontalAlignment: HorizontalAlignment.Fill
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1.0
                            }
                            verticalAlignment: VerticalAlignment.Center

                            // Top Row: Format Badge on Far Right
                            Container {
                                visible: bookManager.lastBookFormat !== ""
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                }

                                // Format Badge
                                Container {
                                    verticalAlignment: VerticalAlignment.Center
                                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                    topPadding: 2
                                    bottomPadding: 2
                                    leftPadding: 8
                                    rightPadding: 8

                                    Label {
                                        text: bookManager.lastBookFormat.toUpperCase()
                                        textStyle.base: SystemDefaults.TextStyles.XSmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#333333")
                                    }
                                }
                            }

                            Label {
                                topMargin: 4
                                text: bookManager.lastBookTitle
                                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                multiline: true
                                autoSize.maxLineCount: 2
                            }

                            Label {
                                topMargin: 2
                                text: bookManager.lastBookAuthor ? bookManager.lastBookAuthor : (qsTr("Chưa rõ tác giả") + Retranslate.onLanguageChanged)
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                autoSize.maxLineCount: 1
                            }

                            // Progress Bar & Percentage
                            Container {
                                topMargin: 6
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout {
                                    orientation: LayoutOrientation.LeftToRight
                                }

                                Container {
                                    verticalAlignment: VerticalAlignment.Center
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    preferredHeight: 8
                                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    Container {
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.create("#10B981")
                                        visible: bookManager.lastBookProgress > 0
                                        layoutProperties: StackLayoutProperties {
                                            spaceQuota: Math.max(1, bookManager.lastBookProgress)
                                        }
                                    }
                                    Container {
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.Transparent
                                        visible: bookManager.lastBookProgress < 100
                                        layoutProperties: StackLayoutProperties {
                                            spaceQuota: Math.max(1, 100 - bookManager.lastBookProgress)
                                        }
                                    }
                                }

                                Label {
                                    leftMargin: 8
                                    verticalAlignment: VerticalAlignment.Center
                                    text: bookManager.lastBookProgress + "%"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                            }

                            Button {
                                topMargin: 8
                                horizontalAlignment: HorizontalAlignment.Fill
                                text: qsTr("Đọc tiếp") + Retranslate.onLanguageChanged
                                imageSource: "asset:///icons/ic_reading.png"
                                onClicked: {
                                    app.openBook(bookManager.lastBookPath);
                                }
                            }
                        }
                    }
                }

                // =============================================================
                // 2. HEADER: SÁCH GẦN ĐÂY / KẾT QUẢ TÌM KIẾM
                // =============================================================
                Header {
                    title: (recentPage.isSearching && bookManager.recentSearchQuery !== "") ?
                           ((qsTr("KẾT QUẢ TÌM KIẾM (%1)").arg(bookManager.recentHistoryCount)) + Retranslate.onLanguageChanged) :
                           (qsTr("SÁCH GẦN ĐÂY") + Retranslate.onLanguageChanged)
                }

                // Search Empty State (when search returns 0 results)
                Container {
                    visible: recentPage.isSearching && bookManager.recentSearchQuery !== "" && bookManager.recentHistoryCount === 0
                    topMargin: 8
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                    topPadding: 1
                    bottomPadding: 1
                    leftPadding: 1
                    rightPadding: 1
                    layout: DockLayout {}

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                        topPadding: 32
                        bottomPadding: 32
                        leftPadding: 24
                        rightPadding: 24

                        ImageView {
                            horizontalAlignment: HorizontalAlignment.Center
                            preferredWidth: 96
                            preferredHeight: 96
                            opacity: 0.35
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_search.png" : "asset:///icons/ic_search_dark.png"
                            scalingMethod: ScalingMethod.AspectFit
                        }

                        Label {
                            topMargin: 16
                            horizontalAlignment: HorizontalAlignment.Center
                            text: qsTr("Không tìm thấy sách phù hợp") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SubtitleText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                        }

                        Label {
                            topMargin: 8
                            horizontalAlignment: HorizontalAlignment.Center
                            multiline: true
                            text: qsTr("Thử tìm kiếm với từ khóa khác hoặc xóa bộ lọc.") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                        }

                        Button {
                            topMargin: 16
                            horizontalAlignment: HorizontalAlignment.Center
                            text: qsTr("Xóa tìm kiếm") + Retranslate.onLanguageChanged
                            onClicked: {
                                bookManager.setRecentSearchQuery("");
                                recentSearchInputField.text = "";
                            }
                        }
                    }
                }

                // Empty State Prompt (when no recent books)
                Container {
                    visible: (bookManager.readingBooksCount + bookManager.completedBooksCount) === 0 && (!recentPage.isSearching || bookManager.recentSearchQuery === "")
                    topMargin: 8
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                    topPadding: 1
                    bottomPadding: 1
                    leftPadding: 1
                    rightPadding: 1
                    layout: DockLayout {}

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                        topPadding: 32
                        bottomPadding: 32
                        leftPadding: 24
                        rightPadding: 24

                        ImageView {
                            horizontalAlignment: HorizontalAlignment.Center
                            preferredWidth: 160
                            preferredHeight: 160
                            opacity: 0.35
                            imageSource: "asset:///icons/ic_book.png"
                            scalingMethod: ScalingMethod.AspectFit
                        }

                        Label {
                            topMargin: 16
                            horizontalAlignment: HorizontalAlignment.Center
                            text: qsTr("Chưa có sách gần đây") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SubtitleText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                        }

                        Label {
                            topMargin: 8
                            horizontalAlignment: HorizontalAlignment.Center
                            multiline: true
                            text: qsTr("Mở sách từ Thư viện hoặc chọn Mở tệp để đọc.") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                        }
                    }
                }

                // =============================================================
                // 3. RECENT BOOKS LIST
                // =============================================================
                ListView {
                    id: historyListView
                    visible: bookManager.recentHistoryCount > 0
                    topMargin: 8
                    horizontalAlignment: HorizontalAlignment.Fill
                    dataModel: bookManager.readingHistoryModel

                    layout: StackListLayout {
                        headerMode: ListHeaderMode.None
                    }

                    listItemComponents: [
                        ListItemComponent {
                            type: "item"
                            CustomListItem {
                                id: recentBookCell
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
                                                app.openBook(path);
                                            }
                                        }

                                        ActionItem {
                                            title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_info.png"
                                            onTriggered: {
                                                recentPage.showBookDetail(ListItemData);
                                            }
                                        }

                                        ActionItem {
                                            title: (ListItemData.isFavorite ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
                                            imageSource: ListItemData.isFavorite ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                var newState = bookManager.toggleFavorite(path);
                                                recentToast.body = newState ? (qsTr("Đã thêm vào yêu thích: ") + ListItemData.title) : (qsTr("Đã bỏ yêu thích: ") + ListItemData.title);
                                                recentToast.show();
                                            }
                                        }

                                        ActionItem {
                                            title: (ListItemData.isReadingList ? qsTr("Bỏ khỏi DS đọc") : qsTr("Thêm vào danh sách đọc")) + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_book.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                var newState = bookManager.toggleReadingList(path);
                                                recentToast.body = newState ? (qsTr("Đã thêm vào DS đọc: ") + ListItemData.title) : (qsTr("Đã bỏ khỏi DS đọc: ") + ListItemData.title);
                                                recentToast.show();
                                            }
                                        }

                                        ActionItem {
                                            title: (ListItemData.isCompleted || ListItemData.progress >= 100 ? qsTr("Đánh dấu chưa đọc") : qsTr("Đánh dấu đã đọc xong")) + Retranslate.onLanguageChanged
                                            imageSource: "asset:///icons/ic_chart.png"
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                var isCompleted = bookManager.toggleBookCompleted(path);
                                                recentToast.body = isCompleted ? (qsTr("Đã đánh dấu đã đọc xong: ") + ListItemData.title) : (qsTr("Đã đánh dấu chưa đọc: ") + ListItemData.title);
                                                recentToast.show();
                                            }
                                        }

                                        DeleteActionItem {
                                            title: qsTr("Xóa khỏi gần đây") + Retranslate.onLanguageChanged
                                            onTriggered: {
                                                var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                                bookManager.removeFromRecent(path);
                                                recentToast.body = qsTr("Đã xóa khỏi gần đây: ") + (ListItemData.title ? ListItemData.title : path);
                                                recentToast.show();
                                            }
                                        }
                                    }
                                ]

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topPadding: 10
                                    bottomPadding: 10
                                    leftPadding: 12
                                    rightPadding: 12
                                    layout: StackLayout {
                                        orientation: LayoutOrientation.LeftToRight
                                    }

                                    // Cover Thumbnail
                                    Container {
                                        preferredWidth: 106
                                        preferredHeight: 152
                                        minWidth: 106
                                        maxWidth: 106
                                        minHeight: 152
                                        maxHeight: 152
                                        verticalAlignment: VerticalAlignment.Center
                                        layout: DockLayout {}
                                        background: bookManager.appTheme === "dark" ? Color.create("#181818") : Color.create("#F5F5F5")

                                        ImageView {
                                            visible: ListItemData.cover !== undefined && ListItemData.cover !== ""
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            verticalAlignment: VerticalAlignment.Fill
                                            imageSource: (ListItemData.cover !== undefined && ListItemData.cover !== "") ? ("file://" + ListItemData.cover) : ""
                                            scalingMethod: ScalingMethod.AspectFit
                                        }

                                        ImageView {
                                            visible: ListItemData.cover === undefined || ListItemData.cover === ""
                                            horizontalAlignment: HorizontalAlignment.Center
                                            verticalAlignment: VerticalAlignment.Center
                                            preferredWidth: 48
                                            preferredHeight: 48
                                            opacity: 0.35
                                            imageSource: "asset:///icons/ic_book.png"
                                        }
                                    }

                                    // Book Details
                                    Container {
                                        leftMargin: 14
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        verticalAlignment: VerticalAlignment.Center

                                        Label {
                                            text: ListItemData.title ? ListItemData.title : (qsTr("Chưa có tiêu đề") + Retranslate.onLanguageChanged)
                                            textStyle.base: SystemDefaults.TextStyles.TitleText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                            multiline: true
                                            autoSize.maxLineCount: 2
                                        }

                                        Label {
                                            topMargin: 2
                                            text: (ListItemData.author ? ListItemData.author : (qsTr("Chưa rõ tác giả") + Retranslate.onLanguageChanged)) + " • " + (ListItemData.format ? ListItemData.format.toUpperCase() : "")
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                            multiline: true
                                            autoSize.maxLineCount: 1
                                        }

                                        // Progress Info & Bar
                                        Container {
                                            topMargin: 6
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            layout: StackLayout {
                                                orientation: LayoutOrientation.LeftToRight
                                            }

                                            Container {
                                                verticalAlignment: VerticalAlignment.Center
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                preferredHeight: 6
                                                minHeight: 6
                                                background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")

                                                layout: StackLayout {
                                                    orientation: LayoutOrientation.LeftToRight
                                                }

                                                Container {
                                                    verticalAlignment: VerticalAlignment.Fill
                                                    background: (ListItemData.isCompleted || ListItemData.progress >= 100) ? Color.create("#059669") : Color.create("#10B981")
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
                                                leftMargin: 8
                                                verticalAlignment: VerticalAlignment.Center
                                                text: (ListItemData.progress !== undefined ? ListItemData.progress : 0) + "%"
                                                textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: (ListItemData.isCompleted) ? Color.create("#10B981") : (bookManager.appTheme === "dark" ? Color.White : Color.Black)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    ]

                    onTriggered: {
                        var item = dataModel.data(indexPath);
                        if (item && item.filePath) {
                            app.openBook(item.filePath);
                        }
                    }
                }
            }
        }
    }

    attachedObjects: [
        BookDetailSheet {
            id: bookDetailSheet
            onRequestOpenBook: {
                app.openBook(filePath);
            }
        },
        SystemToast {
            id: recentToast
            body: ""
        }
    ]

    onCreationCompleted: {
        bookManager.refreshStats();
    }
}
