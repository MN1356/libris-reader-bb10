import bb.cascades 1.3

Page {
    id: addCatalogPage

    property bool isEditing: false
    property string catalogId: ""
    property string catalogName: ""
    property string catalogUrl: ""
    property string catalogUser: ""
    property string catalogPass: ""
    property string catalogDesc: ""

    signal saveRequested(string id, string name, string url, string user, string pass, string desc, bool isEditing)
    signal cancelRequested()

    actionBarVisibility: ChromeVisibility.Visible

    titleBar: TitleBar {
        appearance: TitleBarAppearance.Plain
        title: addCatalogPage.isEditing ?
               (qsTr("Chỉnh Sửa Kho") + Retranslate.onLanguageChanged) :
               (qsTr("Thêm Kho Sách OPDS") + Retranslate.onLanguageChanged)

        dismissAction: ActionItem {
            title: qsTr("Hủy") + Retranslate.onLanguageChanged
            onTriggered: {
                addCatalogPage.cancelRequested();
            }
        }

        acceptAction: ActionItem {
            title: qsTr("Lưu") + Retranslate.onLanguageChanged
            enabled: catUrlField.text.trim().length > 0
            onTriggered: {
                addCatalogPage.saveRequested(
                    addCatalogPage.catalogId,
                    catNameField.text,
                    catUrlField.text,
                    catUserField.text,
                    catPassField.text,
                    catDescField.text,
                    addCatalogPage.isEditing
                );
            }
        }
    }

    actions: [
        ActionItem {
            title: qsTr("Lưu kho sách") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_done.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            enabled: catUrlField.text.trim().length > 0
            onTriggered: {
                addCatalogPage.saveRequested(
                    addCatalogPage.catalogId,
                    catNameField.text,
                    catUrlField.text,
                    catUserField.text,
                    catPassField.text,
                    catDescField.text,
                    addCatalogPage.isEditing
                );
            }
        }
    ]

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: Color.create(bookManager.uiBg)

        ScrollView {
            scrollRole: ScrollRole.Main
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            scrollViewProperties.scrollMode: ScrollMode.Vertical

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                bottomPadding: bookManager.dp(32)
                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                // =============================================================
                // 1. THÔNG TIN KHO SÁCH (CHÍNH)
                // =============================================================
                Header {
                    title: qsTr("THÔNG TIN KHO SÁCH") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(16)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(16)
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Label {
                            text: qsTr("Tên hiển thị:") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiInk)
                        }

                        TextField {
                            id: catNameField
                            topMargin: bookManager.dp(4)
                            text: addCatalogPage.catalogName
                            hintText: qsTr("Ví dụ: Calibre Của Tôi, Kavita Server...") + Retranslate.onLanguageChanged
                            clearButtonVisible: true
                        }

                        Label {
                            topMargin: bookManager.dp(12)
                            text: qsTr("Địa chỉ OPDS URL (*):") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiInk)
                        }

                        TextField {
                            id: catUrlField
                            topMargin: bookManager.dp(4)
                            text: addCatalogPage.catalogUrl
                            hintText: "http://192.168.1.100:8080/opds"
                            inputMode: TextFieldInputMode.Url
                            clearButtonVisible: true
                        }

                        Label {
                            topMargin: bookManager.dp(12)
                            text: qsTr("Mô tả / Ghi chú (tùy chọn):") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }

                        TextField {
                            id: catDescField
                            topMargin: bookManager.dp(4)
                            text: addCatalogPage.catalogDesc
                            hintText: qsTr("Thư viện sách cá nhân trong gia đình") + Retranslate.onLanguageChanged
                            clearButtonVisible: true
                        }
                    }
                }

                // =============================================================
                // 2. GỢI Ý MẪU KHO SÁCH PHỔ BIẾN (Full-bleed pressed rows)
                // =============================================================
                Header {
                    title: qsTr("GỢI Ý MẪU KHO SÁCH (CHẠM ĐỂ ĐIỀN NHANH)") + Retranslate.onLanguageChanged
                }

                // Mẫu 1: Calibre Content Server
                Container {
                    id: presetCalibre
                    property bool pressed: false
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: pressed ? Color.create(bookManager.uiActive) : Color.Transparent
                    topPadding: bookManager.dp(12)
                    bottomPadding: bookManager.dp(12)
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    onTouch: {
                        if (event.touchType === TouchType.Down) pressed = true;
                        else if (event.touchType === TouchType.Up || event.touchType === TouchType.Cancel) pressed = false;
                    }
                    gestureHandlers: [
                        TapHandler {
                            onTapped: {
                                catNameField.text = qsTr("Calibre Máy Chủ");
                                catUrlField.text = "http://192.168.1.100:8080/opds";
                                catDescField.text = qsTr("Máy chủ Calibre trong mạng Wi-Fi");
                            }
                        }
                    ]

                    Container {
                        preferredWidth: bookManager.dp(40)
                        preferredHeight: bookManager.dp(40)
                        background: Color.create(bookManager.uiSurface)
                        verticalAlignment: VerticalAlignment.Center
                        layout: DockLayout {}
                        ImageView {
                            horizontalAlignment: HorizontalAlignment.Center
                            verticalAlignment: VerticalAlignment.Center
                            preferredWidth: bookManager.dp(24)
                            preferredHeight: bookManager.dp(24)
                            scalingMethod: ScalingMethod.AspectFit
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_library.png" : "asset:///icons/ic_library_dark.png"
                        }
                    }

                    Container {
                        leftMargin: bookManager.dp(12)
                        rightMargin: bookManager.dp(8)
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        verticalAlignment: VerticalAlignment.Center

                        Label {
                            text: "Calibre Content Server (Wi-Fi)"
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiInk)
                        }

                        Label {
                            topMargin: bookManager.dp(2)
                            text: qsTr("http://<IP-máy-tính>:8080/opds") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiAccent)
                        }
                    }

                    Label {
                        verticalAlignment: VerticalAlignment.Center
                        text: qsTr("Áp dụng ›") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.W500
                        textStyle.color: Color.create(bookManager.uiAccent)
                    }
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftMargin: bookManager.dp(16)
                    preferredHeight: 1
                    background: Color.create(bookManager.uiHairline)
                }

                // Mẫu 2: Kavita Server
                Container {
                    id: presetKavita
                    property bool pressed: false
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: pressed ? Color.create(bookManager.uiActive) : Color.Transparent
                    topPadding: bookManager.dp(12)
                    bottomPadding: bookManager.dp(12)
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    onTouch: {
                        if (event.touchType === TouchType.Down) pressed = true;
                        else if (event.touchType === TouchType.Up || event.touchType === TouchType.Cancel) pressed = false;
                    }
                    gestureHandlers: [
                        TapHandler {
                            onTapped: {
                                catNameField.text = "Kavita E-Reader";
                                catUrlField.text = "http://192.168.1.100:5000/api/opds/";
                                catDescField.text = qsTr("Máy chủ đọc sách Kavita");
                            }
                        }
                    ]

                    Container {
                        preferredWidth: bookManager.dp(40)
                        preferredHeight: bookManager.dp(40)
                        background: Color.create(bookManager.uiSurface)
                        verticalAlignment: VerticalAlignment.Center
                        layout: DockLayout {}
                        ImageView {
                            horizontalAlignment: HorizontalAlignment.Center
                            verticalAlignment: VerticalAlignment.Center
                            preferredWidth: bookManager.dp(24)
                            preferredHeight: bookManager.dp(24)
                            scalingMethod: ScalingMethod.AspectFit
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_book.png" : "asset:///icons/ic_book_dark.png"
                        }
                    }

                    Container {
                        leftMargin: bookManager.dp(12)
                        rightMargin: bookManager.dp(8)
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        verticalAlignment: VerticalAlignment.Center

                        Label {
                            text: "Kavita E-Reader Server"
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiInk)
                        }

                        Label {
                            topMargin: bookManager.dp(2)
                            text: "http://<IP>:5000/api/opds/<api-key>"
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiAccent)
                        }
                    }

                    Label {
                        verticalAlignment: VerticalAlignment.Center
                        text: qsTr("Áp dụng ›") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.W500
                        textStyle.color: Color.create(bookManager.uiAccent)
                    }
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftMargin: bookManager.dp(16)
                    preferredHeight: 1
                    background: Color.create(bookManager.uiHairline)
                }

                // Mẫu 3: Komga Server
                Container {
                    id: presetKomga
                    property bool pressed: false
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: pressed ? Color.create(bookManager.uiActive) : Color.Transparent
                    topPadding: bookManager.dp(12)
                    bottomPadding: bookManager.dp(12)
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    onTouch: {
                        if (event.touchType === TouchType.Down) pressed = true;
                        else if (event.touchType === TouchType.Up || event.touchType === TouchType.Cancel) pressed = false;
                    }
                    gestureHandlers: [
                        TapHandler {
                            onTapped: {
                                catNameField.text = "Komga Server";
                                catUrlField.text = "http://192.168.1.100:25600/opds/v1.2/catalog";
                                catDescField.text = qsTr("Thư viện truyện và sách Komga");
                            }
                        }
                    ]

                    Container {
                        preferredWidth: bookManager.dp(40)
                        preferredHeight: bookManager.dp(40)
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
                            text: "Komga Comics / Books Server"
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiInk)
                        }

                        Label {
                            topMargin: bookManager.dp(2)
                            text: "http://<IP>:25600/opds/v1.2/catalog"
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiAccent)
                        }
                    }

                    Label {
                        verticalAlignment: VerticalAlignment.Center
                        text: qsTr("Áp dụng ›") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.W500
                        textStyle.color: Color.create(bookManager.uiAccent)
                    }
                }

                // =============================================================
                // 3. TÀI KHOẢN XÁC THỰC (NẾU CÓ)
                // =============================================================
                Header {
                    topMargin: bookManager.dp(8)
                    title: qsTr("XÁC THỰC TÀI KHOẢN (NẾU CÓ)") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(16)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(16)
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Label {
                            text: qsTr("Nếu máy chủ yêu cầu đăng nhập (HTTP Basic Auth), hãy nhập thông tin bên dưới:") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiMuted)
                            multiline: true
                            bottomMargin: bookManager.dp(10)
                        }

                        Label {
                            text: qsTr("Tên đăng nhập (Username):") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiInk)
                        }

                        TextField {
                            id: catUserField
                            topMargin: bookManager.dp(4)
                            text: addCatalogPage.catalogUser
                            hintText: qsTr("Để trống nếu không yêu cầu mật khẩu") + Retranslate.onLanguageChanged
                            clearButtonVisible: true
                        }

                        Label {
                            topMargin: bookManager.dp(12)
                            text: qsTr("Mật khẩu (Password):") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiInk)
                        }

                        TextField {
                            id: catPassField
                            topMargin: bookManager.dp(4)
                            text: addCatalogPage.catalogPass
                            inputMode: TextFieldInputMode.Password
                            hintText: qsTr("Mật khẩu truy cập máy chủ") + Retranslate.onLanguageChanged
                            clearButtonVisible: true
                        }
                    }
                }

                // =============================================================
                // 4. HƯỚNG DẪN KẾT NỐI CALIBRE
                // =============================================================
                Header {
                    title: qsTr("HƯỚNG DẪN KẾT NỐI CALIBRE NHANH") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        ImageView {
                            preferredWidth: bookManager.dp(28)
                            preferredHeight: bookManager.dp(28)
                            verticalAlignment: VerticalAlignment.Top
                            scalingMethod: ScalingMethod.AspectFit
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_info.png" : "asset:///icons/ic_info_dark.png"
                        }

                        Container {
                            leftMargin: bookManager.dp(12)
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            verticalAlignment: VerticalAlignment.Top

                            Label {
                                text: qsTr("Các bước kết nối với máy tính:") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.fontWeight: FontWeight.W500
                                textStyle.color: Color.create(bookManager.uiInk)
                            }

                            Label {
                                topMargin: bookManager.dp(6)
                                text: qsTr("1. Mở Calibre trên máy tính -> Chọn 'Kết nối/chia sẻ' -> 'Khởi động máy chủ nội dung'.") + "\n" +
                                      qsTr("2. Đảm bảo thiết bị BlackBerry và máy tính đang cùng kết nối một mạng Wi-Fi.") + "\n" +
                                      qsTr("3. Xem địa chỉ IP của máy tính (ví dụ: 192.168.1.15) và nhập dạng: http://192.168.1.15:8080/opds vào ô ở trên.") + "\n" +
                                      qsTr("4. Nhấn 'Lưu kho sách' để bắt đầu duyệt toàn bộ thư viện sách!") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                                multiline: true
                            }
                        }
                    }
                }
            }
        }
    }
}
