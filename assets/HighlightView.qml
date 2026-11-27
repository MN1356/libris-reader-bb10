import bb.cascades 1.3
import bb.system 1.2

Container {
    id: highlightViewRoot

    property bool showCurrentBookOnly: true
    property bool showInlineActions: true
    property bool manualFilterSelection: false
    property bool isSearching: false
    property string filterQuery: ""
    property int displayCount: 0
    signal jumpToHighlightTriggered(string filePath, int chapterIndex, string highlightId)
    signal jumpToBookmarkTriggered(string filePath, int chapterIndex, string bookmarkId)

    function toggleScope() {
        manualFilterSelection = true;
        showCurrentBookOnly = !showCurrentBookOnly;
        reloadModel();
    }

    function toggleSearch() {
        isSearching = !isSearching;
        if (isSearching) {
            if (searchInput) searchInput.requestFocus();
        } else {
            filterQuery = "";
            if (searchInput) searchInput.text = "";
            reloadModel();
        }
    }

    function exportMarkdown() {
        var exportedPath = highlightViewRoot.showCurrentBookOnly ?
                           bookManager.exportBookHighlightsToMarkdown() :
                           bookManager.exportAllHighlightsToMarkdown();
        if (exportedPath && exportedPath !== "") {
            var fileName = exportedPath.indexOf('/') !== -1 ? exportedPath.substr(exportedPath.lastIndexOf('/') + 1) : exportedPath;
            exportToast.body = (qsTr("Đã lưu vào Documents: ") + fileName) + Retranslate.onLanguageChanged;
        } else {
            exportToast.body = qsTr("Không có ghi chú nào để xuất.") + Retranslate.onLanguageChanged;
        }
        exportToast.show();
    }

    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    background: Color.create(bookManager.uiBg)

    // 1. COLLAPSIBLE LIVE SEARCH BAR
    Container {
        id: searchInputContainer
        visible: highlightViewRoot.isSearching
        horizontalAlignment: HorizontalAlignment.Fill
        background: Color.create(bookManager.uiCard)
        topPadding: bookManager.dp(10)
        bottomPadding: bookManager.dp(10)
        leftPadding: bookManager.dp(14)
        rightPadding: bookManager.dp(14)
        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

        ImageView {
            preferredWidth: bookManager.dp(24)
            preferredHeight: bookManager.dp(24)
            verticalAlignment: VerticalAlignment.Center
            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_search.png" : "asset:///icons/ic_search_dark.png"
        }

        TextField {
            id: searchInput
            leftMargin: bookManager.dp(10)
            verticalAlignment: VerticalAlignment.Center
            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
            hintText: qsTr("Lọc dấu trang hoặc ghi chú...") + Retranslate.onLanguageChanged
            clearButtonVisible: true
            onTextChanging: {
                highlightViewRoot.filterQuery = text.trim().toLowerCase();
                reloadModel();
            }
        }
    }

    Container {
        visible: highlightViewRoot.isSearching
        horizontalAlignment: HorizontalAlignment.Fill
        preferredHeight: 1
        background: Color.create(bookManager.uiHairline)
    }

    // 2. SEGMENTED TYPE SELECTOR + COMPACT SCOPE BAR
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        background: Color.create(bookManager.uiCard)
        topPadding: bookManager.dp(10)
        bottomPadding: bookManager.dp(10)
        leftPadding: bookManager.dp(14)
        rightPadding: bookManager.dp(14)
        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

        SegmentedControl {
            id: typeSegmentedControl
            horizontalAlignment: HorizontalAlignment.Fill

            Option {
                id: optAll
                text: qsTr("Tất cả") + Retranslate.onLanguageChanged
                value: "all"
                selected: true
            }
            Option {
                id: optBookmarks
                text: qsTr("Dấu trang") + Retranslate.onLanguageChanged
                value: "bookmarks"
            }
            Option {
                id: optHighlights
                text: qsTr("Ghi chú") + Retranslate.onLanguageChanged
                value: "highlights"
            }

            onSelectedIndexChanged: {
                reloadModel();
            }
        }

        Container {
            topMargin: bookManager.dp(8)
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            Label {
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                text: {
                    var bmCount = highlightViewRoot.showCurrentBookOnly ? bookManager.currentBookBookmarkCount : bookManager.totalBookmarkCount;
                    var hlCount = highlightViewRoot.showCurrentBookOnly ? bookManager.currentBookHighlightCount : bookManager.totalHighlightCount;
                    var scope = highlightViewRoot.showCurrentBookOnly ? qsTr("Sách hiện tại") : qsTr("Toàn bộ thư viện");
                    return (scope + " • " + bmCount + " " + qsTr("dấu trang") + " • " + hlCount + " " + qsTr("ghi chú")) + Retranslate.onLanguageChanged;
                }
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: Color.create(bookManager.uiMuted)
                multiline: true
                autoSize.maxLineCount: 2
            }

            Container {
                visible: highlightViewRoot.showInlineActions
                verticalAlignment: VerticalAlignment.Center
                leftPadding: bookManager.dp(10)
                rightPadding: bookManager.dp(10)
                topPadding: bookManager.dp(5)
                bottomPadding: bookManager.dp(5)
                background: Color.create(bookManager.uiSurface)
                layout: DockLayout {}

                gestureHandlers: [
                    TapHandler {
                        onTapped: {
                            highlightViewRoot.toggleScope();
                        }
                    }
                ]

                Label {
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    text: highlightViewRoot.showCurrentBookOnly ? (qsTr("Tất cả sách") + Retranslate.onLanguageChanged) : (qsTr("Sách này") + Retranslate.onLanguageChanged)
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create(bookManager.uiAccent)
                }
            }

            Container {
                visible: highlightViewRoot.showInlineActions
                verticalAlignment: VerticalAlignment.Center
                leftMargin: bookManager.dp(8)
                leftPadding: bookManager.dp(10)
                rightPadding: bookManager.dp(10)
                topPadding: bookManager.dp(5)
                bottomPadding: bookManager.dp(5)
                background: Color.create(bookManager.uiSurface)
                layout: DockLayout {}

                gestureHandlers: [
                    TapHandler {
                        onTapped: {
                            highlightViewRoot.exportMarkdown();
                        }
                    }
                ]

                Label {
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    text: qsTr("Xuất .MD") + Retranslate.onLanguageChanged
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
        background: Color.create(bookManager.uiHairline)
    }

    // 3. CONTENT AREA (Empty state + ListView)
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
        layout: DockLayout {}

        // Empty State
        Container {
            id: emptyStateContainer
            visible: highlightViewRoot.displayCount === 0
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center
            leftPadding: bookManager.dp(28)
            rightPadding: bookManager.dp(28)
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            ImageView {
                horizontalAlignment: HorizontalAlignment.Center
                preferredWidth: bookManager.dp(72)
                preferredHeight: bookManager.dp(72)
                imageSource: highlightViewRoot.filterQuery !== "" ?
                             (bookManager.appTheme === "dark" ? "asset:///icons/ic_search.png" : "asset:///icons/ic_search_dark.png") :
                             (typeSegmentedControl.selectedValue === "highlights" ?
                                 (bookManager.appTheme === "dark" ? "asset:///icons/ic_highlight.png" : "asset:///icons/ic_highlight_dark.png") :
                                 (bookManager.appTheme === "dark" ? "asset:///icons/ic_bookmarks.png" : "asset:///icons/ic_bookmarks_dark.png"))
                opacity: 0.35
                scalingMethod: ScalingMethod.AspectFit
            }

            Label {
                topMargin: bookManager.dp(14)
                horizontalAlignment: HorizontalAlignment.Center
                text: highlightViewRoot.filterQuery !== "" ?
                      (qsTr("Không tìm thấy kết quả phù hợp") + Retranslate.onLanguageChanged) :
                      (typeSegmentedControl.selectedValue === "bookmarks" ? (qsTr("Chưa có dấu trang nào") + Retranslate.onLanguageChanged) :
                       (typeSegmentedControl.selectedValue === "highlights" ? (qsTr("Chưa có ghi chú nào") + Retranslate.onLanguageChanged) :
                       (qsTr("Chưa có dấu trang hoặc ghi chú") + Retranslate.onLanguageChanged)))
                textStyle.base: SystemDefaults.TextStyles.TitleText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.textAlign: TextAlign.Center
                textStyle.color: Color.create(bookManager.uiInk)
                multiline: true
            }

            Label {
                topMargin: bookManager.dp(6)
                horizontalAlignment: HorizontalAlignment.Center
                text: highlightViewRoot.filterQuery !== "" ?
                      (qsTr("Thử tìm kiếm với từ khóa khác hoặc xóa bộ lọc.") + Retranslate.onLanguageChanged) :
                      (typeSegmentedControl.selectedValue === "bookmarks" ? (qsTr("Chạm phím 'H' hoặc nút đánh dấu trong trình đọc để lưu trang.") + Retranslate.onLanguageChanged) :
                       (typeSegmentedControl.selectedValue === "highlights" ? (qsTr("Bôi đen văn bản khi đọc sách để lưu trích dẫn và ghi chú.") + Retranslate.onLanguageChanged) :
                       (qsTr("Đánh dấu trang hoặc lưu trích dẫn khi đọc sách để xem lại tại đây.") + Retranslate.onLanguageChanged)))
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: Color.create(bookManager.uiMuted)
                multiline: true
                textStyle.textAlign: TextAlign.Center
            }
        }

        // BOOKMARKS & HIGHLIGHTS LIST (Clean tap-to-jump cards, long-press ActionSet for edit/delete)
        ListView {
            id: highlightListView
            visible: highlightViewRoot.displayCount > 0
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            topPadding: bookManager.dp(10)

            function itemType(data, indexPath) {
                return "item";
            }

            dataModel: ArrayDataModel {
                id: highlightDataModel
            }

            listItemComponents: [
                ListItemComponent {
                    type: "item"
                    CustomListItem {
                        id: hlItemCell
                        dividerVisible: false
                        highlightAppearance: HighlightAppearance.Frame

                        contextActions: [
                            ActionSet {
                                title: (ListItemData.bookTitle ? ListItemData.bookTitle : qsTr("Ghi chú & Dấu trang")) + Retranslate.onLanguageChanged
                                subtitle: ListItemData.chapterTitle ? ListItemData.chapterTitle : ""

                                ActionItem {
                                    title: (ListItemData.itemType === "bookmark" ? qsTr("Đến trang này") : qsTr("Đọc đoạn này")) + Retranslate.onLanguageChanged
                                    imageSource: "asset:///icons/ic_open.png"
                                    onTriggered: { hlItemCell.ListItem.view.triggerJump(ListItemData); }
                                }
                                ActionItem {
                                    title: (ListItemData.note && ListItemData.note.length > 0 ? qsTr("Sửa ghi chú") : qsTr("Thêm ghi chú")) + Retranslate.onLanguageChanged
                                    imageSource: "asset:///icons/ic_highlight.png"
                                    onTriggered: { hlItemCell.ListItem.view.triggerEditNote(ListItemData); }
                                }
                                ActionItem {
                                    title: qsTr("Lưu ảnh trích dẫn") + Retranslate.onLanguageChanged
                                    imageSource: "asset:///icons/ic_download.png"
                                    onTriggered: { hlItemCell.ListItem.view.triggerExportQuoteCard(ListItemData); }
                                }
                                DeleteActionItem {
                                    title: qsTr("Xóa") + Retranslate.onLanguageChanged
                                    onTriggered: { hlItemCell.ListItem.view.triggerDelete(ListItemData); }
                                }
                            }
                        ]

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            leftPadding: bookManager.dp(12)
                            rightPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(10)

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                background: Color.create(bookManager.uiCard)
                                topPadding: bookManager.dp(12)
                                bottomPadding: bookManager.dp(12)
                                leftPadding: bookManager.dp(12)
                                rightPadding: bookManager.dp(12)
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                // Left Editorial Color Stripe
                                Container {
                                    preferredWidth: bookManager.dp(4)
                                    verticalAlignment: VerticalAlignment.Fill
                                    background: ListItemData.itemType === "bookmark" ? Color.create("#D4A359") :
                                                (ListItemData.color === "green" ? Color.create("#5FA876") :
                                                (ListItemData.color === "blue" ? Color.create("#5B9BD5") :
                                                (ListItemData.color === "pink" ? Color.create("#D6729B") : Color.create("#D9AB60"))))
                                    rightMargin: bookManager.dp(12)
                                }

                                // Content Column
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                        Container {
                                            visible: ListItemData.itemType === "bookmark"
                                            verticalAlignment: VerticalAlignment.Center
                                            background: Color.create("#D4A359")
                                            leftPadding: bookManager.dp(6)
                                            rightPadding: bookManager.dp(6)
                                            topPadding: bookManager.dp(2)
                                            bottomPadding: bookManager.dp(2)
                                            rightMargin: bookManager.dp(8)

                                            Label {
                                                text: qsTr("Dấu trang") + Retranslate.onLanguageChanged
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.fontWeight: FontWeight.Bold
                                                textStyle.color: Color.create("#0D0B09")
                                            }
                                        }

                                        Label {
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            verticalAlignment: VerticalAlignment.Center
                                            text: (!ListItemData.isCurrentBook ? (ListItemData.bookTitle + " • ") : "") + ListItemData.chapterTitle + (ListItemData.progress !== undefined && ListItemData.progress > 0 ? (" (" + ListItemData.progress + "%)") : "")
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: Color.create(bookManager.uiInk)
                                            multiline: true
                                            autoSize.maxLineCount: 2
                                        }

                                        Label {
                                            leftMargin: bookManager.dp(8)
                                            verticalAlignment: VerticalAlignment.Center
                                            text: ListItemData.dateStr
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: Color.create(bookManager.uiMuted)
                                        }
                                    }

                                    Label {
                                        topMargin: bookManager.dp(6)
                                        visible: (ListItemData.text !== undefined && ListItemData.text !== "") || (ListItemData.snippet !== undefined && ListItemData.snippet !== "")
                                        text: ListItemData.itemType === "bookmark" ? (ListItemData.snippet ? ("\"" + ListItemData.snippet + "\"") : "") : ("\"" + ListItemData.text + "\"")
                                        textStyle.base: SystemDefaults.TextStyles.BodyText
                                        textStyle.color: Color.create(bookManager.uiInk)
                                        textStyle.fontStyle: FontStyle.Italic
                                        textStyle.lineHeight: 1.25
                                        multiline: true
                                        autoSize.maxLineCount: 4
                                    }

                                    // Note Block (tap directly to edit)
                                    Container {
                                        visible: ListItemData.note !== undefined && ListItemData.note.length > 0
                                        topMargin: bookManager.dp(8)
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        background: Color.create(bookManager.uiSurface)
                                        topPadding: bookManager.dp(8)
                                        bottomPadding: bookManager.dp(8)
                                        leftPadding: bookManager.dp(10)
                                        rightPadding: bookManager.dp(10)
                                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                        gestureHandlers: [
                                            TapHandler {
                                                onTapped: { hlItemCell.ListItem.view.triggerEditNote(ListItemData); }
                                            }
                                        ]

                                        Label {
                                            text: ListItemData.note
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: Color.create(bookManager.uiInk)
                                            multiline: true
                                            autoSize.maxLineCount: 4
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            ]

            function triggerJump(item) {
                if (item.itemType === "bookmark") {
                    highlightViewRoot.jumpToBookmarkTriggered(item.filePath, item.chapterIndex, item.id);
                } else {
                    highlightViewRoot.jumpToHighlightTriggered(item.filePath, item.chapterIndex, item.id);
                }
            }

            function triggerEditNote(item) {
                var txt = item.itemType === "bookmark" ? item.snippet : item.text;
                noteSheet.openForHighlight(item.id, txt, item.note, item.color, item.bookTitle, "");
            }

            function triggerDelete(item) {
                if (item.itemType === "bookmark") {
                    bookManager.removeBookmark(item.id);
                } else {
                    bookManager.removeHighlight(item.id);
                }
                reloadModel();
            }

            function triggerExportQuoteCard(item) {
                var txt = item.itemType === "bookmark" ? item.snippet : item.text;
                var title = item.bookTitle ? item.bookTitle : "";
                var col = item.color ? item.color : "yellow";
                var outPath = bookManager.exportQuoteCardPng(txt ? txt : "", title, "", col);
                hlViewToast.body = (outPath !== "" ? (qsTr("Đã lưu ảnh trích dẫn: ") + outPath) : qsTr("Không thể tạo ảnh trích dẫn")) + Retranslate.onLanguageChanged;
                hlViewToast.show();
            }

            onTriggered: {
                var item = dataModel.data(indexPath);
                if (item) {
                    triggerJump(item);
                }
            }
        }
    }

    attachedObjects: [
        SystemToast {
            id: exportToast
            body: ""
        },
        SystemToast {
            id: hlViewToast
            body: ""
        },
        HighlightNoteSheet {
            id: noteSheet
            onSavedCallback: { reloadModel(); }
            onDeletedCallback: { reloadModel(); }
        }
    ]

    function reloadModel() {
        var totalHl = bookManager.totalHighlightCount;
        var totalBm = bookManager.totalBookmarkCount;
        var curHl = bookManager.currentBookHighlightCount;
        var curBm = bookManager.currentBookBookmarkCount;

        if (!highlightViewRoot.manualFilterSelection && highlightViewRoot.showCurrentBookOnly && (curHl === 0 && curBm === 0) && (totalHl > 0 || totalBm > 0)) {
            highlightViewRoot.showCurrentBookOnly = false;
        }

        var filterType = typeSegmentedControl.selectedValue ? typeSegmentedControl.selectedValue : "all";
        var curPath = bookManager.currentFilePath;
        var query = highlightViewRoot.filterQuery;
        var items = [];

        if (filterType === "all" || filterType === "bookmarks") {
            var rawBm = highlightViewRoot.showCurrentBookOnly ? bookManager.currentBookBookmarks : bookManager.allBookmarks;
            for (var b = 0; b < rawBm.length; ++b) {
                var bm = rawBm[b];
                var snip = ("" + bm.snippet).toLowerCase();
                var bmNote = ("" + bm.note).toLowerCase();
                var bmBook = ("" + bm.bookTitle).toLowerCase();
                var bmChap = ("" + bm.chapterTitle).toLowerCase();

                if (query.length > 0 && snip.indexOf(query) === -1 && bmNote.indexOf(query) === -1 && bmBook.indexOf(query) === -1 && bmChap.indexOf(query) === -1) {
                    continue;
                }

                items.push({
                    "itemType": "bookmark",
                    "id": bm.id,
                    "filePath": bm.filePath,
                    "bookTitle": bm.bookTitle ? bm.bookTitle : "",
                    "chapterIndex": bm.chapterIndex,
                    "chapterTitle": bm.chapterTitle ? bm.chapterTitle : "",
                    "progress": bm.progress,
                    "snippet": bm.snippet ? bm.snippet : "",
                    "note": bm.note ? bm.note : "",
                    "color": "blue",
                    "createdAt": bm.createdAt ? bm.createdAt : 0,
                    "dateStr": bm.dateStr ? bm.dateStr : "",
                    "isCurrentBook": (bm.filePath === curPath)
                });
            }
        }

        if (filterType === "all" || filterType === "highlights") {
            var rawHl = highlightViewRoot.showCurrentBookOnly ? bookManager.currentBookHighlights : bookManager.allHighlights;
            for (var h = 0; h < rawHl.length; ++h) {
                var hl = rawHl[h];
                var txt = ("" + hl.text).toLowerCase();
                var hlNote = ("" + hl.note).toLowerCase();
                var hlBook = ("" + hl.bookTitle).toLowerCase();
                var hlChap = ("" + hl.chapterTitle).toLowerCase();

                if (query.length > 0 && txt.indexOf(query) === -1 && hlNote.indexOf(query) === -1 && hlBook.indexOf(query) === -1 && hlChap.indexOf(query) === -1) {
                    continue;
                }

                items.push({
                    "itemType": "highlight",
                    "id": hl.id,
                    "filePath": hl.filePath,
                    "bookTitle": hl.bookTitle ? hl.bookTitle : "",
                    "chapterIndex": hl.chapterIndex,
                    "chapterTitle": hl.chapterTitle ? hl.chapterTitle : "",
                    "text": hl.text,
                    "note": hl.note ? hl.note : "",
                    "color": hl.color ? hl.color : "yellow",
                    "createdAt": hl.createdAt ? hl.createdAt : 0,
                    "dateStr": hl.dateStr ? hl.dateStr : "",
                    "isCurrentBook": (hl.filePath === curPath)
                });
            }
        }

        items.sort(function(a, b) {
            return (b.createdAt || 0) - (a.createdAt || 0);
        });

        highlightDataModel.clear();
        highlightDataModel.append(items);
        highlightViewRoot.displayCount = items.length;
    }

    function cleanup() {
        try {
            if (typeof bookManager !== "undefined" && bookManager) {
                bookManager.highlightsChanged.disconnect(reloadModel);
                bookManager.bookmarksChanged.disconnect(reloadModel);
                bookManager.currentBookChanged.disconnect(reloadModel);
            }
        } catch (e) {}
    }

    onCreationCompleted: {
        bookManager.highlightsChanged.connect(reloadModel);
        bookManager.bookmarksChanged.connect(reloadModel);
        bookManager.currentBookChanged.connect(reloadModel);
        reloadModel();
    }

    onVisibleChanged: {
        if (visible) {
            reloadModel();
        }
    }
}
