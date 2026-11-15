import bb.cascades 1.3

Container {
    id: trendBarItem
    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
    horizontalAlignment: HorizontalAlignment.Center
    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

    property string dayName: ""
    property string dayNum: ""
    property string durationText: "-"
    property bool isToday: false
    property int barHeight: 8
    property string appTheme: "dark"

    // Top: Duration label
    Label {
        horizontalAlignment: HorizontalAlignment.Center
        text: trendBarItem.durationText
        textStyle.fontSize: FontSize.XXSmall
        textStyle.fontWeight: trendBarItem.isToday ? FontWeight.Bold : FontWeight.Normal
        textStyle.color: trendBarItem.isToday ? Color.create("#10B981") :
                         (trendBarItem.durationText !== "-" ? Color.create("#10B981") :
                         (trendBarItem.appTheme === "dark" ? Color.create("#71717A") : Color.create("#888888")))
        autoSize.maxLineCount: 1
    }

    // Middle: Bar Slot Track (height: 110)
    Container {
        topMargin: 4
        bottomMargin: 4
        horizontalAlignment: HorizontalAlignment.Center
        preferredWidth: 28
        preferredHeight: 110
        background: trendBarItem.appTheme === "dark" ? Color.create("#111111") : Color.create("#E5E5E5")
        layout: DockLayout {}

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Bottom
            preferredHeight: trendBarItem.barHeight
            background: trendBarItem.isToday ? Color.create("#10B981") :
                        (trendBarItem.barHeight > 8 ? Color.create("#10B981") :
                        (trendBarItem.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")))
        }
    }

    // Bottom: Day Number (e.g. 14)
    Label {
        horizontalAlignment: HorizontalAlignment.Center
        text: trendBarItem.dayNum
        textStyle.fontSize: FontSize.XSmall
        textStyle.fontWeight: trendBarItem.isToday ? FontWeight.Bold : FontWeight.Normal
        textStyle.color: trendBarItem.isToday ? Color.create("#10B981") :
                         (trendBarItem.appTheme === "dark" ? Color.White : Color.create("#000000"))
    }

    // Bottom: Day of Week Name (e.g. T6)
    Label {
        horizontalAlignment: HorizontalAlignment.Center
        text: trendBarItem.dayName
        textStyle.fontSize: FontSize.XXSmall
        textStyle.color: trendBarItem.isToday ? Color.create("#10B981") :
                         (trendBarItem.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777"))
    }
}
