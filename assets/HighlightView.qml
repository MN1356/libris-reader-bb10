import bb.cascades 1.3

Container {
    id: highlightViewRoot

    property bool showCurrentBookOnly: true
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
            reloadModel();
        }
    }

    layout: DockLayout {}
    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        topPadding: 10
        bottomPadding: 16
        leftPadding: 12
        rightPadding: 12

        layout: StackLayout {
            orientation: LayoutOrientation.TopToBottom
        }

        // =====================================================================
        // 1. COLLAPSIBLE SEARCH FIELD (Visible only when search button in TitleBar is tapped)
        // =====================================================================
        Container {
            id: searchInputContainer
            visible: highlightViewRoot.isSearching
            horizontalAlignment: HorizontalAlignment.Fill
            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#F0F0F0")
            topPadding: 6
            bottomPadding: 6
            leftPadding: 10
            rightPadding: 10
            bottomMargin: 10

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            ImageView {
                preferredWidth: 28
                preferredHeight: 28
                verticalAlignment: VerticalAlignment.Center
                imageSource: "asset:///icons/ic_search.png"
            }

            TextField {
                id: searchInput
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

        // =====================================================================
        // 2. TYPE SELECTOR (Single Clean SegmentedControl: Tất cả / Dấu trang / Ghi chú)
        // =====================================================================
        SegmentedControl {
            id: typeSegmentedControl
            horizontalAlignment: HorizontalAlignment.Fill
            bottomMargin: 8

            Option {
                id: optAll
                text: qsTr("Tất cả") + Retranslate.onLanguageChanged
                value: "all"
                selected: true
            }

            Option {
                id: optBookmarks
                text: qsTr("Dấu trang") + " (" + (highlightViewRoot.showCurrentBookOnly ? bookManager.currentBookBookmarkCount : bookManager.totalBookmarkCount) + ")" + Retranslate.onLanguageChanged
                value: "bookmarks"
            }

            Option {
                id: optHighlights
                text: qsTr("Ghi chú") + " (" + (highlightViewRoot.showCurrentBookOnly ? bookManager.currentBookHighlightCount : bookManager.totalHighlightCount) + ")" + Retranslate.onLanguageChanged
                value: "highlights"
            }

            onSelectedIndexChanged: {
                reloadModel();
            }
        }

        // =====================================================================
        // 3. SUB-HEADER: SCOPE STATUS & COMPACT TOGGLE PILL
        // =====================================================================
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            topMargin: 2
            bottomMargin: 10
            leftPadding: 2
            rightPadding: 2

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            Label {
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                text: (highlightViewRoot.showCurrentBookOnly ? qsTr("Phạm vi: Sách hiện tại") : qsTr("Phạm vi: Toàn bộ thư viện")) + Retranslate.onLanguageChanged
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
            }

            // Compact Emerald Scope Switcher Pill
            Container {
                verticalAlignment: VerticalAlignment.Center
                preferredHeight: 40
                preferredWidth: 108
                minHeight: 36
                background: bookManager.appTheme === "dark" ? Color.create("#1A1A1A") : Color.create("#E5E5E5")
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
                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create("#10B981")
                }
            }
        }

        // =====================================================================
        // 4. EMPTY STATE (Clean Centered Icon, Bold Typography, High Contrast)
        // =====================================================================
        Container {
            id: emptyStateContainer
            visible: highlightViewRoot.displayCount === 0
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center
            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
            leftPadding: 32
            rightPadding: 32

            layout: StackLayout {
                orientation: LayoutOrientation.TopToBottom
            }

            ImageView {
                horizontalAlignment: HorizontalAlignment.Center
                preferredWidth: 160
                preferredHeight: 160
                imageSource: typeSegmentedControl.selectedValue === "highlights" ? "asset:///icons/ic_highlight.png" : "asset:///icons/ic_bookmarks.png"
                bottomMargin: 20
                opacity: 0.35
                scalingMethod: ScalingMethod.AspectFit
            }

            Label {
                horizontalAlignment: HorizontalAlignment.Center
                text: typeSegmentedControl.selectedValue === "bookmarks" ? (qsTr("Chưa có dấu trang nào") + Retranslate.onLanguageChanged) :
                      (typeSegmentedControl.selectedValue === "highlights" ? (qsTr("Chưa có ghi chú nào") + Retranslate.onLanguageChanged) :
                      (qsTr("Chưa có dấu trang hoặc ghi chú") + Retranslate.onLanguageChanged))
                textStyle.base: SystemDefaults.TextStyles.BigText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                bottomMargin: 8
            }

            Label {
                horizontalAlignment: HorizontalAlignment.Center
                text: typeSegmentedControl.selectedValue === "bookmarks" ? (qsTr("Chạm phím 'H' hoặc nút đánh dấu trong trình đọc để lưu trang.") + Retranslate.onLanguageChanged) :
                      (typeSegmentedControl.selectedValue === "highlights" ? (qsTr("Bôi đen văn bản khi đọc sách để lưu trích dẫn và ghi chú.") + Retranslate.onLanguageChanged) :
                      (qsTr("Đánh dấu trang hoặc lưu trích dẫn khi đọc sách để xem lại tại đây.") + Retranslate.onLanguageChanged))
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                multiline: true
                textStyle.textAlign: TextAlign.Center
            }
        }

        // =====================================================================
        // 6. BOOKMARKS & HIGHLIGHTS LIST
        // =====================================================================
        ListView {
            id: highlightListView
            visible: highlightViewRoot.displayCount > 0
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

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
                        dividerVisible: true
                        highlightAppearance: HighlightAppearance.Frame

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                            topPadding: 12
                            bottomPadding: 12
                            leftPadding: 12
                            rightPadding: 12
                            bottomMargin: 8

                            layout: StackLayout {
                                orientation: LayoutOrientation.LeftToRight
                            }

                            // Left Color Bar / Ribbon
                            Container {
                                preferredWidth: 6
                                verticalAlignment: VerticalAlignment.Fill
                                background: ListItemData.itemType === "bookmark" ? Color.create("#10B981") :
                                            (ListItemData.color === "green" ? Color.create("#22C55E") :
                                            (ListItemData.color === "blue" ? Color.create("#00A8DF") :
                                            (ListItemData.color === "pink" ? Color.create("#EC4899") : Color.create("#EAB308"))))
                                rightMargin: 10
                            }

                            // Content Container
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                layout: StackLayout {
                                    orientation: LayoutOrientation.TopToBottom
                                }

                                // Header: Bookmark tag badge + Book Title & Chapter & Date
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    bottomMargin: 6
                                    layout: StackLayout {
                                        orientation: LayoutOrientation.LeftToRight
                                    }

                                    // Bookmark tag badge (visible for bookmarks)
                                    Container {
                                        visible: ListItemData.itemType === "bookmark"
                                        verticalAlignment: VerticalAlignment.Center
                                        background: Color.create("#10B981")
                                        leftPadding: 6
                                        rightPadding: 6
                                        topPadding: 2
                                        bottomPadding: 2
                                        rightMargin: 6

                                        Label {
                                            text: qsTr("Dấu trang") + Retranslate.onLanguageChanged
                                            textStyle.base: SystemDefaults.TextStyles.XSmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: Color.White
                                        }
                                    }

                                    Label {
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        text: (!ListItemData.isCurrentBook ? (ListItemData.bookTitle + " • ") : "") + ListItemData.chapterTitle + (ListItemData.progress !== undefined && ListItemData.progress > 0 ? (" (" + ListItemData.progress + "%)") : "")
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                        multiline: true
                                        autoSize.maxLineCount: 2
                                    }

                                    Label {
                                        text: ListItemData.dateStr
                                        textStyle.base: SystemDefaults.TextStyles.XSmallText
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#777777")
                                    }
                                }

                                // Snippet or Quote text
                                Label {
                                    visible: (ListItemData.text !== undefined && ListItemData.text !== "") || (ListItemData.snippet !== undefined && ListItemData.snippet !== "")
                                    text: ListItemData.itemType === "bookmark" ? (ListItemData.snippet ? ("\"" + ListItemData.snippet + "\"") : "") : ("\"" + ListItemData.text + "\"")
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                    textStyle.fontStyle: FontStyle.Italic
                                    multiline: true
                                    autoSize.maxLineCount: 4
                                    bottomMargin: 8
                                }

                                // Note block (if user added note)
                                Container {
                                    visible: ListItemData.note !== undefined && ListItemData.note.length > 0
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    background: bookManager.appTheme === "dark" ? Color.create("#181818") : Color.create("#F5F5F5")
                                    topPadding: 8
                                    bottomPadding: 8
                                    leftPadding: 10
                                    rightPadding: 10
                                    bottomMargin: 8

                                    layout: StackLayout {
                                        orientation: LayoutOrientation.TopToBottom
                                    }

                                    Label {
                                        text: qsTr("Ghi chú:") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.XSmallText
                                        textStyle.fontWeight: FontWeight.Bold
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#777777")
                                        bottomMargin: 2
                                    }

                                    Label {
                                        text: ListItemData.note
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                        multiline: true
                                        autoSize.maxLineCount: 4
                                    }
                                }

                                // Item Action Row (Flat Buttons: Jump, Note, Delete)
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    layout: StackLayout {
                                        orientation: LayoutOrientation.LeftToRight
                                    }

                                    Button {
                                        text: (ListItemData.itemType === "bookmark" ? qsTr("Đến trang này") : qsTr("Đọc đoạn này")) + Retranslate.onLanguageChanged
                                        preferredHeight: 46
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        onClicked: {
                                            hlItemCell.ListItem.view.triggerJump(ListItemData);
                                        }
                                    }

                                    Button {
                                        text: (ListItemData.note && ListItemData.note.length > 0 ? qsTr("Sửa ghi chú") : qsTr("Thêm ghi chú")) + Retranslate.onLanguageChanged
                                        preferredHeight: 46
                                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                        leftMargin: 8
                                        onClicked: {
                                            hlItemCell.ListItem.view.triggerEditNote(ListItemData);
                                        }
                                    }

                                    Button {
                                        text: qsTr("Xóa") + Retranslate.onLanguageChanged
                                        preferredHeight: 46
                                        preferredWidth: 90
                                        leftMargin: 8
                                        onClicked: {
                                            hlItemCell.ListItem.view.triggerDelete(ListItemData);
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
                noteSheet.openForHighlight(item.id, item.itemType === "bookmark" ? (item.snippet ? item.snippet : item.chapterTitle) : item.text, item.note, item.color ? item.color : "blue");
            }

            function triggerDelete(item) {
                if (item.itemType === "bookmark") {
                    bookManager.removeBookmark(item.id);
                } else {
                    bookManager.removeHighlight(item.id);
                }
                reloadModel();
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
        HighlightNoteSheet {
            id: noteSheet
            onSavedCallback: {
                reloadModel();
            }
            onDeletedCallback: {
                reloadModel();
            }
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

        // 1. Collect Bookmarks
        if (filterType === "all" || filterType === "bookmarks") {
            var rawBm = highlightViewRoot.showCurrentBookOnly ? bookManager.currentBookBookmarks : bookManager.allBookmarks;
            for (var b = 0; b < rawBm.length; ++b) {
                var bm = rawBm[b];
                var snip = ("" + bm.snippet).toLowerCase();
                var bmNote = ("" + bm.note).toLowerCase();
                var bmBook = ("" + bm.bookTitle).toLowerCase();
                var bmChap = ("" + bm.chapterTitle).toLowerCase();

                if (query.length > 0) {
                    if (snip.indexOf(query) === -1 && bmNote.indexOf(query) === -1 && bmBook.indexOf(query) === -1 && bmChap.indexOf(query) === -1) {
                        continue;
                    }
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

        // 2. Collect Highlights
        if (filterType === "all" || filterType === "highlights") {
            var rawHl = highlightViewRoot.showCurrentBookOnly ? bookManager.currentBookHighlights : bookManager.allHighlights;
            for (var h = 0; h < rawHl.length; ++h) {
                var hl = rawHl[h];
                var txt = ("" + hl.text).toLowerCase();
                var hlNote = ("" + hl.note).toLowerCase();
                var hlBook = ("" + hl.bookTitle).toLowerCase();
                var hlChap = ("" + hl.chapterTitle).toLowerCase();

                if (query.length > 0) {
                    if (txt.indexOf(query) === -1 && hlNote.indexOf(query) === -1 && hlBook.indexOf(query) === -1 && hlChap.indexOf(query) === -1) {
                        continue;
                    }
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

        // Sort descending by createdAt
        items.sort(function(a, b) {
            return (b.createdAt || 0) - (a.createdAt || 0);
        });

        highlightDataModel.clear();
        highlightDataModel.append(items);
        highlightViewRoot.displayCount = items.length;
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
