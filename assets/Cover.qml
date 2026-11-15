import bb.cascades 1.3

Container {
    id: coverRootContainer
    layout: DockLayout {}
    background: Color.create("#000000")
    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill

    property string bookTitle: (bookManager.currentBookTitle !== "") ? bookManager.currentBookTitle :
                               (bookManager.hasLastBook ? bookManager.lastBookTitle : "")

    property string coverPath: (bookManager.currentCoverPath !== "") ? bookManager.currentCoverPath :
                               (bookManager.hasLastBook ? bookManager.lastBookCover : "")

    property int progressValue: (bookManager.currentFilePath !== "") ? (bookManager.readingProgress >= 0 ? bookManager.readingProgress : 0) :
                                (bookManager.hasLastBook ? (bookManager.lastBookProgress >= 0 ? bookManager.lastBookProgress : 0) : 0)

    // =========================================================================
    // 1. ACTIVE BOOK STATE (Fullscreen Cover, Bottom Title & Sleek Progress Bar)
    // =========================================================================
    Container {
        visible: bookTitle !== ""
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        layout: DockLayout {}

        // Fullscreen Enlarged Book Cover
        ImageView {
            visible: coverPath !== ""
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            imageSource: coverPath !== "" ? ("file://" + coverPath) : ""
            scalingMethod: ScalingMethod.AspectFill
        }

        // Placeholder when no cover image exists
        Container {
            visible: coverPath === ""
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create("#111111")
            layout: DockLayout {}

            ImageView {
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: 96
                preferredHeight: 96
                opacity: 0.35
                imageSource: "asset:///icons/ic_book.png"
                scalingMethod: ScalingMethod.AspectFit
            }
        }

        // Bottom Edge Overlay: Title & Sleek Progress Bar
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Bottom
            background: Color.create("#E6000000")

            layout: StackLayout {
                orientation: LayoutOrientation.TopToBottom
            }

            // Book Title Container
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: 10
                bottomPadding: 8
                leftPadding: 12
                rightPadding: 12

                Label {
                    horizontalAlignment: HorizontalAlignment.Fill
                    text: bookTitle
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.White
                    multiline: true
                    autoSize.maxLineCount: 2
                }
            }

            // Sleek Edge-to-Edge Progress Bar
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: 5
                minHeight: 5
                background: Color.create("#222222")
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }

                Container {
                    verticalAlignment: VerticalAlignment.Fill
                    background: progressValue >= 100 ? Color.create("#059669") : Color.create("#10B981")
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

    // =========================================================================
    // 2. EMPTY STATE (When no book is open and no previous book exists)
    // =========================================================================
    Container {
        visible: bookTitle === ""
        horizontalAlignment: HorizontalAlignment.Center
        verticalAlignment: VerticalAlignment.Center

        ImageView {
            horizontalAlignment: HorizontalAlignment.Center
            preferredWidth: 64
            preferredHeight: 64
            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_app_dark.png" : "asset:///icons/ic_app_light.png"
        }

        Label {
            topMargin: 8
            horizontalAlignment: HorizontalAlignment.Center
            text: "BerryLeaf"
            textStyle.base: SystemDefaults.TextStyles.TitleText
            textStyle.fontWeight: FontWeight.Bold
            textStyle.color: Color.White
        }

        Label {
            topMargin: 4
            horizontalAlignment: HorizontalAlignment.Center
            text: qsTr("Chưa có sách đang đọc") + Retranslate.onLanguageChanged
            textStyle.base: SystemDefaults.TextStyles.SmallText
            textStyle.color: Color.create("#888888")
        }
    }
}
