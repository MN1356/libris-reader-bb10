import bb.cascades 1.3

Sheet {
    id: highlightSheet

    signal highlightJumpRequested(string highlightId)

    Page {
        titleBar: TitleBar {
            title: qsTr("Dấu Trang & Ghi Chú") + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain
            dismissAction: ActionItem {
                title: qsTr("Đóng") + Retranslate.onLanguageChanged
                onTriggered: {
                    highlightSheet.close();
                }
            }
            acceptAction: ActionItem {
                title: (readerHighlightView.isSearching ? qsTr("Đóng") : qsTr("Tìm")) + Retranslate.onLanguageChanged
                onTriggered: {
                    readerHighlightView.toggleSearch();
                }
            }
        }

        HighlightView {
            id: readerHighlightView
            showCurrentBookOnly: true

            onJumpToHighlightTriggered: {
                highlightSheet.close();
                bookManager.jumpToHighlight(highlightId);
                highlightSheet.highlightJumpRequested(highlightId);
            }

            onJumpToBookmarkTriggered: {
                highlightSheet.close();
                bookManager.jumpToBookmark(bookmarkId);
            }
        }
    }

    function cleanup() {
        if (readerHighlightView && typeof readerHighlightView.cleanup === "function") {
            readerHighlightView.cleanup();
        }
    }

    onOpened: {
        if (readerHighlightView) {
            readerHighlightView.reloadModel();
        }
    }
}
