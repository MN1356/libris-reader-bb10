import bb.cascades 1.3

Container {
    id: bookSlotItem
    topMargin: 8
    horizontalAlignment: HorizontalAlignment.Fill
    background: isSelected ?
                (appTheme === "dark" ? Color.create("#14221D") : Color.create("#ECFDF5")) :
                (appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF"))
    topPadding: 10
    bottomPadding: 10
    leftPadding: 12
    rightPadding: 12
    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

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
    property string appTheme: "dark"

    signal bookSelected(string path)
    signal bookOpened(string path)

    // Selection indicator bar on left
    Container {
        visible: bookSlotItem.isSelected
        preferredWidth: 4
        verticalAlignment: VerticalAlignment.Fill
        background: Color.create("#10B981")
        rightMargin: 8
    }

    // Cover Thumbnail
    Container {
        preferredWidth: 48
        preferredHeight: 66
        minWidth: 48
        minHeight: 66
        verticalAlignment: VerticalAlignment.Center
        background: appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
        layout: DockLayout {}

        ImageView {
            id: slotCover
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            scalingMethod: ScalingMethod.AspectFill
            imageSource: "asset:///icons/default_cover.png"
        }
    }

    // Book Information Column (Clickable to select)
    Container {
        leftMargin: 12
        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Center
        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

        // Title + Format Badge
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            Label {
                id: slotTitle
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                verticalAlignment: VerticalAlignment.Center
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: appTheme === "dark" ? Color.White : Color.create("#000000")
                autoSize.maxLineCount: 1
            }

            Container {
                leftMargin: 6
                verticalAlignment: VerticalAlignment.Center
                background: Color.create("#10B981")
                leftPadding: 6
                rightPadding: 6
                topPadding: 1
                bottomPadding: 1

                Label {
                    id: slotFormat
                    text: "EPUB"
                    textStyle.fontSize: FontSize.XXSmall
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.White
                }
            }
        }

        // Author + Duration Read
        Container {
            topMargin: 2
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            Label {
                id: slotAuthor
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                verticalAlignment: VerticalAlignment.Center
                textStyle.base: SystemDefaults.TextStyles.XSmallText
                textStyle.color: appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                autoSize.maxLineCount: 1
            }

            Label {
                id: slotDuration
                leftMargin: 6
                verticalAlignment: VerticalAlignment.Center
                textStyle.base: SystemDefaults.TextStyles.XSmallText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: Color.create("#10B981")
            }
        }

        // Progress Bar + %
        Container {
            topMargin: 6
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            Container {
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                preferredHeight: 6
                background: appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                Container {
                    verticalAlignment: VerticalAlignment.Fill
                    background: Color.create("#10B981")
                    layoutProperties: StackLayoutProperties { spaceQuota: Math.max(1, bookSlotItem.progressVal) }
                }
                Container {
                    verticalAlignment: VerticalAlignment.Fill
                    background: Color.Transparent
                    layoutProperties: StackLayoutProperties { spaceQuota: Math.max(1, 100 - bookSlotItem.progressVal) }
                }
            }

            Label {
                id: slotPageCount
                leftMargin: 8
                verticalAlignment: VerticalAlignment.Center
                textStyle.fontSize: FontSize.XXSmall
                textStyle.color: appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                visible: text !== "" && text.indexOf("%") === -1
            }

            Label {
                id: slotProgressPct
                leftMargin: 4
                verticalAlignment: VerticalAlignment.Center
                textStyle.fontSize: FontSize.XXSmall
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: Color.create("#10B981")
            }
        }
    }

    // Single Prominent Action Button: Đọc
    Button {
        leftMargin: 8
        preferredWidth: 64
        minWidth: 58
        verticalAlignment: VerticalAlignment.Center
        text: qsTr("Đọc") + Retranslate.onLanguageChanged
        appearance: bookSlotItem.isSelected ? ButtonAppearance.Primary : ButtonAppearance.Default
        onClicked: {
            bookSlotItem.bookOpened(bookSlotItem.filePath);
        }
    }

    gestureHandlers: [
        TapHandler {
            onTapped: {
                bookSlotItem.bookSelected(bookSlotItem.filePath);
            }
        }
    ]
}
