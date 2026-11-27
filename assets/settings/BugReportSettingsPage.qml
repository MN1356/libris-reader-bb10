import bb.cascades 1.3
import bb.system 1.2

Page {
    id: bugReportPage

    signal backRequested()

    titleBar: TitleBar {
        title: qsTr("Báo lỗi & Góp ý") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
    }

    attachedObjects: [
        SystemToast {
            id: reportToast
            body: ""
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

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                bottomPadding: bookManager.dp(32)

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(14)
                    bottomPadding: bookManager.dp(8)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(12)
                        bottomPadding: bookManager.dp(12)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)

                        Label {
                            multiline: true
                            text: qsTr("Gửi báo lỗi hoặc góp ý qua Email BlackBerry Hub tới nhatlm0910@gmail.com. Đính kèm nhật ký kỹ thuật là tùy chọn.") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }
                    }
                }

                Header {
                    title: qsTr("NỘI DUNG BÁO LỖI") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(10)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    DropDown {
                        id: categoryDropDown
                        title: qsTr("Phân loại") + Retranslate.onLanguageChanged

                        Option {
                            text: qsTr("Báo lỗi (Bug)") + Retranslate.onLanguageChanged
                            value: "Bug"
                            selected: true
                        }
                        Option {
                            text: qsTr("Lỗi mở/hiển thị sách") + Retranslate.onLanguageChanged
                            value: "Format"
                        }
                        Option {
                            text: qsTr("Góp ý tính năng") + Retranslate.onLanguageChanged
                            value: "Feedback"
                        }
                    }

                    TextField {
                        id: subjectField
                        topMargin: bookManager.dp(10)
                        hintText: qsTr("Tiêu đề ngắn gọn (ví dụ: Lỗi mở file EPUB)...") + Retranslate.onLanguageChanged
                    }

                    TextArea {
                        id: descField
                        topMargin: bookManager.dp(10)
                        minHeight: bookManager.dp(130)
                        hintText: qsTr("Mô tả chi tiết lỗi bạn gặp phải hoặc góp ý tính năng...") + Retranslate.onLanguageChanged
                    }
                }

                Header {
                    topMargin: bookManager.dp(6)
                    title: qsTr("TÙY CHỌN KỸ THUẬT") + Retranslate.onLanguageChanged
                }

                ToggleableListItem {
                    id: includeLogToggle
                    title: qsTr("Đính kèm nhật ký kỹ thuật (Log)") + Retranslate.onLanguageChanged
                    subtitle: qsTr("Không bắt buộc — gửi kèm ~60 dòng log gần nhất để hỗ trợ sửa lỗi") + Retranslate.onLanguageChanged
                    checked: false
                    onToggled: {
                        includeLogToggle.checked = checked;
                    }
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(18)

                    Button {
                        horizontalAlignment: HorizontalAlignment.Fill
                        text: qsTr("Gửi Email tới nhatlm0910@gmail.com") + Retranslate.onLanguageChanged
                        appearance: ControlAppearance.Primary
                        onClicked: {
                            if (subjectField.text.trim() === "" && descField.text.trim() === "") {
                                reportToast.body = qsTr("Vui lòng nhập tiêu đề hoặc mô tả trước khi gửi.") + Retranslate.onLanguageChanged;
                                reportToast.show();
                                return;
                            }
                            var ok = bookManager.sendBugReportEmail(
                                categoryDropDown.selectedValue,
                                subjectField.text,
                                descField.text,
                                includeLogToggle.checked
                            );
                            if (ok) {
                                reportToast.body = qsTr("Đang mở trình soạn Email tới nhatlm0910@gmail.com...") + Retranslate.onLanguageChanged;
                            } else {
                                reportToast.body = qsTr("Không thể mở ứng dụng Email trên thiết bị.") + Retranslate.onLanguageChanged;
                            }
                            reportToast.show();
                        }
                    }
                }
            }
        }
    }
}
