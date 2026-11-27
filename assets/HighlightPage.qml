import bb.cascades 1.3
import bb.system 1.2

Page {
    id: highlightPage

    actionBarVisibility: ChromeVisibility.Visible

    titleBar: TitleBar {
        title: qsTr("Dấu Trang & Ghi Chú") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
        acceptAction: ActionItem {
            title: (mainHighlightView.isSearching ? qsTr("Đóng") : qsTr("Tìm")) + Retranslate.onLanguageChanged
            onTriggered: {
                mainHighlightView.toggleSearch();
            }
        }
    }

    Menu.definition: MenuDefinition {
        helpAction: HelpActionItem {
            title: qsTr("Giới Thiệu") + Retranslate.onLanguageChanged
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
                title: qsTr("Trang chủ") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_home.png"
                onTriggered: {
                    app.switchToTab("home");
                }
            },
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
            title: (mainHighlightView.showCurrentBookOnly ? qsTr("Tất cả sách") : qsTr("Sách hiện tại")) + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_filter.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                mainHighlightView.toggleScope();
            }
        },
        ActionItem {
            title: qsTr("Xuất Markdown") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_export.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                mainHighlightView.exportMarkdown();
            }
        },
        ActionItem {
            title: bookManager.isCurrentChapterBookmarked ?
                   (qsTr("Bỏ dấu trang") + Retranslate.onLanguageChanged) :
                   (qsTr("Đánh dấu trang") + Retranslate.onLanguageChanged)
            imageSource: bookManager.isCurrentChapterBookmarked ? "asset:///icons/ic_bookmark_filled.png" : "asset:///icons/ic_bookmark_add.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            enabled: bookManager.currentFilePath !== ""
            onTriggered: {
                bookManager.toggleBookmarkCurrentPosition();
                mainHighlightView.reloadModel();
            }
        },
        ActionItem {
            title: qsTr("Tủ Sách") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_library.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            onTriggered: {
                app.switchToTab("shelf");
            }
        }
    ]

    HighlightView {
        id: mainHighlightView
        showInlineActions: false

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

    onCreationCompleted: {
        mainHighlightView.showCurrentBookOnly = (bookManager.currentFilePath !== "" && (bookManager.currentBookHighlightCount > 0 || bookManager.currentBookBookmarkCount > 0));
    }

    function reloadHighlights() {
        if (mainHighlightView) {
            mainHighlightView.reloadModel();
        }
    }
}
