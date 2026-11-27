import bb.cascades 1.3
import bb.system 1.2

Page {
    id: readerPage

    // =========================================================================
    // NATIVE BLACKBERRY 10 TOP SWIPE-DOWN APPLICATION MENU (Inside Reader)
    // =========================================================================
    Menu.definition: MenuDefinition {
        helpAction: HelpActionItem {
            title: qsTr("Giới Thiệu") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_info.png"
            onTriggered: {
                app.showAbout();
            }
        }
        settingsAction: SettingsActionItem {
            title: qsTr("Cài Đặt") + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_settings.png"
            onTriggered: {
                settingsSheet.open();
            }
        }
        actions: [
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
    property bool showDisplayHud: false
    property bool showThemePopup: false
    property bool showFontPopup: false
    property bool showLayoutPopup: false
    property bool isScrubSyncing: false
    property bool autoScrollActive: false
    property int autoScrollSpeed: 2
    property int autoScrollPageTickCounter: 0
    property bool isRendering: true
    property string selectedText: ""
    property string pendingSearchQuery: ""
    property int pendingSearchMatchIndex: -1
    property bool readingRulerEnabled: false
    property bool showShortcutsHelp: false
    property bool pdfLockHorizontal: false
    property variant jumpHistory: []

    function pushJumpHistory() {
        var ch = bookManager.currentChapterIndex;
        var pg = bookManager.currentPageInChapter;
        var arr = jumpHistory ? jumpHistory.slice(0) : [];
        if (arr.length > 0) {
            var top = arr[arr.length - 1];
            if (top.chapter === ch && top.page === pg) return;
        }
        arr.push({ chapter: ch, page: pg });
        if (arr.length > 20) arr.shift();
        jumpHistory = arr;
    }

    function jumpBack() {
        var arr = jumpHistory ? jumpHistory.slice(0) : [];
        if (arr.length === 0) {
            readerToast.body = qsTr("Chưa có vị trí trước đó");
            readerToast.show();
            return;
        }
        var target = arr.pop();
        jumpHistory = arr;
        if (target.chapter !== bookManager.currentChapterIndex) {
            bookManager.goToChapter(bookManager.isPdf && !bookManager.isPdfReflow ? (target.chapter + 1) : target.chapter);
        } else if (!bookManager.isPdf || bookManager.isPdfReflow) {
            webView.evaluateJavaScript("if (typeof goToPage === 'function') goToPage(" + target.page + ");");
        }
        readerToast.body = qsTr("Đã quay lại vị trí trước");
        readerToast.show();
    }

    onFullscreenModeChanged: {
        if (fullscreenMode) {
            showDisplayHud = false;
            showThemePopup = false;
            showFontPopup = false;
            showLayoutPopup = false;
        }
        webView.evaluateJavaScript("window.isChromeVisible = " + (!fullscreenMode) + ";");
    }

    onShowDisplayHudChanged: {
        if (!showDisplayHud) {
            showThemePopup = false;
            showFontPopup = false;
            showLayoutPopup = false;
        }
    }

    function syncScrubberFromBookManager() {
        readerPage.isScrubSyncing = true;
        if (bookManager.isPdf && !bookManager.isPdfReflow) {
            pageScrubberSlider.toValue = Math.max(2, bookManager.totalChapters);
            pageScrubberSlider.value = Math.max(1, Math.min(pageScrubberSlider.toValue, bookManager.currentChapterIndex + 1));
        } else {
            pageScrubberSlider.toValue = Math.max(2, bookManager.totalPagesInChapter);
            pageScrubberSlider.value = Math.max(1, Math.min(pageScrubberSlider.toValue, bookManager.currentPageInChapter));
        }
        readerPage.isScrubSyncing = false;
    }

    function rebuildFontList() {
        fontListModel.clear();
        var items = [
            { name: "Sans", value: "sans" },
            { name: "Serif", value: "serif" },
            { name: "Garamond", value: "garamond" },
            { name: "Times", value: "times" },
            { name: "Mono", value: "monospace" },
            { name: "EPUB", value: "original" }
        ];
        var customs = bookManager.customFonts;
        for (var i = 0; i < customs.length; ++i) {
            if (customs[i] && customs[i].value) {
                items.push({ name: customs[i].name, value: customs[i].value });
            }
        }
        fontListModel.append(items);
    }

    onShowFontPopupChanged: {
        if (showFontPopup) { rebuildFontList(); }
    }

    function cycleFontFamily() {
        var order = ["sans", "serif", "garamond", "times", "monospace", "original"];
        var customs = bookManager.customFonts;
        for (var i = 0; i < customs.length; ++i) {
            if (customs[i] && customs[i].value) {
                order.push(customs[i].value);
            }
        }
        var idx = order.indexOf(bookManager.fontFamily);
        var next = order[(idx + 1) % order.length];
        bookManager.setFontFamily(next);
    }

    function toggleAutoScroll() {
        if (!autoScrollActive && ((bookManager.isPdf && !bookManager.isPdfReflow) || bookManager.readingMode !== "scroll")) {
            return;
        }
        autoScrollActive = !autoScrollActive;
        autoScrollPageTickCounter = 0;
        if (autoScrollActive) {
            if (ttsManager.active) {
                ttsManager.stop();
            }
            autoScrollTimer.start();
        } else {
            autoScrollTimer.stop();
        }
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
        appearance: TitleBarAppearance.Plain
        scrollBehavior: TitleBarScrollBehavior.Sticky
        visibility: readerPage.fullscreenMode ? ChromeVisibility.Hidden : ChromeVisibility.Visible

        kindProperties: FreeFormTitleBarKindProperties {
            expandableArea {
                indicatorVisibility: TitleBarExpandableAreaIndicatorVisibility.Hidden
            }
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
                background: bookManager.appTheme === "dark" ? Color.create("#141210") : Color.create("#F7F4EE")

                // 1. Back Button to exit reader (expanded, generous touch target)
                Container {
                    id: backButtonContainer
                    preferredWidth: bookManager.dp(100)
                    minWidth: bookManager.dp(88)
                    verticalAlignment: VerticalAlignment.Fill
                    layout: DockLayout {}

                    ImageButton {
                        accessibility.name: qsTr("Quay lại") + Retranslate.onLanguageChanged
                        horizontalAlignment: HorizontalAlignment.Center
                        verticalAlignment: VerticalAlignment.Center
                        defaultImageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_previous.png" : "asset:///icons/ic_previous_dark.png"
                        pressedImageSource: bookManager.appTheme === "dark" ? "asset:///icons/ic_previous.png" : "asset:///icons/ic_previous_dark.png"
                        preferredWidth: bookManager.dp(88)
                        preferredHeight: bookManager.dp(76)
                        onClicked: {
                            dismissOpeningBook();
                        }
                    }
                }

                // 2. Vertical Divider separating Back button from Book Title
                Container {
                    verticalAlignment: VerticalAlignment.Center
                    preferredWidth: 2
                    preferredHeight: 52
                    background: bookManager.appTheme === "dark" ? Color.create("#2A251E") : Color.create("#E6DFD3")
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
                        textStyle.color: bookManager.appTheme === "dark" ? Color.create("#F5EFE6") : Color.create("#1A1714")
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
            },
            Timer {
                id: autoScrollTimer
                interval: 60
                repeat: true
                onTriggered: {
                    if (!readerPage.autoScrollActive || ttsManager.active) {
                        autoScrollTimer.stop();
                        return;
                    }
                    var spd = Math.max(1, Math.min(5, readerPage.autoScrollSpeed));
                    if ((!bookManager.isPdf || bookManager.isPdfReflow) && bookManager.readingMode === "scroll") {
                        var pxSteps = [1.2, 2.2, 3.5, 5.2, 7.5];
                        readerPage.scrollByDelta(pxSteps[spd - 1]);
                    } else {
                        readerPage.autoScrollPageTickCounter++;
                        var tickThresholds = [300, 215, 150, 100, 66]; // ~18s, 13s, 9s, 6s, 4s
                        if (readerPage.autoScrollPageTickCounter >= tickThresholds[spd - 1]) {
                            readerPage.autoScrollPageTickCounter = 0;
                            readerPage.turnNext();
                        }
                    }
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

                    onNavigationRequested: {
                        if (request.navigationType === WebNavigationType.LinkClicked) {
                            request.action = WebNavigationRequestAction.Ignore;
                            var navUrl = "" + request.url;
                            if (navUrl.indexOf("http://") === 0 || navUrl.indexOf("https://") === 0 || navUrl.indexOf("mailto:") === 0) {
                                Qt.openUrlExternally(navUrl);
                            } else if (navUrl !== "" && navUrl !== "about:blank") {
                                bookManager.openInternalLink(navUrl, "");
                            }
                        }
                    }

                    onLoadingChanged: {
                        if (loadRequest.status === WebLoadStatus.Started) {
                            webContainer.opacity = 0.55;
                        } else if (loadRequest.status === WebLoadStatus.Failed) {
                            readerPage.isRendering = false;
                            bookManager.setIsLoadingBook(false);
                            bookManager.stopWatchdog();
                            webContainer.opacity = 1.0;
                        } else if (loadRequest.status === WebLoadStatus.Succeeded) {
                            readerPage.isRendering = false;
                            bookManager.setIsLoadingBook(false);
                            bookManager.stopWatchdog();
                            webFadeInAnim.play();
                            readerRootContainer.requestFocus();
                            webView.evaluateJavaScript("window.isChromeVisible = " + (!readerPage.fullscreenMode) + "; if (typeof updatePageInfo === 'function') updatePageInfo();");
                            applyBionicLive();
                            var pendingAnc = bookManager.consumePendingScrollAnchor();
                            if (pendingAnc && pendingAnc !== "") {
                                var escAnc = pendingAnc.replace(/\\/g, "\\\\").replace(new RegExp("\x27", "g"), "\\\x27");
                                webView.evaluateJavaScript("if (typeof scrollToAnchor === 'function') scrollToAnchor('" + escAnc + "');");
                            } else {
                                webView.evaluateJavaScript("if (typeof readingMode !== 'undefined' && readingMode === 'scroll') { if (typeof savedTargetScroll !== 'undefined' && savedTargetScroll > 0) { var vp = document.getElementById('reader-viewport'); if (vp) vp.scrollTop = savedTargetScroll; } else if (typeof savedTargetPage !== 'undefined' && savedTargetPage > 1 && typeof goToPage === 'function') { goToPage(savedTargetPage); } } else { if (typeof savedTargetPage !== 'undefined' && savedTargetPage > 1 && typeof goToPage === 'function') { goToPage(savedTargetPage); } else if (typeof savedTargetScroll !== 'undefined' && savedTargetScroll > 0) { var vp = document.getElementById('reader-viewport'); var h = window.innerHeight || (vp ? vp.clientHeight : 0) || 720; var p = Math.floor(savedTargetScroll / h) + 1; if (typeof goToPage === 'function') goToPage(p); } }");
                            }
                            if (readerPage.pendingSearchQuery !== "") {
                                var qPending = readerPage.pendingSearchQuery;
                                var idxPending = readerPage.pendingSearchMatchIndex;
                                readerPage.pendingSearchQuery = "";
                                readerPage.pendingSearchMatchIndex = -1;
                                var escQ = qPending.replace(/\\/g, "\\\\").replace(new RegExp("\x27", "g"), "\\\x27");
                                var searchJs = "if (typeof searchInPage === 'function') { searchInPage('" + escQ + "', false); ";
                                if (idxPending >= 0) {
                                    searchJs += "if (typeof jumpToMatch === 'function') jumpToMatch(" + idxPending + "); ";
                                }
                                searchJs += "}";
                                webView.evaluateJavaScript(searchJs);
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
                            webView.evaluateJavaScript("if (window.getSelection && window.getSelection().removeAllRanges) window.getSelection().removeAllRanges(); var b = document.getElementById('libris-selection-bubble'); if (b) b.style.display = 'none';");
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
                        } else if (str.indexOf("showFootnote:") === 0) {
                            var fnPayload = str.substr(13);
                            var fnColon = fnPayload.indexOf(":");
                            if (fnColon !== -1) {
                                var fnTitle = decodeURIComponent(fnPayload.substring(0, fnColon));
                                var fnBody = decodeURIComponent(fnPayload.substring(fnColon + 1));
                                footnoteOverlayContainer.footnoteTitle = fnTitle.length > 0 ? fnTitle : qsTr("Chú thích");
                                footnoteOverlayContainer.footnoteBody = fnBody;
                                footnoteOverlayContainer.visible = true;
                            }
                        } else if (str.indexOf("openInternalLink:") === 0) {
                            var linkPayload = str.substr(17);
                            var linkColon = linkPayload.indexOf(":");
                            if (linkColon !== -1) {
                                var linkHref = decodeURIComponent(linkPayload.substring(0, linkColon));
                                var linkTitle = decodeURIComponent(linkPayload.substring(linkColon + 1));
                                bookManager.openInternalLink(linkHref, linkTitle);
                            } else {
                                bookManager.openInternalLink(decodeURIComponent(linkPayload), "");
                            }
                        } else if (str.indexOf("openExternalUrl:") === 0) {
                            var extUrl = decodeURIComponent(str.substr(16));
                            Qt.openUrlExternally(extUrl);
                        }
                    }
                }
            }

            // View 2: PDF & DJVU (Libris Vector Engine / Poppler vector render - Fit to Width)
            ScrollView {
                id: pdfScrollView
                visible: bookManager.isPdf && !bookManager.isPdfReflow
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                scrollViewProperties.scrollMode: readerPage.pdfLockHorizontal ? ScrollMode.Vertical : ScrollMode.Both
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

                attachedObjects: [
                    Timer {
                        id: pdfTapTimer
                        interval: 230
                        repeat: false
                        onTriggered: {
                            if (!readerPage.fullscreenMode || pdfScrollView.contentScale <= 1.05) {
                                readerPage.toggleFullscreen();
                            }
                        }
                    }
                ]

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
                                // Double tap: cancel pending chrome toggle, reset or zoom 2x
                                pdfTapTimer.stop();
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
                                pdfTapTimer.start();
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
                            }
                        ]

                        onImageChanged: {
                            pdfImageView.resetPreferredWidth();
                            pdfImageView.resetPreferredHeight();
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
                gestureHandlers: [
                    TapHandler {
                        onTapped: {
                            readerPage.handlePageRenderTimeout();
                        }
                    }
                ]
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                background: bookManager.theme === "dark" ? Color.create("#D0000000") :
                            (bookManager.theme === "sepia" ? Color.create("#D0F4ECD8") : Color.create("#D0FFFFFF"))
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
                        onCreationCompleted: { if (visible) start(); }
                        onVisibleChanged: { if (visible) start(); else stop(); }
                    }

                    Label {
                        topMargin: 14
                        horizontalAlignment: HorizontalAlignment.Center
                        text: qsTr("Đang hiển thị nội dung...") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: bookManager.theme === "dark" ? Color.create("#AAAAAA") :
                                         (bookManager.theme === "sepia" ? Color.create("#665544") : Color.create("#5C554C"))
                    }

                    Label {
                        topMargin: 8
                        horizontalAlignment: HorizontalAlignment.Center
                        text: qsTr("Chạm màn hình để hủy") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: bookManager.theme === "dark" ? Color.White :
                                         (bookManager.theme === "sepia" ? Color.create("#3E2D1D") : Color.Black)
                    }
                }
            }

            // Chrome dismissal on tap is handled directly inside WebView JS (window.isChromeVisible)
            // and pdfScrollView.onTouch so that scroll, text selection, highlights, footnotes, and
            // double-tap zoom remain interactive even while chrome is open.
            Container {
                id: chromeDismissOverlay
                visible: false
                enabled: false
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
            }

            // =========================================================================
            // NON-BLOCKING WARM AMBER NIGHT FILTER OVERLAY
            // =========================================================================
            Container {
                id: warmNightOverlay
                visible: bookManager.warmFilterEnabled
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                touchPropagationMode: TouchPropagationMode.None
                overlapTouchPolicy: OverlapTouchPolicy.Allow
                background: Color.create(0.96, 0.54, 0.08, Math.max(0.06, Math.min(0.42, bookManager.warmFilterIntensity / 145.0)))
            }

            // =========================================================================
            // FOOTNOTE POPUP OVERLAY
            // Displays footnote / annotation content in a floating modal card
            // =========================================================================
            Container {
                id: footnoteOverlayContainer
                visible: false
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                background: Color.create(0.0, 0.0, 0.0, 0.6)
                touchPropagationMode: TouchPropagationMode.Full

                property string footnoteTitle: qsTr("Chú thích")
                property string footnoteBody: ""

                layout: DockLayout {}

                gestureHandlers: [
                    TapHandler {
                        onTapped: {
                            footnoteOverlayContainer.visible = false;
                        }
                    }
                ]

                Container {
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    preferredWidth: bookManager.screenWidth > 0 ? Math.min(bookManager.screenWidth - bookManager.dp(48), bookManager.dp(500)) : bookManager.dp(460)
                    gestureHandlers: [
                        TapHandler {
                            onTapped: {}
                        }
                    ]
                    maxHeight: bookManager.dp(440)
                    background: bookManager.theme === "dark" ? Color.create("#181512") :
                                (bookManager.theme === "sepia" ? Color.create("#F4ECD8") : Color.create("#FFFFFF"))
                    topPadding: bookManager.dp(16)
                    bottomPadding: bookManager.dp(16)
                    leftPadding: bookManager.dp(20)
                    rightPadding: bookManager.dp(20)
                    touchPropagationMode: TouchPropagationMode.Full

                    layout: StackLayout {
                        orientation: LayoutOrientation.TopToBottom
                    }

                    // Header: Title & Close Button
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        layout: StackLayout {
                            orientation: LayoutOrientation.LeftToRight
                        }

                        Label {
                            layoutProperties: StackLayoutProperties {
                                spaceQuota: 1.0
                            }
                            verticalAlignment: VerticalAlignment.Center
                            text: footnoteOverlayContainer.footnoteTitle
                            textStyle.base: SystemDefaults.TextStyles.TitleText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: bookManager.theme === "dark" ? Color.create("#F5EFE6") :
                                             (bookManager.theme === "sepia" ? Color.create("#3E2D1D") : Color.create("#1C1916"))
                            autoSize.maxLineCount: 1
                        }

                        ImageButton {
                            accessibility.name: qsTr("Đóng") + Retranslate.onLanguageChanged
                            verticalAlignment: VerticalAlignment.Center
                            defaultImageSource: bookManager.theme === "dark" ? "asset:///icons/ic_clear.png" :
                                                (bookManager.theme === "sepia" ? "asset:///icons/ic_clear_sepia.png" : "asset:///icons/ic_clear_dark.png")
                            pressedImageSource: bookManager.theme === "dark" ? "asset:///icons/ic_clear.png" :
                                                (bookManager.theme === "sepia" ? "asset:///icons/ic_clear_sepia.png" : "asset:///icons/ic_clear_dark.png")
                            preferredWidth: bookManager.dp(48)
                            preferredHeight: bookManager.dp(48)
                            onClicked: {
                                footnoteOverlayContainer.visible = false;
                            }
                        }
                    }

                    Divider {
                        topMargin: bookManager.dp(8)
                        bottomMargin: bookManager.dp(10)
                    }

                    // Content ScrollView
                    ScrollView {
                        horizontalAlignment: HorizontalAlignment.Fill
                        maxHeight: bookManager.dp(340)
                        scrollRole: ScrollRole.None

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            topPadding: bookManager.dp(4)
                            bottomPadding: bookManager.dp(8)

                            Label {
                                horizontalAlignment: HorizontalAlignment.Fill
                                text: footnoteOverlayContainer.footnoteBody
                                multiline: true
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.color: bookManager.theme === "dark" ? Color.create("#E6DFD3") :
                                                 (bookManager.theme === "sepia" ? Color.create("#3E2D1D") : Color.create("#2B2621"))
                            }
                        }
                    }
                }
            }

            // READING RULER OVERLAY (non-intercepting horizontal highlight band)
            Container {
                id: readingRulerOverlay
                visible: readerPage.readingRulerEnabled && !readerPage.isRendering && (!bookManager.isPdf || bookManager.isPdfReflow)
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Center
                preferredHeight: bookManager.dp(56)
                background: Color.create(0.85, 0.67, 0.38, 0.14)
                touchPropagationMode: TouchPropagationMode.PassThrough
                overlapTouchPolicy: OverlapTouchPolicy.Allow
            }

            // KEYBOARD SHORTCUTS HELP OVERLAY (?)
            Container {
                id: shortcutsOverlay
                visible: readerPage.showShortcutsHelp
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                background: Color.create(0.0, 0.0, 0.0, 0.65)
                layout: DockLayout {}
                gestureHandlers: [
                    TapHandler { onTapped: { readerPage.showShortcutsHelp = false; } }
                ]

                Container {
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    preferredWidth: bookManager.dp(560)
                    maxWidth: bookManager.dp(620)
                    background: Color.create(bookManager.uiCard)
                    topPadding: bookManager.dp(16)
                    bottomPadding: bookManager.dp(16)
                    leftPadding: bookManager.dp(18)
                    rightPadding: bookManager.dp(18)
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                    gestureHandlers: [ TapHandler { onTapped: {} } ]

                    Label {
                        text: qsTr("Phím tắt bàn phím BB10") + Retranslate.onLanguageChanged
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create(bookManager.uiAccent)
                    }
                    Label {
                        topMargin: bookManager.dp(10)
                        multiline: true
                        text: "Space / Shift+Space: Lật trang / TTS\n" +
                              "J / K hoặc N / P: Trang sau / trước, Chương sau / trước\n" +
                              "Z / W (hoặc + / -): Tăng / giảm cỡ chữ\n" +
                              "D / C / A: Đổi theme / Cuộn / Đọc TTS\n" +
                              "M / H / F / G: Đánh dấu / Ghi chú / Tìm / Mục lục\n" +
                              "L / I / S: Từ điển / Thông tin / Cài đặt\n" +
                              "R: Thước kẻ đọc (hoặc xoay PDF)\n" +
                              "U / Backspace: Quay lại vị trí trước\n" +
                              "?: Hiện / ẩn bảng phím tắt"
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create(bookManager.uiInk)
                    }
                    Button {
                        topMargin: bookManager.dp(14)
                        horizontalAlignment: HorizontalAlignment.Right
                        text: qsTr("Đóng") + Retranslate.onLanguageChanged
                        onClicked: { readerPage.showShortcutsHelp = false; }
                    }
                }
            }

            // READING LAYOUT POPUP (mode / columns / line spacing)
            Container {
                id: layoutPickerOverlay
                visible: readerPage.showLayoutPopup && readerPage.showDisplayHud && !readerPage.fullscreenMode && !ttsManager.active
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                background: Color.create(0.0, 0.0, 0.0, 0.35)
                touchPropagationMode: TouchPropagationMode.Full
                layout: DockLayout {}
                gestureHandlers: [ TapHandler { onTapped: { readerPage.showLayoutPopup = false; } } ]

                property bool isSyncingHud: false
                property string syncedReadingMode: bookManager.readingMode
                property string syncedColumnLayout: bookManager.columnLayout
                property string syncedLineSpacing: bookManager.lineSpacing

                function syncHudLayoutControls() {
                    if (!hudReadingModeControl || !hudColumnControl || !hudSpacingControl) {
                        return;
                    }
                    isSyncingHud = true;
                    hudReadingModeControl.selectedIndex = (bookManager.readingMode === "scroll" ? 1 : 0);
                    hudColumnControl.selectedIndex = (bookManager.columnLayout === "double" ? 1 : 0);
                    hudSpacingControl.selectedIndex = (bookManager.lineSpacing === "tight" ? 0 : (bookManager.lineSpacing === "loose" ? 2 : 1));
                    isSyncingHud = false;
                }

                onCreationCompleted: {
                    syncHudLayoutControls();
                }

                onVisibleChanged: {
                    if (visible) {
                        syncHudLayoutControls();
                    }
                }

                onSyncedReadingModeChanged: {
                    syncHudLayoutControls();
                }

                onSyncedColumnLayoutChanged: {
                    syncHudLayoutControls();
                }

                onSyncedLineSpacingChanged: {
                    syncHudLayoutControls();
                }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Bottom
                    leftMargin: bookManager.dp(12)
                    rightMargin: bookManager.dp(12)
                    bottomMargin: bookManager.dp(14)
                    background: Color.create(bookManager.uiCard)
                    touchPropagationMode: TouchPropagationMode.Full
                    gestureHandlers: [ TapHandler { onTapped: {} } ]
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: bookManager.dp(3)
                        background: Color.create(bookManager.uiAccent)
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Label {
                            bottomMargin: bookManager.dp(6)
                            text: qsTr("CHẾ ĐỘ ĐỌC") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }
                        SegmentedControl {
                            id: hudReadingModeControl
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: bookManager.dp(14)
                            onSelectedValueChanged: {
                                if (!layoutPickerOverlay.isSyncingHud && selectedValue && selectedValue !== bookManager.readingMode) { bookManager.setReadingMode(selectedValue); }
                            }
                            Option { text: qsTr("Lật trang") + Retranslate.onLanguageChanged; value: "paged"; selected: bookManager.readingMode === "paged" }
                            Option { text: qsTr("Cuộn") + Retranslate.onLanguageChanged; value: "scroll"; selected: bookManager.readingMode === "scroll" }
                        }
                        Label {
                            bottomMargin: bookManager.dp(6)
                            text: qsTr("SỐ CỘT") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }
                        SegmentedControl {
                            id: hudColumnControl
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: bookManager.dp(14)
                            onSelectedValueChanged: {
                                if (!layoutPickerOverlay.isSyncingHud && selectedValue && selectedValue !== bookManager.columnLayout) { bookManager.setColumnLayout(selectedValue); }
                            }
                            Option { text: qsTr("1 cột") + Retranslate.onLanguageChanged; value: "single"; selected: bookManager.columnLayout !== "double" }
                            Option { text: qsTr("2 cột") + Retranslate.onLanguageChanged; value: "double"; selected: bookManager.columnLayout === "double" }
                        }
                        Label {
                            bottomMargin: bookManager.dp(6)
                            text: qsTr("DÃN DÒNG") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }
                        SegmentedControl {
                            id: hudSpacingControl
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: 0
                            onSelectedValueChanged: {
                                if (!layoutPickerOverlay.isSyncingHud && selectedValue && selectedValue !== bookManager.lineSpacing) { bookManager.setLineSpacing(selectedValue); }
                            }
                            Option { text: qsTr("Hẹp") + Retranslate.onLanguageChanged; value: "tight"; selected: bookManager.lineSpacing === "tight" }
                            Option { text: qsTr("Vừa") + Retranslate.onLanguageChanged; value: "normal"; selected: bookManager.lineSpacing === "normal" }
                            Option { text: qsTr("Rộng") + Retranslate.onLanguageChanged; value: "loose"; selected: bookManager.lineSpacing === "loose" }
                        }
                    }
                }
            }

            // FONT SIZE POPUP (A- / size / A+)
            Container {
                id: fontPickerOverlay
                visible: readerPage.showFontPopup && readerPage.showDisplayHud && !readerPage.fullscreenMode && !ttsManager.active
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                background: Color.create(0.0, 0.0, 0.0, 0.35)
                touchPropagationMode: TouchPropagationMode.Full
                layout: DockLayout {}
                gestureHandlers: [ TapHandler { onTapped: { readerPage.showFontPopup = false; } } ]

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Bottom
                    leftMargin: bookManager.dp(12)
                    rightMargin: bookManager.dp(12)
                    bottomMargin: bookManager.dp(14)
                    background: Color.create(bookManager.uiCard)
                    touchPropagationMode: TouchPropagationMode.Full
                    gestureHandlers: [ TapHandler { onTapped: {} } ]
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: bookManager.dp(3)
                        background: Color.create(bookManager.uiAccent)
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Label {
                            bottomMargin: bookManager.dp(6)
                            text: qsTr("FONT CHỮ") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }

                        ListView {
                            id: fontListView
                            horizontalAlignment: HorizontalAlignment.Fill
                            bottomMargin: bookManager.dp(14)
                            preferredHeight: bookManager.dp(240)
                            maxHeight: bookManager.dp(240)
                            dataModel: fontListModel
                            function pickFont(v) {
                                bookManager.setFontFamily(v);
                                readerPage.applyStyleLive();
                            }
                            listItemComponents: [
                                ListItemComponent {
                                    type: "item"
                                    Container {
                                        id: fontRow
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        preferredHeight: bookManager.dp(44)
                                        leftPadding: bookManager.dp(12)
                                        rightPadding: bookManager.dp(12)
                                        background: ListItemData.value === bookManager.fontFamily ? Color.create(bookManager.uiActive) : Color.Transparent
                                        layout: DockLayout {}
                                        Label {
                                            verticalAlignment: VerticalAlignment.Center
                                            text: ListItemData.name
                                            textStyle.base: SystemDefaults.TextStyles.BodyText
                                            textStyle.fontWeight: ListItemData.value === bookManager.fontFamily ? FontWeight.W500 : FontWeight.Normal
                                            textStyle.color: ListItemData.value === bookManager.fontFamily ? Color.create(bookManager.uiAccent) :
                                                             Color.create(bookManager.uiInk)
                                            autoSize.maxLineCount: 1
                                        }
                                    }
                                }
                            ]
                            onTriggered: {
                                var d = fontListModel.data(indexPath);
                                if (d && d.value) { fontListView.pickFont(d.value); }
                            }
                        }

                        Label {
                            bottomMargin: bookManager.dp(10)
                            text: qsTr("CỠ CHỮ") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            Button {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                preferredHeight: bookManager.dp(46)
                                enabled: bookManager.fontSize > 10
                                imageSource: bookManager.appTheme === "dark" ?
                                             "asset:///icons/ic_font_dec.png" : "asset:///icons/ic_font_dec_dark.png"
                                onClicked: { readerPage.decreaseFont(); }
                            }
                            Label {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                verticalAlignment: VerticalAlignment.Center
                                horizontalAlignment: HorizontalAlignment.Center
                                textStyle.textAlign: TextAlign.Center
                                textStyle.fontWeight: FontWeight.W500
                                textStyle.color: Color.create(bookManager.uiInk)
                                text: bookManager.fontSize + " px"
                            }
                            Button {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                preferredHeight: bookManager.dp(46)
                                enabled: bookManager.fontSize < 36
                                imageSource: bookManager.appTheme === "dark" ?
                                             "asset:///icons/ic_font_inc.png" : "asset:///icons/ic_font_inc_dark.png"
                                onClicked: { readerPage.increaseFont(); }
                            }
                        }
                    }
                }
            }

            // =========================================================================
            // 3-MODE READING THEME POPUP (Opened from Quick HUD Theme Button)
            // =========================================================================
            Container {
                id: themePickerOverlay
                visible: readerPage.showThemePopup && readerPage.showDisplayHud && !readerPage.fullscreenMode && !ttsManager.active
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                background: Color.create(0.0, 0.0, 0.0, 0.35)
                touchPropagationMode: TouchPropagationMode.Full
                layout: DockLayout {}

                gestureHandlers: [
                    TapHandler {
                        onTapped: {
                            readerPage.showThemePopup = false;
                        }
                    }
                ]

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Bottom
                    leftMargin: bookManager.dp(12)
                    rightMargin: bookManager.dp(12)
                    bottomMargin: bookManager.dp(14)
                    background: Color.create(bookManager.uiCard)
                    touchPropagationMode: TouchPropagationMode.Full
                    gestureHandlers: [ TapHandler { onTapped: {} } ]
                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        preferredHeight: bookManager.dp(3)
                        background: Color.create(bookManager.uiAccent)
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        topPadding: bookManager.dp(14)
                        bottomPadding: bookManager.dp(14)
                        leftPadding: bookManager.dp(14)
                        rightPadding: bookManager.dp(14)
                        layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                        Label {
                            bottomMargin: bookManager.dp(10)
                            text: qsTr("MÀU NỀN ĐỌC") + Retranslate.onLanguageChanged
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.fontWeight: FontWeight.W500
                            textStyle.color: Color.create(bookManager.uiMuted)
                        }

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                            // Option 1: Dark (AMOLED)
                            Container {
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                background: bookManager.theme === "dark" ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiHairline)
                                topPadding: bookManager.dp(2)
                                bottomPadding: bookManager.dp(2)
                                leftPadding: bookManager.dp(2)
                                rightPadding: bookManager.dp(2)
                                gestureHandlers: [
                                    TapHandler {
                                        onTapped: {
                                            bookManager.setTheme("dark");
                                            readerPage.showThemePopup = false;
                                        }
                                    }
                                ]
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    background: Color.create("#0E0C0A")
                                    topPadding: bookManager.dp(12)
                                    bottomPadding: bookManager.dp(12)
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                    ImageView {
                                        horizontalAlignment: HorizontalAlignment.Center
                                        preferredWidth: bookManager.dp(36)
                                        preferredHeight: bookManager.dp(36)
                                        scalingMethod: ScalingMethod.AspectFit
                                        imageSource: "asset:///icons/ic_mode_dark.png"
                                    }
                                    Label {
                                        topMargin: bookManager.dp(6)
                                        horizontalAlignment: HorizontalAlignment.Center
                                        text: qsTr("Tối") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.W500
                                        textStyle.color: bookManager.theme === "dark" ? Color.create("#D9AB60") : Color.create("#F5EFE6")
                                    }
                                }
                            }

                            // Option 2: Sepia
                            Container {
                                leftMargin: bookManager.dp(8)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                background: bookManager.theme === "sepia" ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiHairline)
                                topPadding: bookManager.dp(2)
                                bottomPadding: bookManager.dp(2)
                                leftPadding: bookManager.dp(2)
                                rightPadding: bookManager.dp(2)
                                gestureHandlers: [
                                    TapHandler {
                                        onTapped: {
                                            bookManager.setTheme("sepia");
                                            readerPage.showThemePopup = false;
                                        }
                                    }
                                ]
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    background: Color.create("#F4ECD8")
                                    topPadding: bookManager.dp(12)
                                    bottomPadding: bookManager.dp(12)
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                    ImageView {
                                        horizontalAlignment: HorizontalAlignment.Center
                                        preferredWidth: bookManager.dp(36)
                                        preferredHeight: bookManager.dp(36)
                                        scalingMethod: ScalingMethod.AspectFit
                                        imageSource: "asset:///icons/ic_mode_sepia_dark.png"
                                    }
                                    Label {
                                        topMargin: bookManager.dp(6)
                                        horizontalAlignment: HorizontalAlignment.Center
                                        text: qsTr("Sepia") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.W500
                                        textStyle.color: Color.create("#3E2D1D")
                                    }
                                }
                            }

                            // Option 3: Light
                            Container {
                                leftMargin: bookManager.dp(8)
                                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                background: bookManager.theme === "light" ? Color.create(bookManager.uiAccent) : Color.create(bookManager.uiHairline)
                                topPadding: bookManager.dp(2)
                                bottomPadding: bookManager.dp(2)
                                leftPadding: bookManager.dp(2)
                                rightPadding: bookManager.dp(2)
                                gestureHandlers: [
                                    TapHandler {
                                        onTapped: {
                                            bookManager.setTheme("light");
                                            readerPage.showThemePopup = false;
                                        }
                                    }
                                ]
                                Container {
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    background: Color.create("#FFFFFF")
                                    topPadding: bookManager.dp(12)
                                    bottomPadding: bookManager.dp(12)
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }
                                    ImageView {
                                        horizontalAlignment: HorizontalAlignment.Center
                                        preferredWidth: bookManager.dp(36)
                                        preferredHeight: bookManager.dp(36)
                                        scalingMethod: ScalingMethod.AspectFit
                                        imageSource: "asset:///icons/ic_mode_light_dark.png"
                                    }
                                    Label {
                                        topMargin: bookManager.dp(6)
                                        horizontalAlignment: HorizontalAlignment.Center
                                        text: qsTr("Sáng") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.W500
                                        textStyle.color: Color.create("#1C1916")
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- TTS Voice Reader Floating Controller Panel ---
        Container {
            id: ttsControlPanel
            visible: ttsManager.active
            horizontalAlignment: HorizontalAlignment.Fill
            background: bookManager.theme === "dark" ? Color.create("#181512") :
                        (bookManager.theme === "sepia" ? Color.create("#EADEC3") : Color.create("#F4EFE6"))
            topPadding: bookManager.dp(8)
            bottomPadding: bookManager.dp(8)
            leftPadding: bookManager.dp(12)
            rightPadding: bookManager.dp(12)

            layout: StackLayout {
                orientation: LayoutOrientation.TopToBottom
            }

            // Row 1: Sentence status + Settings + Close
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }

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
                              (ttsManager.currentSentenceText !== "" ? ("\"" + ttsManager.currentSentenceText + "\"") : qsTr("Libris TTS"))
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: ttsManager.loading ? Color.create(bookManager.uiAccent) :
                                        (bookManager.theme === "dark" ? Color.create("#F5EFE6") :
                                         (bookManager.theme === "sepia" ? Color.create("#3E2D1D") : Color.create("#1C1916")))
                        textStyle.fontStyle: FontStyle.Italic
                        autoSize.maxLineCount: 1
                    }

                    Label {
                        topMargin: bookManager.dp(2)
                        text: (ttsManager.ttsProvider === "gemini" ?
                               (ttsManager.geminiVoice + " • " + ttsManager.geminiModel) :
                               ttsManager.ttsProvider === "espeak" ? ttsManager.voiceDisplayName :
                               (ttsManager.voiceDisplayName !== "" ? ttsManager.voiceDisplayName : (bookManager.language === "en" ? "Google Translate" : "Google Dịch"))) +
                              " (" + (ttsManager.currentSentenceIndex + 1) + "/" + ttsManager.totalSentences + ")" +
                              (ttsManager.isSleepTimerActive ? (" • ⏱ " + ttsManager.sleepTimerDisplayStr) : "")
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: bookManager.theme === "dark" ? Color.create("#9C9488") :
                                         (bookManager.theme === "sepia" ? Color.create("#665544") : Color.create("#635B51"))
                        autoSize.maxLineCount: 1
                    }
                }

                ImageButton {
                    accessibility.name: qsTr("Cài đặt") + Retranslate.onLanguageChanged
                    verticalAlignment: VerticalAlignment.Center
                    defaultImageSource: bookManager.theme === "dark" ? "asset:///icons/ic_settings.png" :
                                        (bookManager.theme === "sepia" ? "asset:///icons/ic_settings_sepia.png" : "asset:///icons/ic_settings_dark.png")
                    pressedImageSource: bookManager.theme === "dark" ? "asset:///icons/ic_settings.png" :
                                        (bookManager.theme === "sepia" ? "asset:///icons/ic_settings_sepia.png" : "asset:///icons/ic_settings_dark.png")
                    preferredWidth: bookManager.dp(48)
                    preferredHeight: bookManager.dp(48)
                    leftMargin: bookManager.dp(8)
                    onClicked: {
                        settingsSheet.openToTts();
                    }
                }

                Button {
                    verticalAlignment: VerticalAlignment.Center
                    text: qsTr("Đóng") + Retranslate.onLanguageChanged
                    preferredHeight: bookManager.dp(38)
                    leftMargin: bookManager.dp(6)
                    onClicked: {
                        ttsManager.stop();
                    }
                }
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: 1
                topMargin: bookManager.dp(6)
                bottomMargin: bookManager.dp(6)
                background: Color.create(bookManager.uiHairline)
            }

            // Row 2: Playback transport (Prev, single Play/Pause, Next, Sleep Timer)
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                ImageButton {
                    accessibility.name: qsTr("Trước") + Retranslate.onLanguageChanged
                    verticalAlignment: VerticalAlignment.Center
                    defaultImageSource: bookManager.theme === "dark" ? "asset:///icons/ic_previous.png" :
                                        (bookManager.theme === "sepia" ? "asset:///icons/ic_previous_sepia.png" : "asset:///icons/ic_previous_dark.png")
                    pressedImageSource: bookManager.theme === "dark" ? "asset:///icons/ic_previous.png" :
                                        (bookManager.theme === "sepia" ? "asset:///icons/ic_previous_sepia.png" : "asset:///icons/ic_previous_dark.png")
                    preferredWidth: bookManager.dp(48)
                    preferredHeight: bookManager.dp(48)
                    onClicked: {
                        ttsManager.prevSentence();
                    }
                }

                Button {
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.3 }
                    verticalAlignment: VerticalAlignment.Center
                    leftMargin: bookManager.dp(6)
                    preferredHeight: bookManager.dp(40)
                    text: (ttsManager.playing ? qsTr("Tạm dừng") : qsTr("Phát tiếp")) + Retranslate.onLanguageChanged
                    appearance: ControlAppearance.Primary
                    onClicked: {
                        ttsManager.togglePlayPause();
                    }
                }

                ImageButton {
                    accessibility.name: qsTr("Tiếp") + Retranslate.onLanguageChanged
                    verticalAlignment: VerticalAlignment.Center
                    defaultImageSource: bookManager.theme === "dark" ? "asset:///icons/ic_next.png" :
                                        (bookManager.theme === "sepia" ? "asset:///icons/ic_next_sepia.png" : "asset:///icons/ic_next_dark.png")
                    pressedImageSource: bookManager.theme === "dark" ? "asset:///icons/ic_next.png" :
                                        (bookManager.theme === "sepia" ? "asset:///icons/ic_next_sepia.png" : "asset:///icons/ic_next_dark.png")
                    preferredWidth: bookManager.dp(48)
                    preferredHeight: bookManager.dp(48)
                    leftMargin: bookManager.dp(6)
                    onClicked: {
                        ttsManager.nextSentence();
                    }
                }

                Button {
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    verticalAlignment: VerticalAlignment.Center
                    leftMargin: bookManager.dp(8)
                    preferredHeight: bookManager.dp(40)
                    text: ttsManager.isSleepTimerActive ? ("⏱ " + ttsManager.sleepTimerDisplayStr) : (qsTr("Hẹn giờ") + Retranslate.onLanguageChanged)
                    onClicked: {
                        ttsManager.cycleSleepTimer();
                        readerToast.body = ttsManager.sleepAtChapterEnd ?
                                           (qsTr("Hẹn giờ: Dừng khi hết chương") + Retranslate.onLanguageChanged) :
                                           (ttsManager.isSleepTimerActive ?
                                            (qsTr("Hẹn giờ tắt: ") + ttsManager.sleepTimerMinutes + qsTr(" phút") + Retranslate.onLanguageChanged) :
                                            (qsTr("Đã tắt hẹn giờ") + Retranslate.onLanguageChanged));
                        readerToast.show();
                    }
                }
            }

            // Row 3: Quick TTS settings (Speed, Source, Mode, Voice)
            Container {
                topMargin: bookManager.dp(6)
                horizontalAlignment: HorizontalAlignment.Fill
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                Button {
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    verticalAlignment: VerticalAlignment.Center
                    preferredHeight: bookManager.dp(40)
                    text: (qsTr("Tốc độ") + ": " + ttsManager.speed.toFixed(2).replace(/0$/, "") + "x") + Retranslate.onLanguageChanged
                    onClicked: {
                        var steps = [0.75, 1.0, 1.25, 1.5, 1.75, 2.0];
                        var cur = ttsManager.speed;
                        var next = steps[0];
                        for (var i = 0; i < steps.length; ++i) {
                            if (steps[i] > cur + 0.01) { next = steps[i]; break; }
                        }
                        ttsManager.setSpeed(next);
                    }
                }

                Button {
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    verticalAlignment: VerticalAlignment.Center
                    leftMargin: bookManager.dp(6)
                    preferredHeight: bookManager.dp(40)
                    text: (qsTr("Nguồn") + ": " + (ttsManager.ttsProvider === "gemini" ? "Gemini" : (ttsManager.ttsProvider === "espeak" ? "eSpeak" : "Google"))) + Retranslate.onLanguageChanged
                    onClicked: {
                        var nxt = ttsManager.ttsProvider === "google" ? "gemini" : (ttsManager.ttsProvider === "gemini" ? "espeak" : "google");
                        ttsManager.setTtsProvider(nxt);
                    }
                }

                Button {
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    verticalAlignment: VerticalAlignment.Center
                    leftMargin: bookManager.dp(6)
                    preferredHeight: bookManager.dp(40)
                    text: (ttsManager.ttsMode === "offline" ? qsTr("Ngoại tuyến") : qsTr("Trực tuyến")) + Retranslate.onLanguageChanged
                    onClicked: {
                        ttsManager.toggleMode();
                        readerToast.body = (ttsManager.ttsMode === "offline" ?
                                            qsTr("Chế độ ngoại tuyến: đọc âm thanh đã tải hoặc eSpeak-NG") :
                                            qsTr("Chế độ trực tuyến")) + Retranslate.onLanguageChanged;
                        readerToast.show();
                    }
                }

                Button {
                    visible: ttsManager.ttsProvider === "espeak"
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    verticalAlignment: VerticalAlignment.Center
                    leftMargin: bookManager.dp(6)
                    preferredHeight: bookManager.dp(40)
                    text: (qsTr("Giọng") + ": " + ttsManager.espeakVariant) + Retranslate.onLanguageChanged
                    onClicked: {
                        var v = ["std", "f3", "f4", "m1", "m3", "klatt", "Nguyen"];
                        var i = v.indexOf(ttsManager.espeakVariant);
                        ttsManager.setEspeakVariant(v[(i + 1) % v.length]);
                    }
                }
            }
        }

        // In-Page Search Floating Navigation Bar
        Container {
            id: inPageSearchControlPanel
            visible: searchSheet.inPageTotal > 0 && !ttsManager.active && !readerPage.fullscreenMode && !readerPage.showDisplayHud && !readerPage.showLayoutPopup && !readerPage.showFontPopup && !readerPage.showThemePopup
            horizontalAlignment: HorizontalAlignment.Fill
            background: bookManager.theme === "dark" ? Color.create("#181512") :
                        (bookManager.theme === "sepia" ? Color.create("#EADEC3") : Color.create("#F4EFE6"))
            topPadding: bookManager.dp(8)
            bottomPadding: bookManager.dp(8)
            leftPadding: bookManager.dp(12)
            rightPadding: bookManager.dp(12)

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            ImageView {
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: bookManager.dp(24)
                preferredHeight: bookManager.dp(24)
                scalingMethod: ScalingMethod.AspectFit
                imageSource: bookManager.theme === "dark" ? "asset:///icons/ic_search.png" :
                             (bookManager.theme === "sepia" ? "asset:///icons/ic_search_sepia.png" : "asset:///icons/ic_search_dark.png")
            }

            Label {
                leftMargin: bookManager.dp(8)
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                text: searchSheet.activeSearchTerm + " (" + searchSheet.inPageCurrent + "/" + searchSheet.inPageTotal + ")"
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.fontWeight: FontWeight.W500
                textStyle.color: bookManager.theme === "dark" ? Color.create("#F5EFE6") :
                                 (bookManager.theme === "sepia" ? Color.create("#3E2D1D") : Color.create("#1C1916"))
                autoSize.maxLineCount: 1
            }

            Button {
                verticalAlignment: VerticalAlignment.Center
                text: qsTr("Trước") + Retranslate.onLanguageChanged
                preferredHeight: bookManager.dp(40)
                leftMargin: bookManager.dp(6)
                onClicked: {
                    searchSheet.inPagePrev();
                }
            }

            Button {
                verticalAlignment: VerticalAlignment.Center
                text: qsTr("Tiếp") + Retranslate.onLanguageChanged
                preferredHeight: bookManager.dp(40)
                leftMargin: bookManager.dp(6)
                onClicked: {
                    searchSheet.inPageNext();
                }
            }

            Button {
                verticalAlignment: VerticalAlignment.Center
                text: qsTr("Đóng") + Retranslate.onLanguageChanged
                preferredHeight: bookManager.dp(40)
                leftMargin: bookManager.dp(6)
                onClicked: {
                    searchSheet.inPageClear();
                    searchSheet.inPageTotal = 0;
                    searchSheet.inPageCurrent = 0;
                }
            }
        }

        // Dedicated reading progress bar across the screen (fills up according to readingProgress)
        Container {
            visible: bookManager.showFooterProgress && bookManager.showFooterProgressBar
            horizontalAlignment: HorizontalAlignment.Fill
            preferredHeight: 3
            minHeight: 3
            maxHeight: 3
            background: bookManager.theme == "dark" ? Color.create("#1F1B16") : 
                        (bookManager.theme == "sepia" ? Color.create("#E2D7C2") : Color.create("#E6DFD3"))

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            Container {
                verticalAlignment: VerticalAlignment.Fill
                background: bookManager.readingProgress >= 100 ? Color.create("#E6C07B") : Color.create("#D4A359")
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
            preferredHeight: bookManager.dp(38)
            minHeight: bookManager.dp(36)
            maxHeight: bookManager.dp(48)
            background: bookManager.theme == "dark" ? Color.create("#000000") : 
                        (bookManager.theme == "sepia" ? Color.create("#F4ECD8") : Color.create("#FFFFFF"))
            leftPadding: bookManager.marginLeft
            rightPadding: bookManager.marginRight

            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }

            Label {
                visible: bookManager.showFooterChapter
                rightMargin: 12
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
                       ((qsTr("Trang ") + Retranslate.onLanguageChanged) + bookManager.currentPageInChapter + " / " + bookManager.totalPagesInChapter + " • " + bookManager.readingProgress + "%" + (bookManager.showFooterTimeLeft && bookManager.totalPagesInChapter > bookManager.currentPageInChapter ? (" • ~" + Math.max(1, Math.round((bookManager.totalPagesInChapter - bookManager.currentPageInChapter) * 0.8)) + "m") : "")))
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: bookManager.readingProgress >= 100 ? (bookManager.theme == "dark" ? Color.create("#D9AB60") : Color.create("#8A5D16")) : (bookManager.theme == "dark" ? Color.White : (bookManager.theme == "sepia" ? Color.create("#403020") : Color.Black))
            }

            ImageView {
                visible: bookManager.showFooterBattery
                leftMargin: bookManager.dp(12)
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: bookManager.dp(20)
                preferredHeight: bookManager.dp(20)
                imageSource: bookManager.theme == "dark" ? "asset:///icons/ic_bat.png" : "asset:///icons/ic_bat_dark.png"
            }

            Label {
                visible: bookManager.showFooterBattery
                leftMargin: 4
                verticalAlignment: VerticalAlignment.Center
                text: bookManager.batteryLevel + "%"
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: bookManager.theme == "dark" ? Color.create("#9C9488") : Color.create("#5C554C")
            }

            Label {
                visible: bookManager.showFooterClock
                leftMargin: 12
                verticalAlignment: VerticalAlignment.Center
                text: bookManager.currentTime
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.color: bookManager.theme == "dark" ? Color.create("#9C9488") : Color.create("#5C554C")
            }
        }

        // =====================================================================
        // QUICK Aa APPEARANCE HUD DRAWER (Text / Reflow & Native PDF)
        // =====================================================================
        Container {
            id: quickDisplayHud
            visible: !readerPage.fullscreenMode && readerPage.showDisplayHud && !ttsManager.active
            horizontalAlignment: HorizontalAlignment.Fill
            background: Color.create(bookManager.uiAccent)
            topPadding: bookManager.dp(2)

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(bookManager.uiCard)
                topPadding: bookManager.dp(8)
                bottomPadding: bookManager.dp(8)
                leftPadding: bookManager.dp(10)
                rightPadding: bookManager.dp(10)
                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                // 1. Text / EPUB / PDF Reflow HUD (Single Sleek Row of Native Buttons)
                Container {
                    visible: !bookManager.isPdf || bookManager.isPdfReflow
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    // 1. Unified 3-Mode Theme Popup Trigger Button
                    Button {
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        preferredHeight: bookManager.dp(44)
                        appearance: readerPage.showThemePopup ? ControlAppearance.Primary : ControlAppearance.Default
                        imageSource: (readerPage.showThemePopup || bookManager.appTheme === "light") ?
                                     "asset:///icons/ic_palette_dark.png" : "asset:///icons/ic_palette.png"
                        onClicked: {
                            readerPage.showFontPopup = false;
                            readerPage.showLayoutPopup = false;
                            readerPage.showThemePopup = !readerPage.showThemePopup;
                        }
                    }

                    // 2. Typography Popup Trigger (font family + size)
                    Button {
                        leftMargin: bookManager.dp(6)
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        preferredHeight: bookManager.dp(44)
                        appearance: readerPage.showFontPopup ? ControlAppearance.Primary : ControlAppearance.Default
                        imageSource: (readerPage.showFontPopup || bookManager.appTheme === "light") ?
                                     "asset:///icons/ic_typo_dark.png" : "asset:///icons/ic_typo.png"
                        onClicked: {
                            readerPage.showThemePopup = false;
                            readerPage.showLayoutPopup = false;
                            readerPage.showFontPopup = !readerPage.showFontPopup;
                        }
                    }

                    // 3. Reading Layout Popup Trigger (mode / columns / line spacing)
                    Button {
                        leftMargin: bookManager.dp(6)
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        preferredHeight: bookManager.dp(44)
                        appearance: readerPage.showLayoutPopup ? ControlAppearance.Primary : ControlAppearance.Default
                        imageSource: (readerPage.showLayoutPopup || bookManager.appTheme === "light") ?
                                     "asset:///icons/ic_layout_dark.png" : "asset:///icons/ic_layout.png"
                        onClicked: {
                            readerPage.showThemePopup = false;
                            readerPage.showFontPopup = false;
                            readerPage.showLayoutPopup = !readerPage.showLayoutPopup;
                        }
                    }

                    // 4. Warm Night Filter Icon
                    Button {
                        leftMargin: bookManager.dp(6)
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        preferredHeight: bookManager.dp(44)
                        appearance: bookManager.warmFilterEnabled ? ControlAppearance.Primary : ControlAppearance.Default
                        imageSource: (bookManager.warmFilterEnabled || bookManager.appTheme === "light") ?
                                     "asset:///icons/ic_warm_sun_dark.png" : "asset:///icons/ic_warm_sun.png"
                        onClicked: { bookManager.toggleWarmFilter(); }
                    }

                    // 5. Full Settings Icon
                    Button {
                        leftMargin: bookManager.dp(6)
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        preferredHeight: bookManager.dp(44)
                        imageSource: bookManager.appTheme === "dark" ?
                                     "asset:///icons/ic_settings.png" : "asset:///icons/ic_settings_dark.png"
                        onClicked: {
                            readerPage.showDisplayHud = false;
                            settingsSheet.open();
                        }
                    }
                }

                // 2. Native PDF HUD Controls (Single Compact Row of Native Icon Buttons)
                Container {
                    visible: bookManager.isPdf && !bookManager.isPdfReflow
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    Button {
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        preferredHeight: bookManager.dp(44)
                        appearance: bookManager.isPdfInverted ? ControlAppearance.Primary : ControlAppearance.Default
                        imageSource: (bookManager.isPdfInverted || bookManager.appTheme === "light") ?
                                     "asset:///icons/ic_theme_dark.png" : "asset:///icons/ic_theme.png"
                        onClicked: { bookManager.togglePdfInvert(); }
                    }

                    Button {
                        leftMargin: bookManager.dp(6)
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        preferredHeight: bookManager.dp(44)
                        appearance: bookManager.isPdfAutoCrop ? ControlAppearance.Primary : ControlAppearance.Default
                        imageSource: (bookManager.isPdfAutoCrop || bookManager.appTheme === "light") ?
                                     "asset:///icons/ic_crop_dark.png" : "asset:///icons/ic_crop.png"
                        onClicked: {
                            bookManager.togglePdfAutoCrop();
                            readerToast.body = bookManager.isPdfAutoCrop ?
                                               (qsTr("Đã bật cắt lề trắng PDF") + Retranslate.onLanguageChanged) :
                                               (qsTr("Đã tắt cắt lề trắng PDF") + Retranslate.onLanguageChanged);
                            readerToast.show();
                        }
                    }

                    Button {
                        leftMargin: bookManager.dp(6)
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        preferredHeight: bookManager.dp(44)
                        imageSource: bookManager.appTheme === "dark" ?
                                     "asset:///icons/ic_rotate.png" : "asset:///icons/ic_rotate_dark.png"
                        onClicked: { bookManager.rotatePdf(); }
                    }

                    Button {
                        leftMargin: bookManager.dp(6)
                        visible: bookManager.hasPdfText
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        preferredHeight: bookManager.dp(44)
                        imageSource: bookManager.appTheme === "dark" ?
                                     "asset:///icons/ic_text_mode.png" : "asset:///icons/ic_text_mode_dark.png"
                        onClicked: {
                            readerPage.showDisplayHud = false;
                            bookManager.togglePdfReflow();
                        }
                    }

                    Button {
                        leftMargin: bookManager.dp(6)
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        preferredHeight: bookManager.dp(44)
                        appearance: bookManager.warmFilterEnabled ? ControlAppearance.Primary : ControlAppearance.Default
                        imageSource: (bookManager.warmFilterEnabled || bookManager.appTheme === "light") ?
                                     "asset:///icons/ic_warm_sun_dark.png" : "asset:///icons/ic_warm_sun.png"
                        onClicked: { bookManager.toggleWarmFilter(); }
                    }

                    Button {
                        leftMargin: bookManager.dp(6)
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        preferredHeight: bookManager.dp(44)
                        imageSource: bookManager.appTheme === "dark" ?
                                     "asset:///icons/ic_settings.png" : "asset:///icons/ic_settings_dark.png"
                        onClicked: {
                            readerPage.showDisplayHud = false;
                            settingsSheet.open();
                        }
                    }
                }
            }
        }

        // =====================================================================
        // HANDS-FREE AUTO-SCROLL TELEPROMPTER CONTROL BAR
        // =====================================================================
        Container {
            id: autoScrollControlPanel
            visible: readerPage.autoScrollActive && !ttsManager.active && !readerPage.showDisplayHud && !readerPage.showLayoutPopup && !readerPage.showFontPopup && !readerPage.showThemePopup
            horizontalAlignment: HorizontalAlignment.Fill
            background: Color.create(bookManager.uiAccent)
            topPadding: bookManager.dp(2)

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: bookManager.theme === "dark" ? Color.create("#181512") :
                            (bookManager.theme === "sepia" ? Color.create("#EADEC3") : Color.create("#F4EFE6"))
                topPadding: bookManager.dp(6)
                bottomPadding: bookManager.dp(6)
                leftPadding: bookManager.dp(12)
                rightPadding: bookManager.dp(12)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                Label {
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    text: (qsTr("Tự cuộn") + " • " + readerPage.autoScrollSpeed + "x") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.fontWeight: FontWeight.W500
                    textStyle.color: bookManager.theme === "dark" ? Color.create("#D9AB60") :
                                     (bookManager.theme === "sepia" ? Color.create("#3E2D1D") : Color.create("#1C1916"))
                }

                Button {
                    text: "−"
                    preferredWidth: bookManager.dp(44)
                    preferredHeight: bookManager.dp(38)
                    verticalAlignment: VerticalAlignment.Center
                    enabled: readerPage.autoScrollSpeed > 1
                    onClicked: {
                        readerPage.autoScrollSpeed = Math.max(1, readerPage.autoScrollSpeed - 1);
                    }
                }

                Button {
                    leftMargin: bookManager.dp(6)
                    text: "+"
                    preferredWidth: bookManager.dp(44)
                    preferredHeight: bookManager.dp(38)
                    verticalAlignment: VerticalAlignment.Center
                    enabled: readerPage.autoScrollSpeed < 5
                    onClicked: {
                        readerPage.autoScrollSpeed = Math.min(5, readerPage.autoScrollSpeed + 1);
                    }
                }

                Button {
                    leftMargin: bookManager.dp(8)
                    text: qsTr("Dừng") + Retranslate.onLanguageChanged
                    preferredHeight: bookManager.dp(38)
                    verticalAlignment: VerticalAlignment.Center
                    onClicked: {
                        readerPage.autoScrollActive = false;
                        autoScrollTimer.stop();
                    }
                }
            }
        }

        // =====================================================================
        // COMPACT PAGE / CHAPTER SCRUBBER BAR (Visible with chrome)
        // =====================================================================
        Container {
            id: scrubberBar
            visible: !readerPage.fullscreenMode && !readerPage.showDisplayHud && !ttsManager.active && searchSheet.inPageTotal === 0
            horizontalAlignment: HorizontalAlignment.Fill
            background: bookManager.theme === "dark" ? Color.create("#181512") :
                        (bookManager.theme === "sepia" ? Color.create("#EADEC3") : Color.create("#F4EFE6"))
            topPadding: bookManager.dp(4)
            bottomPadding: bookManager.dp(4)
            leftPadding: bookManager.dp(10)
            rightPadding: bookManager.dp(10)
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

            ImageButton {
                accessibility.name: qsTr("Chương trước") + Retranslate.onLanguageChanged
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: bookManager.dp(48)
                preferredHeight: bookManager.dp(48)
                defaultImageSource: bookManager.theme === "dark" ? "asset:///icons/ic_previous.png" :
                                    (bookManager.theme === "sepia" ? "asset:///icons/ic_previous_sepia.png" : "asset:///icons/ic_previous_dark.png")
                pressedImageSource: bookManager.theme === "dark" ? "asset:///icons/ic_previous.png" :
                                    (bookManager.theme === "sepia" ? "asset:///icons/ic_previous_sepia.png" : "asset:///icons/ic_previous_dark.png")
                onClicked: { bookManager.prevChapter(); }
            }

            Slider {
                id: pageScrubberSlider
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                fromValue: 1
                toValue: 2
                value: 1
                onImmediateValueChanged: {
                    if (readerPage.isScrubSyncing) return;
                    var target = Math.max(1, Math.round(immediateValue));
                    scrubReadoutLabel.text = (bookManager.isPdf && !bookManager.isPdfReflow) ?
                                             (target + "/" + Math.max(1, bookManager.totalChapters)) :
                                             (target + "/" + Math.max(1, bookManager.totalPagesInChapter));
                }
                onValueChanged: {
                    if (readerPage.isScrubSyncing) return;
                    readerPage.pushJumpHistory();
                    var target = Math.max(1, Math.round(value));
                    if (bookManager.isPdf && !bookManager.isPdfReflow) {
                        if (target !== (bookManager.currentChapterIndex + 1)) {
                            bookManager.goToChapter(target);
                        }
                    } else {
                        var tot = Math.max(1, bookManager.totalPagesInChapter);
                        var pct = (tot > 1) ? ((target - 1) / (tot - 1)) : 0.0;
                        webView.evaluateJavaScript("if (typeof readingMode !== 'undefined' && readingMode === 'scroll') { var vp = document.getElementById('reader-viewport'); if (vp) { var maxS = Math.max(0, vp.scrollHeight - vp.clientHeight); vp.scrollTop = Math.round(maxS * " + pct + "); updatePageInfo(); } } else if (typeof goToPage === 'function') { goToPage(" + target + "); }");
                    }
                }
            }

            Label {
                id: scrubReadoutLabel
                leftMargin: bookManager.dp(6)
                rightMargin: bookManager.dp(6)
                verticalAlignment: VerticalAlignment.Center
                text: (bookManager.isPdf && !bookManager.isPdfReflow) ?
                      ((bookManager.currentChapterIndex + 1) + "/" + Math.max(1, bookManager.totalChapters)) :
                      (bookManager.currentPageInChapter + "/" + Math.max(1, bookManager.totalPagesInChapter))
                textStyle.base: SystemDefaults.TextStyles.SmallText
                textStyle.fontWeight: FontWeight.W500
                textStyle.color: bookManager.theme === "dark" ? Color.create("#D9AB60") :
                                 (bookManager.theme === "sepia" ? Color.create("#3E2D1D") : Color.create("#1C1916"))
            }

            ImageButton {
                accessibility.name: qsTr("Chương sau") + Retranslate.onLanguageChanged
                verticalAlignment: VerticalAlignment.Center
                preferredWidth: bookManager.dp(48)
                preferredHeight: bookManager.dp(48)
                defaultImageSource: bookManager.theme === "dark" ? "asset:///icons/ic_next.png" :
                                    (bookManager.theme === "sepia" ? "asset:///icons/ic_next_sepia.png" : "asset:///icons/ic_next_dark.png")
                pressedImageSource: bookManager.theme === "dark" ? "asset:///icons/ic_next.png" :
                                    (bookManager.theme === "sepia" ? "asset:///icons/ic_next_sepia.png" : "asset:///icons/ic_next_dark.png")
                onClicked: { bookManager.nextChapter(); }
            }
        }

    }

    function applyStyleLive() {
        if (bookManager.isPdf && !bookManager.isPdfReflow) return;
        var darkFg = bookManager.darkTextTone === "dim" ? "#8E887D" : (bookManager.darkTextTone === "soft" ? "#B8B2A6" : "#DDDDDD");
        var bg = bookManager.theme === "dark" ? "#000000" : (bookManager.theme === "sepia" ? "#F4ECD8" : "#FFFFFF");
        var fg = bookManager.theme === "dark" ? darkFg : (bookManager.theme === "sepia" ? "#3E2D1D" : "#111111");
        var fs = bookManager.fontSize;
        var fontFaceCss = bookManager.currentFontFaceCss();
        var fontCss = bookManager.currentFontCss();
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
        var lh = bookManager.lineSpacing === "tight" ? 1.35 : (bookManager.lineSpacing === "loose" ? 2.0 : 1.65);
        var pTop = bookManager.marginTop;
        var pBottom = bookManager.marginBottom;
        var pLeft = bookManager.marginLeft;
        var pRight = bookManager.marginRight;
        var pIndent = bookManager.paragraphIndent === "none" ? "0" : (bookManager.paragraphIndent === "deep" ? "2.2em" : "1.5em");
        var pSpace = bookManager.paragraphSpacing === "compact" ? "0.25em 0" : (bookManager.paragraphSpacing === "wide" ? "1.0em 0" : "0.5em 0");
        var bodyCss = "body { background-color: " + bg + " !important; color: " + fg + " !important; font-size: " + fs + "px; font-weight: " + bookManager.readerFontWeight + "; line-height: " + lh + "; " + fontCss + "} " +
                      "p:not([class]) { text-indent: " + pIndent + "; margin: " + pSpace + "; } " +
                      "p, li { text-align: " + (bookManager.textAlign === "left" ? "left" : "justify") + " !important; } ";
        var js = "(function() { " +
                 "  var styleEl = document.getElementById('dynamic-theme-style'); " +
                 "  if (!styleEl) { " +
                 "    styleEl = document.createElement('style'); " +
                 "    styleEl.id = 'dynamic-theme-style'; " +
                 "    document.head.appendChild(styleEl); " +
                 "  } " +
                 "  styleEl.innerHTML = " + JSON.stringify(fontFaceCss + bodyCss + extra) + "; ";
        // Also call applyBionicLive at end of applyStyleLive
                 "  var vp = document.getElementById('reader-viewport'); " +
                 "  if (vp) { " +
                 "    vp.style.padding = '" + pTop + "px " + pRight + "px " + pBottom + "px " + pLeft + "px'; " +
                 "    if (typeof readingMode === 'undefined' || readingMode === 'paged') { " +
                 "      var w = window.innerWidth || document.documentElement.clientWidth || (vp ? vp.clientWidth : 0) || 720; " +
                 "      var gap = " + (pLeft + pRight) + "; " +
                 "      var twoCol = (typeof isTwoColMode !== 'undefined' && isTwoColMode); " +
                 "      var colW = twoCol ? Math.max(100, Math.floor((w - gap - gap) / 2)) : Math.max(100, w - gap); " +
                 "      vp.style.webkitColumnWidth = colW + 'px'; " +
                 "      vp.style.columnWidth = colW + 'px'; " +
                 "      vp.style.webkitColumnGap = gap + 'px'; " +
                 "      vp.style.columnGap = gap + 'px'; " +
                 "      vp.dataset.colConfigured = '1'; " +
                 "      vp.dataset.lastWidth = w; " +
                 "    } " +
                 "  } " +
                 "  if (typeof updatePageInfo === 'function') updatePageInfo(); " +
                 "})();";
        webView.evaluateJavaScript(js);
        applyBionicLive();
    }

    function applyBionicLive() {
        if (bookManager.isPdf && !bookManager.isPdfReflow) return;
        var enable = bookManager.bionicReading ? "true" : "false";
        var bjs = "(function(){ var on=" + enable + "; var vp=document.getElementById('reader-viewport')||document.body; if(!vp)return;" +
                  "if(!on){ var bs=vp.querySelectorAll('b.libris-bionic'); for(var i=0;i<bs.length;i++){ var b=bs[i]; b.parentNode.replaceChild(document.createTextNode(b.textContent),b); } vp.normalize(); return; }" +
                  "if(vp.querySelector('b.libris-bionic')) return;" +
                  "var w=document.createTreeWalker(vp,NodeFilter.SHOW_TEXT,null,false); var nodes=[]; var n;" +
                  "while(n=w.nextNode()){ var p=n.parentNode; if(!p)continue; var tag=p.tagName; if(tag==='SCRIPT'||tag==='STYLE'||tag==='B'||tag==='STRONG'||tag==='MARK')continue; if(n.nodeValue&&n.nodeValue.trim().length>2)nodes.push(n); }" +
                  "for(var j=0;j<nodes.length;j++){ var tn=nodes[j]; var txt=tn.nodeValue; var parts=txt.split(/(\\s+)/); var frag=document.createDocumentFragment();" +
                  "for(var k=0;k<parts.length;k++){ var s=parts[k]; if(!s)continue; if(/^\\s+$/.test(s)||s.length<3){ frag.appendChild(document.createTextNode(s)); } else { var cut=Math.ceil(s.length*0.45); var b=document.createElement('b'); b.className='libris-bionic'; b.appendChild(document.createTextNode(s.slice(0,cut))); frag.appendChild(b); frag.appendChild(document.createTextNode(s.slice(cut))); } }" +
                  "tn.parentNode.replaceChild(frag,tn); } })();";
        webView.evaluateJavaScript(bjs);
    }

    function isMangaRtlPaged() {
        return bookManager.currentFormat === "cbz" && bookManager.mangaRtlMode;
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
                    var rtl = isMangaRtlPaged();
                    // Ngang: vuốt nhẹ 25% chiều rộng bàn phím để sang trang mượt mà
                    if (totalDeltaX <= -horizReq || readerRootContainer.kbdTouchAccumX <= -horizReq) {
                        if (rtl) turnPrev(); else turnNext();
                        readerRootContainer.lastPageTurnTime = now;
                        readerRootContainer.kbdPageTurnedInGesture = true;
                    } else if (totalDeltaX >= horizReq || readerRootContainer.kbdTouchAccumX >= horizReq) {
                        if (rtl) turnNext(); else turnPrev();
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
                    var rtlTp = isMangaRtlPaged();
                    var tpNextX = rtlTp ? (readerRootContainer.tpAccumX > 18) : (readerRootContainer.tpAccumX < -18);
                    var tpPrevX = rtlTp ? (readerRootContainer.tpAccumX < -18) : (readerRootContainer.tpAccumX > 18);
                    // Swiping UP (dy < 0) or forward horizontal = Next page
                    if (readerRootContainer.tpAccumY < -15 || tpNextX) {
                        turnNext();
                        readerRootContainer.lastPageTurnTime = now;
                        readerRootContainer.tpAccumY = 0;
                        readerRootContainer.tpAccumX = 0;
                        readerRootContainer.tpPageTurned = true;
                    // Swiping DOWN (dy > 0) or backward horizontal = Prev page
                    } else if (readerRootContainer.tpAccumY > 15 || tpPrevX) {
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
        if (t === 4) {
            if (readerPage.showShortcutsHelp) {
                readerPage.showShortcutsHelp = false;
            } else if (footnoteOverlayContainer.visible) {
                footnoteOverlayContainer.visible = false;
            } else if (readerPage.showLayoutPopup || readerPage.showFontPopup || readerPage.showThemePopup || readerPage.showDisplayHud) {
                readerPage.showLayoutPopup = false;
                readerPage.showFontPopup = false;
                readerPage.showThemePopup = false;
                readerPage.showDisplayHud = false;
            } else {
                dismissOpeningBook();
            }
            return;
        }
        if (bookManager.readingMode === "scroll" || (bookManager.isPdf && !bookManager.isPdfReflow)) {
            // Up = scroll forward/down (+); Down = scroll backward/up (-)
            if (t === 2) {
                accumulateScroll(75 * mult);
            } else if (t === 3) {
                accumulateScroll(-75 * mult);
            }
        } else {
            var rtlNav = isMangaRtlPaged();
            var navNext = (t === 2) || (rtlNav ? (t === 1) : (t === 0));
            var navPrev = (t === 3) || (rtlNav ? (t === 0) : (t === 1));
            if (navNext) {
                turnNext();
            } else if (navPrev) {
                turnPrev();
            }
        }
    }

    function handleHardwareKey(event) {
        var k = event.key;
        var u = ("" + event.unicode).toLowerCase();

        // 1. Space or Shift+Space (supports TTS play/pause)
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

        // 2. Left / Right arrows (respect Manga RTL direction)
        var isRightArrow = (k === 61269 || k === 61479);
        var isLeftArrow = (k === 61268 || k === 61478);
        if (isRightArrow || isLeftArrow) {
            if (bookManager.readingMode === "scroll" || (bookManager.isPdf && !bookManager.isPdfReflow)) {
                if (isRightArrow) scrollLineDown(); else scrollLineUp();
            } else {
                var rtlKey = isMangaRtlPaged();
                var goNextKey = rtlKey ? isLeftArrow : isRightArrow;
                if (goNextKey) turnNext(); else turnPrev();
            }
            return;
        }

        // 3. Hardware PageDown / Down arrow
        if (k === 61271 || k === 61481 || k === 61273 || k === 61483) {
            if (bookManager.readingMode === "scroll" || (bookManager.isPdf && !bookManager.isPdfReflow)) {
                scrollLineDown();
            } else {
                turnNext();
            }
            return;
        }

        // 4. Hardware PageUp / Up arrow / Backspace
        if (k === 8 || k === 61270 || k === 61480 || k === 61272 || k === 61482) {
            if (bookManager.readingMode === "scroll" || (bookManager.isPdf && !bookManager.isPdfReflow)) {
                scrollLineUp();
            } else {
                turnPrev();
            }
            return;
        }

        // 4. Return / Enter: Toggle Fullscreen
        if (k === 13 || k === 61453 || u === "\r" || u === "\n") {
            toggleFullscreen();
            return;
        }

        // 5. N: Next Chapter / Next Page
        if (u === "n" || k === 78 || k === 110) {
            if (bookManager.isPdf && !bookManager.isPdfReflow) {
                bookManager.nextChapter();
            } else {
                turnNext();
            }
            return;
        }

        // 6. P: Prev Chapter / Prev Page
        if (u === "p" || k === 80 || k === 112) {
            if (bookManager.isPdf && !bookManager.isPdfReflow) {
                bookManager.prevChapter();
            } else {
                turnPrev();
            }
            return;
        }

        // 7. J / K: Line Down / Line Up in scroll mode or turn in paged mode
        if (u === "j" || k === 74 || k === 106) {
            scrollLineDown();
            return;
        }
        if (u === "k" || k === 75 || k === 107) {
            scrollLineUp();
            return;
        }

        // 8. T: Jump to Top (First Chapter / Page 1)
        if (u === "t" || k === 84 || k === 116) {
            if (bookManager.isPdf && !bookManager.isPdfReflow) {
                bookManager.goToChapter(1);
            } else if (bookManager.readingMode === "paged") {
                webView.evaluateJavaScript("if (typeof goToPage === 'function') goToPage(1);");
            } else {
                webView.evaluateJavaScript("var vp = document.getElementById('reader-viewport'); if (vp) { vp.scrollTop = 0; updatePageInfo(); }");
            }
            return;
        }

        // 9. B: Jump to Bottom (Last Chapter / Last Page)
        if (u === "b" || k === 66 || k === 98) {
            if (bookManager.isPdf && !bookManager.isPdfReflow) {
                bookManager.goToChapter(bookManager.totalChapters);
            } else if (bookManager.readingMode === "paged") {
                webView.evaluateJavaScript("if (typeof goToPage === 'function' && typeof totalPages !== 'undefined') goToPage(totalPages);");
            } else {
                webView.evaluateJavaScript("var vp = document.getElementById('reader-viewport'); if (vp) { vp.scrollTop = vp.scrollHeight; updatePageInfo(); }");
            }
            return;
        }

        // 10. M: Toggle Bookmark at current position
        if (u === "m" || k === 77 || k === 109) {
            var isBmAdded = bookManager.toggleBookmarkCurrentPosition();
            readerToast.body = isBmAdded ? (qsTr("Đã đánh dấu trang: ") + bookManager.currentChapterTitle) : (qsTr("Đã bỏ dấu trang"));
            readerToast.show();
            return;
        }

        // 10b. H: Open Highlights & Bookmarks sheet
        if (u === "h" || k === 72 || k === 104) {
            highlightSheet.open();
            return;
        }

        // 11. F or /: Open Search
        if (u === "f" || u === "/" || k === 70 || k === 102 || k === 47) {
            searchSheet.open();
            return;
        }

        // 12. D: Cycle Reading Theme (Dark -> Sepia -> Light -> Dark)
        if (u === "d" || k === 68 || k === 100) {
            var curTheme = bookManager.theme;
            var nextTheme = "dark";
            if (curTheme === "dark") nextTheme = "sepia";
            else if (curTheme === "sepia") nextTheme = "light";
            else nextTheme = "dark";
            bookManager.setTheme(nextTheme);
            readerToast.body = nextTheme === "dark" ? qsTr("Giao diện: Tối (AMOLED)") :
                              (nextTheme === "sepia" ? qsTr("Giao diện: Vàng dịu (Sepia)") : qsTr("Giao diện: Sáng"));
            readerToast.show();
            return;
        }

        // 13. A: Toggle Text-to-Speech (TTS)
        if (u === "a" || k === 65 || k === 97) {
            if (!bookManager.isPdf || bookManager.isPdfReflow) {
                bookManager.toggleTts();
            }
            return;
        }

        // 14. C: Toggle Reading Mode (Paged <-> Scroll)
        if (u === "c" || k === 67 || k === 99) {
            if (!bookManager.isPdf || bookManager.isPdfReflow) {
                var nextMode = bookManager.readingMode === "scroll" ? "paged" : "scroll";
                bookManager.setReadingMode(nextMode);
                readerToast.body = nextMode === "scroll" ?
                                   (qsTr("Đã chuyển sang chế độ Cuộn liên tục") + Retranslate.onLanguageChanged) :
                                   (qsTr("Đã chuyển sang chế độ Vuốt lật trang") + Retranslate.onLanguageChanged);
                readerToast.show();
            }
            return;
        }

        // 15. Z, +, =: Increase font size
        if (u === "z" || u === "+" || u === "=" || k === 90 || k === 122 || k === 43 || k === 61) {
            var newFsUp = Math.min(36, bookManager.fontSize + 1);
            bookManager.setFontSize(newFsUp);
            readerToast.body = qsTr("Cỡ chữ: ") + newFsUp + " px";
            readerToast.show();
            return;
        }

        // 16. W, -: Decrease font size
        if (u === "w" || u === "-" || u === "_" || k === 87 || k === 119 || k === 45) {
            var newFsDown = Math.max(10, bookManager.fontSize - 1);
            bookManager.setFontSize(newFsDown);
            readerToast.body = qsTr("Cỡ chữ: ") + newFsDown + " px";
            readerToast.show();
            return;
        }

        // 17. S: Open Settings
        if (u === "s" || k === 83 || k === 115) {
            settingsSheet.open();
            return;
        }

        // 18. G: Open Table of Contents (TOC)
        if (u === "g" || k === 71 || k === 103) {
            tocSheet.open();
            return;
        }

        // 19. L: Open Dictionary
        if (u === "l" || k === 76 || k === 108) {
            dictSheet.open();
            return;
        }

        // 20. I: Book Info
        if (u === "i" || k === 73 || k === 105) {
            bookDetailSheet.open();
            return;
        }

        // 21. R: Rotate PDF (if PDF) or Toggle Reading Ruler
        if (u === "r" || k === 82 || k === 114) {
            if (bookManager.isPdf && !bookManager.isPdfReflow) {
                bookManager.rotatePdf();
            } else {
                readerPage.readingRulerEnabled = !readerPage.readingRulerEnabled;
                readerToast.body = readerPage.readingRulerEnabled ? qsTr("Đã bật thước kẻ đọc") : qsTr("Đã tắt thước kẻ đọc");
                readerToast.show();
            }
            return;
        }

        // 21b. ? : Toggle Keyboard Shortcuts Help Overlay
        if (u === "?" || k === 63) {
            readerPage.showShortcutsHelp = !readerPage.showShortcutsHelp;
            return;
        }

        // 21c. U or Backspace: Jump back to previous reading position
        if (u === "u" || k === 85 || k === 117 || k === 8 || k === 0x01000003) {
            jumpBack();
            return;
        }

        // 22. X: Toggle PDF Reflow (if PDF)
        if (u === "x" || k === 88 || k === 120) {
            if (bookManager.isPdf) bookManager.togglePdfReflow();
            return;
        }

        // 22b. Q: Lock/Unlock PDF Horizontal Scroll
        if (u === "q" || k === 81 || k === 113) {
            if (bookManager.isPdf && !bookManager.isPdfReflow) {
                readerPage.pdfLockHorizontal = !readerPage.pdfLockHorizontal;
                readerToast.body = readerPage.pdfLockHorizontal ? qsTr("Đã khóa cuộn ngang PDF") : qsTr("Đã mở khóa cuộn ngang PDF");
                readerToast.show();
            }
            return;
        }

        // 22c. V: Toggle Bionic Reading
        if (u === "v" || k === 86 || k === 118) {
            if (!bookManager.isPdf || bookManager.isPdfReflow) {
                bookManager.setBionicReading(!bookManager.bionicReading);
                readerToast.body = bookManager.bionicReading ? qsTr("Đã bật Bionic Reading") : qsTr("Đã tắt Bionic Reading");
                readerToast.show();
            }
            return;
        }

        // 23. Number keys 1-9: Jump to percentage (10% - 90%)
        if (k >= 49 && k <= 57) {
            var pctVal = (k - 48) * 0.10;
            if (bookManager.isPdf && !bookManager.isPdfReflow) {
                var targetPg = Math.max(1, Math.min(bookManager.totalChapters, Math.round(bookManager.totalChapters * pctVal)));
                bookManager.goToChapter(targetPg);
                readerToast.body = qsTr("Chuyển đến trang ") + targetPg + "/" + bookManager.totalChapters;
                readerToast.show();
            } else if (bookManager.readingMode === "paged") {
                webView.evaluateJavaScript("if (typeof totalPages !== 'undefined' && totalPages > 1 && typeof goToPage === 'function') { var targetP = Math.max(1, Math.min(totalPages, Math.round(totalPages * " + pctVal + "))); goToPage(targetP); }");
            } else {
                webView.evaluateJavaScript("var vp = document.getElementById('reader-viewport'); if (vp) { var maxS = Math.max(0, vp.scrollHeight - vp.clientHeight); vp.scrollTop = Math.round(maxS * " + pctVal + "); updatePageInfo(); }");
            }
            return;
        }
    }

    function handleThemeChanged() { applyStyleLive(); }
    function handleFontSizeChanged() { applyStyleLive(); }
    function handleFontFamilyChanged() { applyStyleLive(); }
    function handleMarginsChanged() { applyStyleLive(); }
    function handleLineSpacingChanged() { applyStyleLive(); }
    function handleEyeRestReminder() {
        readerToast.body = qsTr("Bạn đã đọc liên tục một chặng — hãy nghỉ mắt 20 giây nhé!") + Retranslate.onLanguageChanged;
        readerToast.show();
    }

    function handlePageRenderTimeout() {
        readerPage.isRendering = false;
        bookManager.setIsLoadingBook(false);
    }

    function handleRequestScrollToAnchor(anchor) {
        if (anchor && anchor !== "") {
            var escAnc = anchor.replace(/\\/g, "\\\\").replace(new RegExp("\x27", "g"), "\\\x27");
            webView.evaluateJavaScript("if (typeof scrollToAnchor === 'function') scrollToAnchor('" + escAnc + "');");
        }
    }

    function handleShowFootnotePopup(fnTitle, fnBody) {
        if (fnBody && fnBody !== "") {
            footnoteOverlayContainer.footnoteTitle = (fnTitle && fnTitle !== "") ? fnTitle : qsTr("Chú thích");
            footnoteOverlayContainer.footnoteBody = fnBody;
            footnoteOverlayContainer.visible = true;
        }
    }

    function handleCurrentChapterChanged() {
        if (footnoteOverlayContainer.visible) {
            footnoteOverlayContainer.visible = false;
        }
        if (!bookManager.isPdf || bookManager.isPdfReflow) {
            readerPage.isRendering = true;
            bookManager.startWatchdog(1200);
            webView.html = bookManager.currentContent;
        }
    }

    function handleReadingModeChanged() {
        if (bookManager.readingMode !== "scroll" && readerPage.autoScrollActive) {
            readerPage.autoScrollActive = false;
            autoScrollTimer.stop();
        }
        if (!bookManager.isPdf || bookManager.isPdfReflow) {
            readerPage.isRendering = true;
            bookManager.startWatchdog(1200);
            webView.html = bookManager.currentContent;
        }
    }

    function handleCurrentPdfImageChanged() {
        readerPage.isRendering = false;
        bookManager.stopWatchdog();
        bookManager.setIsLoadingBook(false);
    }

    function handlePdfOpenError(message) {
        readerPage.isRendering = false;
        bookManager.stopWatchdog();
        bookManager.setIsLoadingBook(false);
        if (message && message !== "") {
            readerToast.body = message;
            readerToast.show();
        }
    }

    function handleTtsCurrentSentenceChanged(index, text) {
        if (!bookManager.isPdf || bookManager.isPdfReflow) {
            webView.evaluateJavaScript("if (typeof highlightTtsSentence === 'function') highlightTtsSentence(" + JSON.stringify(text) + ");");
        }
    }

    function handleTtsActiveChanged(active) {
        if (active) {
            readerPage.autoScrollActive = false;
            autoScrollTimer.stop();
        } else {
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
        if (pdfTapTimer) {
            pdfTapTimer.stop();
        }
        if (inertiaTimer) {
            inertiaTimer.stop();
        }
        if (initialFocusTimer) {
            initialFocusTimer.stop();
        }
        readerPage.autoScrollActive = false;
        if (autoScrollTimer) {
            autoScrollTimer.stop();
        }
        if (footnoteOverlayContainer.visible) {
            footnoteOverlayContainer.visible = false;
        }
        try {
            if (typeof bookManager !== "undefined" && bookManager) {
                bookManager.themeChanged.disconnect(readerPage.handleThemeChanged);
                bookManager.focusModeChanged.disconnect(readerPage.handleFocusModeChanged);
                bookManager.fontSizeChanged.disconnect(readerPage.handleFontSizeChanged);
                bookManager.fontFamilyChanged.disconnect(readerPage.handleFontFamilyChanged);
                bookManager.marginsChanged.disconnect(readerPage.handleMarginsChanged);
                bookManager.lineSpacingChanged.disconnect(readerPage.handleLineSpacingChanged);
                bookManager.textAlignChanged.disconnect(readerPage.handleLineSpacingChanged);
                bookManager.typographyChanged.disconnect(readerPage.handleLineSpacingChanged);
                bookManager.eyeRestReminder.disconnect(readerPage.handleEyeRestReminder);
                bookManager.overridePublisherCssChanged.disconnect(readerPage.handleCurrentChapterChanged);
                bookManager.pageRenderTimeout.disconnect(readerPage.handlePageRenderTimeout);
                bookManager.requestScrollToAnchor.disconnect(readerPage.handleRequestScrollToAnchor);
                bookManager.showFootnotePopup.disconnect(readerPage.handleShowFootnotePopup);
                bookManager.currentChapterChanged.disconnect(readerPage.handleCurrentChapterChanged);
                bookManager.readingModeChanged.disconnect(readerPage.handleReadingModeChanged);
                bookManager.columnLayoutChanged.disconnect(readerPage.handleReadingModeChanged);
                bookManager.currentPdfImageChanged.disconnect(readerPage.handleCurrentPdfImageChanged);
                bookManager.pdfOpenError.disconnect(readerPage.handlePdfOpenError);
                bookManager.pagePositionChanged.disconnect(readerPage.syncScrubberFromBookManager);
                bookManager.volumePageNextRequested.disconnect(readerPage.turnNext);
                bookManager.volumePagePrevRequested.disconnect(readerPage.turnPrev);
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
        try {
            if (dictSheet && typeof dictSheet.cleanup === "function") {
                dictSheet.cleanup();
            }
        } catch (e) {}
        try {
            if (highlightSheet && typeof highlightSheet.cleanup === "function") {
                highlightSheet.cleanup();
            }
        } catch (e) {}
    }

    function handleFocusModeChanged() {
        if (!bookManager.isPdf || bookManager.isPdfReflow) {
            webView.evaluateJavaScript("if (window.setLibrisFocusMode) window.setLibrisFocusMode(" + (bookManager.focusModeEnabled ? "true" : "false") + ");");
        }
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
        bm.focusModeChanged.connect(rp.handleFocusModeChanged);
        bm.fontSizeChanged.connect(rp.handleFontSizeChanged);
        bm.fontFamilyChanged.connect(rp.handleFontFamilyChanged);
        bm.marginsChanged.connect(rp.handleMarginsChanged);
        bm.lineSpacingChanged.connect(rp.handleLineSpacingChanged);
        bm.textAlignChanged.connect(rp.handleLineSpacingChanged);
        bm.typographyChanged.connect(rp.handleLineSpacingChanged);
        bm.eyeRestReminder.connect(rp.handleEyeRestReminder);
        bm.overridePublisherCssChanged.connect(rp.handleCurrentChapterChanged);
        bm.pageRenderTimeout.connect(rp.handlePageRenderTimeout);
        bm.requestScrollToAnchor.connect(rp.handleRequestScrollToAnchor);
        bm.showFootnotePopup.connect(rp.handleShowFootnotePopup);
        bm.currentChapterChanged.connect(rp.handleCurrentChapterChanged);
        bm.readingModeChanged.connect(rp.handleReadingModeChanged);
        bm.columnLayoutChanged.connect(rp.handleReadingModeChanged);
        bm.currentPdfImageChanged.connect(rp.handleCurrentPdfImageChanged);
        bm.pdfOpenError.connect(rp.handlePdfOpenError);
        bm.pagePositionChanged.connect(rp.syncScrubberFromBookManager);
        bm.volumePageNextRequested.connect(rp.turnNext);
        bm.volumePagePrevRequested.connect(rp.turnPrev);
        rp.syncScrubberFromBookManager();

        if (bm.isPdf) {
            readerPage.addAction(pdfCropAction, ActionBarPlacement.InOverflow);
            readerPage.addAction(pdfReflowAction, ActionBarPlacement.InOverflow);
        }
        if (bm.currentFormat === "cbz") {
            readerPage.addAction(mangaModeActionItem, ActionBarPlacement.InOverflow);
        }

        ttsManager.currentSentenceChanged.connect(rp.handleTtsCurrentSentenceChanged);
        ttsManager.activeChanged.connect(rp.handleTtsActiveChanged);
        ttsManager.errorOccurred.connect(rp.handleTtsErrorOccurred);
    }

    function turnNext() {
        if (footnoteOverlayContainer.visible) {
            footnoteOverlayContainer.visible = false;
        }
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
        if (footnoteOverlayContainer.visible) {
            footnoteOverlayContainer.visible = false;
        }
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
        if (bookManager.isPdf && !bookManager.isPdfReflow) {
            bookManager.nextChapter();
        } else {
            webView.evaluateJavaScript("if (typeof scrollDown === 'function') scrollDown(); else window.scrollBy(0, 80);");
        }
    }

    function scrollLineUp() {
        if (bookManager.isPdf && !bookManager.isPdfReflow) {
            bookManager.prevChapter();
        } else {
            webView.evaluateJavaScript("if (typeof scrollUp === 'function') scrollUp(); else window.scrollBy(0, -80);");
        }
    }

    function increaseFont() {
        var newSize = bookManager.fontSize + 2;
        bookManager.setFontSize(newSize);
    }

    function decreaseFont() {
        var newSize = bookManager.fontSize - 2;
        bookManager.setFontSize(newSize);
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
        }
    }

    attachedObjects: [
        ArrayDataModel {
            id: fontListModel
        },
        SettingsSheet {
            id: settingsSheet
            onClosed: { readerRootContainer.requestFocus(); }
        },
        BookDetailSheet {
            id: bookDetailSheet
            hideReadButton: true
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
            }
            onNavigateToSearchResult: {
                readerPage.pushJumpHistory();
                readerPage.pendingSearchQuery = query;
                readerPage.pendingSearchMatchIndex = matchIndex;
                bookManager.goToChapter(chapterIndex);
            }
            onPerformInPageSearch: {
                var escQuery = query.replace(/\\/g, "\\\\").replace(new RegExp("\x27", "g"), "\\\x27");
                webView.evaluateJavaScript("if (typeof searchInPage === 'function') { searchInPage('" + escQuery + "', false); } else { setTimeout(function() { if (typeof searchInPage === 'function') searchInPage('" + escQuery + "', false); }, 180); }");
            }
            onJumpToMatch: {
                webView.evaluateJavaScript("if (typeof jumpToMatch === 'function' && typeof totalMatches !== 'undefined' && totalMatches > 0) { jumpToMatch(" + matchIndex + "); } else { setTimeout(function() { if (typeof jumpToMatch === 'function') jumpToMatch(" + matchIndex + "); }, 220); }");
            }
            onInPageNext: {
                webView.evaluateJavaScript("if (typeof searchNext === 'function') searchNext();");
            }
            onInPagePrev: {
                webView.evaluateJavaScript("if (typeof searchPrev === 'function') searchPrev();");
            }
            onInPageClear: {
                readerPage.pendingSearchQuery = "";
                readerPage.pendingSearchMatchIndex = -1;
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
                readerPage.pushJumpHistory();
                webView.evaluateJavaScript("if (typeof scrollToHighlight === 'function') scrollToHighlight('" + highlightId + "');");
            }
        },
        HighlightNoteSheet {
            id: highlightNoteSheet
            onClosed: { readerRootContainer.requestFocus(); }
            onSavedCallback: {
                webView.evaluateJavaScript("var el = document.querySelector('mark[data-hl-id=\"" + highlightId + "\"]'); if (el) { el.className = 'libris-highlight libris-hl-' + " + JSON.stringify(color) + "; el.setAttribute('data-hl-note', " + JSON.stringify(note) + "); }");
            }
            onDeletedCallback: {
                webView.evaluateJavaScript("if (typeof removeHighlightFromDom === 'function') removeHighlightFromDom('" + highlightId + "');");
            }
        },
        ActionItem {
            id: pdfCropAction
            title: (bookManager.isPdfAutoCrop ? qsTr("Cắt lề PDF") + " ✓" : qsTr("Cắt lề PDF")) + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_crop.png"
            onTriggered: {
                bookManager.togglePdfAutoCrop();
                readerToast.body = bookManager.isPdfAutoCrop ?
                                   (qsTr("Đã bật cắt lề trắng PDF") + Retranslate.onLanguageChanged) :
                                   (qsTr("Đã tắt cắt lề trắng PDF") + Retranslate.onLanguageChanged);
                readerToast.show();
            }
        },
        ActionItem {
            id: pdfReflowAction
            title: (bookManager.isPdfReflow ? qsTr("PDF gốc") : qsTr("Dàn chữ (OCR)")) + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_text_mode.png"
            enabled: bookManager.hasPdfText || bookManager.isPdfReflow
            onTriggered: {
                bookManager.togglePdfReflow();
            }
        },
        ActionItem {
            id: mangaModeActionItem
            title: (bookManager.mangaRtlMode ? qsTr("Manga RTL (Đang bật)") : qsTr("Manga RTL (Đang tắt)")) + Retranslate.onLanguageChanged
            imageSource: "asset:///icons/ic_page.png"
            onTriggered: {
                bookManager.mangaRtlMode = !bookManager.mangaRtlMode;
                bookManager.renderCurrentChapter();
                readerToast.body = bookManager.mangaRtlMode ? qsTr("Đã bật chế độ Manga (Phải qua Trái)") : qsTr("Đã tắt chế độ Manga (Trái qua Phải)");
                readerToast.show();
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
        // ON-BAR ACTIONS (3 Essential Slots: Mục lục | Hiển thị Aa | Đánh dấu)
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
            title: qsTr("Hiển thị") + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.OnBar
            imageSource: "asset:///icons/ic_display_panel.png"
            onTriggered: {
                readerPage.showDisplayHud = !readerPage.showDisplayHud;
            }
        },
        ActionItem {
            title: (bookManager.isCurrentChapterBookmarked ? qsTr("Bỏ dấu trang") : qsTr("Đánh dấu trang")) + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.OnBar
            imageSource: bookManager.isCurrentChapterBookmarked ? "asset:///icons/ic_bookmark_filled.png" : "asset:///icons/ic_bookmark_add.png"
            onTriggered: {
                var isAdded = bookManager.toggleBookmarkCurrentPosition();
                readerToast.body = isAdded ? (qsTr("Đã đánh dấu trang: ") + bookManager.currentChapterTitle) : (qsTr("Đã bỏ dấu trang"));
                readerToast.show();
            }
        },

        // =====================================================================
        // IN-OVERFLOW ACTIONS (Clean & Contextual — PDF/Manga added only when applicable)
        // =====================================================================
        ActionItem {
            title: qsTr("Tìm kiếm") + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: "asset:///icons/ic_search.png"
            onTriggered: {
                searchSheet.open();
            }
        },
        ActionItem {
            title: qsTr("Dấu trang & Ghi chú") + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: "asset:///icons/ic_bookmarks.png"
            onTriggered: {
                highlightSheet.open();
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
            title: (readerPage.autoScrollActive ? qsTr("Dừng") + " " + qsTr("Tự cuộn") : qsTr("Tự cuộn")) + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: "asset:///icons/ic_scroll.png"
            enabled: (!bookManager.isPdf || bookManager.isPdfReflow) && bookManager.readingMode === "scroll"
            onTriggered: {
                readerPage.toggleAutoScroll();
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
            id: favActionItem
            property bool isBookFavorite: bookManager.isFavorite(bookManager.currentFilePath)
            title: (isBookFavorite ? qsTr("Bỏ yêu thích") : qsTr("Yêu thích")) + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: isBookFavorite ? "asset:///icons/ic_star_filled.png" : "asset:///icons/ic_star.png"
            onTriggered: {
                var isFav = bookManager.toggleFavorite(bookManager.currentFilePath);
                favActionItem.isBookFavorite = isFav;
                readerToast.body = isFav ? (qsTr("Đã thêm vào yêu thích: ") + bookManager.currentBookTitle) : (qsTr("Đã bỏ yêu thích"));
                readerToast.show();
            }
        },
        ActionItem {
            id: focusModeActionItem
            title: (bookManager.focusModeEnabled ? qsTr("Chế độ Tập trung (Đang bật)") : qsTr("Chế độ Tập trung")) + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: "asset:///icons/ic_display_aa.png"
            enabled: !bookManager.isPdf || bookManager.isPdfReflow
            onTriggered: {
                bookManager.toggleFocusMode();
                readerToast.body = (bookManager.focusModeEnabled ? qsTr("Đã bật Chế độ Tập trung") : qsTr("Đã tắt Chế độ Tập trung")) + Retranslate.onLanguageChanged;
                readerToast.show();
            }
        },
        ActionItem {
            id: nextSeriesActionItem
            property variant nextBook: (bookManager.bookCount >= 0 && bookManager.totalScannedBooks >= 0) ? bookManager.nextSeriesBook(bookManager.currentFilePath) : ({})
            title: qsTr("Đọc tiếp tập kế") + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: "asset:///icons/ic_next.png"
            enabled: nextBook && nextBook.filePath !== undefined && nextBook.filePath !== ""
            onTriggered: {
                var nb = bookManager.nextSeriesBook(bookManager.currentFilePath);
                if (nb && nb.filePath) {
                    bookManager.openBook(nb.filePath);
                }
            }
        },
        ActionItem {
            title: qsTr("Thông tin") + Retranslate.onLanguageChanged
            ActionBar.placement: ActionBarPlacement.InOverflow
            imageSource: "asset:///icons/ic_info.png"
            onTriggered: {
                bookDetailSheet.open();
            }
        }
    ]
}
