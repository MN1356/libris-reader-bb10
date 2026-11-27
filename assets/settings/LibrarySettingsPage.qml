import bb.cascades 1.3
import bb.system 1.2
import bb.cascades.pickers 1.0

Page {
    id: librarySettingsPage

    signal backRequested()

    titleBar: TitleBar {
        title: qsTr("Tủ sách & Quản lý") + Retranslate.onLanguageChanged
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

                Header {
                    title: qsTr("GIAO DIỆN TỦ SÁCH") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(16)

                    DropDown {
                        id: viewModeDropDown
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
                }

                Header {
                    title: qsTr("QUÉT DỮ LIỆU") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(16)

                    Button {
                        text: qsTr("Quét lại bộ nhớ & thẻ nhớ") + Retranslate.onLanguageChanged
                        horizontalAlignment: HorizontalAlignment.Fill
                        appearance: ControlAppearance.Primary
                        onClicked: {
                            bookManager.scanLibrary();
                            scanToast.body = (qsTr("Đã quét xong! Tổng cộng: %1 cuốn sách.").arg(bookManager.bookCount)) + Retranslate.onLanguageChanged;
                            scanToast.show();
                        }
                    }
                }

                Header {
                    title: qsTr("BẢO TOÀN DỮ LIỆU") + Retranslate.onLanguageChanged
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
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)

                        Label {
                            multiline: true
                            text: qsTr("Tự động sao lưu lịch sử, tiến độ và ghi chú vào bộ nhớ chung khi cài lại app.") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiMuted)
                            bottomMargin: bookManager.dp(8)
                        }

                        Label {
                            text: qsTr("Thư mục lưu trữ bảo toàn:") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SubtitleText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiInk)
                        }

                        Label {
                            topMargin: bookManager.dp(2)
                            bottomMargin: bookManager.dp(12)
                            multiline: true
                            text: bookManager.persistentStorageDir
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiAccent)
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
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_storage.png" : "asset:///icons/ic_storage_dark.png"
                                onClicked: {
                                    bookManager.syncToPersistentStorage();
                                }
                            }

                            Button {
                                leftMargin: bookManager.dp(10)
                                layoutProperties: StackLayoutProperties {
                                    spaceQuota: 1.0
                                }
                                text: qsTr("Khôi Phục") + Retranslate.onLanguageChanged
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_refresh.png" : "asset:///icons/ic_refresh_dark.png"
                                onClicked: {
                                    restoreConfirmDialog.pendingArchivePath = "";
                                    restoreConfirmDialog.show();
                                }
                            }
                        }

                        Container {
                            topMargin: bookManager.dp(10)
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Button {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: qsTr("Xuất .librisbackup") + Retranslate.onLanguageChanged
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_download.png" : "asset:///icons/ic_download_dark.png"
                                onClicked: {
                                    var outPath = bookManager.createBackupArchive();
                                    backupToast.body = (outPath !== "" ? (qsTr("Đã tạo gói sao lưu: ") + outPath) : qsTr("Không thể tạo gói sao lưu")) + Retranslate.onLanguageChanged;
                                    backupToast.show();
                                }
                            }

                            Button {
                                leftMargin: bookManager.dp(10)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: qsTr("Nhập .librisbackup") + Retranslate.onLanguageChanged
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_open.png" : "asset:///icons/ic_open_dark.png"
                                onClicked: {
                                    backupFilePicker.open();
                                }
                            }
                        }

                        Label {
                            visible: bookManager.persistentStorageStatus !== ""
                            topMargin: bookManager.dp(10)
                            multiline: true
                            text: bookManager.persistentStorageStatus
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontStyle: FontStyle.Italic
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }
                    }
                }

                // =========================================================
                // LỊCH SỬ ĐỌC
                // =========================================================
                Header {
                    title: qsTr("LỊCH SỬ ĐỌC") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)

                    Button {
                        horizontalAlignment: HorizontalAlignment.Fill
                        text: qsTr("Xóa toàn bộ lịch sử đọc") + Retranslate.onLanguageChanged
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_recent.png" : "asset:///icons/ic_recent_dark.png"
                        onClicked: {
                            clearHistoryDialog.show();
                        }
                    }
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
                    clearHistoryToast.show();
                }
            }
        },
        SystemToast {
            id: scanToast
            body: ""
        },
        SystemToast {
            id: clearHistoryToast
            body: qsTr("Đã xóa toàn bộ lịch sử đọc.") + Retranslate.onLanguageChanged
        },
        SystemToast {
            id: backupToast
            body: ""
        },
        SystemDialog {
            id: restoreConfirmDialog
            property string pendingArchivePath: ""
            title: qsTr("Khôi phục dữ liệu?") + Retranslate.onLanguageChanged
            body: qsTr("Thao tác này sẽ ghi đè tiến độ đọc, ghi chú và cài đặt hiện tại bằng bản sao lưu. Tiếp tục?") + Retranslate.onLanguageChanged
            confirmButton.label: qsTr("Khôi Phục") + Retranslate.onLanguageChanged
            cancelButton.label: qsTr("Hủy") + Retranslate.onLanguageChanged
            onFinished: {
                if (value === SystemUiResult.ConfirmButtonSelection) {
                    if (pendingArchivePath === "") {
                        bookManager.restoreFromPersistentStorage();
                        backupToast.body = bookManager.persistentStorageStatus;
                        backupToast.show();
                    } else {
                        var ok = bookManager.restoreBackupArchive(pendingArchivePath);
                        backupToast.body = (ok ? qsTr("Đã khôi phục dữ liệu từ gói sao lưu!") : qsTr("Gói sao lưu không hợp lệ hoặc bị lỗi")) + Retranslate.onLanguageChanged;
                        backupToast.show();
                    }
                }
            }
        },
        FilePicker {
            id: backupFilePicker
            type: FileType.Other
            title: qsTr("Chọn tệp sao lưu .librisbackup") + Retranslate.onLanguageChanged
            filter: [ "*.librisbackup", "*.zip" ]
            onFileSelected: {
                if (selectedFiles && selectedFiles.length > 0) {
                    restoreConfirmDialog.pendingArchivePath = selectedFiles[0];
                    restoreConfirmDialog.show();
                }
            }
        }
    ]
}
