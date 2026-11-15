import bb.cascades 1.3

Page {
    id: displaySettingsPage

    signal backRequested()

    titleBar: TitleBar {
        title: qsTr("Hiển thị & Giao diện") + Retranslate.onLanguageChanged
        appearance: TitleBarAppearance.Default
    }

    ScrollView {
        scrollRole: ScrollRole.Main
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
            topPadding: 16
            leftPadding: 16
            rightPadding: 16
            bottomPadding: 32

            // Section 1: Giao diện chủ đề
            Header {
                title: qsTr("GIAO DIỆN") + Retranslate.onLanguageChanged
            }

            DropDown {
                id: appThemeDropDown
                topMargin: 8
                title: qsTr("Chủ đề") + Retranslate.onLanguageChanged

                Option {
                    text: qsTr("Tối") + Retranslate.onLanguageChanged
                    value: "dark"
                    selected: bookManager.appTheme === "dark"
                }
                Option {
                    text: qsTr("Sáng") + Retranslate.onLanguageChanged
                    value: "light"
                    selected: bookManager.appTheme === "light"
                }

                onSelectedValueChanged: {
                    if (selectedValue && selectedValue !== bookManager.appTheme) {
                        bookManager.setAppTheme(selectedValue);
                    }
                }
            }

            DropDown {
                id: appLanguageDropDown
                topMargin: 12
                title: qsTr("Ngôn ngữ") + Retranslate.onLanguageChanged

                Option {
                    text: "Tiếng Việt"
                    value: "vi"
                    selected: bookManager.language === "vi"
                }
                Option {
                    text: "English"
                    value: "en"
                    selected: bookManager.language === "en"
                }

                onSelectedValueChanged: {
                    if (selectedValue && selectedValue !== bookManager.language) {
                        bookManager.setLanguage(selectedValue);
                    }
                }
            }

            DropDown {
                id: themeDropDown
                topMargin: 12
                title: qsTr("Màu nền") + Retranslate.onLanguageChanged

                Option {
                    text: qsTr("Tối") + Retranslate.onLanguageChanged
                    value: "dark"
                    selected: bookManager.theme === "dark"
                }
                Option {
                    text: qsTr("Vàng dịu") + Retranslate.onLanguageChanged
                    value: "sepia"
                    selected: bookManager.theme === "sepia"
                }
                Option {
                    text: qsTr("Sáng") + Retranslate.onLanguageChanged
                    value: "light"
                    selected: bookManager.theme === "light"
                }

                onSelectedValueChanged: {
                    if (selectedValue && selectedValue !== bookManager.theme) {
                        bookManager.setTheme(selectedValue);
                    }
                }
            }

            // Section 2: Cỡ chữ & Giãn dòng
            Header {
                topMargin: 20
                title: qsTr("CỠ CHỮ & GIÃN DÒNG") + Retranslate.onLanguageChanged
            }

            Container {
                visible: !bookManager.isPdf || bookManager.isPdfReflow
                horizontalAlignment: HorizontalAlignment.Fill
                topMargin: 8

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    Label {
                        text: qsTr("Cỡ chữ:") + Retranslate.onLanguageChanged
                        verticalAlignment: VerticalAlignment.Center
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                    }
                    Label {
                        text: bookManager.fontSize + " px"
                        verticalAlignment: VerticalAlignment.Center
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                        textStyle.fontWeight: FontWeight.Bold
                        rightMargin: 12
                    }
                    Button {
                        text: "−"
                        preferredWidth: 54
                        preferredHeight: 44
                        minHeight: 44
                        verticalAlignment: VerticalAlignment.Center
                        enabled: bookManager.fontSize > 10
                        onClicked: {
                            bookManager.setFontSize(bookManager.fontSize - 1);
                        }
                    }
                    Button {
                        text: "+"
                        preferredWidth: 54
                        preferredHeight: 44
                        minHeight: 44
                        leftMargin: 6
                        verticalAlignment: VerticalAlignment.Center
                        enabled: bookManager.fontSize < 36
                        onClicked: {
                            bookManager.setFontSize(bookManager.fontSize + 1);
                        }
                    }
                }

                Slider {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topMargin: 2
                    fromValue: 10
                    toValue: 36
                    value: bookManager.fontSize
                    onImmediateValueChanged: {
                        bookManager.setFontSize(Math.round(immediateValue));
                    }
                }
            }

            DropDown {
                id: fontSizeDropDown
                visible: !bookManager.isPdf || bookManager.isPdfReflow
                topMargin: 4
                title: qsTr("Cỡ chữ mẫu") + Retranslate.onLanguageChanged

                Option {
                    text: qsTr("Rất nhỏ") + Retranslate.onLanguageChanged
                    description: "14 px"
                    value: 14
                    selected: bookManager.fontSize === 14
                }
                Option {
                    text: qsTr("Nhỏ") + Retranslate.onLanguageChanged
                    description: "16 px"
                    value: 16
                    selected: bookManager.fontSize === 16
                }
                Option {
                    text: qsTr("Chuẩn") + Retranslate.onLanguageChanged
                    description: "18 px"
                    value: 18
                    selected: bookManager.fontSize === 18
                }
                Option {
                    text: qsTr("Lớn") + Retranslate.onLanguageChanged
                    description: "22 px"
                    value: 22
                    selected: bookManager.fontSize === 22
                }
                Option {
                    text: qsTr("Rất lớn") + Retranslate.onLanguageChanged
                    description: "26 px"
                    value: 26
                    selected: bookManager.fontSize === 26
                }

                onSelectedValueChanged: {
                    if (selectedValue && selectedValue !== bookManager.fontSize) {
                        bookManager.setFontSize(selectedValue);
                    }
                }
            }

            DropDown {
                id: fontFamilyDropDown
                visible: !bookManager.isPdf || bookManager.isPdfReflow
                topMargin: 12
                title: qsTr("Phông chữ") + Retranslate.onLanguageChanged

                Option {
                    text: qsTr("Mặc định (Slate Pro)") + Retranslate.onLanguageChanged
                    value: "sans"
                    selected: bookManager.fontFamily === "sans" || bookManager.fontFamily === ""
                }
                Option {
                    text: "Georgia"
                    value: "serif"
                    selected: bookManager.fontFamily === "serif"
                }
                Option {
                    text: "Times New Roman"
                    value: "times"
                    selected: bookManager.fontFamily === "times"
                }
                Option {
                    text: "Garamond"
                    value: "garamond"
                    selected: bookManager.fontFamily === "garamond"
                }
                Option {
                    text: "Courier New"
                    value: "monospace"
                    selected: bookManager.fontFamily === "monospace"
                }
                Option {
                    text: qsTr("Nguyên bản sách") + Retranslate.onLanguageChanged
                    value: "original"
                    selected: bookManager.fontFamily === "original"
                }

                onSelectedValueChanged: {
                    if (selectedValue && selectedValue !== bookManager.fontFamily) {
                        bookManager.setFontFamily(selectedValue);
                    }
                }
            }

            DropDown {
                id: lineSpacingDropDown
                topMargin: 12
                title: qsTr("Cách dòng") + Retranslate.onLanguageChanged

                Option {
                    text: qsTr("Hẹp") + Retranslate.onLanguageChanged
                    description: "1.2x"
                    value: "tight"
                    selected: bookManager.lineSpacing === "tight"
                }
                Option {
                    text: qsTr("Chuẩn") + Retranslate.onLanguageChanged
                    description: "1.5x"
                    value: "normal"
                    selected: bookManager.lineSpacing === "normal"
                }
                Option {
                    text: qsTr("Rộng") + Retranslate.onLanguageChanged
                    description: "1.8x"
                    value: "loose"
                    selected: bookManager.lineSpacing === "loose"
                }

                onSelectedValueChanged: {
                    if (selectedValue && selectedValue !== bookManager.lineSpacing) {
                        bookManager.setLineSpacing(selectedValue);
                    }
                }
            }

            // Section 3: Căn lề viền trang sách
            Header {
                topMargin: 20
                title: qsTr("CĂN LỀ") + Retranslate.onLanguageChanged
            }

            // Row 1: Viền trên
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: 8
                bottomPadding: 4

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    Label {
                        text: qsTr("Trên:") + Retranslate.onLanguageChanged
                        verticalAlignment: VerticalAlignment.Center
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                    }
                    Label {
                        text: bookManager.marginTop + " px"
                        verticalAlignment: VerticalAlignment.Center
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                        textStyle.fontWeight: FontWeight.Bold
                        rightMargin: 12
                    }
                    Button {
                        text: "−"
                        preferredWidth: 54
                        preferredHeight: 44
                        minHeight: 44
                        verticalAlignment: VerticalAlignment.Center
                        enabled: bookManager.marginTop > 0
                        onClicked: {
                            bookManager.setMarginTop(bookManager.marginTop - 2);
                        }
                    }
                    Button {
                        text: "+"
                        preferredWidth: 54
                        preferredHeight: 44
                        minHeight: 44
                        leftMargin: 6
                        verticalAlignment: VerticalAlignment.Center
                        enabled: bookManager.marginTop < 60
                        onClicked: {
                            bookManager.setMarginTop(bookManager.marginTop + 2);
                        }
                    }
                }

                Slider {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topMargin: 2
                    fromValue: 0
                    toValue: 60
                    value: bookManager.marginTop
                    onImmediateValueChanged: {
                        bookManager.setMarginTop(Math.round(immediateValue));
                    }
                }
            }

            Divider {}

            // Row 2: Viền dưới
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: 8
                bottomPadding: 4

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    Label {
                        text: qsTr("Dưới:") + Retranslate.onLanguageChanged
                        verticalAlignment: VerticalAlignment.Center
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                    }
                    Label {
                        text: bookManager.marginBottom + " px"
                        verticalAlignment: VerticalAlignment.Center
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                        textStyle.fontWeight: FontWeight.Bold
                        rightMargin: 12
                    }
                    Button {
                        text: "−"
                        preferredWidth: 54
                        preferredHeight: 44
                        minHeight: 44
                        verticalAlignment: VerticalAlignment.Center
                        enabled: bookManager.marginBottom > 0
                        onClicked: {
                            bookManager.setMarginBottom(bookManager.marginBottom - 2);
                        }
                    }
                    Button {
                        text: "+"
                        preferredWidth: 54
                        preferredHeight: 44
                        minHeight: 44
                        leftMargin: 6
                        verticalAlignment: VerticalAlignment.Center
                        enabled: bookManager.marginBottom < 60
                        onClicked: {
                            bookManager.setMarginBottom(bookManager.marginBottom + 2);
                        }
                    }
                }

                Slider {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topMargin: 2
                    fromValue: 0
                    toValue: 60
                    value: bookManager.marginBottom
                    onImmediateValueChanged: {
                        bookManager.setMarginBottom(Math.round(immediateValue));
                    }
                }
            }

            Divider {}

            // Row 3: Viền trái
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: 8
                bottomPadding: 4

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    Label {
                        text: qsTr("Trái:") + Retranslate.onLanguageChanged
                        verticalAlignment: VerticalAlignment.Center
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                    }
                    Label {
                        text: bookManager.marginLeft + " px"
                        verticalAlignment: VerticalAlignment.Center
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                        textStyle.fontWeight: FontWeight.Bold
                        rightMargin: 12
                    }
                    Button {
                        text: "−"
                        preferredWidth: 54
                        preferredHeight: 44
                        minHeight: 44
                        verticalAlignment: VerticalAlignment.Center
                        enabled: bookManager.marginLeft > 0
                        onClicked: {
                            bookManager.setMarginLeft(bookManager.marginLeft - 2);
                        }
                    }
                    Button {
                        text: "+"
                        preferredWidth: 54
                        preferredHeight: 44
                        minHeight: 44
                        leftMargin: 6
                        verticalAlignment: VerticalAlignment.Center
                        enabled: bookManager.marginLeft < 60
                        onClicked: {
                            bookManager.setMarginLeft(bookManager.marginLeft + 2);
                        }
                    }
                }

                Slider {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topMargin: 2
                    fromValue: 0
                    toValue: 60
                    value: bookManager.marginLeft
                    onImmediateValueChanged: {
                        bookManager.setMarginLeft(Math.round(immediateValue));
                    }
                }
            }

            Divider {}

            // Row 4: Viền phải
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: 8
                bottomPadding: 4

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    Label {
                        text: qsTr("Phải:") + Retranslate.onLanguageChanged
                        verticalAlignment: VerticalAlignment.Center
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                    }
                    Label {
                        text: bookManager.marginRight + " px"
                        verticalAlignment: VerticalAlignment.Center
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                        textStyle.fontWeight: FontWeight.Bold
                        rightMargin: 12
                    }
                    Button {
                        text: "−"
                        preferredWidth: 54
                        preferredHeight: 44
                        minHeight: 44
                        verticalAlignment: VerticalAlignment.Center
                        enabled: bookManager.marginRight > 0
                        onClicked: {
                            bookManager.setMarginRight(bookManager.marginRight - 2);
                        }
                    }
                    Button {
                        text: "+"
                        preferredWidth: 54
                        preferredHeight: 44
                        minHeight: 44
                        leftMargin: 6
                        verticalAlignment: VerticalAlignment.Center
                        enabled: bookManager.marginRight < 60
                        onClicked: {
                            bookManager.setMarginRight(bookManager.marginRight + 2);
                        }
                    }
                }

                Slider {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topMargin: 2
                    fromValue: 0
                    toValue: 60
                    value: bookManager.marginRight
                    onImmediateValueChanged: {
                        bookManager.setMarginRight(Math.round(immediateValue));
                    }
                }
            }

            Button {
                topMargin: 8
                bottomMargin: 8
                horizontalAlignment: HorizontalAlignment.Center
                text: qsTr("Đặt lại viền (16px)") + Retranslate.onLanguageChanged
                onClicked: {
                    bookManager.resetMargins();
                }
            }

            // Sample preview box
            Container {
                topMargin: 12
                horizontalAlignment: HorizontalAlignment.Fill
                background: bookManager.theme === "dark" ? Color.create("#161616") : 
                            (bookManager.theme === "sepia" ? Color.create("#F4ECD8") : Color.create("#FFFFFF"))
                leftPadding: bookManager.marginLeft
                rightPadding: bookManager.marginRight
                topPadding: bookManager.marginTop
                bottomPadding: bookManager.marginBottom

                Label {
                    text: qsTr("Văn bản mẫu xem trước hiển thị.") + Retranslate.onLanguageChanged
                    textStyle.fontSize: FontSize.PointValue
                    textStyle.fontSizeValue: bookManager.fontSize
                    textStyle.color: bookManager.theme === "dark" ? Color.create("#E0E0E0") : 
                                     (bookManager.theme === "sepia" ? Color.create("#382818") : Color.create("#111111"))
                    multiline: true
                }
            }

            // Section 4: Tùy chọn PDF (khi đọc PDF)
            Container {
                visible: bookManager.isPdf && !bookManager.isPdfReflow
                horizontalAlignment: HorizontalAlignment.Fill
                topMargin: 20

                Header {
                    title: qsTr("TÙY CHỌN PDF") + Retranslate.onLanguageChanged
                }

                ToggleableListItem {
                    title: qsTr("Đảo màu trang") + Retranslate.onLanguageChanged
                    subtitle: qsTr("Đảo màu văn bản và nền tài liệu PDF") + Retranslate.onLanguageChanged
                    checked: bookManager.isPdfInverted
                    onToggled: {
                        if (checked !== bookManager.isPdfInverted) {
                            bookManager.togglePdfInvert();
                        }
                    }
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topPadding: 8
                    bottomPadding: 8
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    Label {
                        text: qsTr("Xoay trang:") + Retranslate.onLanguageChanged
                        verticalAlignment: VerticalAlignment.Center
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                    }
                    Button {
                        text: qsTr("Xoay 90°") + Retranslate.onLanguageChanged
                        preferredWidth: 120
                        verticalAlignment: VerticalAlignment.Center
                        onClicked: {
                            bookManager.rotatePdf();
                        }
                    }
                }
            }
        }
    }
}
