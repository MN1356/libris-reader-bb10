import bb.cascades 1.3
import bb.system 1.2

Page {
    id: importPage

    actionBarVisibility: ChromeVisibility.Visible

    titleBar: TitleBar {
        title: qsTr("Nhập Sách") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
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
            title: qsTr("Duyệt thư mục") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_folder.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                app.switchToTab("browser");
            }
        },
        ActionItem {
            title: qsTr("Mở Tệp") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_open_file.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            shortcuts: [
                Shortcut { key: "O" }
            ]
            onTriggered: {
                app.openFilePicker();
            }
        }
    ]

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: Color.create(bookManager.uiBg)
        layout: DockLayout {}

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Top
            preferredHeight: 1
            background: Color.create(bookManager.uiHairline)
        }

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Bottom
            preferredHeight: 1
            background: Color.create(bookManager.uiHairline)
        }

        ScrollView {
            scrollRole: ScrollRole.Main
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                bottomPadding: bookManager.dp(24)

                layout: StackLayout {
                    orientation: LayoutOrientation.TopToBottom
                }

                Header {
                    title: qsTr("NGUỒN NHẬP SÁCH") + Retranslate.onLanguageChanged
                }

                // Grouped Import Sources Surface
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftMargin: bookManager.dp(14)
                    rightMargin: bookManager.dp(14)
                    topMargin: bookManager.dp(8)
                    background: Color.create(bookManager.uiCard)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    // Row 1: Truyền sách Wi-Fi
                    Container {
                        id: wifiRow
                        property bool isPressed: false
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: isPressed ? Color.create(bookManager.uiActive) : Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        onTouch: {
                            if (event.isDown()) isPressed = true;
                            else if (event.isUp() || event.isCancel()) isPressed = false;
                        }
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    app.openWifiDrop();
                                }
                            }
                        ]

                        ImageView {
                            preferredWidth: bookManager.dp(36)
                            preferredHeight: bookManager.dp(36)
                            verticalAlignment: VerticalAlignment.Center
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_wifi.png" : "asset:///icons/ic_wifi_dark.png"
                            scalingMethod: ScalingMethod.AspectFit
                        }

                        Container {
                            leftMargin: bookManager.dp(14)
                            rightMargin: bookManager.dp(10)
                            verticalAlignment: VerticalAlignment.Center
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

                            Label {
                                text: qsTr("Truyền sách Wi-Fi") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiInk)
                            }

                            Label {
                                topMargin: bookManager.dp(2)
                                multiline: true
                                text: wifiDropServer.running ? wifiDropServer.serverUrl : (qsTr("Chạm để bật máy chủ") + Retranslate.onLanguageChanged)
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: wifiDropServer.running ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiMuted)
                            }
                        }

                        Container {
                            verticalAlignment: VerticalAlignment.Center
                            background: wifiDropServer.running ? Color.create("#D4A359") : Color.create(bookManager.uiSurface)
                            leftPadding: bookManager.dp(10)
                            rightPadding: bookManager.dp(10)
                            topPadding: bookManager.dp(4)
                            bottomPadding: bookManager.dp(4)

                            Label {
                                text: (wifiDropServer.running ? qsTr("BẬT") : qsTr("TẮT")) + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: wifiDropServer.running ? Color.create("#0D0B09") : Color.create(bookManager.uiMuted)
                            }
                        }
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 1
                        leftMargin: bookManager.dp(64)
                        background: Color.create(bookManager.uiHairline)
                    }

                    // Row 2: Kho sách OPDS
                    Container {
                        id: opdsRow
                        property bool isPressed: false
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: isPressed ? Color.create(bookManager.uiActive) : Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        onTouch: {
                            if (event.isDown()) isPressed = true;
                            else if (event.isUp() || event.isCancel()) isPressed = false;
                        }
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    app.openOpds();
                                }
                            }
                        ]

                        ImageView {
                            preferredWidth: bookManager.dp(36)
                            preferredHeight: bookManager.dp(36)
                            verticalAlignment: VerticalAlignment.Center
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_opds.png" : "asset:///icons/ic_opds_dark.png"
                            scalingMethod: ScalingMethod.AspectFit
                        }

                        Container {
                            leftMargin: bookManager.dp(14)
                            rightMargin: bookManager.dp(10)
                            verticalAlignment: VerticalAlignment.Center
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

                            Label {
                                text: qsTr("Kho sách OPDS") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiInk)
                            }

                            Label {
                                topMargin: bookManager.dp(2)
                                multiline: true
                                text: qsTr("Tải sách từ thư viện công cộng & Calibre") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }
                        }

                        ImageView {
                            preferredWidth: bookManager.dp(20)
                            preferredHeight: bookManager.dp(20)
                            verticalAlignment: VerticalAlignment.Center
                            opacity: 0.65
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_next.png" : "asset:///icons/ic_next_dark.png"
                            scalingMethod: ScalingMethod.AspectFit
                        }
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 1
                        leftMargin: bookManager.dp(64)
                        background: Color.create(bookManager.uiHairline)
                    }

                    // Row 3: Mở tệp từ máy / Thẻ SD
                    Container {
                        id: openRow
                        property bool isPressed: false
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: isPressed ? Color.create(bookManager.uiActive) : Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        onTouch: {
                            if (event.isDown()) isPressed = true;
                            else if (event.isUp() || event.isCancel()) isPressed = false;
                        }
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    app.openFilePicker();
                                }
                            }
                        ]

                        ImageView {
                            preferredWidth: bookManager.dp(36)
                            preferredHeight: bookManager.dp(36)
                            verticalAlignment: VerticalAlignment.Center
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_open_file.png" : "asset:///icons/ic_open_file_dark.png"
                            scalingMethod: ScalingMethod.AspectFit
                        }

                        Container {
                            leftMargin: bookManager.dp(14)
                            rightMargin: bookManager.dp(10)
                            verticalAlignment: VerticalAlignment.Center
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

                            Label {
                                text: qsTr("Mở Tệp") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiInk)
                            }

                            Label {
                                topMargin: bookManager.dp(2)
                                multiline: true
                                text: qsTr("Chọn tệp từ bộ nhớ máy hoặc thẻ SD") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }
                        }

                        ImageView {
                            preferredWidth: bookManager.dp(20)
                            preferredHeight: bookManager.dp(20)
                            verticalAlignment: VerticalAlignment.Center
                            opacity: 0.65
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_next.png" : "asset:///icons/ic_next_dark.png"
                            scalingMethod: ScalingMethod.AspectFit
                        }
                    }
                }

                Header {
                    topMargin: bookManager.dp(14)
                    title: qsTr("THƯ VIỆN MÁY") + Retranslate.onLanguageChanged
                }

                // Library Sync Surface
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftMargin: bookManager.dp(14)
                    rightMargin: bookManager.dp(14)
                    topMargin: bookManager.dp(8)
                    background: Color.create(bookManager.uiCard)
                    topPadding: bookManager.dp(14)
                    bottomPadding: bookManager.dp(14)
                    leftPadding: bookManager.dp(14)
                    rightPadding: bookManager.dp(14)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        Label {
                            verticalAlignment: VerticalAlignment.Center
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: qsTr("Đồng bộ & Quét thư viện") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SubtitleText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create(bookManager.uiInk)
                        }

                        Container {
                            verticalAlignment: VerticalAlignment.Center
                            background: Color.create(bookManager.uiSurface)
                            leftPadding: bookManager.dp(10)
                            rightPadding: bookManager.dp(10)
                            topPadding: bookManager.dp(4)
                            bottomPadding: bookManager.dp(4)

                            Label {
                                text: (bookManager.totalScannedBooks + " " + (bookManager.language === "en" ? (bookManager.totalScannedBooks === 1 ? "book" : "books") : qsTr("cuốn"))) + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiAccent)
                            }
                        }
                    }

                    Label {
                        topMargin: bookManager.dp(6)
                        multiline: true
                        text: ((bookManager.isScanning && bookManager.scanStatusText !== "") ? bookManager.scanStatusText : qsTr("Đã sẵn sàng")) + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: bookManager.isScanning ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiMuted)
                    }

                    Button {
                        topMargin: bookManager.dp(12)
                        horizontalAlignment: HorizontalAlignment.Fill
                        appearance: ControlAppearance.Primary
                        text: qsTr("Quét lại") + Retranslate.onLanguageChanged
                        onClicked: {
                            bookManager.scanLibrary();
                            importToast.body = qsTr("Đang nạp và quét thư viện sách...") + Retranslate.onLanguageChanged;
                            importToast.show();
                        }
                    }
                }
            }
        }
    }

    attachedObjects: [
        SystemToast {
            id: importToast
            body: ""
        }
    ]
}
