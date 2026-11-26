import bb.cascades 1.3

Sheet {
    id: tocSheet

    property string filterText: ""

    Page {
        titleBar: TitleBar {
            title: qsTr("Mục lục") + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain
            dismissAction: ActionItem {
                title: qsTr("Đóng") + Retranslate.onLanguageChanged
                onTriggered: {
                    tocSheet.close();
                }
            }
        }

        Container {
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create(bookManager.uiBg)

            // Full-bleed Search Header
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
                    id: filterField
                    leftMargin: bookManager.dp(10)
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    hintText: qsTr("Lọc tên chương...") + Retranslate.onLanguageChanged
                    clearButtonVisible: true
                    onTextChanging: {
                        tocSheet.filterText = text.trim().toLowerCase();
                    }
                }
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: 1
                background: Color.create(bookManager.uiHairline)
            }

            // Content Area (Full-bleed List + Empty state)
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                layout: DockLayout {}

                Container {
                    visible: tocDataModel.size() === 0
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    leftPadding: bookManager.dp(28)
                    rightPadding: bookManager.dp(28)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    ImageView {
                        horizontalAlignment: HorizontalAlignment.Center
                        preferredWidth: bookManager.dp(56)
                        preferredHeight: bookManager.dp(56)
                        opacity: 0.4
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_toc.png" : "asset:///icons/ic_toc_dark.png"
                        scalingMethod: ScalingMethod.AspectFit
                    }

                    Label {
                        topMargin: bookManager.dp(12)
                        horizontalAlignment: HorizontalAlignment.Center
                        text: (tocSheet.filterText.length > 0 ? qsTr("Không tìm thấy chương phù hợp") : qsTr("Chưa có mục lục khả dụng")) + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SubtitleText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.textAlign: TextAlign.Center
                        textStyle.color: Color.create(bookManager.uiInk)
                        multiline: true
                    }
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
                                id: tocCell
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
                                        visible: bookManager.isPdf ? (ListItemData.page > 0) : (ListItemData.level === 0)
                                        verticalAlignment: VerticalAlignment.Center
                                        leftMargin: bookManager.dp(12)
                                        background: ListItemData.isActive ? Color.create("#D4A359") : Color.create(bookManager.uiSurface)
                                        leftPadding: bookManager.dp(8)
                                        rightPadding: bookManager.dp(8)
                                        topPadding: bookManager.dp(3)
                                        bottomPadding: bookManager.dp(3)

                                        Label {
                                            text: (bookManager.isPdf ? (ListItemData.page !== undefined ? ListItemData.page : ListItemData.pageNumber) : (ListItemData.index !== undefined ? (ListItemData.index + 1) : (ListItemData.chapterIndex !== undefined ? (ListItemData.chapterIndex + 1) : ""))) + ""
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
                            var targetIdx = item.index !== undefined ? item.index : (item.chapterIndex !== undefined ? item.chapterIndex : 0);
                            var targetPage = item.page !== undefined ? item.page : (item.pageNumber !== undefined ? item.pageNumber : 0);
                            var targetAnchor = item.anchor !== undefined ? item.anchor : "";
                            bookManager.jumpToToc(targetIdx, targetPage, targetAnchor);
                            tocSheet.close();
                        }
                    }
                }
            }
        }
    }

    function populateToc(scrollToActive) {
        var raw = bookManager.tocList;
        var activeIdx = bookManager.currentChapterIndex;
        var isPdf = bookManager.isPdf;
        var filtered = [];
        var f = tocSheet.filterText;
        var activeRow = -1;

        for (var i = 0; i < raw.length; ++i) {
            var item = raw[i];
            var title = "" + item.title;
            if (f.length > 0 && title.toLowerCase().indexOf(f) === -1) {
                continue;
            }
            var idx = item.index !== undefined ? item.index : (item.chapterIndex !== undefined ? item.chapterIndex : 0);
            var pg = item.page !== undefined ? item.page : (item.pageNumber !== undefined ? item.pageNumber : 0);
            var isCurrent = isPdf ? (pg === activeIdx) : (idx === activeIdx);
            if (isCurrent && activeRow === -1) {
                activeRow = filtered.length;
            }
            filtered.push({
                "title": item.title,
                "index": idx,
                "chapterIndex": idx,
                "page": pg,
                "pageNumber": pg,
                "anchor": item.anchor !== undefined ? item.anchor : "",
                "level": item.level ? item.level : 0,
                "isActive": isCurrent
            });
        }

        tocDataModel.clear();
        tocDataModel.append(filtered);
        if (scrollToActive && activeRow > 0) {
            tocListView.scrollToItem([activeRow], ScrollAnimation.None);
        }
    }

    onOpened: {
        filterField.text = "";
        tocSheet.filterText = "";
        populateToc(true);
    }

    onFilterTextChanged: {
        populateToc(false);
    }
}
