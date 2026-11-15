import bb.cascades 1.3

Page {
    id: highlightPage

    actionBarVisibility: ChromeVisibility.Visible

    titleBar: TitleBar {
        title: qsTr("Dấu Trang & Ghi Chú") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
        acceptAction: ActionItem {
            title: ""
            imageSource: "asset:///icons/ic_search.png"
            onTriggered: {
                mainHighlightView.toggleSearch();
            }
        }
    }

    Menu.definition: MenuDefinition {
        helpAction: HelpActionItem {
            title: qsTr("Giới thiệu") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_info.png"
            onTriggered: {
                app.showAbout();
            }
        }
        settingsAction: SettingsActionItem {
            title: qsTr("Cài Đặt") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_settings.png"
            onTriggered: {
                app.openSettings();
            }
        }
        actions: [
            ActionItem {
                title: qsTr("Thoát") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_exit.png"
                onTriggered: {
                    app.promptExit();
                }
            }
        ]
    }

    actions: [
        ActionItem {
            title: mainHighlightView.showCurrentBookOnly ? (qsTr("Tất cả sách") + Retranslate.onLanguageChanged) : (qsTr("Sách hiện tại") + Retranslate.onLanguageChanged)
            imageSource: "asset:///icons/ic_library.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                mainHighlightView.toggleScope();
            }
        },
        ActionItem {
            title: bookManager.isCurrentChapterBookmarked ?
                   (qsTr("Bỏ dấu trang") + Retranslate.onLanguageChanged) :
                   (qsTr("Đánh dấu trang") + Retranslate.onLanguageChanged)
            imageSource: bookManager.isCurrentChapterBookmarked ? "asset:///icons/ic_bookmarks.png" : "asset:///icons/ic_bookmark_add.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            enabled: bookManager.currentFilePath !== ""
            onTriggered: {
                bookManager.toggleBookmarkCurrentPosition();
                mainHighlightView.reloadModel();
            }
        },
        ActionItem {
            title: qsTr("Tủ Sách") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_grid.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                app.switchToTab("shelf");
            }
        },
        ActionItem {
            title: qsTr("Thoát") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_exit.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            shortcuts: [
                Shortcut { key: "Q" }
            ]
            onTriggered: {
                app.promptExit();
            }
        }
    ]

    HighlightView {
        id: mainHighlightView
        showCurrentBookOnly: (bookManager.currentFilePath !== "" && (bookManager.currentBookHighlightCount > 0 || bookManager.currentBookBookmarkCount > 0))

        onJumpToHighlightTriggered: {
            if (filePath && filePath !== "") {
                app.openBook(filePath);
            } else {
                app.switchToTab("reading");
            }
            bookManager.jumpToHighlight(highlightId);
        }

        onJumpToBookmarkTriggered: {
            if (filePath && filePath !== "") {
                app.openBook(filePath);
            } else {
                app.switchToTab("reading");
            }
            bookManager.jumpToBookmark(bookmarkId);
        }
    }

    function reloadHighlights() {
        if (mainHighlightView) {
            mainHighlightView.reloadModel();
        }
    }
}
