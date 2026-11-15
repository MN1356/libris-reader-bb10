import bb.cascades 1.3

Sheet {
    id: dictSheet

    property string initialText: ""
    property string activeTab: "dict" // "dict", "translate"

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
            layout: DockLayout {}
            background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")

            ScrollView {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                scrollViewProperties.scrollMode: ScrollMode.Vertical

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    topPadding: 12
                    bottomPadding: 24
                    leftPadding: 12
                    rightPadding: 12

                    // Tab selector
                    SegmentedControl {
                        id: tabControl
                        horizontalAlignment: HorizontalAlignment.Fill
                        bottomMargin: 14

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

                        onSelectedIndexChanged: {
                            dictSheet.activeTab = selectedValue;
                        }
                    }

                    // ==========================================
                    // TAB 1: TRA TỪ ĐIỂN (OFFLINE & ONLINE)
                    // ==========================================
                    Container {
                        visible: dictSheet.activeTab === "dict"
                        horizontalAlignment: HorizontalAlignment.Fill

                        // Search Input
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                            topPadding: 8
                            bottomPadding: 8
                            leftPadding: 10
                            rightPadding: 10
                            bottomMargin: 10

                            layout: StackLayout {
                                orientation: LayoutOrientation.LeftToRight
                            }

                            TextField {
                                id: dictWordField
                                verticalAlignment: VerticalAlignment.Center
                                layoutProperties: StackLayoutProperties {
                                    spaceQuota: 1.0
                                }
                                hintText: qsTr("Nhập từ cần tra...") + Retranslate.onLanguageChanged
                                input.submitKey: SubmitKey.Search
                                input.onSubmitted: {
                                    performDictLookup();
                                }
                            }

                            Button {
                                text: qsTr("Tra từ") + Retranslate.onLanguageChanged
                                preferredWidth: 110
                                verticalAlignment: VerticalAlignment.Center
                                onClicked: {
                                    performDictLookup();
                                }
                            }
                        }

                        // Offline Result Card
                        Container {
                            visible: dictWordField.text.trim().length > 0
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                            topPadding: 14
                            bottomPadding: 14
                            leftPadding: 14
                            rightPadding: 14
                            bottomMargin: 12

                            layout: StackLayout {
                                orientation: LayoutOrientation.TopToBottom
                            }

                            // Header: Word + Phonetic
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout {
                                    orientation: LayoutOrientation.LeftToRight
                                }

                                Label {
                                    id: lblResultWord
                                    verticalAlignment: VerticalAlignment.Center
                                    text: dictManager.lastWord
                                    textStyle.base: SystemDefaults.TextStyles.TitleText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                }

                                Label {
                                    id: lblResultPhonetic
                                    leftMargin: 8
                                    verticalAlignment: VerticalAlignment.Center
                                    text: dictManager.lastPhonetic
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                }

                                Container {
                                    layoutProperties: StackLayoutProperties {
                                        spaceQuota: 1.0
                                    }
                                }

                                Button {
                                    preferredWidth: 120
                                    text: qsTr("Sao chép") + Retranslate.onLanguageChanged
                                    verticalAlignment: VerticalAlignment.Center
                                    onClicked: {
                                        dictManager.copyToClipboard(lblResultWord.text + " " + lblResultPhonetic.text + "\n" + lblResultMeaning.text);
                                    }
                                }
                            }

                            Divider {
                                topMargin: 6
                                bottomMargin: 8
                            }

                            // Definition / Meaning
                            Label {
                                id: lblResultMeaning
                                text: dictManager.lastMeaning
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                multiline: true
                            }

                            // Suggestions if word not found
                            Label {
                                id: lblSuggestions
                                visible: text.length > 0
                                topMargin: 6
                                text: ""
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                multiline: true
                            }
                        }

                        // Online Lookup Language & Action
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: 8
                            layout: StackLayout {
                                orientation: LayoutOrientation.LeftToRight
                            }

                            DropDown {
                                id: dictTargetLangDropDown
                                title: qsTr("Dịch nghĩa sang:") + Retranslate.onLanguageChanged
                                layoutProperties: StackLayoutProperties {
                                    spaceQuota: 1.0
                                }
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
                                leftMargin: 8
                                preferredWidth: 160
                                verticalAlignment: VerticalAlignment.Center
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
                            topMargin: 12

                            ActivityIndicator {
                                visible: dictManager.isLookingUp
                                running: dictManager.isLookingUp
                                preferredWidth: 48
                                preferredHeight: 48
                                onVisibleChanged: { if (visible) start(); else stop(); }
                            }

                            Label {
                                topMargin: 6
                                text: qsTr("Đang tra từ trực tuyến...") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                            }
                        }
                    }

                    // ==========================================
                    // TAB 2: DỊCH ĐOẠN VĂN BẢN (TRANSLATE)
                    // ==========================================
                    Container {
                        visible: dictSheet.activeTab === "translate"
                        horizontalAlignment: HorizontalAlignment.Fill

                        // Input TextArea
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                            topPadding: 10
                            bottomPadding: 10
                            leftPadding: 10
                            rightPadding: 10
                            bottomMargin: 10

                            Label {
                                text: qsTr("Văn bản gốc:") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.XSmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                                bottomMargin: 4
                            }

                            TextArea {
                                id: transInputField
                                preferredHeight: 120
                                hintText: qsTr("Nhập đoạn văn bản cần dịch...") + Retranslate.onLanguageChanged
                            }
                        }

                        // Multilingual Controls: Source & Target Language Pickers + Swap Button
                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: 8
                            layout: StackLayout {
                                orientation: LayoutOrientation.LeftToRight
                            }

                            DropDown {
                                id: sourceLangDropDown
                                title: qsTr("Từ:") + Retranslate.onLanguageChanged
                                layoutProperties: StackLayoutProperties {
                                    spaceQuota: 1.0
                                }
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
                                preferredWidth: 70
                                verticalAlignment: VerticalAlignment.Center
                                text: qsTr("Đổi") + Retranslate.onLanguageChanged
                                onClicked: {
                                    swapTranslationLanguages();
                                }
                            }

                            DropDown {
                                id: targetLangDropDown
                                title: qsTr("Sang:") + Retranslate.onLanguageChanged
                                layoutProperties: StackLayoutProperties {
                                    spaceQuota: 1.0
                                }
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
                            bottomMargin: 10
                            text: qsTr("Dịch Ngay") + Retranslate.onLanguageChanged
                            enabled: !dictManager.isTranslating && transInputField.text.trim().length > 0
                            onClicked: {
                                dictManager.translateText(transInputField.text.trim(), sourceLangDropDown.selectedValue, targetLangDropDown.selectedValue);
                            }
                        }

                        // Translating Indicator
                        Container {
                            visible: dictManager.isTranslating
                            horizontalAlignment: HorizontalAlignment.Center
                            topMargin: 8
                            bottomMargin: 8

                            ActivityIndicator {
                                visible: dictManager.isTranslating
                                running: dictManager.isTranslating
                                preferredWidth: 48
                                preferredHeight: 48
                                onVisibleChanged: { if (visible) start(); else stop(); }
                            }

                            Label {
                                topMargin: 4
                                text: qsTr("Đang dịch văn bản...") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                            }
                        }

                        // Translated Output Card
                        Container {
                            visible: dictManager.lastTranslatedText.length > 0
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
                            topPadding: 12
                            bottomPadding: 12
                            leftPadding: 14
                            rightPadding: 14
                            bottomMargin: 10

                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout {
                                    orientation: LayoutOrientation.LeftToRight
                                }

                                Label {
                                    id: lblTransHeader
                                    verticalAlignment: VerticalAlignment.Center
                                    text: qsTr("Bản dịch:") + Retranslate.onLanguageChanged
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: Color.create("#10B981")
                                }

                                Container {
                                    layoutProperties: StackLayoutProperties {
                                        spaceQuota: 1.0
                                    }
                                }

                                Button {
                                    text: qsTr("Sao chép") + Retranslate.onLanguageChanged
                                    preferredWidth: 120
                                    verticalAlignment: VerticalAlignment.Center
                                    onClicked: {
                                        dictManager.copyToClipboard(dictManager.lastTranslatedText);
                                    }
                                }

                                Button {
                                    leftMargin: 6
                                    text: qsTr("Đọc (TTS)") + Retranslate.onLanguageChanged
                                    preferredWidth: 125
                                    verticalAlignment: VerticalAlignment.Center
                                    onClicked: {
                                        ttsManager.startWithText(dictManager.lastTranslatedText);
                                    }
                                }
                            }

                            Divider {
                                topMargin: 6
                                bottomMargin: 8
                            }

                            Label {
                                id: lblTranslatedResult
                                text: dictManager.lastTranslatedText
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                                multiline: true
                            }
                        }
                    }
                }
            }
        }
    }

    onCreationCompleted: {
        dictManager.onlineLookupDone.connect(function(success, word, phonetic, meaning, errorMsg) {
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
        });
        dictManager.translationDone.connect(function(success, original, translated, detectedLang) {
            if (typeof lblTranslatedResult !== "undefined" && lblTranslatedResult) {
                lblTranslatedResult.text = translated;
            }
            if (typeof lblTransHeader !== "undefined" && lblTransHeader) {
                var srcLabel = detectedLang ? getLanguageName(detectedLang) : (typeof sourceLangDropDown !== "undefined" && sourceLangDropDown ? getLanguageName(sourceLangDropDown.selectedValue) : "");
                var tgtLabel = (typeof targetLangDropDown !== "undefined" && targetLangDropDown) ? getLanguageName(targetLangDropDown.selectedValue) : "";
                if (srcLabel.length > 0 && tgtLabel.length > 0) {
                    lblTransHeader.text = qsTr("Bản dịch") + " (" + srcLabel + " - " + tgtLabel + "):";
                } else {
                    lblTransHeader.text = qsTr("Bản dịch:");
                }
            }
        });
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
            var oldInput = transInputField.text;
            transInputField.text = dictManager.lastTranslatedText;
            dictManager.translateText(transInputField.text.trim(), sourceLangDropDown.selectedValue, targetLangDropDown.selectedValue);
        }
    }

    function performDictLookup() {
        var w = dictWordField.text.trim();
        if (w.length === 0) return;

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
        if (cleanW.indexOf("%") !== -1) {
            try { cleanW = decodeURIComponent(cleanW); } catch(e) {}
        }
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
        if (cleanT.indexOf("%") !== -1) {
            try { cleanT = decodeURIComponent(cleanT); } catch(e) {}
        }
        dictSheet.initialText = "";
        transInputField.text = cleanT;
        dictWordField.text = cleanT.split(" ")[0];
        optTrans.selected = true;
        dictSheet.activeTab = "translate";
        dictSheet.open();
        var src = (typeof sourceLangDropDown !== "undefined" && sourceLangDropDown) ? sourceLangDropDown.selectedValue : "auto";
        var tgt = (typeof targetLangDropDown !== "undefined" && targetLangDropDown) ? targetLangDropDown.selectedValue : "vi";
        dictManager.translateText(cleanT, src, tgt);
    }

    onOpened: {
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
