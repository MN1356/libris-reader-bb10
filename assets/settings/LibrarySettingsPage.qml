import bb.cascades 1.3
import bb.system 1.2

Page {
    id: librarySettingsPage

    signal backRequested()

    titleBar: TitleBar {
        title: qsTr("Tủ sách & Quản lý") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Default
    }

    ScrollView {
        scrollRole: ScrollRole.Main
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
            topPadding: 16
            leftPadding: 16
            rightPadding: 16
            bottomPadding: 32

            Header {
                title: qsTr("GIAO DIỆN TỦ SÁCH") + Retranslate.onLanguageChanged
            }

            DropDown {
                id: viewModeDropDown
                topMargin: 8
                title: qsTr("Kiểu hiển thị") + Retranslate.onLanguageChanged

                Option {
                    text: qsTr("Lưới bìa") + Retranslate.onLanguageChanged
                    value: "mosaic"
                    selected: bookManager.libraryViewMode === "mosaic"
                }
                Option {
                    text: qsTr("Danh sách") + Retranslate.onLanguageChanged
                    value: "detailed"
                    selected: bookManager.libraryViewMode === "detailed"
                }

                onSelectedValueChanged: {
                    if (selectedValue && selectedValue !== bookManager.libraryViewMode) {
                        bookManager.setLibraryViewMode(selectedValue);
                    }
                }
            }

            Header {
                topMargin: 20
                title: qsTr("QUÉT DỮ LIỆU") + Retranslate.onLanguageChanged
            }

            Button {
                topMargin: 8
                text: qsTr("Quét lại bộ nhớ & thẻ nhớ") + Retranslate.onLanguageChanged
                horizontalAlignment: HorizontalAlignment.Fill
                onClicked: {
                    bookManager.scanLibrary();
                    scanFeedbackLabel.visible = true;
                }
            }

            Label {
                id: scanFeedbackLabel
                visible: false
                topMargin: 6
                text: (qsTr("Đã quét xong! Tổng cộng: %1 cuốn sách.").arg(bookManager.bookCount) + Retranslate.onLanguageChanged)
                textStyle.base: SystemDefaults.TextStyles.BodyText
                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                multiline: true
            }

            Header {
                topMargin: 20
                title: qsTr("BẢO TOÀN DỮ LIỆU") + Retranslate.onLanguageChanged
            }

            Container {
                topMargin: 8
                horizontalAlignment: HorizontalAlignment.Fill
                background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F5F5F5")
                topPadding: 14
                bottomPadding: 14
                leftPadding: 14
                rightPadding: 14

                Label {
                    multiline: true
                    text: qsTr("Tự động sao lưu lịch sử, tiến độ và ghi chú vào bộ nhớ chung khi cài lại app.") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                    bottomMargin: 8
                }

                Label {
                    text: qsTr("Thư mục lưu trữ bảo toàn:") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                }

                Label {
                    topMargin: 2
                    bottomMargin: 12
                    multiline: true
                    text: bookManager.persistentStorageDir
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#10B981") : Color.create("#059669")
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    Button {
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                        text: qsTr("Sao Lưu Ngay") + Retranslate.onLanguageChanged
                        imageSource: "asset:///icons/ic_storage.png"
                        onClicked: {
                            bookManager.syncToPersistentStorage();
                        }
                    }

                    Button {
                        leftMargin: 10
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                        text: qsTr("Khôi Phục") + Retranslate.onLanguageChanged
                        imageSource: "asset:///icons/ic_open.png"
                        onClicked: {
                            bookManager.restoreFromPersistentStorage();
                        }
                    }
                }

                Label {
                    topMargin: 10
                    multiline: true
                    text: bookManager.persistentStorageStatus
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontStyle: FontStyle.Italic
                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                }
            }

            // =========================================================
            // LỊCH SỬ ĐỌC
            // =========================================================
            Header {
                topMargin: 20
                title: qsTr("LỊCH SỬ ĐỌC") + Retranslate.onLanguageChanged
            }

            Container {
                topMargin: 8
                horizontalAlignment: HorizontalAlignment.Fill

                Button {
                    horizontalAlignment: HorizontalAlignment.Fill
                    text: qsTr("Xóa toàn bộ lịch sử đọc") + Retranslate.onLanguageChanged
                    imageSource: "asset:///icons/ic_recent.png"
                    onClicked: {
                        clearHistoryDialog.show();
                    }
                }

                Label {
                    id: clearHistoryFeedbackLabel
                    visible: false
                    topMargin: 8
                    text: qsTr("Đã xóa sạch lịch sử đọc gần đây và đặt lại tiến độ.") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.BodyText
                    textStyle.color: Color.create("#10B981")
                    multiline: true
                }
            }
        }
    }

    attachedObjects: [
        SystemDialog {
            id: clearHistoryDialog
            title: qsTr("Xóa Lịch Sử Đọc") + Retranslate.onLanguageChanged
            body: qsTr("Toàn bộ danh sách sách đã đọc gần đây và tiến độ đọc sẽ được xóa. Các đánh dấu (Bookmark) và ghi chú (Highlight) vẫn sẽ được giữ nguyên.\n\nBạn có chắc chắn muốn xóa không?") + Retranslate.onLanguageChanged
            confirmButton.label: qsTr("Xóa lịch sử") + Retranslate.onLanguageChanged
            cancelButton.label: qsTr("Hủy") + Retranslate.onLanguageChanged
            onFinished: {
                if (value === SystemUiResult.ConfirmButtonSelection) {
                    bookManager.clearReadingHistory();
                    clearHistoryFeedbackLabel.visible = true;
                    clearHistoryToast.show();
                }
            }
        },
        SystemToast {
            id: clearHistoryToast
            body: qsTr("Đã xóa toàn bộ lịch sử đọc.") + Retranslate.onLanguageChanged
        }
    ]
}
