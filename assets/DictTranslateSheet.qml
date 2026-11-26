import bb.cascades 1.3
import bb.system 1.2

Sheet {
    id: dictSheet

    property string initialText: ""
    property string savedContext: ""
    property string activeTab: "dict" // "dict", "translate", "vocab"
    property bool hasTranslationOutput: false
    property bool translationSuccess: false
    property variant recentWords: []
    property variant vocabItems: []
    property int vocabIndex: 0
    property bool vocabFlipped: false

    function reloadVocab() {
        vocabItems = dictManager.vocabList();
        if (vocabIndex >= vocabItems.length) {
            vocabIndex = Math.max(0, vocabItems.length - 1);
        }
        vocabFlipped = false;
    }

    function pushRecentWord(w) {
        if (!w) return;
        var list = [];
        list.push(w);
        for (var i = 0; i < recentWords.length && list.length < 5; ++i) {
            if (recentWords[i].toLowerCase() !== w.toLowerCase()) {
                list.push(recentWords[i]);
            }
        }
        recentWords = list;
    }

    Page {
        titleBar: TitleBar {
            title: qsTr("Từ điển & Dịch thuật") + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain
            dismissAction: ActionItem {
                title: qsTr("Đóng") + Retranslate.onLanguageChanged
                onTriggered: {
                    dictSheet.close();
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
                scrollViewProperties.scrollMode: ScrollMode.Vertical

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topPadding: bookManager.dp(12)
                    bottomPadding: bookManager.dp(28)
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)

                    // Tab selector
                    SegmentedControl {
                        id: tabControl
                        horizontalAlignment: HorizontalAlignment.Fill
                        bottomMargin: bookManager.dp(14)

                        Option {
                            id: optDict
                            text: qsTr("Tra từ điển") + Retranslate.onLanguageChanged
                            value: "dict"
                            selected: true
                        }

                        Option {
                            id: optTrans
                            text: qsTr("Dịch văn bản") + Retranslate.onLanguageChanged
                            value: "translate"
                        }

                        Option {
                            id: optVocab
                            text: qsTr("Flashcard") + Retranslate.onLanguageChanged
                            value: "vocab"
                        }

                        onSelectedIndexChanged: {
                            dictSheet.activeTab = selectedValue;
                            if (selectedValue === "vocab") {
                                dictSheet.reloadVocab();
                            }
                        }
                    }

                    // ==========================================
                    // TAB 1: TRA TỪ ĐIỂN (OFFLINE & ONLINE)
                    // ==========================================
                    Container {
                        visible: dictSheet.activeTab === "dict"
                        horizontalAlignment: HorizontalAlignment.Fill

                        // Search Input Bar (flat uiCard, auto-width Primary button)
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(8)
                            bottomPadding: bookManager.dp(8)
                            leftPadding: bookManager.dp(12)
                            rightPadding: bookManager.dp(12)
                            bottomMargin: bookManager.dp(10)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            TextField {
                                id: dictWordField
                                verticalAlignment: VerticalAlignment.Center
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                hintText: qsTr("Nhập từ cần tra...") + Retranslate.onLanguageChanged
                                clearButtonVisible: true
                                input.submitKey: SubmitKey.Search
                                input.onSubmitted: {
                                    performDictLookup();
                                }
                            }

                            Button {
                                leftMargin: bookManager.dp(8)
                                appearance: ControlAppearance.Primary
                                text: qsTr("Tra từ") + Retranslate.onLanguageChanged
                                verticalAlignment: VerticalAlignment.Center
                                onClicked: {
                                    performDictLookup();
                                }
                            }
                        }

                        // Recent Words Quick Chips (up to 4 latest looked-up words)
                        Container {
                            visible: dictSheet.recentWords.length > 0
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: bookManager.dp(10)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Container {
                                visible: dictSheet.recentWords.length > 0
                                background: Color.create(bookManager.uiActive)
                                leftPadding: bookManager.dp(10); rightPadding: bookManager.dp(10); topPadding: bookManager.dp(5); bottomPadding: bookManager.dp(5); rightMargin: bookManager.dp(6)
                                gestureHandlers: [ TapHandler { onTapped: { dictWordField.text = dictSheet.recentWords[0]; performDictLookup(); } } ]
                                Label { text: dictSheet.recentWords.length > 0 ? dictSheet.recentWords[0] : ""; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.fontWeight: FontWeight.W500; textStyle.color: Color.create(bookManager.uiAccent) }
                            }
                            Container {
                                visible: dictSheet.recentWords.length > 1
                                background: Color.create(bookManager.uiSurface)
                                leftPadding: bookManager.dp(10); rightPadding: bookManager.dp(10); topPadding: bookManager.dp(5); bottomPadding: bookManager.dp(5); rightMargin: bookManager.dp(6)
                                gestureHandlers: [ TapHandler { onTapped: { dictWordField.text = dictSheet.recentWords[1]; performDictLookup(); } } ]
                                Label { text: dictSheet.recentWords.length > 1 ? dictSheet.recentWords[1] : ""; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.color: Color.create(bookManager.uiMuted) }
                            }
                            Container {
                                visible: dictSheet.recentWords.length > 2
                                background: Color.create(bookManager.uiSurface)
                                leftPadding: bookManager.dp(10); rightPadding: bookManager.dp(10); topPadding: bookManager.dp(5); bottomPadding: bookManager.dp(5); rightMargin: bookManager.dp(6)
                                gestureHandlers: [ TapHandler { onTapped: { dictWordField.text = dictSheet.recentWords[2]; performDictLookup(); } } ]
                                Label { text: dictSheet.recentWords.length > 2 ? dictSheet.recentWords[2] : ""; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.color: Color.create(bookManager.uiMuted) }
                            }
                            Container {
                                visible: dictSheet.recentWords.length > 3
                                background: Color.create(bookManager.uiSurface)
                                leftPadding: bookManager.dp(10); rightPadding: bookManager.dp(10); topPadding: bookManager.dp(5); bottomPadding: bookManager.dp(5)
                                gestureHandlers: [ TapHandler { onTapped: { dictWordField.text = dictSheet.recentWords[3]; performDictLookup(); } } ]
                                Label { text: dictSheet.recentWords.length > 3 ? dictSheet.recentWords[3] : ""; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.color: Color.create(bookManager.uiMuted) }
                            }
                        }

                        // Result Card (flat uiCard)
                        Container {
                            visible: dictWordField.text.trim().length > 0
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(14)
                            bottomPadding: bookManager.dp(14)
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            bottomMargin: bookManager.dp(12)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            // Header: Word + Phonetic
                            Label {
                                id: lblResultWord
                                text: dictManager.lastWord
                                textStyle.base: SystemDefaults.TextStyles.TitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiInk)
                                multiline: true
                            }

                            Label {
                                id: lblResultPhonetic
                                visible: text.length > 0
                                topMargin: bookManager.dp(2)
                                text: dictManager.lastPhonetic
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiAccent)
                            }

                            Container {
                                topMargin: bookManager.dp(10)
                                bottomMargin: bookManager.dp(10)
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 1
                                background: Color.create(bookManager.uiHairline)
                            }

                            // Definition / Meaning
                            Label {
                                id: lblResultMeaning
                                text: dictManager.lastMeaning
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: Color.create(bookManager.uiInk)
                                multiline: true
                            }

                            // Suggestions if word not found
                            Label {
                                id: lblSuggestions
                                visible: text.length > 0
                                topMargin: bookManager.dp(8)
                                text: ""
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                                multiline: true
                            }

                            // Card Actions Row (never collides with long headwords)
                            Container {
                                visible: lblResultWord.text.length > 0
                                topMargin: bookManager.dp(14)
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Button {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("Lưu từ") + Retranslate.onLanguageChanged
                                    enabled: lblResultWord.text.length > 0 && lblResultMeaning.text.length > 0
                                    onClicked: {
                                        if (dictManager.saveVocabWord(lblResultWord.text, lblResultPhonetic.text, lblResultMeaning.text, dictSheet.savedContext, bookManager.currentBookTitle)) {
                                            dictSheet.reloadVocab();
                                            dictToast.body = qsTr("Đã lưu vào sổ từ vựng!") + Retranslate.onLanguageChanged;
                                            dictToast.show();
                                        }
                                    }
                                }

                                Button {
                                    leftMargin: bookManager.dp(10)
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("Sao chép") + Retranslate.onLanguageChanged
                                    onClicked: {
                                        dictManager.copyToClipboard(lblResultWord.text + " " + lblResultPhonetic.text + "\n" + lblResultMeaning.text);
                                    }
                                }
                            }
                        }

                        // Online Lookup Language & Action
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            DropDown {
                                id: dictTargetLangDropDown
                                title: qsTr("Dịch nghĩa sang:") + Retranslate.onLanguageChanged

                                Option { text: qsTr("Tiếng Việt") + Retranslate.onLanguageChanged; value: "vi"; selected: true }
                                Option { text: qsTr("Tiếng Anh") + Retranslate.onLanguageChanged; value: "en" }
                                Option { text: qsTr("Tiếng Pháp") + Retranslate.onLanguageChanged; value: "fr" }
                                Option { text: qsTr("Tiếng Trung (GT)") + Retranslate.onLanguageChanged; value: "zh-CN" }
                                Option { text: qsTr("Tiếng Nhật") + Retranslate.onLanguageChanged; value: "ja" }
                                Option { text: qsTr("Tiếng Hàn") + Retranslate.onLanguageChanged; value: "ko" }
                                Option { text: qsTr("Tiếng Đức") + Retranslate.onLanguageChanged; value: "de" }
                                Option { text: qsTr("Tiếng Nga") + Retranslate.onLanguageChanged; value: "ru" }
                                Option { text: qsTr("Tiếng T.B.Nha") + Retranslate.onLanguageChanged; value: "es" }

                                onSelectedValueChanged: {
                                    if (dictWordField.text.trim().length > 0) {
                                        performDictLookup();
                                    }
                                }
                            }

                            Button {
                                topMargin: bookManager.dp(10)
                                horizontalAlignment: HorizontalAlignment.Fill
                                text: qsTr("Tra Trực Tuyến") + Retranslate.onLanguageChanged
                                enabled: !dictManager.isLookingUp && dictWordField.text.trim().length > 0
                                onClicked: {
                                    lblResultMeaning.text = qsTr("Đang tra từ điển trực tuyến...");
                                    dictManager.lookupOnline(dictWordField.text.trim(), dictTargetLangDropDown.selectedValue);
                                }
                            }
                        }

                        // Online Looking up Indicator
                        Container {
                            visible: dictManager.isLookingUp
                            horizontalAlignment: HorizontalAlignment.Center
                            topMargin: bookManager.dp(14)

                            ActivityIndicator {
                                visible: dictManager.isLookingUp
                                running: dictManager.isLookingUp
                                preferredWidth: bookManager.dp(48)
                                preferredHeight: bookManager.dp(48)
                                onVisibleChanged: { if (visible) start(); else stop(); }
                            }

                            Label {
                                topMargin: bookManager.dp(6)
                                text: qsTr("Đang tra từ trực tuyến...") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }
                        }
                    }

                    // ==========================================
                    // TAB 2: DỊCH ĐOẠN VĂN BẢN (TRANSLATE)
                    // ==========================================
                    Container {
                        visible: dictSheet.activeTab === "translate"
                        horizontalAlignment: HorizontalAlignment.Fill

                        // Input TextArea Card
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            leftPadding: bookManager.dp(14)
                            rightPadding: bookManager.dp(14)
                            bottomMargin: bookManager.dp(10)

                            Label {
                                text: qsTr("Văn bản gốc:") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                                bottomMargin: bookManager.dp(6)
                            }

                            TextArea {
                                id: transInputField
                                preferredHeight: bookManager.dp(120)
                                hintText: qsTr("Nhập đoạn văn bản cần dịch...") + Retranslate.onLanguageChanged
                            }
                        }

                        // Source & Target Language Pickers + Auto-sized Swap Button
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: bookManager.dp(10)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            DropDown {
                                id: sourceLangDropDown
                                title: qsTr("Từ:") + Retranslate.onLanguageChanged
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                Option { text: qsTr("Tự động") + Retranslate.onLanguageChanged; value: "auto"; selected: true }
                                Option { text: qsTr("Tiếng Anh") + Retranslate.onLanguageChanged; value: "en" }
                                Option { text: qsTr("Tiếng Việt") + Retranslate.onLanguageChanged; value: "vi" }
                                Option { text: qsTr("Tiếng Trung (GT)") + Retranslate.onLanguageChanged; value: "zh-CN" }
                                Option { text: qsTr("Tiếng Trung (PT)") + Retranslate.onLanguageChanged; value: "zh-TW" }
                                Option { text: qsTr("Tiếng Nhật") + Retranslate.onLanguageChanged; value: "ja" }
                                Option { text: qsTr("Tiếng Hàn") + Retranslate.onLanguageChanged; value: "ko" }
                                Option { text: qsTr("Tiếng Pháp") + Retranslate.onLanguageChanged; value: "fr" }
                                Option { text: qsTr("Tiếng Đức") + Retranslate.onLanguageChanged; value: "de" }
                                Option { text: qsTr("Tiếng Nga") + Retranslate.onLanguageChanged; value: "ru" }
                                Option { text: qsTr("Tiếng T.B.Nha") + Retranslate.onLanguageChanged; value: "es" }
                                Option { text: qsTr("Tiếng Ý") + Retranslate.onLanguageChanged; value: "it" }
                                Option { text: qsTr("Tiếng B.Đ.Nha") + Retranslate.onLanguageChanged; value: "pt" }
                                Option { text: qsTr("Tiếng Thái") + Retranslate.onLanguageChanged; value: "th" }
                                Option { text: qsTr("Tiếng Indo") + Retranslate.onLanguageChanged; value: "id" }
                                Option { text: qsTr("Tiếng Ả Rập") + Retranslate.onLanguageChanged; value: "ar" }
                            }

                            Button {
                                leftMargin: bookManager.dp(6)
                                rightMargin: bookManager.dp(6)
                                verticalAlignment: VerticalAlignment.Center
                                text: qsTr("Đổi") + Retranslate.onLanguageChanged
                                onClicked: {
                                    swapTranslationLanguages();
                                }
                            }

                            DropDown {
                                id: targetLangDropDown
                                title: qsTr("Sang:") + Retranslate.onLanguageChanged
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                Option { text: qsTr("Tiếng Việt") + Retranslate.onLanguageChanged; value: "vi"; selected: true }
                                Option { text: qsTr("Tiếng Anh") + Retranslate.onLanguageChanged; value: "en" }
                                Option { text: qsTr("Tiếng Trung (GT)") + Retranslate.onLanguageChanged; value: "zh-CN" }
                                Option { text: qsTr("Tiếng Trung (PT)") + Retranslate.onLanguageChanged; value: "zh-TW" }
                                Option { text: qsTr("Tiếng Nhật") + Retranslate.onLanguageChanged; value: "ja" }
                                Option { text: qsTr("Tiếng Hàn") + Retranslate.onLanguageChanged; value: "ko" }
                                Option { text: qsTr("Tiếng Pháp") + Retranslate.onLanguageChanged; value: "fr" }
                                Option { text: qsTr("Tiếng Đức") + Retranslate.onLanguageChanged; value: "de" }
                                Option { text: qsTr("Tiếng Nga") + Retranslate.onLanguageChanged; value: "ru" }
                                Option { text: qsTr("Tiếng T.B.Nha") + Retranslate.onLanguageChanged; value: "es" }
                                Option { text: qsTr("Tiếng Ý") + Retranslate.onLanguageChanged; value: "it" }
                                Option { text: qsTr("Tiếng B.Đ.Nha") + Retranslate.onLanguageChanged; value: "pt" }
                                Option { text: qsTr("Tiếng Thái") + Retranslate.onLanguageChanged; value: "th" }
                                Option { text: qsTr("Tiếng Indo") + Retranslate.onLanguageChanged; value: "id" }
                                Option { text: qsTr("Tiếng Ả Rập") + Retranslate.onLanguageChanged; value: "ar" }
                            }
                        }

                        // Action Button: Dịch Ngay
                        Button {
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: bookManager.dp(10)
                            appearance: ControlAppearance.Primary
                            text: qsTr("Dịch Ngay") + Retranslate.onLanguageChanged
                            enabled: !dictManager.isTranslating && transInputField.text.trim().length > 0
                            onClicked: {
                                dictSheet.hasTranslationOutput = false;
                                dictSheet.translationSuccess = false;
                                dictManager.translateText(transInputField.text.trim(), sourceLangDropDown.selectedValue, targetLangDropDown.selectedValue);
                            }
                        }

                        // Translating Indicator
                        Container {
                            visible: dictManager.isTranslating
                            horizontalAlignment: HorizontalAlignment.Center
                            topMargin: bookManager.dp(10)
                            bottomMargin: bookManager.dp(10)

                            ActivityIndicator {
                                visible: dictManager.isTranslating
                                running: dictManager.isTranslating
                                preferredWidth: bookManager.dp(48)
                                preferredHeight: bookManager.dp(48)
                                onVisibleChanged: { if (visible) start(); else stop(); }
                            }

                            Label {
                                topMargin: bookManager.dp(6)
                                text: qsTr("Đang dịch văn bản...") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }
                        }

                        // Translated Output Card
                        Container {
                            visible: dictSheet.hasTranslationOutput || dictManager.lastTranslatedText.length > 0
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(14)
                            bottomPadding: bookManager.dp(14)
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            bottomMargin: bookManager.dp(10)

                            Label {
                                id: lblTransHeader
                                text: qsTr("Bản dịch:") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.fontWeight: FontWeight.W500
                                textStyle.color: Color.create(bookManager.uiAccent)
                                multiline: true
                            }

                            Container {
                                topMargin: bookManager.dp(8)
                                bottomMargin: bookManager.dp(10)
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 1
                                background: Color.create(bookManager.uiHairline)
                            }

                            Label {
                                id: lblTranslatedResult
                                text: dictManager.lastTranslatedText
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: Color.create(bookManager.uiInk)
                                multiline: true
                            }

                            Container {
                                visible: dictSheet.translationSuccess && dictManager.lastTranslatedText.length > 0
                                topMargin: bookManager.dp(14)
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Button {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("Sao chép") + Retranslate.onLanguageChanged
                                    onClicked: {
                                        dictManager.copyToClipboard(dictManager.lastTranslatedText);
                                    }
                                }

                                Button {
                                    leftMargin: bookManager.dp(10)
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: qsTr("Đọc (TTS)") + Retranslate.onLanguageChanged
                                    onClicked: {
                                        ttsManager.startWithText(dictManager.lastTranslatedText, targetLangDropDown.selectedValue ? targetLangDropDown.selectedValue : "vi");
                                    }
                                }
                            }

                            Button {
                                topMargin: bookManager.dp(10)
                                horizontalAlignment: HorizontalAlignment.Fill
                                visible: dictSheet.translationSuccess && dictManager.lastTranslatedText.length > 0
                                text: qsTr("Lưu vào Flashcard") + Retranslate.onLanguageChanged
                                onClicked: {
                                    if (dictManager.saveVocabWord(transInputField.text.trim(), "", dictManager.lastTranslatedText, (dictSheet.savedContext !== "" ? dictSheet.savedContext : transInputField.text.trim()), bookManager.currentBookTitle)) {
                                        dictSheet.reloadVocab();
                                        dictToast.body = qsTr("Đã lưu vào sổ từ vựng!") + Retranslate.onLanguageChanged;
                                        dictToast.show();
                                    }
                                }
                            }
                        }
                    }

                    // ==========================================
                    // TAB 3: FLASHCARD & SỔ TỪ VỰNG
                    // ==========================================
                    Container {
                        visible: dictSheet.activeTab === "vocab"
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: bookManager.dp(12)
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Label {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                verticalAlignment: VerticalAlignment.Center
                                text: (qsTr("Tổng số thẻ:") + " " + dictSheet.vocabItems.length) + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.fontWeight: FontWeight.W500
                                textStyle.color: Color.create(bookManager.uiAccent)
                            }

                            Button {
                                text: qsTr("Xuất CSV (Anki)") + Retranslate.onLanguageChanged
                                enabled: dictSheet.vocabItems.length > 0
                                onClicked: {
                                    var csv = dictManager.exportVocabCsv();
                                    dictToast.body = (csv !== "" ? (qsTr("Đã xuất CSV: ") + csv) : qsTr("Không thể xuất CSV")) + Retranslate.onLanguageChanged;
                                    dictToast.show();
                                }
                            }
                        }

                        // Empty state
                        Container {
                            visible: dictSheet.vocabItems.length === 0
                            horizontalAlignment: HorizontalAlignment.Fill
                            topPadding: bookManager.dp(36)
                            bottomPadding: bookManager.dp(36)
                            leftPadding: bookManager.dp(20)
                            rightPadding: bookManager.dp(20)
                            background: Color.create(bookManager.uiCard)

                            Label {
                                horizontalAlignment: HorizontalAlignment.Center
                                text: qsTr("Chưa có từ vựng nào.\nHãy tra từ hoặc dịch rồi bấm 'Lưu từ' để ôn tập Flashcard.") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: Color.create(bookManager.uiMuted)
                                multiline: true
                                textStyle.textAlign: TextAlign.Center
                            }
                        }

                        // Interactive Flashcard (flat uiCard, no 1px border wrapper)
                        Container {
                            visible: dictSheet.vocabItems.length > 0
                            horizontalAlignment: HorizontalAlignment.Fill
                            minHeight: bookManager.dp(230)
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(18)
                            bottomPadding: bookManager.dp(20)
                            leftPadding: bookManager.dp(18)
                            rightPadding: bookManager.dp(18)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            gestureHandlers: [
                                TapHandler {
                                    onTapped: {
                                        dictSheet.vocabFlipped = !dictSheet.vocabFlipped;
                                    }
                                }
                            ]

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Label {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: (dictSheet.vocabItems.length > 0) ? ((dictSheet.vocabIndex + 1) + " / " + dictSheet.vocabItems.length) : ""
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create(bookManager.uiMuted)
                                }

                                Label {
                                    text: (dictSheet.vocabItems.length > 0 && dictSheet.vocabItems[dictSheet.vocabIndex] && dictSheet.vocabItems[dictSheet.vocabIndex].mastered)
                                          ? (qsTr("Đã thuộc ✓") + Retranslate.onLanguageChanged)
                                          : (qsTr("Chạm thẻ để lật") + Retranslate.onLanguageChanged)
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.W500
                                    textStyle.color: Color.create(bookManager.uiAccent)
                                }
                            }

                            Label {
                                topMargin: bookManager.dp(18)
                                horizontalAlignment: HorizontalAlignment.Center
                                text: (dictSheet.vocabItems.length > 0 && dictSheet.vocabItems[dictSheet.vocabIndex]) ? dictSheet.vocabItems[dictSheet.vocabIndex].word : ""
                                textStyle.base: SystemDefaults.TextStyles.BigText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiInk)
                                multiline: true
                                textStyle.textAlign: TextAlign.Center
                            }

                            Label {
                                topMargin: bookManager.dp(4)
                                horizontalAlignment: HorizontalAlignment.Center
                                visible: text.length > 0
                                text: (dictSheet.vocabItems.length > 0 && dictSheet.vocabItems[dictSheet.vocabIndex] && dictSheet.vocabItems[dictSheet.vocabIndex].phonetic) ? dictSheet.vocabItems[dictSheet.vocabIndex].phonetic : ""
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiAccent)
                            }

                            Container {
                                visible: dictSheet.vocabFlipped
                                topMargin: bookManager.dp(14)
                                bottomMargin: bookManager.dp(12)
                                horizontalAlignment: HorizontalAlignment.Fill
                                preferredHeight: 1
                                background: Color.create(bookManager.uiHairline)
                            }

                            Label {
                                visible: dictSheet.vocabFlipped
                                horizontalAlignment: HorizontalAlignment.Fill
                                text: (dictSheet.vocabItems.length > 0 && dictSheet.vocabItems[dictSheet.vocabIndex]) ? dictSheet.vocabItems[dictSheet.vocabIndex].meaning : ""
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: Color.create(bookManager.uiInk)
                                multiline: true
                            }

                            Label {
                                visible: dictSheet.vocabFlipped && text.length > 0
                                topMargin: bookManager.dp(8)
                                text: ((dictSheet.vocabItems.length > 0 && dictSheet.vocabItems[dictSheet.vocabIndex] && dictSheet.vocabItems[dictSheet.vocabIndex].bookTitle)
                                      ? (qsTr("Nguồn:") + " " + dictSheet.vocabItems[dictSheet.vocabIndex].bookTitle) : "") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                                multiline: true
                            }
                        }

                        // Flashcard Navigation & Actions
                        Container {
                            visible: dictSheet.vocabItems.length > 0
                            topMargin: bookManager.dp(12)
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Button {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: qsTr("‹ Trước") + Retranslate.onLanguageChanged
                                enabled: dictSheet.vocabIndex > 0
                                onClicked: {
                                    dictSheet.vocabIndex = Math.max(0, dictSheet.vocabIndex - 1);
                                    dictSheet.vocabFlipped = false;
                                }
                            }

                            Button {
                                leftMargin: bookManager.dp(8)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                appearance: ControlAppearance.Primary
                                text: qsTr("Lật thẻ") + Retranslate.onLanguageChanged
                                onClicked: {
                                    dictSheet.vocabFlipped = !dictSheet.vocabFlipped;
                                }
                            }

                            Button {
                                leftMargin: bookManager.dp(8)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: qsTr("Tiếp ›") + Retranslate.onLanguageChanged
                                enabled: dictSheet.vocabIndex + 1 < dictSheet.vocabItems.length
                                onClicked: {
                                    dictSheet.vocabIndex = Math.min(dictSheet.vocabItems.length - 1, dictSheet.vocabIndex + 1);
                                    dictSheet.vocabFlipped = false;
                                }
                            }
                        }

                        Container {
                            visible: dictSheet.vocabItems.length > 0
                            topMargin: bookManager.dp(10)
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Button {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                text: (dictSheet.vocabItems.length > 0 && dictSheet.vocabItems[dictSheet.vocabIndex] && dictSheet.vocabItems[dictSheet.vocabIndex].mastered)
                                      ? qsTr("Đánh dấu chưa thuộc") + Retranslate.onLanguageChanged
                                      : qsTr("Đã thuộc ✓") + Retranslate.onLanguageChanged
                                onClicked: {
                                    dictManager.toggleVocabMastered(dictSheet.vocabIndex);
                                    dictSheet.reloadVocab();
                                }
                            }

                            Button {
                                leftMargin: bookManager.dp(10)
                                text: qsTr("Xóa") + Retranslate.onLanguageChanged
                                onClicked: {
                                    dictManager.removeVocabWord(dictSheet.vocabIndex);
                                    dictSheet.reloadVocab();
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    function handleOfflineLookupDone(res) {
        if (res && res.found) {
            lblResultWord.text = res.word ? res.word : "";
            lblResultPhonetic.text = res.phonetic ? res.phonetic : "";
            lblResultMeaning.text = res.meaning ? res.meaning : "";
            lblSuggestions.text = "";
        }
    }

    function handleOnlineLookupDone(success, word, phonetic, meaning, errorMsg) {
        if (success) {
            lblResultWord.text = word;
            lblResultPhonetic.text = phonetic;
            lblResultMeaning.text = meaning;
            lblSuggestions.text = "";
        } else {
            lblResultWord.text = word;
            lblResultPhonetic.text = "";
            lblResultMeaning.text = errorMsg.length > 0 ? errorMsg : qsTr("Không tìm thấy kết quả từ điển trực tuyến.");
        }
    }

    function handleTranslationDone(success, original, translated, detectedLang) {
        dictSheet.hasTranslationOutput = true;
        dictSheet.translationSuccess = !!success;
        if (typeof lblTranslatedResult !== "undefined" && lblTranslatedResult) {
            lblTranslatedResult.text = translated;
        }
        if (typeof lblTransHeader !== "undefined" && lblTransHeader) {
            if (!success) {
                lblTransHeader.text = qsTr("Lỗi dịch thuật:");
            } else {
                var srcLabel = detectedLang ? getLanguageName(detectedLang) : (typeof sourceLangDropDown !== "undefined" && sourceLangDropDown ? getLanguageName(sourceLangDropDown.selectedValue) : "");
                var tgtLabel = (typeof targetLangDropDown !== "undefined" && targetLangDropDown) ? getLanguageName(targetLangDropDown.selectedValue) : "";
                if (srcLabel.length > 0 && tgtLabel.length > 0) {
                    lblTransHeader.text = qsTr("Bản dịch") + " (" + srcLabel + " - " + tgtLabel + "):";
                } else {
                    lblTransHeader.text = qsTr("Bản dịch:");
                }
            }
        }
    }

    function cleanup() {
        try {
            if (typeof dictManager !== "undefined" && dictManager) {
                dictManager.offlineLookupDone.disconnect(dictSheet.handleOfflineLookupDone);
                dictManager.onlineLookupDone.disconnect(dictSheet.handleOnlineLookupDone);
                dictManager.translationDone.disconnect(dictSheet.handleTranslationDone);
            }
        } catch (e) {}
    }

    onCreationCompleted: {
        dictManager.offlineLookupDone.connect(dictSheet.handleOfflineLookupDone);
        dictManager.onlineLookupDone.connect(dictSheet.handleOnlineLookupDone);
        dictManager.translationDone.connect(dictSheet.handleTranslationDone);
    }

    function selectDropDownValue(dd, val) {
        if (!dd) return;
        for (var i = 0; i < dd.count(); ++i) {
            var opt = dd.at(i);
            if (opt && opt.value === val) {
                dd.setSelectedOption(opt);
                return;
            }
        }
    }

    function getLanguageName(code) {
        if (!code) return "";
        var c = code.toLowerCase();
        if (c === "auto") return qsTr("Tự động");
        if (c === "vi") return qsTr("Tiếng Việt");
        if (c === "en") return qsTr("Tiếng Anh");
        if (c === "zh-cn" || c === "zh") return qsTr("Tiếng Trung (GT)");
        if (c === "zh-tw") return qsTr("Tiếng Trung (PT)");
        if (c === "ja") return qsTr("Tiếng Nhật");
        if (c === "ko") return qsTr("Tiếng Hàn");
        if (c === "fr") return qsTr("Tiếng Pháp");
        if (c === "de") return qsTr("Tiếng Đức");
        if (c === "ru") return qsTr("Tiếng Nga");
        if (c === "es") return qsTr("Tiếng T.B.Nha");
        if (c === "it") return qsTr("Tiếng Ý");
        if (c === "pt") return qsTr("Tiếng B.Đ.Nha");
        if (c === "th") return qsTr("Tiếng Thái");
        if (c === "id") return qsTr("Tiếng Indo");
        if (c === "ar") return qsTr("Tiếng Ả Rập");
        return code.toUpperCase();
    }

    function swapTranslationLanguages() {
        var srcVal = (typeof sourceLangDropDown !== "undefined" && sourceLangDropDown) ? sourceLangDropDown.selectedValue : "auto";
        var tgtVal = (typeof targetLangDropDown !== "undefined" && targetLangDropDown) ? targetLangDropDown.selectedValue : "vi";

        if (srcVal === "auto") {
            selectDropDownValue(sourceLangDropDown, tgtVal);
            selectDropDownValue(targetLangDropDown, tgtVal === "vi" ? "en" : "vi");
        } else {
            selectDropDownValue(sourceLangDropDown, tgtVal);
            selectDropDownValue(targetLangDropDown, srcVal);
        }

        if (dictManager.lastTranslatedText.length > 0 && transInputField.text.trim().length > 0) {
            transInputField.text = dictManager.lastTranslatedText;
            dictSheet.hasTranslationOutput = false;
            dictSheet.translationSuccess = false;
            dictManager.translateText(transInputField.text.trim(), sourceLangDropDown.selectedValue, targetLangDropDown.selectedValue);
        }
    }

    function performDictLookup() {
        var w = dictWordField.text.trim();
        if (w.length === 0) return;
        pushRecentWord(w);

        var tgt = (typeof dictTargetLangDropDown !== "undefined" && dictTargetLangDropDown) ? dictTargetLangDropDown.selectedValue : "vi";
        if (tgt === "vi") {
            var res = dictManager.lookupOffline(w);
            if (res.found) {
                lblResultWord.text = res.word;
                lblResultPhonetic.text = res.phonetic;
                lblResultMeaning.text = res.meaning;
                lblSuggestions.text = "";
                return;
            }
            if (res.suggestions && res.suggestions.length > 0) {
                lblSuggestions.text = qsTr("Gợi ý từ tương tự: ") + res.suggestions.join(", ");
            } else {
                lblSuggestions.text = "";
            }
        } else {
            lblSuggestions.text = "";
        }

        lblResultWord.text = w;
        lblResultPhonetic.text = "";
        lblResultMeaning.text = qsTr("Đang tra từ điển trực tuyến...");
        dictManager.lookupOnline(w, tgt);
    }

    function openWithWord(word) {
        var cleanW = word ? word.trim() : "";
        dictSheet.savedContext = (dictSheet.initialText && dictSheet.initialText.trim().length > cleanW.length) ? dictSheet.initialText.trim() : "";
        dictSheet.initialText = "";
        dictWordField.text = cleanW;
        transInputField.text = cleanW;
        optDict.selected = true;
        dictSheet.activeTab = "dict";
        dictSheet.open();
        performDictLookup();
    }

    function openWithSentence(text) {
        var cleanT = text ? text.trim() : "";
        dictSheet.savedContext = cleanT;
        dictSheet.initialText = "";
        dictSheet.hasTranslationOutput = false;
        dictSheet.translationSuccess = false;
        transInputField.text = cleanT;
        dictWordField.text = cleanT.split(" ")[0];
        optTrans.selected = true;
        dictSheet.activeTab = "translate";
        dictSheet.open();
        var src = (typeof sourceLangDropDown !== "undefined" && sourceLangDropDown) ? sourceLangDropDown.selectedValue : "auto";
        var tgt = (typeof targetLangDropDown !== "undefined" && targetLangDropDown) ? targetLangDropDown.selectedValue : "vi";
        dictManager.translateText(cleanT, src, tgt);
    }

    attachedObjects: [
        SystemToast {
            id: dictToast
            body: ""
        }
    ]

    onOpened: {
        reloadVocab();
        if (dictSheet.initialText.length > 0) {
            var pending = dictSheet.initialText;
            dictSheet.initialText = "";
            if (pending.indexOf(" ") !== -1 || pending.length > 25) {
                openWithSentence(pending);
            } else {
                openWithWord(pending);
            }
        }
    }
}
