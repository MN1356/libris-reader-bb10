import bb.cascades 1.3

Container {
    id: dayCell
    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    preferredHeight: cellHeight
    minHeight: cellHeight

    property real uiScale: 1.0
    property int cellHeight: 74
    property string dateStr: ""
    property int dayNumber: 0
    property bool isCurrentMonth: true
    property bool isToday: false
    property bool isSelected: false
    property int readingSeconds: 0
    property string durationText: ""
    property int intensityLevel: 0
    property string appTheme: "dark"

    signal tapped(string date)

    topPadding: isSelected ? bookManager.dp(2) : bookManager.dp(1)
    bottomPadding: isSelected ? bookManager.dp(2) : bookManager.dp(1)
    leftPadding: isSelected ? bookManager.dp(2) : bookManager.dp(1)
    rightPadding: isSelected ? bookManager.dp(2) : bookManager.dp(1)
    background: isSelected ?
                Color.create(bookManager.uiAccent) :
                (isToday ? (appTheme === "dark" ? Color.create("#8C672E") : Color.create("#B8893D")) :
                Color.create(bookManager.uiBg))

    layout: DockLayout {}

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: {
            if (isSelected) {
                return appTheme === "dark" ? Color.create("#261F15") : Color.create("#EFE4CF");
            }
            if (!isCurrentMonth) {
                return appTheme === "dark" ? Color.create("#0E0C0A") : Color.create("#EFE9DF");
            }
            if (intensityLevel > 0 || readingSeconds > 0) {
                if (intensityLevel >= 4) {
                    return appTheme === "dark" ? Color.create("#855E24") : Color.create("#D9AB60");
                } else if (intensityLevel >= 3) {
                    return appTheme === "dark" ? Color.create("#5C431C") : Color.create("#E6C58D");
                } else if (intensityLevel >= 2) {
                    return appTheme === "dark" ? Color.create("#3B2C16") : Color.create("#F2DFBA");
                } else {
                    return appTheme === "dark" ? Color.create("#231C11") : Color.create("#FAF2E1");
                }
            }
            return Color.create(bookManager.uiCard);
        }

        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

        topPadding: bookManager.dp(5)
        bottomPadding: bookManager.dp(4)
        leftPadding: bookManager.dp(2)
        rightPadding: bookManager.dp(2)

        // Day Number Slot
        Container {
            horizontalAlignment: HorizontalAlignment.Center
            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
            layout: DockLayout {}

            // Today Accent Badge
            Container {
                visible: isToday && dayNumber > 0
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                topPadding: bookManager.dp(1)
                bottomPadding: bookManager.dp(1)
                leftPadding: bookManager.dp(6)
                rightPadding: bookManager.dp(6)
                minWidth: bookManager.dp(24)
                background: Color.create(bookManager.uiAccent)
                layout: DockLayout {}

                Label {
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    text: dayNumber > 0 ? ("" + dayNumber) : ""
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.textAlign: TextAlign.Center
                    textStyle.color: appTheme === "dark" ? Color.create("#0E0C0A") : Color.create("#FFFFFF")
                    autoSize.maxLineCount: 1
                }
            }

            // Standard Day Number
            Label {
                visible: !isToday && dayNumber > 0
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                text: dayNumber > 0 ? ("" + dayNumber) : ""
                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                textStyle.fontWeight: isSelected ? FontWeight.Bold : FontWeight.Normal
                textStyle.textAlign: TextAlign.Center
                autoSize.maxLineCount: 1
                textStyle.color: {
                    if (isSelected) {
                        return Color.create(bookManager.uiInk);
                    }
                    if (!isCurrentMonth) {
                        return Color.create(bookManager.uiMuted);
                    }
                    if (intensityLevel >= 4) {
                        return appTheme === "dark" ? Color.create("#F5EFE6") : Color.create("#1C1916");
                    }
                    return Color.create(bookManager.uiInk);
                }
            }
        }

        // Clean Reading Duration Text (no nested box to prevent clipping)
        Label {
            visible: durationText !== "" && isCurrentMonth
            horizontalAlignment: HorizontalAlignment.Center
            topMargin: bookManager.dp(2)
            text: durationText
            textStyle.base: SystemDefaults.TextStyles.SmallText
            textStyle.fontWeight: FontWeight.W500
            textStyle.textAlign: TextAlign.Center
            textStyle.color: Color.create(bookManager.uiAccent)
            autoSize.maxLineCount: 1
        }
    }

    gestureHandlers: [
        TapHandler {
            onTapped: {
                if (dayCell.dateStr !== "") {
                    dayCell.tapped(dayCell.dateStr);
                }
            }
        }
    ]
}
