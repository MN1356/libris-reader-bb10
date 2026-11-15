import bb.cascades 1.3

Sheet {
    id: settingsSheet

    NavigationPane {
        id: settingsNavPane

        Page {
            id: mainSettingsPage

            titleBar: TitleBar {
                title: qsTr("Cài Đặt") + Retranslate.onLanguageChanged
                appearance: TitleBarAppearance.Default
                dismissAction: ActionItem {
                    title: qsTr("Đóng") + Retranslate.onLanguageChanged
                    onTriggered: {
                        settingsSheet.close();
                    }
                }
            }

            ScrollView {
                scrollRole: ScrollRole.Main
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    bottomPadding: 24

                    StandardListItem {
                        id: itemDisplay
                        title: qsTr("Hiển thị") + Retranslate.onLanguageChanged
                        description: qsTr("Chủ đề, phông chữ & màu nền") + Retranslate.onLanguageChanged
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_theme.png" : "asset:///icons/ic_theme_dark.png"
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    settingsSheet.openCategory("display");
                                }
                            }
                        ]
                    }

                    StandardListItem {
                        id: itemReading
                        title: qsTr("Thao tác đọc") + Retranslate.onLanguageChanged
                        description: qsTr("Chế độ cuộn & lật trang") + Retranslate.onLanguageChanged
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_reading.png" : "asset:///icons/ic_reading_dark.png"
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    settingsSheet.openCategory("reading");
                                }
                            }
                        ]
                    }

                    StandardListItem {
                        id: itemFooter
                        title: qsTr("Chân trang") + Retranslate.onLanguageChanged
                        description: qsTr("Tên chương, tiến độ, pin & đồng hồ") + Retranslate.onLanguageChanged
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_chart.png" : "asset:///icons/ic_chart_dark.png"
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    settingsSheet.openCategory("footer");
                                }
                            }
                        ]
                    }

                    StandardListItem {
                        id: itemTts
                        title: qsTr("Đọc sách (TTS)") + Retranslate.onLanguageChanged
                        description: qsTr("Bộ máy đọc, giọng nói AI & tốc độ") + Retranslate.onLanguageChanged
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_tts.png" : "asset:///icons/ic_tts_dark.png"
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    settingsSheet.openCategory("tts");
                                }
                            }
                        ]
                    }

                    StandardListItem {
                        id: itemLibrary
                        title: qsTr("Tủ sách") + Retranslate.onLanguageChanged
                        description: qsTr("Bố cục, quét tệp & lịch sử đọc") + Retranslate.onLanguageChanged
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_grid.png" : "asset:///icons/ic_grid_dark.png"
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    settingsSheet.openCategory("library");
                                }
                            }
                        ]
                    }

                    StandardListItem {
                        id: itemAbout
                        title: qsTr("Giới thiệu") + Retranslate.onLanguageChanged
                        description: qsTr("Phiên bản BerryLeaf & hướng dẫn") + Retranslate.onLanguageChanged
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_info.png" : "asset:///icons/ic_info_dark.png"
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    settingsSheet.openCategory("about");
                                }
                            }
                        ]
                    }

                    Divider {
                        topMargin: 8
                        bottomMargin: 8
                    }

                    StandardListItem {
                        id: itemExit
                        title: qsTr("Thoát ứng dụng") + Retranslate.onLanguageChanged
                        description: qsTr("Lưu tiến độ và đóng hoàn toàn BerryLeaf (tránh chạy nền)") + Retranslate.onLanguageChanged
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_exit.png" : "asset:///icons/ic_exit_dark.png"
                        gestureHandlers: [
                            TapHandler {
                                onTapped: {
                                    settingsSheet.close();
                                    app.promptExit();
                                }
                            }
                        ]
                    }
                }
            }
        }

        onPopTransitionEnded: {
            if (page) {
                page.destroy();
            }
        }
    }

    attachedObjects: [
        ComponentDefinition {
            id: displayPageDef
            source: "asset:///settings/DisplaySettingsPage.qml"
        },
        ComponentDefinition {
            id: readingPageDef
            source: "asset:///settings/ReadingSettingsPage.qml"
        },
        ComponentDefinition {
            id: footerPageDef
            source: "asset:///settings/FooterSettingsPage.qml"
        },
        ComponentDefinition {
            id: ttsPageDef
            source: "asset:///settings/TtsSettingsPage.qml"
        },
        ComponentDefinition {
            id: libraryPageDef
            source: "asset:///settings/LibrarySettingsPage.qml"
        },
        ComponentDefinition {
            id: aboutPageDef
            source: "asset:///settings/AboutSettingsPage.qml"
        }
    ]

    function openToTts() {
        settingsSheet.open();
        openCategory("tts");
    }

    function openCategory(cat) {
        var def = null;
        if (cat === "display") def = displayPageDef;
        else if (cat === "reading") def = readingPageDef;
        else if (cat === "footer") def = footerPageDef;
        else if (cat === "tts") def = ttsPageDef;
        else if (cat === "library") def = libraryPageDef;
        else if (cat === "about") def = aboutPageDef;

        if (def) {
            var page = def.createObject();
            if (page) {
                if (page.backRequested) {
                    page.backRequested.connect(function() {
                        settingsNavPane.pop();
                    });
                }
                settingsNavPane.push(page);
            }
        }
    }

    onClosed: {
        if (settingsNavPane.top !== mainSettingsPage) {
            settingsNavPane.pop();
        }
    }
}
