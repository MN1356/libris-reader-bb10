import bb.cascades 1.3

Page {
    id: aboutPage

    signal closeRequested()

    actionBarVisibility: ChromeVisibility.Visible

    titleBar: TitleBar {
        title: qsTr("Thông Tin Ứng Dụng") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
        dismissAction: ActionItem {
            title: qsTr("Đóng") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_previous.png"
            onTriggered: {
                aboutPage.closeRequested();
            }
        }
    }

    actions: [
        ActionItem {
            title: qsTr("Đóng") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_previous.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            shortcuts: [
                Shortcut { key: "Back" }
            ]
            onTriggered: {
                aboutPage.closeRequested();
            }
        },
        ActionItem {
            title: qsTr("Cài Đặt") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_settings.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            shortcuts: [
                Shortcut { key: "S" }
            ]
            onTriggered: {
                aboutPage.closeRequested();
                app.openSettings();
            }
        }
    ]

    Container {
        layout: DockLayout {}
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill

            // Top Segmented Control (About | Help | Changes)
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                topPadding: 8
                bottomPadding: 8
                leftPadding: 14
                rightPadding: 14

                SegmentedControl {
                    id: aboutSegmentedControl
                    horizontalAlignment: HorizontalAlignment.Fill

                    Option {
                        id: optAbout
                        text: qsTr("Giới thiệu") + Retranslate.onLanguageChanged
                        value: "about"
                        selected: true
                    }

                    Option {
                        id: optHelp
                        text: qsTr("Hướng dẫn") + Retranslate.onLanguageChanged
                        value: "help"
                    }

                    Option {
                        id: optChanges
                        text: qsTr("Thay đổi") + Retranslate.onLanguageChanged
                        value: "changes"
                    }
                }
            }

            // Scrollable Content
            ScrollView {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                scrollViewProperties.scrollMode: ScrollMode.Vertical

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topPadding: 14
                    bottomPadding: 28
                    leftPadding: 14
                    rightPadding: 14

                    // =========================================================
                    // SECTION 1: ABOUT
                    // =========================================================
                    Container {
                        id: aboutSectionContainer
                        visible: aboutSegmentedControl.selectedValue === "about"
                        horizontalAlignment: HorizontalAlignment.Fill

                        // App Hero Brand Banner
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                            topPadding: 20
                            bottomPadding: 20
                            leftPadding: 16
                            rightPadding: 16
                            bottomMargin: 14

                            layout: StackLayout {
                                orientation: LayoutOrientation.TopToBottom
                            }

                            ImageView {
                                horizontalAlignment: HorizontalAlignment.Center
                                preferredWidth: 80
                                preferredHeight: 80
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_app_dark.png" : "asset:///icons/ic_app_light.png"
                            }

                            Label {
                                topMargin: 10
                                horizontalAlignment: HorizontalAlignment.Center
                                text: "BerryLeaf"
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            }

                            Label {
                                topMargin: 4
                                horizontalAlignment: HorizontalAlignment.Center
                                text: qsTr("Phiên bản 2.0.0 (Build 69) - BlackBerry 10") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            }

                            Label {
                                topMargin: 4
                                horizontalAlignment: HorizontalAlignment.Center
                                multiline: true
                                textStyle.textAlign: TextAlign.Center
                                text: qsTr("Trình đọc sách Native thuần C++ và Cascades cho BlackBerry 10.") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            }
                        }

                        // Header 1: TỔNG QUAN
                        Header {
                            title: qsTr("TỔNG QUAN") + Retranslate.onLanguageChanged
                        }

                        Container {
                            topMargin: 6
                            bottomMargin: 14
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F5F5F5")
                            topPadding: 12
                            bottomPadding: 12
                            leftPadding: 14
                            rightPadding: 14

                            Label {
                                multiline: true
                                text: qsTr("BerryLeaf là ứng dụng đọc sách chuẩn Native Cascades dành cho BlackBerry 10. Hỗ trợ nhiều định dạng, cử chỉ vuốt mượt mà, tra từ điển ngoại tuyến và đọc giọng nói TTS.") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            }
                        }

                        // Header 2: CÔNG NGHỆ & BỘ XỬ LÝ LÕI
                        Header {
                            title: qsTr("CÔNG NGHỆ VÀ BỘ XỬ LÝ LÕI") + Retranslate.onLanguageChanged
                        }

                        Container {
                            topMargin: 6
                            bottomMargin: 14
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F5F5F5")
                            topPadding: 12
                            bottomPadding: 12
                            leftPadding: 14
                            rightPadding: 14

                            Label {
                                multiline: true
                                text: qsTr("Lõi tài liệu: Poppler C++, DjVuLibre, pugixml DOM và miniz.
Bảng mã Tiếng Việt: Nhận diện tự động UTF-8, UTF-16, TCVN3, Windows-1258, VNI.
Đọc văn bản TTS: BlackBerry Multimedia Services, Google TTS và Gemini AI.
Từ điển ngoại tuyến: Cơ sở dữ liệu SQLite tích hợp trên thiết bị.") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            }
                        }

                        // Header 3: ĐỊNH DẠNG HỖ TRỢ
                        Header {
                            title: qsTr("ĐỊNH DẠNG TÀI LIỆU HỖ TRỢ") + Retranslate.onLanguageChanged
                        }

                        Container {
                            topMargin: 6
                            bottomMargin: 14
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F5F5F5")
                            topPadding: 12
                            bottomPadding: 12
                            leftPadding: 14
                            rightPadding: 14

                            Label {
                                multiline: true
                                text: qsTr("EPUB (.epub): Chuẩn EPUB 2.0 và 3.0 với phông chữ nhúng và mục lục NCX.
PDF (.pdf): Trang đồ họa nguyên bản và chế độ cuộn văn bản Reflow.
DJVU (.djvu, .djv): Tài liệu scan nén chất lượng cao.
FB2 (.fb2): Định dạng FictionBook kèm chú thích chân trang.
TXT (.txt): Văn bản thuần tự động phát hiện mã hóa.") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            }
                        }

                        // Header 4: BẢN QUYỀN & TÁC GIẢ
                        Header {
                            title: qsTr("BẢN QUYỀN VÀ GIẤY PHÉP") + Retranslate.onLanguageChanged
                        }

                        Container {
                            topMargin: 6
                            bottomMargin: 14
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F5F5F5")
                            topPadding: 12
                            bottomPadding: 12
                            leftPadding: 14
                            rightPadding: 14

                            Label {
                                multiline: true
                                text: qsTr("Tác giả: NhatLee (BerryLife Developer Team)
Động cơ nsrreadercore: Alexander Saprykin
Giấy phép: GNU General Public License v2 (GPLv2)
Môi trường: BlackBerry 10 Native SDK 10.3.1") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            }
                        }
                    }

                    // =========================================================
                    // SECTION 2: HELP
                    // =========================================================
                    Container {
                        id: helpSectionContainer
                        visible: aboutSegmentedControl.selectedValue === "help"
                        horizontalAlignment: HorizontalAlignment.Fill

                        // Header: CỬ CHỈ ĐIỀU HƯỚNG
                        Header {
                            title: qsTr("CỬ CHỈ ĐIỀU HƯỚNG NATIVE") + Retranslate.onLanguageChanged
                        }

                        Container {
                            topMargin: 6
                            bottomMargin: 14
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F5F5F5")
                            topPadding: 12
                            bottomPadding: 12
                            leftPadding: 14
                            rightPadding: 14

                            Label {
                                multiline: true
                                text: qsTr("Vuốt mép viền trái: Mở thanh điều hướng Sidebar trượt trái.
Vuốt mép viền trên: Mở Menu ứng dụng BlackBerry 10.
Chạm giữa màn hình: Bật hoặc tắt thanh tiêu đề và thanh công cụ.
Vuốt mép viền phải: Mở danh sách tác vụ mở rộng.") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            }
                        }

                        // Header: PHÍM TẮT PHẦN CỨNG
                        Header {
                            title: qsTr("PHÍM TẮT BÀN PHÍM VẬT LÝ") + Retranslate.onLanguageChanged
                        }

                        Container {
                            topMargin: 6
                            bottomMargin: 14
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F5F5F5")
                            topPadding: 12
                            bottomPadding: 12
                            leftPadding: 14
                            rightPadding: 14

                            Label {
                                multiline: true
                                text: qsTr("Space: Sang trang kế tiếp / Tạm dừng đọc TTS
N: Sang trang kế tiếp
P / U: Về trang trước đó
J / K: Cuộn trang xuống / lên
Z / W: Phóng to / Thu nhỏ cỡ chữ
G / M: Mở mục lục sách
T / B: Nhảy về đầu sách / cuối sách
V: Đổi màu nền Ngày / Đêm
F: Tìm kiếm trong sách
A: Bật hoặc tắt đọc giọng nói TTS
D: Tra từ điển ngoại tuyến
H: Đánh dấu trang hiện tại
O: Mở tệp sách mới
S: Mở bảng cài đặt
Q: Thoát hoàn toàn ứng dụng") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            }
                        }

                        // Header: BÀN RÊ & CẢM ỨNG
                        Header {
                            title: qsTr("HỖ TRỢ THIẾT BỊ ĐẶC BIỆT") + Retranslate.onLanguageChanged
                        }

                        Container {
                            topMargin: 6
                            bottomMargin: 14
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F5F5F5")
                            topPadding: 12
                            bottomPadding: 12
                            leftPadding: 14
                            rightPadding: 14

                            Label {
                                multiline: true
                                text: qsTr("BlackBerry Passport: Vuốt trượt trên bàn phím cảm ứng để lật trang hoặc cuộn dòng.
BlackBerry Classic: Sử dụng bàn rê quang học Trackpad để điều hướng và cuộn trang.") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            }
                        }
                    }

                    // =========================================================
                    // SECTION 3: CHANGES
                    // =========================================================
                    Container {
                        id: changesSectionContainer
                        visible: aboutSegmentedControl.selectedValue === "changes"
                        horizontalAlignment: HorizontalAlignment.Fill

                        Header {
                            title: qsTr("NHẬT KÝ PHIÊN BẢN") + Retranslate.onLanguageChanged
                        }

                        Container {
                            topMargin: 6
                            bottomMargin: 14
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F5F5F5")
                            topPadding: 12
                            bottomPadding: 12
                            leftPadding: 14
                            rightPadding: 14

                            Label {
                                multiline: true
                                text: qsTr("Phiên bản 2.0.0 (Build 69):
- Bảng màu đơn sắc Đen, Trắng và điểm nhấn Emerald theo chuẩn BlackBerry 10.
- Loại bỏ toàn bộ thẻ xanh đậm và ký tự rườm rà.
- Nhận diện tên chương thực tế từ nội dung sách EPUB, TXT, FB2.
- Tối ưu tra từ điển ngoại tuyến và đọc giọng nói TTS.
- Điều hướng mượt mà với bàn phím và bàn rê quang học Passport, Classic, Q10.

Phiên bản 2.0.0 (Bản dựng trước):
- Cấu trúc Navigation Drawer 5 tab Native: Đang đọc, Gần đây, Mục lục, Tủ sách, Duyệt tệp.
- Tự động nhận diện bảng mã tiếng Việt TCVN3, Windows-1258, VNI, UTF-8.
- Đồng bộ dữ liệu lịch sử đọc và cấu hình vào bộ nhớ lưu trữ an toàn.") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            }
                        }
                    }
                }
            }
        }
    }
}
