import bb.cascades 1.3

Page {
    id: opdsBookDetailPage

    property string bookTitle: ""
    property string bookAuthor: ""
    property string bookCoverUrl: ""
    property string bookFormat: "epub"
    property string bookFormatBadge: "EPUB"
    property string bookDownloadUrl: ""
    property string bookEpubUrl: ""
    property string bookPdfUrl: ""
    property string bookSummary: ""

    signal downloadRequested(string title, string author, string downloadUrl, string format, string coverUrl)
    signal backRequested()

    actionBarVisibility: ChromeVisibility.Visible

    titleBar: TitleBar {
        appearance: TitleBarAppearance.Plain
        title: qsTr("Chi Tiết Sách") + Retranslate.onLanguageChanged

        dismissAction: ActionItem {
            title: qsTr("Lùi lại") + Retranslate.onLanguageChanged
            onTriggered: {
                opdsBookDetailPage.backRequested();
            }
        }

        acceptAction: ActionItem {
            title: qsTr("Tải về") + Retranslate.onLanguageChanged
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
            scrollViewProperties.scrollMode: ScrollMode.Vertical

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                bottomPadding: bookManager.dp(36)

                // =============================================================
                // 1. HERO BOOK CARD (COVER + TITLES + DOWNLOAD CTA)
                // =============================================================
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(16)
                    bottomPadding: bookManager.dp(16)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(16)
                        bottomPadding: bookManager.dp(16)
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            // Scaled Cover Box
                            Container {
                                preferredWidth: bookManager.dp(106)
                                preferredHeight: bookManager.dp(152)
                                minWidth: bookManager.dp(106)
                                maxWidth: bookManager.dp(106)
                                minHeight: bookManager.dp(152)
                                maxHeight: bookManager.dp(152)
                                verticalAlignment: VerticalAlignment.Top
                                background: Color.create(bookManager.uiSurface)
                                layout: DockLayout {}

                                ImageView {
                                    property bool isLocalCover: opdsBookDetailPage.bookCoverUrl !== "" &&
                                                                (opdsBookDetailPage.bookCoverUrl.indexOf("file://") === 0 || opdsBookDetailPage.bookCoverUrl.indexOf("asset:///") === 0 || opdsBookDetailPage.bookCoverUrl.indexOf("/") === 0)
                                    visible: isLocalCover
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    imageSource: isLocalCover ? (opdsBookDetailPage.bookCoverUrl.indexOf("/") === 0 ? ("file://" + opdsBookDetailPage.bookCoverUrl) : opdsBookDetailPage.bookCoverUrl) : ""
                                    scalingMethod: ScalingMethod.AspectFill
                                }

                                Container {
                                    visible: opdsBookDetailPage.bookCoverUrl === "" ||
                                             opdsBookDetailPage.bookCoverUrl.indexOf("http://") === 0 || opdsBookDetailPage.bookCoverUrl.indexOf("https://") === 0
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    verticalAlignment: VerticalAlignment.Fill
                                    layout: DockLayout {}

                                    ImageView {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        imageSource: "asset:///icons/default_cover.png"
                                        scalingMethod: ScalingMethod.AspectFill
                                    }

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Center
                                        verticalAlignment: VerticalAlignment.Center
                                        leftPadding: bookManager.dp(8)
                                        rightPadding: bookManager.dp(8)
                                        bottomPadding: bookManager.dp(14)

                                        Label {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            verticalAlignment: VerticalAlignment.Center
                                            text: opdsBookDetailPage.bookTitle
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.W500
                                            textStyle.color: Color.create("#F5EFE6")
                                            multiline: true
                                            autoSize.maxLineCount: 3
                                        }
                                    }
                                }
                            }

                            // Book Title & Author
                            Container {
                                leftMargin: bookManager.dp(16)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                verticalAlignment: VerticalAlignment.Top

                                Label {
                                    text: opdsBookDetailPage.bookTitle
                                    textStyle.base: SystemDefaults.TextStyles.TitleText
                                    textStyle.fontWeight: FontWeight.W500
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    multiline: true
                                    autoSize.maxLineCount: 3
                                }

                                Label {
                                    topMargin: bookManager.dp(4)
                                    visible: opdsBookDetailPage.bookAuthor !== ""
                                    text: opdsBookDetailPage.bookAuthor
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.fontWeight: FontWeight.W500
                                    textStyle.color: Color.create(bookManager.uiAccent)
                                    multiline: true
                                    autoSize.maxLineCount: 2
                                }

                                // Format Pill
                                Container {
                                    topMargin: bookManager.dp(10)
                                    background: Color.create(bookManager.uiSurface)
                                    leftPadding: bookManager.dp(10)
                                    rightPadding: bookManager.dp(10)
                                    topPadding: bookManager.dp(4)
                                    bottomPadding: bookManager.dp(4)
                                    horizontalAlignment: HorizontalAlignment.Left

                                    Label {
                                        text: (qsTr("Định dạng: ") + (opdsBookDetailPage.bookFormatBadge !== "" ? opdsBookDetailPage.bookFormatBadge : "EPUB")) + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.W500
                                        textStyle.color: Color.create(bookManager.uiAccent)
                                    }
                                }
                            }
                        }

                        // Format Picker when both EPUB and PDF are available
                        Container {
                            visible: opdsBookDetailPage.bookEpubUrl !== "" && opdsBookDetailPage.bookPdfUrl !== "" && opdsBookDetailPage.bookEpubUrl !== opdsBookDetailPage.bookPdfUrl
                            topMargin: bookManager.dp(14)
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Button {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: "EPUB"
                                appearance: opdsBookDetailPage.bookFormat === "epub" ? ControlAppearance.Primary : ControlAppearance.Default
                                onClicked: {
                                    opdsBookDetailPage.bookFormat = "epub";
                                    opdsBookDetailPage.bookFormatBadge = "EPUB";
                                    opdsBookDetailPage.bookDownloadUrl = opdsBookDetailPage.bookEpubUrl;
                                }
                            }

                            Button {
                                leftMargin: bookManager.dp(10)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: "PDF"
                                appearance: opdsBookDetailPage.bookFormat === "pdf" ? ControlAppearance.Primary : ControlAppearance.Default
                                onClicked: {
                                    opdsBookDetailPage.bookFormat = "pdf";
                                    opdsBookDetailPage.bookFormatBadge = "PDF";
                                    opdsBookDetailPage.bookDownloadUrl = opdsBookDetailPage.bookPdfUrl;
                                }
                            }
                        }

                        // Download Action Button
                        Button {
                            topMargin: bookManager.dp(14)
                            horizontalAlignment: HorizontalAlignment.Fill
                            appearance: ControlAppearance.Primary
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
                }

                // =============================================================
                // 2. TÓM TẮT & NỘI DUNG GIỚI THIỆU (Edge-to-edge Header)
                // =============================================================
                Header {
                    title: qsTr("TÓM TẮT NỘI DUNG") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(16)
                        bottomPadding: bookManager.dp(16)
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)

                        Label {
                            text: opdsBookDetailPage.bookSummary !== "" ?
                                  opdsBookDetailPage.bookSummary :
                                  (qsTr("Chưa có phần tóm tắt nội dung giới thiệu cho tác phẩm này.") + Retranslate.onLanguageChanged)
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.color: Color.create(bookManager.uiInk)
                            multiline: true
                        }
                    }
                }
            }
        }
    }
}
