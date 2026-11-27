import bb.cascades 1.3
import bb.system 1.2

Page {
    id: updateSettingsPage

    signal backRequested()

    onCreationCompleted: {
        bookManager.updateReadyForRestartDialog.connect(updateSettingsPage.onReadyForRestartDialog);
        bookManager.updateErrorOccurred.connect(updateSettingsPage.onUpdateError);
    }

    function onReadyForRestartDialog(ver, targetPath) {
        installNoticeDialog.body = qsTr("Đã tải xong bản cập nhật %1 và sẵn sàng đưa vào BarInstaller:\n%2\n\nỨng dụng sẽ tự tắt ngay trước khi cài đặt.\nVui lòng CHỜ KHOẢNG 2 PHÚT trước khi mở lại Libris!").replace("%1", ver).replace("%2", targetPath) + Retranslate.onLanguageChanged;
        installNoticeDialog.show();
    }

    function onUpdateError(msg) {
        if (msg && msg !== "") {
            updateToast.body = msg;
            updateToast.show();
        }
    }

    titleBar: TitleBar {
        title: qsTr("Cập nhật ứng dụng") + Retranslate.onLanguageChanged
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
                    title: qsTr("PHIÊN BẢN HIỆN TẠI") + Retranslate.onLanguageChanged
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
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Label {
                            text: "Libris v" + bookManager.appVersion + " (Build " + bookManager.buildNumber + ")"
                            textStyle.base: SystemDefaults.TextStyles.SubtitleText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create(bookManager.uiInk)
                        }

                        Label {
                            topMargin: bookManager.dp(4)
                            text: qsTr("Nguồn cập nhật: GitHub (MN1356/libris-reader-bb10)") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiMuted)
                            multiline: true
                        }

                        Label {
                            topMargin: bookManager.dp(4)
                            text: qsTr("Cơ chế cài đặt: Tải tệp .bar đưa vào thư mục barinstaller, tự tắt app và chờ ~2 phút trước khi mở lại.") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiMuted)
                            multiline: true
                        }

                        Button {
                            topMargin: bookManager.dp(12)
                            horizontalAlignment: HorizontalAlignment.Fill
                            appearance: ControlAppearance.Primary
                            enabled: !bookManager.updateChecking && !bookManager.updateDownloading
                            text: (bookManager.updateChecking ? qsTr("Đang kiểm tra...") : qsTr("Kiểm tra cập nhật trên GitHub")) + Retranslate.onLanguageChanged
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_refresh.png" : "asset:///icons/ic_refresh_dark.png"
                            onClicked: {
                                bookManager.checkForAppUpdate();
                            }
                        }

                        Container {
                            visible: bookManager.updateChecking
                            topMargin: bookManager.dp(10)
                            horizontalAlignment: HorizontalAlignment.Center
                            ActivityIndicator {
                                running: bookManager.updateChecking
                                preferredWidth: bookManager.dp(36)
                                preferredHeight: bookManager.dp(36)
                            }
                        }

                        Label {
                            visible: bookManager.updateStatusText !== ""
                            topMargin: bookManager.dp(10)
                            text: bookManager.updateStatusText
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: bookManager.updateAvailable ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiMuted)
                            multiline: true
                        }
                    }
                }

                // Release Details & Download Card
                Header {
                    visible: bookManager.updateAvailable || bookManager.updateDownloading || bookManager.updateReadyToInstall
                    title: qsTr("PHIÊN BẢN MỚI TRÊN GITHUB") + Retranslate.onLanguageChanged
                }

                Container {
                    visible: bookManager.updateAvailable || bookManager.updateDownloading || bookManager.updateReadyToInstall
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
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Label {
                            text: qsTr("Phiên bản mới: ") + bookManager.updateLatestVersion + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.TitleText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create(bookManager.uiAccent)
                        }

                        Label {
                            visible: bookManager.updateAssetName !== ""
                            topMargin: bookManager.dp(4)
                            text: bookManager.updateAssetName + (bookManager.updateAssetSizeStr !== "" ? (" • " + bookManager.updateAssetSizeStr) : "")
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiMuted)
                            multiline: true
                        }

                        Label {
                            visible: bookManager.updateReleaseNotes !== ""
                            topMargin: bookManager.dp(10)
                            text: bookManager.updateReleaseNotes
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiInk)
                            multiline: true
                        }

                        ProgressIndicator {
                            visible: bookManager.updateDownloading
                            topMargin: bookManager.dp(12)
                            horizontalAlignment: HorizontalAlignment.Fill
                            fromValue: 0
                            toValue: 100
                            value: bookManager.updateProgress
                        }

                        Button {
                            visible: bookManager.updateAvailable && !bookManager.updateDownloading && !bookManager.updateReadyToInstall
                            topMargin: bookManager.dp(12)
                            horizontalAlignment: HorizontalAlignment.Fill
                            appearance: ControlAppearance.Primary
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_download.png" : "asset:///icons/ic_download_dark.png"
                            text: qsTr("Tải về & Cài đặt qua BarInstaller") + Retranslate.onLanguageChanged
                            onClicked: {
                                bookManager.downloadAppUpdate();
                            }
                        }

                        Button {
                            visible: bookManager.updateDownloading
                            topMargin: bookManager.dp(10)
                            horizontalAlignment: HorizontalAlignment.Fill
                            text: qsTr("Hủy tải") + Retranslate.onLanguageChanged
                            onClicked: {
                                bookManager.cancelAppUpdateDownload();
                            }
                        }

                        Button {
                            visible: bookManager.updateReadyToInstall
                            topMargin: bookManager.dp(12)
                            horizontalAlignment: HorizontalAlignment.Fill
                            appearance: ControlAppearance.Primary
                            text: qsTr("Đưa vào BarInstaller & Tự tắt app") + Retranslate.onLanguageChanged
                            onClicked: {
                                updateSettingsPage.onReadyForRestartDialog(bookManager.updateLatestVersion, bookManager.updateTargetBarPath);
                            }
                        }
                    }
                }
            }
        }
    }

    attachedObjects: [
        SystemDialog {
            id: installNoticeDialog
            title: qsTr("Cập nhật qua BarInstaller") + Retranslate.onLanguageChanged
            confirmButton.label: qsTr("Đưa vào BarInstaller & Tắt app") + Retranslate.onLanguageChanged
            cancelButton.label: qsTr("Để sau") + Retranslate.onLanguageChanged
            onFinished: {
                if (result === SystemUiResult.ConfirmButtonSelection) {
                    bookManager.finalizeBarInstallAndQuit();
                }
            }
        },
        SystemToast {
            id: updateToast
            body: ""
        }
    ]
}
