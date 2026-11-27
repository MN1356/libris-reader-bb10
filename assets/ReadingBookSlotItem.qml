import bb.cascades 1.3

Container {
    id: bookSlotItem
    topMargin: editorialMode ? 0 : bookManager.dp(8)
    horizontalAlignment: HorizontalAlignment.Fill
    background: editorialMode ?
                (isSelected ? Color.create(bookManager.uiActive) : Color.Transparent) :
                (isSelected ? Color.create(bookManager.uiActive) : Color.create(bookManager.uiCard))
    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

    property real uiScale: 1.0
    property string filePath: ""
    property alias titleText: slotTitle.text
    property alias authorText: slotAuthor.text
    property alias coverSource: slotCover.imageSource
    property alias formatText: slotFormat.text
    property alias durationText: slotDuration.text
    property alias pageCountText: slotPageCount.text
    property alias progressText: slotProgressPct.text
    property int progressVal: 0
    property bool isSelected: false
    property bool openOnRowTap: false
    property bool showActionButton: true
    property bool editorialMode: false
    property string appTheme: "dark"

    signal bookSelected(string path)
    signal bookOpened(string path)

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        topPadding: bookManager.dp(10)
        bottomPadding: bookManager.dp(10)
        leftPadding: editorialMode ? bookManager.dp(6) : bookManager.dp(12)
        rightPadding: editorialMode ? bookManager.dp(6) : bookManager.dp(12)
        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

    // Selection indicator bar on left
    Container {
        visible: bookSlotItem.isSelected
        preferredWidth: bookManager.dp(4)
        verticalAlignment: VerticalAlignment.Fill
        background: Color.create(bookManager.uiAccent)
        rightMargin: bookManager.dp(8)
    }

    // Cover Thumbnail
    Container {
        preferredWidth: bookManager.dp(46)
        preferredHeight: bookManager.dp(64)
        minWidth: bookManager.dp(46)
        minHeight: bookManager.dp(64)
        verticalAlignment: VerticalAlignment.Center
        background: Color.create(bookManager.uiSurface)
        layout: DockLayout {}

        ImageView {
            id: slotCover
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            scalingMethod: ScalingMethod.AspectFill
            imageSource: "asset:///icons/default_cover.png"
        }

        // Overlay Title on Default Cover
        Container {
            visible: ("" + slotCover.imageSource).indexOf("default_cover") !== -1
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center
            leftPadding: bookManager.dp(4)
            rightPadding: bookManager.dp(4)
            bottomPadding: bookManager.dp(6)

            Label {
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                text: slotTitle.text
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.fontWeight: FontWeight.W500
                textStyle.textAlign: TextAlign.Center
                textStyle.color: Color.create("#F5EFE6")
                multiline: true
                autoSize.maxLineCount: 2
            }
        }
    }

    // Book Information Column
    Container {
        leftMargin: bookManager.dp(10)
        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Center
        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

        // Title + Format Pill
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            Label {
                id: slotTitle
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                rightMargin: bookManager.dp(6)
                verticalAlignment: VerticalAlignment.Center
                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: Color.create(bookManager.uiInk)
                autoSize.maxLineCount: 1
            }

            Container {
                verticalAlignment: VerticalAlignment.Center
                background: Color.create(bookManager.uiSurface)
                leftPadding: bookManager.dp(6)
                rightPadding: bookManager.dp(6)
                topPadding: bookManager.dp(2)
                bottomPadding: bookManager.dp(2)

                Label {
                    id: slotFormat
                    text: "EPUB"
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create(bookManager.uiAccent)
                }
            }
        }

        // Author + Duration Read
        Container {
            topMargin: bookManager.dp(2)
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            Label {
                id: slotAuthor
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                rightMargin: bookManager.dp(6)
                verticalAlignment: VerticalAlignment.Center
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.fontWeight: FontWeight.Normal
                textStyle.color: Color.create(bookManager.uiMuted)
                autoSize.maxLineCount: 1
            }

            Label {
                id: slotDuration
                verticalAlignment: VerticalAlignment.Center
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: Color.create(bookManager.uiAccent)
                autoSize.maxLineCount: 1
            }
        }

        // Progress Bar + Page/Chapter Count + %
        Container {
            topMargin: bookManager.dp(6)
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            Container {
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                preferredHeight: bookManager.dp(5)
                background: Color.create(bookManager.uiHairline)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                Container {
                    visible: bookSlotItem.progressVal > 0
                    verticalAlignment: VerticalAlignment.Fill
                    background: Color.create(bookManager.uiAccent)
                    layoutProperties: StackLayoutProperties { spaceQuota: Math.max(0.001, bookSlotItem.progressVal) }
                }
                Container {
                    visible: bookSlotItem.progressVal < 100
                    verticalAlignment: VerticalAlignment.Fill
                    background: Color.Transparent
                    layoutProperties: StackLayoutProperties { spaceQuota: Math.max(0.001, 100 - bookSlotItem.progressVal) }
                }
            }

            Label {
                id: slotPageCount
                leftMargin: bookManager.dp(8)
                verticalAlignment: VerticalAlignment.Center
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: Color.create(bookManager.uiMuted)
                visible: text !== "" && text.indexOf("%") === -1
                autoSize.maxLineCount: 1
            }

            Label {
                id: slotProgressPct
                leftMargin: bookManager.dp(6)
                verticalAlignment: VerticalAlignment.Center
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: Color.create(bookManager.uiAccent)
                autoSize.maxLineCount: 1
            }
        }
    }

    // Compact Action Pill
    Container {
        visible: bookSlotItem.showActionButton
        leftMargin: bookManager.dp(8)
        verticalAlignment: VerticalAlignment.Center
        background: bookSlotItem.isSelected ?
                    Color.create(bookManager.uiAccent) :
                    Color.create(bookManager.uiSurface)
        leftPadding: bookManager.dp(10)
        rightPadding: bookManager.dp(10)
        topPadding: bookManager.dp(6)
        bottomPadding: bookManager.dp(6)
        layout: DockLayout {}

        Label {
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center
            text: qsTr("Đọc") + Retranslate.onLanguageChanged
            textStyle.base: SystemDefaults.TextStyles.SmallText
            textStyle.fontWeight: FontWeight.Bold
            textStyle.color: bookSlotItem.isSelected ?
                             (appTheme === "dark" ? Color.create("#0E0C0A") : Color.create("#FFFFFF")) :
                             Color.create(bookManager.uiAccent)
        }

        gestureHandlers: [
            TapHandler {
                onTapped: {
                    bookSlotItem.bookOpened(bookSlotItem.filePath);
                }
            }
        ]
    }

        gestureHandlers: [
            TapHandler {
                onTapped: {
                    if (bookSlotItem.openOnRowTap) {
                        bookSlotItem.bookOpened(bookSlotItem.filePath);
                    } else {
                        bookSlotItem.bookSelected(bookSlotItem.filePath);
                    }
                }
            }
        ]
    }

    Container {
        visible: bookSlotItem.editorialMode
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: 1
        background: Color.create(bookManager.uiHairline)
    }
}
