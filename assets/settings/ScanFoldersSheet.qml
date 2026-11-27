import bb.cascades 1.3
import bb.system 1.2

Sheet {
    id: scanFoldersSheet

    property string currentDirPath: "root"
    property bool currentDirSelected: false
    signal applyScanRequested()

    function openPicker() {
        currentDirPath = "root";
        refreshFolders();
        scanFoldersSheet.open();
    }

    function refreshFolders() {
        currentDirSelected = (currentDirPath !== "root") && bookManager.isScanFolderSelected(currentDirPath);
        selectedFoldersModel.clear();
        var sel = bookManager.scanFolders;
        if (sel) {
            for (var i = 0; i < sel.length; ++i) {
                selectedFoldersModel.append({
                    "path": sel[i],
                    "name": sel[i].split("/").pop() || sel[i]
                });
            }
        }

        browseFoldersModel.clear();
        var items = bookManager.listFoldersForPicker(currentDirPath);
        if (items) {
            for (var j = 0; j < items.length; ++j) {
                browseFoldersModel.append(items[j]);
            }
        }
    }

    function openSubfolder(path) {
        currentDirPath = path;
        refreshFolders();
    }

    function goUp() {
        currentDirPath = bookManager.parentFolderForPicker(currentDirPath);
        refreshFolders();
    }

    function toggleFolder(path) {
        bookManager.toggleScanFolder(path);
        refreshFolders();
    }

    Page {
        titleBar: TitleBar {
            title: qsTr("Chọn thư mục quét sách") + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain

            dismissAction: ActionItem {
                title: qsTr("Đóng") + Retranslate.onLanguageChanged
                onTriggered: {
                    scanFoldersSheet.close();
                }
            }

            acceptAction: ActionItem {
                title: qsTr("Quét ngay") + Retranslate.onLanguageChanged
                onTriggered: {
                    scanFoldersSheet.close();
                    scanFoldersSheet.applyScanRequested();
                }
            }
        }

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create(bookManager.uiBg)
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            // Currently Selected Folders Summary
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(bookManager.uiCard)
                topPadding: bookManager.dp(12)
                bottomPadding: bookManager.dp(12)
                leftPadding: bookManager.dp(14)
                rightPadding: bookManager.dp(14)
                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    Label {
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        verticalAlignment: VerticalAlignment.Center
                        text: bookManager.scanFolders.length === 0
                              ? (qsTr("Chế độ tự động: Đang quét toàn bộ bộ nhớ & thẻ nhớ") + Retranslate.onLanguageChanged)
                              : (qsTr("Đang chỉ quét %1 thư mục đã chọn:").replace("%1", bookManager.scanFolders.length) + Retranslate.onLanguageChanged)
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: bookManager.scanFolders.length === 0 ? Color.create(bookManager.uiMuted) : Color.create(bookManager.uiAccent)
                        multiline: true
                    }

                    Button {
                        visible: bookManager.scanFolders.length > 0
                        leftMargin: bookManager.dp(8)
                        text: qsTr("Tự động") + Retranslate.onLanguageChanged
                        onClicked: {
                            bookManager.clearScanFolders();
                            scanFoldersSheet.refreshFolders();
                        }
                    }
                }

                ListView {
                    visible: bookManager.scanFolders.length > 0
                    topMargin: bookManager.dp(8)
                    horizontalAlignment: HorizontalAlignment.Fill
                    preferredHeight: Math.min(bookManager.dp(120), bookManager.scanFolders.length * bookManager.dp(42))
                    dataModel: ArrayDataModel { id: selectedFoldersModel }

                    listItemComponents: [
                        ListItemComponent {
                            type: ""
                            CustomListItem {
                                id: selFolderRow
                                dividerVisible: false
                                highlightAppearance: HighlightAppearance.Frame

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    bottomPadding: bookManager.dp(4)

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        background: Color.create(bookManager.uiSurface)
                                        topPadding: bookManager.dp(8)
                                        bottomPadding: bookManager.dp(8)
                                        leftPadding: bookManager.dp(12)
                                        rightPadding: bookManager.dp(12)
                                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                        Label {
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            verticalAlignment: VerticalAlignment.Center
                                            text: ListItemData.path ? ListItemData.path : ""
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: Color.create(bookManager.uiInk)
                                            autoSize.maxLineCount: 1
                                        }

                                        Label {
                                            leftMargin: bookManager.dp(8)
                                            verticalAlignment: VerticalAlignment.Center
                                            text: "✕"
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: Color.create(bookManager.uiDanger)
                                        }
                                    }
                                }
                            }
                        }
                    ]

                    onTriggered: {
                        var item = dataModel.data(indexPath);
                        if (item && item.path) {
                            bookManager.toggleScanFolder(item.path);
                            scanFoldersSheet.refreshFolders();
                        }
                    }
                }
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: 1
                background: Color.create(bookManager.uiHairline)
            }

            // Current Path Bar + Up / Select Current Folder
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(bookManager.uiSurface)
                topPadding: bookManager.dp(8)
                bottomPadding: bookManager.dp(8)
                leftPadding: bookManager.dp(12)
                rightPadding: bookManager.dp(12)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                Button {
                    visible: scanFoldersSheet.currentDirPath !== "root"
                    rightMargin: bookManager.dp(8)
                    text: qsTr("Lùi lại") + Retranslate.onLanguageChanged
                    onClicked: {
                        scanFoldersSheet.goUp();
                    }
                }

                Label {
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    verticalAlignment: VerticalAlignment.Center
                    text: scanFoldersSheet.currentDirPath === "root"
                          ? (qsTr("Chọn ổ nhớ để duyệt thư mục") + Retranslate.onLanguageChanged)
                          : scanFoldersSheet.currentDirPath
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: Color.create(bookManager.uiMuted)
                    autoSize.maxLineCount: 1
                }

                Button {
                    visible: scanFoldersSheet.currentDirPath !== "root"
                    leftMargin: bookManager.dp(8)
                    appearance: scanFoldersSheet.currentDirSelected ? ControlAppearance.Primary : ControlAppearance.Default
                    text: (scanFoldersSheet.currentDirSelected ? qsTr("✓ Đang chọn") : qsTr("+ Chọn mục này")) + Retranslate.onLanguageChanged
                    onClicked: {
                        scanFoldersSheet.toggleFolder(scanFoldersSheet.currentDirPath);
                    }
                }
            }

            // Folder Browser ListView
            ListView {
                id: folderPickerListView
                scrollRole: ScrollRole.Main
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                dataModel: ArrayDataModel { id: browseFoldersModel }

                function fwdToggle(path) {
                    scanFoldersSheet.toggleFolder(path);
                }

                listItemComponents: [
                    ListItemComponent {
                        type: ""
                        CustomListItem {
                            id: browseFolderRow
                            dividerVisible: false
                            highlightAppearance: HighlightAppearance.Frame

                            contextActions: [
                                ActionSet {
                                    title: ListItemData.name ? ListItemData.name : ""

                                    ActionItem {
                                        title: (ListItemData.isSelected ? qsTr("Bỏ chọn thư mục này") : qsTr("Chọn thư mục này")) + Retranslate.onLanguageChanged
                                        imageSource: ListItemData.isSelected ? "asset:///icons/ic_clear.png" : "asset:///icons/ic_done.png"
                                        onTriggered: {
                                            browseFolderRow.ListItem.view.fwdToggle(ListItemData.path);
                                        }
                                    }
                                }
                            ]

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                background: ListItemData.isSelected ? Color.create(bookManager.uiActive) : Color.create(bookManager.uiBg)

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topPadding: bookManager.dp(14)
                                    bottomPadding: bookManager.dp(14)
                                    leftPadding: bookManager.dp(14)
                                    rightPadding: bookManager.dp(14)
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    ImageView {
                                        preferredWidth: bookManager.dp(32)
                                        preferredHeight: bookManager.dp(32)
                                        verticalAlignment: VerticalAlignment.Center
                                        scalingMethod: ScalingMethod.AspectFit
                                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_folder.png" : "asset:///icons/ic_folder_dark.png"
                                    }

                                    Container {
                                        leftMargin: bookManager.dp(12)
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        verticalAlignment: VerticalAlignment.Center

                                        Label {
                                            text: ListItemData.name ? ListItemData.name : ""
                                            textStyle.base: SystemDefaults.TextStyles.BodyText
                                            textStyle.fontWeight: FontWeight.W500
                                            textStyle.color: ListItemData.isSelected ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiInk)
                                        }

                                        Label {
                                            topMargin: bookManager.dp(2)
                                            text: ListItemData.subtext ? ListItemData.subtext : ""
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: Color.create(bookManager.uiMuted)
                                            autoSize.maxLineCount: 1
                                        }
                                    }

                                    Label {
                                        visible: ListItemData.isSelected ? true : false
                                        leftMargin: bookManager.dp(8)
                                        verticalAlignment: VerticalAlignment.Center
                                        text: qsTr("✓ Đã chọn") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiAccent)
                                    }

                                    Label {
                                        leftMargin: bookManager.dp(8)
                                        verticalAlignment: VerticalAlignment.Center
                                        text: "›"
                                        textStyle.base: SystemDefaults.TextStyles.TitleText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                    }
                                }

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    preferredHeight: 1
                                    background: Color.create(bookManager.uiHairline)
                                }
                            }
                        }
                    }
                ]

                onTriggered: {
                    var item = dataModel.data(indexPath);
                    if (item && item.path) {
                        openSubfolder(item.path);
                    }
                }
            }
        }
    }
}
