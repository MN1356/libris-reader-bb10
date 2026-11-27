import bb.cascades 1.3

Page {
    id: displaySettingsPage

    signal backRequested()
    property variant dynamicFontOptions: []

    function rebuildCustomFontOptions() {
        for (var i = 0; i < dynamicFontOptions.length; ++i) {
            var oldOpt = dynamicFontOptions[i];
            if (oldOpt) {
                fontFamilyDropDown.remove(oldOpt);
                oldOpt.destroy();
            }
        }
        dynamicFontOptions = [];
        var list = bookManager.customFonts;
        var created = [];
        for (var j = 0; j < list.length; ++j) {
            var item = list[j];
            if (!item || !item.value) continue;
            var opt = customFontOptionDef.createObject(fontFamilyDropDown);
            if (opt) {
                opt.text = item.name;
                opt.value = item.value;
                opt.selected = (bookManager.fontFamily === item.value);
                fontFamilyDropDown.add(opt);
                created.push(opt);
            }
        }
        dynamicFontOptions = created;
    }

    onCreationCompleted: {
        rebuildCustomFontOptions();
        bookManager.customFontsChanged.connect(rebuildCustomFontOptions);
    }

    attachedObjects: [
        ComponentDefinition {
            id: customFontOptionDef
            Option {}
        }
    ]

    titleBar: TitleBar {
        title: qsTr("Hiển thị & Giao diện") + Retranslate.onLanguageChanged
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

                // =========================================================
                // Section 1: GIAO DIỆN ỨNG DỤNG
                // =========================================================
                Header {
                    title: qsTr("GIAO DIỆN ỨNG DỤNG") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(16)

                    DropDown {
                        id: appThemeDropDown
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
                        topMargin: bookManager.dp(12)
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
                }

                // =========================================================
                // Section 1B: TRONG SÁCH (IN BOOKS)
                // =========================================================
                Header {
                    title: qsTr("TRONG SÁCH") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(16)

                    DropDown {
                        id: themeDropDown
                        title: qsTr("Màu nền trang đọc") + Retranslate.onLanguageChanged

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
                }

                // =========================================================
                // Section 2: CỠ CHỮ & GIÃN DÒNG (hidden on fixed-layout PDF)
                // =========================================================
                Container {
                    visible: !bookManager.isPdf || bookManager.isPdfReflow
                    horizontalAlignment: HorizontalAlignment.Fill

                    Header {
                        title: qsTr("CỠ CHỮ & GIÃN DÒNG") + Retranslate.onLanguageChanged
                    }

                    SliderStepperRow {
                        labelText: qsTr("Cỡ chữ:") + Retranslate.onLanguageChanged
                        unit: " px"
                        currentValue: bookManager.fontSize
                        minValue: 10
                        maxValue: 36
                        step: 1
                        onValueSelected: {
                            bookManager.setFontSize(newValue);
                        }
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        bottomPadding: bookManager.dp(16)

                        DropDown {
                            id: fontFamilyDropDown
                            topMargin: bookManager.dp(4)
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
                            topMargin: bookManager.dp(12)
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

                        DropDown {
                            topMargin: bookManager.dp(10)
                            title: qsTr("Căn lề văn bản") + Retranslate.onLanguageChanged
                            Option {
                                text: qsTr("Căn đều hai bên") + Retranslate.onLanguageChanged
                                value: "justify"
                                selected: bookManager.textAlign !== "left"
                            }
                            Option {
                                text: qsTr("Căn trái") + Retranslate.onLanguageChanged
                                value: "left"
                                selected: bookManager.textAlign === "left"
                            }
                            onSelectedValueChanged: {
                                if (selectedValue && selectedValue !== bookManager.textAlign) {
                                    bookManager.setTextAlign(selectedValue);
                                }
                            }
                        }

                        ToggleableListItem {
                            topMargin: bookManager.dp(10)
                            title: qsTr("Bỏ CSS gốc của sách") + Retranslate.onLanguageChanged
                            subtitle: qsTr("Dùng định dạng chuẩn của Libris thay cho CSS nhúng") + Retranslate.onLanguageChanged
                            checked: bookManager.overridePublisherCss
                            onToggled: {
                                bookManager.setOverridePublisherCss(checked);
                            }
                        }

                        DropDown {
                            topMargin: bookManager.dp(10)
                            title: qsTr("Thụt đầu dòng") + Retranslate.onLanguageChanged
                            Option {
                                text: qsTr("Không thụt (0)") + Retranslate.onLanguageChanged
                                value: "none"
                                selected: bookManager.paragraphIndent === "none"
                            }
                            Option {
                                text: qsTr("Chuẩn (1.5em)") + Retranslate.onLanguageChanged
                                value: "normal"
                                selected: bookManager.paragraphIndent === "normal"
                            }
                            Option {
                                text: qsTr("Sâu (2.2em)") + Retranslate.onLanguageChanged
                                value: "deep"
                                selected: bookManager.paragraphIndent === "deep"
                            }
                            onSelectedValueChanged: {
                                if (selectedValue && selectedValue !== bookManager.paragraphIndent) {
                                    bookManager.setParagraphIndent(selectedValue);
                                }
                            }
                        }

                        DropDown {
                            topMargin: bookManager.dp(10)
                            title: qsTr("Khoảng cách đoạn") + Retranslate.onLanguageChanged
                            Option {
                                text: qsTr("Gọn") + Retranslate.onLanguageChanged
                                value: "compact"
                                selected: bookManager.paragraphSpacing === "compact"
                            }
                            Option {
                                text: qsTr("Chuẩn") + Retranslate.onLanguageChanged
                                value: "normal"
                                selected: bookManager.paragraphSpacing === "normal"
                            }
                            Option {
                                text: qsTr("Thoáng") + Retranslate.onLanguageChanged
                                value: "wide"
                                selected: bookManager.paragraphSpacing === "wide"
                            }
                            onSelectedValueChanged: {
                                if (selectedValue && selectedValue !== bookManager.paragraphSpacing) {
                                    bookManager.setParagraphSpacing(selectedValue);
                                }
                            }
                        }

                        DropDown {
                            topMargin: bookManager.dp(10)
                            title: qsTr("Độ đậm chữ") + Retranslate.onLanguageChanged
                            Option {
                                text: qsTr("Thường (400)") + Retranslate.onLanguageChanged
                                value: 400
                                selected: bookManager.readerFontWeight === 400
                            }
                            Option {
                                text: qsTr("Vừa (500)") + Retranslate.onLanguageChanged
                                value: 500
                                selected: bookManager.readerFontWeight === 500
                            }
                            Option {
                                text: qsTr("Đậm (600)") + Retranslate.onLanguageChanged
                                value: 600
                                selected: bookManager.readerFontWeight === 600
                            }
                            onSelectedValueChanged: {
                                if (selectedValue && selectedValue !== bookManager.readerFontWeight) {
                                    bookManager.setReaderFontWeight(selectedValue);
                                }
                            }
                        }

                        DropDown {
                            topMargin: bookManager.dp(10)
                            title: qsTr("Độ sáng chữ nền Tối") + Retranslate.onLanguageChanged
                            Option {
                                text: qsTr("Sáng rõ") + Retranslate.onLanguageChanged
                                value: "bright"
                                selected: bookManager.darkTextTone === "bright"
                            }
                            Option {
                                text: qsTr("Dịu mắt") + Retranslate.onLanguageChanged
                                value: "soft"
                                selected: bookManager.darkTextTone === "soft"
                            }
                            Option {
                                text: qsTr("Rất dịu (đọc đêm)") + Retranslate.onLanguageChanged
                                value: "dim"
                                selected: bookManager.darkTextTone === "dim"
                            }
                            onSelectedValueChanged: {
                                if (selectedValue && selectedValue !== bookManager.darkTextTone) {
                                    bookManager.setDarkTextTone(selectedValue);
                                }
                            }
                        }

                        // Custom .ttf / .otf Fonts Card
                        Container {
                            topMargin: bookManager.dp(12)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            leftPadding: bookManager.dp(14)
                            rightPadding: bookManager.dp(14)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            Label {
                                multiline: true
                                text: qsTr("Phông chữ tùy chỉnh (.ttf / .otf)") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.fontWeight: FontWeight.W500
                                textStyle.color: Color.create(bookManager.uiInk)
                            }

                            Label {
                                topMargin: bookManager.dp(3)
                                multiline: true
                                text: (qsTr("Thư mục phông chữ:") + " /accounts/1000/shared/documents/fonts\n" + qsTr("Đã tìm thấy %1 phông chữ tùy chỉnh").arg(bookManager.customFonts.length)) + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }

                            Button {
                                topMargin: bookManager.dp(10)
                                horizontalAlignment: HorizontalAlignment.Fill
                                text: qsTr("Quét lại phông chữ") + Retranslate.onLanguageChanged
                                onClicked: {
                                    bookManager.refreshCustomFonts();
                                }
                            }
                        }

                        // Live typography & margin preview card (uses actual reader canvas theme colors)
                        Container {
                            topMargin: bookManager.dp(12)
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
                                textStyle.fontSizeValue: Math.max(5.5, Math.min(16.0, bookManager.fontSize * 0.45))
                                textStyle.color: bookManager.theme === "dark" ? Color.create("#E0E0E0") :
                                                 (bookManager.theme === "sepia" ? Color.create("#382818") : Color.create("#111111"))
                                multiline: true
                            }
                        }
                    }
                }

                // =========================================================
                // Section 3: BẢO VỆ MẮT & TỰ ĐỘNG
                // =========================================================
                Header {
                    title: qsTr("BẢO VỆ MẮT & TỰ ĐỘNG") + Retranslate.onLanguageChanged
                }

                ToggleableListItem {
                    title: qsTr("Bionic Reading (Đọc nhanh)") + Retranslate.onLanguageChanged
                    subtitle: qsTr("Làm đậm nửa đầu mỗi từ giúp mắt bắt nhịp đọc nhanh hơn") + Retranslate.onLanguageChanged
                    checked: bookManager.bionicReading
                    onToggled: {
                        bookManager.setBionicReading(checked);
                    }
                }

                ToggleableListItem {
                    title: qsTr("Lọc dịu mắt (Ánh sáng ấm)") + Retranslate.onLanguageChanged
                    subtitle: qsTr("Áp lớp lọc ánh sáng ấm dịu mắt khi đọc ban đêm") + Retranslate.onLanguageChanged
                    checked: bookManager.warmFilterEnabled
                    showDivider: !bookManager.warmFilterEnabled
                    onToggled: {
                        bookManager.setWarmFilterEnabled(checked);
                    }
                }

                Container {
                    visible: bookManager.warmFilterEnabled
                    horizontalAlignment: HorizontalAlignment.Fill

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        topPadding: bookManager.dp(4)
                        bottomPadding: bookManager.dp(10)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                            Label {
                                text: qsTr("Cường độ lọc ấm") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            }
                            Label {
                                text: bookManager.warmFilterIntensity + "%"
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.fontWeight: FontWeight.W500
                                textStyle.color: Color.create(bookManager.uiAccent)
                            }
                        }

                        Slider {
                            id: warmIntensitySlider
                            horizontalAlignment: HorizontalAlignment.Fill
                            fromValue: 10
                            toValue: 60
                            value: bookManager.warmFilterIntensity
                            onImmediateValueChanged: {
                                bookManager.setWarmFilterIntensity(Math.round(immediateValue));
                            }
                        }
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftMargin: bookManager.dp(16)
                        preferredHeight: 1
                        background: Color.create(bookManager.uiHairline)
                    }
                }

                ToggleableListItem {
                    title: qsTr("Chế độ Tập trung (Thước đọc)") + Retranslate.onLanguageChanged
                    subtitle: qsTr("Làm mờ vùng chữ xung quanh, chạm để dời dải đọc") + Retranslate.onLanguageChanged
                    checked: bookManager.focusModeEnabled
                    onToggled: {
                        bookManager.setFocusModeEnabled(checked);
                    }
                }

                ToggleableListItem {
                    title: qsTr("Tự động chế độ Đêm (20:00 - 06:00)") + Retranslate.onLanguageChanged
                    subtitle: qsTr("Tự chuyển nền đọc Tối AMOLED vào buổi tối và Sepia ban ngày") + Retranslate.onLanguageChanged
                    checked: bookManager.autoNightTheme
                    showDivider: true
                    onToggled: {
                        bookManager.setAutoNightTheme(checked);
                    }
                }

                // Per-Format Reader Screen Brightness
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(10)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        Label {
                            text: (qsTr("Độ sáng màn hình đọc") + (bookManager.currentFormat !== "" ? (" (" + bookManager.currentFormat.toUpperCase() + ")") : "")) + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create(bookManager.uiInk)
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        }
                        Label {
                            text: bookManager.readerBrightness + "%"
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.fontWeight: FontWeight.Bold
                            textStyle.color: Color.create(bookManager.uiAccent)
                        }
                    }

                    Label {
                        topMargin: bookManager.dp(2)
                        text: qsTr("Lưu riêng cho từng định dạng sách (EPUB, PDF, CBZ...)") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create(bookManager.uiMuted)
                        multiline: true
                    }

                    Slider {
                        id: settingsBrightnessSlider
                        topMargin: bookManager.dp(6)
                        horizontalAlignment: HorizontalAlignment.Fill
                        fromValue: 20
                        toValue: 100
                        value: bookManager.readerBrightness
                        onImmediateValueChanged: {
                            var v = Math.round(immediateValue);
                            if (v !== bookManager.readerBrightness) {
                                bookManager.setReaderBrightness(v);
                            }
                        }
                    }
                }

                // =========================================================
                // Section 4: CĂN LỀ
                // =========================================================
                Header {
                    topMargin: bookManager.dp(8)
                    title: qsTr("CĂN LỀ") + Retranslate.onLanguageChanged
                }

                SliderStepperRow {
                    labelText: qsTr("Trên:") + Retranslate.onLanguageChanged
                    unit: " px"
                    currentValue: bookManager.marginTop
                    minValue: 0
                    maxValue: 60
                    step: 2
                    showDivider: true
                    onValueSelected: {
                        bookManager.setMarginTop(newValue);
                    }
                }

                SliderStepperRow {
                    labelText: qsTr("Dưới:") + Retranslate.onLanguageChanged
                    unit: " px"
                    currentValue: bookManager.marginBottom
                    minValue: 0
                    maxValue: 60
                    step: 2
                    showDivider: true
                    onValueSelected: {
                        bookManager.setMarginBottom(newValue);
                    }
                }

                SliderStepperRow {
                    labelText: qsTr("Trái:") + Retranslate.onLanguageChanged
                    unit: " px"
                    currentValue: bookManager.marginLeft
                    minValue: 0
                    maxValue: 60
                    step: 2
                    showDivider: true
                    onValueSelected: {
                        bookManager.setMarginLeft(newValue);
                    }
                }

                SliderStepperRow {
                    labelText: qsTr("Phải:") + Retranslate.onLanguageChanged
                    unit: " px"
                    currentValue: bookManager.marginRight
                    minValue: 0
                    maxValue: 60
                    step: 2
                    showDivider: false
                    onValueSelected: {
                        bookManager.setMarginRight(newValue);
                    }
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(6)
                    bottomPadding: bookManager.dp(12)

                    Button {
                        horizontalAlignment: HorizontalAlignment.Center
                        text: qsTr("Đặt lại viền (16px)") + Retranslate.onLanguageChanged
                        onClicked: {
                            bookManager.resetMargins();
                        }
                    }
                }

                // =========================================================
                // Section 5: TÙY CHỌN PDF (khi đọc PDF nguyên bản)
                // =========================================================
                Container {
                    visible: bookManager.isPdf && !bookManager.isPdfReflow
                    horizontalAlignment: HorizontalAlignment.Fill
                    topMargin: bookManager.dp(8)

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
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        topPadding: bookManager.dp(12)
                        bottomPadding: bookManager.dp(12)
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        Label {
                            text: qsTr("Xoay trang:") + Retranslate.onLanguageChanged
                            verticalAlignment: VerticalAlignment.Center
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textStyle.color: Color.create(bookManager.uiInk)
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        }

                        Button {
                            text: qsTr("Xoay 90°") + Retranslate.onLanguageChanged
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
}
