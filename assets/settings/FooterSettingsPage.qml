import bb.cascades 1.3

Page {
    id: footerSettingsPage

    signal backRequested()

    titleBar: TitleBar {
        title: qsTr("Chân trang") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Default
    }

    ScrollView {
        scrollRole: ScrollRole.Main
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            bottomPadding: 32

            // =========================================================
            // 1. Thao tác nhanh (Đưa lên phía trên)
            // =========================================================
            Header {
                title: qsTr("THAO TÁC NHANH") + Retranslate.onLanguageChanged
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: 12
                bottomPadding: 16
                leftPadding: 16
                rightPadding: 16
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
                    leftMargin: 10
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
        }
    }
}
