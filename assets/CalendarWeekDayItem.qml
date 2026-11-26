import bb.cascades 1.3

Container {
    id: weekDayItem
    horizontalAlignment: HorizontalAlignment.Fill
    bottomMargin: bookManager.dp(6)

    property real uiScale: 1.0
    property string dateStr: ""
    property bool isSelected: false
    property bool isToday: false
    property bool hasReading: false
    property string appTheme: "dark"

    property string bPath0: ""
    property string bPath1: ""
    property string bPath2: ""
    property string bPath3: ""
    property int extraBooksCount: 0

    signal daySelected(string date)
    signal bookOpened(string path)

    background: isSelected ?
                Color.create(bookManager.uiActive) :
                Color.create(bookManager.uiCard)

    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

    // Left accent indicator on selected day or today
    Container {
        visible: weekDayItem.isSelected || weekDayItem.isToday
        preferredWidth: bookManager.dp(4)
        verticalAlignment: VerticalAlignment.Fill
        background: Color.create(bookManager.uiAccent)
    }

    Container {
        id: innerContainer
        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
        horizontalAlignment: HorizontalAlignment.Fill
        topPadding: weekDayItem.hasReading ? bookManager.dp(11) : bookManager.dp(9)
        bottomPadding: weekDayItem.hasReading ? bookManager.dp(11) : bookManager.dp(9)
        leftPadding: bookManager.dp(14)
        rightPadding: bookManager.dp(14)
        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

        // Single-line Header Row: Day on left, Today badge + duration/dash on right
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            Label {
                id: dayLabel
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                verticalAlignment: VerticalAlignment.Center
                rightMargin: bookManager.dp(8)
                text: ""
                textStyle.base: weekDayItem.hasReading ? SystemDefaults.TextStyles.SubtitleText : SystemDefaults.TextStyles.SmallText
                textStyle.fontWeight: (weekDayItem.hasReading || weekDayItem.isToday) ? FontWeight.Bold : FontWeight.Normal
                textStyle.color: (weekDayItem.hasReading || weekDayItem.isToday) ? Color.create(bookManager.uiInk) : Color.create(bookManager.uiMuted)
                autoSize.maxLineCount: 1
            }

            Container {
                id: todayBadge
                visible: weekDayItem.isToday
                verticalAlignment: VerticalAlignment.Center
                rightMargin: bookManager.dp(8)
                background: Color.create(bookManager.uiAccent)
                topPadding: bookManager.dp(1)
                bottomPadding: bookManager.dp(1)
                leftPadding: bookManager.dp(6)
                rightPadding: bookManager.dp(6)
                Label {
                    text: qsTr("Hôm nay") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: appTheme === "dark" ? Color.create("#0E0C0A") : Color.create("#FFFFFF")
                }
            }

            Label {
                id: durationLabel
                verticalAlignment: VerticalAlignment.Center
                text: "—"
                textStyle.base: weekDayItem.hasReading ? SystemDefaults.TextStyles.SubtitleText : SystemDefaults.TextStyles.SmallText
                textStyle.fontWeight: weekDayItem.hasReading ? FontWeight.Bold : FontWeight.Normal
                textStyle.color: weekDayItem.hasReading ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiMuted)
                autoSize.maxLineCount: 1
            }
        }

        // 1px Hairline Divider
        Container {
            id: bookListDivider
            visible: false
            topMargin: bookManager.dp(8)
            bottomMargin: bookManager.dp(6)
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: 1
            background: Color.create(bookManager.uiHairline)
        }

        // Books read container (up to 4 rows + overflow count indicator)
        Container {
            id: booksContainer
            visible: false
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            Container {
                id: bItem0
                visible: false
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: bookManager.dp(3)
                bottomPadding: bookManager.dp(3)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                gestureHandlers: [
                    TapHandler { onTapped: { if (weekDayItem.bPath0 !== "") weekDayItem.bookOpened(weekDayItem.bPath0); } }
                ]
                Label {
                    id: bTitle0
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    rightMargin: bookManager.dp(8)
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.W500
                    textStyle.color: Color.create(bookManager.uiInk)
                    autoSize.maxLineCount: 1
                }
                Label {
                    id: bDur0
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create(bookManager.uiAccent)
                    autoSize.maxLineCount: 1
                }
            }

            Container {
                id: bItem1
                visible: false
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: bookManager.dp(3)
                bottomPadding: bookManager.dp(3)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                gestureHandlers: [
                    TapHandler { onTapped: { if (weekDayItem.bPath1 !== "") weekDayItem.bookOpened(weekDayItem.bPath1); } }
                ]
                Label {
                    id: bTitle1
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    rightMargin: bookManager.dp(8)
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.W500
                    textStyle.color: Color.create(bookManager.uiInk)
                    autoSize.maxLineCount: 1
                }
                Label {
                    id: bDur1
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create(bookManager.uiAccent)
                    autoSize.maxLineCount: 1
                }
            }

            Container {
                id: bItem2
                visible: false
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: bookManager.dp(3)
                bottomPadding: bookManager.dp(3)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                gestureHandlers: [
                    TapHandler { onTapped: { if (weekDayItem.bPath2 !== "") weekDayItem.bookOpened(weekDayItem.bPath2); } }
                ]
                Label {
                    id: bTitle2
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    rightMargin: bookManager.dp(8)
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.W500
                    textStyle.color: Color.create(bookManager.uiInk)
                    autoSize.maxLineCount: 1
                }
                Label {
                    id: bDur2
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create(bookManager.uiAccent)
                    autoSize.maxLineCount: 1
                }
            }

            Container {
                id: bItem3
                visible: false
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: bookManager.dp(3)
                bottomPadding: bookManager.dp(3)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                gestureHandlers: [
                    TapHandler { onTapped: { if (weekDayItem.bPath3 !== "") weekDayItem.bookOpened(weekDayItem.bPath3); } }
                ]
                Label {
                    id: bTitle3
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    rightMargin: bookManager.dp(8)
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.W500
                    textStyle.color: Color.create(bookManager.uiInk)
                    autoSize.maxLineCount: 1
                }
                Label {
                    id: bDur3
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create(bookManager.uiAccent)
                    autoSize.maxLineCount: 1
                }
            }

            Label {
                visible: weekDayItem.extraBooksCount > 0
                topMargin: bookManager.dp(2)
                text: "+" + weekDayItem.extraBooksCount + " " + qsTr("cuốn khác") + Retranslate.onLanguageChanged
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: Color.create(bookManager.uiMuted)
            }
        }
    }

    function updateDay(dayData, selDate) {
        if (!dayData) return;
        dateStr = dayData.dateStr;
        isSelected = (dateStr === selDate);
        isToday = dayData.isToday;
        hasReading = dayData.hasReading;

        dayLabel.text = dayData.fullDow + ", " + dayData.dayNumber + "/" + dayData.month;
        if (hasReading) {
            durationLabel.text = (dayData.shortDurationStr && dayData.shortDurationStr !== "") ? dayData.shortDurationStr : dayData.durationStr;
        } else {
            durationLabel.text = "—";
        }

        bPath0 = ""; bPath1 = ""; bPath2 = ""; bPath3 = "";
        extraBooksCount = 0;

        var books = dayData.books;
        if (books && books.length > 0) {
            booksContainer.visible = true;
            bookListDivider.visible = true;

            var items = [bItem0, bItem1, bItem2, bItem3];
            var titles = [bTitle0, bTitle1, bTitle2, bTitle3];
            var durs = [bDur0, bDur1, bDur2, bDur3];
            var paths = ["", "", "", ""];

            for (var k = 0; k < 4; ++k) {
                if (k < books.length) {
                    items[k].visible = true;
                    titles[k].text = books[k].title;
                    durs[k].text = books[k].shortDayDurationStr ? books[k].shortDayDurationStr : books[k].dayDurationStr;
                    paths[k] = books[k].filePath ? books[k].filePath : "";
                } else {
                    items[k].visible = false;
                }
            }
            bPath0 = paths[0];
            bPath1 = paths[1];
            bPath2 = paths[2];
            bPath3 = paths[3];
            extraBooksCount = (books.length > 4) ? (books.length - 4) : 0;
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
