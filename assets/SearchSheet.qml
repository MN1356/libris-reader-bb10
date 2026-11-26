import bb.cascades 1.3

Sheet {
    id: searchSheet

    signal performInPageSearch(string query)
    signal jumpToMatch(int matchIndex)
    signal navigateToSearchResult(int chapterIndex, string query, int matchIndex)
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
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create(bookManager.uiBg)
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            // Top Search Controls (padded horizontally, full-bleed ListView below)
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: bookManager.dp(12)
                bottomPadding: bookManager.dp(10)
                leftPadding: bookManager.dp(16)
                rightPadding: bookManager.dp(16)

                // Search Input Row
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: Color.create(bookManager.uiCard)
                    topPadding: bookManager.dp(8)
                    bottomPadding: bookManager.dp(8)
                    leftPadding: bookManager.dp(12)
                    rightPadding: bookManager.dp(12)
                    bottomMargin: bookManager.dp(10)
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    ImageView {
                        preferredWidth: bookManager.dp(24)
                        preferredHeight: bookManager.dp(24)
                        rightMargin: bookManager.dp(8)
                        verticalAlignment: VerticalAlignment.Center
                        scalingMethod: ScalingMethod.AspectFit
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_search.png" : "asset:///icons/ic_search_dark.png"
                    }

                    TextField {
                        id: searchInputField
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        hintText: qsTr("Nhập từ khóa tìm kiếm...") + Retranslate.onLanguageChanged
                        clearButtonVisible: true
                        input.submitKey: SubmitKey.Search
                        input.onSubmitted: {
                            executeSearch();
                        }
                    }

                    Button {
                        leftMargin: bookManager.dp(8)
                        text: qsTr("Tìm") + Retranslate.onLanguageChanged
                        appearance: ControlAppearance.Primary
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
                    topMargin: bookManager.dp(10)
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: Color.create(bookManager.uiCard)
                    topPadding: bookManager.dp(8)
                    bottomPadding: bookManager.dp(8)
                    leftPadding: bookManager.dp(12)
                    rightPadding: bookManager.dp(12)
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    Label {
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        multiline: true
                        text: (resultsDataModel && resultsDataModel.size() > 0) ?
                              (qsTr("Tìm thấy %1 kết quả trong chương").arg(resultsDataModel.size()) + Retranslate.onLanguageChanged) :
                              (qsTr("Không tìm thấy kết quả trong chương") + Retranslate.onLanguageChanged)
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.W500
                        textStyle.color: (resultsDataModel && resultsDataModel.size() > 0) ? Color.create(bookManager.uiInk) : Color.create(bookManager.uiMuted)
                    }

                    Button {
                        leftMargin: bookManager.dp(8)
                        text: qsTr("Trước") + Retranslate.onLanguageChanged
                        verticalAlignment: VerticalAlignment.Center
                        enabled: (resultsDataModel && resultsDataModel.size() > 0) || searchSheet.inPageTotal > 0
                        onClicked: {
                            searchSheet.inPagePrev();
                        }
                    }

                    Button {
                        leftMargin: bookManager.dp(6)
                        text: qsTr("Tiếp") + Retranslate.onLanguageChanged
                        verticalAlignment: VerticalAlignment.Center
                        enabled: (resultsDataModel && resultsDataModel.size() > 0) || searchSheet.inPageTotal > 0
                        onClicked: {
                            searchSheet.inPageNext();
                        }
                    }
                }
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: 1
                background: Color.create(bookManager.uiHairline)
            }

            // Searching Indicator
            Container {
                visible: bookManager.isSearching
                horizontalAlignment: HorizontalAlignment.Center
                topMargin: bookManager.dp(28)
                bottomMargin: bookManager.dp(20)

                ActivityIndicator {
                    visible: bookManager.isSearching
                    running: bookManager.isSearching
                    preferredWidth: bookManager.dp(56)
                    preferredHeight: bookManager.dp(56)
                    onVisibleChanged: { if (visible) start(); else stop(); }
                }

                Label {
                    topMargin: bookManager.dp(10)
                    text: qsTr("Đang tìm kiếm trong sách...") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: Color.create(bookManager.uiMuted)
                }
            }

            // Search Results List (Full-bleed for BOTH Full-Book and In-Chapter modes)
            ListView {
                id: searchResultsList
                scrollRole: ScrollRole.Main
                visible: !bookManager.isSearching && (typeof resultsDataModel !== "undefined" && resultsDataModel ? resultsDataModel.size() > 0 : false)
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }

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
                                background: Color.create(bookManager.uiBg)

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    topPadding: bookManager.dp(12)
                                    bottomPadding: bookManager.dp(12)
                                    leftPadding: bookManager.dp(16)
                                    rightPadding: bookManager.dp(16)
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    // Chapter / Result Title
                                    Label {
                                        text: ListItemData.chapterTitle ? ListItemData.chapterTitle : ""
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.W500
                                        textStyle.color: Color.create(bookManager.uiAccent)
                                        multiline: true
                                        autoSize.maxLineCount: 2
                                    }

                                    // Snippet
                                    Label {
                                        topMargin: bookManager.dp(4)
                                        text: ListItemData.snippet ? ListItemData.snippet : ""
                                        textFormat: TextFormat.Html
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        multiline: true
                                        autoSize.maxLineCount: 5
                                    }
                                }

                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    leftMargin: bookManager.dp(16)
                                    preferredHeight: 1
                                    background: Color.create(bookManager.uiHairline)
                                }
                            }
                        }
                    }
                ]

                onTriggered: {
                    var item = dataModel.data(indexPath);
                    if (item) {
                        searchSheet.jumpedFromSearch = true;
                        var q = searchInputField.text.trim();
                        var mIdx = (typeof item.matchIndex !== "undefined" && item.matchIndex !== null) ? item.matchIndex : -1;
                        if (scopeControl.selectedValue === "full" && item.chapterIndex !== bookManager.currentChapterIndex) {
                            searchSheet.navigateToSearchResult(item.chapterIndex, q, mIdx);
                            searchSheet.close();
                        } else {
                            searchSheet.close();
                            searchSheet.performInPageSearch(q);
                            if (mIdx >= 0) {
                                searchSheet.jumpToMatch(mIdx);
                            }
                        }
                    }
                }
            }

            // Empty Results State
            Container {
                visible: !bookManager.isSearching && (typeof resultsDataModel !== "undefined" && resultsDataModel ? resultsDataModel.size() === 0 : false) && searchSheet.activeSearchTerm.length > 0
                horizontalAlignment: HorizontalAlignment.Fill
                topMargin: bookManager.dp(40)
                leftPadding: bookManager.dp(24)
                rightPadding: bookManager.dp(24)

                Label {
                    horizontalAlignment: HorizontalAlignment.Center
                    multiline: true
                    textStyle.textAlign: TextAlign.Center
                    text: (scopeControl.selectedValue === "chapter" ?
                           (qsTr("Không tìm thấy kết quả nào trong chương.") + Retranslate.onLanguageChanged) :
                           (qsTr("Không tìm thấy kết quả nào trong sách.") + Retranslate.onLanguageChanged))
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                    textStyle.fontWeight: FontWeight.W500
                    textStyle.color: Color.create(bookManager.uiInk)
                }

                Label {
                    topMargin: bookManager.dp(8)
                    horizontalAlignment: HorizontalAlignment.Center
                    multiline: true
                    textStyle.textAlign: TextAlign.Center
                    text: qsTr("Hãy thử tìm kiếm với từ khóa khác ngắn hơn.") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: Color.create(bookManager.uiMuted)
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
