import bb.cascades 1.3
import bb.system 1.2

Page {
    id: readerPage

    // =========================================================================
    // NATIVE BLACKBERRY 10 TOP SWIPE-DOWN APPLICATION MENU (Inside Reader)
    // =========================================================================
    Menu.definition: MenuDefinition {
        settingsAction: SettingsActionItem {
            title: qsTr("Cài Đặt") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_settings.png"
            onTriggered: {
                settingsSheet.open();
            }
        }
        actions: [
            ActionItem {
                title: qsTr("Tủ sách") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_book.png"
                onTriggered: {
                    bookManager.saveCurrentProgress();
                    bookManager.setIsReaderActive(false);
                    if (ttsManager.active) {
                        ttsManager.stop();
                    }
                    if (navPane) {
                        navPane.pop();
                    }
                    app.switchToTab("shelf");
                }
            },
            ActionItem {
                title: qsTr("Mục lục") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_toc.png"
                onTriggered: {
                    tocSheet.open();
                }
            },
            ActionItem {
                title: qsTr("Ghi chú") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_highlight.png"
                onTriggered: {
                    highlightSheet.open();
                }
            },
            ActionItem {
                title: qsTr("Thông tin") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_info.png"
                onTriggered: {
                    bookDetailSheet.open();
                }
            },
            ActionItem {
                title: qsTr("Thoát") + Retranslate.onLanguageChanged
                imageSource: "asset:///icons/ic_exit.png"
                onTriggered: {
                    app.promptExit();
                }
            }
        ]
    }

    property variant navPane: null
    property bool fullscreenMode: true
    property bool isRendering: true
    property string selectedText: ""

    onFullscreenModeChanged: {
        webView.evaluateJavaScript("window.isChromeVisible = " + (!fullscreenMode) + ";");
    }

    function dismissOpeningBook() {
        cleanup();
        bookManager.saveCurrentProgress();
        bookManager.setIsReaderActive(false);
        readerPage.isRendering = false;
        bookManager.setIsLoadingBook(false);
        bookManager.stopWatchdog();
        if (ttsManager.active) {
            ttsManager.stop();
        }
        if (navPane) {
            navPane.pop();
        }
    }

    actionBarVisibility: fullscreenMode ? ChromeVisibility.Hidden : ChromeVisibility.Visible

    function toggleFullscreen() {
        fullscreenMode = !fullscreenMode;
    }

    titleBar: TitleBar {
        kind: TitleBarKind.FreeForm
        scrollBehavior: TitleBarScrollBehavior.Sticky
        visibility: readerPage.fullscreenMode ? ChromeVisibility.Hidden : ChromeVisibility.Visible

        kindProperties: FreeFormTitleBarKindProperties {
            Container {
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                minHeight: 88
                preferredHeight: 96
                leftPadding: 0
                rightPadding: 0
                topPadding: 0
                bottomPadding: 0

                // 1. Back Button to exit reader (expanded, generous touch target)
                Container {
                    id: backButtonContainer
                    preferredWidth: 112
                    minWidth: 96
                    verticalAlignment: VerticalAlignment.Fill
                    layout: DockLayout {}

                    ImageButton {
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center
                        defaultImageSource: "asset:///icons/ic_previous.png"
                        pressedImageSource: "asset:///icons/ic_previous.png"
                        preferredWidth: 104
                        preferredHeight: 88
                        onClicked: {
                            dismissOpeningBook();
                        }
                    }

                    gestureHandlers: [
                        TapHandler {
                            onTapped: {
                                dismissOpeningBook();
                            }
                        }
                    ]
                }

                // 2. Vertical Divider separating Back button from Book Title
                Container {
                    verticalAlignment: VerticalAlignment.Center
                    preferredWidth: 2
                    preferredHeight: 52
                    background: Color.create(1.0, 1.0, 1.0, 0.22)
                }

                // 3. Clickable Book Title area navigating to Book Information
                // Centered horizontally across the entire remaining area from divider to right edge
                Container {
                    verticalAlignment: VerticalAlignment.Fill
                    horizontalAlignment: HorizontalAlignment.Fill
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1.0
                    }
                    leftPadding: 16
                    rightPadding: 16
                    layout: DockLayout {}

                    gestureHandlers: [
                        TapHandler {
                            onTapped: {
                                bookDetailSheet.open();
                            }
                        }
                    ]

                    Label {
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center
                        text: bookManager.currentBookTitle
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.textAlign: TextAlign.Center
                        textStyle.color: Color.White
                        autoSize.maxLineCount: 1
                    }
                }
            }
        }
    }

    Container {
        id: readerRootContainer
        layout: StackLayout {
            orientation: LayoutOrientation.TopToBottom
        }
        background: bookManager.theme == "dark" ? Color.create("#000000") : 
                    (bookManager.theme == "sepia" ? Color.create("#F4ECD8") : Color.create("#FFFFFF"))

        focusPolicy: FocusPolicy.KeyAndTouch
        inputRoute.primaryKeyTarget: true

        property real kbdTouchStartY: 0
        property real kbdTouchStartX: 0
        property real kbdTouchLastY: 0
        property real kbdTouchLastX: 0
        property real kbdTouchAccumY: 0
        property real kbdTouchAccumX: 0
        property real lastPageTurnTime: 0
        property bool kbdPageTurnedInGesture: false

        property real lastKbdTouchTime: 0
        property real lastKbdTouchX: 0
        property real lastKbdTouchY: 0
        property int lastKbdTouchType: -1
        property real kbdVelocityY: 0
        property real kbdLastMoveTime: 0

        property real pendingScrollDelta: 0
        property real lastTrackpadTime: 0
        property real lastTrackpadDy: 0
        property real lastTrackpadDx: 0
        property bool lastTrackpadIsMove: false
        property real tpAccumY: 0
        property real tpAccumX: 0
        property bool tpPageTurned: false

        attachedObjects: [
            Timer {
                id: inertiaTimer
                interval: 16
                repeat: true
                onTriggered: {
                    if (Math.abs(readerRootContainer.kbdVelocityY) < 0.04) {
                        inertiaTimer.stop();
                        readerRootContainer.kbdVelocityY = 0;
                        return;
                    }
                    var step = readerRootContainer.kbdVelocityY * 16 * 0.35;
                    readerPage.scrollByDelta(step);
                    readerRootContainer.kbdVelocityY *= 0.86;
                }
            },
            Timer {
                id: initialFocusTimer
                interval: 200
                repeat: false
                onTriggered: {
                    readerRootContainer.requestFocus();
                }
            }
        ]

        eventHandlers: [
            TouchKeyboardHandler {
                onTouch: { readerPage.handleKbdTouch(event); }
            },
            TrackpadHandler {
                consumeTrackpadEvents: ConsumeTrackpadEvent.Any
                onTrackpad: { readerPage.handleTrackpad(event); }
            },
            NavigationHandler {
                consumeNavigationEvents: ConsumeNavigationEvent.Any
                onNavigation: { readerPage.handleNavigation(event); }
            }
        ]
        keyListeners: [
            KeyListener {
                onKeyPressed: { readerPage.handleHardwareKey(event); }
            }
        ]

        // --- Reading Viewport Area (Takes all available space above footer) ---
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            layoutProperties: StackLayoutProperties {
                spaceQuota: 1.0
            }
            layout: DockLayout {}

            // View 1: EPUB / TXT / FB2 / PDF Reflow (Direct viewport-constrained WebView)
            Container {
                id: webContainer
                visible: !bookManager.isPdf || bookManager.isPdfReflow
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layout: DockLayout {}

                animations: [
                    FadeTransition {
                        id: webFadeInAnim
                        toOpacity: 1.0
                        duration: 180
                        easingCurve: StockCurve.CubicOut
                    }
                ]

                attachedObjects: [
                    LayoutUpdateHandler {
                        id: webLayoutHandler
                        onLayoutFrameChanged: {
                            if (layoutFrame.height > 0) {
                                webView.preferredHeight = layoutFrame.height;
                                webView.minHeight = layoutFrame.height;
                            }
                            if (layoutFrame.width > 0) {
                                webView.preferredWidth = layoutFrame.width;
                                webView.minWidth = layoutFrame.width;
                            }
                        }
                    }
                ]

                WebView {
                    id: webView
                    html: bookManager.currentContent
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    preferredWidth: bookManager.screenWidth > 0 ? bookManager.screenWidth : 720
                    preferredHeight: 720
                    minWidth: 320
                    minHeight: 320
                    settings.defaultFontSize: bookManager.fontSize
                    settings.zoomToFitEnabled: false
                    focusPolicy: FocusPolicy.Touch

                    onFocusedChanged: {
                        if (focused) {
                            readerRootContainer.requestFocus();
                        }
                    }

                    onTouch: {
                        readerRootContainer.requestFocus();
                    }

                    onLoadingChanged: {
                        if (loadRequest.status === WebLoadStatus.Started) {
                            webContainer.opacity = 0.55;
                        } else {
                            readerPage.isRendering = false;
                            bookManager.setIsLoadingBook(false);
                            bookManager.stopWatchdog();
                            webFadeInAnim.play();
                            readerRootContainer.requestFocus();
                            webView.evaluateJavaScript("if (typeof updatePageInfo === 'function') updatePageInfo();");
                            var pendingAnc = bookManager.consumePendingScrollAnchor();
                            if (pendingAnc && pendingAnc !== "") {
                                webView.evaluateJavaScript("if (typeof scrollToAnchor === 'function') scrollToAnchor('" + pendingAnc + "');");
                            } else {
                                webView.evaluateJavaScript("if (typeof readingMode !== 'undefined' && readingMode === 'scroll') { if (typeof savedTargetScroll !== 'undefined' && savedTargetScroll > 0) { var vp = document.getElementById('reader-viewport'); if (vp) vp.scrollTop = savedTargetScroll; } else if (typeof savedTargetPage !== 'undefined' && savedTargetPage > 1 && typeof goToPage === 'function') { goToPage(savedTargetPage); } } else { if (typeof savedTargetPage !== 'undefined' && savedTargetPage > 1 && typeof goToPage === 'function') { goToPage(savedTargetPage); } else if (typeof savedTargetScroll !== 'undefined' && savedTargetScroll > 0) { var vp = document.getElementById('reader-viewport'); var h = window.innerHeight || (vp ? vp.clientHeight : 0) || 720; var p = Math.floor(savedTargetScroll / h) + 1; if (typeof goToPage === 'function') goToPage(p); } }");
                            }
                            if (ttsManager.active && ttsManager.currentSentenceText !== "") {
                                webView.evaluateJavaScript("if (typeof highlightTtsSentence === 'function') highlightTtsSentence(" + JSON.stringify(ttsManager.currentSentenceText) + ");");
                            }
                        }
                    }

                    onMessageReceived: {
                        var str = "" + message.data;
                        if (str === "nextChapter") {
                            bookManager.nextChapter();
                        } else if (str === "prevChapter") {
                            bookManager.prevChapter();
                        } else if (str === "showChrome") {
                            readerPage.fullscreenMode = false;
                            readerPage.selectedText = "";
                            webView.evaluateJavaScript("if (window.getSelection && window.getSelection().removeAllRanges) window.getSelection().removeAllRanges();");
                        } else if (str === "hideChrome") {
                            readerPage.fullscreenMode = true;
                            readerPage.selectedText = "";
                            readerRootContainer.requestFocus();
                        } else if (str === "toggleChrome") {
                            readerPage.toggleFullscreen();
                            readerPage.selectedText = "";
                            readerRootContainer.requestFocus();
                            webView.evaluateJavaScript("if (window.getSelection && window.getSelection().removeAllRanges) window.getSelection().removeAllRanges(); var b = document.getElementById('berry-selection-bubble'); if (b) b.style.display = 'none';");
                        } else if (str.indexOf("pageInfo:") === 0) {
                            var parts = str.split(":");
                            if (parts.length >= 3) {
                                var cur = parseInt(parts[1]);
                                var tot = parseInt(parts[2]);
                                var scr = parts.length >= 4 ? parseInt(parts[3]) : 0;
                                bookManager.setPagePosition(cur, tot, scr);
                            }
                        } else if (str.indexOf("textSelected:") === 0) {
                            var rawText = str.substr(13);
                            if (rawText.length > 0) {
                                readerPage.selectedText = decodeURIComponent(rawText);
                            } else {
                                readerPage.selectedText = "";
                            }
                        } else if (str.indexOf("copyText:") === 0) {
                            var rawText = str.substr(9);
                            if (rawText.length > 0) {
                                var txt = decodeURIComponent(rawText);
                                bookManager.copyText(txt);
                            }
                        } else if (str.indexOf("speakText:") === 0) {
                            var rawText = str.substr(10);
                            if (rawText.length > 0) {
                                var txt = decodeURIComponent(rawText);
                                ttsManager.startWithText(txt, "", false);
                            }
                        } else if (str.indexOf("lookupWord:") === 0) {
                            var rawText = str.substr(11);
                            if (rawText.length > 0) {
                                var txt = decodeURIComponent(rawText);
                                dictSheet.openWithWord(txt);
                            }
                        } else if (str.indexOf("translateSentence:") === 0) {
                            var rawText = str.substr(18);
                            if (rawText.length > 0) {
                                var txt = decodeURIComponent(rawText);
                                dictSheet.openWithSentence(txt);
                            }
                        } else if (str.indexOf("searchKeyword:") === 0) {
                            var rawText = str.substr(14);
                            if (rawText.length > 0) {
                                var txt = decodeURIComponent(rawText);
                                searchSheet.openWithQuery(txt);
                            }
                        } else if (str.indexOf("searchStats:") === 0) {
                            var sParts = str.split(":");
                            if (sParts.length >= 3) {
                                searchSheet.inPageCurrent = parseInt(sParts[1]);
                                searchSheet.inPageTotal = parseInt(sParts[2]);
                            }
                        } else if (str.indexOf("requestHighlight:") === 0) {
                            var rawHl = str.substr(17);
                            if (rawHl.length > 0) {
                                var txtHl = decodeURIComponent(rawHl);
                                var newHlId = bookManager.addHighlight(txtHl, "", "yellow");
                                webView.evaluateJavaScript("if (typeof highlightTextInContainer === 'function') highlightTextInContainer(document.getElementById('reader-viewport') || document.body, " + JSON.stringify(txtHl) + ", " + JSON.stringify(newHlId) + ", 'yellow', '');");
                                highlightNoteSheet.openForHighlight(newHlId, txtHl, "", "yellow");
                            }
                        } else if (str.indexOf("clickedHighlight:") === 0) {
                            var clickHlId = str.substr(17);
                            var hlObj = bookManager.getHighlight(clickHlId);
                            if (hlObj && hlObj.id) {
                                highlightNoteSheet.openForHighlight(hlObj.id, hlObj.text, hlObj.note, hlObj.color);
                            }
                        }
                    }
                }
            }

            // View 2: PDF & DJVU (NSR Reader Engine / Poppler vector render - Fit to Width)
            ScrollView {
                id: pdfScrollView
                visible: bookManager.isPdf && !bookManager.isPdfReflow
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                scrollViewProperties.scrollMode: ScrollMode.Both
                scrollViewProperties.pinchToZoomEnabled: true
                scrollViewProperties.minContentScale: 1.0
                scrollViewProperties.maxContentScale: 5.0
                focusPolicy: FocusPolicy.KeyAndTouch
                scrollRole: ScrollRole.Main

                property real pdfTouchStartX: 0
                property real pdfTouchStartY: 0
                property real pdfTouchStartTime: 0
                property real lastPdfTapTime: 0
                property bool scrollPdfToBottomOnLoad: false

                function getPdfContentHeight() {
                    if (pdfLayoutHandler.layoutFrame.height > 0) {
                        return pdfLayoutHandler.layoutFrame.height;
                    }
                    if (pdfImageView.preferredHeight > 0) {
                        return pdfImageView.preferredHeight;
                    }
                    return pdfScrollView.viewableArea.height;
                }

                onTouch: {
                    if (event.isDown()) {
                        pdfTouchStartX = event.windowX;
                        pdfTouchStartY = event.windowY;
                        pdfTouchStartTime = new Date().getTime();
                    } else if (event.isUp()) {
                        var dx = event.windowX - pdfTouchStartX;
                        var dy = event.windowY - pdfTouchStartY;
                        var dt = new Date().getTime() - pdfTouchStartTime;

                        if (Math.abs(dx) < 30 && Math.abs(dy) < 30 && dt < 450) {
                            var now = new Date().getTime();
                            if (now - lastPdfTapTime < 350) {
                                // Double tap: reset zoom if zoomed in, or zoom in 2x if normal
                                lastPdfTapTime = 0;
                                if (pdfScrollView.contentScale > 1.05) {
                                    pdfScrollView.resetViewableArea(ScrollAnimation.Smooth);
                                } else {
                                    var cx = pdfScrollView.viewableArea.x + event.localX;
                                    var cy = pdfScrollView.viewableArea.y + event.localY;
                                    pdfScrollView.zoomToPoint(cx, cy, 2.0, ScrollAnimation.Smooth);
                                }
                            } else {
                                lastPdfTapTime = now;
                                // Only toggle fullscreen on single tap when normal scale
                                if (pdfScrollView.contentScale <= 1.05) {
                                    readerPage.toggleFullscreen();
                                }
                            }
                        } else if (pdfScrollView.contentScale <= 1.05) {
                            // Cuộn liền trang kiểu truyền thống (chỉ khi không zoom)
                            var curY = pdfScrollView.viewableArea.y;
                            var maxScrollY = Math.max(0, getPdfContentHeight() - pdfScrollView.viewableArea.height);
                            if (dy < -60 && curY >= maxScrollY - 10) {
                                scrollPdfToBottomOnLoad = false;
                                bookManager.nextChapter();
                            } else if (dy > 60 && curY <= 10) {
                                scrollPdfToBottomOnLoad = true;
                                bookManager.prevChapter();
                            }
                        }
                    }
                }

                Container {
                    id: pdfContainer
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Top
                    topPadding: bookManager.marginTop
                    bottomPadding: bookManager.marginBottom
                    leftPadding: bookManager.marginLeft
                    rightPadding: bookManager.marginRight
                    layout: DockLayout {}

                    ImageView {
                        id: pdfImageView
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Top
                        image: bookManager.currentPdfImage
                        scalingMethod: ScalingMethod.AspectFit

                        attachedObjects: [
                            LayoutUpdateHandler {
                                id: pdfLayoutHandler
                                onLayoutFrameChanged: {
                                    if (layoutFrame.height > 0) {
                                        pdfImageView.preferredHeight = layoutFrame.height;
                                    }
                                    if (layoutFrame.width > 0) {
                                        pdfImageView.preferredWidth = layoutFrame.width;
                                    }
                                }
                            }
                        ]

                        onImageChanged: {
                            readerPage.isRendering = false;
                            bookManager.stopWatchdog();
                            pdfScrollView.resetViewableArea(ScrollAnimation.None);
                            if (pdfScrollView.scrollPdfToBottomOnLoad) {
                                var maxScroll = Math.max(0, pdfScrollView.getPdfContentHeight() - pdfScrollView.viewableArea.height);
                                pdfScrollView.scrollToPoint(0, maxScroll, ScrollAnimation.None);
                                pdfScrollView.scrollPdfToBottomOnLoad = false;
                            } else {
                                pdfScrollView.scrollToPoint(0, 0, ScrollAnimation.None);
                            }
                            pdfImageView.opacity = 0.7;
                            pdfFadeInQuickAnim.play();
                        }

                        animations: [
                            FadeTransition {
                                id: pdfFadeInQuickAnim
                                toOpacity: 1.0
                                duration: 160
                                easingCurve: StockCurve.CubicOut
                            }
                        ]
                    }
                }
            }

            // Page rendering loading overlay
            Container {
                id: readerRenderOverlay
                visible: readerPage.isRendering
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                background: bookManager.theme == "dark" ? Color.create("#D0000000") : Color.create("#D0FFFFFF")
                layout: DockLayout {}

                Container {
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center

                    ActivityIndicator {
                        horizontalAlignment: HorizontalAlignment.Center
                        visible: readerRenderOverlay.visible
                        running: readerRenderOverlay.visible
                        preferredWidth: 60
                        preferredHeight: 60
                        onVisibleChanged: { if (visible) start(); else stop(); }
                    }

                    Label {
                        topMargin: 14
                        horizontalAlignment: HorizontalAlignment.Center
                        text: qsTr("Đang hiển thị nội dung...") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#AAAAAA") : Color.create("#555555")
                    }

                    Label {
                        topMargin: 8
                        horizontalAlignment: HorizontalAlignment.Center
                        text: qsTr("Chạm màn hình để hủy") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.XSmallText
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                    }
                }
            }

            // =========================================================================
            // TAP TO DISMISS CHROME OVERLAY
            // When titleBar & bottom bar are visible (!fullscreenMode), ANY tap on the
            // reading viewport immediately dismisses chrome back to immersive reading!
            // =========================================================================
            Container {
                id: chromeDismissOverlay
                visible: !readerPage.fullscreenMode && !ttsManager.active
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                background: Color.create(0.0, 0.0, 0.0, 0.005)
                touchPropagationMode: TouchPropagationMode.Full
                layout: DockLayout {}

                gestureHandlers: [
                    TapHandler {
                        onTapped: {
                            readerPage.fullscreenMode = true;
                            webView.evaluateJavaScript("window.isChromeVisible = false; if (typeof clearInPageSearch === 'function') clearInPageSearch(); if (window.getSelection && window.getSelection().removeAllRanges) window.getSelection().removeAllRanges(); var b = document.getElementById('berry-selection-bubble'); if (b) b.style.display = 'none';");
                        }
                    }
                ]
            }
        }

        // --- TTS Voice Reader Floating Controller Panel ---
        Container {
            id: ttsControlPanel
            visible: ttsManager.active
            horizontalAlignment: HorizontalAlignment.Fill
            background: bookManager.appTheme === "dark" ? Color.create("#1C1C1C") : Color.create("#F8F9FA")
            topPadding: 8
            bottomPadding: 8
            leftPadding: 12
            rightPadding: 12

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            // Bên trái: Đoạn TTS đang chạy và tên voice + model phía dưới
            Container {
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 1.0
                }
                verticalAlignment: VerticalAlignment.Center
                layout: StackLayout {
                    orientation: LayoutOrientation.TopToBottom
                }

                Label {
                    text: ttsManager.loading ? 
                          (qsTr("Đang tải dữ liệu...") + Retranslate.onLanguageChanged) :
                          (ttsManager.currentSentenceText !== "" ? ("\"" + ttsManager.currentSentenceText + "\"") : qsTr("BerryLeaf TTS"))
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: ttsManager.loading ? Color.create("#FFA500") : 
                                    (bookManager.appTheme === "dark" ? Color.White : Color.Black)
                    textStyle.fontStyle: FontStyle.Italic
                    autoSize.maxLineCount: 1
                }

                Label {
                    topMargin: 2
                    text: (ttsManager.ttsProvider === "gemini" ? 
                           (ttsManager.geminiVoice + " • " + ttsManager.geminiModel) : 
                           (ttsManager.voiceDisplayName !== "" ? ttsManager.voiceDisplayName : (bookManager.language === "en" ? "Google Translate" : "Google Dịch"))) + 
                          " (" + (ttsManager.currentSentenceIndex + 1) + "/" + ttsManager.totalSentences + ")"
                    textStyle.base: SystemDefaults.TextStyles.XSmallText
                    textStyle.color: bookManager.appTheme === "dark" ? Color.create("#888888") : Color.create("#777777")
                    autoSize.maxLineCount: 1
                }
            }

            // Bên phải: Nút back, tiếp, pause, bánh răng, thoát
            Container {
                verticalAlignment: VerticalAlignment.Center
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }

                // Nút lùi (Back)
                ImageButton {
                    verticalAlignment: VerticalAlignment.Center
                    defaultImageSource: "asset:///icons/ic_previous.png"
                    pressedImageSource: "asset:///icons/ic_previous.png"
                    preferredWidth: 44
                    preferredHeight: 44
                    onClicked: {
                        ttsManager.prevSentence();
                    }
                }

                // Nút tiếp (Next)
                ImageButton {
                    verticalAlignment: VerticalAlignment.Center
                    defaultImageSource: "asset:///icons/ic_next.png"
                    pressedImageSource: "asset:///icons/ic_next.png"
                    preferredWidth: 44
                    preferredHeight: 44
                    leftMargin: 4
                    onClicked: {
                        ttsManager.nextSentence();
                    }
                }

                // Nút Pause / Play
                Button {
                    verticalAlignment: VerticalAlignment.Center
                    text: ttsManager.playing ? qsTr("Dừng") : qsTr("Đọc")
                    preferredWidth: 64
                    preferredHeight: 44
                    leftMargin: 6
                    onClicked: {
                        ttsManager.togglePlayPause();
                    }
                }

                // Nút Bánh răng Cài đặt (Settings)
                ImageButton {
                    verticalAlignment: VerticalAlignment.Center
                    defaultImageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_settings.png" : "asset:///icons/ic_settings_dark.png"
                    pressedImageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_settings.png" : "asset:///icons/ic_settings_dark.png"
                    preferredWidth: 44
                    preferredHeight: 44
                    leftMargin: 6
                    onClicked: {
                        settingsSheet.openToTts();
                    }
                }

                // Nút Thoát
                Button {
                    verticalAlignment: VerticalAlignment.Center
                    text: qsTr("Đóng")
                    preferredWidth: 64
                    preferredHeight: 44
                    leftMargin: 6
                    onClicked: {
                        ttsManager.stop();
                    }
                }
            }
        }

        // In-Page Search Floating Navigation Bar
        Container {
            id: inPageSearchControlPanel
            visible: searchSheet.inPageTotal > 0 && !ttsManager.active && !readerPage.fullscreenMode
            horizontalAlignment: HorizontalAlignment.Fill
            background: bookManager.appTheme === "dark" ? Color.create("#111111") : Color.create("#FFFFFF")
            topPadding: 8
            bottomPadding: 8
            leftPadding: 12
            rightPadding: 12

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            ImageView {
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: 32
                preferredHeight: 32
                imageSource: "asset:///icons/ic_search.png"
            }

            Label {
                leftMargin: 8
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                text: searchSheet.activeSearchTerm + " (" + searchSheet.inPageCurrent + "/" + searchSheet.inPageTotal + ")"
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.fontWeight: FontWeight.Bold
                textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.Black
                autoSize.maxLineCount: 1
            }

            Button {
                verticalAlignment: VerticalAlignment.Center
                text: qsTr("Trước")
                preferredWidth: 70
                preferredHeight: 44
                leftMargin: 6
                onClicked: {
                    searchSheet.inPagePrev();
                }
            }

            Button {
                verticalAlignment: VerticalAlignment.Center
                text: qsTr("Tiếp")
                preferredWidth: 64
                preferredHeight: 44
                leftMargin: 6
                onClicked: {
                    searchSheet.inPageNext();
                }
            }

            Button {
                verticalAlignment: VerticalAlignment.Center
                text: qsTr("Đóng")
                preferredWidth: 64
                preferredHeight: 44
                leftMargin: 6
                onClicked: {
                    searchSheet.inPageClear();
                    searchSheet.inPageTotal = 0;
                    searchSheet.inPageCurrent = 0;
                }
            }
        }

        // Dedicated reading progress bar across the screen (fills up according to readingProgress)
        Container {
            visible: bookManager.showFooterProgress
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: 3
            minHeight: 3
            maxHeight: 3
            background: bookManager.theme == "dark" ? Color.create("#1A1A1A") : 
                        (bookManager.theme == "sepia" ? Color.create("#E2D7C2") : Color.create("#E2E8F0"))

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            Container {
                verticalAlignment: VerticalAlignment.Fill
                background: bookManager.readingProgress >= 100 ? Color.create("#059669") : Color.create("#10B981")
                layoutProperties: StackLayoutProperties {
                    spaceQuota: Math.max(0.001, bookManager.readingProgress)
                }
            }
            Container {
                verticalAlignment: VerticalAlignment.Fill
                background: Color.Transparent
                layoutProperties: StackLayoutProperties {
                    spaceQuota: Math.max(0.001, 100 - bookManager.readingProgress)
                }
            }
        }

        // Dedicated bottom progress & chapter status bar (NEVER overlaps reading content)
        Container {
            id: bottomStatusBar
            visible: (bookManager.showFooterChapter || bookManager.showFooterProgress || bookManager.showFooterBattery || bookManager.showFooterClock)
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: 38
            minHeight: 36
            maxHeight: 44
            background: bookManager.theme == "dark" ? Color.create("#000000") : 
                        (bookManager.theme == "sepia" ? Color.create("#F4ECD8") : Color.create("#FFFFFF"))
            leftPadding: bookManager.marginLeft
            rightPadding: bookManager.marginRight

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            Label {
                visible: bookManager.showFooterChapter
                verticalAlignment: VerticalAlignment.Center
                text: bookManager.currentChapterTitle
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: bookManager.theme == "dark" ? Color.create("#AAAAAA") : Color.create("#403020")
                autoSize.maxLineCount: 1
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 1.0
                }
            }

            Label {
                visible: bookManager.showFooterProgress
                verticalAlignment: VerticalAlignment.Center
                text: bookManager.isPdf ? 
                      ((qsTr("Trang ") + Retranslate.onLanguageChanged) + bookManager.currentPageInChapter + " / " + bookManager.totalPagesInChapter + " • " + bookManager.readingProgress + "%") :
                      (bookManager.totalPagesInChapter <= 1 && bookManager.currentChapterIndex === 0 && (bookManager.currentChapterTitle === qsTr("Bìa sách") || bookManager.currentChapterTitle === "Bìa sách") ?
                       ((qsTr("Bìa sách") + Retranslate.onLanguageChanged) + " • " + bookManager.readingProgress + "%") :
                       ((qsTr("Trang ") + Retranslate.onLanguageChanged) + bookManager.currentPageInChapter + " / " + bookManager.totalPagesInChapter + " • " + bookManager.readingProgress + "%"))
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: bookManager.readingProgress >= 100 ? Color.create("#10B981") : (bookManager.theme == "dark" ? Color.White : (bookManager.theme == "sepia" ? Color.create("#403020") : Color.Black))
            }

            Label {
                visible: bookManager.showFooterBattery
                leftMargin: 12
                verticalAlignment: VerticalAlignment.Center
                text: (qsTr("PIN: ") + Retranslate.onLanguageChanged) + bookManager.batteryLevel + "%"
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: bookManager.theme == "dark" ? Color.create("#888888") : Color.create("#665544")
            }

            Label {
                visible: bookManager.showFooterClock
                leftMargin: 12
                verticalAlignment: VerticalAlignment.Center
                text: bookManager.currentTime
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: bookManager.theme == "dark" ? Color.create("#888888") : Color.create("#665544")
            }
        }
    }

    function applyStyleLive() {
        if (bookManager.isPdf) return;
        var bg = bookManager.theme === "dark" ? "#000000" : (bookManager.theme === "sepia" ? "#F4ECD8" : "#FFFFFF");
        var fg = bookManager.theme === "dark" ? "#DDDDDD" : (bookManager.theme === "sepia" ? "#3E2D1D" : "#111111");
        var fs = bookManager.fontSize;
        var fontCss = "";
        if (bookManager.fontFamily === "serif") {
            fontCss = "font-family: Georgia, 'Times New Roman', serif !important; ";
        } else if (bookManager.fontFamily === "times") {
            fontCss = "font-family: 'Times New Roman', Times, serif !important; ";
        } else if (bookManager.fontFamily === "garamond") {
            fontCss = "font-family: Garamond, 'EB Garamond', serif !important; ";
        } else if (bookManager.fontFamily === "monospace") {
            fontCss = "font-family: 'Courier New', Courier, monospace !important; ";
        } else if (bookManager.fontFamily === "original") {
            fontCss = "";
        } else {
            fontCss = "font-family: 'Slate Pro', 'Helvetica Neue', Helvetica, Arial, sans-serif !important; ";
        }
        var extra = "";
        if (bookManager.theme === "dark") {
            extra = ".banner, div[class*='box'], div[class*='frame'], div[class*='card'], blockquote { background-color: #1A1A1A !important; border: 1px solid #444 !important; border-radius: 10px; color: #DDD !important; } " +
                    ".banner *, div[class*='box'] * { color: inherit; } " +
                    "font[color='#000000'], font[color='black'], font[color='#111111'] { color: inherit !important; } " +
                    "hr, .divider { border-color: #444 !important; background-color: #444 !important; } ";
        } else if (bookManager.theme === "sepia") {
            extra = ".banner, div[class*='box'], div[class*='frame'], div[class*='card'], blockquote { background-color: #EADFC8 !important; border: 1px solid #C0B098 !important; border-radius: 10px; color: #3E2D1D !important; } " +
                    ".banner *, div[class*='box'] * { color: inherit; } " +
                    "font[color='#000000'], font[color='black'] { color: inherit !important; } ";
        }
        var pTop = bookManager.marginTop;
        var pBottom = bookManager.marginBottom;
        var pLeft = bookManager.marginLeft;
        var pRight = bookManager.marginRight;
        var js = "(function() { " +
                 "  var styleEl = document.getElementById('dynamic-theme-style'); " +
                 "  if (!styleEl) { " +
                 "    styleEl = document.createElement('style'); " +
                 "    styleEl.id = 'dynamic-theme-style'; " +
                 "    document.head.appendChild(styleEl); " +
                 "  } " +
                 "  styleEl.innerHTML = 'body { background-color: " + bg + " !important; color: " + fg + " !important; font-size: " + fs + "px; " + fontCss + "} " +
                 "p:not([class]) { text-indent: 1.5em; margin: 0.5em 0; } ' + " +
                 JSON.stringify(extra) + "; " +
                 "  var vp = document.getElementById('reader-viewport'); " +
                 "  if (vp) { " +
                 "    vp.style.padding = '" + pTop + "px " + pRight + "px " + pBottom + "px " + pLeft + "px'; " +
                 "    var w = window.innerWidth || document.documentElement.clientWidth || (vp ? vp.clientWidth : 0) || 720; " +
                 "    var gap = " + (pLeft + pRight) + "; " +
                 "    var colW = Math.max(100, w - gap); " +
                 "    vp.style.webkitColumnWidth = colW + 'px'; " +
                 "    vp.style.columnWidth = colW + 'px'; " +
                 "    vp.style.webkitColumnGap = gap + 'px'; " +
                 "    vp.style.columnGap = gap + 'px'; " +
                 "  } " +
                 "  if (typeof updatePageInfo === 'function') updatePageInfo(); " +
                 "})();";
        webView.evaluateJavaScript(js);
    }

    function initFocus() {
        readerRootContainer.requestFocus();
        if (initialFocusTimer) {
            initialFocusTimer.start();
        }
    }

    function accumulateScroll(deltaY) {
        scrollByDelta(deltaY);
    }

    function flushPendingScroll() {
    }

    function handleKbdTouch(event) {
        var now = new Date().getTime();
        var t = event.touchType;
        var curY = event.screenY;
        var curX = event.screenX;

        // Deduplicate events that bubble from child controls in the same tick (< 25ms)
        if (readerRootContainer.lastKbdTouchType === t &&
            Math.abs(readerRootContainer.lastKbdTouchY - curY) < 0.01 &&
            Math.abs(readerRootContainer.lastKbdTouchX - curX) < 0.01 &&
            (now - readerRootContainer.lastKbdTouchTime < 25)) {
            return;
        }
        readerRootContainer.lastKbdTouchType = t;
        readerRootContainer.lastKbdTouchY = curY;
        readerRootContainer.lastKbdTouchX = curX;
        readerRootContainer.lastKbdTouchTime = now;

        // TouchType: 0 = Down, 1 = Move, 2 = Up, 3 = Cancel
        if (t === 0) {
            readerRootContainer.kbdTouchStartY = curY;
            readerRootContainer.kbdTouchStartX = curX;
            readerRootContainer.kbdTouchLastY = curY;
            readerRootContainer.kbdTouchLastX = curX;
            readerRootContainer.kbdTouchAccumY = 0;
            readerRootContainer.kbdTouchAccumX = 0;
            readerRootContainer.kbdPageTurnedInGesture = false;
            readerRootContainer.kbdVelocityY = 0;
            readerRootContainer.kbdLastMoveTime = now;
            inertiaTimer.stop();
        } else if (t === 1) {
            var dt = now - readerRootContainer.kbdLastMoveTime;
            if (dt <= 0) dt = 16;
            readerRootContainer.kbdLastMoveTime = now;

            if (readerRootContainer.kbdTouchLastY <= 0) {
                readerRootContainer.kbdTouchLastY = curY;
                readerRootContainer.kbdTouchLastX = curX;
                return;
            }

            var stepY = curY - readerRootContainer.kbdTouchLastY;
            var stepX = curX - readerRootContainer.kbdTouchLastX;
            readerRootContainer.kbdTouchLastY = curY;
            readerRootContainer.kbdTouchLastX = curX;

            if (stepY > 80) stepY = 80;
            else if (stepY < -80) stepY = -80;

            // Velocity tracking for kinetic momentum coasting
            var instVelocityY = -stepY / dt;
            readerRootContainer.kbdVelocityY = 0.60 * instVelocityY + 0.40 * readerRootContainer.kbdVelocityY;

            if (bookManager.readingMode === "scroll" || (bookManager.isPdf && !bookManager.isPdfReflow)) {
                // Swiping UP (stepY < 0) advances DOWN into content (scrollDelta > 0)
                // Swiping DOWN (stepY > 0) scrolls UP into earlier content (scrollDelta < 0)
                // Passport touchkeyboard: giảm nhạy theo phương dọc
                var scrollDelta = -stepY * 0.35;
                if (Math.abs(scrollDelta) >= 0.5) {
                    scrollByDelta(scrollDelta);
                }
            } else {
                // Paged mode (EPUB / TXT / FB2 / Reflow)
                // Passport touchkeyboard: giảm nhạy dọc, vuốt ngang 70% chiều rộng bàn phím mới sang trang
                readerRootContainer.kbdTouchAccumY += stepY;
                readerRootContainer.kbdTouchAccumX += stepX;

                var kbdW = (typeof bookManager !== "undefined" && bookManager.screenWidth > 0) ? bookManager.screenWidth : 1440;
                var horizReq = 0.25 * kbdW;
                var totalDeltaX = curX - readerRootContainer.kbdTouchStartX;

                if (!readerRootContainer.kbdPageTurnedInGesture && (now - readerRootContainer.lastPageTurnTime > 260)) {
                    // Ngang: vuốt nhẹ 25% chiều rộng bàn phím để sang trang mượt mà
                    if (totalDeltaX <= -horizReq || readerRootContainer.kbdTouchAccumX <= -horizReq) {
                        turnNext();
                        readerRootContainer.lastPageTurnTime = now;
                        readerRootContainer.kbdPageTurnedInGesture = true;
                    } else if (totalDeltaX >= horizReq || readerRootContainer.kbdTouchAccumX >= horizReq) {
                        turnPrev();
                        readerRootContainer.lastPageTurnTime = now;
                        readerRootContainer.kbdPageTurnedInGesture = true;
                    // Dọc: giảm nhạy (phải vuốt dứt khoát >= 220px)
                    } else if (readerRootContainer.kbdTouchAccumY < -220) {
                        turnNext();
                        readerRootContainer.lastPageTurnTime = now;
                        readerRootContainer.kbdPageTurnedInGesture = true;
                    } else if (readerRootContainer.kbdTouchAccumY > 220) {
                        turnPrev();
                        readerRootContainer.lastPageTurnTime = now;
                        readerRootContainer.kbdPageTurnedInGesture = true;
                    }
                }
            }
        } else if (t === 2 || t === 3) {
            readerRootContainer.kbdTouchLastY = 0;
            readerRootContainer.kbdTouchLastX = 0;
            readerRootContainer.kbdTouchAccumY = 0;
            readerRootContainer.kbdTouchAccumX = 0;
            readerRootContainer.kbdPageTurnedInGesture = false;

            // Start kinetic inertia coasting if flicked with velocity in scroll mode
            if (bookManager.readingMode === "scroll" || (bookManager.isPdf && !bookManager.isPdfReflow)) {
                if (Math.abs(readerRootContainer.kbdVelocityY) > 0.25) {
                    inertiaTimer.start();
                }
            }
        }
    }

    function handleTrackpad(event) {
        var now = new Date().getTime();
        // If capacitive touch keyboard on Passport is active, defer to it
        if (now - readerRootContainer.lastKbdTouchTime < 300) {
            return;
        }

        var isM = event.isMove();
        var dy = event.deltaY;
        var dx = event.deltaX;

        // Deduplicate bubbled event from child controls in same tick (< 20ms)
        if (readerRootContainer.lastTrackpadDy === dy &&
            readerRootContainer.lastTrackpadDx === dx &&
            readerRootContainer.lastTrackpadIsMove === isM &&
            (now - readerRootContainer.lastTrackpadTime < 20)) {
            return;
        }
        readerRootContainer.lastTrackpadDy = dy;
        readerRootContainer.lastTrackpadDx = dx;
        readerRootContainer.lastTrackpadIsMove = isM;
        readerRootContainer.lastTrackpadTime = now;

        if (isM) {
            if (bookManager.readingMode === "scroll" || (bookManager.isPdf && !bookManager.isPdfReflow)) {
                // Swiping UP on Passport/Classic keyboard has deltaY < 0.
                // In reading, swiping UP advances DOWN into content (positive scrollDelta).
                var scrollDelta = -dy * 2.0;
                if (Math.abs(scrollDelta) >= 0.8) {
                    accumulateScroll(scrollDelta);
                }
            } else {
                // Paged mode (EPUB / TXT / FB2 / PDF Reflow)
                readerRootContainer.tpAccumY += dy;
                readerRootContainer.tpAccumX += dx;

                // Calibrated threshold for Classic trackpad navigation
                if (!readerRootContainer.tpPageTurned && (now - readerRootContainer.lastPageTurnTime > 260)) {
                    // Swiping UP (dy < 0) or LEFT (dx < 0) = Next page
                    if (readerRootContainer.tpAccumY < -15 || readerRootContainer.tpAccumX < -18) {
                        turnNext();
                        readerRootContainer.lastPageTurnTime = now;
                        readerRootContainer.tpAccumY = 0;
                        readerRootContainer.tpAccumX = 0;
                        readerRootContainer.tpPageTurned = true;
                    // Swiping DOWN (dy > 0) or RIGHT (dx > 0) = Prev page
                    } else if (readerRootContainer.tpAccumY > 15 || readerRootContainer.tpAccumX > 18) {
                        turnPrev();
                        readerRootContainer.lastPageTurnTime = now;
                        readerRootContainer.tpAccumY = 0;
                        readerRootContainer.tpAccumX = 0;
                        readerRootContainer.tpPageTurned = true;
                    }
                }
            }
        } else if (event.isEnd()) {
            readerRootContainer.tpAccumY = 0;
            readerRootContainer.tpAccumX = 0;
            readerRootContainer.tpPageTurned = false;
            flushPendingScroll();
        } else if (event.isPressed()) {
            turnNext();
        }
    }

    function handleNavigation(event) {
        var now = new Date().getTime();
        // If trackpad move was handled very recently, ignore navigation duplicate
        if (now - readerRootContainer.lastTrackpadTime < 150) {
            return;
        }

        var t = event.navigationEventType;
        var mult = event.multiplier > 0 ? event.multiplier : 1;

        // NavigationEventType: Left = 0, Right = 1, Up = 2, Down = 3, Cancel = 4
        if (bookManager.readingMode === "scroll" || (bookManager.isPdf && !bookManager.isPdfReflow)) {
            // Up = scroll forward/down (+); Down = scroll backward/up (-)
            if (t === 2) {
                accumulateScroll(75 * mult);
            } else if (t === 3) {
                accumulateScroll(-75 * mult);
            }
        } else {
            // Up (2) or Left (0) = Next page; Down (3) or Right (1) = Prev page
            if (t === 2 || t === 0) {
                turnNext();
            } else if (t === 3 || t === 1) {
                turnPrev();
            }
        }
    }

    function handleHardwareKey(event) {
        var k = event.key;
        var u = ("" + event.unicode).toLowerCase();

        // Space or Shift+Space (supports TTS play/pause)
        if (k === 32 || u === " ") {
            if (ttsManager.active) {
                ttsManager.togglePlayPause();
            } else if (event.isShiftPressed) {
                turnPrev();
            } else if (bookManager.readingMode === "scroll" || (bookManager.isPdf && !bookManager.isPdfReflow)) {
                scrollLineDown();
            } else {
                turnNext();
            }
            return;
        }

        // Hardware PageDown / Down arrow / Right arrow
        if (k === 61271 || k === 61269 || k === 61479 || k === 61481 || k === 61273 || k === 61483) {
            if (bookManager.readingMode === "scroll" || (bookManager.isPdf && !bookManager.isPdfReflow)) {
                scrollLineDown();
            } else {
                turnNext();
            }
            return;
        }

        // Hardware PageUp / Up arrow / Left arrow / Backspace
        if (k === 8 || k === 61270 || k === 61268 || k === 61478 || k === 61480 || k === 61272 || k === 61482) {
            if (bookManager.readingMode === "scroll" || (bookManager.isPdf && !bookManager.isPdfReflow)) {
                scrollLineUp();
            } else {
                turnPrev();
            }
            return;
        }
    }

    function handleThemeChanged() { applyStyleLive(); }
    function handleFontSizeChanged() { applyStyleLive(); }
    function handleFontFamilyChanged() { applyStyleLive(); }
    function handleMarginsChanged() { applyStyleLive(); }
    function handleLineSpacingChanged() { applyStyleLive(); }

    function handlePageRenderTimeout() {
        readerPage.isRendering = false;
        bookManager.setIsLoadingBook(false);
    }

    function handleRequestScrollToAnchor(anchor) {
        if (anchor && anchor !== "") {
            webView.evaluateJavaScript("if (typeof scrollToAnchor === 'function') scrollToAnchor('" + anchor + "');");
        }
    }

    function handleCurrentChapterChanged() {
        if (!bookManager.isPdf) {
            readerPage.isRendering = true;
            bookManager.startWatchdog(1200);
            webView.html = bookManager.currentContent;
        }
    }

    function handleReadingModeChanged() {
        if (bookManager.isPdf) {
            readerPage.isRendering = true;
            bookManager.startWatchdog(1200);
        }
        webView.html = bookManager.currentContent;
    }

    function handleCurrentPdfImageChanged() {
        readerPage.isRendering = false;
        bookManager.stopWatchdog();
        bookManager.setIsLoadingBook(false);
    }

    function handleTtsCurrentSentenceChanged(index, text) {
        if (!bookManager.isPdf || bookManager.isPdfReflow) {
            webView.evaluateJavaScript("if (typeof highlightTtsSentence === 'function') highlightTtsSentence(" + JSON.stringify(text) + ");");
        }
    }

    function handleTtsActiveChanged(active) {
        if (!active) {
            webView.evaluateJavaScript("if (typeof clearTtsHighlight === 'function') clearTtsHighlight();");
        }
    }

    function handleTtsErrorOccurred(message) {
        if (message && message !== "") {
            ttsToast.body = message;
            ttsToast.show();
        }
    }

    function cleanup() {
        try {
            if (typeof bookManager !== "undefined" && bookManager) {
                bookManager.themeChanged.disconnect(readerPage.handleThemeChanged);
                bookManager.fontSizeChanged.disconnect(readerPage.handleFontSizeChanged);
                bookManager.fontFamilyChanged.disconnect(readerPage.handleFontFamilyChanged);
                bookManager.marginsChanged.disconnect(readerPage.handleMarginsChanged);
                bookManager.lineSpacingChanged.disconnect(readerPage.handleLineSpacingChanged);
                bookManager.pageRenderTimeout.disconnect(readerPage.handlePageRenderTimeout);
                bookManager.requestScrollToAnchor.disconnect(readerPage.handleRequestScrollToAnchor);
                bookManager.currentChapterChanged.disconnect(readerPage.handleCurrentChapterChanged);
                bookManager.readingModeChanged.disconnect(readerPage.handleReadingModeChanged);
                bookManager.currentPdfImageChanged.disconnect(readerPage.handleCurrentPdfImageChanged);
            }
        } catch (e) {}
        try {
            if (typeof ttsManager !== "undefined" && ttsManager) {
                ttsManager.currentSentenceChanged.disconnect(readerPage.handleTtsCurrentSentenceChanged);
                ttsManager.activeChanged.disconnect(readerPage.handleTtsActiveChanged);
                ttsManager.errorOccurred.disconnect(readerPage.handleTtsErrorOccurred);
            }
        } catch (e) {}
        try {
            if (searchSheet && typeof searchSheet.cleanup === "function") {
                searchSheet.cleanup();
            }
        } catch (e) {}
    }

    onCreationCompleted: {
        bookManager.setIsReaderActive(true);
        initFocus();
        bookManager.setIsLoadingBook(false);
        if (bookManager.isPdf) {
            if (bookManager.currentPdfImage) {
                readerPage.isRendering = false;
            } else {
                bookManager.startWatchdog(1200);
            }
        } else {
            readerPage.isRendering = false;
        }

        var bm = bookManager;
        var rp = readerPage;
        bm.themeChanged.connect(rp.handleThemeChanged);
        bm.fontSizeChanged.connect(rp.handleFontSizeChanged);
        bm.fontFamilyChanged.connect(rp.handleFontFamilyChanged);
        bm.marginsChanged.connect(rp.handleMarginsChanged);
        bm.lineSpacingChanged.connect(rp.handleLineSpacingChanged);
        bm.pageRenderTimeout.connect(rp.handlePageRenderTimeout);
        bm.requestScrollToAnchor.connect(rp.handleRequestScrollToAnchor);
        bm.currentChapterChanged.connect(rp.handleCurrentChapterChanged);
        bm.readingModeChanged.connect(rp.handleReadingModeChanged);
        bm.currentPdfImageChanged.connect(rp.handleCurrentPdfImageChanged);

        ttsManager.currentSentenceChanged.connect(rp.handleTtsCurrentSentenceChanged);
        ttsManager.activeChanged.connect(rp.handleTtsActiveChanged);
        ttsManager.errorOccurred.connect(rp.handleTtsErrorOccurred);
    }

    function turnNext() {
        if (bookManager.isPdf && !bookManager.isPdfReflow) {
            pdfScrollView.scrollPdfToBottomOnLoad = false;
            bookManager.nextChapter();
        } else if (bookManager.readingMode === "paged") {
            webView.evaluateJavaScript("if (typeof nextPage === 'function') nextPage();");
        } else {
            webView.evaluateJavaScript("if (typeof pageDown === 'function') pageDown();");
        }
    }

    function turnPrev() {
        if (bookManager.isPdf && !bookManager.isPdfReflow) {
            pdfScrollView.scrollPdfToBottomOnLoad = true;
            bookManager.prevChapter();
        } else if (bookManager.readingMode === "paged") {
            webView.evaluateJavaScript("if (typeof prevPage === 'function') prevPage();");
        } else {
            webView.evaluateJavaScript("if (typeof pageUp === 'function') pageUp();");
        }
    }

    function scrollByDelta(deltaY) {
        if (bookManager.isPdf && !bookManager.isPdfReflow) {
            // PDF Scroll View continuous vertical scrolling
            var curY = pdfScrollView.viewableArea.y;
            var maxScrollY = Math.max(0, pdfScrollView.getPdfContentHeight() - pdfScrollView.viewableArea.height);
            var targetY = curY + deltaY;

            if (maxScrollY <= 15) {
                if (deltaY > 30) {
                    pdfScrollView.scrollPdfToBottomOnLoad = false;
                    bookManager.nextChapter();
                } else if (deltaY < -30) {
                    pdfScrollView.scrollPdfToBottomOnLoad = true;
                    bookManager.prevChapter();
                }
            } else {
                if (targetY > maxScrollY + 25 && deltaY > 0) {
                    pdfScrollView.scrollPdfToBottomOnLoad = false;
                    bookManager.nextChapter();
                } else if (targetY < -25 && deltaY < 0) {
                    pdfScrollView.scrollPdfToBottomOnLoad = true;
                    bookManager.prevChapter();
                } else {
                    pdfScrollView.scrollToPoint(pdfScrollView.viewableArea.x, Math.max(0, Math.min(maxScrollY, targetY)), ScrollAnimation.None);
                }
            }
        } else {
            // EPUB / TXT / FB2 / PDF Reflow (WebView)
            if (bookManager.readingMode === "scroll") {
                webView.evaluateJavaScript("scrollByDelta(" + Math.round(deltaY) + ");");
            } else {
                // Paged mode
                if (deltaY > 30) turnNext();
                else if (deltaY < -30) turnPrev();
            }
        }
    }

    function scrollLineDown() {
        if (bookManager.isPdf) {
            bookManager.nextChapter();
        } else {
            webView.evaluateJavaScript("if (typeof scrollDown === 'function') scrollDown(); else window.scrollBy(0, 80);");
        }
    }

    function scrollLineUp() {
        if (bookManager.isPdf) {
            bookManager.prevChapter();
        } else {
            webView.evaluateJavaScript("if (typeof scrollUp === 'function') scrollUp(); else window.scrollBy(0, -80);");
        }
    }

    function increaseFont() {
        var newSize = bookManager.fontSize + 2;
        bookManager.setFontSize(newSize);
        applyStyleLive();
    }

    function decreaseFont() {
        var newSize = bookManager.fontSize - 2;
        bookManager.setFontSize(newSize);
        applyStyleLive();
    }

    function toggleTheme() {
        if (bookManager.isPdf) {
            bookManager.togglePdfInvert();
        } else {
            if (bookManager.theme === "dark") {
                bookManager.setTheme("sepia");
            } else if (bookManager.theme === "sepia") {
                bookManager.setTheme("light");
            } else {
                bookManager.setTheme("dark");
            }
            applyStyleLive();
        }
    }

    attachedObjects: [
        SettingsSheet {
            id: settingsSheet
            onClosed: { readerRootContainer.requestFocus(); }
        },
        BookDetailSheet {
            id: bookDetailSheet
            onClosed: { readerRootContainer.requestFocus(); }
        },
        TocSheet {
            id: tocSheet
            onClosed: { readerRootContainer.requestFocus(); }
        },
        SearchSheet {
            id: searchSheet
            onClosed: {
                readerRootContainer.requestFocus();
                if (!searchSheet.jumpedFromSearch) {
                    webView.evaluateJavaScript("if (typeof clearInPageSearch === 'function') clearInPageSearch();");
                    searchSheet.inPageTotal = 0;
                    searchSheet.inPageCurrent = 0;
                }
            }
            onPerformInPageSearch: {
                var escQuery = query.replace(/\\/g, "\\\\").replace(new RegExp("\x27", "g"), "\\\x27");
                webView.evaluateJavaScript("if (typeof searchInPage === 'function') searchInPage('" + escQuery + "', false);");
            }
            onJumpToMatch: {
                webView.evaluateJavaScript("if (typeof jumpToMatch === 'function') jumpToMatch(" + matchIndex + ");");
            }
            onInPageNext: {
                webView.evaluateJavaScript("if (typeof searchNext === 'function') searchNext();");
            }
            onInPagePrev: {
                webView.evaluateJavaScript("if (typeof searchPrev === 'function') searchPrev();");
            }
            onInPageClear: {
                webView.evaluateJavaScript("if (typeof clearInPageSearch === 'function') clearInPageSearch();");
                searchSheet.inPageTotal = 0;
                searchSheet.inPageCurrent = 0;
            }
        },
        DictTranslateSheet {
            id: dictSheet
            onClosed: { readerRootContainer.requestFocus(); }
        },
        HighlightSheet {
            id: highlightSheet
            onClosed: { readerRootContainer.requestFocus(); }
            onHighlightJumpRequested: {
                webView.evaluateJavaScript("if (typeof scrollToHighlight === 'function') scrollToHighlight('" + highlightId + "');");
            }
        },
        HighlightNoteSheet {
            id: highlightNoteSheet
            onClosed: { readerRootContainer.requestFocus(); }
            onSavedCallback: {
                webView.evaluateJavaScript("var el = document.querySelector('mark[data-hl-id=\"" + highlightId + "\"]'); if (el) { el.className = 'berry-highlight berry-hl-' + " + JSON.stringify(currentColor) + "; el.setAttribute('data-hl-note', " + JSON.stringify(currentNote) + "); }");
            }
            onDeletedCallback: {
                webView.evaluateJavaScript("if (typeof removeHighlightFromDom === 'function') removeHighlightFromDom('" + highlightId + "');");
            }
        },
        SystemToast {
            id: ttsToast
            body: ""
        },
        SystemToast {
            id: readerToast
            body: ""
        }
    ]

    actions: [
        // =====================================================================
        // ON-BAR ACTIONS (Bottom Action Bar: Mục lục, Tìm kiếm, Mode đọc)
        // =====================================================================
        ActionItem {
            title: qsTr("Mục lục") + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.OnBar
            imageSource: "asset:///icons/ic_toc.png"
            onTriggered: {
                tocSheet.open();
            }
        },
        ActionItem {
            title: qsTr("Tìm kiếm") + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.OnBar
            imageSource: "asset:///icons/ic_search.png"
            onTriggered: {
                searchSheet.open();
            }
        },
        ActionItem {
            title: bookManager.theme === "dark" ? (qsTr("Tối") + Retranslate.onLanguageChanged) :
                   (bookManager.theme === "sepia" ? (qsTr("Sepia") + Retranslate.onLanguageChanged) :
                   (qsTr("Sáng") + Retranslate.onLanguageChanged))
            ActionBar.placement: ActionBarPlacement.OnBar
            imageSource: bookManager.theme === "dark" ? "asset:///icons/ic_mode_dark.png" :
                         (bookManager.theme === "sepia" ? "asset:///icons/ic_mode_sepia.png" :
                         "asset:///icons/ic_mode_light.png")
            onTriggered: {
                if (bookManager.theme === "light") {
                    bookManager.setTheme("sepia");
                } else if (bookManager.theme === "sepia") {
                    bookManager.setTheme("dark");
                } else {
                    bookManager.setTheme("light");
                }
            }
        },

        // =====================================================================
        // IN-OVERFLOW ACTIONS (Right Menu: Clean & Minimal)
        // =====================================================================
        ActionItem {
            title: qsTr("Dấu trang & Ghi chú") + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: "asset:///icons/ic_bookmarks.png"
            onTriggered: {
                highlightSheet.open();
            }
        },
        ActionItem {
            title: bookManager.readingMode === "scroll" ?
                   (qsTr("Chế độ Vuốt trang (Swipe)") + Retranslate.onLanguageChanged) :
                   (qsTr("Chế độ Cuộn (Scroll)") + Retranslate.onLanguageChanged)
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: bookManager.readingMode === "scroll" ? "asset:///icons/ic_page.png" : "asset:///icons/ic_scroll.png"
            enabled: !bookManager.isPdf || bookManager.isPdfReflow
            onTriggered: {
                var nextMode = bookManager.readingMode === "scroll" ? "paged" : "scroll";
                bookManager.setReadingMode(nextMode);
                readerToast.body = nextMode === "scroll" ?
                                   (qsTr("Đã chuyển sang chế độ Cuộn liên tục") + Retranslate.onLanguageChanged) :
                                   (qsTr("Đã chuyển sang chế độ Vuốt lật trang") + Retranslate.onLanguageChanged);
                readerToast.show();
            }
        },
        ActionItem {
            title: (bookManager.isCurrentChapterBookmarked ? qsTr("Bỏ dấu trang") : qsTr("Đánh dấu trang")) + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: bookManager.isCurrentChapterBookmarked ? "asset:///icons/ic_bookmarks.png" : "asset:///icons/ic_bookmark_add.png"
            onTriggered: {
                var isAdded = bookManager.toggleBookmarkCurrentPosition();
                readerToast.body = isAdded ? (qsTr("Đã đánh dấu trang: ") + bookManager.currentChapterTitle) : (qsTr("Đã bỏ dấu trang"));
                readerToast.show();
            }
        },
        ActionItem {
            id: favActionItem
            title: (bookManager.isFavorite(bookManager.currentFilePath) ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: bookManager.isFavorite(bookManager.currentFilePath) ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
            onTriggered: {
                var isFav = bookManager.toggleFavorite(bookManager.currentFilePath);
                favActionItem.imageSource = isFav ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png";
                favActionItem.title = (isFav ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged;
                readerToast.body = isFav ? (qsTr("Đã thêm vào yêu thích: ") + bookManager.currentBookTitle) : (qsTr("Đã bỏ yêu thích"));
                readerToast.show();
            }
        },
        ActionItem {
            title: (bookManager.isPdfReflow ? qsTr("PDF gốc") : qsTr("Dàn chữ (OCR)")) + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: "asset:///icons/ic_text_mode.png"
            enabled: bookManager.isPdf && (bookManager.hasPdfText || bookManager.isPdfReflow)
            onTriggered: {
                bookManager.togglePdfReflow();
            }
        },
        ActionItem {
            title: (ttsManager.active ? qsTr("Dừng (TTS)") : qsTr("Đọc (TTS)")) + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: "asset:///icons/ic_tts.png"
            enabled: !bookManager.isPdf || bookManager.isPdfReflow
            onTriggered: {
                bookManager.toggleTts();
            }
        },
        ActionItem {
            title: qsTr("Từ điển") + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: "asset:///icons/ic_translate.png"
            onTriggered: {
                dictSheet.open();
            }
        },
        ActionItem {
            title: qsTr("Thông tin") + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: "asset:///icons/ic_info.png"
            onTriggered: {
                bookDetailSheet.open();
            }
        },
        ActionItem {
            title: qsTr("Thoát") + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: "asset:///icons/ic_exit.png"
            shortcuts: [
                Shortcut { key: "Q" }
            ]
            onTriggered: {
                app.promptExit();
            }
        }
    ]

    // Hardware Keyboard shortcuts (Q10, Classic, Passport)
    shortcuts: [
        Shortcut {
            key: "Space"
            onTriggered: {
                if (ttsManager.active) {
                    ttsManager.togglePlayPause();
                } else if (bookManager.readingMode === "scroll" || (bookManager.isPdf && !bookManager.isPdfReflow)) {
                    scrollLineDown();
                } else {
                    turnNext();
                }
            }
        },
        Shortcut {
            key: "m"
            onTriggered: {
                tocSheet.open();
            }
        },
        Shortcut {
            key: "g"
            onTriggered: {
                tocSheet.open();
            }
        },
        Shortcut {
            key: "h"
            onTriggered: {
                var isBmAdded = bookManager.toggleBookmarkCurrentPosition();
                readerToast.body = isBmAdded ? (qsTr("Đã đánh dấu trang: ") + bookManager.currentChapterTitle) : (qsTr("Đã bỏ dấu trang"));
                readerToast.show();
            }
        },
        Shortcut {
            key: "s"
            onTriggered: {
                settingsSheet.open();
            }
        },
        Shortcut {
            key: "l"
            onTriggered: {
                dictSheet.open();
            }
        },
        Shortcut {
            key: "d"
            onTriggered: {
                dictSheet.open();
            }
        },
        Shortcut {
            key: "o"
            onTriggered: {
                app.openFilePicker();
            }
        },
        Shortcut {
            key: "v"
            onTriggered: {
                if (bookManager.isPdf && !bookManager.isPdfReflow) {
                    bookManager.togglePdfInvert();
                } else {
                    bookManager.setTheme(bookManager.theme === "dark" ? "light" : "dark");
                }
            }
        },
        Shortcut {
            key: "x"
            onTriggered: {
                if (bookManager.isPdf) {
                    bookManager.togglePdfReflow();
                }
            }
        },
        Shortcut {
            key: "a"
            onTriggered: {
                if (!bookManager.isPdf || bookManager.isPdfReflow) bookManager.toggleTts();
            }
        },
        Shortcut {
            key: "t"
            onTriggered: {
                if (bookManager.isPdf) bookManager.goToChapter(1);
                else bookManager.goToChapter(0);
            }
        },
        Shortcut {
            key: "u"
            onTriggered: turnPrev()
        },
        Shortcut {
            key: "j"
            onTriggered: scrollLineDown()
        },
        Shortcut {
            key: "k"
            onTriggered: scrollLineUp()
        },
        Shortcut {
            key: "n"
            onTriggered: turnNext()
        },
        Shortcut {
            key: "p"
            onTriggered: turnPrev()
        },
        Shortcut {
            key: "f"
            onTriggered: {
                searchSheet.open();
            }
        },
        Shortcut {
            key: "Return"
            onTriggered: toggleFullscreen()
        },
        Shortcut {
            key: "Enter"
            onTriggered: toggleFullscreen()
        },
        Shortcut {
            key: "r"
            onTriggered: {
                if (bookManager.isPdf) bookManager.rotatePdf();
            }
        },
        Shortcut {
            key: "i"
            onTriggered: {
                bookDetailSheet.open();
            }
        },
        Shortcut {
            key: "z"
            onTriggered: {
                bookManager.setFontSize(Math.min(36, bookManager.fontSize + 1));
            }
        },
        Shortcut {
            key: "w"
            onTriggered: {
                bookManager.setFontSize(Math.max(12, bookManager.fontSize - 1));
            }
        },
        Shortcut {
            key: "c"
            onTriggered: {
                if (!bookManager.isPdf || bookManager.isPdfReflow) {
                    var nextMode = bookManager.readingMode === "scroll" ? "paged" : "scroll";
                    bookManager.setReadingMode(nextMode);
                    readerToast.body = nextMode === "scroll" ?
                                       (qsTr("Đã chuyển sang chế độ Cuộn liên tục") + Retranslate.onLanguageChanged) :
                                       (qsTr("Đã chuyển sang chế độ Vuốt lật trang") + Retranslate.onLanguageChanged);
                    readerToast.show();
                }
            }
        },
        Shortcut {
            key: "b"
            onTriggered: {
                if (bookManager.isPdf) bookManager.goToChapter(bookManager.totalChapters);
                else bookManager.goToChapter(bookManager.totalChapters - 1);
            }
        }
    ]
}
