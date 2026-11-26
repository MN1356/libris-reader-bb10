import bb.cascades 1.3

Container {
    id: coverRootContainer
    layout: DockLayout {}
    background: Color.create("#0E0C0A")
    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill

    property string bookTitle: (bookManager.currentBookTitle !== "") ? bookManager.currentBookTitle :
                               (bookManager.hasLastBook ? bookManager.lastBookTitle : "")

    property string coverPath: (bookManager.currentCoverPath !== "") ? bookManager.currentCoverPath :
                               (bookManager.hasLastBook ? bookManager.lastBookCover : "")

    property string bookFormat: (bookManager.currentFormat !== "") ? bookManager.currentFormat :
                               (bookManager.hasLastBook ? bookManager.lastBookFormat : "epub")

    property int progressValue: (bookManager.currentFilePath !== "") ? (bookManager.readingProgress >= 0 ? bookManager.readingProgress : 0) :
                                (bookManager.hasLastBook ? (bookManager.lastBookProgress >= 0 ? bookManager.lastBookProgress : 0) : 0)

    // 1. Active Book State
    Container {
        visible: bookTitle !== ""
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        layout: DockLayout {}

        ImageView {
            visible: coverPath !== ""
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            imageSource: coverPath !== "" ? ("file://" + coverPath) : ""
            scalingMethod: ScalingMethod.AspectFill
        }

        Container {
            visible: coverPath === ""
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            layout: DockLayout {}

            ImageView {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                imageSource: {
                    var fmt = coverRootContainer.bookFormat ? coverRootContainer.bookFormat.toLowerCase() : "epub";
                    if (fmt === "azw" || fmt === "azw3" || fmt === "kf8" || fmt === "prc") fmt = "mobi";
                    if (fmt !== "pdf" && fmt !== "epub" && fmt !== "mobi" && fmt !== "fb2" && fmt !== "cbz" && fmt !== "djvu" && fmt !== "txt") fmt = "epub";
                    return "asset:///icons/default_cover_" + fmt + ".png";
                }
                scalingMethod: ScalingMethod.AspectFill
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                leftPadding: bookManager.dp(18)
                rightPadding: bookManager.dp(18)
                bottomPadding: bookManager.dp(24)

                Label {
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    text: bookTitle
                    textStyle.base: SystemDefaults.TextStyles.TitleText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.textAlign: TextAlign.Center
                    textStyle.color: Color.create("#F5EFE6")
                    multiline: true
                    autoSize.maxLineCount: 3
                }
            }
        }

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Bottom
            background: Color.create("#E60E0C0A")
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            Container {
                visible: coverPath !== ""
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: bookManager.dp(8)
                bottomPadding: bookManager.dp(8)
                leftPadding: bookManager.dp(12)
                rightPadding: bookManager.dp(12)

                Label {
                    horizontalAlignment: HorizontalAlignment.Fill
                    text: bookTitle
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create("#F5EFE6")
                    multiline: true
                    autoSize.maxLineCount: 2
                }
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: bookManager.dp(4)
                minHeight: bookManager.dp(4)
                background: Color.create("#26211B")
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                Container {
                    verticalAlignment: VerticalAlignment.Fill
                    background: progressValue >= 100 ? Color.create("#E6C07B") : Color.create("#D9AB60")
                    visible: progressValue > 0
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: Math.max(0.001, progressValue)
                    }
                }
                Container {
                    verticalAlignment: VerticalAlignment.Fill
                    background: Color.Transparent
                    visible: progressValue < 100
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: Math.max(0.001, 100 - progressValue)
                    }
                }
            }
        }
    }

    // 2. Empty State
    Container {
        visible: bookTitle === ""
        horizontalAlignment: HorizontalAlignment.Center
        verticalAlignment: VerticalAlignment.Center

        ImageView {
            horizontalAlignment: HorizontalAlignment.Center
            preferredWidth: bookManager.dp(56)
            preferredHeight: bookManager.dp(56)
            imageSource: "asset:///icons/ic_app_dark.png"
        }

        Label {
            topMargin: bookManager.dp(8)
            horizontalAlignment: HorizontalAlignment.Center
            text: "Libris"
            textStyle.base: SystemDefaults.TextStyles.TitleText
            textStyle.fontWeight: FontWeight.Bold
            textStyle.color: Color.create("#D9AB60")
        }

        Label {
            topMargin: bookManager.dp(4)
            horizontalAlignment: HorizontalAlignment.Center
            text: qsTr("Chưa có sách đang đọc") + Retranslate.onLanguageChanged
            textStyle.base: SystemDefaults.TextStyles.SmallText
            textStyle.color: Color.create("#9C9488")
        }
    }
}
