import bb.cascades 1.3

Page {
    id: readingSettingsPage

    signal backRequested()

    property bool isSyncing: false
    property string syncedReadingMode: bookManager.readingMode
    property string syncedColumnLayout: bookManager.columnLayout

    function syncDropDowns() {
        if (!modeDropDown || !optModePaged || !optModeScroll || !columnDropDown || !optColSingle || !optColDouble) {
            return;
        }
        isSyncing = true;
        modeDropDown.setSelectedOption(bookManager.readingMode === "scroll" ? optModeScroll : optModePaged);
        columnDropDown.setSelectedOption(bookManager.columnLayout === "double" ? optColDouble : optColSingle);
        isSyncing = false;
    }

    onCreationCompleted: {
        syncDropDowns();
    }

    onSyncedReadingModeChanged: {
        syncDropDowns();
    }

    onSyncedColumnLayoutChanged: {
        syncDropDowns();
    }

    titleBar: TitleBar {
        title: qsTr("Thao tác đọc") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Plain
    }

    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: Color.create(bookManager.uiBg)

        ScrollView {
            scrollRole: ScrollRole.Main
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                bottomPadding: bookManager.dp(32)

                // Section 1: Chế độ đọc
                Header {
                    title: qsTr("CHẾ ĐỘ ĐỌC") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(16)
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)

                    DropDown {
                        id: modeDropDown
                        title: qsTr("Chuyển trang") + Retranslate.onLanguageChanged

                        Option {
                            id: optModePaged
                            text: qsTr("Vuốt lật trang (Paged)") + Retranslate.onLanguageChanged
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_page.png" : "asset:///icons/ic_page_dark.png"
                            value: "paged"
                            selected: bookManager.readingMode === "paged"
                        }
                        Option {
                            id: optModeScroll
                            text: qsTr("Cuộn liên tục (Scroll)") + Retranslate.onLanguageChanged
                            imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_scroll.png" : "asset:///icons/ic_scroll_dark.png"
                            value: "scroll"
                            selected: bookManager.readingMode === "scroll"
                        }

                        onSelectedValueChanged: {
                            if (!readingSettingsPage.isSyncing && selectedValue && selectedValue !== bookManager.readingMode) {
                                bookManager.setReadingMode(selectedValue);
                            }
                        }
                    }

                    DropDown {
                        id: columnDropDown
                        topMargin: bookManager.dp(12)
                        title: qsTr("Bố cục cột") + Retranslate.onLanguageChanged

                        Option {
                            id: optColSingle
                            text: qsTr("1 cột (Toàn màn hình)") + Retranslate.onLanguageChanged
                            value: "single"
                            selected: bookManager.columnLayout !== "double"
                        }
                        Option {
                            id: optColDouble
                            text: qsTr("2 cột (Trang sách đôi)") + Retranslate.onLanguageChanged
                            value: "double"
                            selected: bookManager.columnLayout === "double"
                        }

                        onSelectedValueChanged: {
                            if (!readingSettingsPage.isSyncing && selectedValue && selectedValue !== bookManager.columnLayout) {
                                bookManager.setColumnLayout(selectedValue);
                            }
                        }
                    }
                }

                // Section 2: Truyện tranh & Manga
                Header {
                    title: qsTr("TRUYỆN TRANH & MANGA") + Retranslate.onLanguageChanged
                }

                ToggleableListItem {
                    title: qsTr("Chế độ Manga (Phải qua Trái)") + Retranslate.onLanguageChanged
                    subtitle: qsTr("Chạm bên trái để lật trang tiếp theo, vuốt phải sang trái để đọc") + Retranslate.onLanguageChanged
                    checked: bookManager.mangaRtlMode
                    showDivider: false
                    onToggled: {
                        if (checked !== bookManager.mangaRtlMode) {
                            bookManager.mangaRtlMode = checked;
                        }
                    }
                }

                // Section 3: Mục tiêu & Phím cứng
                Header {
                    topMargin: bookManager.dp(8)
                    title: qsTr("MỤC TIÊU & PHÍM CỨNG") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(8)
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)

                    DropDown {
                        id: dailyGoalDropDown
                        title: qsTr("Mục tiêu đọc hằng ngày") + Retranslate.onLanguageChanged

                        Option {
                            text: "15 " + (bookManager.language === "en" ? "min" : "phút")
                            value: 15
                            selected: bookManager.dailyGoalMinutes === 15
                        }
                        Option {
                            text: "30 " + (bookManager.language === "en" ? "min" : "phút")
                            value: 30
                            selected: bookManager.dailyGoalMinutes === 30
                        }
                        Option {
                            text: "45 " + (bookManager.language === "en" ? "min" : "phút")
                            value: 45
                            selected: bookManager.dailyGoalMinutes === 45
                        }
                        Option {
                            text: "60 " + (bookManager.language === "en" ? "min" : "phút")
                            value: 60
                            selected: bookManager.dailyGoalMinutes === 60
                        }
                        Option {
                            text: "90 " + (bookManager.language === "en" ? "min" : "phút")
                            value: 90
                            selected: bookManager.dailyGoalMinutes === 90
                        }

                        onSelectedValueChanged: {
                            if (selectedValue && selectedValue !== bookManager.dailyGoalMinutes) {
                                bookManager.setDailyGoalMinutes(selectedValue);
                            }
                        }
                    }
                }

                ToggleableListItem {
                    title: qsTr("Lật trang bằng phím âm lượng") + Retranslate.onLanguageChanged
                    subtitle: qsTr("Nhấn Giảm âm lượng để sang trang tiếp, Tăng âm lượng để lùi trang (tự nhả khi bật TTS)") + Retranslate.onLanguageChanged
                    checked: bookManager.volumeKeyTurnEnabled
                    showDivider: false
                    onToggled: {
                        if (checked !== bookManager.volumeKeyTurnEnabled) {
                            bookManager.setVolumeKeyTurnEnabled(checked);
                        }
                    }
                }
            }
        }
    }
}
