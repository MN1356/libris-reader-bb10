import bb.cascades 1.3

Container {
    id: trendBarItem
    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
    horizontalAlignment: HorizontalAlignment.Fill
    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

    property real uiScale: 1.0
    property int trackHeight: 96
    property int minEmptyHeight: 4
    property string dateStr: ""
    property string dayName: ""
    property string dayNum: ""
    property string durationText: ""
    property bool isToday: false
    property bool metGoal: false
    property int barHeight: 4
    property string appTheme: "dark"

    signal tapped(string date)

    // Top: Duration label (blank line height preserved when empty so all bars stay baseline-aligned)
    Label {
        horizontalAlignment: HorizontalAlignment.Center
        text: trendBarItem.durationText !== "" ? trendBarItem.durationText : " "
        opacity: trendBarItem.durationText !== "" ? 1.0 : 0.0
        textStyle.base: SystemDefaults.TextStyles.SmallText
        textStyle.fontWeight: (trendBarItem.isToday || trendBarItem.durationText !== "") ? FontWeight.Bold : FontWeight.Normal
        textStyle.color: Color.create(bookManager.uiAccent)
        autoSize.maxLineCount: 1
    }

    // Middle: Sleek Open Bar Track (no grey vertical cages on empty days)
    Container {
        topMargin: bookManager.dp(4)
        bottomMargin: bookManager.dp(4)
        horizontalAlignment: HorizontalAlignment.Center
        preferredWidth: bookManager.dp(24)
        preferredHeight: trendBarItem.trackHeight
        background: Color.Transparent
        layout: DockLayout {}

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Bottom
            preferredHeight: trendBarItem.barHeight
            background: trendBarItem.metGoal ?
                        Color.create(bookManager.uiAccent) :
                        ((trendBarItem.durationText !== "") ?
                         (appTheme === "dark" ? Color.create("#C99A4E") : Color.create("#B8893D")) :
                         Color.create(bookManager.uiHairline))
        }
    }

    // Bottom: Day Number
    Label {
        horizontalAlignment: HorizontalAlignment.Center
        text: trendBarItem.dayNum
        textStyle.base: SystemDefaults.TextStyles.SmallText
        textStyle.fontWeight: trendBarItem.isToday ? FontWeight.Bold : FontWeight.Normal
        textStyle.color: trendBarItem.isToday ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiInk)
        autoSize.maxLineCount: 1
    }

    // Bottom: Day of Week Name
    Label {
        horizontalAlignment: HorizontalAlignment.Center
        text: trendBarItem.dayName
        textStyle.base: SystemDefaults.TextStyles.SmallText
        textStyle.fontWeight: trendBarItem.isToday ? FontWeight.Bold : FontWeight.Normal
        textStyle.color: trendBarItem.isToday ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiMuted)
        autoSize.maxLineCount: 1
    }

    gestureHandlers: [
        TapHandler {
            onTapped: {
                if (trendBarItem.dateStr !== "") {
                    trendBarItem.tapped(trendBarItem.dateStr);
                }
            }
        }
    ]
}
