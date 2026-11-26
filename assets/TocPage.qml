import bb.cascades 1.3

Page {
    id: tocPage

    actionBarVisibility: ChromeVisibility.Visible

    titleBar: TitleBar {
        title: (bookManager.currentFilePath !== "" ?
               (qsTr("Mục Lục") + (bookManager.currentBookTitle ? (" - " + bookManager.currentBookTitle) : "")) :
               qsTr("Mục Lục Sách")) + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
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
            title: qsTr("Đang Đọc") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_reading.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                app.switchToTab("reading");
            }
        },
        ActionItem {
            title: qsTr("Tủ Sách") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_library.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                app.switchToTab("shelf");
            }
        }
    ]

    property string filterText: ""

    Container {
        layout: DockLayout {}
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: Color.create(bookManager.uiBg)

        // 1. Empty State (no book open or empty TOC)
        Container {
            visible: bookManager.currentFilePath === "" || bookManager.tocList.length === 0
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center
            leftPadding: bookManager.dp(28)
            rightPadding: bookManager.dp(28)
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            ImageView {
                horizontalAlignment: HorizontalAlignment.Center
                preferredWidth: bookManager.dp(56)
                preferredHeight: bookManager.dp(56)
                opacity: 0.45
                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_toc.png" : "asset:///icons/ic_toc_dark.png"
                scalingMethod: ScalingMethod.AspectFit
            }

            Label {
                topMargin: bookManager.dp(14)
                horizontalAlignment: HorizontalAlignment.Center
                text: qsTr("Chưa có mục lục khả dụng") + Retranslate.onLanguageChanged
                textStyle.base: SystemDefaults.TextStyles.TitleText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.textAlign: TextAlign.Center
                textStyle.color: Color.create(bookManager.uiInk)
                multiline: true
            }

            Label {
                topMargin: bookManager.dp(6)
                horizontalAlignment: HorizontalAlignment.Center
                multiline: true
                text: qsTr("Hãy mở một cuốn sách từ 'Tủ Sách' hoặc 'Gần Đây' để xem danh sách chương và điều hướng nhanh.") + Retranslate.onLanguageChanged
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.textAlign: TextAlign.Center
                textStyle.color: Color.create(bookManager.uiMuted)
            }

            Button {
                topMargin: bookManager.dp(16)
                horizontalAlignment: HorizontalAlignment.Center
                appearance: ControlAppearance.Primary
                text: qsTr("Đến Tủ Sách") + Retranslate.onLanguageChanged
                onClicked: {
                    app.switchToTab("shelf");
                }
            }
        }

        // 2. Active Book TOC
        Container {
            visible: bookManager.currentFilePath !== "" && bookManager.tocList.length > 0
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(bookManager.uiCard)
                topPadding: bookManager.dp(10)
                bottomPadding: bookManager.dp(10)
                leftPadding: bookManager.dp(16)
                rightPadding: bookManager.dp(16)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                ImageView {
                    preferredWidth: bookManager.dp(24)
                    preferredHeight: bookManager.dp(24)
                    verticalAlignment: VerticalAlignment.Center
                    imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_search.png" : "asset:///icons/ic_search_dark.png"
                }

                TextField {
                    id: tocSearchField
                    leftMargin: bookManager.dp(10)
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    hintText: qsTr("Lọc theo tên chương...") + Retranslate.onLanguageChanged
                    clearButtonVisible: true
                    onTextChanging: {
                        tocPage.filterText = text.trim().toLowerCase();
                        reloadTocModel();
                    }
                }
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: 1
                background: Color.create(bookManager.uiHairline)
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                layout: DockLayout {}

                Label {
                    visible: tocDataModel.size() === 0
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    text: qsTr("Không tìm thấy chương phù hợp") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                    textStyle.textAlign: TextAlign.Center
                    textStyle.color: Color.create(bookManager.uiMuted)
                    multiline: true
                }

                ListView {
                    id: tocListView
                    visible: tocDataModel.size() > 0
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill

                    function itemType(data, indexPath) {
                        return "item";
                    }

                    dataModel: ArrayDataModel {
                        id: tocDataModel
                    }

                    listItemComponents: [
                        ListItemComponent {
                            type: "item"
                            CustomListItem {
                                dividerVisible: true
                                highlightAppearance: HighlightAppearance.Frame

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    minHeight: bookManager.dp(58)
                                    leftPadding: bookManager.dp(16) + Math.min(ListItemData.level ? ListItemData.level : 0, 4) * bookManager.dp(18)
                                    rightPadding: bookManager.dp(16)
                                    topPadding: bookManager.dp(12)
                                    bottomPadding: bookManager.dp(12)
                                    background: ListItemData.isActive ? Color.create(bookManager.uiActive) : Color.create(bookManager.uiBg)
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    Container {
                                        visible: ListItemData.isActive
                                        preferredWidth: bookManager.dp(4)
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.create("#D4A359")
                                        rightMargin: bookManager.dp(10)
                                    }

                                    Label {
                                        verticalAlignment: VerticalAlignment.Center
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        text: ListItemData.title ? ListItemData.title : ""
                                        textStyle.base: SystemDefaults.TextStyles.BodyText
                                        textStyle.color: (ListItemData.isActive || (ListItemData.level ? ListItemData.level : 0) === 0) ?
                                                         Color.create(bookManager.uiInk) : Color.create(bookManager.uiMuted)
                                        textStyle.fontWeight: ListItemData.isActive ? FontWeight.Bold : ((ListItemData.level ? ListItemData.level : 0) === 0 ? FontWeight.W500 : FontWeight.Normal)
                                        multiline: true
                                        autoSize.maxLineCount: 3
                                    }

                                    Container {
                                        visible: bookManager.isPdf ? (ListItemData.pageNumber > 0) : (ListItemData.level === 0)
                                        verticalAlignment: VerticalAlignment.Center
                                        leftMargin: bookManager.dp(12)
                                        background: ListItemData.isActive ? Color.create("#D4A359") : Color.create(bookManager.uiSurface)
                                        topPadding: bookManager.dp(3)
                                        bottomPadding: bookManager.dp(3)
                                        leftPadding: bookManager.dp(8)
                                        rightPadding: bookManager.dp(8)

                                        Label {
                                            text: (bookManager.isPdf ? (ListItemData.pageNumber !== undefined ? ListItemData.pageNumber : ListItemData.page) : (ListItemData.chapterIndex !== undefined ? (ListItemData.chapterIndex + 1) : (ListItemData.index !== undefined ? (ListItemData.index + 1) : ""))) + ""
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: ListItemData.isActive ? Color.create("#0D0B09") : Color.create(bookManager.uiMuted)
                                        }
                                    }
                                }
                            }
                        }
                    ]

                    onTriggered: {
                        var item = dataModel.data(indexPath);
                        if (item) {
                            var targetIdx = item.chapterIndex !== undefined ? item.chapterIndex : (item.index !== undefined ? item.index : 0);
                            var targetPage = item.pageNumber !== undefined ? item.pageNumber : (item.page !== undefined ? item.page : 0);
                            var targetAnchor = item.anchor !== undefined ? item.anchor : "";

                            if (bookManager.currentFilePath !== "") {
                                app.openBook(bookManager.currentFilePath);
                            }
                            app.switchToTab("reading");
                            bookManager.jumpToToc(targetIdx, targetPage, targetAnchor);
                        }
                    }
                }
            }
        }
    }

    function reloadTocModel() {
        tocDataModel.clear();
        var rawList = bookManager.tocList;
        if (!rawList) return;
        var filtered = [];
        var activeIdx = bookManager.currentChapterIndex;
        var isPdf = bookManager.isPdf;
        for (var i = 0; i < rawList.length; ++i) {
            var item = rawList[i];
            if (filterText !== "") {
                if (item.title && item.title.toLowerCase().indexOf(filterText) === -1) {
                    continue;
                }
            }
            var idx = item.chapterIndex !== undefined ? item.chapterIndex : (item.index !== undefined ? item.index : 0);
            var pg = item.pageNumber !== undefined ? item.pageNumber : (item.page !== undefined ? item.page : 0);
            var isCurrent = isPdf ? (pg === activeIdx) : (idx === activeIdx);
            filtered.push({
                title: item.title,
                chapterIndex: idx,
                index: idx,
                pageNumber: pg,
                page: pg,
                anchor: item.anchor !== undefined ? item.anchor : "",
                level: item.level ? item.level : 0,
                isActive: isCurrent
            });
        }
        tocDataModel.append(filtered);
    }

    onCreationCompleted: {
        reloadTocModel();
        bookManager.tocListChanged.connect(reloadTocModel);
        bookManager.currentChapterChanged.connect(reloadTocModel);
    }
}
