import bb.cascades 1.3
import bb.system 1.2

Sheet {
    id: wifiDropSheet

    signal bookOpenRequested(string filePath)

    onOpened: {
        wifiDropServer.refreshIp();
        if (!wifiDropServer.running) {
            wifiDropServer.start();
        }
        refreshModel();
    }

    function refreshModel() {
        dropFilesModel.clear();
        var files = wifiDropServer.uploadedFiles;
        for (var i = 0; i < files.length; ++i) {
            dropFilesModel.append(files[i]);
        }
    }

    Page {
        titleBar: TitleBar {
            title: qsTr("Truyền sách Wi-Fi") + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain
            dismissAction: ActionItem {
                title: qsTr("Đóng") + Retranslate.onLanguageChanged
                onTriggered: {
                    wifiDropSheet.close();
                }
            }
            acceptAction: ActionItem {
                title: qsTr("Làm mới") + Retranslate.onLanguageChanged
                onTriggered: {
                    wifiDropServer.refreshIp();
                    if (!wifiDropServer.running) {
                        wifiDropServer.start();
                    }
                    refreshModel();
                }
            }
        }

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create(bookManager.uiBg)
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            // Top Server Control Block
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: bookManager.dp(16)
                bottomPadding: bookManager.dp(16)
                leftPadding: bookManager.dp(16)
                rightPadding: bookManager.dp(16)
                background: Color.create(bookManager.uiCard)
                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                // Header with Toggle
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    ImageView {
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_wifi.png" : "asset:///icons/ic_wifi_dark.png"
                        preferredWidth: bookManager.dp(32)
                        preferredHeight: bookManager.dp(32)
                        verticalAlignment: VerticalAlignment.Center
                        scalingMethod: ScalingMethod.AspectFit
                    }

                    Label {
                        text: qsTr("Máy chủ WiFi Book Drop") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SubtitleText
                        textStyle.fontWeight: FontWeight.W500
                        textStyle.color: Color.create(bookManager.uiInk)
                        verticalAlignment: VerticalAlignment.Center
                        leftMargin: bookManager.dp(10)
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    }

                    ToggleButton {
                        checked: wifiDropServer.running
                        verticalAlignment: VerticalAlignment.Center
                        onCheckedChanged: {
                            if (checked) {
                                wifiDropServer.start();
                            } else {
                                wifiDropServer.stop();
                            }
                        }
                    }
                }

                // Server Status & URL Surface (no 1px border wrapper)
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topMargin: bookManager.dp(14)
                    topPadding: bookManager.dp(14)
                    bottomPadding: bookManager.dp(14)
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    background: wifiDropServer.running ?
                                Color.create(bookManager.uiActive) :
                                Color.create(bookManager.uiSurface)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    Label {
                        text: wifiDropServer.running ?
                              (qsTr("● MÁY CHỦ ĐANG HOẠT ĐỘNG") + Retranslate.onLanguageChanged) :
                              (qsTr("○ MÁY CHỦ ĐÃ TẠM DỪNG") + Retranslate.onLanguageChanged)
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.W500
                        textStyle.color: wifiDropServer.running ?
                                         Color.create(bookManager.uiAccent) :
                                         Color.create(bookManager.uiMuted)
                    }

                    Label {
                        visible: wifiDropServer.running && wifiDropServer.serverUrl.length > 0
                        topMargin: bookManager.dp(6)
                        text: wifiDropServer.serverUrl
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiAccent)
                        multiline: true
                    }

                    Container {
                        visible: wifiDropServer.running && wifiDropServer.accessPin.length > 0
                        horizontalAlignment: HorizontalAlignment.Fill
                        topMargin: bookManager.dp(10)
                        topPadding: bookManager.dp(12)
                        bottomPadding: bookManager.dp(12)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)
                        background: Color.create(bookManager.uiCard)
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        Container {
                            verticalAlignment: VerticalAlignment.Center
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            Label {
                                text: qsTr("MÃ PIN TRUY CẬP") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }
                            Label {
                                topMargin: bookManager.dp(2)
                                text: qsTr("Nhập mã này trên trình duyệt để mở khóa") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                                multiline: true
                            }
                        }

                        Label {
                            verticalAlignment: VerticalAlignment.Center
                            leftMargin: bookManager.dp(12)
                            text: wifiDropServer.accessPin.split("").join(" ")
                            textStyle.base: SystemDefaults.TextStyles.BigText
                            textStyle.fontSize: FontSize.PointValue
                            textStyle.fontSizeValue: 18
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create(bookManager.uiAccent)
                        }
                    }

                    Label {
                        visible: wifiDropServer.running && wifiDropServer.serverUrl.length === 0
                        topMargin: bookManager.dp(6)
                        text: qsTr("Chưa phát hiện địa chỉ Wi-Fi. Vui lòng kết nối vào mạng Wi-Fi.") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create(bookManager.uiDanger)
                        multiline: true
                    }

                    Label {
                        topMargin: bookManager.dp(6)
                        text: wifiDropServer.running ?
                              (qsTr("Mở trình duyệt trên máy tính hoặc điện thoại trong cùng mạng Wi-Fi và truy cập địa chỉ trên để truyền sách không dây.") + Retranslate.onLanguageChanged) :
                              (qsTr("Bật công tắc phía trên để kích hoạt máy chủ nhận sách không dây.") + Retranslate.onLanguageChanged)
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create(bookManager.uiMuted)
                        multiline: true
                    }
                }

                // Target path caption
                Label {
                    topMargin: bookManager.dp(10)
                    multiline: true
                    text: qsTr("Thư mục lưu: /accounts/1000/shared/books (Tự động cập nhật thư viện)") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: Color.create(bookManager.uiMuted)
                }
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: 1
                background: Color.create(bookManager.uiHairline)
            }

            // Section Header: Session Uploads
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: bookManager.dp(14)
                bottomPadding: bookManager.dp(8)
                leftPadding: bookManager.dp(16)
                rightPadding: bookManager.dp(16)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                Label {
                    text: qsTr("SÁCH ĐÃ NHẬN TRONG PHIÊN") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.W500
                    textStyle.color: Color.create(bookManager.uiMuted)
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                }

                Label {
                    text: (wifiDropServer.sessionUploadCount + " " + (qsTr("tệp") + Retranslate.onLanguageChanged))
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.W500
                    textStyle.color: Color.create(bookManager.uiAccent)
                    verticalAlignment: VerticalAlignment.Center
                }
            }

            // Empty state view
            Container {
                visible: wifiDropServer.sessionUploadCount === 0
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: bookManager.dp(40)
                bottomPadding: bookManager.dp(32)
                leftPadding: bookManager.dp(24)
                rightPadding: bookManager.dp(24)
                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                ImageView {
                    imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_download.png" : "asset:///icons/ic_download_dark.png"
                    preferredWidth: bookManager.dp(68)
                    preferredHeight: bookManager.dp(68)
                    horizontalAlignment: HorizontalAlignment.Center
                    scalingMethod: ScalingMethod.AspectFit
                    opacity: 0.45
                }

                Label {
                    topMargin: bookManager.dp(14)
                    horizontalAlignment: HorizontalAlignment.Center
                    text: qsTr("Chưa có sách nào được truyền") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                    textStyle.fontWeight: FontWeight.W500
                    textStyle.color: Color.create(bookManager.uiInk)
                }

                Label {
                    topMargin: bookManager.dp(6)
                    horizontalAlignment: HorizontalAlignment.Center
                    multiline: true
                    textStyle.textAlign: TextAlign.Center
                    text: qsTr("Hỗ trợ EPUB, PDF, MOBI, AZW3, PRC, FB2, TXT, CBZ, DJVU.") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: Color.create(bookManager.uiMuted)
                }
            }

            // Full-bleed list of uploaded files
            ListView {
                id: dropFilesListView
                scrollRole: ScrollRole.Main
                visible: wifiDropServer.sessionUploadCount > 0
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

                function itemType(data, indexPath) {
                    return "item";
                }

                function readDroppedBook(path) {
                    if (path) {
                        wifiDropSheet.close();
                        app.openBook(path);
                    }
                }

                dataModel: ArrayDataModel {
                    id: dropFilesModel
                }

                listItemComponents: [
                    ListItemComponent {
                        type: "item"
                        CustomListItem {
                            id: dropItemCell
                            dividerVisible: false
                            highlightAppearance: HighlightAppearance.Full

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                background: Color.create(bookManager.uiBg)

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topPadding: bookManager.dp(12)
                                    bottomPadding: bookManager.dp(12)
                                    leftPadding: bookManager.dp(16)
                                    rightPadding: bookManager.dp(16)
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    // Format Pill
                                    Container {
                                        minWidth: bookManager.dp(52)
                                        leftPadding: bookManager.dp(8)
                                        rightPadding: bookManager.dp(8)
                                        topPadding: bookManager.dp(6)
                                        bottomPadding: bookManager.dp(6)
                                        background: Color.create(bookManager.uiSurface)
                                        verticalAlignment: VerticalAlignment.Center
                                        layout: DockLayout {}

                                        Label {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            verticalAlignment: VerticalAlignment.Center
                                            text: ListItemData.format ? ListItemData.format : "BOOK"
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.W500
                                            textStyle.color: Color.create(bookManager.uiAccent)
                                        }
                                    }

                                    // File Details
                                    Container {
                                        leftMargin: bookManager.dp(12)
                                        rightMargin: bookManager.dp(8)
                                        verticalAlignment: VerticalAlignment.Center
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                        Label {
                                            text: ListItemData.name ? ListItemData.name : ""
                                            textStyle.base: SystemDefaults.TextStyles.PrimaryText
                                            textStyle.fontWeight: FontWeight.W500
                                            textStyle.color: Color.create(bookManager.uiInk)
                                            multiline: false
                                            autoSize.maxLineCount: 1
                                        }

                                        Container {
                                            topMargin: bookManager.dp(2)
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Label {
                                                text: ListItemData.size ? ListItemData.size : ""
                                                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                                textStyle.color: Color.create(bookManager.uiMuted)
                                            }

                                            Label {
                                                visible: ListItemData.time ? true : false
                                                leftMargin: bookManager.dp(8)
                                                text: ListItemData.time ? ("• " + ListItemData.time) : ""
                                                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                                textStyle.color: Color.create(bookManager.uiMuted)
                                            }
                                        }
                                    }

                                    // Read Now CTA (auto-sized)
                                    Button {
                                        text: qsTr("Đọc") + Retranslate.onLanguageChanged
                                        appearance: ControlAppearance.Primary
                                        verticalAlignment: VerticalAlignment.Center
                                        onClicked: {
                                            dropItemCell.ListItem.view.readDroppedBook(ListItemData.path);
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
                    if (item && item.path) {
                        wifiDropSheet.close();
                        app.openBook(item.path);
                    }
                }
            }
        }
    }

    onCreationCompleted: {
        wifiDropServer.uploadedFilesChanged.connect(refreshModel);
    }
}
