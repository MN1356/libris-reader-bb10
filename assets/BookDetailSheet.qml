import bb.cascades 1.3

Sheet {
    id: bookDetailSheet

    signal requestOpenBook(string filePath)

    property string bookTitle: ""
    property string bookAuthor: ""
    property string bookCover: ""
    property string bookFormat: ""
    property string bookPath: ""
    property string bookFileSize: ""
    property string bookDateModified: ""
    property string bookDescription: ""
    property string bookGenre: ""
    property string bookSeries: ""
    property int bookProgress: -1

    function openWithBook(item) {
        if (item) {
            bookTitle = item.title ? ("" + item.title) : "";
            bookAuthor = item.author ? ("" + item.author) : "";
            bookCover = item.cover ? ("" + item.cover) : "";
            bookFormat = item.format ? ("" + item.format) : "";
            bookPath = item.filePath ? ("" + item.filePath) : (item.path ? ("" + item.path) : "");
            bookFileSize = item.fileSize ? ("" + item.fileSize) : "";
            bookDateModified = item.dateModified ? ("" + item.dateModified) : (item.date ? ("" + item.date) : "");
            bookDescription = item.description ? ("" + item.description) : "";
            bookGenre = item.genre ? ("" + item.genre) : "";
            var s = item.series ? ("" + item.series) : (bookManager.currentBookSeries ? bookManager.currentBookSeries : "");
            var sIdx = item.seriesIndex !== undefined ? parseInt(item.seriesIndex) : (bookManager.currentBookSeriesIndex > 0 ? bookManager.currentBookSeriesIndex : 0);
            bookSeries = s !== "" ? (s + (sIdx > 0 ? (" #" + sIdx) : "")) : "";
            bookProgress = (item.progress !== undefined) ? parseInt(item.progress) : -1;
        }
        bookDetailSheet.open();
    }

    onClosed: {
        bookTitle = "";
        bookAuthor = "";
        bookCover = "";
        bookFormat = "";
        bookPath = "";
        bookFileSize = "";
        bookDateModified = "";
        bookDescription = "";
        bookGenre = "";
        bookSeries = "";
        bookProgress = -1;
    }

    Page {
        titleBar: TitleBar {
            title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain
            dismissAction: ActionItem {
                title: qsTr("Đóng") + Retranslate.onLanguageChanged
                onTriggered: {
                    bookDetailSheet.close();
                }
            }
        }

        Container {
            layout: DockLayout {}
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

                    // --- Top Header: Cover & Main Title/Author ---
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 16
                        bottomPadding: 16
                        leftPadding: 16
                        rightPadding: 16

                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }

                        // Cover Thumbnail
                        Container {
                            preferredWidth: 104
                            preferredHeight: 148
                            minWidth: 104
                            maxWidth: 104
                            verticalAlignment: VerticalAlignment.Center
                            background: bookManager.appTheme === "dark" ? Color.create("#1A1A1A") : Color.create("#E8E8E8")
                            layout: DockLayout {}

                            ImageView {
                                property string displayCover: bookDetailSheet.bookCover !== "" ? bookDetailSheet.bookCover : bookManager.currentCoverPath
                                visible: displayCover !== ""
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                imageSource: displayCover !== "" ? ("file://" + displayCover) : ""
                                scalingMethod: ScalingMethod.AspectFill
                            }

                            Container {
                                property string displayCover: bookDetailSheet.bookCover !== "" ? bookDetailSheet.bookCover : bookManager.currentCoverPath
                                property string displayFmt: (bookDetailSheet.bookFormat !== "" ? bookDetailSheet.bookFormat : bookManager.currentFormat).toLowerCase()
                                visible: displayCover === ""
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                layout: DockLayout {}

                                ImageView {
                                    horizontalAlignment: HorizontalAlignment.Center
                                    verticalAlignment: VerticalAlignment.Center
                                    preferredWidth: 44
                                    preferredHeight: 44
                                    imageSource: "asset:///icons/ic_book.png"
                                    opacity: 0.5
                                }

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Bottom
                                    background: displayFmt === "pdf" ? Color.create("#DC2626") :
                                                displayFmt === "epub" ? Color.create("#10B981") :
                                                displayFmt === "fb2" ? Color.create("#9333EA") :
                                                Color.create("#2563EB")
                                    topPadding: 3
                                    bottomPadding: 3

                                    Label {
                                        horizontalAlignment: HorizontalAlignment.Center
                                        verticalAlignment: VerticalAlignment.Center
                                        text: (bookDetailSheet.bookFormat !== "" ? bookDetailSheet.bookFormat : bookManager.currentFormat).toUpperCase()
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.White
                                    }
                                }
                            }
                        }

                        // Right: Title, Author & Format Badge
                        Container {
                            leftMargin: 16
                            verticalAlignment: VerticalAlignment.Center
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1.0
                            }

                            Label {
                                text: bookDetailSheet.bookTitle !== "" ? bookDetailSheet.bookTitle : bookManager.currentBookTitle
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                multiline: true
                                autoSize.maxLineCount: 3
                            }

                            Label {
                                topMargin: 4
                                text: bookDetailSheet.bookAuthor !== "" ? bookDetailSheet.bookAuthor : (bookManager.currentBookAuthor !== "" ? bookManager.currentBookAuthor : (qsTr("Tác giả chưa rõ") + Retranslate.onLanguageChanged))
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#AAAAAA") : Color.create("#666666")
                                multiline: true
                                autoSize.maxLineCount: 2
                            }

                            Container {
                                topMargin: 8
                                layout: StackLayout {
                                    orientation: LayoutOrientation.LeftToRight
                                }

                                Container {
                                    background: Color.create("#2010B981")
                                    leftPadding: 8
                                    rightPadding: 8
                                    topPadding: 3
                                    bottomPadding: 3
                                    verticalAlignment: VerticalAlignment.Center

                                    Label {
                                        text: (bookDetailSheet.bookFormat !== "" ? bookDetailSheet.bookFormat : bookManager.currentFormat).toUpperCase()
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create("#10B981")
                                    }
                                }

                                Label {
                                    leftMargin: 10
                                    verticalAlignment: VerticalAlignment.Center
                                    text: bookDetailSheet.bookFileSize !== "" ? bookDetailSheet.bookFileSize : bookManager.currentBookFileSize
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                }
                            }
                        }
                    }

                    // Prominent Read Primary Button
                    Container {
                        topMargin: 14
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 64
                        minHeight: 56
                        background: Color.create("#10B981")
                        layout: DockLayout {}

                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    var p = bookDetailSheet.bookPath !== "" ? bookDetailSheet.bookPath : bookManager.currentFilePath;
                                    bookDetailSheet.close();
                                    bookDetailSheet.requestOpenBook(p);
                                }
                            }
                        ]

                        Container {
                            horizontalAlignment: HorizontalAlignment.Center
                            verticalAlignment: VerticalAlignment.Center
                            layout: StackLayout {
                                orientation: LayoutOrientation.LeftToRight
                            }

                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: 32
                                preferredHeight: 32
                                imageSource: "asset:///icons/ic_reading.png"
                            }

                            Label {
                                leftMargin: 10
                                verticalAlignment: VerticalAlignment.Center
                                text: qsTr("Đọc cuốn sách này") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.White
                            }
                        }
                    }

                    // --- Section 1: Thông tin xuất bản & Tập tin ---
                    Header {
                        topMargin: 20
                        title: qsTr("THÔNG TIN XUẤT BẢN & TẬP TIN") + Retranslate.onLanguageChanged
                    }

                    Container {
                        topMargin: 8
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 14
                        bottomPadding: 14
                        leftPadding: 16
                        rightPadding: 16

                        // Row: Bộ sách
                        Container {
                            visible: bookDetailSheet.bookSeries !== ""
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Label {
                                preferredWidth: 210
                                minWidth: 180
                                text: qsTr("Bộ sách") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                            }
                            Label {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: bookDetailSheet.bookSeries
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                multiline: true
                            }
                        }

                        // Divider
                        Container {
                            visible: bookDetailSheet.bookSeries !== ""
                            topMargin: 10
                            bottomMargin: 10
                            horizontalAlignment: HorizontalAlignment.Fill
                            preferredHeight: 1
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        }

                        // Row: Thể loại (Ẩn khi rỗng hoặc spam)
                        Container {
                            property string displayGenre: bookDetailSheet.bookGenre !== "" ? bookDetailSheet.bookGenre : bookManager.currentBookGenre
                            visible: displayGenre !== ""
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Label {
                                preferredWidth: 210
                                minWidth: 180
                                text: qsTr("Thể loại") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                            }
                            Label {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: parent.displayGenre
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                multiline: true
                            }
                        }

                        // Divider
                        Container {
                            property string displayGenre: bookDetailSheet.bookGenre !== "" ? bookDetailSheet.bookGenre : bookManager.currentBookGenre
                            visible: displayGenre !== ""
                            topMargin: 10
                            bottomMargin: 10
                            horizontalAlignment: HorizontalAlignment.Fill
                            preferredHeight: 1
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        }

                        // Row: Tiến độ đọc
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Label {
                                preferredWidth: 210
                                minWidth: 180
                                text: qsTr("Tiến độ đọc") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                            }

                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                verticalAlignment: VerticalAlignment.Center

                                Label {
                                    property int p: bookDetailSheet.bookProgress >= 0 ? bookDetailSheet.bookProgress : bookManager.readingProgress
                                    text: p + "%"
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: p >= 100 ? Color.create("#10B981") : (bookManager.appTheme === "dark" ? Color.White : Color.Black)
                                    verticalAlignment: VerticalAlignment.Center
                                }

                                Container {
                                    property int p: bookDetailSheet.bookProgress >= 0 ? bookDetailSheet.bookProgress : bookManager.readingProgress
                                    leftMargin: 12
                                    verticalAlignment: VerticalAlignment.Center
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    preferredHeight: 6
                                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    Container {
                                        preferredHeight: 6
                                        background: Color.create("#10B981")
                                        layoutProperties: StackLayoutProperties {
                                            spaceQuota: Math.max(1, parent.p)
                                        }
                                    }
                                    Container {
                                        preferredHeight: 6
                                        background: Color.Transparent
                                        layoutProperties: StackLayoutProperties {
                                            spaceQuota: Math.max(0.001, 100 - parent.p)
                                        }
                                    }
                                }
                            }
                        }

                        // Divider
                        Container {
                            topMargin: 10
                            bottomMargin: 10
                            horizontalAlignment: HorizontalAlignment.Fill
                            preferredHeight: 1
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        }

                        // Row: Ngày cập nhật
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Label {
                                preferredWidth: 210
                                minWidth: 180
                                text: qsTr("Ngày sửa đổi") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                            }
                            Label {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: bookDetailSheet.bookDateModified !== "" ? bookDetailSheet.bookDateModified : bookManager.currentBookDateModified
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            }
                        }

                        // Divider
                        Container {
                            topMargin: 10
                            bottomMargin: 10
                            horizontalAlignment: HorizontalAlignment.Fill
                            preferredHeight: 1
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        }

                        // Row: Đường dẫn tệp
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Label {
                                preferredWidth: 210
                                minWidth: 180
                                text: qsTr("Đường dẫn tệp") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                            }
                            Label {
                                property string rawPath: bookDetailSheet.bookPath !== "" ? bookDetailSheet.bookPath : bookManager.currentFilePath
                                property string friendlyPath: rawPath.indexOf("/accounts/1000/removable/sdcard") >= 0 ?
                                                              rawPath.replace(new RegExp("^/accounts/1000/removable/sdcard[0-9]*/?"), "SD Card / ") :
                                                              (rawPath.indexOf("/accounts/1000/shared") >= 0 ? rawPath.replace(new RegExp("^/accounts/1000/shared/?"), "Internal / ") : rawPath)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: friendlyPath
                                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#AAAAAA") : Color.create("#555555")
                                multiline: true
                            }
                        }
                    }

                    // --- Section 2: Mô tả sách ---
                    Header {
                        topMargin: 20
                        title: qsTr("MÔ TẢ SÁCH") + Retranslate.onLanguageChanged
                    }

                    Container {
                        topMargin: 8
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 16
                        bottomPadding: 16
                        leftPadding: 16
                        rightPadding: 16

                        Label {
                            property string displayDesc: bookDetailSheet.bookDescription !== "" ? bookDetailSheet.bookDescription : bookManager.currentBookDescription
                            visible: displayDesc !== ""
                            text: displayDesc
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.lineHeight: 1.25
                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                            multiline: true
                        }

                        Label {
                            property string displayDesc: bookDetailSheet.bookDescription !== "" ? bookDetailSheet.bookDescription : bookManager.currentBookDescription
                            visible: displayDesc === ""
                            text: qsTr("Chưa có mô tả cho cuốn sách này.") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.fontStyle: FontStyle.Italic
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                        }
                    }
                }
            }
        }
    }
}
