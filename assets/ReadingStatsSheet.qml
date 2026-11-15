import bb.cascades 1.3

Sheet {
    id: statsSheet

    property string mainMode: "calendar" // "calendar", "by_book", "trends", "overview"
    property string selectedBookPath: ""
    property int curYear: 2026
    property int curMonth: 11
    property string selectedDate: ""
    property string viewMode: "month" // "month" or "week"
    property string curWeekAnchorDate: ""

    onOpened: {
        var now = new Date();
        curYear = now.getFullYear();
        curMonth = now.getMonth() + 1; // 1-12
        var d = now.getDate();
        selectedDate = bookManager.formatDateYMD(curYear, curMonth, d);
        curWeekAnchorDate = selectedDate;
        refreshAll();
    }

    onCreationCompleted: {
        bookManager.languageChanged.connect(refreshAll);
    }

    function prevMonth() {
        if (curMonth === 1) {
            curMonth = 12;
            curYear--;
        } else {
            curMonth--;
        }
        refreshCalendarAndMonthStats();
    }

    function nextMonth() {
        if (curMonth === 12) {
            curMonth = 1;
            curYear++;
        } else {
            curMonth++;
        }
        refreshCalendarAndMonthStats();
    }

    function goToday() {
        var now = new Date();
        curYear = now.getFullYear();
        curMonth = now.getMonth() + 1;
        var d = now.getDate();
        selectedDate = bookManager.formatDateYMD(curYear, curMonth, d);
        curWeekAnchorDate = selectedDate;
        refreshAll();
    }

    function prevWeek() {
        var wd = bookManager.getWeekData(curWeekAnchorDate);
        curWeekAnchorDate = wd.prevWeekDate;
        refreshWeekView();
    }

    function nextWeek() {
        var wd = bookManager.getWeekData(curWeekAnchorDate);
        curWeekAnchorDate = wd.nextWeekDate;
        refreshWeekView();
    }

    function goCurrentWeek() {
        var now = new Date();
        curWeekAnchorDate = bookManager.formatDateYMD(now.getFullYear(), now.getMonth() + 1, now.getDate());
        refreshWeekView();
    }

    function selectDay(dateStr) {
        if (!dateStr || dateStr === "") return;
        selectedDate = dateStr;
        curWeekAnchorDate = dateStr;
        updateCalendarSelection();
        refreshWeekSelection();
        refreshSelectedDayDetails();
    }

    function refreshAll() {
        refreshCalendarAndMonthStats();
        refreshWeekView();
        refreshSelectedDayDetails();
        refreshOverallStats();
        refreshByBookStats();
        refreshTrendsStats();
    }

    function refreshWeekSelection() {
        var weekDayContainers = [
            weekDay0, weekDay1, weekDay2, weekDay3, weekDay4, weekDay5, weekDay6
        ];
        for (var i = 0; i < 7; ++i) {
            weekDayContainers[i].isSelected = (weekDayContainers[i].dateStr === selectedDate);
        }
    }

    function refreshWeekView() {
        if (!curWeekAnchorDate || curWeekAnchorDate === "") {
            var now = new Date();
            curWeekAnchorDate = bookManager.formatDateYMD(now.getFullYear(), now.getMonth() + 1, now.getDate());
        }
        var wd = bookManager.getWeekData(curWeekAnchorDate);
        weekTitleLabel.text = wd.weekTitle;
        weekTotalTimeLabel.text = wd.totalDurationStr;
        weekReadingDaysLabel.text = wd.readingDays + " / 7 " + (qsTr("ngày") + Retranslate.onLanguageChanged);
        weekAvgDailyTimeLabel.text = wd.avgDailyTimeStr;

        var days = wd.days;
        var weekDayContainers = [
            weekDay0, weekDay1, weekDay2, weekDay3, weekDay4, weekDay5, weekDay6
        ];
        for (var i = 0; i < 7; ++i) {
            if (i < days.length) {
                weekDayContainers[i].appTheme = bookManager.appTheme;
                weekDayContainers[i].updateDay(days[i], selectedDate);
            }
        }
    }

    function getCellsList() {
        return [
            c0, c1, c2, c3, c4, c5, c6,
            c7, c8, c9, c10, c11, c12, c13,
            c14, c15, c16, c17, c18, c19, c20,
            c21, c22, c23, c24, c25, c26, c27,
            c28, c29, c30, c31, c32, c33, c34,
            c35, c36, c37, c38, c39, c40, c41
        ];
    }

    function updateCalendarSelection() {
        var cells = getCellsList();
        for (var i = 0; i < 42; ++i) {
            cells[i].isSelected = (cells[i].dateStr === selectedDate);
        }
    }

    function refreshCalendarAndMonthStats() {
        // 1. Month stats
        var ms = bookManager.getMonthStats(curYear, curMonth);
        monthTitleLabel.text = ms.monthName;
        monthTotalTimeLabel.text = ms.totalTimeStr;
        monthReadingDaysValueLabel.text = ms.readingDays + " / " + ms.daysInMonth;
        monthReadingDaysRateLabel.text = ms.completionRate + "% " + (qsTr("tỷ lệ hoàn thành") + Retranslate.onLanguageChanged);
        monthAvgTimeLabel.text = ms.avgDailyTimeStr;
        monthMaxStreakValueLabel.text = ms.maxStreak + " " + qsTr("ngày");

        // 2. Calendar grid
        var days = bookManager.getCalendarDays(curYear, curMonth);
        var cells = getCellsList();
        for (var i = 0; i < 42; ++i) {
            var d = days[i];
            cells[i].dateStr = d.dateStr;
            cells[i].dayNumber = d.day;
            cells[i].isCurrentMonth = d.isCurrentMonth;
            cells[i].isToday = d.isToday;
            cells[i].readingSeconds = d.seconds;
            cells[i].durationText = d.durationStr;
            cells[i].intensityLevel = d.intensityLevel;
            cells[i].isSelected = (d.dateStr === selectedDate);
            cells[i].appTheme = bookManager.appTheme;
        }
    }

    function refreshSelectedDayDetails() {
        if (!selectedDate || selectedDate === "") {
            var now = new Date();
            selectedDate = bookManager.formatDateYMD(now.getFullYear(), now.getMonth() + 1, now.getDate());
        }

        var daySecs = bookManager.getDailyReadingSeconds(selectedDate);
        var durStr = bookManager.getDailyReadingTimeStr(selectedDate);

        var dateParts = selectedDate.split("-");
        var y = dateParts[0];
        var m = dateParts[1];
        var d = dateParts[2];
        selectedDayTitleLabel.text = qsTr("Ngày") + " " + d + "/" + m + "/" + y;

        if (daySecs > 0) {
            selectedDayTotalLabel.text = qsTr("Tổng thời gian đọc: ") + durStr;
            selectedDayTotalLabel.textStyle.color = Color.create("#10B981");
            noDayReadingLabel.visible = false;
        } else {
            selectedDayTotalLabel.text = qsTr("Chưa có thời gian đọc");
            selectedDayTotalLabel.textStyle.color = bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777");
            noDayReadingLabel.visible = true;
        }

        var books = bookManager.getDayBooks(selectedDate);
        var cards = [bookCard0, bookCard1, bookCard2, bookCard3, bookCard4];

        for (var i = 0; i < 5; ++i) {
            if (i < books.length) {
                var b = books[i];
                cards[i].visible = true;
                cards[i].filePath = b.filePath;
                cards[i].titleText = b.title;
                cards[i].authorText = b.author && b.author !== "" ? b.author : qsTr("Chưa rõ tác giả");
                cards[i].coverSource = b.cover && b.cover !== "" ? b.cover : "asset:///icons/default_cover.png";
                cards[i].formatText = b.formatBadge ? b.formatBadge : (b.format ? b.format.toUpperCase() : "EPUB");
                cards[i].durationText = b.dayDurationStr;
                cards[i].pageCountText = b.pageCountStr ? b.pageCountStr : (b.totalPages > 0 ? (b.totalPages + " " + qsTr("trang")) : "");
                cards[i].progressVal = b.progress ? b.progress : 0;
                cards[i].progressText = (b.progress ? b.progress : 0) + "%";
            } else {
                cards[i].visible = false;
            }
        }
    }

    function refreshOverallStats() {
        var os = bookManager.getOverallStats();
        allTimeTotalLabel.text = os.totalReadingTimeStr;
        allTimeDaysLabel.text = os.totalReadingDays + " " + qsTr("ngày");
        allTimeCurrentStreakLabel.text = os.currentStreak + " " + qsTr("ngày liên tiếp");
        allTimeMaxStreakLabel.text = os.allTimeStreak + " " + qsTr("ngày");
        if (os.bestDaySeconds > 0) {
            var bp = os.bestDayDate.split("-");
            var dateFmt = bp.length === 3 ? (bp[2] + "/" + bp[1] + "/" + bp[0]) : os.bestDayDate;
            allTimeBestDayLabel.text = os.bestDayTimeStr + " (" + dateFmt + ")";
        } else {
            allTimeBestDayLabel.text = qsTr("Chưa có");
        }
        allTimeBooksLabel.text = os.completedBooks + " " + qsTr("đã xong") + " • " + os.readingBooks + " " + qsTr("đang đọc");
    }

    function refreshByBookStats() {
        if (!selectedBookPath || selectedBookPath === "") {
            if (bookManager.lastBookPath && bookManager.lastBookPath !== "") {
                selectedBookPath = bookManager.lastBookPath;
            } else if (bookManager.currentFilePath && bookManager.currentFilePath !== "") {
                selectedBookPath = bookManager.currentFilePath;
            }
        }

        var allList = bookManager.getBookStatsList();
        if ((!selectedBookPath || selectedBookPath === "") && allList.length > 0) {
            selectedBookPath = allList[0].filePath;
        }

        if (selectedBookPath && selectedBookPath !== "") {
            var bs = bookManager.getBookDetailedStats(selectedBookPath);
            byBookHeroContainer.visible = true;
            byBookNoDataContainer.visible = false;

            byBookCover.imageSource = (bs.cover && bs.cover !== "") ? bs.cover : "asset:///icons/default_cover.png";
            byBookTitle.text = bs.title ? bs.title : "";
            byBookAuthor.text = bs.author ? bs.author : "";
            byBookFormatBadge.text = bs.formatBadge ? bs.formatBadge : "EPUB";
            byBookProgressText.text = bs.progressStr ? bs.progressStr : "0%";
            byBookTimeSpent.text = bs.totalTimeStr ? bs.totalTimeStr : "--";
            byBookRemaining.text = bs.remainingTimeStr ? bs.remainingTimeStr : "--";
            byBookSpeed.text = bs.readingSpeedStr ? bs.readingSpeedStr : "--";
            byBookPages.text = bs.pageStr ? bs.pageStr : "--";

            var prog = bs.progress ? bs.progress : 0;
            bookManager.setControlSpaceQuota(byBookProgressBar, Math.max(1, prog));
            bookManager.setControlSpaceQuota(byBookProgressEmpty, Math.max(1, 100 - prog));
        } else {
            byBookHeroContainer.visible = false;
            byBookNoDataContainer.visible = true;
        }

        var slots = [
            byBookSlot0, byBookSlot1, byBookSlot2, byBookSlot3, byBookSlot4, byBookSlot5
        ];
        for (var i = 0; i < 6; ++i) {
            if (i < allList.length) {
                var b = allList[i];
                slots[i].visible = true;
                slots[i].filePath = b.filePath;
                slots[i].titleText = b.title ? b.title : "";
                slots[i].authorText = b.author ? b.author : "";
                slots[i].coverSource = (b.cover && b.cover !== "") ? b.cover : "asset:///icons/default_cover.png";
                slots[i].formatText = b.formatBadge ? b.formatBadge : "EPUB";
                slots[i].durationText = b.totalTimeStr ? b.totalTimeStr : "";
                slots[i].pageCountText = b.pageStr ? b.pageStr : "";
                slots[i].progressText = b.progressStr ? b.progressStr : "";
                slots[i].progressVal = b.progress ? b.progress : 0;
                slots[i].isSelected = (b.filePath === selectedBookPath);
            } else {
                slots[i].visible = false;
            }
        }
    }

    function refreshTrendsStats() {
        var trendDays = bookManager.getRecentDaysTrend(7);
        var trendCols = [trendCol0, trendCol1, trendCol2, trendCol3, trendCol4, trendCol5, trendCol6];
        var weekTotalSecs = 0;
        var activeDays = 0;
        var maxDaySecs = 0;
        var maxDayName = "";

        for (var i = 0; i < 7; ++i) {
            if (i < trendDays.length) {
                var td = trendDays[i];
                trendCols[i].visible = true;
                trendCols[i].dayName = td.dayName;
                trendCols[i].dayNum = td.dayNum;
                trendCols[i].durationText = (td.seconds > 0) ? (td.shortDurationStr ? td.shortDurationStr : td.durationStr) : "-";
                trendCols[i].isToday = td.isToday;
                var h = Math.max(8, Math.round(td.heightRatio * 110));
                trendCols[i].barHeight = h;

                weekTotalSecs += td.seconds;
                if (td.seconds > 0) {
                    activeDays++;
                    if (td.seconds > maxDaySecs) {
                        maxDaySecs = td.seconds;
                        maxDayName = td.dayName + " (" + td.durationStr + ")";
                    }
                }
            } else {
                trendCols[i].visible = false;
            }
        }

        trendTotalWeekLabel.text = bookManager.formatDuration(weekTotalSecs);
        var avgSecs = activeDays > 0 ? Math.round(weekTotalSecs / activeDays) : 0;
        trendAvgDailyLabel.text = bookManager.formatDuration(avgSecs);
        trendBestDayLabel.text = (maxDaySecs > 0) ? maxDayName : qsTr("Chưa có");

        // Format distribution
        var dist = bookManager.getFormatDistribution();
        epubTimeLabel.text = dist.epubTimeStr + " (" + dist.epubBooks + " " + qsTr("cuốn") + " • " + dist.epubPct + "%)";
        bookManager.setControlSpaceQuota(epubBarFill, Math.max(1, dist.epubPct));
        bookManager.setControlSpaceQuota(epubBarEmpty, Math.max(1, 100 - dist.epubPct));

        pdfTimeLabel.text = dist.pdfTimeStr + " (" + dist.pdfBooks + " " + qsTr("cuốn") + " • " + dist.pdfPct + "%)";
        bookManager.setControlSpaceQuota(pdfBarFill, Math.max(1, dist.pdfPct));
        bookManager.setControlSpaceQuota(pdfBarEmpty, Math.max(1, 100 - dist.pdfPct));

        txtTimeLabel.text = dist.txtTimeStr + " (" + dist.txtBooks + " " + qsTr("cuốn") + " • " + dist.txtPct + "%)";
        bookManager.setControlSpaceQuota(txtBarFill, Math.max(1, dist.txtPct));
        bookManager.setControlSpaceQuota(txtBarEmpty, Math.max(1, 100 - dist.txtPct));

        fb2TimeLabel.text = dist.fb2TimeStr + " (" + dist.fb2Books + " " + qsTr("cuốn") + " • " + dist.fb2Pct + "%)";
        bookManager.setControlSpaceQuota(fb2BarFill, Math.max(1, dist.fb2Pct));
        bookManager.setControlSpaceQuota(fb2BarEmpty, Math.max(1, 100 - dist.fb2Pct));
    }

    Page {
        titleBar: TitleBar {
            title: qsTr("Thống Kê Đọc Sách") + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain
            dismissAction: ActionItem {
                title: qsTr("Đóng") + Retranslate.onLanguageChanged
                onTriggered: {
                    statsSheet.close();
                }
            }
            acceptAction: ActionItem {
                title: qsTr("Hôm nay") + Retranslate.onLanguageChanged
                onTriggered: {
                    statsSheet.goToday();
                }
            }
        }

        ScrollView {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            scrollViewProperties.scrollMode: ScrollMode.Vertical

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                topPadding: 14
                bottomPadding: 28
                leftPadding: 14
                rightPadding: 14

                layout: StackLayout {
                    orientation: LayoutOrientation.TopToBottom
                }

                // =============================================================
                // MAIN READING STATS VIEW SELECTOR (KOReader Style)
                // [ Lịch đọc ] [ Theo sách ] [ Xu hướng ] [ Tổng quan ]
                // =============================================================
                SegmentedControl {
                    id: statsModeControl
                    horizontalAlignment: HorizontalAlignment.Fill
                    bottomMargin: 14

                    Option {
                        id: optCalendar
                        text: qsTr("Lịch đọc") + Retranslate.onLanguageChanged
                        value: "calendar"
                        selected: true
                    }
                    Option {
                        id: optByBook
                        text: qsTr("Theo sách") + Retranslate.onLanguageChanged
                        value: "by_book"
                    }
                    Option {
                        id: optTrends
                        text: qsTr("Xu hướng") + Retranslate.onLanguageChanged
                        value: "trends"
                    }
                    Option {
                        id: optOverview
                        text: qsTr("Tổng quan") + Retranslate.onLanguageChanged
                        value: "overview"
                    }

                    onSelectedValueChanged: {
                        statsSheet.mainMode = selectedValue;
                        if (selectedValue === "by_book") {
                            statsSheet.refreshByBookStats();
                        } else if (selectedValue === "trends") {
                            statsSheet.refreshTrendsStats();
                        } else if (selectedValue === "overview") {
                            statsSheet.refreshOverallStats();
                        } else if (selectedValue === "calendar") {
                            if (statsSheet.viewMode === "month") {
                                statsSheet.refreshCalendarAndMonthStats();
                            } else {
                                statsSheet.refreshWeekView();
                            }
                        }
                    }
                }

                // =============================================================
                // MODE 1: LỊCH ĐỌC (CALENDAR VIEW)
                // =============================================================
                Container {
                    id: calendarModeContainer
                    visible: statsSheet.mainMode === "calendar"
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    // VIEW MODE SWITCHER: LỊCH THÁNG | LỊCH TUẦN
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        bottomMargin: 12
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        Button {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: qsTr("Tháng") + Retranslate.onLanguageChanged
                            appearance: statsSheet.viewMode === "month" ? ButtonAppearance.Primary : ButtonAppearance.Default
                            onClicked: {
                                statsSheet.viewMode = "month";
                                statsSheet.refreshCalendarAndMonthStats();
                            }
                        }

                        Button {
                            leftMargin: 8
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: qsTr("Tuần") + Retranslate.onLanguageChanged
                            appearance: statsSheet.viewMode === "week" ? ButtonAppearance.Primary : ButtonAppearance.Default
                            onClicked: {
                                statsSheet.viewMode = "week";
                                if (!statsSheet.curWeekAnchorDate || statsSheet.curWeekAnchorDate === "") {
                                    statsSheet.curWeekAnchorDate = statsSheet.selectedDate;
                                }
                                statsSheet.refreshWeekView();
                            }
                        }
                    }

                // =============================================================
                // 1. MONTH NAVIGATOR BAR
                // =============================================================
                Container {
                    visible: statsSheet.viewMode === "month"
                    horizontalAlignment: HorizontalAlignment.Fill
                    bottomMargin: 14
                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                    topPadding: 1
                    bottomPadding: 1
                    leftPadding: 1
                    rightPadding: 1
                    layout: DockLayout {}

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                        topPadding: 10
                        bottomPadding: 10
                        leftPadding: 12
                        rightPadding: 12

                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }

                        // Left: "Hôm nay" button (balanced width 96px)
                        Button {
                            preferredWidth: 96
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Hôm nay") + Retranslate.onLanguageChanged
                            onClicked: {
                                statsSheet.goToday();
                            }
                        }

                        // Center: Month & Year Title (dead center)
                        Label {
                            id: monthTitleLabel
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            verticalAlignment: VerticalAlignment.Center
                            horizontalAlignment: HorizontalAlignment.Center
                            text: ""
                            textStyle.base: SystemDefaults.TextStyles.TitleText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.textAlign: TextAlign.Center
                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                            autoSize.maxLineCount: 1
                        }

                        // Right: Month Navigation Stepper [ ‹ ] [ › ] (46px + 4px + 46px = 96px)
                        Button {
                            preferredWidth: 46
                            verticalAlignment: VerticalAlignment.Center
                            text: "‹"
                            onClicked: {
                                statsSheet.prevMonth();
                            }
                        }

                        Button {
                            leftMargin: 4
                            preferredWidth: 46
                            verticalAlignment: VerticalAlignment.Center
                            text: "›"
                            onClicked: {
                                statsSheet.nextMonth();
                            }
                        }
                    }
                }

                // =============================================================
                // 2. MONTHLY SUMMARY METRICS (2x2 Grid with scientific layout)
                // =============================================================
                Container {
                    visible: statsSheet.viewMode === "month"
                    horizontalAlignment: HorizontalAlignment.Fill
                    bottomMargin: 14
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    // Row 1
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        bottomMargin: 8
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        // Metric 1: Tổng thời gian
                        Container {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                            topPadding: 1
                            bottomPadding: 1
                            leftPadding: 1
                            rightPadding: 1
                            layout: DockLayout {}

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                                topPadding: 10
                                bottomPadding: 10
                                leftPadding: 12
                                rightPadding: 12

                                Label {
                                    text: qsTr("TỔNG THỜI GIAN THÁNG") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    id: monthTotalTimeLabel
                                    topMargin: 2
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    topMargin: 1
                                    text: qsTr("thời gian đọc trong tháng") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    autoSize.maxLineCount: 1
                                }
                            }
                        }

                        Container { preferredWidth: 8 }

                        // Metric 2: Số ngày đọc
                        Container {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                            topPadding: 1
                            bottomPadding: 1
                            leftPadding: 1
                            rightPadding: 1
                            layout: DockLayout {}

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                                topPadding: 10
                                bottomPadding: 10
                                leftPadding: 12
                                rightPadding: 12

                                Label {
                                    text: qsTr("SỐ NGÀY ĐỌC") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    id: monthReadingDaysValueLabel
                                    topMargin: 2
                                    text: "-- / --"
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    id: monthReadingDaysRateLabel
                                    topMargin: 1
                                    text: "--% " + qsTr("tỷ lệ hoàn thành") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.color: Color.create("#10B981")
                                    autoSize.maxLineCount: 1
                                }
                            }
                        }
                    }

                    // Row 2
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        // Metric 3: Trung bình / ngày
                        Container {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                            topPadding: 1
                            bottomPadding: 1
                            leftPadding: 1
                            rightPadding: 1
                            layout: DockLayout {}

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                                topPadding: 10
                                bottomPadding: 10
                                leftPadding: 12
                                rightPadding: 12

                                Label {
                                    text: qsTr("TRUNG BÌNH / NGÀY") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    id: monthAvgTimeLabel
                                    topMargin: 2
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    topMargin: 1
                                    text: qsTr("mỗi ngày có đọc") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    autoSize.maxLineCount: 1
                                }
                            }
                        }

                        Container { preferredWidth: 8 }

                        // Metric 4: Chuỗi liên tục
                        Container {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                            topPadding: 1
                            bottomPadding: 1
                            leftPadding: 1
                            rightPadding: 1
                            layout: DockLayout {}

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                                topPadding: 10
                                bottomPadding: 10
                                leftPadding: 12
                                rightPadding: 12

                                Label {
                                    text: qsTr("CHUỖI DÀI NHẤT") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#F59E0B")
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    id: monthMaxStreakValueLabel
                                    topMargin: 2
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#F59E0B")
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    topMargin: 1
                                    text: qsTr("ngày đọc liên tục") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    autoSize.maxLineCount: 1
                                }
                            }
                        }
                    }
                }

                // =============================================================
                // 3. 7-COLUMN MONTHLY CALENDAR CARD
                // =============================================================
                Header {
                    visible: statsSheet.viewMode === "month"
                    title: qsTr("LỊCH THÁNG") + Retranslate.onLanguageChanged
                }

                Container {
                    visible: statsSheet.viewMode === "month"
                    topMargin: 6
                    bottomMargin: 14
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                    topPadding: 1
                    bottomPadding: 1
                    leftPadding: 1
                    rightPadding: 1
                    layout: DockLayout {}

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                        topPadding: 10
                        bottomPadding: 12
                        leftPadding: 8
                        rightPadding: 8

                        layout: StackLayout {
                            orientation: LayoutOrientation.TopToBottom
                        }

                        // Weekday Header Row
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        bottomMargin: 4
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        Label {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: qsTr("T2") + Retranslate.onLanguageChanged
                            textStyle.textAlign: TextAlign.Center
                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                        }
                        Label {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: qsTr("T3") + Retranslate.onLanguageChanged
                            textStyle.textAlign: TextAlign.Center
                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                        }
                        Label {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: qsTr("T4") + Retranslate.onLanguageChanged
                            textStyle.textAlign: TextAlign.Center
                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                        }
                        Label {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: qsTr("T5") + Retranslate.onLanguageChanged
                            textStyle.textAlign: TextAlign.Center
                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                        }
                        Label {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: qsTr("T6") + Retranslate.onLanguageChanged
                            textStyle.textAlign: TextAlign.Center
                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                        }
                        Label {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: qsTr("T7") + Retranslate.onLanguageChanged
                            textStyle.textAlign: TextAlign.Center
                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                        }
                        Label {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            text: qsTr("CN") + Retranslate.onLanguageChanged
                            textStyle.textAlign: TextAlign.Center
                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create("#FB7185")
                        }
                    }

                    // Week Row 0
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 108
                        minHeight: 100
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        CalendarDayCell { id: c0; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c1; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c2; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c3; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c4; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c5; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c6; onTapped: statsSheet.selectDay(date) }
                    }

                    // Week Row 1
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 108
                        minHeight: 100
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        CalendarDayCell { id: c7; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c8; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c9; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c10; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c11; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c12; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c13; onTapped: statsSheet.selectDay(date) }
                    }

                    // Week Row 2
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 108
                        minHeight: 100
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        CalendarDayCell { id: c14; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c15; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c16; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c17; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c18; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c19; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c20; onTapped: statsSheet.selectDay(date) }
                    }

                    // Week Row 3
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 108
                        minHeight: 100
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        CalendarDayCell { id: c21; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c22; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c23; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c24; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c25; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c26; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c27; onTapped: statsSheet.selectDay(date) }
                    }

                    // Week Row 4
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 108
                        minHeight: 100
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        CalendarDayCell { id: c28; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c29; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c30; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c31; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c32; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c33; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c34; onTapped: statsSheet.selectDay(date) }
                    }

                    // Week Row 5
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: 108
                        minHeight: 100
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        CalendarDayCell { id: c35; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c36; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c37; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c38; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c39; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c40; onTapped: statsSheet.selectDay(date) }
                        CalendarDayCell { id: c41; onTapped: statsSheet.selectDay(date) }
                    }

                    // Heatmap Legend Bar
                    Container {
                        topMargin: 10
                        horizontalAlignment: HorizontalAlignment.Center
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        Label {
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Ít hơn") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                        }

                        Container {
                            leftMargin: 6
                            verticalAlignment: VerticalAlignment.Center
                            preferredWidth: 12
                            preferredHeight: 12
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#E5E5E5")
                        }
                        Container {
                            leftMargin: 4
                            verticalAlignment: VerticalAlignment.Center
                            preferredWidth: 12
                            preferredHeight: 12
                            background: bookManager.appTheme === "dark" ? Color.create("#0C2419") : Color.create("#ECFDF5")
                        }
                        Container {
                            leftMargin: 4
                            verticalAlignment: VerticalAlignment.Center
                            preferredWidth: 12
                            preferredHeight: 12
                            background: bookManager.appTheme === "dark" ? Color.create("#0F3122") : Color.create("#D1FAE5")
                        }
                        Container {
                            leftMargin: 4
                            verticalAlignment: VerticalAlignment.Center
                            preferredWidth: 12
                            preferredHeight: 12
                            background: bookManager.appTheme === "dark" ? Color.create("#133F2C") : Color.create("#BBF7D0")
                        }
                        Container {
                            leftMargin: 4
                            verticalAlignment: VerticalAlignment.Center
                            preferredWidth: 12
                            preferredHeight: 12
                            background: bookManager.appTheme === "dark" ? Color.create("#164E36") : Color.create("#A7F3D0")
                        }

                        Label {
                            leftMargin: 6
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Nhiều hơn") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                        }
                    }
                    }
                }

                // =============================================================
                // 3B. WEEK VIEW (CHIA THEO TUẦN)
                // =============================================================
                // Week Navigator Bar
                Container {
                    visible: statsSheet.viewMode === "week"
                    horizontalAlignment: HorizontalAlignment.Fill
                    bottomMargin: 14
                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                    topPadding: 1
                    bottomPadding: 1
                    leftPadding: 1
                    rightPadding: 1
                    layout: DockLayout {}

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                        topPadding: 10
                        bottomPadding: 10
                        leftPadding: 12
                        rightPadding: 12

                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }

                        // Left: "Tuần này" button (balanced width 96px)
                        Button {
                            preferredWidth: 96
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Tuần này") + Retranslate.onLanguageChanged
                            onClicked: {
                                statsSheet.goCurrentWeek();
                            }
                        }

                        // Center: Week Title (dead center)
                        Label {
                            id: weekTitleLabel
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            verticalAlignment: VerticalAlignment.Center
                            horizontalAlignment: HorizontalAlignment.Center
                            text: ""
                            textStyle.base: SystemDefaults.TextStyles.TitleText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.textAlign: TextAlign.Center
                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                            autoSize.maxLineCount: 1
                        }

                        // Right: Week Navigation Stepper [ ‹ ] [ › ] (46px + 4px + 46px = 96px)
                        Button {
                            preferredWidth: 46
                            verticalAlignment: VerticalAlignment.Center
                            text: "‹"
                            onClicked: {
                                statsSheet.prevWeek();
                            }
                        }

                        Button {
                            leftMargin: 4
                            preferredWidth: 46
                            verticalAlignment: VerticalAlignment.Center
                            text: "›"
                            onClicked: {
                                statsSheet.nextWeek();
                            }
                        }
                    }
                }

                // Week Summary Metrics (3 columns)
                Container {
                    visible: statsSheet.viewMode === "week"
                    horizontalAlignment: HorizontalAlignment.Fill
                    bottomMargin: 14
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    // Metric 1: Tổng thời gian tuần
                    Container {
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        topPadding: 1
                        bottomPadding: 1
                        leftPadding: 1
                        rightPadding: 1
                        layout: DockLayout {}

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                            topPadding: 10
                            bottomPadding: 10
                            leftPadding: 10
                            rightPadding: 10

                            Label {
                                text: qsTr("TỔNG THỜI GIAN TUẦN") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.XSmallText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create("#10B981")
                                autoSize.maxLineCount: 1
                            }
                            Label {
                                id: weekTotalTimeLabel
                                topMargin: 2
                                text: "--"
                                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create("#10B981")
                                autoSize.maxLineCount: 1
                            }
                        }
                    }

                    Container { preferredWidth: 8 }

                    // Metric 2: Số ngày đọc
                    Container {
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        topPadding: 1
                        bottomPadding: 1
                        leftPadding: 1
                        rightPadding: 1
                        layout: DockLayout {}

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                            topPadding: 10
                            bottomPadding: 10
                            leftPadding: 10
                            rightPadding: 10

                            Label {
                                text: qsTr("SỐ NGÀY ĐỌC") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.XSmallText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create("#10B981")
                                autoSize.maxLineCount: 1
                            }
                            Label {
                                id: weekReadingDaysLabel
                                topMargin: 2
                                text: "--"
                                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create("#10B981")
                                autoSize.maxLineCount: 1
                            }
                        }
                    }

                    Container { preferredWidth: 8 }

                    // Metric 3: TB mỗi ngày có đọc
                    Container {
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        topPadding: 1
                        bottomPadding: 1
                        leftPadding: 1
                        rightPadding: 1
                        layout: DockLayout {}

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                            topPadding: 10
                            bottomPadding: 10
                            leftPadding: 10
                            rightPadding: 10

                            Label {
                                text: qsTr("TRUNG BÌNH / NGÀY") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.XSmallText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create("#F59E0B")
                                autoSize.maxLineCount: 1
                            }
                            Label {
                                id: weekAvgDailyTimeLabel
                                topMargin: 2
                                text: "--"
                                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create("#F59E0B")
                                autoSize.maxLineCount: 1
                            }
                        }
                    }
                }

                // Header for Week Days List
                Header {
                    visible: statsSheet.viewMode === "week"
                    title: qsTr("HOẠT ĐỘNG TUẦN") + Retranslate.onLanguageChanged
                }

                // 7 Day Items for Week
                Container {
                    visible: statsSheet.viewMode === "week"
                    horizontalAlignment: HorizontalAlignment.Fill
                    bottomMargin: 14
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    CalendarWeekDayItem {
                        id: weekDay0
                        onDaySelected: statsSheet.selectDay(date)
                    }
                    CalendarWeekDayItem {
                        id: weekDay1
                        onDaySelected: statsSheet.selectDay(date)
                    }
                    CalendarWeekDayItem {
                        id: weekDay2
                        onDaySelected: statsSheet.selectDay(date)
                    }
                    CalendarWeekDayItem {
                        id: weekDay3
                        onDaySelected: statsSheet.selectDay(date)
                    }
                    CalendarWeekDayItem {
                        id: weekDay4
                        onDaySelected: statsSheet.selectDay(date)
                    }
                    CalendarWeekDayItem {
                        id: weekDay5
                        onDaySelected: statsSheet.selectDay(date)
                    }
                    CalendarWeekDayItem {
                        id: weekDay6
                        onDaySelected: statsSheet.selectDay(date)
                    }
                }

                // =============================================================
                // 4. SELECTED DAY DETAIL PANEL (Ghi "Ngày DD/MM/YYYY")
                // =============================================================

                Container {
                    topMargin: 6
                    bottomMargin: 14
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                    topPadding: 1
                    bottomPadding: 1
                    leftPadding: 1
                    rightPadding: 1
                    layout: DockLayout {}

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                        topPadding: 14
                        bottomPadding: 14
                        leftPadding: 14
                        rightPadding: 14

                        layout: StackLayout {
                            orientation: LayoutOrientation.TopToBottom
                        }

                    // Selected Day Header
                    Label {
                        id: selectedDayTitleLabel
                        text: ""
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                    }

                    Label {
                        id: selectedDayTotalLabel
                        topMargin: 2
                        text: qsTr("Chưa có thời gian đọc") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create("#10B981")
                    }

                    // Empty state label
                    Label {
                        id: noDayReadingLabel
                        visible: true
                        topMargin: 6
                        text: qsTr("Không có hoạt động đọc sách trong ngày này.") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                    }

                    // Book list slot 0
                    Container {
                        id: bookCard0
                        visible: false
                        topMargin: 10
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 10
                        bottomPadding: 10
                        leftPadding: 12
                        rightPadding: 12
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        property string filePath: ""
                        property alias titleText: book0Title.text
                        property alias authorText: book0Author.text
                        property alias coverSource: book0Cover.imageSource
                        property alias formatText: book0Format.text
                        property alias durationText: book0Duration.text
                        property alias pageCountText: book0PageCount.text
                        property alias progressText: book0ProgressPct.text
                        property int progressVal: 0

                        Container {
                            preferredWidth: 48
                            preferredHeight: 66
                            minWidth: 48
                            minHeight: 66
                            verticalAlignment: VerticalAlignment.Center
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                            layout: DockLayout {}

                            ImageView {
                                id: book0Cover
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                scalingMethod: ScalingMethod.AspectFill
                                imageSource: "asset:///icons/default_cover.png"
                            }
                        }

                        Container {
                            leftMargin: 12
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Center

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    id: book0Title
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
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
                                        id: book0Format
                                        text: "EPUB"
                                        textStyle.fontSize: FontSize.XXSmall
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.White
                                    }
                                }
                            }

                            Container {
                                topMargin: 2
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    id: book0Author
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    autoSize.maxLineCount: 1
                                }

                                Label {
                                    id: book0Duration
                                    leftMargin: 6
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                            }

                            Container {
                                topMargin: 6
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    verticalAlignment: VerticalAlignment.Center
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    preferredHeight: 6
                                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    Container {
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.create("#10B981")
                                        layoutProperties: StackLayoutProperties { spaceQuota: Math.max(1, bookCard0.progressVal) }
                                    }
                                    Container {
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.Transparent
                                        layoutProperties: StackLayoutProperties { spaceQuota: Math.max(1, 100 - bookCard0.progressVal) }
                                    }
                                }

                                Label {
                                    id: book0PageCount
                                    leftMargin: 8
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.XXSmall
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                }

                                Label {
                                    id: book0ProgressPct
                                    leftMargin: 4
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.XXSmall
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                            }
                        }

                        Button {
                            leftMargin: 8
                            preferredWidth: 60
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Đọc") + Retranslate.onLanguageChanged
                            onClicked: {
                                statsSheet.close();
                                app.openBook(bookCard0.filePath);
                            }
                        }
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    statsSheet.close();
                                    app.openBook(bookCard0.filePath);
                                }
                            }
                        ]
                    }

                    // Book list slot 1
                    Container {
                        id: bookCard1
                        visible: false
                        topMargin: 8
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 10
                        bottomPadding: 10
                        leftPadding: 12
                        rightPadding: 12
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        property string filePath: ""
                        property alias titleText: book1Title.text
                        property alias authorText: book1Author.text
                        property alias coverSource: book1Cover.imageSource
                        property alias formatText: book1Format.text
                        property alias durationText: book1Duration.text
                        property alias pageCountText: book1PageCount.text
                        property alias progressText: book1ProgressPct.text
                        property int progressVal: 0

                        Container {
                            preferredWidth: 48
                            preferredHeight: 66
                            minWidth: 48
                            minHeight: 66
                            verticalAlignment: VerticalAlignment.Center
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                            layout: DockLayout {}

                            ImageView {
                                id: book1Cover
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                scalingMethod: ScalingMethod.AspectFill
                                imageSource: "asset:///icons/default_cover.png"
                            }
                        }

                        Container {
                            leftMargin: 12
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Center

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    id: book1Title
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
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
                                        id: book1Format
                                        text: "EPUB"
                                        textStyle.fontSize: FontSize.XXSmall
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.White
                                    }
                                }
                            }

                            Container {
                                topMargin: 2
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    id: book1Author
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    autoSize.maxLineCount: 1
                                }

                                Label {
                                    id: book1Duration
                                    leftMargin: 6
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                            }

                            Container {
                                topMargin: 6
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    verticalAlignment: VerticalAlignment.Center
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    preferredHeight: 6
                                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    Container {
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.create("#10B981")
                                        layoutProperties: StackLayoutProperties { spaceQuota: Math.max(1, bookCard1.progressVal) }
                                    }
                                    Container {
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.Transparent
                                        layoutProperties: StackLayoutProperties { spaceQuota: Math.max(1, 100 - bookCard1.progressVal) }
                                    }
                                }

                                Label {
                                    id: book1PageCount
                                    leftMargin: 8
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.XXSmall
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                }

                                Label {
                                    id: book1ProgressPct
                                    leftMargin: 4
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.XXSmall
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                            }
                        }

                        Button {
                            leftMargin: 8
                            preferredWidth: 60
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Đọc") + Retranslate.onLanguageChanged
                            onClicked: {
                                statsSheet.close();
                                app.openBook(bookCard1.filePath);
                            }
                        }
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    statsSheet.close();
                                    app.openBook(bookCard1.filePath);
                                }
                            }
                        ]
                    }

                    // Book list slot 2
                    Container {
                        id: bookCard2
                        visible: false
                        topMargin: 8
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 10
                        bottomPadding: 10
                        leftPadding: 12
                        rightPadding: 12
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        property string filePath: ""
                        property alias titleText: book2Title.text
                        property alias authorText: book2Author.text
                        property alias coverSource: book2Cover.imageSource
                        property alias formatText: book2Format.text
                        property alias durationText: book2Duration.text
                        property alias pageCountText: book2PageCount.text
                        property alias progressText: book2ProgressPct.text
                        property int progressVal: 0

                        Container {
                            preferredWidth: 48
                            preferredHeight: 66
                            minWidth: 48
                            minHeight: 66
                            verticalAlignment: VerticalAlignment.Center
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                            layout: DockLayout {}

                            ImageView {
                                id: book2Cover
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                scalingMethod: ScalingMethod.AspectFill
                                imageSource: "asset:///icons/default_cover.png"
                            }
                        }

                        Container {
                            leftMargin: 12
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Center

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    id: book2Title
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
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
                                        id: book2Format
                                        text: "EPUB"
                                        textStyle.fontSize: FontSize.XXSmall
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.White
                                    }
                                }
                            }

                            Container {
                                topMargin: 2
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    id: book2Author
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    autoSize.maxLineCount: 1
                                }

                                Label {
                                    id: book2Duration
                                    leftMargin: 6
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                            }

                            Container {
                                topMargin: 6
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    verticalAlignment: VerticalAlignment.Center
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    preferredHeight: 6
                                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    Container {
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.create("#10B981")
                                        layoutProperties: StackLayoutProperties { spaceQuota: Math.max(1, bookCard2.progressVal) }
                                    }
                                    Container {
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.Transparent
                                        layoutProperties: StackLayoutProperties { spaceQuota: Math.max(1, 100 - bookCard2.progressVal) }
                                    }
                                }

                                Label {
                                    id: book2PageCount
                                    leftMargin: 8
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.XXSmall
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                }

                                Label {
                                    id: book2ProgressPct
                                    leftMargin: 4
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.XXSmall
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                            }
                        }

                        Button {
                            leftMargin: 8
                            preferredWidth: 60
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Đọc") + Retranslate.onLanguageChanged
                            onClicked: {
                                statsSheet.close();
                                app.openBook(bookCard2.filePath);
                            }
                        }
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    statsSheet.close();
                                    app.openBook(bookCard2.filePath);
                                }
                            }
                        ]
                    }

                    // Book list slot 3
                    Container {
                        id: bookCard3
                        visible: false
                        topMargin: 8
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 10
                        bottomPadding: 10
                        leftPadding: 12
                        rightPadding: 12
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        property string filePath: ""
                        property alias titleText: book3Title.text
                        property alias authorText: book3Author.text
                        property alias coverSource: book3Cover.imageSource
                        property alias formatText: book3Format.text
                        property alias durationText: book3Duration.text
                        property alias pageCountText: book3PageCount.text
                        property alias progressText: book3ProgressPct.text
                        property int progressVal: 0

                        Container {
                            preferredWidth: 48
                            preferredHeight: 66
                            minWidth: 48
                            minHeight: 66
                            verticalAlignment: VerticalAlignment.Center
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                            layout: DockLayout {}

                            ImageView {
                                id: book3Cover
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                scalingMethod: ScalingMethod.AspectFill
                                imageSource: "asset:///icons/default_cover.png"
                            }
                        }

                        Container {
                            leftMargin: 12
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Center

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    id: book3Title
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
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
                                        id: book3Format
                                        text: "EPUB"
                                        textStyle.fontSize: FontSize.XXSmall
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.White
                                    }
                                }
                            }

                            Container {
                                topMargin: 2
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    id: book3Author
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    autoSize.maxLineCount: 1
                                }

                                Label {
                                    id: book3Duration
                                    leftMargin: 6
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                            }

                            Container {
                                topMargin: 6
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    verticalAlignment: VerticalAlignment.Center
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    preferredHeight: 6
                                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    Container {
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.create("#10B981")
                                        layoutProperties: StackLayoutProperties { spaceQuota: Math.max(1, bookCard3.progressVal) }
                                    }
                                    Container {
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.Transparent
                                        layoutProperties: StackLayoutProperties { spaceQuota: Math.max(1, 100 - bookCard3.progressVal) }
                                    }
                                }

                                Label {
                                    id: book3PageCount
                                    leftMargin: 8
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.XXSmall
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                }

                                Label {
                                    id: book3ProgressPct
                                    leftMargin: 4
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.XXSmall
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                            }
                        }

                        Button {
                            leftMargin: 8
                            preferredWidth: 60
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Đọc") + Retranslate.onLanguageChanged
                            onClicked: {
                                statsSheet.close();
                                app.openBook(bookCard3.filePath);
                            }
                        }
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    statsSheet.close();
                                    app.openBook(bookCard3.filePath);
                                }
                            }
                        ]
                    }

                    // Book list slot 4
                    Container {
                        id: bookCard4
                        visible: false
                        topMargin: 8
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 10
                        bottomPadding: 10
                        leftPadding: 12
                        rightPadding: 12
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        property string filePath: ""
                        property alias titleText: book4Title.text
                        property alias authorText: book4Author.text
                        property alias coverSource: book4Cover.imageSource
                        property alias formatText: book4Format.text
                        property alias durationText: book4Duration.text
                        property alias pageCountText: book4PageCount.text
                        property alias progressText: book4ProgressPct.text
                        property int progressVal: 0

                        Container {
                            preferredWidth: 48
                            preferredHeight: 66
                            minWidth: 48
                            minHeight: 66
                            verticalAlignment: VerticalAlignment.Center
                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                            layout: DockLayout {}

                            ImageView {
                                id: book4Cover
                                horizontalAlignment: HorizontalAlignment.Fill
                                verticalAlignment: VerticalAlignment.Fill
                                scalingMethod: ScalingMethod.AspectFill
                                imageSource: "asset:///icons/default_cover.png"
                            }
                        }

                        Container {
                            leftMargin: 12
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            horizontalAlignment: HorizontalAlignment.Fill
                            verticalAlignment: VerticalAlignment.Center

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    id: book4Title
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
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
                                        id: book4Format
                                        text: "EPUB"
                                        textStyle.fontSize: FontSize.XXSmall
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.White
                                    }
                                }
                            }

                            Container {
                                topMargin: 2
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    id: book4Author
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    autoSize.maxLineCount: 1
                                }

                                Label {
                                    id: book4Duration
                                    leftMargin: 6
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                            }

                            Container {
                                topMargin: 6
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    verticalAlignment: VerticalAlignment.Center
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    preferredHeight: 6
                                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                    Container {
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.create("#10B981")
                                        layoutProperties: StackLayoutProperties { spaceQuota: Math.max(1, bookCard4.progressVal) }
                                    }
                                    Container {
                                        verticalAlignment: VerticalAlignment.Fill
                                        background: Color.Transparent
                                        layoutProperties: StackLayoutProperties { spaceQuota: Math.max(1, 100 - bookCard4.progressVal) }
                                    }
                                }

                                Label {
                                    id: book4PageCount
                                    leftMargin: 8
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.XXSmall
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                }

                                Label {
                                    id: book4ProgressPct
                                    leftMargin: 4
                                    verticalAlignment: VerticalAlignment.Center
                                    textStyle.fontSize: FontSize.XXSmall
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                            }
                        }

                        Button {
                            leftMargin: 8
                            preferredWidth: 60
                            verticalAlignment: VerticalAlignment.Center
                            text: qsTr("Đọc") + Retranslate.onLanguageChanged
                            onClicked: {
                                statsSheet.close();
                                app.openBook(bookCard4.filePath);
                            }
                        }
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    statsSheet.close();
                                    app.openBook(bookCard4.filePath);
                                }
                            }
                        ]
                    }
                    }
                }
                } // End of calendarModeContainer

                // =============================================================
                // MODE 2: THEO SÁCH (BY BOOK VIEW - KOReader Style)
                // =============================================================
                Container {
                    id: byBookModeContainer
                    visible: statsSheet.mainMode === "by_book"
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    Header {
                        title: qsTr("SÁCH ĐANG CHỌN") + Retranslate.onLanguageChanged
                    }

                    // Hero Card for Selected Book
                    Container {
                        id: byBookHeroContainer
                        topMargin: 6
                        bottomMargin: 14
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        topPadding: 1
                        bottomPadding: 1
                        leftPadding: 1
                        rightPadding: 1
                        layout: DockLayout {}

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                            topPadding: 14
                            bottomPadding: 14
                            leftPadding: 14
                            rightPadding: 14
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            // Book Header: Cover + Title + Author + Format
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    preferredWidth: 70
                                    preferredHeight: 96
                                    minWidth: 70
                                    minHeight: 96
                                    verticalAlignment: VerticalAlignment.Center
                                    background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                    layout: DockLayout {}

                                    ImageView {
                                        id: byBookCover
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        scalingMethod: ScalingMethod.AspectFill
                                        imageSource: "asset:///icons/default_cover.png"
                                    }
                                }

                                Container {
                                    leftMargin: 12
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                        Label {
                                            id: byBookTitle
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            textStyle.base: SystemDefaults.TextStyles.TitleText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                                            autoSize.maxLineCount: 2
                                        }

                                        Container {
                                            leftMargin: 8
                                            verticalAlignment: VerticalAlignment.Center
                                            background: Color.create("#10B981")
                                            leftPadding: 8
                                            rightPadding: 8
                                            topPadding: 2
                                            bottomPadding: 2

                                            Label {
                                                id: byBookFormatBadge
                                                text: "EPUB"
                                                textStyle.fontSize: FontSize.XXSmall
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: Color.White
                                            }
                                        }
                                    }

                                    Label {
                                        id: byBookAuthor
                                        topMargin: 4
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                        autoSize.maxLineCount: 1
                                    }

                                    // Progress Bar + Percentage
                                    Container {
                                        topMargin: 8
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                        Container {
                                            verticalAlignment: VerticalAlignment.Center
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            preferredHeight: 8
                                            background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Container {
                                                id: byBookProgressBar
                                                verticalAlignment: VerticalAlignment.Fill
                                                background: Color.create("#10B981")
                                                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                                            }
                                            Container {
                                                id: byBookProgressEmpty
                                                verticalAlignment: VerticalAlignment.Fill
                                                background: Color.Transparent
                                                layoutProperties: StackLayoutProperties { spaceQuota: 99 }
                                            }
                                        }

                                        Label {
                                            id: byBookProgressText
                                            leftMargin: 8
                                            verticalAlignment: VerticalAlignment.Center
                                            text: "0%"
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: Color.create("#10B981")
                                        }
                                    }
                                }
                            }

                            Divider {
                                topMargin: 12
                                bottomMargin: 10
                            }

                            // 2x2 Metric Grid
                            // Row 1: Thời gian đã đọc | Ước tính còn lại
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                                    topPadding: 8
                                    bottomPadding: 8
                                    leftPadding: 10
                                    rightPadding: 10
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Label {
                                        text: qsTr("THỜI GIAN ĐÃ ĐỌC") + Retranslate.onLanguageChanged
                                        textStyle.fontSize: FontSize.XXSmall
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    }
                                    Label {
                                        id: byBookTimeSpent
                                        topMargin: 2
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create("#10B981")
                                        autoSize.maxLineCount: 1
                                    }
                                }

                                Container {
                                    leftMargin: 8
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                                    topPadding: 8
                                    bottomPadding: 8
                                    leftPadding: 10
                                    rightPadding: 10
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Label {
                                        text: qsTr("ƯỚC TÍNH CÒN LẠI") + Retranslate.onLanguageChanged
                                        textStyle.fontSize: FontSize.XXSmall
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    }
                                    Label {
                                        id: byBookRemaining
                                        topMargin: 2
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create("#10B981")
                                        autoSize.maxLineCount: 1
                                    }
                                }
                            }

                            // Row 2: Tốc độ đọc | Trang đã đọc
                            Container {
                                topMargin: 8
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                                    topPadding: 8
                                    bottomPadding: 8
                                    leftPadding: 10
                                    rightPadding: 10
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Label {
                                        text: qsTr("TỐC ĐỘ ĐỌC") + Retranslate.onLanguageChanged
                                        textStyle.fontSize: FontSize.XXSmall
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    }
                                    Label {
                                        id: byBookSpeed
                                        topMargin: 2
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                                    }
                                }

                                Container {
                                    leftMargin: 8
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                                    topPadding: 8
                                    bottomPadding: 8
                                    leftPadding: 10
                                    rightPadding: 10
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Label {
                                        text: qsTr("TRANG ĐÃ ĐỌC") + Retranslate.onLanguageChanged
                                        textStyle.fontSize: FontSize.XXSmall
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    }
                                    Label {
                                        id: byBookPages
                                        topMargin: 2
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                                    }
                                }
                            }

                            // Open book button
                            Button {
                                topMargin: 12
                                horizontalAlignment: HorizontalAlignment.Fill
                                text: qsTr("Đọc tiếp cuốn sách này") + Retranslate.onLanguageChanged
                                appearance: ButtonAppearance.Primary
                                onClicked: {
                                    statsSheet.close();
                                    app.openBook(statsSheet.selectedBookPath);
                                }
                            }
                        }
                    }

                    // Empty state container
                    Container {
                        id: byBookNoDataContainer
                        visible: false
                        topMargin: 6
                        bottomMargin: 14
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 24
                        bottomPadding: 24
                        leftPadding: 16
                        rightPadding: 16
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Label {
                            horizontalAlignment: HorizontalAlignment.Center
                            text: qsTr("Chưa có dữ liệu sách đọc. Hãy mở một cuốn sách để bắt đầu ghi nhận thống kê!") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                            multiline: true
                        }
                    }

                    Header {
                        title: qsTr("TẤT CẢ SÁCH TRONG THỐNG KÊ") + Retranslate.onLanguageChanged
                    }

                    // 6 Book slots
                    ReadingBookSlotItem {
                        id: byBookSlot0
                        visible: false
                        appTheme: bookManager.appTheme
                        onBookSelected: {
                            statsSheet.selectedBookPath = path;
                            statsSheet.refreshByBookStats();
                        }
                        onBookOpened: {
                            statsSheet.close();
                            app.openBook(path);
                        }
                    }

                    ReadingBookSlotItem {
                        id: byBookSlot1
                        visible: false
                        appTheme: bookManager.appTheme
                        onBookSelected: {
                            statsSheet.selectedBookPath = path;
                            statsSheet.refreshByBookStats();
                        }
                        onBookOpened: {
                            statsSheet.close();
                            app.openBook(path);
                        }
                    }

                    ReadingBookSlotItem {
                        id: byBookSlot2
                        visible: false
                        appTheme: bookManager.appTheme
                        onBookSelected: {
                            statsSheet.selectedBookPath = path;
                            statsSheet.refreshByBookStats();
                        }
                        onBookOpened: {
                            statsSheet.close();
                            app.openBook(path);
                        }
                    }

                    ReadingBookSlotItem {
                        id: byBookSlot3
                        visible: false
                        appTheme: bookManager.appTheme
                        onBookSelected: {
                            statsSheet.selectedBookPath = path;
                            statsSheet.refreshByBookStats();
                        }
                        onBookOpened: {
                            statsSheet.close();
                            app.openBook(path);
                        }
                    }

                    ReadingBookSlotItem {
                        id: byBookSlot4
                        visible: false
                        appTheme: bookManager.appTheme
                        onBookSelected: {
                            statsSheet.selectedBookPath = path;
                            statsSheet.refreshByBookStats();
                        }
                        onBookOpened: {
                            statsSheet.close();
                            app.openBook(path);
                        }
                    }

                    ReadingBookSlotItem {
                        id: byBookSlot5
                        visible: false
                        appTheme: bookManager.appTheme
                        onBookSelected: {
                            statsSheet.selectedBookPath = path;
                            statsSheet.refreshByBookStats();
                        }
                        onBookOpened: {
                            statsSheet.close();
                            app.openBook(path);
                        }
                    }
                } // End of byBookModeContainer

                // =============================================================
                // MODE 3: XU HƯỚNG (TRENDS VIEW - KOReader Style)
                // =============================================================
                Container {
                    id: trendsModeContainer
                    visible: statsSheet.mainMode === "trends"
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    Header {
                        title: qsTr("XU HƯỚNG 7 NGÀY GẦN NHẤT") + Retranslate.onLanguageChanged
                    }

                    Container {
                        topMargin: 6
                        bottomMargin: 14
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        topPadding: 1
                        bottomPadding: 1
                        leftPadding: 1
                        rightPadding: 1
                        layout: DockLayout {}

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                            topPadding: 14
                            bottomPadding: 14
                            leftPadding: 12
                            rightPadding: 12
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            // 3 Summary Metrics: Tuần này, Trung bình, Cao nhất
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                    Label {
                                        text: qsTr("TỔNG TUẦN NÀY") + Retranslate.onLanguageChanged
                                        textStyle.fontSize: FontSize.XXSmall
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    }
                                    Label {
                                        id: trendTotalWeekLabel
                                        topMargin: 2
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create("#10B981")
                                    }
                                }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                    Label {
                                        text: qsTr("TRUNG BÌNH/NGÀY") + Retranslate.onLanguageChanged
                                        textStyle.fontSize: FontSize.XXSmall
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    }
                                    Label {
                                        id: trendAvgDailyLabel
                                        topMargin: 2
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create("#10B981")
                                    }
                                }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                    Label {
                                        text: qsTr("CAO NHẤT") + Retranslate.onLanguageChanged
                                        textStyle.fontSize: FontSize.XXSmall
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    }
                                    Label {
                                        id: trendBestDayLabel
                                        topMargin: 2
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                                        autoSize.maxLineCount: 1
                                    }
                                }
                            }

                            Divider {
                                topMargin: 10
                                bottomMargin: 10
                            }

                            // 7-Column Bar Chart
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                ReadingTrendBarItem {
                                    id: trendCol0
                                    appTheme: bookManager.appTheme
                                }
                                ReadingTrendBarItem {
                                    id: trendCol1
                                    appTheme: bookManager.appTheme
                                }
                                ReadingTrendBarItem {
                                    id: trendCol2
                                    appTheme: bookManager.appTheme
                                }
                                ReadingTrendBarItem {
                                    id: trendCol3
                                    appTheme: bookManager.appTheme
                                }
                                ReadingTrendBarItem {
                                    id: trendCol4
                                    appTheme: bookManager.appTheme
                                }
                                ReadingTrendBarItem {
                                    id: trendCol5
                                    appTheme: bookManager.appTheme
                                }
                                ReadingTrendBarItem {
                                    id: trendCol6
                                    appTheme: bookManager.appTheme
                                }
                            }
                        }
                    }

                    // Format Breakdown
                    Header {
                        title: qsTr("PHÂN BỔ THEO ĐỊNH DẠNG TÀI LIỆU") + Retranslate.onLanguageChanged
                    }

                    Container {
                        topMargin: 6
                        bottomMargin: 14
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        topPadding: 1
                        bottomPadding: 1
                        leftPadding: 1
                        rightPadding: 1
                        layout: DockLayout {}

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                            topPadding: 14
                            bottomPadding: 14
                            leftPadding: 14
                            rightPadding: 14
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            // EPUB
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Label {
                                    text: "EPUB"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                                Label {
                                    id: epubTimeLabel
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    textStyle.textAlign: TextAlign.Right
                                }
                            }
                            Container {
                                topMargin: 4
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 6
                                background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Container {
                                    id: epubBarFill
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.create("#10B981")
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                                }
                                Container {
                                    id: epubBarEmpty
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.Transparent
                                    layoutProperties: StackLayoutProperties { spaceQuota: 99 }
                                }
                            }

                            // PDF
                            Container {
                                topMargin: 10
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Label {
                                    text: "PDF"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#EF4444")
                                }
                                Label {
                                    id: pdfTimeLabel
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    textStyle.textAlign: TextAlign.Right
                                }
                            }
                            Container {
                                topMargin: 4
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 6
                                background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Container {
                                    id: pdfBarFill
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.create("#EF4444")
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                                }
                                Container {
                                    id: pdfBarEmpty
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.Transparent
                                    layoutProperties: StackLayoutProperties { spaceQuota: 99 }
                                }
                            }

                            // TXT
                            Container {
                                topMargin: 10
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Label {
                                    text: "TXT"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#3B82F6")
                                }
                                Label {
                                    id: txtTimeLabel
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    textStyle.textAlign: TextAlign.Right
                                }
                            }
                            Container {
                                topMargin: 4
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 6
                                background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Container {
                                    id: txtBarFill
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.create("#3B82F6")
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                                }
                                Container {
                                    id: txtBarEmpty
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.Transparent
                                    layoutProperties: StackLayoutProperties { spaceQuota: 99 }
                                }
                            }

                            // FB2
                            Container {
                                topMargin: 10
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Label {
                                    text: "FB2"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#F59E0B")
                                }
                                Label {
                                    id: fb2TimeLabel
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                    textStyle.textAlign: TextAlign.Right
                                }
                            }
                            Container {
                                topMargin: 4
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 6
                                background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Container {
                                    id: fb2BarFill
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.create("#F59E0B")
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                                }
                                Container {
                                    id: fb2BarEmpty
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.Transparent
                                    layoutProperties: StackLayoutProperties { spaceQuota: 99 }
                                }
                            }
                        }
                    }
                } // End of trendsModeContainer

                // =============================================================
                // MODE 4: TỔNG QUAN TÍCH LŨY (OVERVIEW - KOReader Style)
                // =============================================================
                Container {
                    id: overviewModeContainer
                    visible: statsSheet.mainMode === "overview"
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    Header {
                        title: qsTr("TỔNG QUAN TÍCH LŨY") + Retranslate.onLanguageChanged
                    }

                    // Hero Dashboard Banner (Total Time & Total Days)
                    Container {
                        topMargin: 6
                        bottomMargin: 14
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                        topPadding: 16
                        bottomPadding: 16
                        leftPadding: 16
                        rightPadding: 16
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        Container {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                            Label {
                                text: qsTr("TỔNG THỜI GIAN ĐÃ ĐỌC") + Retranslate.onLanguageChanged
                                textStyle.fontSize: FontSize.XXSmall
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                            }
                            Label {
                                id: allTimeTotalLabel
                                topMargin: 4
                                text: "--"
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create("#10B981")
                            }
                        }

                        Container {
                            leftMargin: 12
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                            Label {
                                text: qsTr("TỔNG SỐ NGÀY ĐÃ ĐỌC") + Retranslate.onLanguageChanged
                                textStyle.fontSize: FontSize.XXSmall
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                            }
                            Label {
                                id: allTimeDaysLabel
                                topMargin: 4
                                text: "--"
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create("#10B981")
                            }
                        }
                    }

                    // Detailed Streak & Record Card
                    Container {
                        bottomMargin: 20
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: bookManager.appTheme === "dark" ? Color.create("#222222") : Color.create("#E5E5E5")
                        topPadding: 1
                        bottomPadding: 1
                        leftPadding: 1
                        rightPadding: 1
                        layout: DockLayout {}

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                            topPadding: 14
                            bottomPadding: 14
                            leftPadding: 14
                            rightPadding: 14
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            // Chuỗi hiện tại
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("Chuỗi ngày đọc hiện tại") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                }
                                Label {
                                    id: allTimeCurrentStreakLabel
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }
                            }

                            Divider { topMargin: 6; bottomMargin: 6 }

                            // Chuỗi kỷ lục
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("Kỷ lục chuỗi ngày dài nhất") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                }
                                Label {
                                    id: allTimeMaxStreakLabel
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#F59E0B")
                                }
                            }

                            Divider { topMargin: 6; bottomMargin: 6 }

                            // Ngày đọc nhiều nhất
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("Kỷ lục ngày đọc nhiều nhất") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                }
                                Label {
                                    id: allTimeBestDayLabel
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                                }
                            }

                            Divider { topMargin: 6; bottomMargin: 6 }

                            // Tình trạng sách
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("Tổng kết tủ sách") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E9AA8") : Color.create("#777777")
                                }
                                Label {
                                    id: allTimeBooksLabel
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                                }
                            }
                        }
                    }
                } // End of overviewModeContainer
            }
        }
    }
}
