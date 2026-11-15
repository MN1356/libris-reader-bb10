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
            imageSource: "asset:///icons/ic_grid.png"
            ActionBar.placement: ActionBarPlacement.OnBar
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

    property string filterText: ""

    Container {
        layout: DockLayout {}
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")

        // =====================================================================
        // VIEW 1: KHI CHƯA MỞ SÁCH (EMPTY STATE)
        // =====================================================================
        Container {
            visible: bookManager.currentFilePath === "" || bookManager.tocList.length === 0
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center
            preferredWidth: Math.min(bookManager.screenWidth > 0 ? bookManager.screenWidth - 48 : 500, 500)
            leftPadding: 20
            rightPadding: 20

            layout: StackLayout {
                orientation: LayoutOrientation.TopToBottom
            }

            ImageView {
                horizontalAlignment: HorizontalAlignment.Center
                preferredWidth: 64
                preferredHeight: 64
                imageSource: "asset:///icons/ic_toc.png"
            }

            Label {
                topMargin: 16
                horizontalAlignment: HorizontalAlignment.Center
                text: qsTr("Chưa có mục lục khả dụng") + Retranslate.onLanguageChanged
                textStyle.base: SystemDefaults.TextStyles.TitleText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
            }

            Label {
                topMargin: 8
                horizontalAlignment: HorizontalAlignment.Center
                multiline: true
                text: qsTr("Hãy mở một cuốn sách từ 'Tủ Sách' hoặc 'Gần Đây' để xem danh sách chương và điều hướng nhanh.") + Retranslate.onLanguageChanged
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#777777")
            }

            Button {
                topMargin: 20
                horizontalAlignment: HorizontalAlignment.Center
                text: qsTr("Đến Tủ Sách") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_grid.png"
                onClicked: {
                    app.switchToTab("shelf");
                }
            }
        }

        // =====================================================================
        // VIEW 2: KHI ĐÃ CÓ MỤC LỤC SÁCH
        // =====================================================================
        Container {
            visible: bookManager.currentFilePath !== "" && bookManager.tocList.length > 0
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            topPadding: 10
            bottomPadding: 10
            leftPadding: 12
            rightPadding: 12

            layout: StackLayout {
                orientation: LayoutOrientation.TopToBottom
            }

            // Search filter field
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                topPadding: 6
                bottomPadding: 6
                leftPadding: 10
                rightPadding: 10
                bottomMargin: 8

                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }

                ImageView {
                    preferredWidth: 28
                    preferredHeight: 28
                    verticalAlignment: VerticalAlignment.Center
                    imageSource: "asset:///icons/ic_search.png"
                }

                TextField {
                    id: tocSearchField
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

            // TOC List
            ListView {
                id: tocListView
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

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
                                minHeight: 64
                                leftPadding: 12 + (ListItemData.level ? (ListItemData.level * 18) : 0)
                                rightPadding: 12
                                topPadding: 8
                                bottomPadding: 8

                                background: ListItemData.isActive ? 
                                            (bookManager.appTheme === "dark" ? Color.create("#2510B981") : Color.create("#1510B981")) :
                                            (bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF"))

                                layout: StackLayout {
                                    orientation: LayoutOrientation.LeftToRight
                                }

                                // Active bar
                                Container {
                                    visible: ListItemData.isActive
                                    preferredWidth: 4
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.create("#10B981")
                                    rightMargin: 8
                                }

                                // Chapter title
                                Label {
                                    verticalAlignment: VerticalAlignment.Center
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: ListItemData.title
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                    textStyle.fontWeight: ListItemData.isActive ? FontWeight.Bold : FontWeight.Normal
                                    multiline: true
                                    autoSize.maxLineCount: 3
                                }

                                // Page / Chapter Badge
                                Container {
                                    visible: bookManager.isPdf ? (ListItemData.pageNumber > 0) : (ListItemData.level === 0)
                                    verticalAlignment: VerticalAlignment.Center
                                    background: ListItemData.isActive ? Color.create("#10B981") : 
                                                (bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5"))
                                    topPadding: 3
                                    bottomPadding: 3
                                    leftPadding: 8
                                    rightPadding: 8

                                    Label {
                                        text: (bookManager.isPdf ? (ListItemData.pageNumber !== undefined ? ListItemData.pageNumber : ListItemData.page) : (ListItemData.chapterIndex !== undefined ? (ListItemData.chapterIndex + 1) : (ListItemData.index !== undefined ? (ListItemData.index + 1) : ""))) + ""
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Medium
                                        textStyle.color: ListItemData.isActive ? Color.White : 
                                                        (bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#777777"))
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
            var copy = {
                title: item.title,
                chapterIndex: idx,
                index: idx,
                pageNumber: pg,
                page: pg,
                anchor: item.anchor !== undefined ? item.anchor : "",
                level: item.level ? item.level : 0,
                isActive: isCurrent
            };
            filtered.push(copy);
        }
        tocDataModel.append(filtered);
    }

    onCreationCompleted: {
        reloadTocModel();
        bookManager.tocListChanged.connect(reloadTocModel);
        bookManager.currentChapterChanged.connect(reloadTocModel);
    }
}
