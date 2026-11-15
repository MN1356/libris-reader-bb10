import bb.cascades 1.3

Page {
    id: ttsSettingsPage

    signal backRequested()

    titleBar: TitleBar {
        title: qsTr("Giọng đọc sách") + Retranslate.onLanguageChanged
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

            Header {
                title: qsTr("BỘ MÁY & GIỌNG ĐỌC") + Retranslate.onLanguageChanged
            }

            // 1. DropDown Chọn Bộ máy TTS
            DropDown {
                id: ttsProviderDropDown
                topMargin: 8
                title: qsTr("Bộ máy") + Retranslate.onLanguageChanged

                Option {
                    text: qsTr("Google Dịch") + Retranslate.onLanguageChanged
                    value: "google"
                    selected: ttsManager.ttsProvider === "google"
                }
                Option {
                    text: qsTr("Google Gemini AI") + Retranslate.onLanguageChanged
                    value: "gemini"
                    selected: ttsManager.ttsProvider === "gemini"
                }

                onSelectedValueChanged: {
                    if (selectedValue && selectedValue !== ttsManager.ttsProvider) {
                        ttsManager.setTtsProvider(selectedValue);
                    }
                }
            }

            // 2. Google Gemini AI Audio
            Container {
                visible: ttsManager.ttsProvider === "gemini"
                topMargin: 8
                horizontalAlignment: HorizontalAlignment.Fill

                Label {
                    topMargin: 4
                    text: qsTr("Google API Key:") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")
                }

                TextField {
                    id: geminiApiKeyField
                    topMargin: 2
                    hintText: "AIzaSy..."
                    text: ttsManager.geminiApiKey
                    onTextChanging: {
                        ttsManager.setGeminiApiKey(text);
                    }
                }

                // Nút Test Call và Báo trạng thái API Key
                Container {
                    topMargin: 8
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }

                    Button {
                        id: testGeminiApiBtn
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1.0
                        }
                        text: ttsManager.isTestingGemini ? 
                              (qsTr("Đang kiểm tra...") + Retranslate.onLanguageChanged) : 
                              (qsTr("Kiểm tra") + Retranslate.onLanguageChanged)
                        enabled: !ttsManager.isTestingGemini
                        onClicked: {
                            ttsManager.testGeminiApiKey(geminiApiKeyField.text);
                        }
                    }

                    ActivityIndicator {
                        visible: ttsManager.isTestingGemini
                        running: ttsManager.isTestingGemini
                        verticalAlignment: VerticalAlignment.Center
                        leftMargin: 8
                        onVisibleChanged: { if (visible) start(); else stop(); }
                    }
                }

                // Hộp hiển thị kết quả kiểm tra
                Container {
                    visible: ttsManager.geminiTestStatus !== ""
                    topMargin: 6
                    bottomMargin: 4
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: ttsManager.geminiTestState === "success" ?
                                (bookManager.appTheme === "dark" ? Color.create("#064E3B") : Color.create("#DCFCE7")) :
                                (ttsManager.geminiTestState === "error" ?
                                 (bookManager.appTheme === "dark" ? Color.create("#7F1D1D") : Color.create("#FEE2E2")) :
                                 (bookManager.appTheme === "dark" ? Color.create("#141414") : Color.create("#F5F5F5")))
                    topPadding: 10
                    bottomPadding: 10
                    leftPadding: 12
                    rightPadding: 12

                    Label {
                        text: ttsManager.geminiTestStatus
                        multiline: true
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: ttsManager.geminiTestState === "success" ?
                                         (bookManager.appTheme === "dark" ? Color.create("#34D399") : Color.create("#15803D")) :
                                         (ttsManager.geminiTestState === "error" ?
                                          (bookManager.appTheme === "dark" ? Color.create("#F87171") : Color.create("#B91C1C")) :
                                          (bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#666666")))
                    }
                }

                DropDown {
                    id: geminiVoiceDropDown
                    topMargin: 8
                    title: qsTr("Giọng đọc") + Retranslate.onLanguageChanged

                    Option {
                        text: qsTr("Puck (Nam - Ấm áp)") + Retranslate.onLanguageChanged
                        value: "Puck"
                        selected: ttsManager.geminiVoice === "Puck" || ttsManager.geminiVoice === ""
                    }
                    Option {
                        text: qsTr("Charon (Nam - Trầm tĩnh)") + Retranslate.onLanguageChanged
                        value: "Charon"
                        selected: ttsManager.geminiVoice === "Charon"
                    }
                    Option {
                        text: qsTr("Kore (Nữ - Trong trẻo)") + Retranslate.onLanguageChanged
                        value: "Kore"
                        selected: ttsManager.geminiVoice === "Kore"
                    }
                    Option {
                        text: qsTr("Fenrir (Nam - Uy lực)") + Retranslate.onLanguageChanged
                        value: "Fenrir"
                        selected: ttsManager.geminiVoice === "Fenrir"
                    }
                    Option {
                        text: qsTr("Aoede (Nữ - Êm dịu)") + Retranslate.onLanguageChanged
                        value: "Aoede"
                        selected: ttsManager.geminiVoice === "Aoede"
                    }
                    Option {
                        text: qsTr("Zephyr (Nữ - Sống động)") + Retranslate.onLanguageChanged
                        value: "Zephyr"
                        selected: ttsManager.geminiVoice === "Zephyr"
                    }
                    Option {
                        text: qsTr("Leda (Nữ - Điềm đạm)") + Retranslate.onLanguageChanged
                        value: "Leda"
                        selected: ttsManager.geminiVoice === "Leda"
                    }

                    onSelectedValueChanged: {
                        if (selectedValue && selectedValue !== ttsManager.geminiVoice) {
                            ttsManager.setGeminiVoice(selectedValue);
                        }
                    }
                }

                DropDown {
                    id: geminiModelPresetDropDown
                    topMargin: 8
                    title: qsTr("Mô hình") + Retranslate.onLanguageChanged

                    Option {
                        text: qsTr("Gemini 2.5 Flash") + Retranslate.onLanguageChanged
                        value: "gemini-2.5-flash"
                        selected: ttsManager.geminiModel === "gemini-2.5-flash" || ttsManager.geminiModel === "gemini-2.0-flash" || ttsManager.geminiModel === ""
                    }
                    Option {
                        text: qsTr("Gemini 2.5 Flash (Âm thanh gốc)") + Retranslate.onLanguageChanged
                        value: "gemini-2.5-flash-native-audio"
                        selected: ttsManager.geminiModel === "gemini-2.5-flash-native-audio"
                    }
                    Option {
                        text: qsTr("Gemini 3.8 Flash TTS") + Retranslate.onLanguageChanged
                        value: "gemini-3.8-flash-tts"
                        selected: ttsManager.geminiModel === "gemini-3.8-flash-tts"
                    }
                    Option {
                        text: qsTr("Tùy chỉnh") + Retranslate.onLanguageChanged
                        value: "custom"
                        selected: ttsManager.geminiModel !== "gemini-2.5-flash" && 
                                  ttsManager.geminiModel !== "gemini-2.0-flash" && 
                                  ttsManager.geminiModel !== "" && 
                                  ttsManager.geminiModel !== "gemini-2.5-flash-native-audio" && 
                                  ttsManager.geminiModel !== "gemini-3.8-flash-tts"
                    }

                    onSelectedValueChanged: {
                        if (selectedValue && selectedValue !== "custom") {
                            geminiModelField.text = selectedValue;
                            ttsManager.setGeminiModel(selectedValue);
                        }
                    }
                }

                TextField {
                    id: geminiModelField
                    topMargin: 2
                    hintText: "gemini-2.5-flash"
                    text: ttsManager.geminiModel
                    onTextChanging: {
                        ttsManager.setGeminiModel(text);
                    }
                }

                DropDown {
                    id: geminiStyleDropDown
                    topMargin: 8
                    title: qsTr("Phong cách") + Retranslate.onLanguageChanged

                    Option {
                        text: qsTr("Tự động") + Retranslate.onLanguageChanged
                        value: "auto"
                        selected: ttsManager.geminiStyle === "auto" || ttsManager.geminiStyle === ""
                    }
                    Option {
                        text: qsTr("Kể chuyện") + Retranslate.onLanguageChanged
                        value: "storytelling"
                        selected: ttsManager.geminiStyle === "storytelling"
                    }
                    Option {
                        text: qsTr("Trang trọng") + Retranslate.onLanguageChanged
                        value: "formal"
                        selected: ttsManager.geminiStyle === "formal"
                    }
                    Option {
                        text: qsTr("Thư thái") + Retranslate.onLanguageChanged
                        value: "calm"
                        selected: ttsManager.geminiStyle === "calm"
                    }

                    onSelectedValueChanged: {
                        if (selectedValue && selectedValue !== ttsManager.geminiStyle) {
                            ttsManager.setGeminiStyle(selectedValue);
                        }
                    }
                }
            }

            Header {
                topMargin: 16
                title: qsTr("NGÔN NGỮ & TỐC ĐỘ") + Retranslate.onLanguageChanged
            }

            // Ngôn ngữ
            DropDown {
                id: ttsLanguageDropDown
                topMargin: 8
                title: qsTr("Ngôn ngữ") + Retranslate.onLanguageChanged

                Option {
                    text: qsTr("Tự động nhận diện") + Retranslate.onLanguageChanged
                    value: "auto"
                    selected: ttsManager.language === "auto"
                }
                Option {
                    text: qsTr("Tiếng Việt chuẩn (vi)") + Retranslate.onLanguageChanged
                    value: "vi"
                    selected: ttsManager.language === "vi"
                }
                Option {
                    text: qsTr("English - Giọng Mỹ (en-US)") + Retranslate.onLanguageChanged
                    value: "en-US"
                    selected: ttsManager.language === "en-US" || ttsManager.language === "en"
                }
                Option {
                    text: qsTr("English - Giọng Anh (en-GB)") + Retranslate.onLanguageChanged
                    value: "en-GB"
                    selected: ttsManager.language === "en-GB"
                }
                Option {
                    text: qsTr("English - Giọng Úc (en-AU)") + Retranslate.onLanguageChanged
                    value: "en-AU"
                    selected: ttsManager.language === "en-AU"
                }
                Option {
                    text: qsTr("English - Giọng Ấn Độ (en-IN)") + Retranslate.onLanguageChanged
                    value: "en-IN"
                    selected: ttsManager.language === "en-IN"
                }
                Option {
                    text: qsTr("Français - Pháp (fr-FR)") + Retranslate.onLanguageChanged
                    value: "fr-FR"
                    selected: ttsManager.language === "fr-FR" || ttsManager.language === "fr"
                }
                Option {
                    text: qsTr("Français - Canada (fr-CA)") + Retranslate.onLanguageChanged
                    value: "fr-CA"
                    selected: ttsManager.language === "fr-CA"
                }
                Option {
                    text: qsTr("Español - Tây Ban Nha (es-ES)") + Retranslate.onLanguageChanged
                    value: "es-ES"
                    selected: ttsManager.language === "es-ES" || ttsManager.language === "es"
                }
                Option {
                    text: qsTr("Español - Mỹ Latinh (es-US)") + Retranslate.onLanguageChanged
                    value: "es-US"
                    selected: ttsManager.language === "es-US"
                }
                Option {
                    text: qsTr("Deutsch (de)") + Retranslate.onLanguageChanged
                    value: "de"
                    selected: ttsManager.language === "de"
                }
                Option {
                    text: qsTr("Русский (ru)") + Retranslate.onLanguageChanged
                    value: "ru"
                    selected: ttsManager.language === "ru"
                }
                Option {
                    text: qsTr("中文 - Giản thể (zh-CN)") + Retranslate.onLanguageChanged
                    value: "zh-CN"
                    selected: ttsManager.language === "zh-CN"
                }
                Option {
                    text: qsTr("中文 - Đài Loan (zh-TW)") + Retranslate.onLanguageChanged
                    value: "zh-TW"
                    selected: ttsManager.language === "zh-TW"
                }
                Option {
                    text: qsTr("中文 - Quảng Đông / HK (zh-HK)") + Retranslate.onLanguageChanged
                    value: "zh-HK"
                    selected: ttsManager.language === "zh-HK"
                }
                Option {
                    text: qsTr("日本語 (ja)") + Retranslate.onLanguageChanged
                    value: "ja"
                    selected: ttsManager.language === "ja"
                }
                Option {
                    text: qsTr("한국어 (ko)") + Retranslate.onLanguageChanged
                    value: "ko"
                    selected: ttsManager.language === "ko"
                }

                onSelectedValueChanged: {
                    if (selectedValue && selectedValue !== ttsManager.language) {
                        ttsManager.setLanguage(selectedValue);
                    }
                }
            }

            // Tốc độ
            DropDown {
                id: ttsSpeedDropDown
                topMargin: 12
                title: qsTr("Tốc độ") + Retranslate.onLanguageChanged

                Option {
                    text: qsTr("Chậm") + Retranslate.onLanguageChanged
                    description: "0.8x"
                    value: 0.8
                    selected: ttsManager.speed == 0.8
                }
                Option {
                    text: qsTr("Chuẩn") + Retranslate.onLanguageChanged
                    description: "1.0x"
                    value: 1.0
                    selected: ttsManager.speed == 1.0
                }
                Option {
                    text: qsTr("Nhanh") + Retranslate.onLanguageChanged
                    description: "1.25x"
                    value: 1.25
                    selected: ttsManager.speed == 1.25
                }
                Option {
                    text: qsTr("Rất nhanh") + Retranslate.onLanguageChanged
                    description: "1.5x"
                    value: 1.5
                    selected: ttsManager.speed == 1.5
                }

                onSelectedValueChanged: {
                    if (selectedValue) {
                        ttsManager.setSpeed(selectedValue);
                    }
                }
            }

            // Cao độ giọng đọc
            DropDown {
                id: ttsPitchDropDown
                topMargin: 12
                title: qsTr("Cao độ") + Retranslate.onLanguageChanged

                Option {
                    text: qsTr("Trầm") + Retranslate.onLanguageChanged
                    value: 0.8
                    selected: ttsManager.pitch == 0.8
                }
                Option {
                    text: qsTr("Chuẩn") + Retranslate.onLanguageChanged
                    value: 1.0
                    selected: ttsManager.pitch == 1.0
                }
                Option {
                    text: qsTr("Bổng") + Retranslate.onLanguageChanged
                    value: 1.2
                    selected: ttsManager.pitch == 1.2
                }
                Option {
                    text: qsTr("Rất bổng") + Retranslate.onLanguageChanged
                    value: 1.4
                    selected: ttsManager.pitch == 1.4
                }

                onSelectedValueChanged: {
                    if (selectedValue) {
                        ttsManager.setPitch(selectedValue);
                    }
                }
            }

            // Nút nghe thử
            Button {
                id: testVoiceButton
                topMargin: 14
                horizontalAlignment: HorizontalAlignment.Fill
                text: ttsManager.loading ? (qsTr("Đang tải dữ liệu...") + Retranslate.onLanguageChanged) :
                      (ttsManager.playing ? (qsTr("Dừng đọc") + Retranslate.onLanguageChanged) : (qsTr("Nghe thử giọng") + Retranslate.onLanguageChanged))
                onClicked: {
                    if (ttsManager.playing) {
                        ttsManager.stop();
                    } else {
                        ttsManager.testVoice();
                    }
                }
            }

            Header {
                topMargin: 20
                title: qsTr("PHÁT & NGOẠI TUYẾN") + Retranslate.onLanguageChanged
            }

            DropDown {
                id: ttsModeDropDown
                topMargin: 8
                title: qsTr("Chế độ") + Retranslate.onLanguageChanged

                Option {
                    text: qsTr("Trực tuyến") + Retranslate.onLanguageChanged
                    value: "online"
                    selected: ttsManager.ttsMode === "online"
                }
                Option {
                    text: qsTr("Ngoại tuyến") + Retranslate.onLanguageChanged
                    value: "offline"
                    selected: ttsManager.ttsMode === "offline"
                }

                onSelectedValueChanged: {
                    if (selectedValue && selectedValue !== ttsManager.ttsMode) {
                        ttsManager.setTtsMode(selectedValue);
                    }
                }
            }

            ToggleableListItem {
                title: qsTr("Tự chuyển chương") + Retranslate.onLanguageChanged
                subtitle: qsTr("Tự động chuyển sang chương tiếp theo khi đọc hết") + Retranslate.onLanguageChanged
                checked: bookManager.ttsAutoAdvance
                onToggled: {
                    bookManager.setTtsAutoAdvance(checked);
                }
            }

            Button {
                topMargin: 12
                horizontalAlignment: HorizontalAlignment.Fill
                text: ttsManager.isCaching ? 
                      ((qsTr("Đang tải: ") + Retranslate.onLanguageChanged) + ttsManager.cacheProgress + " / " + ttsManager.totalSentences) : 
                      (qsTr("Tải chương để nghe Offline") + Retranslate.onLanguageChanged)
                enabled: !ttsManager.isCaching && ttsManager.totalSentences > 0
                onClicked: {
                    ttsManager.preCacheChapter();
                }
            }

            Button {
                topMargin: 10
                text: qsTr("Xóa bộ nhớ đệm") + Retranslate.onLanguageChanged
                horizontalAlignment: HorizontalAlignment.Fill
                onClicked: {
                    ttsManager.clearCache();
                    clearCacheLabel.visible = true;
                }
            }

            Label {
                id: clearCacheLabel
                visible: false
                topMargin: 6
                text: qsTr("Đã dọn dẹp xong bộ nhớ đệm âm thanh.") + Retranslate.onLanguageChanged
                textStyle.base: SystemDefaults.TextStyles.BodyText
                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                multiline: true
            }
        }
    }
}
