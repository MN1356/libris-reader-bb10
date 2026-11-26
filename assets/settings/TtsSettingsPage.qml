import bb.cascades 1.3
import bb.system 1.2

Page {
    id: ttsSettingsPage
    property bool isInitializingSleepDropdown: true

    signal backRequested()

    titleBar: TitleBar {
        title: qsTr("Giọng đọc sách") + Retranslate.onLanguageChanged
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
                // Section 1: BỘ MÁY & GIỌNG ĐỌC
                // =========================================================
                Header {
                    title: qsTr("BỘ MÁY & GIỌNG ĐỌC") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(16)

                    DropDown {
                        id: ttsProviderDropDown
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
                        Option {
                            text: qsTr("eSpeak-NG (Ngoại tuyến)") + Retranslate.onLanguageChanged
                            value: "espeak"
                            selected: ttsManager.ttsProvider === "espeak"
                        }

                        onSelectedValueChanged: {
                            if (selectedValue && selectedValue !== ttsManager.ttsProvider) {
                                ttsManager.setTtsProvider(selectedValue);
                            }
                        }
                    }

                    // Google Gemini AI Audio config
                    Container {
                        visible: ttsManager.ttsProvider === "gemini"
                        topMargin: bookManager.dp(10)
                        horizontalAlignment: HorizontalAlignment.Fill

                        Label {
                            text: qsTr("Google API Key:") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }

                        TextField {
                            id: geminiApiKeyField
                            topMargin: bookManager.dp(4)
                            hintText: "AIzaSy..."
                            inputMode: TextFieldInputMode.Password
                            clearButtonVisible: true
                            text: ttsManager.geminiApiKey
                            onTextChanging: {
                                ttsManager.setGeminiApiKey(text);
                            }
                        }

                        Container {
                            topMargin: bookManager.dp(10)
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Button {
                                id: testGeminiApiBtn
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
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
                                leftMargin: bookManager.dp(8)
                                onVisibleChanged: { if (visible) start(); else stop(); }
                            }
                        }

                        Container {
                            visible: ttsManager.geminiTestStatus !== ""
                            topMargin: bookManager.dp(8)
                            bottomMargin: bookManager.dp(4)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: ttsManager.geminiTestState === "success" ?
                                        Color.create(bookManager.uiActive) :
                                        Color.create(bookManager.uiSurface)
                            topPadding: bookManager.dp(10)
                            bottomPadding: bookManager.dp(10)
                            leftPadding: bookManager.dp(12)
                            rightPadding: bookManager.dp(12)

                            Label {
                                text: ttsManager.geminiTestStatus
                                multiline: true
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: ttsManager.geminiTestState === "success" ?
                                                 Color.create(bookManager.uiAccent) :
                                                 (ttsManager.geminiTestState === "error" ?
                                                  Color.create(bookManager.uiDanger) :
                                                  Color.create(bookManager.uiMuted))
                            }
                        }

                        DropDown {
                            id: geminiVoiceDropDown
                            topMargin: bookManager.dp(10)
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
                            topMargin: bookManager.dp(10)
                            title: qsTr("Mô hình") + Retranslate.onLanguageChanged

                            Option {
                                text: "Gemini 2.5 Flash TTS"
                                value: "gemini-2.5-flash-preview-tts"
                                selected: ttsManager.geminiModel === "gemini-2.5-flash-preview-tts" ||
                                          ttsManager.geminiModel === "gemini-2.5-flash" ||
                                          ttsManager.geminiModel === "gemini-2.0-flash" ||
                                          ttsManager.geminiModel === "gemini-2.5-flash-native-audio" ||
                                          ttsManager.geminiModel === ""
                            }
                            Option {
                                text: "Gemini 2.5 Pro TTS"
                                value: "gemini-2.5-pro-preview-tts"
                                selected: ttsManager.geminiModel === "gemini-2.5-pro-preview-tts"
                            }
                            Option {
                                text: qsTr("Tùy chỉnh") + Retranslate.onLanguageChanged
                                value: "custom"
                                selected: ttsManager.geminiModel !== "gemini-2.5-flash-preview-tts" &&
                                          ttsManager.geminiModel !== "gemini-2.5-pro-preview-tts" &&
                                          ttsManager.geminiModel !== "gemini-2.5-flash" &&
                                          ttsManager.geminiModel !== "gemini-2.0-flash" &&
                                          ttsManager.geminiModel !== "gemini-2.5-flash-native-audio" &&
                                          ttsManager.geminiModel !== ""
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
                            visible: geminiModelPresetDropDown.selectedValue === "custom"
                            topMargin: bookManager.dp(6)
                            hintText: "gemini-2.5-flash-preview-tts"
                            clearButtonVisible: true
                            text: ttsManager.geminiModel
                            onTextChanging: {
                                ttsManager.setGeminiModel(text);
                            }
                        }

                        DropDown {
                            id: geminiStyleDropDown
                            topMargin: bookManager.dp(10)
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
                    // eSpeak-NG offline voice config
                    Container {
                        visible: ttsManager.ttsProvider === "espeak" || ttsManager.ttsMode === "offline"
                        topMargin: bookManager.dp(10)
                        horizontalAlignment: HorizontalAlignment.Fill

                        DropDown {
                            id: espeakRegionDropDown
                            title: qsTr("Giọng vùng miền (Tiếng Việt)") + Retranslate.onLanguageChanged
                            Option { text: qsTr("Miền Bắc") + Retranslate.onLanguageChanged; value: "vi"; selected: ttsManager.espeakViRegion === "vi" }
                            Option { text: qsTr("Miền Nam") + Retranslate.onLanguageChanged; value: "vi-vn-x-south"; selected: ttsManager.espeakViRegion === "vi-vn-x-south" }
                            Option { text: qsTr("Miền Trung") + Retranslate.onLanguageChanged; value: "vi-vn-x-central"; selected: ttsManager.espeakViRegion === "vi-vn-x-central" }
                            onSelectedValueChanged: {
                                if (selectedValue && selectedValue !== ttsManager.espeakViRegion) ttsManager.setEspeakViRegion(selectedValue);
                            }
                        }

                        DropDown {
                            id: espeakVariantDropDown
                            topMargin: bookManager.dp(10)
                            title: qsTr("Chất giọng eSpeak-NG") + Retranslate.onLanguageChanged
                            Option { text: qsTr("Chuẩn (mượt nhất)") + Retranslate.onLanguageChanged; value: "std"; selected: ttsManager.espeakVariant === "std" }
                            Option { text: qsTr("Nữ êm dịu") + Retranslate.onLanguageChanged; value: "f3"; selected: ttsManager.espeakVariant === "f3" }
                            Option { text: qsTr("Nữ sáng") + Retranslate.onLanguageChanged; value: "f4"; selected: ttsManager.espeakVariant === "f4" }
                            Option { text: qsTr("Nam chuẩn") + Retranslate.onLanguageChanged; value: "m1"; selected: ttsManager.espeakVariant === "m1" }
                            Option { text: qsTr("Nam trầm ấm") + Retranslate.onLanguageChanged; value: "m3"; selected: ttsManager.espeakVariant === "m3" }
                            Option { text: qsTr("Klatt rõ chữ") + Retranslate.onLanguageChanged; value: "klatt"; selected: ttsManager.espeakVariant === "klatt" }
                            Option { text: qsTr("Giọng Nguyễn") + Retranslate.onLanguageChanged; value: "Nguyen"; selected: ttsManager.espeakVariant === "Nguyen" }
                            onSelectedValueChanged: {
                                if (selectedValue && selectedValue !== ttsManager.espeakVariant) ttsManager.setEspeakVariant(selectedValue);
                            }
                        }
                    }
                }

                // =========================================================
                // Section 2: NGÔN NGỮ & TỐC ĐỘ
                // =========================================================
                Header {
                    title: qsTr("NGÔN NGỮ & TỐC ĐỘ") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(16)

                    DropDown {
                        id: ttsLanguageDropDown
                        title: qsTr("Ngôn ngữ") + Retranslate.onLanguageChanged

                        Option { text: qsTr("Tự động nhận diện") + Retranslate.onLanguageChanged; value: "auto"; selected: ttsManager.language === "auto" }
                        Option { text: qsTr("Tiếng Việt chuẩn (vi)") + Retranslate.onLanguageChanged; value: "vi"; selected: ttsManager.language === "vi" }
                        Option { text: qsTr("English - Giọng Mỹ (en-US)") + Retranslate.onLanguageChanged; value: "en-US"; selected: ttsManager.language === "en-US" || ttsManager.language === "en" }
                        Option { text: qsTr("English - Giọng Anh (en-GB)") + Retranslate.onLanguageChanged; value: "en-GB"; selected: ttsManager.language === "en-GB" }
                        Option { text: qsTr("English - Giọng Úc (en-AU)") + Retranslate.onLanguageChanged; value: "en-AU"; selected: ttsManager.language === "en-AU" }
                        Option { text: qsTr("English - Giọng Ấn Độ (en-IN)") + Retranslate.onLanguageChanged; value: "en-IN"; selected: ttsManager.language === "en-IN" }
                        Option { text: qsTr("Français - Pháp (fr-FR)") + Retranslate.onLanguageChanged; value: "fr-FR"; selected: ttsManager.language === "fr-FR" || ttsManager.language === "fr" }
                        Option { text: qsTr("Français - Canada (fr-CA)") + Retranslate.onLanguageChanged; value: "fr-CA"; selected: ttsManager.language === "fr-CA" }
                        Option { text: qsTr("Español - Tây Ban Nha (es-ES)") + Retranslate.onLanguageChanged; value: "es-ES"; selected: ttsManager.language === "es-ES" || ttsManager.language === "es" }
                        Option { text: qsTr("Español - Mỹ Latinh (es-US)") + Retranslate.onLanguageChanged; value: "es-US"; selected: ttsManager.language === "es-US" }
                        Option { text: qsTr("Deutsch (de)") + Retranslate.onLanguageChanged; value: "de"; selected: ttsManager.language === "de" }
                        Option { text: qsTr("Русский (ru)") + Retranslate.onLanguageChanged; value: "ru"; selected: ttsManager.language === "ru" }
                        Option { text: qsTr("中文 - Giản thể (zh-CN)") + Retranslate.onLanguageChanged; value: "zh-CN"; selected: ttsManager.language === "zh-CN" }
                        Option { text: qsTr("中文 - Đài Loan (zh-TW)") + Retranslate.onLanguageChanged; value: "zh-TW"; selected: ttsManager.language === "zh-TW" }
                        Option { text: qsTr("中文 - Quảng Đông / HK (zh-HK)") + Retranslate.onLanguageChanged; value: "zh-HK"; selected: ttsManager.language === "zh-HK" }
                        Option { text: qsTr("日本語 (ja)") + Retranslate.onLanguageChanged; value: "ja"; selected: ttsManager.language === "ja" }
                        Option { text: qsTr("한국어 (ko)") + Retranslate.onLanguageChanged; value: "ko"; selected: ttsManager.language === "ko" }

                        onSelectedValueChanged: {
                            if (selectedValue && selectedValue !== ttsManager.language) {
                                ttsManager.setLanguage(selectedValue);
                            }
                        }
                    }

                    DropDown {
                        id: ttsSpeedDropDown
                        topMargin: bookManager.dp(12)
                        title: qsTr("Tốc độ") + Retranslate.onLanguageChanged

                        Option { text: qsTr("Chậm") + Retranslate.onLanguageChanged; description: "0.8x"; value: 0.8; selected: ttsManager.speed == 0.8 }
                        Option { text: qsTr("Chuẩn") + Retranslate.onLanguageChanged; description: "1.0x"; value: 1.0; selected: ttsManager.speed == 1.0 }
                        Option { text: qsTr("Nhanh") + Retranslate.onLanguageChanged; description: "1.25x"; value: 1.25; selected: ttsManager.speed == 1.25 }
                        Option { text: qsTr("Rất nhanh") + Retranslate.onLanguageChanged; description: "1.5x"; value: 1.5; selected: ttsManager.speed == 1.5 }

                        onSelectedValueChanged: {
                            if (selectedValue) {
                                ttsManager.setSpeed(selectedValue);
                            }
                        }
                    }

                    DropDown {
                        id: ttsPitchDropDown
                        topMargin: bookManager.dp(12)
                        title: qsTr("Cao độ") + Retranslate.onLanguageChanged

                        Option { text: qsTr("Trầm") + Retranslate.onLanguageChanged; value: 0.8; selected: ttsManager.pitch == 0.8 }
                        Option { text: qsTr("Chuẩn") + Retranslate.onLanguageChanged; value: 1.0; selected: ttsManager.pitch == 1.0 }
                        Option { text: qsTr("Bổng") + Retranslate.onLanguageChanged; value: 1.2; selected: ttsManager.pitch == 1.2 }
                        Option { text: qsTr("Rất bổng") + Retranslate.onLanguageChanged; value: 1.4; selected: ttsManager.pitch == 1.4 }

                        onSelectedValueChanged: {
                            if (selectedValue) {
                                ttsManager.setPitch(selectedValue);
                            }
                        }
                    }

                    Button {
                        id: testVoiceButton
                        topMargin: bookManager.dp(14)
                        horizontalAlignment: HorizontalAlignment.Fill
                        appearance: ControlAppearance.Primary
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
                }

                // =========================================================
                // Section 3: PHÁT & NGOẠI TUYẾN
                // =========================================================
                Header {
                    title: qsTr("PHÁT & NGOẠI TUYẾN") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(6)

                    DropDown {
                        id: ttsModeDropDown
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

                    Label {
                        visible: ttsManager.ttsMode === "offline"
                        topMargin: bookManager.dp(8)
                        multiline: true
                        text: qsTr("Chế độ ngoại tuyến: ưu tiên phát âm thanh đã tải, câu chưa có sẽ tự đọc bằng eSpeak-NG tích hợp sẵn.") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create(bookManager.uiMuted)
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

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(12)
                    bottomPadding: bookManager.dp(16)

                    Button {
                        visible: ttsManager.ttsProvider !== "espeak"
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
                        topMargin: bookManager.dp(10)
                        text: qsTr("Xóa bộ nhớ đệm") + Retranslate.onLanguageChanged
                        horizontalAlignment: HorizontalAlignment.Fill
                        onClicked: {
                            ttsManager.clearCache();
                            clearCacheToast.show();
                        }
                    }
                }

                // =========================================================
                // Section 4: HẸN GIỜ TẮT
                // =========================================================
                Header {
                    title: qsTr("HẸN GIỜ TẮT") + Retranslate.onLanguageChanged
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    leftPadding: bookManager.dp(16)
                    rightPadding: bookManager.dp(16)
                    topPadding: bookManager.dp(10)
                    bottomPadding: bookManager.dp(16)

                    DropDown {
                        id: sleepTimerDropDown
                        title: qsTr("Tự động dừng sau") + Retranslate.onLanguageChanged

                        Option { text: qsTr("Tắt (Không hẹn giờ)") + Retranslate.onLanguageChanged; value: "0"; selected: !ttsManager.isSleepTimerActive }
                        Option { text: qsTr("15 phút") + Retranslate.onLanguageChanged; value: "15"; selected: ttsManager.sleepTimerMinutes === 15 && !ttsManager.sleepAtChapterEnd }
                        Option { text: qsTr("30 phút") + Retranslate.onLanguageChanged; value: "30"; selected: ttsManager.sleepTimerMinutes === 30 && !ttsManager.sleepAtChapterEnd }
                        Option { text: qsTr("45 phút") + Retranslate.onLanguageChanged; value: "45"; selected: ttsManager.sleepTimerMinutes === 45 && !ttsManager.sleepAtChapterEnd }
                        Option { text: qsTr("60 phút") + Retranslate.onLanguageChanged; value: "60"; selected: ttsManager.sleepTimerMinutes === 60 && !ttsManager.sleepAtChapterEnd }
                        Option { text: qsTr("Khi đọc xong chương này") + Retranslate.onLanguageChanged; value: "chapter"; selected: ttsManager.sleepAtChapterEnd }

                        onCreationCompleted: {
                            ttsSettingsPage.isInitializingSleepDropdown = false;
                        }

                        onSelectedValueChanged: {
                            if (ttsSettingsPage.isInitializingSleepDropdown) return;
                            if (selectedValue === "0") {
                                ttsManager.cancelSleepTimer();
                            } else if (selectedValue === "chapter") {
                                ttsManager.setSleepAtChapterEnd(true);
                            } else {
                                var mins = parseInt(selectedValue);
                                if (mins > 0) {
                                    ttsManager.startSleepTimer(mins);
                                }
                            }
                        }
                    }

                    Label {
                        visible: ttsManager.isSleepTimerActive
                        topMargin: bookManager.dp(8)
                        multiline: true
                        text: ttsManager.sleepAtChapterEnd ?
                              (qsTr("Trạng thái: Sẽ tự động dừng khi đọc hết chương.") + Retranslate.onLanguageChanged) :
                              (qsTr("Thời gian còn lại: ") + ttsManager.sleepTimerDisplayStr + Retranslate.onLanguageChanged)
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create(bookManager.uiAccent)
                        textStyle.fontWeight: FontWeight.W500
                    }
                }
            }
        }
    }

    attachedObjects: [
        SystemToast {
            id: clearCacheToast
            body: qsTr("Đã dọn dẹp xong bộ nhớ đệm âm thanh.") + Retranslate.onLanguageChanged
        }
    ]
}
