import bb.cascades 1.3

Sheet {
    id: highlightNoteSheet

    property string highlightId: ""
    property string quoteText: ""
    property string currentNote: ""
    property string currentColor: "yellow"
    signal savedCallback(string highlightId, string note, string color)
    signal deletedCallback(string highlightId)

    function openForHighlight(id, text, note, color) {
        highlightId = id;
        quoteText = text;
        currentNote = note ? note : "";
        currentColor = color ? color : "yellow";
        noteInput.text = currentNote;
        highlightNoteSheet.open();
    }

    Page {
        titleBar: TitleBar {
            title: (highlightId.indexOf("bm_") === 0 ? qsTr("Ghi Chú Dấu Trang") : qsTr("Ghi Chú Highlight")) + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain

            dismissAction: ActionItem {
                title: qsTr("Đóng") + Retranslate.onLanguageChanged
                onTriggered: {
                    highlightNoteSheet.close();
                }
            }

            acceptAction: ActionItem {
                title: qsTr("Lưu") + Retranslate.onLanguageChanged
                onTriggered: {
                    saveAndClose();
                }
            }
        }

        Container {
            layout: DockLayout {}
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")

            ScrollView {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                scrollViewProperties.scrollMode: ScrollMode.Vertical

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topPadding: 16
                    bottomPadding: 32
                    leftPadding: 16
                    rightPadding: 16

                    layout: StackLayout {
                        orientation: LayoutOrientation.TopToBottom
                    }

                    // =========================================================
                    // 1. QUOTE CARD (Flat Card with Color Bar)
                    // =========================================================
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 14
                        bottomPadding: 14
                        leftPadding: 14
                        rightPadding: 14
                        bottomMargin: 16

                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }

                        // Color Indicator Stripe
                        Container {
                            preferredWidth: 6
                            verticalAlignment: VerticalAlignment.Fill
                            background: currentColor === "green" ? Color.create("#22C55E") :
                                        (currentColor === "blue" ? Color.create("#00A8DF") :
                                        (currentColor === "pink" ? Color.create("#EC4899") : Color.create("#EAB308")))
                            rightMargin: 12
                        }

                        Container {
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1.0
                            }
                            layout: StackLayout {
                                orientation: LayoutOrientation.TopToBottom
                            }

                            Label {
                                text: (highlightId.indexOf("bm_") === 0 ? qsTr("Dấu trang này:") : qsTr("Đoạn trích:")) + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.XSmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                bottomMargin: 6
                            }

                            Label {
                                text: "\"" + quoteText + "\""
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                textStyle.fontStyle: FontStyle.Italic
                                multiline: true
                                autoSize.maxLineCount: 6
                            }
                        }
                    }

                    // =========================================================
                    // 2. COLOR PICKER ROW
                    // =========================================================
                    Container {
                        visible: highlightId.indexOf("bm_") !== 0
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 12
                        bottomPadding: 12
                        leftPadding: 14
                        rightPadding: 14
                        bottomMargin: 16

                        layout: StackLayout {
                            orientation: LayoutOrientation.TopToBottom
                        }

                        Label {
                            text: qsTr("Màu sắc:") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                            bottomMargin: 10
                        }

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout {
                                orientation: LayoutOrientation.LeftToRight
                            }

                            // Yellow Button
                            Button {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: qsTr("Vàng") + Retranslate.onLanguageChanged
                                onClicked: {
                                    currentColor = "yellow";
                                }
                            }

                            // Green Button
                            Button {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: qsTr("Xanh lá") + Retranslate.onLanguageChanged
                                onClicked: {
                                    currentColor = "green";
                                }
                            }

                            // Blue Button
                            Button {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: qsTr("Xanh dương") + Retranslate.onLanguageChanged
                                onClicked: {
                                    currentColor = "blue";
                                }
                            }

                            // Pink Button
                            Button {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: qsTr("Hồng") + Retranslate.onLanguageChanged
                                onClicked: {
                                    currentColor = "pink";
                                }
                            }
                        }
                    }

                    // =========================================================
                    // 3. NOTE INPUT (TextArea)
                    // =========================================================
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 14
                        bottomPadding: 14
                        leftPadding: 14
                        rightPadding: 14
                        bottomMargin: 20

                        layout: StackLayout {
                            orientation: LayoutOrientation.TopToBottom
                        }

                        Label {
                            text: qsTr("Ghi chú của bạn:") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                            bottomMargin: 8
                        }

                        TextArea {
                            id: noteInput
                            hintText: qsTr("Nhập ghi chú cho đoạn này...") + Retranslate.onLanguageChanged
                            preferredHeight: 180
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                        }
                    }

                    // =========================================================
                    // 4. ACTION BUTTONS
                    // =========================================================
                    Button {
                        horizontalAlignment: HorizontalAlignment.Fill
                        text: qsTr("Lưu Ghi Chú") + Retranslate.onLanguageChanged
                        bottomMargin: 10
                        onClicked: {
                            saveAndClose();
                        }
                    }

                    Button {
                        horizontalAlignment: HorizontalAlignment.Fill
                        text: qsTr("Xóa Highlight") + Retranslate.onLanguageChanged
                        onClicked: {
                            deleteAndClose();
                        }
                    }
                }
            }
        }
    }

    function saveAndClose() {
        var n = noteInput.text ? noteInput.text.trim() : "";
        if (highlightId.indexOf("bm_") === 0) {
            bookManager.updateBookmarkNote(highlightId, n);
        } else {
            bookManager.updateHighlight(highlightId, n, currentColor);
        }
        highlightNoteSheet.savedCallback(highlightId, n, currentColor);
        highlightNoteSheet.close();
    }

    function deleteAndClose() {
        if (highlightId.indexOf("bm_") === 0) {
            bookManager.removeBookmark(highlightId);
        } else {
            bookManager.removeHighlight(highlightId);
        }
        highlightNoteSheet.deletedCallback(highlightId);
        highlightNoteSheet.close();
    }
}
