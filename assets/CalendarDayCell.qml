import bb.cascades 1.3

Container {
    id: dayCell
    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
    horizontalAlignment: HorizontalAlignment.Fill
    preferredHeight: 96
    minHeight: 88

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

    // Outer border (highlighted on selection or today)
    topPadding: 1
    bottomPadding: 1
    leftPadding: 1
    rightPadding: 1
    background: isSelected ?
                Color.create("#10B981") :
                (isToday ? Color.create("#059669") :
                (appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")))

    layout: DockLayout {}

    // Inner cell area
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: {
            if (isSelected) {
                return appTheme === "dark" ? Color.create("#064E3B") : Color.create("#D1FAE5");
            }
            if (!isCurrentMonth) {
                return appTheme === "dark" ? Color.create("#0A0C10") : Color.create("#FAFAFA");
            }
            if (intensityLevel > 0 || readingSeconds > 0) {
                if (intensityLevel >= 4) {
                    return appTheme === "dark" ? Color.create("#134E35") : Color.create("#A7F3D0");
                } else if (intensityLevel >= 3) {
                    return appTheme === "dark" ? Color.create("#0F3F2A") : Color.create("#BBF7D0");
                } else if (intensityLevel >= 2) {
                    return appTheme === "dark" ? Color.create("#0B3020") : Color.create("#D1FAE5");
                } else {
                    return appTheme === "dark" ? Color.create("#082216") : Color.create("#ECFDF5");
                }
            }
            return appTheme === "dark" ? Color.create("#12141A") : Color.create("#FFFFFF");
        }

        layout: StackLayout {
            orientation: LayoutOrientation.TopToBottom
        }

        topPadding: 6
        bottomPadding: 6
        leftPadding: 2
        rightPadding: 2

        // Top: Day Number Container (Today badge circle or plain text)
        Container {
            horizontalAlignment: HorizontalAlignment.Center
            preferredHeight: 30
            minHeight: 30
            layout: DockLayout {}

            // 1A. Today indicator badge (Pill/Circle around day number)
            Container {
                visible: isToday && dayNumber > 0
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: 38
                preferredHeight: 26
                minWidth: 34
                minHeight: 24
                background: Color.create("#10B981")
                layout: DockLayout {}

                Label {
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    text: dayNumber > 0 ? ("" + dayNumber) : ""
                    textStyle.fontSize: FontSize.XSmall
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.textAlign: TextAlign.Center
                    textStyle.color: Color.White
                }
            }

            // 1B. Standard Day Number (when not today)
            Label {
                visible: !isToday && dayNumber > 0
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                text: dayNumber > 0 ? ("" + dayNumber) : ""
                textStyle.fontSize: FontSize.XSmall
                textStyle.fontWeight: isSelected ? FontWeight.Bold : FontWeight.Normal
                textStyle.textAlign: TextAlign.Center
                textStyle.color: {
                    if (isSelected) {
                        return appTheme === "dark" ? Color.White : Color.create("#065F46");
                    }
                    if (!isCurrentMonth) {
                        return appTheme === "dark" ? Color.create("#555555") : Color.create("#AAAAAA");
                    }
                    if (intensityLevel > 0 || readingSeconds > 0) {
                        return appTheme === "dark" ? Color.White : Color.create("#065F46");
                    }
                    return appTheme === "dark" ? Color.create("#E5E5E5") : Color.create("#000000");
                }
            }
        }

        // Bottom: Centered Duration Pill
        Container {
            visible: durationText !== "" && isCurrentMonth
            horizontalAlignment: HorizontalAlignment.Center
            topMargin: 4
            leftPadding: 4
            rightPadding: 4
            topPadding: 1
            bottomPadding: 1
            preferredHeight: 20
            minHeight: 20
            layout: DockLayout {}
            background: isSelected ?
                        (appTheme === "dark" ? Color.create("#10B981") : Color.create("#059669")) :
                        (appTheme === "dark" ? Color.create("#065F46") : Color.create("#10B981"))

            Label {
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                text: durationText
                textStyle.fontSize: FontSize.XXSmall
                textStyle.fontWeight: FontWeight.Bold
                textStyle.textAlign: TextAlign.Center
                textStyle.color: Color.White
                autoSize.maxLineCount: 1
            }
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
