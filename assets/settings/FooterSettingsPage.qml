import bb.cascades 1.3

Page {
    id: footerSettingsPage

    signal backRequested()

    titleBar: TitleBar {
        title: qsTr("Chân trang") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
    }

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: Color.create(bookManager.uiBg)

        ScrollView {
            scrollRole: ScrollRole.Main
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                bottomPadding: bookManager.dp(32)

                // =========================================================
                // 1. Live Footer Preview Card
                // =========================================================
                Header {
                    title: qsTr("XEM TRƯỚC CHÂN TRANG") + Retranslate.onLanguageChanged
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
                        topPadding: bookManager.dp(12)
                        bottomPadding: bookManager.dp(12)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Label {
                                visible: bookManager.showFooterChapter
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: qsTr("Chương 3: Đêm tĩnh lặng") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }
                            Label {
                                visible: !bookManager.showFooterChapter
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: (!bookManager.showFooterProgress && !bookManager.showFooterBattery && !bookManager.showFooterClock) ?
                                      (qsTr("Tất cả mục chân trang đang ẩn") + Retranslate.onLanguageChanged) : ""
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.fontStyle: FontStyle.Italic
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }
                            Label {
                                visible: bookManager.showFooterProgress
                                leftMargin: bookManager.dp(10)
                                text: "42/180 (23%)"
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.fontWeight: FontWeight.W500
                                textStyle.color: Color.create(bookManager.uiAccent)
                            }
                            Label {
                                visible: bookManager.showFooterBattery
                                leftMargin: bookManager.dp(10)
                                text: "86%"
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }
                            Label {
                                visible: bookManager.showFooterClock
                                leftMargin: bookManager.dp(10)
                                text: "21:40"
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiInk)
                            }
                        }
                    }
                }

                // =========================================================
                // 2. Mục chân trang (Footer Items)
                // =========================================================
                Header {
                    title: qsTr("MỤC HIỂN THỊ CHÂN TRANG") + Retranslate.onLanguageChanged
                }

                ToggleableListItem {
                    title: qsTr("Tên chương") + Retranslate.onLanguageChanged
                    subtitle: qsTr("Hiển thị tiêu đề chương đang đọc ở chân trang") + Retranslate.onLanguageChanged
                    checked: bookManager.showFooterChapter
                    onToggled: {
                        bookManager.setShowFooterChapter(checked);
                    }
                }

                ToggleableListItem {
                    title: qsTr("Tiến độ đọc") + Retranslate.onLanguageChanged
                    subtitle: qsTr("Hiển thị số trang và phần trăm đọc sách") + Retranslate.onLanguageChanged
                    checked: bookManager.showFooterProgress
                    onToggled: {
                        bookManager.setShowFooterProgress(checked);
                    }
                }

                ToggleableListItem {
                    title: qsTr("Mức pin") + Retranslate.onLanguageChanged
                    subtitle: qsTr("Hiển thị phần trăm pin của thiết bị") + Retranslate.onLanguageChanged
                    checked: bookManager.showFooterBattery
                    onToggled: {
                        bookManager.setShowFooterBattery(checked);
                    }
                }

                ToggleableListItem {
                    title: qsTr("Đồng hồ") + Retranslate.onLanguageChanged
                    subtitle: qsTr("Hiển thị thời gian hiện tại") + Retranslate.onLanguageChanged
                    checked: bookManager.showFooterClock
                    showDivider: false
                    onToggled: {
                        bookManager.setShowFooterClock(checked);
                    }
                }

                // =========================================================
                // 3. Thao tác nhanh (Ở phía dưới sau các tùy chọn)
                // =========================================================
                Header {
                    topMargin: bookManager.dp(8)
                    title: qsTr("THAO TÁC NHANH") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topPadding: bookManager.dp(12)
                    bottomPadding: bookManager.dp(16)
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    Button {
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                        text: qsTr("Bật tất cả") + Retranslate.onLanguageChanged
                        onClicked: {
                            bookManager.setShowFooterChapter(true);
                            bookManager.setShowFooterProgress(true);
                            bookManager.setShowFooterBattery(true);
                            bookManager.setShowFooterClock(true);
                        }
                    }

                    Button {
                        leftMargin: bookManager.dp(10)
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                        text: qsTr("Tắt tất cả") + Retranslate.onLanguageChanged
                        onClicked: {
                            bookManager.setShowFooterChapter(false);
                            bookManager.setShowFooterProgress(false);
                            bookManager.setShowFooterBattery(false);
                            bookManager.setShowFooterClock(false);
                        }
                    }
                }
            }
        }
    }
}
