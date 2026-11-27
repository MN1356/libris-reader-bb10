import bb.cascades 1.3
import bb.system 1.2

Sheet {
    id: shelfManagerSheet

    property string mode: "manage" // "manage" | "assign"
    property string targetBookPath: ""
    property string targetBookTitle: ""

    function openForManage() {
        mode = "manage";
        targetBookPath = "";
        targetBookTitle = "";
        refreshModel();
        shelfManagerSheet.open();
    }

    function openForBook(filePath, title) {
        mode = "assign";
        targetBookPath = filePath ? filePath : "";
        targetBookTitle = title ? title : "";
        refreshModel();
        shelfManagerSheet.open();
    }

    function refreshModel() {
        shelvesModel.clear();
        var list = bookManager.customShelves;
        for (var i = 0; i < list.length; ++i) {
            var name = list[i];
            shelvesModel.append({
                "name": name,
                "count": bookManager.getShelfBookCount(name),
                "isAssigned": (mode === "assign" && targetBookPath !== "") ? bookManager.isBookInShelf(targetBookPath, name) : false,
                "isActiveFilter": (bookManager.categoryFilter === ("shelf:" + name))
            });
        }
    }

    onCreationCompleted: {
        bookManager.customShelvesChanged.connect(refreshModel);
    }

    Page {
        titleBar: TitleBar {
            title: (shelfManagerSheet.mode === "assign" ? qsTr("Thêm vào kệ sách") : qsTr("Quản lý Kệ sách")) + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain
            dismissAction: ActionItem {
                title: qsTr("Xong") + Retranslate.onLanguageChanged
                onTriggered: {
                    shelfManagerSheet.close();
                }
            }
        }

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create(bookManager.uiBg)
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            // Book context header when assigning a book
            Container {
                visible: shelfManagerSheet.mode === "assign" && shelfManagerSheet.targetBookTitle !== ""
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(bookManager.uiSurface)
                topPadding: bookManager.dp(10)
                bottomPadding: bookManager.dp(10)
                leftPadding: bookManager.dp(16)
                rightPadding: bookManager.dp(16)

                Label {
                    text: shelfManagerSheet.targetBookTitle
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.W500
                    textStyle.color: Color.create(bookManager.uiAccent)
                    autoSize.maxLineCount: 1
                }
            }

            // Create new shelf inline bar
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(bookManager.uiCard)
                topPadding: bookManager.dp(10)
                bottomPadding: bookManager.dp(10)
                leftPadding: bookManager.dp(16)
                rightPadding: bookManager.dp(16)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                TextField {
                    id: newShelfField
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    hintText: qsTr("Tên kệ sách mới...") + Retranslate.onLanguageChanged
                    clearButtonVisible: true
                    input.submitKey: SubmitKey.Done
                    input.onSubmitted: {
                        createShelfAction();
                    }
                }

                Button {
                    leftMargin: bookManager.dp(10)
                    verticalAlignment: VerticalAlignment.Center
                    appearance: ControlAppearance.Primary
                    text: qsTr("Tạo") + Retranslate.onLanguageChanged
                    onClicked: {
                        createShelfAction();
                    }
                }
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: 1
                background: Color.create(bookManager.uiHairline)
            }

            // Empty state
            Container {
                visible: shelvesModel.size() === 0
                horizontalAlignment: HorizontalAlignment.Fill
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                topPadding: bookManager.dp(48)
                leftPadding: bookManager.dp(24)
                rightPadding: bookManager.dp(24)

                ImageView {
                    horizontalAlignment: HorizontalAlignment.Center
                    preferredWidth: bookManager.dp(76)
                    preferredHeight: bookManager.dp(76)
                    opacity: 0.45
                    imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_library.png" : "asset:///icons/ic_library_dark.png"
                    scalingMethod: ScalingMethod.AspectFit
                }

                Label {
                    topMargin: bookManager.dp(16)
                    horizontalAlignment: HorizontalAlignment.Center
                    text: qsTr("Chưa có kệ sách tùy chỉnh nào") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                    textStyle.fontWeight: FontWeight.W500
                    textStyle.color: Color.create(bookManager.uiInk)
                }
            }

            // Shelves ListView (full-bleed)
            ListView {
                id: shelvesListView
                visible: shelvesModel.size() > 0
                scrollRole: ScrollRole.Main
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

                function fwdDeleteShelf(name) {
                    if (bookManager.deleteShelf(name)) {
                        shelfToast.body = qsTr("Đã xóa kệ sách: ") + name;
                        shelfToast.show();
                    }
                }

                dataModel: ArrayDataModel {
                    id: shelvesModel
                }

                listItemComponents: [
                    ListItemComponent {
                        type: ""
                        CustomListItem {
                            id: shelfCell
                            dividerVisible: false
                            highlightAppearance: HighlightAppearance.Frame

                            contextActions: [
                                ActionSet {
                                    title: ListItemData.name ? ListItemData.name : ""

                                    DeleteActionItem {
                                        title: qsTr("Xóa kệ") + Retranslate.onLanguageChanged
                                        onTriggered: {
                                            shelfCell.ListItem.view.fwdDeleteShelf(ListItemData.name);
                                        }
                                    }
                                }
                            ]

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                background: (ListItemData.isAssigned || ListItemData.isActiveFilter) ?
                                            Color.create(bookManager.uiActive) :
                                            Color.Transparent

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    minHeight: bookManager.dp(80)
                                    topPadding: bookManager.dp(18)
                                    bottomPadding: bookManager.dp(18)
                                    leftPadding: bookManager.dp(18)
                                    rightPadding: bookManager.dp(18)
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    ImageView {
                                        preferredWidth: bookManager.dp(40)
                                        preferredHeight: bookManager.dp(40)
                                        verticalAlignment: VerticalAlignment.Center
                                        scalingMethod: ScalingMethod.AspectFit
                                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_library.png" : "asset:///icons/ic_library_dark.png"
                                    }

                                    Container {
                                        leftMargin: bookManager.dp(14)
                                        verticalAlignment: VerticalAlignment.Center
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

                                        Label {
                                            text: ListItemData.name ? ListItemData.name : ""
                                            textStyle.base: SystemDefaults.TextStyles.TitleText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: (ListItemData.isAssigned || ListItemData.isActiveFilter) ?
                                                             Color.create(bookManager.uiAccent) :
                                                             Color.create(bookManager.uiInk)
                                        }

                                        Label {
                                            topMargin: bookManager.dp(3)
                                            text: ListItemData.count + " " + (qsTr("cuốn") + Retranslate.onLanguageChanged)
                                            textStyle.base: SystemDefaults.TextStyles.BodyText
                                            textStyle.color: Color.create(bookManager.uiMuted)
                                        }
                                    }

                                    Label {
                                        visible: ListItemData.isAssigned || ListItemData.isActiveFilter
                                        verticalAlignment: VerticalAlignment.Center
                                        text: "✓"
                                        textStyle.base: SystemDefaults.TextStyles.TitleText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiAccent)
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
                    if (!item || !item.name) return;
                    if (shelfManagerSheet.mode === "assign" && shelfManagerSheet.targetBookPath !== "") {
                        bookManager.toggleBookInShelf(shelfManagerSheet.targetBookPath, item.name);
                        shelfManagerSheet.refreshModel();
                    } else {
                        bookManager.setCategoryFilter("shelf:" + item.name);
                        shelfManagerSheet.close();
                    }
                }
            }
        }
    }

    function createShelfAction() {
        var name = newShelfField.text ? newShelfField.text.trim() : "";
        if (name === "") return;
        if (bookManager.createShelf(name)) {
            if (shelfManagerSheet.mode === "assign" && shelfManagerSheet.targetBookPath !== "") {
                bookManager.toggleBookInShelf(shelfManagerSheet.targetBookPath, name);
            }
            newShelfField.text = "";
            newShelfField.loseFocus();
            shelfToast.body = qsTr("Đã tạo kệ sách: ") + name;
            shelfToast.show();
            refreshModel();
        }
    }

    attachedObjects: [
        SystemToast {
            id: shelfToast
            body: ""
        }
    ]
}
