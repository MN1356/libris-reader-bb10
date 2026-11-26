import bb.cascades 1.3

Sheet {
    id: statsSheet

    property string mainMode: "overview" // "overview", "calendar", "by_book"
    property string selectedBookPath: ""
    property int curYear: 2026
    property int curMonth: 11
    property string selectedDate: ""
    property string viewMode: "month" // "month" or "week"
    property string curWeekAnchorDate: ""
    property bool isOpen: false
    property bool row5HasCurrentMonth: true
    property bool hasSelectedDayReading: false
    property bool hasAnyBookStats: false
    property int trackedBooksCount: 0
    property bool selectedBookIsPaged: false
    property bool hasAnyFormatData: false

    // Shared reactive KPI strings for Calendar Month Summary
    property string monthTitleStr: ""
    property string weekTitleStr: ""
    property string monthTotalShortStr: "--"
    property string monthDaysValStr: "-- / --"
    property string monthAvgShortStr: "--"

    // Responsive sizing for 1:1 square screens: 720x720 (Q10/Classic) vs 1440x1440 (Passport)
    property bool isHiRes: bookManager.isHiRes
    property real uiScale: bookManager.uiScale
    property int calendarCellHeight: isHiRes ? 116 : 74
    property int trendTrackHeight: isHiRes ? 132 : 84

    function sp(px) {
        return bookManager.dp(px);
    }

    function formatDays(n) {
        if (bookManager.language === "en") {
            return n + (n === 1 ? " day" : " days");
        }
        return n + " " + qsTr("ngày");
    }

    function formatConsecutiveDays(n) {
        if (bookManager.language === "en") {
            return n + (n === 1 ? " consecutive day" : " consecutive days");
        }
        return n + " " + qsTr("ngày liên tục");
    }

    function formatBooks(n) {
        if (bookManager.language === "en") {
            return n + (n === 1 ? " book" : " books");
        }
        return n + " " + qsTr("cuốn");
    }

    function calcDisplayMins(secs) {
        if (secs <= 0) return 0;
        if (secs < 60) return 1;
        return Math.round(secs / 60.0);
    }

    function calcGoalPct(secs, goalMins) {
        if (secs <= 0) return 0;
        return Math.min(100, Math.round(calcDisplayMins(secs) * 100 / Math.max(1, goalMins)));
    }

    onOpened: {
        isOpen = true;
        var now = new Date();
        curYear = now.getFullYear();
        curMonth = now.getMonth() + 1;
        var d = now.getDate();
        selectedDate = bookManager.formatDateYMD(curYear, curMonth, d);
        curWeekAnchorDate = selectedDate;
        refreshAll();
    }

    onClosed: {
        isOpen = false;
    }

    onCreationCompleted: {
        bookManager.languageChanged.connect(onLanguageOrThemeChanged);
        bookManager.appThemeChanged.connect(onLanguageOrThemeChanged);
    }

    function onLanguageOrThemeChanged() {
        if (statsSheet.isOpen) {
            refreshAll();
        }
    }

    function resolveDefaultCover(cov, fmt) {
        if (cov && cov !== "" && cov.indexOf("default_cover") === -1) {
            return (cov.indexOf("/") === 0) ? ("file://" + cov) : cov;
        }
        var f = (fmt ? fmt : "epub").toLowerCase();
        if (f === "azw" || f === "azw3" || f === "kf8" || f === "prc") f = "mobi";
        else if (f === "djv") f = "djvu";
        if (f !== "pdf" && f !== "epub" && f !== "mobi" && f !== "fb2" && f !== "cbz" && f !== "djvu" && f !== "txt") f = "epub";
        return "asset:///icons/default_cover_" + f + ".png";
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
        mainMode = "calendar";
        viewMode = "month";
        statsModeControl.selectedIndex = 1;
        refreshAll();
        statsScrollView.resetViewableArea(ScrollAnimation.Default);
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

    function selectDay(dateStr) {
        if (!dateStr || dateStr === "") return;
        selectedDate = dateStr;
        curWeekAnchorDate = dateStr;
        var parts = dateStr.split("-");
        if (parts.length === 3) {
            var y = parseInt(parts[0], 10);
            var m = parseInt(parts[1], 10);
            if (!isNaN(y) && !isNaN(m) && (y !== curYear || m !== curMonth)) {
                curYear = y;
                curMonth = m;
                refreshCalendarAndMonthStats();
            }
        }
        updateCalendarSelection();
        refreshWeekSelection();
        refreshSelectedDayDetails();
    }

    function jumpToDateFromTrend(dateStr) {
        if (!dateStr || dateStr === "") return;
        var parts = dateStr.split("-");
        if (parts.length === 3) {
            var y = parseInt(parts[0], 10);
            var m = parseInt(parts[1], 10);
            if (!isNaN(y) && !isNaN(m)) {
                curYear = y;
                curMonth = m;
            }
        }
        selectedDate = dateStr;
        curWeekAnchorDate = dateStr;
        mainMode = "calendar";
        viewMode = "month";
        statsModeControl.selectedIndex = 1;
        refreshCalendarAndMonthStats();
        refreshSelectedDayDetails();
        statsScrollView.resetViewableArea(ScrollAnimation.Default);
    }

    function openBookFromStats(filePath) {
        if (!filePath || filePath === "") return;
        statsSheet.close();
        app.openBook(filePath);
    }

    function selectBookFromStats(filePath) {
        if (!filePath || filePath === "") return;
        selectedBookPath = filePath;
        refreshSelectedBookHero();
        for (var i = 0; i < allBooksListContainer.count(); ++i) {
            var slot = allBooksListContainer.at(i);
            if (slot) {
                slot.isSelected = (slot.filePath === selectedBookPath);
            }
        }
        statsScrollView.resetViewableArea(ScrollAnimation.Default);
    }

    function clearDynamicChildren(container) {
        for (var i = container.count() - 1; i >= 0; --i) {
            var child = container.at(i);
            container.remove(child);
            if (child && typeof child.destroy === "function") {
                child.destroy();
            }
        }
    }

    function refreshAll() {
        refreshCalendarAndMonthStats();
        refreshWeekView();
        refreshSelectedDayDetails();
        refreshOverallStats();
        refreshTrendsStats();
        refreshByBookStats();
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
        weekTitleStr = wd.weekTitle;
        weekTotalTimeLabel.text = wd.shortTotalDurationStr ? wd.shortTotalDurationStr : wd.totalDurationStr;
        weekReadingDaysLabel.text = wd.readingDays + " / 7";
        weekAvgDailyTimeLabel.text = wd.shortAvgDailyTimeStr ? wd.shortAvgDailyTimeStr : wd.avgDailyTimeStr;

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
        var ms = bookManager.getMonthStats(curYear, curMonth);
        monthTitleStr = ms.monthName;
        monthTotalShortStr = ms.shortTotalTimeStr ? ms.shortTotalTimeStr : ms.totalTimeStr;
        monthDaysValStr = ms.readingDays + " / " + ms.daysInMonth;
        monthAvgShortStr = ms.shortAvgDailyTimeStr ? ms.shortAvgDailyTimeStr : ms.avgDailyTimeStr;

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
        row5HasCurrentMonth = (days.length >= 36 && days[35].isCurrentMonth);
    }

    function refreshSelectedDayDetails() {
        if (!selectedDate || selectedDate === "") {
            var now = new Date();
            selectedDate = bookManager.formatDateYMD(now.getFullYear(), now.getMonth() + 1, now.getDate());
        }

        var daySecs = bookManager.getDailyReadingSeconds(selectedDate);
        var durStr = (daySecs > 0) ? bookManager.formatShortDuration(daySecs) : "";

        var dateParts = selectedDate.split("-");
        var y = dateParts[0];
        var m = dateParts[1];
        var d = dateParts[2];
        selectedDayTitleLabel.text = qsTr("Ngày") + " " + d + "/" + m + "/" + y;

        if (daySecs > 0) {
            hasSelectedDayReading = true;
            selectedDayTotalLabel.text = durStr;
        } else {
            hasSelectedDayReading = false;
            selectedDayTotalLabel.text = qsTr("Chưa có thời gian đọc");
        }

        var books = bookManager.getDayBooks(selectedDate);
        clearDynamicChildren(dayBooksListContainer);
        noDayReadingLabel.visible = (books.length === 0 && daySecs <= 0);
        dayDetailDivider.visible = (books.length > 0 || noDayReadingLabel.visible);

        for (var i = 0; i < books.length; ++i) {
            var b = books[i];
            var card = bookSlotDef.createObject();
            if (!card) continue;
            card.uiScale = statsSheet.uiScale;
            card.appTheme = bookManager.appTheme;
            card.openOnRowTap = true;
            card.showActionButton = true;
            card.isSelected = false;
            card.filePath = b.filePath;
            card.titleText = b.title;
            card.authorText = (b.author && b.author !== "") ? b.author : qsTr("Chưa rõ tác giả");
            card.coverSource = resolveDefaultCover(b.cover, b.format);
            card.formatText = b.formatBadge ? b.formatBadge : (b.format ? b.format.toUpperCase() : "EPUB");
            card.durationText = b.shortDayDurationStr ? b.shortDayDurationStr : b.dayDurationStr;
            card.pageCountText = b.pageCountStr ? b.pageCountStr : "";
            card.progressVal = b.progress ? b.progress : 0;
            card.progressText = (b.progress ? b.progress : 0) + "%";
            card.bookOpened.connect(statsSheet.openBookFromStats);
            dayBooksListContainer.add(card);
        }
    }

    function refreshOverallStats() {
        var os = bookManager.getOverallStats();
        allTimeTotalLabel.text = (os.shortTotalReadingTimeStr && os.shortTotalReadingTimeStr !== "") ? os.shortTotalReadingTimeStr : os.totalReadingTimeStr;
        allTimeDaysLabel.text = formatDays(os.totalReadingDays);
        allTimeCurrentStreakLabel.text = formatConsecutiveDays(os.currentStreak);
        allTimeMaxStreakLabel.text = formatDays(os.allTimeStreak);
        if (os.bestDaySeconds > 0) {
            var bp = os.bestDayDate.split("-");
            var dateFmt = (bp.length === 3) ? (bp[2] + "/" + bp[1] + "/" + bp[0]) : os.bestDayDate;
            var bestDur = (os.shortBestDayTimeStr && os.shortBestDayTimeStr !== "") ? os.shortBestDayTimeStr : os.bestDayTimeStr;
            allTimeBestDayLabel.text = bestDur + " (" + dateFmt + ")";
        } else {
            allTimeBestDayLabel.text = qsTr("Chưa có");
        }
        allTimeBooksLabel.text = os.completedBooks + " " + qsTr("đã xong") + " • " + os.readingBooks + " " + qsTr("đang đọc");
    }

    function refreshSelectedBookHero() {
        if (!hasAnyBookStats || !selectedBookPath || selectedBookPath === "") {
            return;
        }
        var bs = bookManager.getBookDetailedStats(selectedBookPath);
        selectedBookIsPaged = bs.isPagedFmt ? true : false;
        byBookCover.imageSource = resolveDefaultCover(bs.cover, bs.format);
        byBookTitle.text = bs.title ? bs.title : "";
        byBookAuthor.text = (bs.author && bs.author !== "") ? bs.author : qsTr("Chưa rõ tác giả");
        byBookFormatBadge.text = bs.formatBadge ? bs.formatBadge : "EPUB";
        byBookProgressText.text = bs.progressStr ? bs.progressStr : "0%";
        byBookTimeSpent.text = (bs.shortTotalTimeStr && bs.shortTotalTimeStr !== "") ? bs.shortTotalTimeStr : (bs.totalTimeStr ? bs.totalTimeStr : "--");
        byBookRemaining.text = bs.remainingTimeStr ? bs.remainingTimeStr : "--";
        byBookSpeed.text = bs.readingSpeedStr ? bs.readingSpeedStr : "--";
        byBookPages.text = bs.pageStr ? bs.pageStr : "--";

        var prog = bs.progress ? bs.progress : 0;
        byBookProgressBar.visible = (prog > 0);
        byBookProgressEmpty.visible = (prog < 100);
        bookManager.setControlSpaceQuota(byBookProgressBar, Math.max(0.001, prog));
        bookManager.setControlSpaceQuota(byBookProgressEmpty, Math.max(0.001, 100 - prog));
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
        trackedBooksCount = allList.length;
        if ((!selectedBookPath || selectedBookPath === "") && allList.length > 0) {
            selectedBookPath = allList[0].filePath;
        }

        hasAnyBookStats = (allList.length > 0 && selectedBookPath && selectedBookPath !== "");
        refreshSelectedBookHero();

        clearDynamicChildren(allBooksListContainer);
        if (allList.length > 1) {
            for (var i = 0; i < allList.length; ++i) {
                var b = allList[i];
                var slot = bookSlotDef.createObject();
                if (!slot) continue;
                slot.uiScale = statsSheet.uiScale;
                slot.appTheme = bookManager.appTheme;
                slot.openOnRowTap = false;
                slot.showActionButton = true;
                slot.filePath = b.filePath;
                slot.titleText = b.title ? b.title : "";
                slot.authorText = (b.author && b.author !== "") ? b.author : qsTr("Chưa rõ tác giả");
                slot.coverSource = resolveDefaultCover(b.cover, b.format);
                slot.formatText = b.formatBadge ? b.formatBadge : "EPUB";
                slot.durationText = b.shortTotalTimeStr ? b.shortTotalTimeStr : (b.totalTimeStr ? b.totalTimeStr : "");
                slot.pageCountText = (b.totalPages > 0 && b.pageStr) ? b.pageStr : "";
                slot.progressText = b.progressStr ? b.progressStr : "";
                slot.progressVal = b.progress ? b.progress : 0;
                slot.isSelected = (b.filePath === selectedBookPath);
                slot.bookSelected.connect(statsSheet.selectBookFromStats);
                slot.bookOpened.connect(statsSheet.openBookFromStats);
                allBooksListContainer.add(slot);
            }
        }
    }

    function applyDistRow(rowCtrl, fillBar, emptyBar, labelCtrl, timeStr, booksCount, pct, isVisible, topGap) {
        rowCtrl.visible = isVisible;
        rowCtrl.topMargin = topGap;
        if (!isVisible) return;
        var durShort = timeStr;
        labelCtrl.text = durShort + " (" + formatBooks(booksCount) + " • " + pct + "%)";
        fillBar.visible = (pct > 0);
        emptyBar.visible = (pct < 100);
        bookManager.setControlSpaceQuota(fillBar, Math.max(0.001, pct));
        bookManager.setControlSpaceQuota(emptyBar, Math.max(0.001, 100 - pct));
    }

    function refreshTrendsStats() {
        var trendDays = bookManager.getRecentDaysTrend(7);
        var trendCols = [trendCol0, trendCol1, trendCol2, trendCol3, trendCol4, trendCol5, trendCol6];
        var goalSecs = Math.max(60, bookManager.dailyGoalMinutes * 60);
        var weekTotalSecs = 0;
        var activeDays = 0;
        var maxDaySecs = 0;
        var maxDayName = "";

        for (var i = 0; i < 7; ++i) {
            if (i < trendDays.length) {
                var td = trendDays[i];
                trendCols[i].visible = true;
                trendCols[i].appTheme = bookManager.appTheme;
                trendCols[i].dateStr = td.dateStr;
                trendCols[i].dayName = td.dayName;
                trendCols[i].dayNum = td.dayNum;
                trendCols[i].durationText = (td.seconds > 0) ? (td.shortDurationStr ? td.shortDurationStr : td.durationStr) : "";
                trendCols[i].isToday = td.isToday;
                trendCols[i].metGoal = (td.seconds >= goalSecs);
                var minEmptyH = Math.max(3, sp(4));
                var h = (td.seconds > 0)
                        ? Math.max(minEmptyH + sp(6), Math.round(td.heightRatio * trendTrackHeight))
                        : minEmptyH;
                trendCols[i].barHeight = h;

                weekTotalSecs += td.seconds;
                if (td.seconds > 0) {
                    activeDays++;
                    if (td.seconds > maxDaySecs) {
                        maxDaySecs = td.seconds;
                        var bestShort = td.shortDurationStr ? td.shortDurationStr : td.durationStr;
                        maxDayName = td.dayName + " (" + bestShort + ")";
                    }
                }
            } else {
                trendCols[i].visible = false;
            }
        }

        trendTotalWeekLabel.text = (weekTotalSecs > 0) ? bookManager.formatShortDuration(weekTotalSecs) : bookManager.formatDuration(0);
        var avgSecs = activeDays > 0 ? Math.round(weekTotalSecs / activeDays) : 0;
        trendAvgDailyLabel.text = (avgSecs > 0) ? bookManager.formatShortDuration(avgSecs) : bookManager.formatDuration(0);
        trendBestDayLabel.text = (maxDaySecs > 0) ? maxDayName : qsTr("Chưa có");

        var dist = bookManager.getFormatDistribution();
        var anyActiveSeconds = (dist.epubSeconds + dist.pdfSeconds + dist.txtSeconds + dist.fb2Seconds + dist.mobiSeconds + dist.cbzSeconds) > 0;

        var fmtSpecs = [
            { row: epubDistRow, fill: epubBarFill, empty: epubBarEmpty, label: epubTimeLabel, name: "EPUB", secs: dist.epubSeconds, timeStr: bookManager.formatShortDuration(dist.epubSeconds), books: dist.epubBooks, pct: dist.epubPct },
            { row: pdfDistRow, fill: pdfBarFill, empty: pdfBarEmpty, label: pdfTimeLabel, name: "PDF / DjVu", secs: dist.pdfSeconds, timeStr: bookManager.formatShortDuration(dist.pdfSeconds), books: dist.pdfBooks, pct: dist.pdfPct },
            { row: txtDistRow, fill: txtBarFill, empty: txtBarEmpty, label: txtTimeLabel, name: "TXT", secs: dist.txtSeconds, timeStr: bookManager.formatShortDuration(dist.txtSeconds), books: dist.txtBooks, pct: dist.txtPct },
            { row: fb2DistRow, fill: fb2BarFill, empty: fb2BarEmpty, label: fb2TimeLabel, name: "FB2", secs: dist.fb2Seconds, timeStr: bookManager.formatShortDuration(dist.fb2Seconds), books: dist.fb2Books, pct: dist.fb2Pct },
            { row: mobiDistRow, fill: mobiBarFill, empty: mobiBarEmpty, label: mobiTimeLabel, name: "MOBI", secs: dist.mobiSeconds, timeStr: bookManager.formatShortDuration(dist.mobiSeconds), books: dist.mobiBooks, pct: dist.mobiPct },
            { row: cbzDistRow, fill: cbzBarFill, empty: cbzBarEmpty, label: cbzTimeLabel, name: "CBZ", secs: dist.cbzSeconds, timeStr: bookManager.formatShortDuration(dist.cbzSeconds), books: dist.cbzBooks, pct: dist.cbzPct }
        ];

        var shownCount = 0;
        var otherParts = [];
        for (var j = 0; j < fmtSpecs.length; ++j) {
            var fs = fmtSpecs[j];
            var showRow = anyActiveSeconds ? (fs.secs > 0) : (fs.books > 0);
            applyDistRow(fs.row, fs.fill, fs.empty, fs.label, fs.secs > 0 ? fs.timeStr : bookManager.formatDuration(0), fs.books, fs.pct, showRow, shownCount === 0 ? 0 : sp(10));
            if (showRow) {
                shownCount++;
            } else if (fs.books > 0) {
                otherParts.push(fs.name + " (" + fs.books + ")");
            }
        }

        hasAnyFormatData = (shownCount > 0 || otherParts.length > 0);
        otherFormatsDivider.visible = (shownCount > 0 && otherParts.length > 0);
        otherFormatsNoteLabel.visible = (otherParts.length > 0);
        if (otherParts.length > 0) {
            otherFormatsNoteLabel.text = qsTr("Thư viện còn:") + " " + otherParts.join(" • ");
        } else {
            otherFormatsNoteLabel.text = "";
        }
        noFormatDataLabel.visible = !hasAnyFormatData;
    }

    attachedObjects: [
        ComponentDefinition {
            id: bookSlotDef
            source: "asset:///ReadingBookSlotItem.qml"
        }
    ]

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

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create(bookManager.uiBg)

            ScrollView {
                id: statsScrollView
                scrollRole: ScrollRole.Main
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                scrollViewProperties.scrollMode: ScrollMode.Vertical

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: Color.create(bookManager.uiBg)
                    topPadding: statsSheet.sp(10)
                    bottomPadding: statsSheet.sp(24)
                    leftPadding: statsSheet.sp(14)
                    rightPadding: statsSheet.sp(14)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    // =========================================================
                    // 3 BALANCED TABS: [ Tổng quan ] [ Lịch đọc ] [ Theo sách ]
                    // =========================================================
                    SegmentedControl {
                        id: statsModeControl
                        horizontalAlignment: HorizontalAlignment.Fill
                        bottomMargin: statsSheet.sp(12)

                        Option {
                            id: optOverview
                            text: qsTr("Tổng quan") + Retranslate.onLanguageChanged
                            value: "overview"
                            selected: true
                        }
                        Option {
                            id: optCalendar
                            text: qsTr("Lịch đọc") + Retranslate.onLanguageChanged
                            value: "calendar"
                        }
                        Option {
                            id: optByBook
                            text: qsTr("Theo sách") + Retranslate.onLanguageChanged
                            value: "by_book"
                        }

                        onSelectedValueChanged: {
                            statsSheet.mainMode = selectedValue;
                            statsScrollView.resetViewableArea(ScrollAnimation.Default);
                            if (selectedValue === "overview") {
                                statsSheet.refreshOverallStats();
                                statsSheet.refreshTrendsStats();
                            } else if (selectedValue === "calendar") {
                                if (statsSheet.viewMode === "month") {
                                    statsSheet.refreshCalendarAndMonthStats();
                                    statsSheet.refreshSelectedDayDetails();
                                } else {
                                    statsSheet.refreshWeekView();
                                }
                            } else if (selectedValue === "by_book") {
                                statsSheet.refreshByBookStats();
                            }
                        }
                    }

                    // =========================================================
                    // TAB 1: TỔNG QUAN (GOAL + 7-DAY TREND + ALL-TIME + FORMATS)
                    // =========================================================
                    Container {
                        id: overviewModeContainer
                        visible: statsSheet.mainMode === "overview"
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        // 1. Today's Goal & Streak Card
                        Container {
                            bottomMargin: statsSheet.sp(14)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: statsSheet.sp(14)
                            bottomPadding: statsSheet.sp(14)
                            leftPadding: statsSheet.sp(14)
                            rightPadding: statsSheet.sp(14)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    text: qsTr("MỤC TIÊU HÔM NAY") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }

                                Label {
                                    verticalAlignment: VerticalAlignment.Center
                                    text: statsSheet.formatDays(bookManager.currentStreak) + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiAccent)
                                }
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                topMargin: statsSheet.sp(6)
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    text: (statsSheet.calcDisplayMins(bookManager.todayReadingSeconds) + " / " + bookManager.dailyGoalMinutes + " " + (bookManager.language === "en" ? "min" : qsTr("phút"))) + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiInk)
                                }

                                Label {
                                    verticalAlignment: VerticalAlignment.Center
                                    text: statsSheet.calcGoalPct(bookManager.todayReadingSeconds, bookManager.dailyGoalMinutes) + "%"
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiAccent)
                                }
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                topMargin: statsSheet.sp(8)
                                preferredHeight: statsSheet.sp(6)
                                background: Color.create(bookManager.uiHairline)
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    visible: bookManager.todayReadingSeconds > 0
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.create(bookManager.uiAccent)
                                    layoutProperties: StackLayoutProperties {
                                        spaceQuota: Math.max(0.001, statsSheet.calcGoalPct(bookManager.todayReadingSeconds, bookManager.dailyGoalMinutes))
                                    }
                                }
                                Container {
                                    visible: statsSheet.calcGoalPct(bookManager.todayReadingSeconds, bookManager.dailyGoalMinutes) < 100
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.Transparent
                                    layoutProperties: StackLayoutProperties {
                                        spaceQuota: Math.max(0.001, 100 - statsSheet.calcGoalPct(bookManager.todayReadingSeconds, bookManager.dailyGoalMinutes))
                                    }
                                }
                            }
                        }

                        // 2. 7-Day Reading Activity
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: statsSheet.sp(6)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Label {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                verticalAlignment: VerticalAlignment.Center
                                text: qsTr("7 NGÀY GẦN NHẤT") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiAccent)
                            }

                            Label {
                                verticalAlignment: VerticalAlignment.Center
                                text: qsTr("Mục tiêu: ") + bookManager.dailyGoalMinutes + " " + qsTr("phút/ngày") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }
                        }

                        Container {
                            bottomMargin: statsSheet.sp(14)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: statsSheet.sp(12)
                            bottomPadding: statsSheet.sp(12)
                            leftPadding: statsSheet.sp(12)
                            rightPadding: statsSheet.sp(12)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            // 3 Inline Metric Columns separated by 1px Hairline dividers
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Fill
                                    rightPadding: statsSheet.sp(6)
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                    Label {
                                        text: qsTr("TỔNG 7 NGÀY") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        autoSize.maxLineCount: 1
                                    }
                                    Label {
                                        id: trendTotalWeekLabel
                                        topMargin: statsSheet.sp(2)
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.BodyText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiAccent)
                                        multiline: true
                                        autoSize.maxLineCount: 2
                                    }
                                }

                                Container {
                                    preferredWidth: 1
                                    verticalAlignment: VerticalAlignment.Fill
                                    leftMargin: statsSheet.sp(4)
                                    rightMargin: statsSheet.sp(8)
                                    background: Color.create(bookManager.uiHairline)
                                }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Fill
                                    rightPadding: statsSheet.sp(6)
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                    Label {
                                        text: qsTr("TB / NGÀY") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        autoSize.maxLineCount: 1
                                    }
                                    Label {
                                        id: trendAvgDailyLabel
                                        topMargin: statsSheet.sp(2)
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.BodyText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiInk)
                                        multiline: true
                                        autoSize.maxLineCount: 2
                                    }
                                }

                                Container {
                                    preferredWidth: 1
                                    verticalAlignment: VerticalAlignment.Fill
                                    leftMargin: statsSheet.sp(4)
                                    rightMargin: statsSheet.sp(8)
                                    background: Color.create(bookManager.uiHairline)
                                }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                    Label {
                                        text: qsTr("CAO NHẤT") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        autoSize.maxLineCount: 1
                                    }
                                    Label {
                                        id: trendBestDayLabel
                                        topMargin: statsSheet.sp(2)
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.BodyText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiInk)
                                        multiline: true
                                        autoSize.maxLineCount: 2
                                    }
                                }
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 1
                                topMargin: statsSheet.sp(10)
                                bottomMargin: statsSheet.sp(10)
                                background: Color.create(bookManager.uiHairline)
                            }

                            // 7-Column Bar Chart (tap any column to inspect that day in Calendar)
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                ReadingTrendBarItem { id: trendCol0; uiScale: statsSheet.uiScale; trackHeight: statsSheet.trendTrackHeight; appTheme: bookManager.appTheme; onTapped: statsSheet.jumpToDateFromTrend(date) }
                                ReadingTrendBarItem { id: trendCol1; uiScale: statsSheet.uiScale; trackHeight: statsSheet.trendTrackHeight; appTheme: bookManager.appTheme; onTapped: statsSheet.jumpToDateFromTrend(date) }
                                ReadingTrendBarItem { id: trendCol2; uiScale: statsSheet.uiScale; trackHeight: statsSheet.trendTrackHeight; appTheme: bookManager.appTheme; onTapped: statsSheet.jumpToDateFromTrend(date) }
                                ReadingTrendBarItem { id: trendCol3; uiScale: statsSheet.uiScale; trackHeight: statsSheet.trendTrackHeight; appTheme: bookManager.appTheme; onTapped: statsSheet.jumpToDateFromTrend(date) }
                                ReadingTrendBarItem { id: trendCol4; uiScale: statsSheet.uiScale; trackHeight: statsSheet.trendTrackHeight; appTheme: bookManager.appTheme; onTapped: statsSheet.jumpToDateFromTrend(date) }
                                ReadingTrendBarItem { id: trendCol5; uiScale: statsSheet.uiScale; trackHeight: statsSheet.trendTrackHeight; appTheme: bookManager.appTheme; onTapped: statsSheet.jumpToDateFromTrend(date) }
                                ReadingTrendBarItem { id: trendCol6; uiScale: statsSheet.uiScale; trackHeight: statsSheet.trendTrackHeight; appTheme: bookManager.appTheme; onTapped: statsSheet.jumpToDateFromTrend(date) }
                            }
                        }

                        // 3. All-Time Summary & Records
                        Label {
                            bottomMargin: statsSheet.sp(6)
                            text: qsTr("TÍCH LŨY") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create(bookManager.uiAccent)
                        }

                        Container {
                            bottomMargin: statsSheet.sp(14)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: statsSheet.sp(14)
                            bottomPadding: statsSheet.sp(10)
                            leftPadding: statsSheet.sp(14)
                            rightPadding: statsSheet.sp(14)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            // Hero 2-Column All-Time Figures
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Fill
                                    rightPadding: statsSheet.sp(8)
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                    Label {
                                        text: qsTr("TỔNG THỜI GIAN ĐÃ ĐỌC") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        multiline: true
                                    }
                                    Label {
                                        id: allTimeTotalLabel
                                        topMargin: statsSheet.sp(4)
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.TitleText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiAccent)
                                        autoSize.maxLineCount: 1
                                    }
                                }

                                Container {
                                    preferredWidth: 1
                                    verticalAlignment: VerticalAlignment.Fill
                                    leftMargin: statsSheet.sp(4)
                                    rightMargin: statsSheet.sp(12)
                                    background: Color.create(bookManager.uiHairline)
                                }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                    Label {
                                        text: qsTr("TỔNG SỐ NGÀY ĐÃ ĐỌC") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        multiline: true
                                    }
                                    Label {
                                        id: allTimeDaysLabel
                                        topMargin: statsSheet.sp(4)
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.TitleText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiInk)
                                        autoSize.maxLineCount: 1
                                    }
                                }
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 1
                                topMargin: statsSheet.sp(12)
                                background: Color.create(bookManager.uiHairline)
                            }

                            // 4 Clean Key-Value Rows
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                topPadding: statsSheet.sp(9)
                                bottomPadding: statsSheet.sp(9)
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: statsSheet.sp(10)
                                    verticalAlignment: VerticalAlignment.Center
                                    text: qsTr("Chuỗi ngày đọc hiện tại") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    multiline: true
                                }
                                Label {
                                    id: allTimeCurrentStreakLabel
                                    verticalAlignment: VerticalAlignment.Center
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.textAlign: TextAlign.Right
                                    textStyle.color: Color.create(bookManager.uiAccent)
                                    autoSize.maxLineCount: 1
                                }
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 1
                                background: Color.create(bookManager.uiHairline)
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                topPadding: statsSheet.sp(9)
                                bottomPadding: statsSheet.sp(9)
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: statsSheet.sp(10)
                                    verticalAlignment: VerticalAlignment.Center
                                    text: qsTr("Kỷ lục chuỗi ngày dài nhất") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    multiline: true
                                }
                                Label {
                                    id: allTimeMaxStreakLabel
                                    verticalAlignment: VerticalAlignment.Center
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.textAlign: TextAlign.Right
                                    textStyle.color: Color.create(bookManager.uiAccent)
                                    autoSize.maxLineCount: 1
                                }
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 1
                                background: Color.create(bookManager.uiHairline)
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                topPadding: statsSheet.sp(9)
                                bottomPadding: statsSheet.sp(9)
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: statsSheet.sp(10)
                                    verticalAlignment: VerticalAlignment.Center
                                    text: qsTr("Kỷ lục ngày đọc nhiều nhất") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    multiline: true
                                }
                                Label {
                                    id: allTimeBestDayLabel
                                    verticalAlignment: VerticalAlignment.Center
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.textAlign: TextAlign.Right
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    multiline: true
                                    autoSize.maxLineCount: 2
                                }
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 1
                                background: Color.create(bookManager.uiHairline)
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                topPadding: statsSheet.sp(9)
                                bottomPadding: statsSheet.sp(4)
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: statsSheet.sp(10)
                                    verticalAlignment: VerticalAlignment.Center
                                    text: qsTr("Tổng kết tủ sách") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    multiline: true
                                }
                                Label {
                                    id: allTimeBooksLabel
                                    verticalAlignment: VerticalAlignment.Center
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.textAlign: TextAlign.Right
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    multiline: true
                                    autoSize.maxLineCount: 2
                                }
                            }
                        }

                        // 4. Active Format Distribution (hides 0-min formats into a single muted footnote)
                        Label {
                            bottomMargin: statsSheet.sp(6)
                            text: qsTr("THEO ĐỊNH DẠNG") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create(bookManager.uiAccent)
                        }

                        Container {
                            bottomMargin: statsSheet.sp(16)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: statsSheet.sp(14)
                            bottomPadding: statsSheet.sp(14)
                            leftPadding: statsSheet.sp(14)
                            rightPadding: statsSheet.sp(14)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            // EPUB
                            Container {
                                id: epubDistRow
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                    Label {
                                        text: "EPUB"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiAccent)
                                    }
                                    Label {
                                        id: epubTimeLabel
                                        leftMargin: statsSheet.sp(8)
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        textStyle.textAlign: TextAlign.Right
                                        autoSize.maxLineCount: 1
                                    }
                                }
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topMargin: statsSheet.sp(5)
                                    preferredHeight: statsSheet.sp(5)
                                    background: Color.create(bookManager.uiHairline)
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                    Container { id: epubBarFill; verticalAlignment: VerticalAlignment.Fill; background: Color.create(bookManager.uiAccent); layoutProperties: StackLayoutProperties { spaceQuota: 1 } }
                                    Container { id: epubBarEmpty; verticalAlignment: VerticalAlignment.Fill; background: Color.Transparent; layoutProperties: StackLayoutProperties { spaceQuota: 99 } }
                                }
                            }

                            // PDF / DjVu
                            Container {
                                id: pdfDistRow
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                    Label {
                                        text: "PDF / DjVu"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#C99A4E") : Color.create("#A6752B")
                                    }
                                    Label {
                                        id: pdfTimeLabel
                                        leftMargin: statsSheet.sp(8)
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        textStyle.textAlign: TextAlign.Right
                                        autoSize.maxLineCount: 1
                                    }
                                }
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topMargin: statsSheet.sp(5)
                                    preferredHeight: statsSheet.sp(5)
                                    background: Color.create(bookManager.uiHairline)
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                    Container { id: pdfBarFill; verticalAlignment: VerticalAlignment.Fill; background: bookManager.appTheme === "dark" ? Color.create("#C99A4E") : Color.create("#A6752B"); layoutProperties: StackLayoutProperties { spaceQuota: 1 } }
                                    Container { id: pdfBarEmpty; verticalAlignment: VerticalAlignment.Fill; background: Color.Transparent; layoutProperties: StackLayoutProperties { spaceQuota: 99 } }
                                }
                            }

                            // TXT
                            Container {
                                id: txtDistRow
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                    Label {
                                        text: "TXT"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#A89F91") : Color.create("#756B5E")
                                    }
                                    Label {
                                        id: txtTimeLabel
                                        leftMargin: statsSheet.sp(8)
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        textStyle.textAlign: TextAlign.Right
                                        autoSize.maxLineCount: 1
                                    }
                                }
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topMargin: statsSheet.sp(5)
                                    preferredHeight: statsSheet.sp(5)
                                    background: Color.create(bookManager.uiHairline)
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                    Container { id: txtBarFill; verticalAlignment: VerticalAlignment.Fill; background: bookManager.appTheme === "dark" ? Color.create("#A89F91") : Color.create("#756B5E"); layoutProperties: StackLayoutProperties { spaceQuota: 1 } }
                                    Container { id: txtBarEmpty; verticalAlignment: VerticalAlignment.Fill; background: Color.Transparent; layoutProperties: StackLayoutProperties { spaceQuota: 99 } }
                                }
                            }

                            // FB2
                            Container {
                                id: fb2DistRow
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                    Label {
                                        text: "FB2"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8E806D") : Color.create("#6E6150")
                                    }
                                    Label {
                                        id: fb2TimeLabel
                                        leftMargin: statsSheet.sp(8)
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        textStyle.textAlign: TextAlign.Right
                                        autoSize.maxLineCount: 1
                                    }
                                }
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topMargin: statsSheet.sp(5)
                                    preferredHeight: statsSheet.sp(5)
                                    background: Color.create(bookManager.uiHairline)
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                    Container { id: fb2BarFill; verticalAlignment: VerticalAlignment.Fill; background: bookManager.appTheme === "dark" ? Color.create("#8E806D") : Color.create("#6E6150"); layoutProperties: StackLayoutProperties { spaceQuota: 1 } }
                                    Container { id: fb2BarEmpty; verticalAlignment: VerticalAlignment.Fill; background: Color.Transparent; layoutProperties: StackLayoutProperties { spaceQuota: 99 } }
                                }
                            }

                            // MOBI / AZW3
                            Container {
                                id: mobiDistRow
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                    Label {
                                        text: "MOBI / AZW3"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#B8864D") : Color.create("#8E5F2D")
                                    }
                                    Label {
                                        id: mobiTimeLabel
                                        leftMargin: statsSheet.sp(8)
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        textStyle.textAlign: TextAlign.Right
                                        autoSize.maxLineCount: 1
                                    }
                                }
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topMargin: statsSheet.sp(5)
                                    preferredHeight: statsSheet.sp(5)
                                    background: Color.create(bookManager.uiHairline)
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                    Container { id: mobiBarFill; verticalAlignment: VerticalAlignment.Fill; background: bookManager.appTheme === "dark" ? Color.create("#B8864D") : Color.create("#8E5F2D"); layoutProperties: StackLayoutProperties { spaceQuota: 1 } }
                                    Container { id: mobiBarEmpty; verticalAlignment: VerticalAlignment.Fill; background: Color.Transparent; layoutProperties: StackLayoutProperties { spaceQuota: 99 } }
                                }
                            }

                            // CBZ
                            Container {
                                id: cbzDistRow
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                    Label {
                                        text: "CBZ"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#8C7355") : Color.create("#6D5539")
                                    }
                                    Label {
                                        id: cbzTimeLabel
                                        leftMargin: statsSheet.sp(8)
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        textStyle.textAlign: TextAlign.Right
                                        autoSize.maxLineCount: 1
                                    }
                                }
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topMargin: statsSheet.sp(5)
                                    preferredHeight: statsSheet.sp(5)
                                    background: Color.create(bookManager.uiHairline)
                                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                    Container { id: cbzBarFill; verticalAlignment: VerticalAlignment.Fill; background: bookManager.appTheme === "dark" ? Color.create("#8C7355") : Color.create("#6D5539"); layoutProperties: StackLayoutProperties { spaceQuota: 1 } }
                                    Container { id: cbzBarEmpty; verticalAlignment: VerticalAlignment.Fill; background: Color.Transparent; layoutProperties: StackLayoutProperties { spaceQuota: 99 } }
                                }
                            }

                            Container {
                                id: otherFormatsDivider
                                visible: false
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 1
                                topMargin: statsSheet.sp(10)
                                bottomMargin: statsSheet.sp(8)
                                background: Color.create(bookManager.uiHairline)
                            }

                            Label {
                                id: otherFormatsNoteLabel
                                visible: false
                                horizontalAlignment: HorizontalAlignment.Fill
                                text: ""
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                                multiline: true
                            }

                            Label {
                                id: noFormatDataLabel
                                visible: false
                                horizontalAlignment: HorizontalAlignment.Center
                                text: qsTr("Chưa có dữ liệu đọc sách") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }
                        }
                    } // End of overviewModeContainer

                    // =========================================================
                    // TAB 2: LỊCH ĐỌC (CALENDAR VIEW - CLEAN, ZERO DUPLICATE CHARTS)
                    // =========================================================
                    Container {
                        id: calendarModeContainer
                        visible: statsSheet.mainMode === "calendar"
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        // 1. Unified Period Control Bar ([Tháng | Tuần] + ‹ Title ›)
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: statsSheet.sp(10)
                            background: Color.create(bookManager.uiCard)
                            topPadding: statsSheet.sp(6)
                            bottomPadding: statsSheet.sp(6)
                            leftPadding: statsSheet.sp(8)
                            rightPadding: statsSheet.sp(8)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            // Month | Week Switcher Pill
                            Container {
                                verticalAlignment: VerticalAlignment.Center
                                background: Color.create(bookManager.uiSurface)
                                topPadding: statsSheet.sp(2); bottomPadding: statsSheet.sp(2)
                                leftPadding: statsSheet.sp(2); rightPadding: statsSheet.sp(2)
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    verticalAlignment: VerticalAlignment.Center
                                    leftPadding: statsSheet.sp(10)
                                    rightPadding: statsSheet.sp(10)
                                    topPadding: statsSheet.sp(5)
                                    bottomPadding: statsSheet.sp(5)
                                    background: statsSheet.viewMode === "month" ?
                                                Color.create(bookManager.uiAccent) :
                                                Color.Transparent
                                    gestureHandlers: [
                                        TapHandler {
                                            onTapped: {
                                                statsSheet.viewMode = "month";
                                                statsSheet.refreshCalendarAndMonthStats();
                                                statsSheet.refreshSelectedDayDetails();
                                            }
                                        }
                                    ]
                                    Label {
                                        text: qsTr("Tháng") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: statsSheet.viewMode === "month" ?
                                                         (bookManager.appTheme === "dark" ? Color.create("#0D0B09") : Color.White) :
                                                         Color.create(bookManager.uiMuted)
                                    }
                                }

                                Container {
                                    verticalAlignment: VerticalAlignment.Center
                                    leftPadding: statsSheet.sp(10)
                                    rightPadding: statsSheet.sp(10)
                                    topPadding: statsSheet.sp(5)
                                    bottomPadding: statsSheet.sp(5)
                                    background: statsSheet.viewMode === "week" ?
                                                Color.create(bookManager.uiAccent) :
                                                Color.Transparent
                                    gestureHandlers: [
                                        TapHandler {
                                            onTapped: {
                                                statsSheet.viewMode = "week";
                                                if (!statsSheet.curWeekAnchorDate || statsSheet.curWeekAnchorDate === "") {
                                                    statsSheet.curWeekAnchorDate = statsSheet.selectedDate;
                                                }
                                                statsSheet.refreshWeekView();
                                            }
                                        }
                                    ]
                                    Label {
                                        text: qsTr("Tuần") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: statsSheet.viewMode === "week" ?
                                                         (bookManager.appTheme === "dark" ? Color.create("#0D0B09") : Color.White) :
                                                         Color.create(bookManager.uiMuted)
                                    }
                                }
                            }

                            // Prev Chevron
                            Container {
                                leftMargin: statsSheet.sp(8)
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: statsSheet.sp(38)
                                preferredHeight: statsSheet.sp(34)
                                background: Color.create(bookManager.uiSurface)
                                layout: DockLayout {}
                                gestureHandlers: [
                                    TapHandler {
                                        onTapped: {
                                            if (statsSheet.viewMode === "month") {
                                                statsSheet.prevMonth();
                                            } else {
                                                statsSheet.prevWeek();
                                            }
                                        }
                                    }
                                ]
                                Label {
                                    horizontalAlignment: HorizontalAlignment.Center
                                    verticalAlignment: VerticalAlignment.Center
                                    text: "‹"
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiInk)
                                }
                            }

                            // Dynamic Month or Week Title
                            Label {
                                id: calendarPeriodTitleLabel
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                verticalAlignment: VerticalAlignment.Center
                                horizontalAlignment: HorizontalAlignment.Center
                                leftMargin: statsSheet.sp(6)
                                rightMargin: statsSheet.sp(6)
                                text: statsSheet.viewMode === "month" ? statsSheet.monthTitleStr : statsSheet.weekTitleStr
                                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.textAlign: TextAlign.Center
                                textStyle.color: Color.create(bookManager.uiInk)
                                multiline: true
                                autoSize.maxLineCount: 2
                            }

                            // Next Chevron
                            Container {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: statsSheet.sp(38)
                                preferredHeight: statsSheet.sp(34)
                                background: Color.create(bookManager.uiSurface)
                                layout: DockLayout {}
                                gestureHandlers: [
                                    TapHandler {
                                        onTapped: {
                                            if (statsSheet.viewMode === "month") {
                                                statsSheet.nextMonth();
                                            } else {
                                                statsSheet.nextWeek();
                                            }
                                        }
                                    }
                                ]
                                Label {
                                    horizontalAlignment: HorizontalAlignment.Center
                                    verticalAlignment: VerticalAlignment.Center
                                    text: "›"
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiInk)
                                }
                            }
                        }

                        // 2A. Unified Month Summary Strip (1 Card, 3 Clean Columns)
                        Container {
                            visible: statsSheet.viewMode === "month"
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: statsSheet.sp(10)
                            background: Color.create(bookManager.uiCard)
                            topPadding: statsSheet.sp(10)
                            bottomPadding: statsSheet.sp(10)
                            leftPadding: statsSheet.sp(12)
                            rightPadding: statsSheet.sp(12)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightPadding: statsSheet.sp(6)
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                Label {
                                    text: qsTr("TỔNG THÁNG") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    topMargin: statsSheet.sp(2)
                                    text: statsSheet.monthTotalShortStr
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiAccent)
                                    autoSize.maxLineCount: 1
                                }
                            }

                            Container {
                                preferredWidth: 1
                                verticalAlignment: VerticalAlignment.Fill
                                leftMargin: statsSheet.sp(4)
                                rightMargin: statsSheet.sp(10)
                                background: Color.create(bookManager.uiHairline)
                            }

                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightPadding: statsSheet.sp(6)
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                Label {
                                    text: qsTr("NGÀY ĐỌC") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    topMargin: statsSheet.sp(2)
                                    text: statsSheet.monthDaysValStr
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    autoSize.maxLineCount: 1
                                }
                            }

                            Container {
                                preferredWidth: 1
                                verticalAlignment: VerticalAlignment.Fill
                                leftMargin: statsSheet.sp(4)
                                rightMargin: statsSheet.sp(10)
                                background: Color.create(bookManager.uiHairline)
                            }

                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                Label {
                                    text: qsTr("TB / NGÀY") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    topMargin: statsSheet.sp(2)
                                    text: statsSheet.monthAvgShortStr
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiAccent)
                                    autoSize.maxLineCount: 1
                                }
                            }
                        }

                        // 2B. Unified Week Summary Strip (1 Card, 3 Clean Columns)
                        Container {
                            visible: statsSheet.viewMode === "week"
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: statsSheet.sp(12)
                            background: Color.create(bookManager.uiCard)
                            topPadding: statsSheet.sp(10)
                            bottomPadding: statsSheet.sp(10)
                            leftPadding: statsSheet.sp(12)
                            rightPadding: statsSheet.sp(12)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightPadding: statsSheet.sp(6)
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                Label {
                                    text: qsTr("TỔNG TUẦN") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    id: weekTotalTimeLabel
                                    topMargin: statsSheet.sp(2)
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiAccent)
                                    autoSize.maxLineCount: 1
                                }
                            }

                            Container {
                                preferredWidth: 1
                                verticalAlignment: VerticalAlignment.Fill
                                leftMargin: statsSheet.sp(4)
                                rightMargin: statsSheet.sp(10)
                                background: Color.create(bookManager.uiHairline)
                            }

                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightPadding: statsSheet.sp(6)
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                Label {
                                    text: qsTr("NGÀY ĐỌC") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    id: weekReadingDaysLabel
                                    topMargin: statsSheet.sp(2)
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    autoSize.maxLineCount: 1
                                }
                            }

                            Container {
                                preferredWidth: 1
                                verticalAlignment: VerticalAlignment.Fill
                                leftMargin: statsSheet.sp(4)
                                rightMargin: statsSheet.sp(10)
                                background: Color.create(bookManager.uiHairline)
                            }

                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                Label {
                                    text: qsTr("TB / NGÀY") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                    autoSize.maxLineCount: 1
                                }
                                Label {
                                    id: weekAvgDailyTimeLabel
                                    topMargin: statsSheet.sp(2)
                                    text: "--"
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiAccent)
                                    autoSize.maxLineCount: 1
                                }
                            }
                        }

                        // 3. 7-Column Monthly Calendar Grid
                        Container {
                            visible: statsSheet.viewMode === "month"
                            bottomMargin: statsSheet.sp(10)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: statsSheet.sp(8)
                            bottomPadding: statsSheet.sp(8)
                            leftPadding: statsSheet.sp(6)
                            rightPadding: statsSheet.sp(6)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            // Weekday Header Row
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                bottomMargin: statsSheet.sp(4)
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("T2") + Retranslate.onLanguageChanged
                                    textStyle.textAlign: TextAlign.Center
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("T3") + Retranslate.onLanguageChanged
                                    textStyle.textAlign: TextAlign.Center
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("T4") + Retranslate.onLanguageChanged
                                    textStyle.textAlign: TextAlign.Center
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("T5") + Retranslate.onLanguageChanged
                                    textStyle.textAlign: TextAlign.Center
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("T6") + Retranslate.onLanguageChanged
                                    textStyle.textAlign: TextAlign.Center
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("T7") + Retranslate.onLanguageChanged
                                    textStyle.textAlign: TextAlign.Center
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("CN") + Retranslate.onLanguageChanged
                                    textStyle.textAlign: TextAlign.Center
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#E07A5F") : Color.create("#B84323")
                                }
                            }

                            // Week Row 0
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: statsSheet.calendarCellHeight
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                CalendarDayCell { id: c0; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c1; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c2; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c3; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c4; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c5; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c6; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                            }

                            // Week Row 1
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: statsSheet.calendarCellHeight
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                CalendarDayCell { id: c7; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c8; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c9; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c10; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c11; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c12; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c13; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                            }

                            // Week Row 2
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: statsSheet.calendarCellHeight
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                CalendarDayCell { id: c14; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c15; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c16; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c17; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c18; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c19; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c20; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                            }

                            // Week Row 3
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: statsSheet.calendarCellHeight
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                CalendarDayCell { id: c21; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c22; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c23; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c24; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c25; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c26; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c27; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                            }

                            // Week Row 4
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: statsSheet.calendarCellHeight
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                CalendarDayCell { id: c28; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c29; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c30; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c31; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c32; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c33; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c34; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                            }

                            // Week Row 5 (auto-hidden when month fits in 5 rows)
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                visible: statsSheet.row5HasCurrentMonth
                                preferredHeight: statsSheet.calendarCellHeight
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                CalendarDayCell { id: c35; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c36; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c37; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c38; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c39; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c40; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                                CalendarDayCell { id: c41; uiScale: statsSheet.uiScale; cellHeight: statsSheet.calendarCellHeight; onTapped: statsSheet.selectDay(date) }
                            }

                            // Heatmap Legend
                            Container {
                                topMargin: statsSheet.sp(8)
                                horizontalAlignment: HorizontalAlignment.Center
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    verticalAlignment: VerticalAlignment.Center
                                    text: qsTr("Ít hơn") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }

                                Container {
                                    leftMargin: statsSheet.sp(6)
                                    verticalAlignment: VerticalAlignment.Center
                                    preferredWidth: statsSheet.sp(12)
                                    preferredHeight: statsSheet.sp(12)
                                    background: Color.create(bookManager.uiSurface)
                                }
                                Container {
                                    leftMargin: statsSheet.sp(4)
                                    verticalAlignment: VerticalAlignment.Center
                                    preferredWidth: statsSheet.sp(12)
                                    preferredHeight: statsSheet.sp(12)
                                    background: bookManager.appTheme === "dark" ? Color.create("#231C11") : Color.create("#FAF2E1")
                                }
                                Container {
                                    leftMargin: statsSheet.sp(4)
                                    verticalAlignment: VerticalAlignment.Center
                                    preferredWidth: statsSheet.sp(12)
                                    preferredHeight: statsSheet.sp(12)
                                    background: bookManager.appTheme === "dark" ? Color.create("#3B2C16") : Color.create("#F2DFBA")
                                }
                                Container {
                                    leftMargin: statsSheet.sp(4)
                                    verticalAlignment: VerticalAlignment.Center
                                    preferredWidth: statsSheet.sp(12)
                                    preferredHeight: statsSheet.sp(12)
                                    background: bookManager.appTheme === "dark" ? Color.create("#5C431C") : Color.create("#E6C58D")
                                }
                                Container {
                                    leftMargin: statsSheet.sp(4)
                                    verticalAlignment: VerticalAlignment.Center
                                    preferredWidth: statsSheet.sp(12)
                                    preferredHeight: statsSheet.sp(12)
                                    background: bookManager.appTheme === "dark" ? Color.create("#855E24") : Color.create("#D9AB60")
                                }

                                Label {
                                    leftMargin: statsSheet.sp(6)
                                    verticalAlignment: VerticalAlignment.Center
                                    text: qsTr("Nhiều hơn") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                            }
                        }

                        // 4. Week Activity List (Week mode)
                        Label {
                            visible: statsSheet.viewMode === "week"
                            bottomMargin: statsSheet.sp(6)
                            text: qsTr("HOẠT ĐỘNG TUẦN") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create(bookManager.uiAccent)
                        }

                        Container {
                            visible: statsSheet.viewMode === "week"
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: statsSheet.sp(12)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            CalendarWeekDayItem { id: weekDay0; uiScale: statsSheet.uiScale; appTheme: bookManager.appTheme; onDaySelected: statsSheet.selectDay(date); onBookOpened: statsSheet.openBookFromStats(path) }
                            CalendarWeekDayItem { id: weekDay1; uiScale: statsSheet.uiScale; appTheme: bookManager.appTheme; onDaySelected: statsSheet.selectDay(date); onBookOpened: statsSheet.openBookFromStats(path) }
                            CalendarWeekDayItem { id: weekDay2; uiScale: statsSheet.uiScale; appTheme: bookManager.appTheme; onDaySelected: statsSheet.selectDay(date); onBookOpened: statsSheet.openBookFromStats(path) }
                            CalendarWeekDayItem { id: weekDay3; uiScale: statsSheet.uiScale; appTheme: bookManager.appTheme; onDaySelected: statsSheet.selectDay(date); onBookOpened: statsSheet.openBookFromStats(path) }
                            CalendarWeekDayItem { id: weekDay4; uiScale: statsSheet.uiScale; appTheme: bookManager.appTheme; onDaySelected: statsSheet.selectDay(date); onBookOpened: statsSheet.openBookFromStats(path) }
                            CalendarWeekDayItem { id: weekDay5; uiScale: statsSheet.uiScale; appTheme: bookManager.appTheme; onDaySelected: statsSheet.selectDay(date); onBookOpened: statsSheet.openBookFromStats(path) }
                            CalendarWeekDayItem { id: weekDay6; uiScale: statsSheet.uiScale; appTheme: bookManager.appTheme; onDaySelected: statsSheet.selectDay(date); onBookOpened: statsSheet.openBookFromStats(path) }
                        }

                        // 5. Selected Day Detail Card (Month mode only)
                        Container {
                            visible: statsSheet.viewMode === "month"
                            bottomMargin: statsSheet.sp(14)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: statsSheet.sp(12)
                            bottomPadding: statsSheet.sp(12)
                            leftPadding: statsSheet.sp(12)
                            rightPadding: statsSheet.sp(12)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    id: selectedDayTitleLabel
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    rightMargin: statsSheet.sp(8)
                                    text: ""
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create(bookManager.uiInk)
                                }

                                Label {
                                    id: selectedDayTotalLabel
                                    verticalAlignment: VerticalAlignment.Center
                                    text: qsTr("Chưa có thời gian đọc") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.textAlign: TextAlign.Right
                                    textStyle.color: statsSheet.hasSelectedDayReading ?
                                                     Color.create(bookManager.uiAccent) :
                                                     Color.create(bookManager.uiMuted)
                                }
                            }

                            Container {
                                id: dayDetailDivider
                                visible: false
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 1
                                topMargin: statsSheet.sp(8)
                                bottomMargin: statsSheet.sp(4)
                                background: Color.create(bookManager.uiHairline)
                            }

                            Label {
                                id: noDayReadingLabel
                                visible: true
                                topMargin: statsSheet.sp(4)
                                text: qsTr("Không có hoạt động đọc sách trong ngày này.") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                                multiline: true
                            }

                            Container {
                                id: dayBooksListContainer
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                            }
                        }
                    } // End of calendarModeContainer

                    // =========================================================
                    // TAB 3: THEO SÁCH (BY BOOK VIEW)
                    // =========================================================
                    Container {
                        id: byBookModeContainer
                        visible: statsSheet.mainMode === "by_book"
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Label {
                            visible: statsSheet.hasAnyBookStats
                            bottomMargin: statsSheet.sp(6)
                            text: qsTr("SÁCH ĐANG CHỌN") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create(bookManager.uiAccent)
                        }

                        // Hero Card for Selected Book
                        Container {
                            id: byBookHeroContainer
                            visible: statsSheet.hasAnyBookStats
                            bottomMargin: statsSheet.sp(14)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: statsSheet.sp(14)
                            bottomPadding: statsSheet.sp(14)
                            leftPadding: statsSheet.sp(14)
                            rightPadding: statsSheet.sp(14)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            // Book Header: Cover + Title + Format + Author + Progress
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    preferredWidth: statsSheet.sp(68)
                                    preferredHeight: statsSheet.sp(96)
                                    minWidth: statsSheet.sp(68)
                                    minHeight: statsSheet.sp(96)
                                    verticalAlignment: VerticalAlignment.Center
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}

                                    ImageView {
                                        id: byBookCover
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        scalingMethod: ScalingMethod.AspectFill
                                        imageSource: "asset:///icons/default_cover.png"
                                    }

                                    Container {
                                        visible: ("" + byBookCover.imageSource).indexOf("default_cover") !== -1
                                        horizontalAlignment: HorizontalAlignment.Center
                                        verticalAlignment: VerticalAlignment.Center
                                        leftPadding: statsSheet.sp(6)
                                        rightPadding: statsSheet.sp(6)
                                        bottomPadding: statsSheet.sp(10)

                                        Label {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            verticalAlignment: VerticalAlignment.Center
                                            text: byBookTitle.text
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.textAlign: TextAlign.Center
                                            textStyle.color: Color.White
                                            multiline: true
                                            autoSize.maxLineCount: 3
                                        }
                                    }
                                }

                                Container {
                                    leftMargin: statsSheet.sp(12)
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Center
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                        Label {
                                            id: byBookTitle
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            rightMargin: statsSheet.sp(8)
                                            verticalAlignment: VerticalAlignment.Center
                                            textStyle.base: SystemDefaults.TextStyles.BodyText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: Color.create(bookManager.uiInk)
                                            multiline: true
                                            autoSize.maxLineCount: 2
                                        }

                                        Container {
                                            verticalAlignment: VerticalAlignment.Center
                                            background: Color.create(bookManager.uiAccent)
                                            leftPadding: statsSheet.sp(7)
                                            rightPadding: statsSheet.sp(7)
                                            topPadding: statsSheet.sp(2)
                                            bottomPadding: statsSheet.sp(2)

                                            Label {
                                                id: byBookFormatBadge
                                                text: "EPUB"
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: Color.create("#0D0B09")
                                            }
                                        }
                                    }

                                    Label {
                                        id: byBookAuthor
                                        topMargin: statsSheet.sp(4)
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        autoSize.maxLineCount: 1
                                    }

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        topMargin: statsSheet.sp(8)
                                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                        Container {
                                            verticalAlignment: VerticalAlignment.Center
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            preferredHeight: statsSheet.sp(6)
                                            background: Color.create(bookManager.uiHairline)
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Container {
                                                id: byBookProgressBar
                                                verticalAlignment: VerticalAlignment.Fill
                                                background: Color.create(bookManager.uiAccent)
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
                                            leftMargin: statsSheet.sp(8)
                                            verticalAlignment: VerticalAlignment.Center
                                            text: "0%"
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: Color.create(bookManager.uiAccent)
                                        }
                                    }
                                }
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 1
                                topMargin: statsSheet.sp(12)
                                bottomMargin: statsSheet.sp(10)
                                background: Color.create(bookManager.uiHairline)
                            }

                            // Clean 2x2 Typographic Metric Table (1px Hairline dividers, no nested grey cages)
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Fill
                                    rightPadding: statsSheet.sp(10)
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Label {
                                        text: qsTr("THỜI GIAN ĐÃ ĐỌC") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        multiline: true
                                    }
                                    Label {
                                        id: byBookTimeSpent
                                        topMargin: statsSheet.sp(2)
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.BodyText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiAccent)
                                        multiline: true
                                        autoSize.maxLineCount: 2
                                    }
                                }

                                Container {
                                    preferredWidth: 1
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.create(bookManager.uiHairline)
                                }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Fill
                                    leftPadding: statsSheet.sp(10)
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Label {
                                        text: qsTr("ƯỚC TÍNH CÒN LẠI") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        multiline: true
                                    }
                                    Label {
                                        id: byBookRemaining
                                        topMargin: statsSheet.sp(2)
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.BodyText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiInk)
                                        multiline: true
                                        autoSize.maxLineCount: 2
                                    }
                                }
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 1
                                topMargin: statsSheet.sp(10)
                                bottomMargin: statsSheet.sp(10)
                                background: Color.create(bookManager.uiHairline)
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Fill
                                    rightPadding: statsSheet.sp(10)
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Label {
                                        text: qsTr("TỐC ĐỘ ĐỌC") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        multiline: true
                                    }
                                    Label {
                                        id: byBookSpeed
                                        topMargin: statsSheet.sp(2)
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.BodyText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiInk)
                                        multiline: true
                                        autoSize.maxLineCount: 2
                                    }
                                }

                                Container {
                                    preferredWidth: 1
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: Color.create(bookManager.uiHairline)
                                }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    verticalAlignment: VerticalAlignment.Fill
                                    leftPadding: statsSheet.sp(10)
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Label {
                                        text: (statsSheet.selectedBookIsPaged ? qsTr("TRANG ĐÃ ĐỌC") : qsTr("TIẾN ĐỘ CHƯƠNG")) + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        multiline: true
                                    }
                                    Label {
                                        id: byBookPages
                                        topMargin: statsSheet.sp(2)
                                        text: "--"
                                        textStyle.base: SystemDefaults.TextStyles.BodyText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: Color.create(bookManager.uiInk)
                                        multiline: true
                                        autoSize.maxLineCount: 2
                                    }
                                }
                            }

                            Button {
                                horizontalAlignment: HorizontalAlignment.Fill
                                topMargin: statsSheet.sp(14)
                                text: qsTr("Đọc tiếp cuốn sách này") + Retranslate.onLanguageChanged
                                appearance: ControlAppearance.Primary
                                onClicked: {
                                    statsSheet.openBookFromStats(statsSheet.selectedBookPath);
                                }
                            }
                        }

                        // Empty state container
                        Container {
                            id: byBookNoDataContainer
                            visible: !statsSheet.hasAnyBookStats
                            topMargin: statsSheet.sp(6)
                            bottomMargin: statsSheet.sp(14)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: statsSheet.sp(24)
                            bottomPadding: statsSheet.sp(24)
                            leftPadding: statsSheet.sp(16)
                            rightPadding: statsSheet.sp(16)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            Label {
                                horizontalAlignment: HorizontalAlignment.Center
                                text: qsTr("Chưa có dữ liệu sách đọc. Hãy mở một cuốn sách để bắt đầu ghi nhận thống kê!") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.textAlign: TextAlign.Center
                                textStyle.color: Color.create(bookManager.uiMuted)
                                multiline: true
                            }
                        }

                        Label {
                            visible: statsSheet.hasAnyBookStats && statsSheet.trackedBooksCount > 1
                            bottomMargin: statsSheet.sp(4)
                            text: qsTr("TẤT CẢ SÁCH TRONG THỐNG KÊ") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create(bookManager.uiAccent)
                        }

                        Container {
                            id: allBooksListContainer
                            visible: statsSheet.hasAnyBookStats && statsSheet.trackedBooksCount > 1
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                        }
                    } // End of byBookModeContainer
                }
            }
        }
    }
}
