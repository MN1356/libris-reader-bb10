import bb.cascades 1.3
import bb.system 1.2

Sheet {
    id: opdsSheet

    property string lastDownloadedBookPath: ""
    property string lastDownloadedBookTitle: ""

    onOpened: {
        opdsManager.loadCatalogs();
    }

    onClosed: {
        while (opdsNavPane.top !== opdsPage) {
            opdsNavPane.pop();
        }
    }

    function openAddCatalogPage(item) {
        var page = addCatalogPageDef.createObject();
        if (!page) return;

        if (item) {
            page.isEditing = true;
            page.catalogId = item.id ? item.id : "";
            page.catalogName = item.name ? item.name : "";
            page.catalogUrl = item.url ? item.url : "";
            page.catalogUser = item.username ? item.username : "";
            page.catalogPass = item.password ? item.password : "";
            page.catalogDesc = item.description ? item.description : "";
        } else {
            page.isEditing = false;
            page.catalogId = "";
            page.catalogName = "";
            page.catalogUrl = "";
            page.catalogUser = "";
            page.catalogPass = "";
            page.catalogDesc = "";
        }

        page.cancelRequested.connect(function() {
            opdsNavPane.pop();
        });

        page.saveRequested.connect(function(id, name, url, user, pass, desc, isEdit) {
            if (isEdit && id !== "") {
                opdsManager.updateCatalog(id, name, url, user, pass, desc);
                opdsToast.body = qsTr("Đã cập nhật kho sách!") + Retranslate.onLanguageChanged;
            } else {
                opdsManager.addCatalog(name, url, user, pass, desc);
                opdsToast.body = qsTr("Đã thêm kho sách thành công!") + Retranslate.onLanguageChanged;
            }
            opdsNavPane.pop();
            opdsToast.show();
        });

        opdsNavPane.push(page);
    }

    function showBookDetailPage(item) {
        if (!item) return;
        var page = bookDetailPageDef.createObject();
        if (!page) return;

        page.bookTitle = item.title ? item.title : "";
        page.bookAuthor = item.author ? item.author : "";
        page.bookCoverUrl = item.coverUrl ? item.coverUrl : "";
        page.bookFormat = item.format ? item.format : "epub";
        page.bookFormatBadge = item.formatBadge ? item.formatBadge : "EPUB";
        page.bookDownloadUrl = item.downloadUrl ? item.downloadUrl : "";
        page.bookSummary = item.summary ? item.summary : "";

        page.backRequested.connect(function() {
            opdsNavPane.pop();
        });

        page.downloadRequested.connect(function(title, author, dlUrl, fmt, covUrl) {
            opdsNavPane.pop();
            opdsManager.downloadBook(title, author, dlUrl, fmt, covUrl);
        });

        opdsNavPane.push(page);
    }

    NavigationPane {
        id: opdsNavPane
        backButtonsVisible: false

        Page {
            id: opdsPage
            actionBarVisibility: ChromeVisibility.Visible

            titleBar: TitleBar {
                appearance: TitleBarAppearance.Plain
                title: opdsManager.isAtRoot ?
                       (qsTr("Kho Sách OPDS") + Retranslate.onLanguageChanged) :
                       (opdsManager.currentTitle ? opdsManager.currentTitle : (qsTr("Danh mục sách") + Retranslate.onLanguageChanged))

                dismissAction: ActionItem {
                    title: opdsManager.isAtRoot ?
                           (qsTr("Đóng") + Retranslate.onLanguageChanged) :
                           (qsTr("Lùi lại") + Retranslate.onLanguageChanged)
                    imageSource: opdsManager.isAtRoot ? "asset:///icons/ic_clear.png" : "asset:///icons/ic_previous.png"
                    onTriggered: {
                        if (opdsManager.isAtRoot) {
                            opdsSheet.close();
                        } else {
                            opdsManager.goBack();
                        }
                    }
                }

                acceptAction: ActionItem {
                    title: opdsManager.isAtRoot ?
                           (qsTr("Thêm kho") + Retranslate.onLanguageChanged) :
                           (qsTr("Tìm kiếm") + Retranslate.onLanguageChanged)
                    imageSource: opdsManager.isAtRoot ? "asset:///icons/ic_add.png" : "asset:///icons/ic_search.png"
                    onTriggered: {
                        if (opdsManager.isAtRoot) {
                            opdsSheet.openAddCatalogPage(null);
                        } else {
                            searchBarContainer.visible = !searchBarContainer.visible;
                            if (searchBarContainer.visible) {
                                opdsSearchInput.requestFocus();
                            }
                        }
                    }
                }
            }

            actions: [
                ActionItem {
                    title: qsTr("Trang chủ") + Retranslate.onLanguageChanged
                    imageSource: "asset:///icons/ic_library.png"
                    ActionBar.placement: ActionBarPlacement.OnBar
                    enabled: !opdsManager.isAtRoot
                    onTriggered: {
                        opdsManager.goHome();
                    }
                },
                ActionItem {
                    title: qsTr("Làm mới") + Retranslate.onLanguageChanged
                    imageSource: "asset:///icons/ic_reflow.png"
                    ActionBar.placement: ActionBarPlacement.OnBar
                    enabled: !opdsManager.isAtRoot
                    onTriggered: {
                        opdsManager.refreshFeed();
                    }
                },
                ActionItem {
                    title: qsTr("Thêm kho mới") + Retranslate.onLanguageChanged
                    imageSource: "asset:///icons/ic_add.png"
                    ActionBar.placement: ActionBarPlacement.OnBar
                    enabled: opdsManager.isAtRoot
                    onTriggered: {
                        opdsSheet.openAddCatalogPage(null);
                    }
                },
                ActionItem {
                    title: qsTr("Khôi phục mặc định") + Retranslate.onLanguageChanged
                    imageSource: "asset:///icons/ic_reflow.png"
                    ActionBar.placement: ActionBarPlacement.InOverflow
                    enabled: opdsManager.isAtRoot
                    onTriggered: {
                        opdsManager.resetDefaultCatalogs();
                        opdsToast.body = qsTr("Đã khôi phục các kho sách mặc định.");
                        opdsToast.show();
                    }
                },
                ActionItem {
                    title: qsTr("Đóng") + Retranslate.onLanguageChanged
                    imageSource: "asset:///icons/ic_clear.png"
                    ActionBar.placement: ActionBarPlacement.InOverflow
                    onTriggered: {
                        opdsSheet.close();
                    }
                }
            ]

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                layout: StackLayout {
                    orientation: LayoutOrientation.TopToBottom
                }

                // =============================================================
                // 1. ACTIVE DOWNLOAD PROGRESS STRIP
                // =============================================================
                Container {
                    visible: opdsManager.isDownloading
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#E5E5E5")
                    topPadding: 10
                    bottomPadding: 10
                    leftPadding: 16
                    rightPadding: 16
                    layout: StackLayout {
                        orientation: LayoutOrientation.TopToBottom
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }

                        ImageView {
                            preferredWidth: 26
                            preferredHeight: 26
                            verticalAlignment: VerticalAlignment.Center
                            imageSource: "asset:///icons/ic_download.png"
                        }

                        Label {
                            leftMargin: 10
                            verticalAlignment: VerticalAlignment.Center
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: opdsManager.downloadStatus
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                            autoSize.maxLineCount: 1
                        }

                        Button {
                            leftMargin: 8
                            preferredWidth: 95
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Hủy") + Retranslate.onLanguageChanged
                            onClicked: {
                                opdsManager.cancelDownload();
                            }
                        }
                    }

                    // Progress Bar Track & Fill
                    Container {
                        topMargin: 8
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 4
                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }

                        Container {
                            preferredHeight: 4
                            background: Color.create("#10B981")
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: Math.max(1, opdsManager.downloadProgress)
                            }
                        }
                        Container {
                            preferredHeight: 4
                            background: Color.Transparent
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: Math.max(0.001, 100 - opdsManager.downloadProgress)
                            }
                        }
                    }
                }

                // =============================================================
                // 2. DOWNLOAD SUCCESS BANNER (QUICK READ)
                // =============================================================
                Container {
                    visible: opdsSheet.lastDownloadedBookPath !== ""
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: Color.create("#15803D")
                    topPadding: 10
                    bottomPadding: 10
                    leftPadding: 16
                    rightPadding: 16
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    ImageView {
                        preferredWidth: 26
                        preferredHeight: 26
                        verticalAlignment: VerticalAlignment.Center
                        imageSource: "asset:///icons/ic_book.png"
                    }

                    Label {
                        leftMargin: 10
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        text: (qsTr("Đã tải xong: ") + opdsSheet.lastDownloadedBookTitle) + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.White
                        autoSize.maxLineCount: 1
                    }

                    Button {
                        leftMargin: 8
                        preferredWidth: 125
                        verticalAlignment: VerticalAlignment.Center
                        text: qsTr("Đọc ngay ›") + Retranslate.onLanguageChanged
                        onClicked: {
                            var p = opdsSheet.lastDownloadedBookPath;
                            opdsSheet.lastDownloadedBookPath = "";
                            opdsSheet.close();
                            app.openBook(p);
                        }
                    }
                }

                // =============================================================
                // 3. SEARCH BAR INSIDE FEED
                // =============================================================
                Container {
                    id: searchBarContainer
                    visible: false
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#E5E5E5")
                    topPadding: 8
                    bottomPadding: 8
                    leftPadding: 14
                    rightPadding: 14
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    ImageView {
                        verticalAlignment: VerticalAlignment.Center
                        preferredWidth: 28
                        preferredHeight: 28
                        imageSource: "asset:///icons/ic_search.png"
                    }

                    TextField {
                        id: opdsSearchInput
                        leftMargin: 8
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        hintText: qsTr("Tìm kiếm trong kho sách này...") + Retranslate.onLanguageChanged
                        input.submitKey: SubmitKey.Search
                        clearButtonVisible: true
                        input.onSubmitted: {
                            opdsSearchInput.loseFocus();
                            opdsManager.search(text);
                        }
                    }

                    Button {
                        leftMargin: 8
                        preferredWidth: 85
                        verticalAlignment: VerticalAlignment.Center
                        text: qsTr("Tìm") + Retranslate.onLanguageChanged
                        onClicked: {
                            opdsSearchInput.loseFocus();
                            opdsManager.search(opdsSearchInput.text);
                        }
                    }
                }

                // =============================================================
                // 4. VIEW 1: NATIVE CATALOGS LIST (ROOT)
                // =============================================================
                Container {
                    visible: opdsManager.isAtRoot
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    layout: DockLayout {}

                    ListView {
                        horizontalAlignment: HorizontalAlignment.Fill
                        verticalAlignment: VerticalAlignment.Fill
                        dataModel: opdsManager.catalogModel

                        layout: StackListLayout {
                            headerMode: ListHeaderMode.Sticky
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
                                    dividerVisible: true
                                    highlightAppearance: HighlightAppearance.Frame

                                    contextActions: [
                                        ActionSet {
                                            title: ListItemData.name ? ListItemData.name : ""
                                            subtitle: ListItemData.url ? ListItemData.url : ""

                                            ActionItem {
                                                title: qsTr("Mở kho sách") + Retranslate.onLanguageChanged
                                                imageSource: "asset:///icons/ic_open.png"
                                                onTriggered: {
                                                    opdsManager.openCatalog(ListItemData.id);
                                                }
                                            }

                                            ActionItem {
                                                title: qsTr("Chỉnh sửa") + Retranslate.onLanguageChanged
                                                imageSource: "asset:///icons/ic_settings.png"
                                                enabled: !ListItemData.isDefault
                                                onTriggered: {
                                                    opdsSheet.openAddCatalogPage(ListItemData);
                                                }
                                            }

                                            DeleteActionItem {
                                                title: qsTr("Xóa kho sách") + Retranslate.onLanguageChanged
                                                enabled: !ListItemData.isDefault
                                                onTriggered: {
                                                    opdsManager.removeCatalog(ListItemData.id);
                                                }
                                            }
                                        }
                                    ]

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        minHeight: 80
                                        topPadding: 14
                                        bottomPadding: 14
                                        leftPadding: 16
                                        rightPadding: 16
                                        layout: StackLayout {
                                            orientation: LayoutOrientation.LeftToRight
                                        }

                                        // Catalog Text Info
                                        Container {
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            verticalAlignment: VerticalAlignment.Center

                                            Label {
                                                text: ListItemData.name ? ListItemData.name : ""
                                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                                                autoSize.maxLineCount: 1
                                            }

                                            Label {
                                                topMargin: 2
                                                text: ListItemData.description ? ListItemData.description : ""
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#A1A1AA") : Color.create("#777777")
                                                multiline: true
                                                autoSize.maxLineCount: 2
                                            }

                                            Label {
                                                topMargin: 2
                                                text: ListItemData.url ? ListItemData.url : ""
                                                textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#71717A") : Color.create("#888888")
                                                autoSize.maxLineCount: 1
                                            }
                                        }

                                        // Next Chevron Arrow
                                        ImageView {
                                            verticalAlignment: VerticalAlignment.Center
                                            preferredWidth: 22
                                            preferredHeight: 22
                                            imageSource: "asset:///icons/ic_next.png"
                                            opacity: 0.55
                                        }
                                    }
                                }
                            }
                        ]

                        onTriggered: {
                            var item = dataModel.data(indexPath);
                            if (item && item.id) {
                                opdsManager.openCatalog(item.id);
                            }
                        }
                    }
                }

                // =============================================================
                // 5. VIEW 2: FEED EXPLORER (INSIDE CATALOG)
                // =============================================================
                Container {
                    visible: !opdsManager.isAtRoot
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    layout: DockLayout {}

                    // Loading Spinner
                    Container {
                        visible: opdsManager.isLoading
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center

                        ActivityIndicator {
                            visible: opdsManager.isLoading
                            running: opdsManager.isLoading
                            preferredWidth: 64
                            preferredHeight: 64
                            horizontalAlignment: HorizontalAlignment.Center
                            onVisibleChanged: { if (visible) start(); else stop(); }
                        }

                        Label {
                            topMargin: 14
                            horizontalAlignment: HorizontalAlignment.Center
                            text: opdsManager.statusMessage ? opdsManager.statusMessage : (qsTr("Đang tải dữ liệu...") + Retranslate.onLanguageChanged)
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                        }
                    }

                    // Error / Empty Message
                    Container {
                        visible: !opdsManager.isLoading && opdsManager.statusMessage !== "" && opdsManager.feedModel.size() === 0
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center
                        leftPadding: 24
                        rightPadding: 24

                        Label {
                            horizontalAlignment: HorizontalAlignment.Center
                            text: opdsManager.statusMessage
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#A1A1AA") : Color.create("#777777")
                            multiline: true
                            textStyle.textAlign: TextAlign.Center
                        }

                        Button {
                            topMargin: 16
                            horizontalAlignment: HorizontalAlignment.Center
                            text: qsTr("Thử lại") + Retranslate.onLanguageChanged
                            imageSource: "asset:///icons/ic_reflow.png"
                            onClicked: {
                                opdsManager.refreshFeed();
                            }
                        }
                    }

                    // Feed Items ListView
                    ListView {
                        visible: !opdsManager.isLoading
                        horizontalAlignment: HorizontalAlignment.Fill
                        verticalAlignment: VerticalAlignment.Fill
                        dataModel: opdsManager.feedModel

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
                                            title: ListItemData.title ? ListItemData.title : ""
                                            subtitle: ListItemData.author ? ListItemData.author : ""

                                            ActionItem {
                                                title: qsTr("Tải sách về máy") + Retranslate.onLanguageChanged
                                                imageSource: "asset:///icons/ic_download.png"
                                                enabled: ListItemData.entryType === "book"
                                                onTriggered: {
                                                    opdsManager.downloadBook(ListItemData.title, ListItemData.author, ListItemData.downloadUrl, ListItemData.format, ListItemData.coverUrl);
                                                }
                                            }

                                            ActionItem {
                                                title: qsTr("Xem tóm tắt & chi tiết") + Retranslate.onLanguageChanged
                                                imageSource: "asset:///icons/ic_info.png"
                                                enabled: ListItemData.entryType === "book"
                                                onTriggered: {
                                                    opdsSheet.showBookDetailPage(ListItemData);
                                                }
                                            }
                                        }
                                    ]

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        minHeight: 90
                                        topPadding: 12
                                        bottomPadding: 12
                                        leftPadding: 16
                                        rightPadding: 16

                                        // -----------------------------------------------------
                                        // SUB-VIEW A: NAVIGATION ITEM (CATEGORY / SECTION)
                                        // -----------------------------------------------------
                                        Container {
                                            visible: ListItemData.entryType === "navigation"
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Container {
                                                preferredWidth: 48
                                                preferredHeight: 48
                                                minWidth: 48
                                                maxWidth: 48
                                                minHeight: 48
                                                maxHeight: 48
                                                background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                                verticalAlignment: VerticalAlignment.Center
                                                layout: DockLayout {}

                                                ImageView {
                                                    horizontalAlignment: HorizontalAlignment.Center
                                                    verticalAlignment: VerticalAlignment.Center
                                                    preferredWidth: 28
                                                    preferredHeight: 28
                                                    imageSource: "asset:///icons/ic_folder.png"
                                                }
                                            }

                                            Container {
                                                leftMargin: 14
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                verticalAlignment: VerticalAlignment.Center

                                                Label {
                                                    text: ListItemData.title ? ListItemData.title : ""
                                                    textStyle.base: SystemDefaults.TextStyles.TitleText
                                                    textStyle.fontWeight: FontWeight.Bold
                                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                                                    autoSize.maxLineCount: 2
                                                }

                                                Label {
                                                    topMargin: 2
                                                    text: ListItemData.subtext ? ListItemData.subtext : ""
                                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#A1A1AA") : Color.create("#777777")
                                                    autoSize.maxLineCount: 1
                                                }
                                            }

                                            ImageView {
                                                verticalAlignment: VerticalAlignment.Center
                                                preferredWidth: 20
                                                preferredHeight: 20
                                                imageSource: "asset:///icons/ic_next.png"
                                                opacity: 0.55
                                            }
                                        }

                                        // -----------------------------------------------------
                                        // SUB-VIEW B: NEXT PAGE ITEM (PAGINATION)
                                        // -----------------------------------------------------
                                        Container {
                                            visible: ListItemData.entryType === "nextPage"
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Container {
                                                preferredWidth: 48
                                                preferredHeight: 48
                                                minWidth: 48
                                                maxWidth: 48
                                                minHeight: 48
                                                maxHeight: 48
                                                background: Color.create("#10B981")
                                                verticalAlignment: VerticalAlignment.Center
                                                layout: DockLayout {}

                                                ImageView {
                                                    horizontalAlignment: HorizontalAlignment.Center
                                                    verticalAlignment: VerticalAlignment.Center
                                                    preferredWidth: 28
                                                    preferredHeight: 28
                                                    imageSource: "asset:///icons/ic_next.png"
                                                }
                                            }

                                            Container {
                                                leftMargin: 14
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                verticalAlignment: VerticalAlignment.Center

                                                Label {
                                                    text: ListItemData.title ? ListItemData.title : ""
                                                    textStyle.base: SystemDefaults.TextStyles.TitleText
                                                    textStyle.fontWeight: FontWeight.Bold
                                                    textStyle.color: Color.create("#10B981")
                                                }

                                                Label {
                                                    topMargin: 2
                                                    text: ListItemData.subtext ? ListItemData.subtext : ""
                                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#A1A1AA") : Color.create("#777777")
                                                }
                                            }
                                        }

                                        // -----------------------------------------------------
                                        // SUB-VIEW C: DOWNLOADABLE BOOK ITEM
                                        // -----------------------------------------------------
                                        Container {
                                            visible: ListItemData.entryType === "book"
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            // Book Cover Thumbnail (70x100)
                                            Container {
                                                preferredWidth: 70
                                                preferredHeight: 100
                                                minWidth: 70
                                                maxWidth: 70
                                                minHeight: 100
                                                maxHeight: 100
                                                verticalAlignment: VerticalAlignment.Center
                                                background: bookManager.appTheme === "dark" ? Color.create("#1C1917") : Color.create("#E5E5E5")
                                                layout: DockLayout {}

                                                ImageView {
                                                    visible: ListItemData.coverUrl !== undefined && ListItemData.coverUrl !== ""
                                                    horizontalAlignment: HorizontalAlignment.Fill
                                                    verticalAlignment: VerticalAlignment.Fill
                                                    imageSource: ListItemData.coverUrl ? ListItemData.coverUrl : ""
                                                    scalingMethod: ScalingMethod.AspectFill
                                                }

                                                Container {
                                                    visible: ListItemData.coverUrl === undefined || ListItemData.coverUrl === ""
                                                    horizontalAlignment: HorizontalAlignment.Center
                                                    verticalAlignment: VerticalAlignment.Center
                                                    ImageView {
                                                        preferredWidth: 36
                                                        preferredHeight: 36
                                                        imageSource: "asset:///icons/ic_book.png"
                                                        opacity: 0.6
                                                    }
                                                }
                                            }

                                            // Book Details Column
                                            Container {
                                                leftMargin: 14
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                verticalAlignment: VerticalAlignment.Center

                                                Label {
                                                    text: ListItemData.title ? ListItemData.title : ""
                                                    textStyle.base: SystemDefaults.TextStyles.TitleText
                                                    textStyle.fontWeight: FontWeight.Bold
                                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                                                    multiline: true
                                                    autoSize.maxLineCount: 2
                                                }

                                                Label {
                                                    topMargin: 2
                                                    visible: ListItemData.author !== undefined && ListItemData.author !== ""
                                                    text: ListItemData.author ? ListItemData.author : ""
                                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#10B981") : Color.create("#10B981")
                                                    autoSize.maxLineCount: 1
                                                }

                                                Label {
                                                    topMargin: 4
                                                    visible: ListItemData.summary !== undefined && ListItemData.summary !== ""
                                                    text: ListItemData.summary ? ListItemData.summary : ""
                                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#A1A1AA") : Color.create("#777777")
                                                    multiline: true
                                                    autoSize.maxLineCount: 2
                                                }

                                                // Format Capsule Chip
                                                Container {
                                                    topMargin: 4
                                                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                                    leftPadding: 6
                                                    rightPadding: 6
                                                    topPadding: 2
                                                    bottomPadding: 2
                                                    horizontalAlignment: HorizontalAlignment.Left

                                                    Label {
                                                        text: ListItemData.formatBadge ? ListItemData.formatBadge : "EPUB"
                                                        textStyle.base: SystemDefaults.TextStyles.XSmallText
                                                        textStyle.fontWeight: FontWeight.Bold
                                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#E4E4E7") : Color.create("#777777")
                                                    }
                                                }
                                            }

                                            // Download Button
                                            Button {
                                                leftMargin: 10
                                                preferredWidth: 100
                                                verticalAlignment: VerticalAlignment.Center
                                                text: qsTr("Tải về") + Retranslate.onLanguageChanged
                                                imageSource: "asset:///icons/ic_download.png"
                                                onClicked: {
                                                    opdsManager.downloadBook(ListItemData.title, ListItemData.author, ListItemData.downloadUrl, ListItemData.format, ListItemData.coverUrl);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        ]

                        onTriggered: {
                            var item = dataModel.data(indexPath);
                            if (!item) return;

                            if (item.entryType === "navigation") {
                                opdsManager.openFeed(item.navUrl, item.title);
                            } else if (item.entryType === "nextPage") {
                                opdsManager.openFeed(item.navUrl, item.title, false);
                            } else if (item.entryType === "book") {
                                opdsSheet.showBookDetailPage(item);
                            }
                        }
                    }
                }
            }
        }
    }

    attachedObjects: [
        SystemToast {
            id: opdsToast
            body: ""
        },
        ComponentDefinition {
            id: addCatalogPageDef
            source: "AddCatalogPage.qml"
        },
        ComponentDefinition {
            id: bookDetailPageDef
            source: "OpdsBookDetailPage.qml"
        }
    ]

    onCreationCompleted: {
        opdsManager.bookDownloaded.connect(function(filePath, title) {
            opdsSheet.lastDownloadedBookPath = filePath;
            opdsSheet.lastDownloadedBookTitle = title;
            opdsToast.body = qsTr("Đã tải xong: ") + title;
            opdsToast.show();
        });
        opdsManager.downloadFailed.connect(function(err) {
            opdsToast.body = err;
            opdsToast.show();
        });
    }
}
