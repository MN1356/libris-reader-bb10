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

    function openShelfAssign(filePath, title) {
        shelfManagerSheet.openForBook(filePath, title);
    }

    actionBarVisibility: ChromeVisibility.Visible

    titleBar: TitleBar {
        kind: TitleBarKind.FreeForm
        appearance: TitleBarAppearance.Plain
        kindProperties: FreeFormTitleBarKindProperties {
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                leftPadding: bookManager.dp(16)
                rightPadding: bookManager.dp(8)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                Label {
                    text: "Libris"
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    textStyle.base: SystemDefaults.TextStyles.TitleText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create("#F5EFE6")
                }

                Container {
                    verticalAlignment: VerticalAlignment.Center
                    preferredWidth: bookManager.dp(52)
                    preferredHeight: bookManager.dp(52)
                    layout: DockLayout {}
                    gestureHandlers: [
                        TapHandler {
                            onTapped: {
                                recentPage.isSearching = !recentPage.isSearching;
                                if (!recentPage.isSearching) {
                                    bookManager.setRecentSearchQuery("");
                                    recentSearchInputField.text = "";
                                }
                            }
                        }
                    ]
                    ImageView {
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center
                        preferredWidth: bookManager.dp(26)
                        preferredHeight: bookManager.dp(26)
                        imageSource: recentPage.isSearching ? "asset:///icons/ic_clear.png" : "asset:///icons/ic_search.png"
                    }
                }
            }
            expandableArea {
                indicatorVisibility: TitleBarExpandableAreaIndicatorVisibility.Hidden
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
            title: qsTr("Tủ Sách") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_library.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                app.switchToTab("shelf");
            }
        },
        ActionItem {
            title: qsTr("Mở Tệp") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_open_file.png"
            ActionBar.placement: ActionBarPlacement.Signature
            shortcuts: [
                Shortcut { key: "O" },
                Shortcut { key: "E" }
            ]
            onTriggered: {
                app.openFilePicker();
            }
        },
        ActionItem {
            title: qsTr("Tiến Độ") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_calendar.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                app.openReadingStats();
            }
        },
        ActionItem {
            title: qsTr("Truyền Wi-Fi") + Retranslate.onLanguageChanged
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
            title: qsTr("Kho OPDS") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_opds.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            shortcuts: [
                Shortcut { key: "K" }
            ]
            onTriggered: {
                app.openOpds();
            }
        },
        ActionItem {
            title: qsTr("Quét lại") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_refresh.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            onTriggered: {
                bookManager.scanLibrary();
                recentToast.body = qsTr("Đang nạp và quét thư viện sách...") + Retranslate.onLanguageChanged;
                recentToast.show();
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

        // Inline Live Search Bar
        Container {
            id: searchBarContainer
            visible: recentPage.isSearching
            horizontalAlignment: HorizontalAlignment.Fill
            background: Color.create(bookManager.uiCard)
            topPadding: bookManager.dp(10)
            bottomPadding: bookManager.dp(10)
            leftPadding: bookManager.dp(14)
            rightPadding: bookManager.dp(14)
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            TextField {
                id: recentSearchInputField
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
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
        }

        Container {
            visible: recentPage.isSearching
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: 1
            background: Color.create(bookManager.uiHairline)
        }

        ListView {
            id: homeListView
            property bool isSearchingActive: recentPage.isSearching

            function fwdToast(msg) {
                recentToast.body = msg;
                recentToast.show();
            }
            function fwd_showBookDetail() {
                return recentPage.showBookDetail.apply(recentPage, arguments);
            }
            function fwd_openShelfAssign() {
                return recentPage.openShelfAssign.apply(recentPage, arguments);
            }
            function fwdClearSearch() {
                bookManager.setRecentSearchQuery("");
                recentSearchInputField.text = "";
            }
            function fwdOpenImport() {
                app.switchToTab("import");
            }
            function itemType(data, indexPath) {
                return (data && data.rowType) ? data.rowType : "book";
            }

            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            dataModel: bookManager.homeFeedModel

            layout: StackListLayout {
                headerMode: ListHeaderMode.None
            }

            listItemComponents: [
                // 1. DASHBOARD HEADER: Calm Goal Strip + Hero Card + Quote Recall
                ListItemComponent {
                    type: "dashboard"
                    Container {
                        id: dashRoot
                        horizontalAlignment: HorizontalAlignment.Fill
                        topPadding: bookManager.dp(12)
                        bottomPadding: bookManager.dp(4)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        // A. Unified Daily Reading Goal Card
                        Container {
                            visible: !dashRoot.ListItem.view.isSearchingActive || bookManager.recentSearchQuery === ""
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: bookManager.dp(10)
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            leftPadding: bookManager.dp(14)
                            rightPadding: bookManager.dp(14)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            gestureHandlers: [
                                TapHandler {
                                    onTapped: {
                                        app.openReadingStats();
                                    }
                                }
                            ]

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    verticalAlignment: VerticalAlignment.Center
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: {
                                        var mins = Math.floor(bookManager.todayReadingSeconds / 60);
                                        var goal = Math.max(1, bookManager.dailyGoalMinutes);
                                        var pct = Math.min(100, Math.round((bookManager.todayReadingSeconds / 60.0) / goal * 100));
                                        return (qsTr("Mục tiêu hôm nay") + ": " + mins + "/" + bookManager.dailyGoalMinutes + " " + (bookManager.language === "en" ? "min" : "phút") + " (" + pct + "%)") + Retranslate.onLanguageChanged;
                                    }
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiInk)
                                }

                                Container {
                                    verticalAlignment: VerticalAlignment.Center
                                    background: Color.create(bookManager.uiAccent)
                                    leftPadding: bookManager.dp(10)
                                    rightPadding: bookManager.dp(10)
                                    topPadding: bookManager.dp(3)
                                    bottomPadding: bookManager.dp(3)

                                    Label {
                                        text: (bookManager.currentStreak + " " + (bookManager.language === "en" ? (bookManager.currentStreak === 1 ? "day" : "days") : qsTr("ngày"))) + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#0E0C0A") : Color.White
                                    }
                                }
                            }

                            Container {
                                topMargin: bookManager.dp(8)
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: bookManager.dp(4)
                                minHeight: bookManager.dp(4)
                                background: Color.create(bookManager.uiSurface)
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.create("#D4A359")
                                    visible: bookManager.todayReadingSeconds > 0
                                    layoutProperties: StackLayoutProperties {
                                        spaceQuota: Math.max(0.001, Math.min(100, Math.round((bookManager.todayReadingSeconds / 60.0) / Math.max(1, bookManager.dailyGoalMinutes) * 100)))
                                    }
                                }
                                Container {
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.Transparent
                                    visible: Math.round((bookManager.todayReadingSeconds / 60.0) / Math.max(1, bookManager.dailyGoalMinutes) * 100) < 100
                                    layoutProperties: StackLayoutProperties {
                                        spaceQuota: Math.max(0.001, 100 - Math.min(100, Math.round((bookManager.todayReadingSeconds / 60.0) / Math.max(1, bookManager.dailyGoalMinutes) * 100)))
                                    }
                                }
                            }
                        }

                        // B. Editorial Continue Reading Hero Card
                        Container {
                            visible: bookManager.hasLastBook && (!dashRoot.ListItem.view.isSearchingActive || bookManager.recentSearchQuery === "" ||
                                     bookManager.lastBookTitle.toLowerCase().indexOf(bookManager.recentSearchQuery.toLowerCase()) >= 0 ||
                                     bookManager.lastBookAuthor.toLowerCase().indexOf(bookManager.recentSearchQuery.toLowerCase()) >= 0)
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: bookManager.dp(10)
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(14)
                            bottomPadding: bookManager.dp(14)
                            leftPadding: bookManager.dp(14)
                            rightPadding: bookManager.dp(14)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            contextActions: [
                                ActionSet {
                                    title: bookManager.lastBookTitle
                                    subtitle: bookManager.lastBookAuthor

                                    ActionItem {
                                        title: qsTr("Đọc sách") + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_open.png"
                                        onTriggered: { app.openBook(bookManager.lastBookPath); }
                                    }

                                    ActionItem {
                                        title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_info.png"
                                        onTriggered: {
                                            dashRoot.ListItem.view.fwd_showBookDetail({
                                                "title": bookManager.lastBookTitle,
                                                "author": bookManager.lastBookAuthor,
                                                "cover": bookManager.lastBookCover,
                                                "format": bookManager.lastBookFormat,
                                                "filePath": bookManager.lastBookPath,
                                                "progress": bookManager.lastBookProgress
                                            });
                                        }
                                    }

                                    ActionItem {
                                        title: (bookManager.isFavorite(bookManager.lastBookPath) ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
                                        imageSource: bookManager.isFavorite(bookManager.lastBookPath) ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
                                        onTriggered: {
                                            var newState = bookManager.toggleFavorite(bookManager.lastBookPath);
                                            dashRoot.ListItem.view.fwdToast(newState ? (qsTr("Đã thêm vào yêu thích: ") + bookManager.lastBookTitle) : (qsTr("Đã bỏ yêu thích: ") + bookManager.lastBookTitle));
                                        }
                                    }

                                    ActionItem {
                                        title: (bookManager.isReadingList(bookManager.lastBookPath) ? qsTr("Bỏ khỏi DS đọc") : qsTr("Thêm vào danh sách đọc")) + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_book.png"
                                        onTriggered: {
                                            var newState = bookManager.toggleReadingList(bookManager.lastBookPath);
                                            dashRoot.ListItem.view.fwdToast(newState ? (qsTr("Đã thêm vào DS đọc: ") + bookManager.lastBookTitle) : (qsTr("Đã bỏ khỏi DS đọc: ") + bookManager.lastBookTitle));
                                        }
                                    }

                                    ActionItem {
                                        title: qsTr("Thêm vào kệ...") + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_library.png"
                                        onTriggered: {
                                            dashRoot.ListItem.view.fwd_openShelfAssign(bookManager.lastBookPath, bookManager.lastBookTitle);
                                        }
                                    }

                                    ActionItem {
                                        title: (bookManager.lastBookProgress >= 100 ? qsTr("Đánh dấu chưa đọc") : qsTr("Đánh dấu đã đọc xong")) + Retranslate.onLanguageChanged
                                        imageSource: "asset:///icons/ic_done.png"
                                        onTriggered: {
                                            var isCompleted = bookManager.toggleBookCompleted(bookManager.lastBookPath);
                                            dashRoot.ListItem.view.fwdToast(isCompleted ? (qsTr("Đã đánh dấu đã đọc xong: ") + bookManager.lastBookTitle) : (qsTr("Đã đánh dấu chưa đọc: ") + bookManager.lastBookTitle));
                                        }
                                    }

                                    DeleteActionItem {
                                        title: qsTr("Xóa khỏi gần đây") + Retranslate.onLanguageChanged
                                        onTriggered: {
                                            var removedTitle = bookManager.lastBookTitle;
                                            bookManager.removeFromRecent(bookManager.lastBookPath);
                                            dashRoot.ListItem.view.fwdToast(qsTr("Đã xóa khỏi gần đây: ") + removedTitle);
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

                            // Cover Thumbnail
                            Container {
                                preferredWidth: bookManager.dp(106)
                                preferredHeight: bookManager.dp(152)
                                minWidth: bookManager.dp(106)
                                maxWidth: bookManager.dp(106)
                                minHeight: bookManager.dp(152)
                                maxHeight: bookManager.dp(152)
                                verticalAlignment: VerticalAlignment.Center
                                layout: DockLayout {}
                                background: Color.create(bookManager.uiSurface)

                                ImageView {
                                    visible: bookManager.lastBookCover !== ""
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    imageSource: bookManager.lastBookCover !== "" ? ("file://" + bookManager.lastBookCover) : ""
                                    scalingMethod: ScalingMethod.AspectFill
                                }

                                Container {
                                    visible: bookManager.lastBookCover === ""
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    layout: DockLayout {}

                                    ImageView {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        imageSource: {
                                            var fmt = bookManager.lastBookFormat ? bookManager.lastBookFormat.toLowerCase() : "epub";
                                            if (fmt === "azw" || fmt === "azw3" || fmt === "kf8" || fmt === "prc") fmt = "mobi";
                                            if (fmt !== "pdf" && fmt !== "epub" && fmt !== "mobi" && fmt !== "fb2" && fmt !== "cbz" && fmt !== "djvu" && fmt !== "txt") fmt = "epub";
                                            return "asset:///icons/default_cover_" + fmt + ".png";
                                        }
                                        scalingMethod: ScalingMethod.AspectFill
                                    }

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Center
                                        verticalAlignment: VerticalAlignment.Center
                                        leftPadding: bookManager.dp(8)
                                        rightPadding: bookManager.dp(8)
                                        bottomPadding: bookManager.dp(12)

                                        Label {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: bookManager.lastBookTitle ? bookManager.lastBookTitle : ""
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Normal
                                            textStyle.textAlign: TextAlign.Center
                                            textStyle.color: Color.create("#F5EFE6")
                                            multiline: true
                                            autoSize.maxLineCount: 3
                                        }
                                    }
                                }
                            }

                            // Hero Metadata & Action
                            Container {
                                leftMargin: bookManager.dp(14)
                                horizontalAlignment: HorizontalAlignment.Fill
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                verticalAlignment: VerticalAlignment.Center

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    Label {
                                        verticalAlignment: VerticalAlignment.Center
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        text: qsTr("ĐANG ĐỌC") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiAccent)
                                    }

                                    Container {
                                        visible: bookManager.lastBookFormat !== ""
                                        verticalAlignment: VerticalAlignment.Center
                                        background: Color.create(bookManager.uiSurface)
                                        topPadding: bookManager.dp(2)
                                        bottomPadding: bookManager.dp(2)
                                        leftPadding: bookManager.dp(8)
                                        rightPadding: bookManager.dp(8)

                                        Label {
                                            text: bookManager.lastBookFormat.toUpperCase()
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: Color.create(bookManager.uiInk)
                                        }
                                    }
                                }

                                Label {
                                    topMargin: bookManager.dp(4)
                                    text: bookManager.lastBookTitle
                                    textStyle.base: SystemDefaults.TextStyles.TitleText
                                    textStyle.fontWeight: FontWeight.W300
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    multiline: true
                                    autoSize.maxLineCount: 2
                                }

                                Label {
                                    topMargin: bookManager.dp(2)
                                    text: bookManager.lastBookAuthor ? bookManager.lastBookAuthor : (qsTr("Chưa rõ tác giả") + Retranslate.onLanguageChanged)
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    autoSize.maxLineCount: 1
                                }

                                Container {
                                    topMargin: bookManager.dp(8)
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    Container {
                                        verticalAlignment: VerticalAlignment.Center
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        preferredHeight: bookManager.dp(5)
                                        minHeight: bookManager.dp(5)
                                        background: Color.create(bookManager.uiSurface)
                                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                        Container {
                                            verticalAlignment: VerticalAlignment.Fill
                                            background: Color.create("#D4A359")
                                            visible: bookManager.lastBookProgress > 0
                                            layoutProperties: StackLayoutProperties {
                                                spaceQuota: Math.max(0.001, bookManager.lastBookProgress)
                                            }
                                        }
                                        Container {
                                            verticalAlignment: VerticalAlignment.Fill
                                            background: Color.Transparent
                                            visible: bookManager.lastBookProgress < 100
                                            layoutProperties: StackLayoutProperties {
                                                spaceQuota: Math.max(0.001, 100 - bookManager.lastBookProgress)
                                            }
                                        }
                                    }

                                    Label {
                                        leftMargin: bookManager.dp(8)
                                        verticalAlignment: VerticalAlignment.Center
                                        text: bookManager.lastBookProgress + "%"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiAccent)
                                    }
                                }

                                Button {
                                    topMargin: bookManager.dp(10)
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    appearance: ControlAppearance.Primary
                                    text: qsTr("Đọc tiếp") + Retranslate.onLanguageChanged
                                    onClicked: {
                                        app.openBook(bookManager.lastBookPath);
                                    }
                                }
                            }
                        }

                        // C. Daily Highlight Recall Card
                        Container {
                            visible: !dashRoot.ListItem.view.isSearchingActive && bookManager.totalHighlightCount > 0 && bookManager.dailyRecallQuote.text !== undefined && bookManager.dailyRecallQuote.text !== ""
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: bookManager.dp(4)
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            leftPadding: bookManager.dp(14)
                            rightPadding: bookManager.dp(14)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            gestureHandlers: [
                                TapHandler {
                                    onTapped: {
                                        var q = bookManager.dailyRecallQuote;
                                        if (q && q.filePath) {
                                            app.openBook(q.filePath);
                                            if (q.id) {
                                                bookManager.jumpToHighlight(q.id);
                                            }
                                        }
                                    }
                                }
                            ]

                            Container {
                                preferredWidth: bookManager.dp(3)
                                verticalAlignment: VerticalAlignment.Fill
                                background: Color.create("#D4A359")
                                rightMargin: bookManager.dp(12)
                            }

                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                Label {
                                    text: qsTr("TRÍCH DẪN HÔM NAY") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiAccent)
                                }

                                Label {
                                    topMargin: bookManager.dp(4)
                                    text: bookManager.dailyRecallQuote.text ? ("\"" + bookManager.dailyRecallQuote.text + "\"") : ""
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.fontStyle: FontStyle.Italic
                                    textStyle.lineHeight: 1.25
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    multiline: true
                                    autoSize.maxLineCount: 3
                                }

                                Label {
                                    topMargin: bookManager.dp(4)
                                    text: bookManager.dailyRecallQuote.bookTitle ? bookManager.dailyRecallQuote.bookTitle : ""
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    autoSize.maxLineCount: 1
                                }
                            }
                        }
                    }
                },

                // 2. FULL-BLEED SECTION HEADER
                ListItemComponent {
                    type: "section"
                    Header {
                        title: (ListItem.view.isSearchingActive && bookManager.recentSearchQuery !== "") ?
                               ((qsTr("KẾT QUẢ TÌM KIẾM (%1)").arg(bookManager.recentHistoryCount)) + Retranslate.onLanguageChanged) :
                               ((bookManager.hasLastBook ? qsTr("Sách khác gần đây") : qsTr("SÁCH GẦN ĐÂY")) + Retranslate.onLanguageChanged)
                    }
                },

                // 3. SERENE UNBOXED EMPTY STATE
                ListItemComponent {
                    type: "empty"
                    Container {
                        id: emptyRoot
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredWidth: bookManager.screenWidth
                        minWidth: bookManager.screenWidth
                        topPadding: bookManager.dp(36)
                        bottomPadding: bookManager.dp(36)
                        leftPadding: bookManager.dp(28)
                        rightPadding: bookManager.dp(28)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        ImageView {
                            horizontalAlignment: HorizontalAlignment.Center
                            preferredWidth: bookManager.dp(72)
                            preferredHeight: bookManager.dp(72)
                            opacity: 0.4
                            imageSource: (emptyRoot.ListItem.view.isSearchingActive && bookManager.recentSearchQuery !== "") ?
                                         (bookManager.appTheme === "dark" ? "asset:///icons/ic_search.png" : "asset:///icons/ic_search_dark.png") :
                                         (bookManager.appTheme === "dark" ? "asset:///icons/ic_book.png" : "asset:///icons/ic_book_dark.png")
                            scalingMethod: ScalingMethod.AspectFit
                        }

                        Label {
                            topMargin: bookManager.dp(14)
                            horizontalAlignment: HorizontalAlignment.Center
                            multiline: true
                            text: (emptyRoot.ListItem.view.isSearchingActive && bookManager.recentSearchQuery !== "") ?
                                  (qsTr("Không tìm thấy sách phù hợp") + Retranslate.onLanguageChanged) :
                                  (qsTr("Chưa có sách gần đây") + Retranslate.onLanguageChanged)
                            textStyle.base: SystemDefaults.TextStyles.SubtitleText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.textAlign: TextAlign.Center
                            textStyle.color: Color.create(bookManager.uiInk)
                        }

                        Label {
                            topMargin: bookManager.dp(6)
                            horizontalAlignment: HorizontalAlignment.Center
                            multiline: true
                            text: (emptyRoot.ListItem.view.isSearchingActive && bookManager.recentSearchQuery !== "") ?
                                  (qsTr("Thử tìm kiếm với từ khóa khác hoặc xóa bộ lọc.") + Retranslate.onLanguageChanged) :
                                  (qsTr("Mở sách từ Thư viện hoặc chọn Mở tệp để đọc.") + Retranslate.onLanguageChanged)
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.textAlign: TextAlign.Center
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }

                        Button {
                            topMargin: bookManager.dp(16)
                            horizontalAlignment: HorizontalAlignment.Center
                            appearance: ControlAppearance.Primary
                            text: (emptyRoot.ListItem.view.isSearchingActive && bookManager.recentSearchQuery !== "") ?
                                  (qsTr("Xóa tìm kiếm") + Retranslate.onLanguageChanged) :
                                  (qsTr("Nhập sách") + Retranslate.onLanguageChanged)
                            onClicked: {
                                if (emptyRoot.ListItem.view.isSearchingActive && bookManager.recentSearchQuery !== "") {
                                    emptyRoot.ListItem.view.fwdClearSearch();
                                } else {
                                    emptyRoot.ListItem.view.fwdOpenImport();
                                }
                            }
                        }
                    }
                },

                // 4. RECENT BOOK ITEM
                ListItemComponent {
                    type: "book"
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
                                        recentBookCell.ListItem.view.fwd_showBookDetail(ListItemData);
                                    }
                                }

                                ActionItem {
                                    title: (ListItemData.isFavorite ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
                                    imageSource: ListItemData.isFavorite ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
                                    onTriggered: {
                                        var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                        var newState = bookManager.toggleFavorite(path);
                                        recentBookCell.ListItem.view.fwdToast(newState ? (qsTr("Đã thêm vào yêu thích: ") + ListItemData.title) : (qsTr("Đã bỏ yêu thích: ") + ListItemData.title));
                                    }
                                }

                                ActionItem {
                                    title: (ListItemData.isReadingList ? qsTr("Bỏ khỏi DS đọc") : qsTr("Thêm vào danh sách đọc")) + Retranslate.onLanguageChanged
                                    imageSource: "asset:///icons/ic_book.png"
                                    onTriggered: {
                                        var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                        var newState = bookManager.toggleReadingList(path);
                                        recentBookCell.ListItem.view.fwdToast(newState ? (qsTr("Đã thêm vào DS đọc: ") + ListItemData.title) : (qsTr("Đã bỏ khỏi DS đọc: ") + ListItemData.title));
                                    }
                                }

                                ActionItem {
                                    title: qsTr("Thêm vào kệ...") + Retranslate.onLanguageChanged
                                    imageSource: "asset:///icons/ic_library.png"
                                    onTriggered: {
                                        var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                        recentBookCell.ListItem.view.fwd_openShelfAssign(path, ListItemData.title);
                                    }
                                }

                                ActionItem {
                                    title: (ListItemData.isCompleted || ListItemData.progress >= 100 ? qsTr("Đánh dấu chưa đọc") : qsTr("Đánh dấu đã đọc xong")) + Retranslate.onLanguageChanged
                                    imageSource: "asset:///icons/ic_done.png"
                                    onTriggered: {
                                        var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                        var isCompleted = bookManager.toggleBookCompleted(path);
                                        recentBookCell.ListItem.view.fwdToast(isCompleted ? (qsTr("Đã đánh dấu đã đọc xong: ") + ListItemData.title) : (qsTr("Đã đánh dấu chưa đọc: ") + ListItemData.title));
                                    }
                                }

                                DeleteActionItem {
                                    title: qsTr("Xóa khỏi gần đây") + Retranslate.onLanguageChanged
                                    onTriggered: {
                                        var path = ListItemData.filePath ? ListItemData.filePath : ListItemData.path;
                                        bookManager.removeFromRecent(path);
                                        recentBookCell.ListItem.view.fwdToast(qsTr("Đã xóa khỏi gần đây: ") + (ListItemData.title ? ListItemData.title : path));
                                    }
                                }
                            }
                        ]

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            topPadding: bookManager.dp(10)
                            bottomPadding: bookManager.dp(10)
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
                                layout: DockLayout {}
                                background: Color.create(bookManager.uiSurface)

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
                                    layout: DockLayout {}

                                    ImageView {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        imageSource: {
                                            var fmt = ListItemData.format ? ListItemData.format.toLowerCase() : "epub";
                                            if (fmt === "azw" || fmt === "azw3" || fmt === "kf8" || fmt === "prc") fmt = "mobi";
                                            if (fmt !== "pdf" && fmt !== "epub" && fmt !== "mobi" && fmt !== "fb2" && fmt !== "cbz" && fmt !== "djvu" && fmt !== "txt") fmt = "epub";
                                            return "asset:///icons/default_cover_" + fmt + ".png";
                                        }
                                        scalingMethod: ScalingMethod.AspectFill
                                    }

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Center
                                        verticalAlignment: VerticalAlignment.Center
                                        leftPadding: bookManager.dp(6)
                                        rightPadding: bookManager.dp(6)
                                        bottomPadding: bookManager.dp(10)

                                        Label {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            text: ListItemData.title ? ListItemData.title : ""
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Normal
                                            textStyle.textAlign: TextAlign.Center
                                            textStyle.color: Color.create("#F5EFE6")
                                            multiline: true
                                            autoSize.maxLineCount: 3
                                        }
                                    }
                                }
                            }

                            // Book Details
                            Container {
                                leftMargin: bookManager.dp(14)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                verticalAlignment: VerticalAlignment.Center

                                Label {
                                    text: ListItemData.title ? ListItemData.title : (qsTr("Chưa có tiêu đề") + Retranslate.onLanguageChanged)
                                    textStyle.base: SystemDefaults.TextStyles.TitleText
                                    textStyle.fontWeight: FontWeight.W300
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    multiline: true
                                    autoSize.maxLineCount: 2
                                }

                                Label {
                                    topMargin: bookManager.dp(2)
                                    text: (ListItemData.author ? ListItemData.author : (qsTr("Chưa rõ tác giả") + Retranslate.onLanguageChanged)) + " • " + (ListItemData.format ? ListItemData.format.toUpperCase() : "")
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    multiline: true
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
                                        text: (ListItemData.progress !== undefined ? ListItemData.progress : 0) + "%"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: (ListItemData.isCompleted || ListItemData.progress >= 100) ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiInk)
                                    }
                                }
                            }
                        }
                    }
                }
            ]

            onTriggered: {
                var item = dataModel.data(indexPath);
                if (item && item.rowType === "book" && item.filePath) {
                    app.openBook(item.filePath);
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
        ShelfManagerSheet {
            id: shelfManagerSheet
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
