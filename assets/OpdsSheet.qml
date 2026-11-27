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
        var guard = 0;
        while (opdsNavPane.top && opdsNavPane.top !== opdsPage && guard++ < 8) {
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
        page.bookEpubUrl = item.epubUrl ? item.epubUrl : "";
        page.bookPdfUrl = item.pdfUrl ? item.pdfUrl : "";
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

        onPopTransitionEnded: {
            if (page) {
                page.destroy();
            }
        }

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
                    imageSource: "asset:///icons/ic_refresh.png"
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
                    imageSource: "asset:///icons/ic_refresh.png"
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
                    enabled: !opdsManager.isAtRoot
                    onTriggered: {
                        opdsSheet.close();
                    }
                }
            ]

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                background: Color.create(bookManager.uiBg)
                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                // =============================================================
                // 1. ACTIVE DOWNLOAD PROGRESS STRIP
                // =============================================================
                Container {
                    visible: opdsManager.isDownloading
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: Color.create(bookManager.uiCard)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(10)
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        ImageView {
                            preferredWidth: bookManager.dp(24)
                            preferredHeight: bookManager.dp(24)
                            verticalAlignment: VerticalAlignment.Center
                            scalingMethod: ScalingMethod.AspectFit
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_download.png" : "asset:///icons/ic_download_dark.png"
                        }

                        Label {
                            leftMargin: bookManager.dp(10)
                            rightMargin: bookManager.dp(8)
                            verticalAlignment: VerticalAlignment.Center
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: opdsManager.downloadStatus + (opdsManager.queuedCount > 0 ? (" (+" + opdsManager.queuedCount + " " + qsTr("chờ") + ")") : "") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiInk)
                            autoSize.maxLineCount: 1
                        }

                        Button {
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Hủy") + Retranslate.onLanguageChanged
                            onClicked: {
                                opdsManager.cancelDownload();
                            }
                        }
                    }

                    // Progress Bar Track & Fill
                    Container {
                        topMargin: bookManager.dp(8)
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: bookManager.dp(4)
                        background: Color.create(bookManager.uiSurface)
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        Container {
                            preferredHeight: bookManager.dp(4)
                            background: Color.create(bookManager.uiAccent)
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: Math.max(1, opdsManager.downloadProgress)
                            }
                        }
                        Container {
                            preferredHeight: bookManager.dp(4)
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
                    background: Color.create(bookManager.uiActive)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(10)
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    ImageView {
                        preferredWidth: bookManager.dp(24)
                        preferredHeight: bookManager.dp(24)
                        verticalAlignment: VerticalAlignment.Center
                        scalingMethod: ScalingMethod.AspectFit
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_book.png" : "asset:///icons/ic_book_dark.png"
                    }

                    Label {
                        leftMargin: bookManager.dp(10)
                        rightMargin: bookManager.dp(8)
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        text: (qsTr("Đã tải xong: ") + opdsSheet.lastDownloadedBookTitle) + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.W500
                        textStyle.color: Color.create(bookManager.uiInk)
                        autoSize.maxLineCount: 1
                    }

                    Button {
                        verticalAlignment: VerticalAlignment.Center
                        appearance: ControlAppearance.Primary
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
                    background: Color.create(bookManager.uiCard)
                    topPadding: bookManager.dp(8)
                    bottomPadding: bookManager.dp(8)
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    ImageView {
                        verticalAlignment: VerticalAlignment.Center
                        preferredWidth: bookManager.dp(24)
                        preferredHeight: bookManager.dp(24)
                        scalingMethod: ScalingMethod.AspectFit
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_search.png" : "asset:///icons/ic_search_dark.png"
                    }

                    TextField {
                        id: opdsSearchInput
                        leftMargin: bookManager.dp(8)
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
                        leftMargin: bookManager.dp(8)
                        verticalAlignment: VerticalAlignment.Center
                        appearance: ControlAppearance.Primary
                        text: qsTr("Tìm") + Retranslate.onLanguageChanged
                        onClicked: {
                            opdsSearchInput.loseFocus();
                            opdsManager.search(opdsSearchInput.text);
                        }
                    }
                }

                Container {
                    visible: opdsManager.isDownloading || opdsSheet.lastDownloadedBookPath !== "" || searchBarContainer.visible
                    horizontalAlignment: HorizontalAlignment.Fill
                    preferredHeight: 1
                    background: Color.create(bookManager.uiHairline)
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
                        scrollRole: ScrollRole.Main
                        horizontalAlignment: HorizontalAlignment.Fill
                        verticalAlignment: VerticalAlignment.Fill
                        dataModel: opdsManager.catalogModel

                        function fwd_openAddCatalogPage() {
                            return opdsSheet.openAddCatalogPage.apply(opdsSheet, arguments);
                        }

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
                                    id: ctxCell393
                                    dividerVisible: false
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
                                                    ctxCell393.ListItem.view.fwd_openAddCatalogPage(ListItemData);
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
                                        background: Color.create(bookManager.uiBg)

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            topPadding: bookManager.dp(14)
                                            bottomPadding: bookManager.dp(14)
                                            leftPadding: bookManager.dp(16)
                                            rightPadding: bookManager.dp(16)
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            // Catalog Text Info
                                            Container {
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                rightMargin: bookManager.dp(10)
                                                verticalAlignment: VerticalAlignment.Center

                                                Label {
                                                    text: ListItemData.name ? ListItemData.name : ""
                                                    textStyle.base: SystemDefaults.TextStyles.PrimaryText
                                                    textStyle.fontWeight: FontWeight.W500
                                                    textStyle.color: Color.create(bookManager.uiInk)
                                                    autoSize.maxLineCount: 1
                                                }

                                                Label {
                                                    topMargin: bookManager.dp(2)
                                                    visible: ListItemData.description !== undefined && ListItemData.description !== ""
                                                    text: ListItemData.description ? ListItemData.description : ""
                                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                                    textStyle.color: Color.create(bookManager.uiMuted)
                                                    multiline: true
                                                    autoSize.maxLineCount: 2
                                                }

                                                Label {
                                                    topMargin: bookManager.dp(2)
                                                    text: ListItemData.url ? ListItemData.url : ""
                                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                                    textStyle.color: Color.create(bookManager.uiAccent)
                                                    autoSize.maxLineCount: 1
                                                }
                                            }

                                            // Next Chevron Arrow
                                            ImageView {
                                                verticalAlignment: VerticalAlignment.Center
                                                preferredWidth: bookManager.dp(20)
                                                preferredHeight: bookManager.dp(20)
                                                scalingMethod: ScalingMethod.AspectFit
                                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_next.png" : "asset:///icons/ic_next_dark.png"
                                                opacity: 0.45
                                            }
                                        }

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            leftMargin: bookManager.dp(16)
                                            preferredHeight: 1
                                            background: Color.create(bookManager.uiHairline)
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
                            preferredWidth: bookManager.dp(56)
                            preferredHeight: bookManager.dp(56)
                            horizontalAlignment: HorizontalAlignment.Center
                            onVisibleChanged: { if (visible) start(); else stop(); }
                        }

                        Label {
                            topMargin: bookManager.dp(12)
                            horizontalAlignment: HorizontalAlignment.Center
                            text: opdsManager.statusMessage ? opdsManager.statusMessage : (qsTr("Đang tải dữ liệu...") + Retranslate.onLanguageChanged)
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }
                    }

                    // Error / Empty Message
                    Container {
                        visible: !opdsManager.isLoading && opdsManager.statusMessage !== "" && opdsManager.feedModel.size() === 0
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center
                        leftPadding: bookManager.dp(24)
                        rightPadding: bookManager.dp(24)

                        Label {
                            horizontalAlignment: HorizontalAlignment.Center
                            text: opdsManager.statusMessage
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.color: Color.create(bookManager.uiMuted)
                            multiline: true
                            textStyle.textAlign: TextAlign.Center
                        }

                        Button {
                            topMargin: bookManager.dp(16)
                            horizontalAlignment: HorizontalAlignment.Center
                            text: qsTr("Thử lại") + Retranslate.onLanguageChanged
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_refresh.png" : "asset:///icons/ic_refresh_dark.png"
                            onClicked: {
                                opdsManager.refreshFeed();
                            }
                        }
                    }

                    // Feed Items ListView (fwd_showBookDetailPage declared INSIDE ListView scope)
                    ListView {
                        id: opdsFeedListView
                        scrollRole: ScrollRole.Main
                        visible: !opdsManager.isLoading && opdsManager.feedModel.size() > 0
                        horizontalAlignment: HorizontalAlignment.Fill
                        verticalAlignment: VerticalAlignment.Fill
                        dataModel: opdsManager.feedModel

                        function fwd_showBookDetailPage() {
                            return opdsSheet.showBookDetailPage.apply(opdsSheet, arguments);
                        }

                        layout: StackListLayout {
                            headerMode: ListHeaderMode.None
                        }

                        listItemComponents: [
                            ListItemComponent {
                                type: "item"
                                CustomListItem {
                                    id: ctxCell569
                                    dividerVisible: false
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
                                                    ctxCell569.ListItem.view.fwd_showBookDetailPage(ListItemData);
                                                }
                                            }
                                        }
                                    ]

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        background: Color.create(bookManager.uiBg)

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            topPadding: bookManager.dp(12)
                                            bottomPadding: bookManager.dp(12)
                                            leftPadding: bookManager.dp(16)
                                            rightPadding: bookManager.dp(16)

                                            // SUB-VIEW A: NAVIGATION ITEM
                                            Container {
                                                visible: ListItemData.entryType === "navigation"
                                                horizontalAlignment: HorizontalAlignment.Fill
                                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                                Container {
                                                    preferredWidth: bookManager.dp(44)
                                                    preferredHeight: bookManager.dp(44)
                                                    minWidth: bookManager.dp(44)
                                                    maxWidth: bookManager.dp(44)
                                                    minHeight: bookManager.dp(44)
                                                    maxHeight: bookManager.dp(44)
                                                    background: Color.create(bookManager.uiSurface)
                                                    verticalAlignment: VerticalAlignment.Center
                                                    layout: DockLayout {}

                                                    ImageView {
                                                        horizontalAlignment: HorizontalAlignment.Center
                                                        verticalAlignment: VerticalAlignment.Center
                                                        preferredWidth: bookManager.dp(24)
                                                        preferredHeight: bookManager.dp(24)
                                                        scalingMethod: ScalingMethod.AspectFit
                                                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_folder.png" : "asset:///icons/ic_folder_dark.png"
                                                    }
                                                }

                                                Container {
                                                    leftMargin: bookManager.dp(12)
                                                    rightMargin: bookManager.dp(8)
                                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                    verticalAlignment: VerticalAlignment.Center

                                                    Label {
                                                        text: ListItemData.title ? ListItemData.title : ""
                                                        textStyle.base: SystemDefaults.TextStyles.PrimaryText
                                                        textStyle.fontWeight: FontWeight.W500
                                                        textStyle.color: Color.create(bookManager.uiInk)
                                                        multiline: true
                                                        autoSize.maxLineCount: 2
                                                    }

                                                    Label {
                                                        topMargin: bookManager.dp(2)
                                                        visible: ListItemData.subtext !== undefined && ListItemData.subtext !== ""
                                                        text: ListItemData.subtext ? ListItemData.subtext : ""
                                                        textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                                        textStyle.color: Color.create(bookManager.uiMuted)
                                                        autoSize.maxLineCount: 1
                                                    }
                                                }

                                                ImageView {
                                                    verticalAlignment: VerticalAlignment.Center
                                                    preferredWidth: bookManager.dp(20)
                                                    preferredHeight: bookManager.dp(20)
                                                    scalingMethod: ScalingMethod.AspectFit
                                                    imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_next.png" : "asset:///icons/ic_next_dark.png"
                                                    opacity: 0.45
                                                }
                                            }

                                            // SUB-VIEW B: NEXT PAGE ITEM (PAGINATION)
                                            Container {
                                                visible: ListItemData.entryType === "nextPage"
                                                horizontalAlignment: HorizontalAlignment.Fill
                                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                                Container {
                                                    preferredWidth: bookManager.dp(44)
                                                    preferredHeight: bookManager.dp(44)
                                                    minWidth: bookManager.dp(44)
                                                    maxWidth: bookManager.dp(44)
                                                    minHeight: bookManager.dp(44)
                                                    maxHeight: bookManager.dp(44)
                                                    background: Color.create(bookManager.uiActive)
                                                    verticalAlignment: VerticalAlignment.Center
                                                    layout: DockLayout {}

                                                    ImageView {
                                                        horizontalAlignment: HorizontalAlignment.Center
                                                        verticalAlignment: VerticalAlignment.Center
                                                        preferredWidth: bookManager.dp(22)
                                                        preferredHeight: bookManager.dp(22)
                                                        scalingMethod: ScalingMethod.AspectFit
                                                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_next.png" : "asset:///icons/ic_next_dark.png"
                                                    }
                                                }

                                                Container {
                                                    leftMargin: bookManager.dp(12)
                                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                    verticalAlignment: VerticalAlignment.Center

                                                    Label {
                                                        text: ListItemData.title ? ListItemData.title : ""
                                                        textStyle.base: SystemDefaults.TextStyles.PrimaryText
                                                        textStyle.fontWeight: FontWeight.W500
                                                        textStyle.color: Color.create(bookManager.uiAccent)
                                                    }

                                                    Label {
                                                        topMargin: bookManager.dp(2)
                                                        visible: ListItemData.subtext !== undefined && ListItemData.subtext !== ""
                                                        text: ListItemData.subtext ? ListItemData.subtext : ""
                                                        textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                                        textStyle.color: Color.create(bookManager.uiMuted)
                                                    }
                                                }
                                            }

                                            // SUB-VIEW C: DOWNLOADABLE BOOK ITEM
                                            Container {
                                                visible: ListItemData.entryType === "book"
                                                horizontalAlignment: HorizontalAlignment.Fill
                                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                                // Scaled Book Cover Thumbnail
                                                Container {
                                                    preferredWidth: bookManager.dp(68)
                                                    preferredHeight: bookManager.dp(98)
                                                    minWidth: bookManager.dp(68)
                                                    maxWidth: bookManager.dp(68)
                                                    minHeight: bookManager.dp(98)
                                                    maxHeight: bookManager.dp(98)
                                                    verticalAlignment: VerticalAlignment.Center
                                                    background: Color.create(bookManager.uiSurface)
                                                    layout: DockLayout {}

                                                    ImageView {
                                                        property bool isLocalCover: ListItemData.coverUrl !== undefined && ListItemData.coverUrl !== "" &&
                                                                                    (ListItemData.coverUrl.indexOf("file://") === 0 || ListItemData.coverUrl.indexOf("asset:///") === 0 || ListItemData.coverUrl.indexOf("/") === 0)
                                                        visible: isLocalCover
                                                        horizontalAlignment: HorizontalAlignment.Fill
                                                        verticalAlignment: VerticalAlignment.Fill
                                                        imageSource: isLocalCover ? (ListItemData.coverUrl.indexOf("/") === 0 ? ("file://" + ListItemData.coverUrl) : ListItemData.coverUrl) : ""
                                                        scalingMethod: ScalingMethod.AspectFill
                                                    }

                                                    Container {
                                                        visible: ListItemData.coverUrl === undefined || ListItemData.coverUrl === "" ||
                                                                 (ListItemData.coverUrl.indexOf("http://") === 0 || ListItemData.coverUrl.indexOf("https://") === 0)
                                                        horizontalAlignment: HorizontalAlignment.Fill
                                                        verticalAlignment: VerticalAlignment.Fill
                                                        layout: DockLayout {}

                                                        ImageView {
                                                            horizontalAlignment: HorizontalAlignment.Fill
                                                            verticalAlignment: VerticalAlignment.Fill
                                                            imageSource: "asset:///icons/default_cover.png"
                                                            scalingMethod: ScalingMethod.AspectFill
                                                        }

                                                        Container {
                                                            horizontalAlignment: HorizontalAlignment.Center
                                                            verticalAlignment: VerticalAlignment.Center
                                                            leftPadding: bookManager.dp(4)
                                                            rightPadding: bookManager.dp(4)
                                                            bottomPadding: bookManager.dp(8)

                                                            Label {
                                                                horizontalAlignment: HorizontalAlignment.Center
                                                                verticalAlignment: VerticalAlignment.Center
                                                                text: ListItemData.title ? ListItemData.title : ""
                                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                                textStyle.fontWeight: FontWeight.W500
                                                                textStyle.color: Color.create("#F5EFE6")
                                                                multiline: true
                                                                autoSize.maxLineCount: 2
                                                            }
                                                        }
                                                    }
                                                }

                                                // Book Details Column
                                                Container {
                                                    leftMargin: bookManager.dp(12)
                                                    rightMargin: bookManager.dp(8)
                                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                                    verticalAlignment: VerticalAlignment.Center

                                                    Label {
                                                        text: ListItemData.title ? ListItemData.title : ""
                                                        textStyle.base: SystemDefaults.TextStyles.PrimaryText
                                                        textStyle.fontWeight: FontWeight.W500
                                                        textStyle.color: Color.create(bookManager.uiInk)
                                                        multiline: true
                                                        autoSize.maxLineCount: 2
                                                    }

                                                    Label {
                                                        topMargin: bookManager.dp(2)
                                                        visible: ListItemData.author !== undefined && ListItemData.author !== ""
                                                        text: ListItemData.author ? ListItemData.author : ""
                                                        textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                                        textStyle.color: Color.create(bookManager.uiAccent)
                                                        autoSize.maxLineCount: 1
                                                    }

                                                    Label {
                                                        topMargin: bookManager.dp(4)
                                                        visible: ListItemData.summary !== undefined && ListItemData.summary !== ""
                                                        text: ListItemData.summary ? ListItemData.summary : ""
                                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                                        textStyle.color: Color.create(bookManager.uiMuted)
                                                        multiline: true
                                                        autoSize.maxLineCount: 2
                                                    }

                                                    // Format Capsule Pill
                                                    Container {
                                                        topMargin: bookManager.dp(6)
                                                        background: Color.create(bookManager.uiSurface)
                                                        leftPadding: bookManager.dp(8)
                                                        rightPadding: bookManager.dp(8)
                                                        topPadding: bookManager.dp(2)
                                                        bottomPadding: bookManager.dp(2)
                                                        horizontalAlignment: HorizontalAlignment.Left

                                                        Label {
                                                            text: ListItemData.formatBadge ? ListItemData.formatBadge : "EPUB"
                                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                                            textStyle.fontWeight: FontWeight.W500
                                                            textStyle.color: Color.create(bookManager.uiAccent)
                                                        }
                                                    }
                                                }

                                                // Auto-sized Download Button
                                                Button {
                                                    verticalAlignment: VerticalAlignment.Center
                                                    text: qsTr("Tải về") + Retranslate.onLanguageChanged
                                                    onClicked: {
                                                        opdsManager.downloadBook(ListItemData.title, ListItemData.author, ListItemData.downloadUrl, ListItemData.format, ListItemData.coverUrl);
                                                    }
                                                }
                                            }
                                        }

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            leftMargin: bookManager.dp(16)
                                            preferredHeight: 1
                                            background: Color.create(bookManager.uiHairline)
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
        opdsManager.downloadQueued.connect(function(title, queueSize) {
            opdsToast.body = (qsTr("Đã thêm vào hàng đợi") + " (" + queueSize + "): " + title) + Retranslate.onLanguageChanged;
            opdsToast.show();
        });
    }
}
