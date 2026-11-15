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
            layout: DockLayout {}
            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                topPadding: 12
                bottomPadding: 12
                leftPadding: 12
                rightPadding: 12

                // Search Filter for chapters
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                    topPadding: 6
                    bottomPadding: 6
                    leftPadding: 10
                    rightPadding: 10
                    bottomMargin: 10

                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    ImageView {
                        preferredWidth: 32
                        preferredHeight: 32
                        verticalAlignment: VerticalAlignment.Center
                        imageSource: "asset:///icons/ic_search.png"
                    }

                    TextField {
                        id: filterField
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                        hintText: qsTr("Lọc tên chương...") + Retranslate.onLanguageChanged
                        clearButtonVisible: true
                        onTextChanging: {
                            tocSheet.filterText = text.trim().toLowerCase();
                        }
                    }
                }

                // Table of Contents List
                ListView {
                    id: tocListView
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1.0
                    }

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
                                    minHeight: 68
                                    leftPadding: 12 + (ListItemData.level ? (ListItemData.level * 18) : 0)
                                    rightPadding: 12
                                    topPadding: 10
                                    bottomPadding: 10

                                    // Highlight background for the currently active chapter
                                    background: (ListItemData.isActive) ? 
                                                (bookManager.appTheme === "dark" ? Color.create("#2510B981") : Color.create("#1510B981")) :
                                                (bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF"))

                                    layout: StackLayout {
                                        orientation: LayoutOrientation.LeftToRight
                                    }

                                    // Active indicator bar
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
                                        layoutProperties: StackLayoutProperties {
                                            spaceQuota: 1.0
                                        }
                                        text: ListItemData.title
                                        textStyle.base: SystemDefaults.TextStyles.BodyText
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                        textStyle.fontWeight: ListItemData.isActive ? FontWeight.Bold : FontWeight.Normal
                                        multiline: true
                                        autoSize.maxLineCount: 3
                                    }

                                    // Page or Chapter Badge on the right
                                    Container {
                                        visible: bookManager.isPdf ? (ListItemData.page > 0) : (ListItemData.level === 0)
                                        verticalAlignment: VerticalAlignment.Center
                                        leftMargin: 8
                                        background: ListItemData.isActive ? Color.create("#10B981") : 
                                                    (bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5"))
                                        leftPadding: 8
                                        rightPadding: 8
                                        topPadding: 3
                                        bottomPadding: 3

                                        Label {
                                            text: (bookManager.isPdf ? (ListItemData.page !== undefined ? ListItemData.page : ListItemData.pageNumber) : (ListItemData.index !== undefined ? (ListItemData.index + 1) : (ListItemData.chapterIndex !== undefined ? (ListItemData.chapterIndex + 1) : ""))) + ""
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

    function populateToc() {
        var raw = bookManager.tocList;
        var activeIdx = bookManager.currentChapterIndex;
        var isPdf = bookManager.isPdf;
        var filtered = [];
        var f = tocSheet.filterText;

        for (var i = 0; i < raw.length; ++i) {
            var item = raw[i];
            var title = "" + item.title;
            if (f.length > 0 && title.toLowerCase().indexOf(f) === -1) {
                continue;
            }
            var idx = item.index !== undefined ? item.index : (item.chapterIndex !== undefined ? item.chapterIndex : 0);
            var pg = item.page !== undefined ? item.page : (item.pageNumber !== undefined ? item.pageNumber : 0);
            var isCurrent = isPdf ? (pg === activeIdx) : (idx === activeIdx);
            var copy = {
                "title": item.title,
                "index": idx,
                "chapterIndex": idx,
                "page": pg,
                "pageNumber": pg,
                "anchor": item.anchor !== undefined ? item.anchor : "",
                "level": item.level ? item.level : 0,
                "isActive": isCurrent
            };
            filtered.push(copy);
        }

        tocDataModel.clear();
        tocDataModel.append(filtered);
    }

    onOpened: {
        filterField.text = "";
        tocSheet.filterText = "";
        populateToc();
    }

    onFilterTextChanged: {
        populateToc();
    }
}
