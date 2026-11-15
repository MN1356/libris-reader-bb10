import bb.cascades 1.3

Page {
    id: readingSettingsPage

    signal backRequested()

    titleBar: TitleBar {
        title: qsTr("Thao tác đọc") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Default
    }

    ScrollView {
        scrollRole: ScrollRole.Main
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            bottomPadding: 32

            // Section 1: Phương thức chuyển trang
            Header {
                title: qsTr("CHẾ ĐỘ ĐỌC") + Retranslate.onLanguageChanged
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: 12
                bottomPadding: 16
                leftPadding: 16
                rightPadding: 16

                DropDown {
                    id: modeDropDown
                    title: qsTr("Chuyển trang") + Retranslate.onLanguageChanged

                    Option {
                        text: qsTr("Vuốt lật trang (Paged)") + Retranslate.onLanguageChanged
                        imageSource: "asset:///icons/ic_page.png"
                        value: "paged"
                        selected: bookManager.readingMode === "paged"
                    }
                    Option {
                        text: qsTr("Cuộn liên tục (Scroll)") + Retranslate.onLanguageChanged
                        imageSource: "asset:///icons/ic_scroll.png"
                        value: "scroll"
                        selected: bookManager.readingMode === "scroll"
                    }

                    onSelectedValueChanged: {
                        if (selectedValue && selectedValue !== bookManager.readingMode) {
                            bookManager.setReadingMode(selectedValue);
                        }
                    }
                }
            }
        }
    }
}
