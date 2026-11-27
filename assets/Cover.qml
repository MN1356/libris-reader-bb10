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

    property int goalSeconds: Math.max(60, bookManager.dailyGoalMinutes * 60)
    property int goalPct: Math.min(100, Math.round((bookManager.todayReadingSeconds * 100) / goalSeconds))

    // 1. Reader Active State: Book Cover + % Progress
    Container {
        visible: bookManager.isReaderActive && bookTitle !== ""
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
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                Label {
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    verticalAlignment: VerticalAlignment.Center
                    text: bookTitle
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create("#F5EFE6")
                    multiline: true
                    autoSize.maxLineCount: 2
                }

                Label {
                    leftMargin: bookManager.dp(8)
                    verticalAlignment: VerticalAlignment.Center
                    text: progressValue + "%"
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create("#D9AB60")
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

    // 2. Main UI Active Frame: Today Reading Time / Daily Goal over dimmed current book cover
    Container {
        visible: !bookManager.isReaderActive
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        layout: DockLayout {}

        // Dimmed book cover background
        ImageView {
            visible: bookTitle !== "" && coverPath !== ""
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            imageSource: coverPath !== "" ? ("file://" + coverPath) : ""
            scalingMethod: ScalingMethod.AspectFill
            opacity: 0.24
        }

        ImageView {
            visible: bookTitle !== "" && coverPath === ""
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            imageSource: {
                var fmt = coverRootContainer.bookFormat ? coverRootContainer.bookFormat.toLowerCase() : "epub";
                if (fmt === "azw" || fmt === "azw3" || fmt === "kf8" || fmt === "prc") fmt = "mobi";
                if (fmt !== "pdf" && fmt !== "epub" && fmt !== "mobi" && fmt !== "fb2" && fmt !== "cbz" && fmt !== "djvu" && fmt !== "txt") fmt = "epub";
                return "asset:///icons/default_cover_" + fmt + ".png";
            }
            scalingMethod: ScalingMethod.AspectFill
            opacity: 0.20
        }

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create(0.05, 0.04, 0.03, 0.55)
        }

        // Foreground Daily Goal & Today Time
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Center
            leftPadding: bookManager.dp(16)
            rightPadding: bookManager.dp(16)
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            Label {
                horizontalAlignment: HorizontalAlignment.Center
                text: qsTr("Hôm nay") + Retranslate.onLanguageChanged
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: Color.create("#B8AD9E")
            }

            Label {
                topMargin: bookManager.dp(4)
                horizontalAlignment: HorizontalAlignment.Center
                text: bookManager.todayReadingTimeStr
                textStyle.base: SystemDefaults.TextStyles.TitleText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: Color.create("#D9AB60")
            }

            Label {
                topMargin: bookManager.dp(4)
                horizontalAlignment: HorizontalAlignment.Center
                text: qsTr("Mục tiêu:") + " " + bookManager.dailyGoalMinutes + " " + qsTr("phút") + " (" + goalPct + "%)"
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: Color.create("#E6DFD3")
            }

            Container {
                topMargin: bookManager.dp(10)
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: bookManager.dp(6)
                minHeight: bookManager.dp(6)
                background: Color.create("#26211B")
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                Container {
                    verticalAlignment: VerticalAlignment.Fill
                    background: goalPct >= 100 ? Color.create("#5FA876") : Color.create("#D9AB60")
                    visible: goalPct > 0
                    layoutProperties: StackLayoutProperties { spaceQuota: Math.max(0.001, goalPct) }
                }
                Container {
                    verticalAlignment: VerticalAlignment.Fill
                    background: Color.Transparent
                    visible: goalPct < 100
                    layoutProperties: StackLayoutProperties { spaceQuota: Math.max(0.001, 100 - goalPct) }
                }
            }

            Label {
                visible: bookTitle !== ""
                topMargin: bookManager.dp(12)
                horizontalAlignment: HorizontalAlignment.Center
                text: bookTitle
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: Color.create("#C8BFB0")
                autoSize.maxLineCount: 1
            }
        }
    }
}
