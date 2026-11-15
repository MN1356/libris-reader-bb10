import bb.cascades 1.3

Page {
    id: opdsBookDetailPage

    property string bookTitle: ""
    property string bookAuthor: ""
    property string bookCoverUrl: ""
    property string bookFormat: "epub"
    property string bookFormatBadge: "EPUB"
    property string bookDownloadUrl: ""
    property string bookSummary: ""

    signal downloadRequested(string title, string author, string downloadUrl, string format, string coverUrl)
    signal backRequested()

    actionBarVisibility: ChromeVisibility.Visible

    titleBar: TitleBar {
        appearance: TitleBarAppearance.Plain
        title: qsTr("Chi Tiết Sách") + Retranslate.onLanguageChanged

        dismissAction: ActionItem {
            title: qsTr("Lùi lại") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_previous.png"
            onTriggered: {
                opdsBookDetailPage.backRequested();
            }
        }

        acceptAction: ActionItem {
            title: qsTr("Tải về") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_download.png"
            enabled: opdsBookDetailPage.bookDownloadUrl !== ""
            onTriggered: {
                opdsBookDetailPage.downloadRequested(
                    opdsBookDetailPage.bookTitle,
                    opdsBookDetailPage.bookAuthor,
                    opdsBookDetailPage.bookDownloadUrl,
                    opdsBookDetailPage.bookFormat,
                    opdsBookDetailPage.bookCoverUrl
                );
            }
        }
    }

    actions: [
        ActionItem {
            title: qsTr("Tải sách về máy") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_download.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            enabled: opdsBookDetailPage.bookDownloadUrl !== ""
            onTriggered: {
                opdsBookDetailPage.downloadRequested(
                    opdsBookDetailPage.bookTitle,
                    opdsBookDetailPage.bookAuthor,
                    opdsBookDetailPage.bookDownloadUrl,
                    opdsBookDetailPage.bookFormat,
                    opdsBookDetailPage.bookCoverUrl
                );
            }
        },
        ActionItem {
            title: qsTr("Lùi lại") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_previous.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            onTriggered: {
                opdsBookDetailPage.backRequested();
            }
        }
    ]

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
        layout: DockLayout {}

        ScrollView {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            scrollViewProperties.scrollMode: ScrollMode.Vertical

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: 16
                bottomPadding: 36
                leftPadding: 16
                rightPadding: 16

                layout: StackLayout {
                    orientation: LayoutOrientation.TopToBottom
                }

                // =============================================================
                // 1. HERO BOOK CARD (COVER + TITLES + DOWNLOAD CTA)
                // =============================================================
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                    topPadding: 16
                    bottomPadding: 16
                    leftPadding: 16
                    rightPadding: 16
                    bottomMargin: 16

                    layout: StackLayout {
                        orientation: LayoutOrientation.TopToBottom
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        // Large Cover Box (100x145)
                        Container {
                            preferredWidth: 100
                            preferredHeight: 145
                            minWidth: 100
                            maxWidth: 100
                            minHeight: 145
                            maxHeight: 145
                            verticalAlignment: VerticalAlignment.Top
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                            layout: DockLayout {}

                            ImageView {
                                visible: opdsBookDetailPage.bookCoverUrl !== ""
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                imageSource: opdsBookDetailPage.bookCoverUrl
                                scalingMethod: ScalingMethod.AspectFill
                            }

                            Container {
                                visible: opdsBookDetailPage.bookCoverUrl === ""
                                horizontalAlignment: HorizontalAlignment.Center
                                verticalAlignment: VerticalAlignment.Center
                                ImageView {
                                    preferredWidth: 48
                                    preferredHeight: 48
                                    imageSource: "asset:///icons/ic_book.png"
                                    opacity: 0.55
                                }
                            }
                        }

                        // Book Title & Author
                        Container {
                            leftMargin: 16
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            verticalAlignment: VerticalAlignment.Top

                            Label {
                                text: opdsBookDetailPage.bookTitle
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                                multiline: true
                                autoSize.maxLineCount: 3
                            }

                            Label {
                                topMargin: 4
                                visible: opdsBookDetailPage.bookAuthor !== ""
                                text: opdsBookDetailPage.bookAuthor
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#10B981") : Color.create("#10B981")
                                multiline: true
                                autoSize.maxLineCount: 2
                            }

                            // Format Chip
                            Container {
                                topMargin: 8
                                background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                leftPadding: 8
                                rightPadding: 8
                                topPadding: 3
                                bottomPadding: 3
                                horizontalAlignment: HorizontalAlignment.Left

                                Label {
                                    text: (qsTr("Định dạng: ") + (opdsBookDetailPage.bookFormatBadge !== "" ? opdsBookDetailPage.bookFormatBadge : "EPUB")) + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#E4E4E7") : Color.create("#777777")
                                }
                            }
                        }
                    }

                    // Download Action Button
                    Button {
                        topMargin: 16
                        horizontalAlignment: HorizontalAlignment.Fill
                        text: (qsTr("Tải Sách Về Thiết Bị (") + (opdsBookDetailPage.bookFormatBadge !== "" ? opdsBookDetailPage.bookFormatBadge : "EPUB") + ")") + Retranslate.onLanguageChanged
                        imageSource: "asset:///icons/ic_download.png"
                        enabled: opdsBookDetailPage.bookDownloadUrl !== ""
                        onClicked: {
                            opdsBookDetailPage.downloadRequested(
                                opdsBookDetailPage.bookTitle,
                                opdsBookDetailPage.bookAuthor,
                                opdsBookDetailPage.bookDownloadUrl,
                                opdsBookDetailPage.bookFormat,
                                opdsBookDetailPage.bookCoverUrl
                            );
                        }
                    }
                }

                // =============================================================
                // 2. TÓM TẮT & NỘI DUNG GIỚI THIỆU
                // =============================================================
                Header {
                    title: qsTr("TÓM TẮT NỘI DUNG") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                    topPadding: 16
                    bottomPadding: 16
                    leftPadding: 16
                    rightPadding: 16

                    layout: StackLayout {
                        orientation: LayoutOrientation.TopToBottom
                    }

                    Label {
                        text: opdsBookDetailPage.bookSummary !== "" ?
                              opdsBookDetailPage.bookSummary :
                              (qsTr("Chưa có phần tóm tắt nội dung giới thiệu cho tác phẩm này.") + Retranslate.onLanguageChanged)
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#E4E4E7") : Color.create("#777777")
                        multiline: true
                    }
                }
            }
        }
    }
}
