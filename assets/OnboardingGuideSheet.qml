import bb.cascades 1.3

Sheet {
    id: onboardingSheet

    property int currentStep: 0
    property int totalSteps: 7
    property double swipeStartX: -1

    // Page 3 interactive source tab ("sd" | "wifi" | "opds")
    property string activeBookSource: "sd"

    // Page 4 Visual BlackBerry Keymap selected key & hardware model ("passport" | "q20" | "q10")
    property string selectedKey: "SPACE"
    property string keyboardModel: bookManager.isHiRes ? "passport" : "q10"

    // Page 5 interactive reading tools tab ("highlight" | "dict" | "tts")
    property string activeToolDemo: "highlight"

    function stepNext() {
        if (currentStep < (totalSteps - 1)) {
            currentStep += 1;
        } else {
            finishOnboarding();
        }
    }

    function stepPrev() {
        if (currentStep > 0) {
            currentStep -= 1;
        }
    }

    function openFromStart() {
        currentStep = 0;
        swipeStartX = -1;
        activeBookSource = "sd";
        selectedKey = "SPACE";
        keyboardModel = bookManager.isHiRes ? "passport" : "q10";
        activeToolDemo = "highlight";
        onboardingSheet.open();
    }

    function finishOnboarding() {
        bookManager.setOnboardingCompleted(true);
        onboardingSheet.close();
    }

    function keyBadgeLabel(k) {
        if (k === "SPACE") return "Space";
        if (k === "BKSP") return "Bksp";
        if (k === "ENT") return "Enter";
        return k;
    }

    function keyCategory(k) {
        if (k === "Q" || k === "W") return qsTr("PDF");
        if (k === "Z" || k === "X" || k === "D" || k === "C" || k === "V" || k === "ENT") return qsTr("GIAO DIỆN");
        if (k === "R" || k === "I" || k === "O" || k === "A" || k === "S" || k === "F" || k === "H" || k === "L" || k === "M") return qsTr("CÔNG CỤ");
        return qsTr("ĐIỀU HƯỚNG");
    }

    function keyTitle(k) {
        if (k === "Q") return qsTr("Khóa cuộn ngang PDF");
        if (k === "W") return qsTr("Bật / tắt PDF Reflow");
        if (k === "R") return qsTr("Reading Ruler / Xoay PDF");
        if (k === "T") return qsTr("Lên đầu trang / chương");
        if (k === "U") return qsTr("Quay lại vị trí trước");
        if (k === "I") return qsTr("Thông tin sách");
        if (k === "O") return qsTr("Bảng phím tắt");
        if (k === "P") return qsTr("Chương trước");
        if (k === "A") return qsTr("Đọc thành tiếng (TTS)");
        if (k === "S") return qsTr("Mở Cài đặt");
        if (k === "D") return qsTr("Đổi màu nền đọc");
        if (k === "F") return qsTr("Tìm kiếm trong sách");
        if (k === "G") return qsTr("Mở Mục lục (TOC)");
        if (k === "H") return qsTr("Đánh dấu & Ghi chú");
        if (k === "J") return qsTr("Trang sau / Cuộn xuống");
        if (k === "K") return qsTr("Trang trước / Cuộn lên");
        if (k === "L") return qsTr("Tra từ điển");
        if (k === "BKSP") return qsTr("Trang trước");
        if (k === "Z") return qsTr("Tăng cỡ chữ");
        if (k === "X") return qsTr("Giảm cỡ chữ");
        if (k === "C") return qsTr("Đổi kiểu đọc (Trang / Cuộn)");
        if (k === "V") return qsTr("Bionic Reading");
        if (k === "SPACE") return qsTr("Trang sau / Tạm dừng TTS");
        if (k === "B") return qsTr("Xuống cuối trang / chương");
        if (k === "N") return qsTr("Chương sau");
        if (k === "M") return qsTr("Đánh dấu trang (Bookmark)");
        return qsTr("Chế độ toàn màn hình");
    }

    function keyDesc(k) {
        if (k === "Q") return qsTr("Giữ cố định chiều ngang khi phóng to trang PDF.");
        if (k === "W") return qsTr("Chuyển PDF sang dạng văn bản cuộn dễ đọc hơn.");
        if (k === "R") return qsTr("Bật thước tập trung dòng đọc hoặc xoay ngang PDF.");
        if (k === "T") return qsTr("Nhảy nhanh về đầu chương hoặc đầu trang hiện tại.");
        if (k === "U") return qsTr("Trở về vị trí vừa đọc trước khi nhảy mục lục.");
        if (k === "I") return qsTr("Xem chi tiết sách, tiến độ và thống kê đọc.");
        if (k === "O") return qsTr("Mở hoặc ẩn nhanh danh sách toàn bộ phím tắt.");
        if (k === "P") return qsTr("Chuyển lùi về chương trước đó trong sách.");
        if (k === "A") return qsTr("Bật hoặc tắt đọc sách tự động bằng giọng nói.");
        if (k === "S") return qsTr("Mở nhanh trang cài đặt giao diện và đọc sách.");
        if (k === "D") return qsTr("Chuyển qua lại giữa nền Tối, Sepia và Sáng.");
        if (k === "F") return qsTr("Tìm từ khóa hoặc cụm từ trong toàn bộ cuốn sách.");
        if (k === "G") return qsTr("Mở danh sách chương để nhảy đến phần mong muốn.");
        if (k === "H") return qsTr("Mở danh sách bookmark, highlight và ghi chú.");
        if (k === "J") return qsTr("Sang trang tiếp theo hoặc cuộn xuống một nhịp.");
        if (k === "K") return qsTr("Quay lại trang trước hoặc cuộn lên một nhịp.");
        if (k === "L") return qsTr("Mở từ điển tra nhanh nghĩa từ ngay khi đang đọc.");
        if (k === "BKSP") return qsTr("Lùi về trang trước hoặc cuộn lên một nhịp.");
        if (k === "Z") return qsTr("Phóng to chữ đọc thêm 1 nấc ngay lập tức.");
        if (k === "X") return qsTr("Thu nhỏ chữ đọc đi 1 nấc ngay lập tức.");
        if (k === "C") return qsTr("Chuyển giữa lật từng trang và cuộn liên tục.");
        if (k === "V") return qsTr("Tô đậm đầu từ giúp mắt lướt và đọc nhanh hơn.");
        if (k === "SPACE") return qsTr("Sang trang tiếp theo, hoặc phát / tạm dừng TTS.");
        if (k === "B") return qsTr("Nhảy nhanh xuống cuối chương hoặc cuối trang hiện tại.");
        if (k === "N") return qsTr("Chuyển nhanh sang chương kế tiếp trong sách.");
        if (k === "M") return qsTr("Lưu hoặc bỏ đánh dấu trang đang đọc hiện tại.");
        return qsTr("Ẩn hoặc hiện thanh trạng thái để tập trung đọc.");
    }

    function handlePassportKey(event) {
        if (currentStep !== 4) return;
        var k = event.key;
        if (k === 32) {
            selectedKey = "SPACE";
            return;
        }
        if (k === 8) {
            selectedKey = "BKSP";
            return;
        }
        if (k === 13 || k === 61453) {
            selectedKey = "ENT";
            return;
        }
        var u = ("" + event.unicode).toUpperCase();
        var mapped = "QWRTUIOPASDFGHJKLZXCVBNM";
        if (u.length === 1 && mapped.indexOf(u) !== -1) {
            selectedKey = u;
        }
    }

    Page {
        actionBarVisibility: ChromeVisibility.Hidden

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create(bookManager.uiBg)
            layout: StackLayout {
                orientation: LayoutOrientation.TopToBottom
            }
            keyListeners: [
                KeyListener {
                    onKeyPressed: { onboardingSheet.handlePassportKey(event); }
                }
            ]

            // =================================================================
            // TOP-RIGHT SKIP
            // =================================================================
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                topPadding: bookManager.dp(10)
                bottomPadding: bookManager.dp(4)
                leftPadding: bookManager.dp(18)
                rightPadding: bookManager.dp(18)
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }

                Container {
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                }

                Container {
                    visible: onboardingSheet.currentStep < (onboardingSheet.totalSteps - 1)
                    topPadding: bookManager.dp(6)
                    bottomPadding: bookManager.dp(6)
                    leftPadding: bookManager.dp(14)
                    rightPadding: bookManager.dp(4)
                    onTouch: {
                        if (event.isUp()) {
                            onboardingSheet.finishOnboarding();
                        }
                    }

                    Label {
                        text: qsTr("Skip") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiAccent)
                    }
                }
            }

            // =================================================================
            // MAIN VERTICALLY-CENTERED INTERACTIVE STAGE (SWIPE LEFT / RIGHT)
            // =================================================================
            Container {
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                leftPadding: bookManager.dp(20)
                rightPadding: bookManager.dp(20)
                layout: DockLayout {}
                onTouch: {
                    if (event.isDown()) {
                        onboardingSheet.swipeStartX = event.windowX;
                    } else if (event.isUp() && onboardingSheet.swipeStartX >= 0) {
                        var dx = event.windowX - onboardingSheet.swipeStartX;
                        onboardingSheet.swipeStartX = -1;
                        if (dx < -70) {
                            onboardingSheet.stepNext();
                        } else if (dx > 70) {
                            onboardingSheet.stepPrev();
                        }
                    } else if (event.isCancel()) {
                        onboardingSheet.swipeStartX = -1;
                    }
                }

                // -------------------------------------------------------------
                // PAGE 0: WELCOME HERO (TRUE CENTER)
                // -------------------------------------------------------------
                Container {
                    visible: onboardingSheet.currentStep === 0
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Center
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    ImageView {
                        horizontalAlignment: HorizontalAlignment.Center
                        preferredWidth: bookManager.dp(156)
                        preferredHeight: bookManager.dp(156)
                        scalingMethod: ScalingMethod.AspectFit
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_app_dark.png" : "asset:///icons/ic_app_light.png"
                    }

                    Label {
                        topMargin: bookManager.dp(18)
                        horizontalAlignment: HorizontalAlignment.Center
                        textStyle.textAlign: TextAlign.Center
                        text: "Libris"
                        textStyle.base: SystemDefaults.TextStyles.BigText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiAccent)
                    }

                    Label {
                        topMargin: bookManager.dp(4)
                        horizontalAlignment: HorizontalAlignment.Center
                        textStyle.textAlign: TextAlign.Center
                        text: qsTr("Chào mừng") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiInk)
                    }

                    Label {
                        topMargin: bookManager.dp(16)
                        horizontalAlignment: HorizontalAlignment.Center
                        textStyle.textAlign: TextAlign.Center
                        text: "EPUB · PDF · MOBI · CBZ · TXT"
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiMuted)
                    }

                    Label {
                        topMargin: bookManager.dp(22)
                        horizontalAlignment: HorizontalAlignment.Center
                        textStyle.textAlign: TextAlign.Center
                        text: qsTr("Vuốt trái để tiếp tục ›") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiAccent)
                    }
                }

                // -------------------------------------------------------------
                // PAGE 1: LANGUAGE (CENTERED)
                // -------------------------------------------------------------
                Container {
                    visible: onboardingSheet.currentStep === 1
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Center
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    ImageView {
                        horizontalAlignment: HorizontalAlignment.Center
                        preferredWidth: bookManager.dp(84)
                        preferredHeight: bookManager.dp(84)
                        scalingMethod: ScalingMethod.AspectFit
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_translate.png" : "asset:///icons/ic_translate_dark.png"
                    }

                    Label {
                        topMargin: bookManager.dp(14)
                        bottomMargin: bookManager.dp(24)
                        horizontalAlignment: HorizontalAlignment.Center
                        text: qsTr("Ngôn ngữ") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.BigText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiInk)
                    }

                    // Native BB10 RadioGroup Language Picker
                    Divider {}

                    RadioGroup {
                        id: langRadioGroup
                        horizontalAlignment: HorizontalAlignment.Fill
                        dividersVisible: true
                        Option {
                            text: "English"
                            value: "en"
                            selected: bookManager.language === "en"
                        }
                        Option {
                            text: "Tiếng Việt"
                            value: "vi"
                            selected: bookManager.language === "vi"
                        }
                        onSelectedOptionChanged: {
                            if (selectedOption && selectedOption.value !== bookManager.language) {
                                bookManager.setLanguage(selectedOption.value);
                            }
                        }
                    }

                    Divider {}
                }

                // -------------------------------------------------------------
                // PAGE 2: THEME (LARGE DEVICE MOCKUPS CENTERED)
                // -------------------------------------------------------------
                Container {
                    visible: onboardingSheet.currentStep === 2
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Center
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    Label {
                        bottomMargin: bookManager.dp(24)
                        horizontalAlignment: HorizontalAlignment.Center
                        text: qsTr("Giao diện") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.BigText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiInk)
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        Container {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            rightMargin: bookManager.dp(8)
                            topPadding: bookManager.dp(8); bottomPadding: bookManager.dp(8)
                            onTouch: {
                                if (event.isUp()) {
                                    bookManager.setAppTheme("dark");
                                }
                            }

                            ImageView {
                                horizontalAlignment: HorizontalAlignment.Center
                                preferredWidth: bookManager.dp(192)
                                preferredHeight: bookManager.dp(244)
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: "asset:///onboarding/ob_theme_dark_preview.png"
                                opacity: bookManager.appTheme === "dark" ? 1.0 : 0.55
                            }

                            Label {
                                topMargin: bookManager.dp(14)
                                horizontalAlignment: HorizontalAlignment.Center
                                text: qsTr("Tối") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.appTheme === "dark" ? bookManager.uiAccent : bookManager.uiInk)
                            }

                            Container {
                                topMargin: bookManager.dp(8)
                                horizontalAlignment: HorizontalAlignment.Center
                                preferredWidth: bookManager.dp(44)
                                preferredHeight: bookManager.dp(4)
                                background: Color.create(bookManager.appTheme === "dark" ? bookManager.uiAccent : "transparent")
                            }
                        }

                        Container {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            leftMargin: bookManager.dp(8)
                            topPadding: bookManager.dp(8); bottomPadding: bookManager.dp(8)
                            onTouch: {
                                if (event.isUp()) {
                                    bookManager.setAppTheme("light");
                                }
                            }

                            ImageView {
                                horizontalAlignment: HorizontalAlignment.Center
                                preferredWidth: bookManager.dp(192)
                                preferredHeight: bookManager.dp(244)
                                scalingMethod: ScalingMethod.AspectFit
                                imageSource: "asset:///onboarding/ob_theme_light_preview.png"
                                opacity: bookManager.appTheme === "light" ? 1.0 : 0.55
                            }

                            Label {
                                topMargin: bookManager.dp(14)
                                horizontalAlignment: HorizontalAlignment.Center
                                text: qsTr("Sáng") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.appTheme === "light" ? bookManager.uiAccent : bookManager.uiInk)
                            }

                            Container {
                                topMargin: bookManager.dp(8)
                                horizontalAlignment: HorizontalAlignment.Center
                                preferredWidth: bookManager.dp(44)
                                preferredHeight: bookManager.dp(4)
                                background: Color.create(bookManager.appTheme === "light" ? bookManager.uiAccent : "transparent")
                            }
                        }
                    }
                }

                // -------------------------------------------------------------
                // PAGE 3: ADD BOOKS (INTERACTIVE 3-TAB SHOWCASE)
                // -------------------------------------------------------------
                Container {
                    visible: onboardingSheet.currentStep === 3
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Center
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    ImageView {
                        horizontalAlignment: HorizontalAlignment.Center
                        preferredWidth: bookManager.dp(300)
                        preferredHeight: bookManager.dp(160)
                        scalingMethod: ScalingMethod.AspectFit
                        imageSource: bookManager.appTheme === "dark" ? "asset:///onboarding/ob_books_dark.png" : "asset:///onboarding/ob_books_light.png"
                    }

                    Label {
                        topMargin: bookManager.dp(12)
                        bottomMargin: bookManager.dp(18)
                        horizontalAlignment: HorizontalAlignment.Center
                        text: qsTr("Thêm sách") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.BigText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiInk)
                    }

                    // Native BB10 SegmentedControl for Book Sources
                    SegmentedControl {
                        id: bookSourceControl
                        horizontalAlignment: HorizontalAlignment.Fill
                        bottomMargin: bookManager.dp(12)
                        Option {
                            text: qsTr("Thẻ SD") + Retranslate.onLanguageChanged
                            value: "sd"
                            selected: true
                        }
                        Option {
                            text: "Wi-Fi Drop"
                            value: "wifi"
                        }
                        Option {
                            text: "OPDS"
                            value: "opds"
                        }
                        onSelectedOptionChanged: {
                            if (selectedOption) {
                                onboardingSheet.activeBookSource = selectedOption.value;
                            }
                        }
                    }

                    Divider {}

                    // Live Interactive Source Preview Card
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(18); bottomPadding: bookManager.dp(18)
                        leftPadding: bookManager.dp(18); rightPadding: bookManager.dp(18)
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        ImageView {
                            preferredWidth: bookManager.dp(52); preferredHeight: bookManager.dp(52)
                            rightMargin: bookManager.dp(16)
                            verticalAlignment: VerticalAlignment.Center
                            imageSource: onboardingSheet.activeBookSource === "sd"
                                ? (bookManager.appTheme === "dark" ? "asset:///icons/ic_sdcard.png" : "asset:///icons/ic_sdcard_dark.png")
                                : (onboardingSheet.activeBookSource === "wifi"
                                    ? (bookManager.appTheme === "dark" ? "asset:///icons/ic_wifi.png" : "asset:///icons/ic_wifi_dark.png")
                                    : (bookManager.appTheme === "dark" ? "asset:///icons/ic_opds.png" : "asset:///icons/ic_opds_dark.png"))
                        }

                        Container {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            verticalAlignment: VerticalAlignment.Center
                            Label {
                                text: onboardingSheet.activeBookSource === "sd"
                                    ? "/shared/books  ·  MicroSD"
                                    : (onboardingSheet.activeBookSource === "wifi" ? "http://192.168.1.x:8080" : "Standard Ebooks · Calibre")
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiAccent)
                            }
                            Label {
                                topMargin: bookManager.dp(4)
                                multiline: true
                                text: (onboardingSheet.activeBookSource === "sd"
                                    ? qsTr("Tự động quét sách trong máy & thẻ nhớ")
                                    : (onboardingSheet.activeBookSource === "wifi"
                                        ? qsTr("Mở trình duyệt máy tính để gửi sách không dây")
                                        : qsTr("Tải sách miễn phí trực tiếp từ kho OPDS"))) + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: Color.create(bookManager.uiInk)
                            }
                        }
                    }
                }

                // -------------------------------------------------------------
                // PAGE 4: VISUAL BLACKBERRY PASSPORT KEYMAP & TOUCH GESTURES
                // -------------------------------------------------------------
                Container {
                    visible: onboardingSheet.currentStep === 4
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Center
                    leftPadding: bookManager.dp(14)
                    rightPadding: bookManager.dp(14)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    // Header
                    Label {
                        text: qsTr("ĐIỀU KHIỂN") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiAccent)
                    }

                    Label {
                        topMargin: bookManager.dp(2)
                        text: qsTr("Bàn phím & cử chỉ") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiInk)
                    }

                    Label {
                        topMargin: bookManager.dp(2)
                        horizontalAlignment: HorizontalAlignment.Fill
                        multiline: true
                        text: qsTr("Chạm phím trên hình hoặc bấm trực tiếp phím cứng để xem chức năng") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create(bookManager.uiMuted)
                    }

                    // Device Keyboard Switcher: Passport | Classic (Q20) | Q10
                    SegmentedControl {
                        topMargin: bookManager.dp(10)
                        horizontalAlignment: HorizontalAlignment.Fill

                        Option {
                            text: "Passport"
                            value: "passport"
                            selected: onboardingSheet.keyboardModel === "passport"
                        }
                        Option {
                            text: "Classic (Q20)"
                            value: "q20"
                            selected: onboardingSheet.keyboardModel === "q20"
                        }
                        Option {
                            text: "Q10"
                            value: "q10"
                            selected: onboardingSheet.keyboardModel === "q10"
                        }

                        onSelectedValueChanged: {
                            if (selectedValue) {
                                onboardingSheet.keyboardModel = selectedValue;
                            }
                        }
                    }

                    // Selected Key Detail Card
                    Container {
                        topMargin: bookManager.dp(12)
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(12)
                        bottomPadding: bookManager.dp(12)
                        leftPadding: bookManager.dp(12)
                        rightPadding: bookManager.dp(12)
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        Container {
                            verticalAlignment: VerticalAlignment.Center
                            minWidth: bookManager.dp(74)
                            preferredHeight: bookManager.dp(56)
                            leftPadding: bookManager.dp(10)
                            rightPadding: bookManager.dp(10)
                            background: Color.create(bookManager.uiAccent)
                            layout: DockLayout {}

                            Label {
                                horizontalAlignment: HorizontalAlignment.Center
                                verticalAlignment: VerticalAlignment.Center
                                text: onboardingSheet.keyBadgeLabel(onboardingSheet.selectedKey)
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create("#ffffff")
                            }
                        }

                        Container {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            leftMargin: bookManager.dp(14)
                            verticalAlignment: VerticalAlignment.Center
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            Label {
                                text: onboardingSheet.keyCategory(onboardingSheet.selectedKey) + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiAccent)
                            }

                            Label {
                                topMargin: bookManager.dp(2)
                                horizontalAlignment: HorizontalAlignment.Fill
                                multiline: true
                                text: onboardingSheet.keyTitle(onboardingSheet.selectedKey) + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiInk)
                            }

                            Label {
                                topMargin: bookManager.dp(2)
                                horizontalAlignment: HorizontalAlignment.Fill
                                multiline: true
                                text: onboardingSheet.keyDesc(onboardingSheet.selectedKey) + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }
                        }
                    }

                    // Authentic BlackBerry Physical Keyboard Map (Passport / Classic Q20 / Q10)
                    Container {
                        topMargin: bookManager.dp(10)
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.theme === "light" ? "#D8DCE2" : "#111317")
                        topPadding: bookManager.dp(8)
                        bottomPadding: bookManager.dp(8)
                        leftPadding: bookManager.dp(6)
                        rightPadding: bookManager.dp(6)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        // Classic (Q20) Toolbelt Row (Call · Menu · Trackpad · Back · End)
                        Container {
                            visible: onboardingSheet.keyboardModel === "q20"
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: bookManager.dp(5)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(34)
                                    opacity: 0.45
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "📞"; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.color: Color.create(bookManager.uiMuted) }
                                }
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(34)
                                    opacity: 0.45
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "⋮⋮"; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(bookManager.uiMuted) }
                                }
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.4 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(34)
                                    background: Color.create(onboardingSheet.selectedKey === "SPACE" ? bookManager.uiAccent : bookManager.uiCard)
                                    layout: DockLayout {}
                                    onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "SPACE"; }
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "Trackpad"; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "SPACE" ? "#ffffff" : bookManager.uiInk) }
                                }
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(34)
                                    opacity: 0.45
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "↩"; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(bookManager.uiMuted) }
                                }
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    preferredHeight: bookManager.dp(34)
                                    opacity: 0.45
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "✕"; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(bookManager.uiMuted) }
                                }
                            }

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                topMargin: bookManager.dp(5)
                                preferredHeight: bookManager.dp(2)
                                background: Color.create(bookManager.theme === "light" ? "#B5BCC7" : "#3A404B")
                            }
                        }

                        // Row 1: Q W E R T Y U I O P
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            // Q
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "Q" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "Q"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "Q"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "Q" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "Q"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // W
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "W" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "W"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "W"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "W" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "W"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // E (unassigned)
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                opacity: 0.30
                                background: Color.create(bookManager.uiSurface)
                                layout: DockLayout {}
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "E"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.color: Color.create(bookManager.uiMuted) }
                            }
                            // R
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "R" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "R"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "R"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "R" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "R"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // T
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "T" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "T"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "T"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "T" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "T"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // Y (unassigned)
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                opacity: 0.30
                                background: Color.create(bookManager.uiSurface)
                                layout: DockLayout {}
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "Y"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.color: Color.create(bookManager.uiMuted) }
                            }
                            // U
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "U" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "U"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "U"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "U" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "U"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // I
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "I" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "I"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "I"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "I" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "I"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // O
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "O" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "O"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "O"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "O" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "O"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // P
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "P" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "P"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "P"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "P" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "P"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                        }

                        // Metallic Passport Fret 1
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            topMargin: bookManager.dp(5)
                            bottomMargin: bookManager.dp(5)
                            preferredHeight: bookManager.dp(2)
                            background: Color.create(bookManager.theme === "light" ? "#B5BCC7" : "#3A404B")
                        }

                        // Row 2: A S D F G H J K L BKSP
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            // A
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "A" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "A"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "A"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "A" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "A"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // S
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "S" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "S"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "S"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "S" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "S"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // D
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "D" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "D"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "D"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "D" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "D"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // F
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "F" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "F"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "F"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "F" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "F"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // G
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "G" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "G"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "G"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "G" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "G"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // H
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "H" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "H"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "H"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "H" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "H"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // J
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "J" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "J"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "J"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "J" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "J"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // K
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "K" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "K"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "K"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "K" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "K"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // L
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "L" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "L"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "L"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "L" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "L"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // BKSP
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "BKSP" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "BKSP"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "⌫"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "BKSP" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "BKSP"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                        }

                        // Metallic Passport Fret 2
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            topMargin: bookManager.dp(5)
                            bottomMargin: bookManager.dp(5)
                            preferredHeight: bookManager.dp(2)
                            background: Color.create(bookManager.theme === "light" ? "#B5BCC7" : "#3A404B")
                        }

                        // Row 3 (Passport): Shift Z X C V [SPACE] B N M Enter
                        Container {
                            visible: onboardingSheet.keyboardModel === "passport"
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            // Shift (unassigned)
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 0.85 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                opacity: 0.30
                                background: Color.create(bookManager.uiSurface)
                                layout: DockLayout {}
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "⇧"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.color: Color.create(bookManager.uiMuted) }
                            }
                            // Z
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "Z" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "Z"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "Z"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "Z" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "Z"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // X
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "X" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "X"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "X"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "X" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "X"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // C
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "C" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "C"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "C"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "C" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "C"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // V
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "V" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "V"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "V"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "V" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "V"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // SPACE
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.9 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "SPACE" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "SPACE"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "SPACE"; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "SPACE" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "SPACE"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // B
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "B" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "B"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "B"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "B" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "B"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // N
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "N" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "N"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "N"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "N" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "N"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // M
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                rightMargin: bookManager.dp(4)
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "M" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "M"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "M"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "M" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "M"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                            // ENT
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 0.95 }
                                preferredHeight: bookManager.dp(50)
                                background: Color.create(onboardingSheet.selectedKey === "ENT" ? bookManager.uiAccent : bookManager.uiCard)
                                layout: DockLayout {}
                                onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "ENT"; }
                                Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "↵"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "ENT" ? "#ffffff" : bookManager.uiInk) }
                                Container { visible: onboardingSheet.selectedKey !== "ENT"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                            }
                        }

                        // Row 3 & Row 4 (Q10 / Classic Q20): alt Z X C V B N M $ ↵  +  ⇧ 0 [SPACE] sym ⇧
                        Container {
                            visible: onboardingSheet.keyboardModel !== "passport"
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            // Row 3: alt Z X C V B N M $ ↵
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(44)
                                    opacity: 0.30
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "alt"; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.color: Color.create(bookManager.uiMuted) }
                                }
                                // Z
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(44)
                                    background: Color.create(onboardingSheet.selectedKey === "Z" ? bookManager.uiAccent : bookManager.uiCard)
                                    layout: DockLayout {}
                                    onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "Z"; }
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "Z"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "Z" ? "#ffffff" : bookManager.uiInk) }
                                    Container { visible: onboardingSheet.selectedKey !== "Z"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                                }
                                // X
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(44)
                                    background: Color.create(onboardingSheet.selectedKey === "X" ? bookManager.uiAccent : bookManager.uiCard)
                                    layout: DockLayout {}
                                    onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "X"; }
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "X"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "X" ? "#ffffff" : bookManager.uiInk) }
                                    Container { visible: onboardingSheet.selectedKey !== "X"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                                }
                                // C
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(44)
                                    background: Color.create(onboardingSheet.selectedKey === "C" ? bookManager.uiAccent : bookManager.uiCard)
                                    layout: DockLayout {}
                                    onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "C"; }
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "C"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "C" ? "#ffffff" : bookManager.uiInk) }
                                    Container { visible: onboardingSheet.selectedKey !== "C"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                                }
                                // V
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(44)
                                    background: Color.create(onboardingSheet.selectedKey === "V" ? bookManager.uiAccent : bookManager.uiCard)
                                    layout: DockLayout {}
                                    onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "V"; }
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "V"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "V" ? "#ffffff" : bookManager.uiInk) }
                                    Container { visible: onboardingSheet.selectedKey !== "V"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                                }
                                // B
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(44)
                                    background: Color.create(onboardingSheet.selectedKey === "B" ? bookManager.uiAccent : bookManager.uiCard)
                                    layout: DockLayout {}
                                    onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "B"; }
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "B"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "B" ? "#ffffff" : bookManager.uiInk) }
                                    Container { visible: onboardingSheet.selectedKey !== "B"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                                }
                                // N
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(44)
                                    background: Color.create(onboardingSheet.selectedKey === "N" ? bookManager.uiAccent : bookManager.uiCard)
                                    layout: DockLayout {}
                                    onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "N"; }
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "N"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "N" ? "#ffffff" : bookManager.uiInk) }
                                    Container { visible: onboardingSheet.selectedKey !== "N"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                                }
                                // M
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(44)
                                    background: Color.create(onboardingSheet.selectedKey === "M" ? bookManager.uiAccent : bookManager.uiCard)
                                    layout: DockLayout {}
                                    onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "M"; }
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "M"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "M" ? "#ffffff" : bookManager.uiInk) }
                                    Container { visible: onboardingSheet.selectedKey !== "M"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                                }
                                // $
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(44)
                                    opacity: 0.30
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "$"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.color: Color.create(bookManager.uiMuted) }
                                }
                                // ENT
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    preferredHeight: bookManager.dp(44)
                                    background: Color.create(onboardingSheet.selectedKey === "ENT" ? bookManager.uiAccent : bookManager.uiCard)
                                    layout: DockLayout {}
                                    onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "ENT"; }
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "↵"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "ENT" ? "#ffffff" : bookManager.uiInk) }
                                    Container { visible: onboardingSheet.selectedKey !== "ENT"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                                }
                            }

                            // Metallic Fret 3
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                topMargin: bookManager.dp(5)
                                bottomMargin: bookManager.dp(5)
                                preferredHeight: bookManager.dp(2)
                                background: Color.create(bookManager.theme === "light" ? "#B5BCC7" : "#3A404B")
                            }

                            // Row 4 (Q10 / Classic Q20): ⇧ 0 [SPACE] sym ⇧
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(42)
                                    opacity: 0.30
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "⇧"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.color: Color.create(bookManager.uiMuted) }
                                }
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(42)
                                    opacity: 0.30
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "0"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.color: Color.create(bookManager.uiMuted) }
                                }
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 3.4 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(42)
                                    background: Color.create(onboardingSheet.selectedKey === "SPACE" ? bookManager.uiAccent : bookManager.uiCard)
                                    layout: DockLayout {}
                                    onTouch: { if (event.isUp()) onboardingSheet.selectedKey = "SPACE"; }
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "SPACE"; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.fontWeight: FontWeight.Bold; textStyle.color: Color.create(onboardingSheet.selectedKey === "SPACE" ? "#ffffff" : bookManager.uiInk) }
                                    Container { visible: onboardingSheet.selectedKey !== "SPACE"; horizontalAlignment: HorizontalAlignment.Fill; verticalAlignment: VerticalAlignment.Bottom; preferredHeight: bookManager.dp(3); opacity: 0.65; background: Color.create(bookManager.uiAccent) }
                                }
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    rightMargin: bookManager.dp(4)
                                    preferredHeight: bookManager.dp(42)
                                    opacity: 0.30
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "sym"; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.color: Color.create(bookManager.uiMuted) }
                                }
                                Container {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    preferredHeight: bookManager.dp(42)
                                    opacity: 0.30
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}
                                    Label { horizontalAlignment: HorizontalAlignment.Center; verticalAlignment: VerticalAlignment.Center; text: "⇧"; textStyle.base: SystemDefaults.TextStyles.BodyText; textStyle.color: Color.create(bookManager.uiMuted) }
                                }
                            }
                        }
                    }

                    // Touch Gestures Footer Strip
                    Container {
                        topMargin: bookManager.dp(12)
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(10)
                        bottomPadding: bookManager.dp(10)
                        leftPadding: bookManager.dp(12)
                        rightPadding: bookManager.dp(12)

                        Label {
                            horizontalAlignment: HorizontalAlignment.Fill
                            multiline: true
                            text: qsTr("Cử chỉ chạm: Trái / Phải lật trang · Giữa mở thanh công cụ") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }
                    }
                }

                // -------------------------------------------------------------
                // PAGE 5: READING TOOLS (INTERACTIVE LIVE EXCERPT)
                // -------------------------------------------------------------
                Container {
                    visible: onboardingSheet.currentStep === 5
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Center
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    ImageView {
                        horizontalAlignment: HorizontalAlignment.Center
                        preferredWidth: bookManager.dp(300)
                        preferredHeight: bookManager.dp(160)
                        scalingMethod: ScalingMethod.AspectFit
                        imageSource: bookManager.appTheme === "dark" ? "asset:///onboarding/ob_tools_dark.png" : "asset:///onboarding/ob_tools_light.png"
                    }

                    Label {
                        topMargin: bookManager.dp(12)
                        bottomMargin: bookManager.dp(18)
                        horizontalAlignment: HorizontalAlignment.Center
                        text: qsTr("Công cụ đọc") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.BigText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiInk)
                    }

                    // Native BB10 SegmentedControl for Reading Tools
                    SegmentedControl {
                        id: toolDemoControl
                        horizontalAlignment: HorizontalAlignment.Fill
                        bottomMargin: bookManager.dp(12)
                        Option {
                            text: "Highlight"
                            value: "highlight"
                            selected: true
                        }
                        Option {
                            text: qsTr("Từ điển") + Retranslate.onLanguageChanged
                            value: "dict"
                        }
                        Option {
                            text: "TTS"
                            value: "tts"
                        }
                        onSelectedOptionChanged: {
                            if (selectedOption) {
                                onboardingSheet.activeToolDemo = selectedOption.value;
                            }
                        }
                    }

                    Divider {}

                    // Live Tool Demo Card
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        background: Color.create(bookManager.uiCard)
                        topPadding: bookManager.dp(18); bottomPadding: bookManager.dp(18)
                        leftPadding: bookManager.dp(18); rightPadding: bookManager.dp(18)
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                        ImageView {
                            preferredWidth: bookManager.dp(48); preferredHeight: bookManager.dp(48)
                            rightMargin: bookManager.dp(16)
                            verticalAlignment: VerticalAlignment.Center
                            imageSource: onboardingSheet.activeToolDemo === "highlight"
                                ? (bookManager.appTheme === "dark" ? "asset:///icons/ic_highlight.png" : "asset:///icons/ic_highlight_dark.png")
                                : (onboardingSheet.activeToolDemo === "dict"
                                    ? (bookManager.appTheme === "dark" ? "asset:///icons/ic_translate.png" : "asset:///icons/ic_translate_dark.png")
                                    : (bookManager.appTheme === "dark" ? "asset:///icons/ic_tts.png" : "asset:///icons/ic_tts_dark.png"))
                        }

                        Container {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                            verticalAlignment: VerticalAlignment.Center
                            Label {
                                text: onboardingSheet.activeToolDemo === "highlight"
                                    ? "“A room without books...”  ·  PNG"
                                    : (onboardingSheet.activeToolDemo === "dict"
                                        ? "Serendipity  →  " + qsTr("Sự tình cờ may mắn")
                                        : "▶  eSpeak-NG Offline · Cloud TTS") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiAccent)
                            }
                            Label {
                                topMargin: bookManager.dp(4)
                                multiline: true
                                text: (onboardingSheet.activeToolDemo === "highlight"
                                    ? qsTr("Tô màu, ghi chú & xuất ảnh PNG")
                                    : (onboardingSheet.activeToolDemo === "dict"
                                        ? qsTr("Bôi đen từ để tra ngay trên trang")
                                        : qsTr("eSpeak-NG ngoại tuyến & Cloud TTS"))) + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: Color.create(bookManager.uiInk)
                            }
                        }
                    }
                }

                // -------------------------------------------------------------
                // PAGE 6: READY HERO (TRUE CENTER)
                // -------------------------------------------------------------
                Container {
                    visible: onboardingSheet.currentStep === 6
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Center
                    leftPadding: bookManager.dp(12)
                    rightPadding: bookManager.dp(12)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    ImageView {
                        horizontalAlignment: HorizontalAlignment.Center
                        preferredWidth: bookManager.dp(88)
                        preferredHeight: bookManager.dp(88)
                        scalingMethod: ScalingMethod.AspectFit
                        imageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_done.png" : "asset:///icons/ic_done_dark.png"
                    }

                    Label {
                        topMargin: bookManager.dp(20)
                        bottomMargin: bookManager.dp(28)
                        horizontalAlignment: HorizontalAlignment.Center
                        textStyle.textAlign: TextAlign.Center
                        text: qsTr("Sẵn sàng!") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.BigText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiInk)
                    }

                    Button {
                        horizontalAlignment: HorizontalAlignment.Fill
                        appearance: ControlAppearance.Primary
                        text: qsTr("Bắt đầu đọc") + Retranslate.onLanguageChanged
                        onClicked: {
                            onboardingSheet.finishOnboarding();
                        }
                    }
                }
            }

            // =================================================================
            // NATIVE BB10 DARK BOTTOM ACTION BAR (Left | 7 Dots | Right)
            // =================================================================
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: bookManager.dp(1)
                background: Color.create("#2B2F36")
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create("#14161A")
                topPadding: bookManager.dp(10)
                bottomPadding: bookManager.dp(12)
                leftPadding: bookManager.dp(16)
                rightPadding: bookManager.dp(16)
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }

                // Left Arrow (Previous)
                Container {
                    preferredWidth: bookManager.dp(78)
                    preferredHeight: bookManager.dp(62)
                    background: Color.create(onboardingSheet.currentStep > 0 ? "#22262E" : "transparent")
                    opacity: onboardingSheet.currentStep > 0 ? 1.0 : 0.22
                    layout: DockLayout {}
                    onTouch: {
                        if (event.isUp() && onboardingSheet.currentStep > 0) {
                            onboardingSheet.stepPrev();
                        }
                    }

                    ImageView {
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center
                        preferredWidth: bookManager.dp(38)
                        preferredHeight: bookManager.dp(38)
                        imageSource: "asset:///icons/ic_previous.png"
                    }
                }

                // Center Graphic Step Dots (7 steps)
                Container {
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1.0
                    }
                    verticalAlignment: VerticalAlignment.Center

                    Container {
                        horizontalAlignment: HorizontalAlignment.Center
                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }

                        Container {
                            preferredWidth: onboardingSheet.currentStep === 0 ? bookManager.dp(32) : bookManager.dp(12)
                            preferredHeight: bookManager.dp(12)
                            rightMargin: bookManager.dp(8)
                            opacity: onboardingSheet.currentStep === 0 ? 1.0 : 0.55
                            background: Color.create(bookManager.uiAccent)
                        }
                        Container {
                            preferredWidth: onboardingSheet.currentStep === 1 ? bookManager.dp(32) : bookManager.dp(12)
                            preferredHeight: bookManager.dp(12)
                            rightMargin: bookManager.dp(8)
                            opacity: onboardingSheet.currentStep >= 1 ? (onboardingSheet.currentStep === 1 ? 1.0 : 0.55) : 1.0
                            background: Color.create(onboardingSheet.currentStep >= 1 ? bookManager.uiAccent : bookManager.uiHairline)
                        }
                        Container {
                            preferredWidth: onboardingSheet.currentStep === 2 ? bookManager.dp(32) : bookManager.dp(12)
                            preferredHeight: bookManager.dp(12)
                            rightMargin: bookManager.dp(8)
                            opacity: onboardingSheet.currentStep >= 2 ? (onboardingSheet.currentStep === 2 ? 1.0 : 0.55) : 1.0
                            background: Color.create(onboardingSheet.currentStep >= 2 ? bookManager.uiAccent : bookManager.uiHairline)
                        }
                        Container {
                            preferredWidth: onboardingSheet.currentStep === 3 ? bookManager.dp(32) : bookManager.dp(12)
                            preferredHeight: bookManager.dp(12)
                            rightMargin: bookManager.dp(8)
                            opacity: onboardingSheet.currentStep >= 3 ? (onboardingSheet.currentStep === 3 ? 1.0 : 0.55) : 1.0
                            background: Color.create(onboardingSheet.currentStep >= 3 ? bookManager.uiAccent : bookManager.uiHairline)
                        }
                        Container {
                            preferredWidth: onboardingSheet.currentStep === 4 ? bookManager.dp(32) : bookManager.dp(12)
                            preferredHeight: bookManager.dp(12)
                            rightMargin: bookManager.dp(8)
                            opacity: onboardingSheet.currentStep >= 4 ? (onboardingSheet.currentStep === 4 ? 1.0 : 0.55) : 1.0
                            background: Color.create(onboardingSheet.currentStep >= 4 ? bookManager.uiAccent : bookManager.uiHairline)
                        }
                        Container {
                            preferredWidth: onboardingSheet.currentStep === 5 ? bookManager.dp(32) : bookManager.dp(12)
                            preferredHeight: bookManager.dp(12)
                            rightMargin: bookManager.dp(8)
                            opacity: onboardingSheet.currentStep >= 5 ? (onboardingSheet.currentStep === 5 ? 1.0 : 0.55) : 1.0
                            background: Color.create(onboardingSheet.currentStep >= 5 ? bookManager.uiAccent : bookManager.uiHairline)
                        }
                        Container {
                            preferredWidth: onboardingSheet.currentStep === 6 ? bookManager.dp(32) : bookManager.dp(12)
                            preferredHeight: bookManager.dp(12)
                            opacity: 1.0
                            background: Color.create(onboardingSheet.currentStep >= 6 ? bookManager.uiAccent : bookManager.uiHairline)
                        }
                    }
                }

                // Right Arrow (Next / Done)
                Container {
                    preferredWidth: bookManager.dp(78)
                    preferredHeight: bookManager.dp(62)
                    background: Color.create(onboardingSheet.currentStep === (onboardingSheet.totalSteps - 1) ? bookManager.uiAccent : "#22262E")
                    layout: DockLayout {}
                    onTouch: {
                        if (event.isUp()) {
                            onboardingSheet.stepNext();
                        }
                    }

                    ImageView {
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center
                        preferredWidth: bookManager.dp(38)
                        preferredHeight: bookManager.dp(38)
                        imageSource: onboardingSheet.currentStep === (onboardingSheet.totalSteps - 1)
                            ? (bookManager.appTheme === "dark" ? "asset:///icons/ic_done_dark.png" : "asset:///icons/ic_done.png")
                            : "asset:///icons/ic_next.png"
                    }
                }
            }
        }
    }
}
