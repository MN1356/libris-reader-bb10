import bb.cascades 1.3
import bb.system 1.2

Sheet {
    id: highlightNoteSheet

    property string highlightId: ""
    property string quoteText: ""
    property string currentNote: ""
    property string currentColor: "yellow"
    property string bookTitle: ""
    property string bookAuthor: ""
    signal savedCallback(string highlightId, string note, string color)
    signal deletedCallback(string highlightId)

    function openForHighlight(id, text, note, color, bTitle, bAuthor) {
        highlightId = id;
        quoteText = text;
        currentNote = note ? note : "";
        currentColor = color ? color : "yellow";
        bookTitle = bTitle ? bTitle : bookManager.currentBookTitle;
        bookAuthor = bAuthor ? bAuthor : bookManager.currentBookAuthor;
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
            background: Color.create(bookManager.uiBg)

            ScrollView {
                scrollRole: ScrollRole.Main
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                scrollViewProperties.scrollMode: ScrollMode.Vertical

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topPadding: bookManager.dp(16)
                    bottomPadding: bookManager.dp(28)
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    // 1. Quote Card
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)
                        bottomMargin: bookManager.dp(16)
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        Container {
                            preferredWidth: bookManager.dp(4)
                            verticalAlignment: VerticalAlignment.Fill
                            background: currentColor === "green" ? Color.create("#5FA876") :
                                        (currentColor === "blue" ? Color.create("#5B9BD5") :
                                        (currentColor === "pink" ? Color.create("#D6729B") : Color.create("#D9AB60")))
                            rightMargin: bookManager.dp(12)
                        }

                        Container {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            Label {
                                text: (highlightId.indexOf("bm_") === 0 ? qsTr("Dấu trang này:") : qsTr("Đoạn trích:")) + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                                bottomMargin: bookManager.dp(6)
                            }

                            Label {
                                text: "\"" + quoteText + "\""
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: Color.create(bookManager.uiInk)
                                textStyle.fontStyle: FontStyle.Italic
                                textStyle.lineHeight: 1.25
                                multiline: true
                                autoSize.maxLineCount: 6
                            }
                        }
                    }

                    // 2. Tactile Color Swatch Row
                    Container {
                        visible: highlightId.indexOf("bm_") !== 0
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(12)
                        bottomPadding: bookManager.dp(12)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)
                        bottomMargin: bookManager.dp(16)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Label {
                            text: qsTr("Màu sắc:") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiMuted)
                            bottomMargin: bookManager.dp(10)
                        }

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            // Yellow Swatch
                            Container {
                                accessibility.name: qsTr("Vàng") + Retranslate.onLanguageChanged
                                minHeight: bookManager.dp(48)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                background: currentColor === "yellow" ? Color.create(bookManager.uiActive) : Color.create(bookManager.uiSurface)
                                topPadding: bookManager.dp(8)
                                bottomPadding: bookManager.dp(6)
                                leftPadding: bookManager.dp(4)
                                rightPadding: bookManager.dp(4)
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                gestureHandlers: [ TapHandler { onTapped: { currentColor = "yellow"; } } ]

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    preferredHeight: bookManager.dp(6)
                                    background: Color.create("#D9AB60")
                                }
                                Label {
                                    topMargin: bookManager.dp(6)
                                    horizontalAlignment: HorizontalAlignment.Center
                                    text: qsTr("Vàng") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: currentColor === "yellow" ? FontWeight.Bold : FontWeight.Normal
                                    textStyle.color: currentColor === "yellow" ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiInk)
                                }
                            }

                            // Green Swatch
                            Container {
                                accessibility.name: qsTr("Xanh lá") + Retranslate.onLanguageChanged
                                minHeight: bookManager.dp(48)
                                leftMargin: bookManager.dp(8)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                background: currentColor === "green" ? Color.create(bookManager.uiActive) : Color.create(bookManager.uiSurface)
                                topPadding: bookManager.dp(8)
                                bottomPadding: bookManager.dp(6)
                                leftPadding: bookManager.dp(4)
                                rightPadding: bookManager.dp(4)
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                gestureHandlers: [ TapHandler { onTapped: { currentColor = "green"; } } ]

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    preferredHeight: bookManager.dp(6)
                                    background: Color.create("#5FA876")
                                }
                                Label {
                                    topMargin: bookManager.dp(6)
                                    horizontalAlignment: HorizontalAlignment.Center
                                    text: qsTr("Xanh lá") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: currentColor === "green" ? FontWeight.Bold : FontWeight.Normal
                                    textStyle.color: currentColor === "green" ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiInk)
                                }
                            }

                            // Blue Swatch
                            Container {
                                accessibility.name: qsTr("Xanh dương") + Retranslate.onLanguageChanged
                                minHeight: bookManager.dp(48)
                                leftMargin: bookManager.dp(8)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                background: currentColor === "blue" ? Color.create(bookManager.uiActive) : Color.create(bookManager.uiSurface)
                                topPadding: bookManager.dp(8)
                                bottomPadding: bookManager.dp(6)
                                leftPadding: bookManager.dp(4)
                                rightPadding: bookManager.dp(4)
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                gestureHandlers: [ TapHandler { onTapped: { currentColor = "blue"; } } ]

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    preferredHeight: bookManager.dp(6)
                                    background: Color.create("#5B9BD5")
                                }
                                Label {
                                    topMargin: bookManager.dp(6)
                                    horizontalAlignment: HorizontalAlignment.Center
                                    text: qsTr("Xanh dương") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: currentColor === "blue" ? FontWeight.Bold : FontWeight.Normal
                                    textStyle.color: currentColor === "blue" ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiInk)
                                    autoSize.maxLineCount: 1
                                }
                            }

                            // Pink Swatch
                            Container {
                                accessibility.name: qsTr("Hồng") + Retranslate.onLanguageChanged
                                minHeight: bookManager.dp(48)
                                leftMargin: bookManager.dp(8)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                background: currentColor === "pink" ? Color.create(bookManager.uiActive) : Color.create(bookManager.uiSurface)
                                topPadding: bookManager.dp(8)
                                bottomPadding: bookManager.dp(6)
                                leftPadding: bookManager.dp(4)
                                rightPadding: bookManager.dp(4)
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                gestureHandlers: [ TapHandler { onTapped: { currentColor = "pink"; } } ]

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    preferredHeight: bookManager.dp(6)
                                    background: Color.create("#D6729B")
                                }
                                Label {
                                    topMargin: bookManager.dp(6)
                                    horizontalAlignment: HorizontalAlignment.Center
                                    text: qsTr("Hồng") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: currentColor === "pink" ? FontWeight.Bold : FontWeight.Normal
                                    textStyle.color: currentColor === "pink" ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiInk)
                                }
                            }
                        }
                    }

                    // 3. Unboxed Native TextArea
                    Label {
                        text: qsTr("Ghi chú của bạn:") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create(bookManager.uiMuted)
                        bottomMargin: bookManager.dp(8)
                    }

                    TextArea {
                        id: noteInput
                        hintText: qsTr("Nhập ghi chú cho đoạn này...") + Retranslate.onLanguageChanged
                        minHeight: bookManager.dp(150)
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                    }

                    // 4. Secondary Actions Row (Export + Share) & Confirmed Delete
                    Container {
                        topMargin: bookManager.dp(16)
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        Button {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: qsTr("Xuất ảnh Quote Card") + Retranslate.onLanguageChanged
                            onClicked: {
                                var imgPath = bookManager.exportQuoteCardImage(quoteText, bookTitle, bookAuthor, noteInput.text);
                                if (imgPath && imgPath !== "") {
                                    noteToast.body = (qsTr("Đã lưu ảnh Quote Card: ") + imgPath) + Retranslate.onLanguageChanged;
                                } else {
                                    noteToast.body = qsTr("Không thể xuất ảnh Quote Card") + Retranslate.onLanguageChanged;
                                }
                                noteToast.show();
                            }
                        }

                        Button {
                            leftMargin: bookManager.dp(10)
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: qsTr("Chia sẻ Quote Card") + Retranslate.onLanguageChanged
                            onClicked: {
                                var imgPath = bookManager.exportQuoteCardImage(quoteText, bookTitle, bookAuthor, noteInput.text);
                                if (imgPath && imgPath !== "") {
                                    app.shareFile(imgPath, "image/png");
                                } else {
                                    noteToast.body = qsTr("Không thể xuất ảnh Quote Card") + Retranslate.onLanguageChanged;
                                    noteToast.show();
                                }
                            }
                        }
                    }

                    Button {
                        topMargin: bookManager.dp(12)
                        horizontalAlignment: HorizontalAlignment.Fill
                        text: (highlightId.indexOf("bm_") === 0 ? qsTr("Xóa Dấu Trang") : qsTr("Xóa Highlight")) + Retranslate.onLanguageChanged
                        onClicked: {
                            deleteConfirmDialog.body = (highlightId.indexOf("bm_") === 0 ? qsTr("Bạn có chắc muốn xóa dấu trang này?") : qsTr("Bạn có chắc muốn xóa highlight này?")) + Retranslate.onLanguageChanged;
                            deleteConfirmDialog.show();
                        }
                    }
                }
            }
        }
    }

    attachedObjects: [
        SystemToast {
            id: noteToast
            body: ""
        },
        SystemDialog {
            id: deleteConfirmDialog
            title: qsTr("Xác nhận xóa") + Retranslate.onLanguageChanged
            confirmButton.label: qsTr("Xóa") + Retranslate.onLanguageChanged
            cancelButton.label: qsTr("Hủy") + Retranslate.onLanguageChanged
            onFinished: {
                if (value === SystemUiResult.ConfirmButtonSelection) {
                    deleteAndClose();
                }
            }
        }
    ]

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
