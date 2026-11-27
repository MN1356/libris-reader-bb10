import bb.cascades 1.3

Container {
    id: dayCell
    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    preferredHeight: cellHeight
    minHeight: cellHeight

    topMargin: bookManager.dp(2)
    bottomMargin: bookManager.dp(2)
    leftMargin: bookManager.dp(2)
    rightMargin: bookManager.dp(2)

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
    property variant cellPaint
    property variant cellSelPaint
    property variant cellTodayPaint
    property variant cellOutPaint

    signal tapped(string date)

    background: {
        if (isSelected) return cellSelPaint;
        if (isToday) return cellTodayPaint;
        if (!isCurrentMonth) return cellOutPaint;
        return cellPaint;
    }

    layout: DockLayout {}

    // Inner content (Day number + optional short duration)
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        topPadding: bookManager.dp(6)
        bottomPadding: bookManager.dp(13)
        leftPadding: bookManager.dp(2)
        rightPadding: bookManager.dp(2)
        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

        // Day Number Slot (Today uses cell border + accent bold number, no filled yellow box)
        Container {
            horizontalAlignment: HorizontalAlignment.Center
            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
            layout: DockLayout {}

            Label {
                visible: dayNumber > 0
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                text: dayNumber > 0 ? ("" + dayNumber) : ""
                textStyle.base: bookManager.isHiRes ? SystemDefaults.TextStyles.TitleText : SystemDefaults.TextStyles.SubtitleText
                textStyle.fontWeight: (isSelected || isToday) ? FontWeight.Bold : FontWeight.Normal
                textStyle.textAlign: TextAlign.Center
                autoSize.maxLineCount: 1
                textStyle.color: {
                    if (isSelected || isToday) return Color.create(bookManager.uiAccent);
                    if (!isCurrentMonth) return Color.create(bookManager.uiMuted);
                    return Color.create(bookManager.uiInk);
                }
            }
        }

        // Clean Reading Duration Text
        Label {
            visible: durationText !== "" && isCurrentMonth
            horizontalAlignment: HorizontalAlignment.Center
            topMargin: bookManager.dp(1)
            text: durationText
            textStyle.base: SystemDefaults.TextStyles.SmallText
            textStyle.fontWeight: FontWeight.W500
            textStyle.textAlign: TextAlign.Center
            textStyle.color: Color.create(bookManager.uiMuted)
            autoSize.maxLineCount: 1
        }
    }

    // P4 Bottom Intensity Bar (scales width & gold intensity by reading level)
    Container {
        visible: isCurrentMonth && (readingSeconds > 0 || intensityLevel > 0)
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Bottom
        leftPadding: bookManager.dp(4)
        rightPadding: bookManager.dp(4)
        bottomPadding: bookManager.dp(3)

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: bookManager.dp(4)
            background: Color.create(bookManager.uiHairline)
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            Container {
                verticalAlignment: VerticalAlignment.Fill
                layoutProperties: StackLayoutProperties {
                    spaceQuota: intensityLevel >= 4 ? 100 : (intensityLevel >= 3 ? 80 : (intensityLevel >= 2 ? 55 : 30))
                }
                background: {
                    if (intensityLevel >= 4) return Color.create(bookManager.uiAccent);
                    if (intensityLevel >= 3) return appTheme === "dark" ? Color.create("#C99A4E") : Color.create("#B57F2A");
                    if (intensityLevel >= 2) return appTheme === "dark" ? Color.create("#8C672E") : Color.create("#D9B06A");
                    return appTheme === "dark" ? Color.create("#5C431C") : Color.create("#EBD2A3");
                }
            }

            Container {
                visible: intensityLevel < 4
                verticalAlignment: VerticalAlignment.Fill
                background: Color.Transparent
                layoutProperties: StackLayoutProperties {
                    spaceQuota: intensityLevel >= 4 ? 0.001 : (intensityLevel >= 3 ? 20 : (intensityLevel >= 2 ? 45 : 70))
                }
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
