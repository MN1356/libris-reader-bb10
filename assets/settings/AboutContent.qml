/*
 * AboutContent — shared editorial content component used by both AboutPage and AboutSettingsPage.
 */
import bb.cascades 1.3

Container {
    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    background: Color.create(bookManager.uiBg)

    // Top Segmented Control (Giới thiệu | Hướng dẫn | Thay đổi)
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        topPadding: bookManager.dp(10)
        bottomPadding: bookManager.dp(10)
        leftPadding: bookManager.dp(16)
        rightPadding: bookManager.dp(16)

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

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: 1
        background: Color.create(bookManager.uiHairline)
    }

    // Scrollable Content
    ScrollView {
        scrollRole: ScrollRole.Main
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        scrollViewProperties.scrollMode: ScrollMode.Vertical

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            bottomPadding: bookManager.dp(32)

            // =========================================================
            // SECTION 1: ABOUT
            // =========================================================
            Container {
                id: aboutSectionContainer
                visible: aboutSegmentedControl.selectedValue === "about"
                horizontalAlignment: HorizontalAlignment.Fill

                // App Hero Brand Banner (borderless, breathes on uiBg)
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topPadding: bookManager.dp(24)
                    bottomPadding: bookManager.dp(20)
                    leftPadding: bookManager.dp(20)
                    rightPadding: bookManager.dp(20)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    ImageView {
                        horizontalAlignment: HorizontalAlignment.Center
                        preferredWidth: bookManager.dp(76)
                        preferredHeight: bookManager.dp(76)
                        scalingMethod: ScalingMethod.AspectFit
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_app_dark.png" : "asset:///icons/ic_app_light.png"
                    }

                    Label {
                        topMargin: bookManager.dp(12)
                        horizontalAlignment: HorizontalAlignment.Center
                        text: "Libris"
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiAccent)
                    }

                    Label {
                        topMargin: bookManager.dp(4)
                        horizontalAlignment: HorizontalAlignment.Center
                        text: qsTr("Phiên bản %1 (Build %2) - BlackBerry 10").arg(bookManager.appVersion).arg(bookManager.buildNumber) + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create(bookManager.uiInk)
                    }

                    Label {
                        topMargin: bookManager.dp(4)
                        horizontalAlignment: HorizontalAlignment.Center
                        multiline: true
                        textStyle.textAlign: TextAlign.Center
                        text: qsTr("Trình đọc sách Native thuần C++ và Cascades cho BlackBerry 10.") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create(bookManager.uiMuted)
                    }
                }

                // Header 1: TỔNG QUAN
                Header {
                    title: qsTr("TỔNG QUAN") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(14)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)

                        Label {
                            multiline: true
                            text: (qsTr("Phiên bản hiện tại: %1 (Build %2)").arg(bookManager.appVersion).arg(bookManager.buildNumber) + "\n\n" + qsTr("Libris là ứng dụng đọc sách chuẩn Native Cascades dành cho BlackBerry 10. Hỗ trợ nhiều định dạng, cử chỉ vuốt mượt mà, tra từ điển ngoại tuyến và đọc giọng nói TTS.")) + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiInk)
                        }
                    }
                }

                // Header 2: CÔNG NGHỆ & BỘ XỬ LÝ LÕI
                Header {
                    title: qsTr("CÔNG NGHỆ VÀ BỘ XỬ LÝ LÕI") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(14)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)

                        Label {
                            multiline: true
                            text: qsTr("Lõi tài liệu: Poppler C++, DjVuLibre, pugixml DOM và miniz.\nBảng mã Tiếng Việt: Nhận diện tự động UTF-8, UTF-16, TCVN3, Windows-1258, VNI.\nĐọc văn bản TTS: BlackBerry Multimedia Services, Google TTS và Gemini AI.\nTừ điển ngoại tuyến: Cơ sở dữ liệu SQLite tích hợp trên thiết bị.") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiInk)
                        }
                    }
                }

                // Header 3: ĐỊNH DẠNG HỖ TRỢ
                Header {
                    title: qsTr("ĐỊNH DẠNG TÀI LIỆU HỖ TRỢ") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(14)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)

                        Label {
                            multiline: true
                            text: qsTr("EPUB (.epub): Chuẩn EPUB 2.0 và 3.0 với phông chữ nhúng và mục lục NCX.\nMOBI / AZW3 (.mobi, .azw, .azw3, .kf8, .prc): Sách điện tử Mobipocket / Kindle KF8 với mục lục, CSS và ảnh nhúng.\nCBZ (.cbz): Truyện tranh nén với lật trang mượt mà và trích xuất ComicInfo.\nPDF (.pdf): Trang đồ họa nguyên bản và chế độ cuộn văn bản Reflow.\nDJVU (.djvu, .djv): Tài liệu scan nén chất lượng cao.\nFB2 (.fb2): Định dạng FictionBook kèm chú thích chân trang.\nTXT (.txt): Văn bản thuần tự động phát hiện mã hóa.") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiInk)
                        }
                    }
                }

                // Header 4: BẢN QUYỀN & TÁC GIẢ
                Header {
                    title: qsTr("BẢN QUYỀN VÀ GIẤY PHÉP") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(14)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)

                        Label {
                            multiline: true
                            text: qsTr("Tác giả: NhatLee (Libris Developer Team)\nBộ dựng Vector PDF & DjVu: Libris PDF/DjVu Engine\nGiấy phép: GNU General Public License v2 (GPLv2)\nMôi trường: BlackBerry 10 Native SDK 10.3.1") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiInk)
                        }
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

                Header {
                    topMargin: bookManager.dp(8)
                    title: qsTr("CỬ CHỈ ĐIỀU HƯỚNG NATIVE") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(14)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)

                        Label {
                            multiline: true
                            text: qsTr("Vuốt mép viền trái: Mở thanh điều hướng Sidebar trượt trái.\nVuốt mép viền trên: Mở Menu ứng dụng BlackBerry 10.\nChạm giữa màn hình: Bật hoặc tắt thanh tiêu đề và thanh công cụ.\nVuốt mép viền phải: Mở danh sách tác vụ mở rộng.") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiInk)
                        }
                    }
                }

                Header {
                    title: qsTr("PHÍM TẮT BÀN PHÍM VẬT LÝ") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(14)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)

                        Label {
                            multiline: true
                            text: qsTr("Space: Sang trang kế tiếp / Tạm dừng đọc TTS\nN: Sang trang kế tiếp\nP / U: Về trang trước đó\nJ / K: Cuộn trang xuống / lên\nZ / W: Phóng to / Thu nhỏ cỡ chữ\nG / M: Mở mục lục sách\nT / B: Nhảy về đầu sách / cuối sách\nV: Đổi màu nền Ngày / Đêm\nF: Tìm kiếm trong sách\nA: Bật hoặc tắt đọc giọng nói TTS\nD: Tra từ điển ngoại tuyến\nH: Đánh dấu trang hiện tại\nO: Mở tệp sách mới\nS: Mở bảng cài đặt\nQ: Thoát hoàn toàn ứng dụng") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiInk)
                        }
                    }
                }

                Header {
                    title: qsTr("HỖ TRỢ THIẾT BỊ ĐẶC BIỆT") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(14)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)

                        Label {
                            multiline: true
                            text: qsTr("BlackBerry Passport: Vuốt trượt trên bàn phím cảm ứng để lật trang hoặc cuộn dòng.\nBlackBerry Classic: Sử dụng bàn rê quang học Trackpad để điều hướng và cuộn trang.") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiInk)
                        }
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
                    topMargin: bookManager.dp(8)
                    title: qsTr("NHẬT KÝ PHIÊN BẢN") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(14)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)

                        Label {
                            multiline: true
                            text: bookManager.changelogText
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiInk)
                        }
                    }
                }
            }
        }
    }
}
