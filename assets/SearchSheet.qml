import bb.cascades 1.3

Sheet {
    id: searchSheet

    signal performInPageSearch(string query)
    signal jumpToMatch(int matchIndex)
    signal inPageNext()
    signal inPagePrev()
    signal inPageClear()

    property int inPageCurrent: 0
    property int inPageTotal: 0
    property string activeSearchTerm: ""
    property bool jumpedFromSearch: false

    onClosed: {
        if (!searchSheet.jumpedFromSearch) {
            searchSheet.inPageClear();
        }
        searchSheet.jumpedFromSearch = false;
    }

    Page {
        titleBar: TitleBar {
            title: qsTr("Tìm kiếm trong sách") + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain
            dismissAction: ActionItem {
                title: qsTr("Đóng") + Retranslate.onLanguageChanged
                onTriggered: {
                    searchSheet.inPageClear();
                    searchSheet.close();
                }
            }
        }

        Container {
            layout: DockLayout {}
            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                topPadding: 12
                bottomPadding: 12
                leftPadding: 12
                rightPadding: 12

                // Search Input Box
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                    topPadding: 6
                    bottomPadding: 6
                    leftPadding: 10
                    rightPadding: 10
                    bottomMargin: 8

                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    ImageView {
                        preferredWidth: 32
                        preferredHeight: 32
                        verticalAlignment: VerticalAlignment.Center
                        imageSource: "asset:///icons/ic_search.png"
                    }

                    TextField {
                        id: searchInputField
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                        hintText: qsTr("Nhập từ khóa tìm kiếm...") + Retranslate.onLanguageChanged
                        input.submitKey: SubmitKey.Search
                        input.onSubmitted: {
                            executeSearch();
                        }
                    }

                    Button {
                        text: qsTr("Tìm") + Retranslate.onLanguageChanged
                        preferredWidth: 105
                        verticalAlignment: VerticalAlignment.Center
                        onClicked: {
                            executeSearch();
                        }
                    }
                }

                // Scope SegmentedControl: Full Book vs In Chapter
                SegmentedControl {
                    id: scopeControl
                    horizontalAlignment: HorizontalAlignment.Fill
                    bottomMargin: 10

                    Option {
                        id: optFullBook
                        text: qsTr("Toàn bộ sách") + Retranslate.onLanguageChanged
                        value: "full"
                        selected: true
                    }

                    Option {
                        id: optChapter
                        text: qsTr("Trong chương") + Retranslate.onLanguageChanged
                        value: "chapter"
                    }

                    onSelectedIndexChanged: {
                        if (searchInputField.text.trim().length > 0) {
                            executeSearch();
                        }
                    }
                }

                // In-Chapter Navigation Toolbar (Visible when in-chapter mode is selected)
                Container {
                    visible: scopeControl.selectedValue === "chapter" && searchSheet.activeSearchTerm.length > 0 && !bookManager.isSearching
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                    topPadding: 8
                    bottomPadding: 8
                    leftPadding: 12
                    rightPadding: 12
                    bottomMargin: 10

                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    Label {
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                        text: (resultsDataModel && resultsDataModel.size() > 0) ? 
                              (qsTr("Tìm thấy %1 kết quả trong chương").arg(resultsDataModel.size()) + Retranslate.onLanguageChanged) : 
                              (qsTr("Không tìm thấy kết quả trong chương") + Retranslate.onLanguageChanged)
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: (resultsDataModel && resultsDataModel.size() > 0) ? (bookManager.appTheme === "dark" ? Color.White : Color.Black) : (bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#777777"))
                    }

                    Button {
                        text: qsTr("Trước") + Retranslate.onLanguageChanged
                        preferredWidth: 110
                        verticalAlignment: VerticalAlignment.Center
                        enabled: (resultsDataModel && resultsDataModel.size() > 0) || searchSheet.inPageTotal > 0
                        onClicked: {
                            searchSheet.inPagePrev();
                        }
                    }

                    Button {
                        leftMargin: 6
                        text: qsTr("Tiếp") + Retranslate.onLanguageChanged
                        preferredWidth: 110
                        verticalAlignment: VerticalAlignment.Center
                        enabled: (resultsDataModel && resultsDataModel.size() > 0) || searchSheet.inPageTotal > 0
                        onClicked: {
                            searchSheet.inPageNext();
                        }
                    }
                }

                // Searching Indicator
                Container {
                    visible: bookManager.isSearching
                    horizontalAlignment: HorizontalAlignment.Center
                    topMargin: 20
                    bottomMargin: 20

                    ActivityIndicator {
                        visible: bookManager.isSearching
                        running: bookManager.isSearching
                        preferredWidth: 64
                        preferredHeight: 64
                        onVisibleChanged: { if (visible) start(); else stop(); }
                    }

                    Label {
                        topMargin: 8
                        text: qsTr("Đang tìm kiếm trong sách...") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#777777")
                    }
                }

                // Search Results List (Visible for BOTH Full-Book and In-Chapter modes)
                ListView {
                    visible: !bookManager.isSearching && (typeof resultsDataModel !== "undefined" && resultsDataModel ? resultsDataModel.size() > 0 : false)
                    id: searchResultsList
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1.0
                    }

                    function itemType(data, indexPath) {
                        return "item";
                    }

                    dataModel: ArrayDataModel {
                        id: resultsDataModel
                    }

                    listItemComponents: [
                        ListItemComponent {
                            type: "item"
                            CustomListItem {
                                dividerVisible: false
                                highlightAppearance: HighlightAppearance.Full
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")

                                    Container {
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        topPadding: 12
                                        bottomPadding: 12
                                        leftPadding: 14
                                        rightPadding: 14

                                        layout: StackLayout {
                                            orientation: LayoutOrientation.TopToBottom
                                        }

                                        // Chapter / Result Title
                                        Label {
                                            text: ListItemData.chapterTitle ? ListItemData.chapterTitle : ""
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.Bold
                                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#10B981") : Color.create("#059669")
                                            multiline: true
                                            autoSize.maxLineCount: 2
                                        }

                                        // Snippet (Full paragraph / sentence excerpt with bold matching keyword)
                                        Label {
                                            topMargin: 4
                                            text: ListItemData.snippet ? ListItemData.snippet : ""
                                            textFormat: TextFormat.Html
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                            multiline: true
                                            autoSize.maxLineCount: 5
                                        }
                                    }

                                    Divider {
                                        topMargin: 0
                                        bottomMargin: 0
                                    }
                                }
                            }
                        }
                    ]

                    onTriggered: {
                        var item = dataModel.data(indexPath);
                        if (item) {
                            searchSheet.jumpedFromSearch = true;
                            if (scopeControl.selectedValue === "full") {
                                bookManager.goToChapter(item.chapterIndex);
                            }
                            searchSheet.close();
                            // Trigger in-page highlight for the query in the target chapter
                            searchSheet.performInPageSearch(searchInputField.text.trim());
                            if (typeof item.matchIndex !== "undefined" && item.matchIndex !== null) {
                                searchSheet.jumpToMatch(item.matchIndex);
                            }
                        }
                    }
                }

                // Empty Results State
                Container {
                    visible: !bookManager.isSearching && (typeof resultsDataModel !== "undefined" && resultsDataModel ? resultsDataModel.size() === 0 : false) && searchSheet.activeSearchTerm.length > 0
                    horizontalAlignment: HorizontalAlignment.Center
                    topMargin: 30

                    Label {
                        horizontalAlignment: HorizontalAlignment.Center
                        text: (scopeControl.selectedValue === "chapter" ? 
                               (qsTr("Không tìm thấy kết quả nào trong chương.") + Retranslate.onLanguageChanged) : 
                               (qsTr("Không tìm thấy kết quả nào trong sách.") + Retranslate.onLanguageChanged))
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#777777")
                    }

                    Label {
                        topMargin: 8
                        horizontalAlignment: HorizontalAlignment.Center
                        text: qsTr("Hãy thử tìm kiếm với từ khóa khác ngắn hơn.") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#555555") : Color.create("#94A3B8")
                    }
                }
            }
        }
    }

    function executeSearch() {
        var query = searchInputField.text.trim();
        searchSheet.activeSearchTerm = query;
        if (query.length === 0) return;

        var scope = scopeControl.selectedValue;
        bookManager.searchBook(query, scope);
        if (scope === "chapter") {
            searchSheet.performInPageSearch(query);
        }
    }

    function openWithQuery(q) {
        if (q && q.length > 0) {
            searchInputField.text = q;
        }
        searchSheet.open();
    }

    onOpened: {
        if (searchInputField.text.trim().length > 0) {
            executeSearch();
        }
    }

    function updateResults() {
        if (typeof resultsDataModel !== "undefined" && resultsDataModel) {
            resultsDataModel.clear();
            resultsDataModel.append(bookManager.searchResults);
        }
    }

    onCreationCompleted: {
        var bm = bookManager;
        var ss = searchSheet;
        bm.searchResultsChanged.connect(ss.updateResults);
    }

    function cleanup() {
        try {
            if (typeof bookManager !== "undefined" && bookManager) {
                bookManager.searchResultsChanged.disconnect(searchSheet.updateResults);
            }
        } catch (e) {}
    }
}
