import bb.cascades 1.3

Sheet {
    id: settingsSheet

    NavigationPane {
        id: settingsNavPane

        Page {
            id: mainSettingsPage

            titleBar: TitleBar {
                title: qsTr("Cài Đặt") + Retranslate.onLanguageChanged
                appearance: TitleBarAppearance.Plain
                dismissAction: ActionItem {
                    title: qsTr("Đóng") + Retranslate.onLanguageChanged
                    onTriggered: {
                        settingsSheet.close();
                    }
                }
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
                        bottomPadding: bookManager.dp(28)

                        // =========================================================
                        // GROUP 1: TRẢI NGHIỆM ĐỌC
                        // =========================================================
                        Header {
                            title: qsTr("TRẢI NGHIỆM ĐỌC") + Retranslate.onLanguageChanged
                        }

                        // Row 1: Hiển thị
                        Container {
                            id: rowDisplay
                            property bool pressed: false
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: pressed ? Color.create(bookManager.uiActive) : Color.Transparent
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            onTouch: {
                                if (event.touchType === TouchType.Down) pressed = true;
                                else if (event.touchType === TouchType.Up || event.touchType === TouchType.Cancel) pressed = false;
                            }
                            gestureHandlers: [
                                TapHandler { onTapped: { settingsSheet.openCategory("display"); } }
                            ]

                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: bookManager.dp(32)
                                preferredHeight: bookManager.dp(32)
                                rightMargin: bookManager.dp(14)
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_theme.png" : "asset:///icons/ic_theme_dark.png"
                            }
                            Container {
                                verticalAlignment: VerticalAlignment.Center
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(8)
                                Label {
                                    text: qsTr("Hiển thị") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.PrimaryText
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    bottomMargin: 0
                                }
                                Label {
                                    topMargin: bookManager.dp(2)
                                    multiline: true
                                    text: ((bookManager.appTheme === "dark" ? qsTr("Tối") : qsTr("Sáng")) + " • " + bookManager.currentFontDisplayName + " " + bookManager.fontSize + " px") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                            }
                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: bookManager.dp(20)
                                preferredHeight: bookManager.dp(20)
                                opacity: 0.45
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_next.png" : "asset:///icons/ic_next_dark.png"
                            }
                        }

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            leftMargin: bookManager.dp(62)
                            preferredHeight: 1
                            background: Color.create(bookManager.uiHairline)
                        }

                        // Row 2: Thao tác đọc
                        Container {
                            id: rowReading
                            property bool pressed: false
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: pressed ? Color.create(bookManager.uiActive) : Color.Transparent
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            onTouch: {
                                if (event.touchType === TouchType.Down) pressed = true;
                                else if (event.touchType === TouchType.Up || event.touchType === TouchType.Cancel) pressed = false;
                            }
                            gestureHandlers: [
                                TapHandler { onTapped: { settingsSheet.openCategory("reading"); } }
                            ]

                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: bookManager.dp(32)
                                preferredHeight: bookManager.dp(32)
                                rightMargin: bookManager.dp(14)
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_reading.png" : "asset:///icons/ic_reading_dark.png"
                            }
                            Container {
                                verticalAlignment: VerticalAlignment.Center
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(8)
                                Label {
                                    text: qsTr("Thao tác đọc") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.PrimaryText
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    bottomMargin: 0
                                }
                                Label {
                                    topMargin: bookManager.dp(2)
                                    multiline: true
                                    text: ((bookManager.readingMode === "scroll" ? qsTr("Cuộn") : qsTr("Lật trang")) + " • " + (bookManager.columnLayout === "double" ? qsTr("2 cột") : qsTr("1 cột"))) + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                            }
                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: bookManager.dp(20)
                                preferredHeight: bookManager.dp(20)
                                opacity: 0.45
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_next.png" : "asset:///icons/ic_next_dark.png"
                            }
                        }

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            leftMargin: bookManager.dp(62)
                            preferredHeight: 1
                            background: Color.create(bookManager.uiHairline)
                        }

                        // Row 3: Chân trang
                        Container {
                            id: rowFooter
                            property bool pressed: false
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: pressed ? Color.create(bookManager.uiActive) : Color.Transparent
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            onTouch: {
                                if (event.touchType === TouchType.Down) pressed = true;
                                else if (event.touchType === TouchType.Up || event.touchType === TouchType.Cancel) pressed = false;
                            }
                            gestureHandlers: [
                                TapHandler { onTapped: { settingsSheet.openCategory("footer"); } }
                            ]

                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: bookManager.dp(32)
                                preferredHeight: bookManager.dp(32)
                                rightMargin: bookManager.dp(14)
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_chart.png" : "asset:///icons/ic_chart_dark.png"
                            }
                            Container {
                                verticalAlignment: VerticalAlignment.Center
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(8)
                                Label {
                                    text: qsTr("Chân trang") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.PrimaryText
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    bottomMargin: 0
                                }
                                Label {
                                    topMargin: bookManager.dp(2)
                                    multiline: true
                                    text: qsTr("Tên chương, tiến độ, pin & đồng hồ") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                            }
                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: bookManager.dp(20)
                                preferredHeight: bookManager.dp(20)
                                opacity: 0.45
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_next.png" : "asset:///icons/ic_next_dark.png"
                            }
                        }

                        // =========================================================
                        // GROUP 2: TÍNH NĂNG & KHO SÁCH
                        // =========================================================
                        Header {
                            topMargin: bookManager.dp(8)
                            title: qsTr("TÍNH NĂNG & KHO SÁCH") + Retranslate.onLanguageChanged
                        }

                        // Row 4: Đọc sách (TTS)
                        Container {
                            id: rowTts
                            property bool pressed: false
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: pressed ? Color.create(bookManager.uiActive) : Color.Transparent
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            onTouch: {
                                if (event.touchType === TouchType.Down) pressed = true;
                                else if (event.touchType === TouchType.Up || event.touchType === TouchType.Cancel) pressed = false;
                            }
                            gestureHandlers: [
                                TapHandler { onTapped: { settingsSheet.openCategory("tts"); } }
                            ]

                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: bookManager.dp(32)
                                preferredHeight: bookManager.dp(32)
                                rightMargin: bookManager.dp(14)
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_tts.png" : "asset:///icons/ic_tts_dark.png"
                            }
                            Container {
                                verticalAlignment: VerticalAlignment.Center
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(8)
                                Label {
                                    text: qsTr("Đọc sách (TTS)") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.PrimaryText
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    bottomMargin: 0
                                }
                                Label {
                                    topMargin: bookManager.dp(2)
                                    multiline: true
                                    text: (ttsManager.voiceDisplayName + " • " + Number(ttsManager.speed).toFixed(1) + "x")
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                            }
                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: bookManager.dp(20)
                                preferredHeight: bookManager.dp(20)
                                opacity: 0.45
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_next.png" : "asset:///icons/ic_next_dark.png"
                            }
                        }

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            leftMargin: bookManager.dp(62)
                            preferredHeight: 1
                            background: Color.create(bookManager.uiHairline)
                        }

                        // Row 5: Tủ sách
                        Container {
                            id: rowLibrary
                            property bool pressed: false
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: pressed ? Color.create(bookManager.uiActive) : Color.Transparent
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            onTouch: {
                                if (event.touchType === TouchType.Down) pressed = true;
                                else if (event.touchType === TouchType.Up || event.touchType === TouchType.Cancel) pressed = false;
                            }
                            gestureHandlers: [
                                TapHandler { onTapped: { settingsSheet.openCategory("library"); } }
                            ]

                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: bookManager.dp(32)
                                preferredHeight: bookManager.dp(32)
                                rightMargin: bookManager.dp(14)
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_library.png" : "asset:///icons/ic_library_dark.png"
                            }
                            Container {
                                verticalAlignment: VerticalAlignment.Center
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(8)
                                Label {
                                    text: qsTr("Tủ sách") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.PrimaryText
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    bottomMargin: 0
                                }
                                Label {
                                    topMargin: bookManager.dp(2)
                                    multiline: true
                                    text: ((bookManager.libraryViewMode === "mosaic" ? qsTr("Dạng lưới") : qsTr("Danh sách")) + " • " + bookManager.totalScannedBooks + " " + (bookManager.language === "en" ? (bookManager.totalScannedBooks === 1 ? "book" : "books") : qsTr("cuốn"))) + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                            }
                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: bookManager.dp(20)
                                preferredHeight: bookManager.dp(20)
                                opacity: 0.45
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_next.png" : "asset:///icons/ic_next_dark.png"
                            }
                        }

                        // =========================================================
                        // GROUP 3: ỨNG DỤNG
                        // =========================================================
                        Header {
                            topMargin: bookManager.dp(8)
                            title: qsTr("ỨNG DỤNG") + Retranslate.onLanguageChanged
                        }

                        // Row 6: Giới thiệu
                        Container {
                            id: rowAbout
                            property bool pressed: false
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: pressed ? Color.create(bookManager.uiActive) : Color.Transparent
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            onTouch: {
                                if (event.touchType === TouchType.Down) pressed = true;
                                else if (event.touchType === TouchType.Up || event.touchType === TouchType.Cancel) pressed = false;
                            }
                            gestureHandlers: [
                                TapHandler { onTapped: { settingsSheet.openCategory("about"); } }
                            ]

                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: bookManager.dp(32)
                                preferredHeight: bookManager.dp(32)
                                rightMargin: bookManager.dp(14)
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_info.png" : "asset:///icons/ic_info_dark.png"
                            }
                            Container {
                                verticalAlignment: VerticalAlignment.Center
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(8)
                                Label {
                                    text: qsTr("Giới thiệu") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.PrimaryText
                                    textStyle.color: Color.create(bookManager.uiInk)
                                    bottomMargin: 0
                                }
                                Label {
                                    topMargin: bookManager.dp(2)
                                    multiline: true
                                    text: qsTr("Phiên bản Libris & hướng dẫn") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }
                            }
                            ImageView {
                                verticalAlignment: VerticalAlignment.Center
                                preferredWidth: bookManager.dp(20)
                                preferredHeight: bookManager.dp(20)
                                opacity: 0.45
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_next.png" : "asset:///icons/ic_next_dark.png"
                            }
                        }
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
