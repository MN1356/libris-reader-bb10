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
            imageSource: "asset:///icons/ic_clear.png"
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
        },
        ActionItem {
            title: qsTr("Hủy bỏ") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_clear.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            onTriggered: {
                addCatalogPage.cancelRequested();
            }
        }
    ]

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
        layout: DockLayout {}

        ScrollView {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            scrollViewProperties.scrollMode: ScrollMode.Vertical

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: 14
                bottomPadding: 32
                leftPadding: 16
                rightPadding: 16

                layout: StackLayout {
                    orientation: LayoutOrientation.TopToBottom
                }

                // =============================================================
                // 1. THÔNG TIN KHO SÁCH (CHÍNH)
                // =============================================================
                Header {
                    title: qsTr("THÔNG TIN KHO SÁCH") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                    topPadding: 14
                    bottomPadding: 16
                    leftPadding: 16
                    rightPadding: 16
                    bottomMargin: 18

                    layout: StackLayout {
                        orientation: LayoutOrientation.TopToBottom
                    }

                    // Tên kho sách
                    Label {
                        text: qsTr("Tên hiển thị:") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                    }

                    TextField {
                        id: catNameField
                        topMargin: 2
                        text: addCatalogPage.catalogName
                        hintText: qsTr("Ví dụ: Calibre Của Tôi, Kavita Server...") + Retranslate.onLanguageChanged
                        clearButtonVisible: true
                    }

                    // Địa chỉ URL
                    Label {
                        topMargin: 12
                        text: qsTr("Địa chỉ OPDS URL (*):") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                    }

                    TextField {
                        id: catUrlField
                        topMargin: 2
                        text: addCatalogPage.catalogUrl
                        hintText: "http://192.168.1.100:8080/opds"
                        inputMode: TextFieldInputMode.Url
                        clearButtonVisible: true
                    }

                    // Ghi chú mô tả
                    Label {
                        topMargin: 12
                        text: qsTr("Mô tả / Ghi chú (tùy chọn):") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#A1A1AA") : Color.create("#777777")
                    }

                    TextField {
                        id: catDescField
                        topMargin: 2
                        text: addCatalogPage.catalogDesc
                        hintText: qsTr("Thư viện sách cá nhân trong gia đình") + Retranslate.onLanguageChanged
                        clearButtonVisible: true
                    }
                }

                // =============================================================
                // 2. GỢI Ý MẪU KHO SÁCH PHỔ BIẾN (CLICK ĐIỀN NHANH)
                // =============================================================
                Header {
                    title: qsTr("GỢI Ý MẪU KHO SÁCH (CHẠM ĐỂ ĐIỀN NHANH)") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    bottomMargin: 18

                    layout: StackLayout {
                        orientation: LayoutOrientation.TopToBottom
                    }

                    // Mẫu 1: Calibre Content Server
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 12
                        bottomPadding: 12
                        leftPadding: 14
                        rightPadding: 14
                        bottomMargin: 8
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    catNameField.text = bookManager.language === "en" ? "Calibre Server" : "Calibre Máy Chủ";
                                    catUrlField.text = "http://192.168.1.100:8080/opds";
                                    catDescField.text = bookManager.language === "en" ? "Calibre server on local Wi-Fi" : "Máy chủ Calibre trong mạng Wi-Fi";
                                }
                            }
                        ]

                        Container {
                            preferredWidth: 42
                            preferredHeight: 42
                            background: bookManager.appTheme === "dark" ? Color.create("#181818") : Color.create("#E0F2FE")
                            verticalAlignment: VerticalAlignment.Center
                            layout: DockLayout {}
                            ImageView {
                                horizontalAlignment: HorizontalAlignment.Center
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: 26
                                preferredHeight: 26
                                imageSource: "asset:///icons/ic_library.png"
                            }
                        }

                        Container {
                            leftMargin: 12
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            verticalAlignment: VerticalAlignment.Center

                            Label {
                                text: "Calibre Content Server (Wi-Fi)"
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                            }

                            Label {
                                topMargin: 1
                                text: (bookManager.language === "en" ? "http://<PC-IP-Address>:8080/opds" : "http://<IP-máy-tính>:8080/opds") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.XSmallText
                                textStyle.color: Color.create("#10B981")
                            }
                        }

                        Label {
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Áp dụng ›") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create("#10B981")
                        }
                    }

                    // Mẫu 2: Kavita Server
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 12
                        bottomPadding: 12
                        leftPadding: 14
                        rightPadding: 14
                        bottomMargin: 8
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    catNameField.text = "Kavita E-Reader";
                                    catUrlField.text = "http://192.168.1.100:5000/api/opds/";
                                    catDescField.text = bookManager.language === "en" ? "Kavita reading server" : "Máy chủ đọc sách Kavita";
                                }
                            }
                        ]

                        Container {
                            preferredWidth: 42
                            preferredHeight: 42
                            background: bookManager.appTheme === "dark" ? Color.create("#2A1B3D") : Color.create("#F3E8FF")
                            verticalAlignment: VerticalAlignment.Center
                            layout: DockLayout {}
                            ImageView {
                                horizontalAlignment: HorizontalAlignment.Center
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: 26
                                preferredHeight: 26
                                imageSource: "asset:///icons/ic_book.png"
                            }
                        }

                        Container {
                            leftMargin: 12
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            verticalAlignment: VerticalAlignment.Center

                            Label {
                                text: "Kavita E-Reader Server"
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                            }

                            Label {
                                topMargin: 1
                                text: "http://<IP>:5000/api/opds/<api-key>"
                                textStyle.base: SystemDefaults.TextStyles.XSmallText
                                textStyle.color: Color.create("#10B981")
                            }
                        }

                        Label {
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Áp dụng ›") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create("#10B981")
                        }
                    }

                    // Mẫu 3: Komga Server
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 12
                        bottomPadding: 12
                        leftPadding: 14
                        rightPadding: 14
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    catNameField.text = "Komga Server";
                                    catUrlField.text = "http://192.168.1.100:25600/opds/v1.0/catalog";
                                    catDescField.text = bookManager.language === "en" ? "Komga comics and book library" : "Thư viện truyện và sách Komga";
                                }
                            }
                        ]

                        Container {
                            preferredWidth: 42
                            preferredHeight: 42
                            background: bookManager.appTheme === "dark" ? Color.create("#1E2D28") : Color.create("#DCFCE7")
                            verticalAlignment: VerticalAlignment.Center
                            layout: DockLayout {}
                            ImageView {
                                horizontalAlignment: HorizontalAlignment.Center
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: 26
                                preferredHeight: 26
                                imageSource: "asset:///icons/ic_folder.png"
                            }
                        }

                        Container {
                            leftMargin: 12
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            verticalAlignment: VerticalAlignment.Center

                            Label {
                                text: "Komga Comics / Books Server"
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                            }

                            Label {
                                topMargin: 1
                                text: "http://<IP>:25600/opds/v1.0/catalog"
                                textStyle.base: SystemDefaults.TextStyles.XSmallText
                                textStyle.color: Color.create("#10B981")
                            }
                        }

                        Label {
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Áp dụng ›") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create("#10B981")
                        }
                    }
                }

                // =============================================================
                // 3. TÀI KHOẢN XÁC THỰC (NẾU CÓ)
                // =============================================================
                Header {
                    title: qsTr("XÁC THỰC TÀI KHOẢN (NẾU CÓ)") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                    topPadding: 14
                    bottomPadding: 16
                    leftPadding: 16
                    rightPadding: 16
                    bottomMargin: 18

                    layout: StackLayout {
                        orientation: LayoutOrientation.TopToBottom
                    }

                    Label {
                        text: qsTr("Nếu máy chủ yêu cầu đăng nhập (HTTP Basic Auth), hãy nhập thông tin bên dưới:") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#A1A1AA") : Color.create("#777777")
                        multiline: true
                        bottomMargin: 8
                    }

                    // Username
                    Label {
                        text: qsTr("Tên đăng nhập (Username):") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                    }

                    TextField {
                        id: catUserField
                        topMargin: 2
                        text: addCatalogPage.catalogUser
                        hintText: qsTr("Để trống nếu không yêu cầu mật khẩu") + Retranslate.onLanguageChanged
                        clearButtonVisible: true
                    }

                    // Password
                    Label {
                        topMargin: 12
                        text: qsTr("Mật khẩu (Password):") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                    }

                    TextField {
                        id: catPassField
                        topMargin: 2
                        text: addCatalogPage.catalogPass
                        inputMode: TextFieldInputMode.Password
                        hintText: qsTr("Mật khẩu truy cập máy chủ") + Retranslate.onLanguageChanged
                        clearButtonVisible: true
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
                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                    topPadding: 14
                    bottomPadding: 14
                    leftPadding: 16
                    rightPadding: 16

                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    ImageView {
                        preferredWidth: 32
                        preferredHeight: 32
                        verticalAlignment: VerticalAlignment.Top
                        imageSource: "asset:///icons/ic_info.png"
                    }

                    Container {
                        leftMargin: 12
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        verticalAlignment: VerticalAlignment.Top

                        Label {
                            text: qsTr("Các bước kết nối với máy tính:") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                        }

                        Label {
                            topMargin: 6
                            text: qsTr("1. Mở Calibre trên máy tính -> Chọn 'Kết nối/chia sẻ' -> 'Khởi động máy chủ nội dung'.\n" +
                                       "2. Đảm bảo thiết bị BlackBerry và máy tính đang cùng kết nối một mạng Wi-Fi.\n" +
                                       "3. Xem địa chỉ IP của máy tính (ví dụ: 192.168.1.15) và nhập dạng: http://192.168.1.15:8080/opds vào ô ở trên.\n" +
                                       "4. Nhấn 'Lưu kho sách' để bắt đầu duyệt toàn bộ thư viện sách!") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#E5E5E5") : Color.create("#777777")
                            multiline: true
                        }
                    }
                }
            }
        }
    }
}
