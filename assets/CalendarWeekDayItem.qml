import bb.cascades 1.3

Container {
    id: weekDayItem
    horizontalAlignment: HorizontalAlignment.Fill
    bottomMargin: 8

    property string dateStr: ""
    property bool isSelected: false
    property bool isToday: false
    property string appTheme: "dark"

    signal daySelected(string date)

    // Outer border
    topPadding: 1
    bottomPadding: 1
    leftPadding: 1
    rightPadding: 1
    background: isSelected ? Color.create("#10B981") : (appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5"))

    layout: DockLayout {}

    Container {
        id: innerContainer
        horizontalAlignment: HorizontalAlignment.Fill
        background: isSelected ?
                    (appTheme === "dark" ? Color.create("#064E3B") : Color.create("#D1FAE5")) :
                    (appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF"))
        topPadding: 10
        bottomPadding: 10
        leftPadding: 12
        rightPadding: 12

        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

        // Header Row: Day of week + date on left, duration on right
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            // Day label (e.g. Thứ Hai, 10/11)
            Label {
                id: dayLabel
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                verticalAlignment: VerticalAlignment.Center
                text: ""
                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: isSelected ? (appTheme === "dark" ? Color.White : Color.create("#065F46")) : (appTheme === "dark" ? Color.White : Color.create("#000000"))
                autoSize.maxLineCount: 1
            }

            // Today Badge
            Container {
                id: todayBadge
                visible: false
                verticalAlignment: VerticalAlignment.Center
                rightMargin: 8
                background: Color.create("#10B981")
                topPadding: 2
                bottomPadding: 2
                leftPadding: 6
                rightPadding: 6
                Label {
                    text: qsTr("Hôm nay") + Retranslate.onLanguageChanged
                    textStyle.fontSize: FontSize.XXSmall
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.White
                }
            }

            // Day duration
            Label {
                id: durationLabel
                verticalAlignment: VerticalAlignment.Center
                text: ""
                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: Color.create("#10B981")
                autoSize.maxLineCount: 1
            }
        }

        // Divider
        Container {
            id: bookListDivider
            visible: false
            topMargin: 8
            bottomMargin: 8
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: 1
            background: appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
        }

        // Books read container (up to 4 books displayed cleanly)
        Container {
            id: booksContainer
            visible: false
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            // Book item 0
            Container {
                id: bItem0
                visible: false
                horizontalAlignment: HorizontalAlignment.Fill
                bottomMargin: 4
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                Label {
                    id: bTitle0
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.Bold
                    autoSize.maxLineCount: 1
                }
                Label {
                    id: bDur0
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: Color.create("#10B981")
                    autoSize.maxLineCount: 1
                }
            }
            // Book item 1
            Container {
                id: bItem1
                visible: false
                horizontalAlignment: HorizontalAlignment.Fill
                bottomMargin: 4
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                Label {
                    id: bTitle1
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.Bold
                    autoSize.maxLineCount: 1
                }
                Label {
                    id: bDur1
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: Color.create("#10B981")
                    autoSize.maxLineCount: 1
                }
            }
            // Book item 2
            Container {
                id: bItem2
                visible: false
                horizontalAlignment: HorizontalAlignment.Fill
                bottomMargin: 4
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                Label {
                    id: bTitle2
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.Bold
                    autoSize.maxLineCount: 1
                }
                Label {
                    id: bDur2
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: Color.create("#10B981")
                    autoSize.maxLineCount: 1
                }
            }
            // Book item 3
            Container {
                id: bItem3
                visible: false
                horizontalAlignment: HorizontalAlignment.Fill
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                Label {
                    id: bTitle3
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.Bold
                    autoSize.maxLineCount: 1
                }
                Label {
                    id: bDur3
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: Color.create("#10B981")
                    autoSize.maxLineCount: 1
                }
            }
        }

        // Empty state when no books read
        Label {
            id: emptyDayLabel
            topMargin: 4
            text: qsTr("Không có hoạt động đọc sách") + Retranslate.onLanguageChanged
            textStyle.base: SystemDefaults.TextStyles.XSmallText
            textStyle.color: appTheme === "dark" ? Color.create("#71717A") : Color.create("#888888")
            visible: true
        }
    }

    function updateDay(dayData, selDate) {
        if (!dayData) return;
        dateStr = dayData.dateStr;
        isSelected = (dateStr === selDate);
        isToday = dayData.isToday;
        todayBadge.visible = isToday;

        dayLabel.text = dayData.fullDow + ", " + dayData.dayNumber + "/" + dayData.month;
        if (dayData.hasReading) {
            durationLabel.text = dayData.durationStr;
            durationLabel.textStyle.color = Color.create("#10B981");
            emptyDayLabel.visible = false;
        } else {
            durationLabel.text = qsTr("Chưa đọc") + Retranslate.onLanguageChanged;
            durationLabel.textStyle.color = appTheme === "dark" ? Color.create("#71717A") : Color.create("#888888");
            emptyDayLabel.visible = true;
        }

        var books = dayData.books;
        if (books && books.length > 0) {
            booksContainer.visible = true;
            bookListDivider.visible = true;

            var items = [bItem0, bItem1, bItem2, bItem3];
            var titles = [bTitle0, bTitle1, bTitle2, bTitle3];
            var durs = [bDur0, bDur1, bDur2, bDur3];

            for (var k = 0; k < 4; ++k) {
                if (k < books.length) {
                    items[k].visible = true;
                    titles[k].text = books[k].title;
                    titles[k].textStyle.color = isSelected ? Color.White : (appTheme === "dark" ? Color.create("#E5E5E5") : Color.create("#111111"));
                    durs[k].text = books[k].dayDurationStr;
                } else {
                    items[k].visible = false;
                }
            }
        } else {
            booksContainer.visible = false;
            bookListDivider.visible = false;
        }
    }

    gestureHandlers: [
        TapHandler {
            onTapped: {
                if (weekDayItem.dateStr !== "") {
                    weekDayItem.daySelected(weekDayItem.dateStr);
                }
            }
        }
    ]
}
