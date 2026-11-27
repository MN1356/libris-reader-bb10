import bb.cascades 1.3
import bb.system 1.2

Sheet {
    id: pdfOcrTextSheet

    property int pageNumber: 1
    property string pageText: ""
    property string selectedText: ""
    property string htmlContent: ""
    property bool isOpen: false

    onOpened: {
        isOpen = true;
    }

    onClosed: {
        isOpen = false;
    }

    signal switchToReflowRequested()
    signal lookupRequested(string word)
    signal translateRequested(string text)

    function escapeHtmlStr(s) {
        if (!s) return "";
        return s.replace(/&/g, "&amp;")
                .replace(/</g, "&lt;")
                .replace(/>/g, "&gt;")
                .replace(/\"/g, "&quot;");
    }

    function buildOcrHtml(txt) {
        var isDark = (bookManager.theme === "dark");
        var isSepia = (bookManager.theme === "sepia");
        var bg = isDark ? "#0D0B09" : (isSepia ? "#F4ECD8" : "#FAF7F2");
        var fg = isDark ? "#F5EFE6" : (isSepia ? "#3E2D1D" : "#1C1916");
        var accent = "#D4A359";
        var hlBg = isDark ? "rgba(212,163,89,0.34)" : "rgba(212,163,89,0.40)";
        var pxSize = Math.max(15, Math.min(26, Math.round(bookManager.fontSize * 0.65)));

        var norm = (txt || "").replace(/\r\n/g, "\n").replace(/\r/g, "\n").trim();
        var blocks = norm.split(/\n{2,}/);
        var bodyHtml = "";
        for (var i = 0; i < blocks.length; ++i) {
            var b = blocks[i].trim();
            if (b.length === 0) continue;
            bodyHtml += "<p>" + escapeHtmlStr(b).replace(/\n/g, "<br/>") + "</p>";
        }
        if (bodyHtml === "") {
            bodyHtml = "<p>" + escapeHtmlStr(norm) + "</p>";
        }

        var html = "<!DOCTYPE html><html><head><meta charset='utf-8'/>"
            + "<meta name='viewport' content='width=device-width, initial-scale=1.0, user-scalable=no'/>"
            + "<style>"
            + "html,body{margin:0;padding:0;background:" + bg + ";color:" + fg + ";font-family:sans-serif;"
            + "font-size:" + pxSize + "px;line-height:1.6;-webkit-user-select:text;user-select:text;-webkit-touch-callout:default;}"
            + "#ocr-root{padding:16px 16px 28px 16px;word-wrap:break-word;}"
            + "p{margin:0 0 14px 0;}"
            + "mark.tts-active{background:" + hlBg + ";color:" + fg + ";border-bottom:2px solid " + accent + ";padding:1px 2px;border-radius:3px;}"
            + "</style></head><body>"
            + "<div id='ocr-root'>" + bodyHtml + "</div>"
            + "<script>"
            + "function reportSel(){var s=window.getSelection?window.getSelection().toString():'';if(window.navigator&&window.navigator.cascades){window.navigator.cascades.postMessage('sel:'+encodeURIComponent((s||'').trim()));}}"
            + "document.addEventListener('selectionchange',function(){setTimeout(reportSel,60);},false);"
            + "document.addEventListener('touchend',function(){setTimeout(reportSel,90);},false);"
            + "document.addEventListener('mouseup',function(){setTimeout(reportSel,60);},false);"
            + "function clearTts(){var ms=document.querySelectorAll('mark.tts-active');for(var i=0;i<ms.length;i++){var m=ms[i],p=m.parentNode;if(p){while(m.firstChild)p.insertBefore(m.firstChild,m);p.removeChild(m);p.normalize();}}}"
            + "function highlightSentence(txt){clearTts();if(!txt)return;var t=txt.trim();if(t.length<2)return;"
            + "var needle=t.length>36?t.substring(0,36).trim():t;"
            + "var root=document.getElementById('ocr-root')||document.body;"
            + "var w=document.createTreeWalker(root,NodeFilter.SHOW_TEXT,null,false);var n;"
            + "while((n=w.nextNode())){var v=n.nodeValue;var idx=v.indexOf(needle);"
            + "if(idx===-1&&needle.length>16){idx=v.indexOf(needle.substring(0,16));}"
            + "if(idx!==-1){try{var len=Math.min(v.length-idx,t.length);var r=document.createRange();r.setStart(n,idx);r.setEnd(n,idx+len);var mk=document.createElement('mark');mk.className='tts-active';r.surroundContents(mk);if(mk.scrollIntoView)mk.scrollIntoView({block:'center'});break;}catch(e){}}}}"
            + "</script></body></html>";
        return html;
    }

    function openForPage(pg) {
        var targetPg = (pg && pg >= 1) ? pg : Math.max(1, bookManager.currentChapterIndex);
        pageNumber = targetPg;
        var txt = bookManager.getPdfPageText(targetPg);
        pageText = txt ? txt : "";
        selectedText = "";
        htmlContent = pageText.length > 0 ? buildOcrHtml(pageText) : "";
        if (pageText.length > 0) {
            ocrWebView.html = htmlContent;
        }
        if (!pdfOcrTextSheet.isOpen) {
            pdfOcrTextSheet.open();
        }
    }

    function syncTtsSentence(sentence) {
        if (pdfOcrTextSheet.isOpen && pdfOcrTextSheet.pageText.length > 0 && sentence) {
            ocrWebView.evaluateJavaScript("if(typeof highlightSentence==='function')highlightSentence(" + JSON.stringify(sentence) + ");");
        }
    }

    function activeText() {
        var sel = selectedText ? selectedText.trim() : "";
        if (sel.length > 0) return sel;
        return pageText ? pageText.trim() : "";
    }

    attachedObjects: [
        SystemToast {
            id: ocrToast
            body: ""
        },
        LayoutUpdateHandler {
            id: ocrWebLayoutHandler
            onLayoutFrameChanged: {
                if (layoutFrame.width > 0) {
                    ocrWebView.preferredWidth = layoutFrame.width;
                    ocrWebView.minWidth = layoutFrame.width;
                }
                if (layoutFrame.height > 0) {
                    ocrWebView.preferredHeight = layoutFrame.height;
                    ocrWebView.minHeight = layoutFrame.height;
                }
            }
        }
    ]

    Page {
        titleBar: TitleBar {
            title: qsTr("Văn bản OCR") + " • " + qsTr("Trang %1").arg(pdfOcrTextSheet.pageNumber) + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain

            dismissAction: ActionItem {
                title: qsTr("Đóng") + Retranslate.onLanguageChanged
                onTriggered: {
                    pdfOcrTextSheet.close();
                }
            }

            acceptAction: ActionItem {
                title: qsTr("Dàn chữ") + Retranslate.onLanguageChanged
                enabled: pdfOcrTextSheet.pageText.length > 0
                onTriggered: {
                    pdfOcrTextSheet.close();
                    pdfOcrTextSheet.switchToReflowRequested();
                }
            }
        }

        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create(bookManager.uiBg)
            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

            // Status / selection banner
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(bookManager.uiCard)
                topPadding: bookManager.dp(10)
                bottomPadding: bookManager.dp(10)
                leftPadding: bookManager.dp(14)
                rightPadding: bookManager.dp(14)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                Label {
                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                    verticalAlignment: VerticalAlignment.Center
                    multiline: true
                    text: pdfOcrTextSheet.pageText.length === 0
                          ? (qsTr("Trang PDF này chưa có lớp chữ OCR.") + Retranslate.onLanguageChanged)
                          : (pdfOcrTextSheet.selectedText.trim().length > 0
                             ? (qsTr("Đã chọn %1 ký tự").replace("%1", pdfOcrTextSheet.selectedText.trim().length) + Retranslate.onLanguageChanged)
                             : (qsTr("Nhấn giữ vào chữ để bôi đen chọn đoạn, hoặc dùng thao tác bên dưới.") + Retranslate.onLanguageChanged))
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: pdfOcrTextSheet.selectedText.trim().length > 0
                                     ? Color.create(bookManager.uiAccent)
                                     : Color.create(bookManager.uiMuted)
                }
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: 1
                background: Color.create(bookManager.uiHairline)
            }

            // Selectable OCR WebView (native BB10 WebKit selection handles + live TTS highlight)
            Container {
                layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layout: DockLayout {}
                layoutHandlers: [ ocrWebLayoutHandler ]

                WebView {
                    id: ocrWebView
                    visible: pdfOcrTextSheet.pageText.length > 0
                    horizontalAlignment: HorizontalAlignment.Fill
                    verticalAlignment: VerticalAlignment.Fill
                    preferredWidth: bookManager.screenWidth > 0 ? bookManager.screenWidth : 720
                    preferredHeight: 600
                    settings.zoomToFitEnabled: false

                    onLoadingChanged: {
                        if (loadRequest.status === WebLoadStatus.Succeeded) {
                            if (ttsManager.active && ttsManager.currentSentenceText !== "") {
                                ocrWebView.evaluateJavaScript("if(typeof highlightSentence==='function')highlightSentence(" + JSON.stringify(ttsManager.currentSentenceText) + ");");
                            }
                        }
                    }

                    onMessageReceived: {
                        var s = "" + message.data;
                        if (s.indexOf("sel:") === 0) {
                            var raw = s.substr(4);
                            pdfOcrTextSheet.selectedText = raw.length > 0 ? decodeURIComponent(raw) : "";
                        }
                    }
                }

                Label {
                    visible: pdfOcrTextSheet.pageText.length === 0
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    leftMargin: bookManager.dp(20)
                    rightMargin: bookManager.dp(20)
                    multiline: true
                    text: qsTr("Trang PDF này được quét dạng hình ảnh và chưa có lớp văn bản (OCR text layer).") + Retranslate.onLanguageChanged
                    textStyle.base: SystemDefaults.TextStyles.BodyText
                    textStyle.textAlign: TextAlign.Center
                    textStyle.color: Color.create(bookManager.uiMuted)
                }
            }

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                preferredHeight: 1
                background: Color.create(bookManager.uiHairline)
            }

            // Bottom Action Bar
            Container {
                visible: pdfOcrTextSheet.pageText.length > 0
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create(bookManager.uiCard)
                topPadding: bookManager.dp(10)
                bottomPadding: bookManager.dp(10)
                leftPadding: bookManager.dp(10)
                rightPadding: bookManager.dp(10)
                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    Button {
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        text: (pdfOcrTextSheet.selectedText.trim().length > 0 ? qsTr("Sao chép") : qsTr("Chép cả trang")) + Retranslate.onLanguageChanged
                        appearance: ControlAppearance.Primary
                        onClicked: {
                            var txt = pdfOcrTextSheet.activeText();
                            if (txt.length > 0) {
                                bookManager.copyText(txt);
                                ocrToast.body = qsTr("Đã sao chép vào bộ nhớ tạm") + Retranslate.onLanguageChanged;
                                ocrToast.show();
                            }
                        }
                    }

                    Button {
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        leftMargin: bookManager.dp(6)
                        text: qsTr("Đọc (TTS)") + Retranslate.onLanguageChanged
                        onClicked: {
                            var txt = pdfOcrTextSheet.activeText();
                            if (txt.length > 0) {
                                pdfOcrTextSheet.close();
                                if (pdfOcrTextSheet.selectedText.trim().length > 0) {
                                    ttsManager.startWithText(txt, "", false);
                                } else {
                                    bookManager.startTts();
                                }
                            }
                        }
                    }
                }

                Container {
                    topMargin: bookManager.dp(6)
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                    Button {
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        text: qsTr("Tra từ") + Retranslate.onLanguageChanged
                        enabled: pdfOcrTextSheet.selectedText.trim().length > 0
                        onClicked: {
                            var w = pdfOcrTextSheet.selectedText.trim();
                            if (w.length > 0) {
                                pdfOcrTextSheet.close();
                                pdfOcrTextSheet.lookupRequested(w);
                            }
                        }
                    }

                    Button {
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        leftMargin: bookManager.dp(6)
                        text: qsTr("Dịch") + Retranslate.onLanguageChanged
                        onClicked: {
                            var t = pdfOcrTextSheet.activeText();
                            if (t.length > 0) {
                                pdfOcrTextSheet.close();
                                pdfOcrTextSheet.translateRequested(t);
                            }
                        }
                    }

                    Button {
                        layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                        leftMargin: bookManager.dp(6)
                        text: qsTr("Highlight") + Retranslate.onLanguageChanged
                        enabled: pdfOcrTextSheet.selectedText.trim().length > 0
                        onClicked: {
                            var hl = pdfOcrTextSheet.selectedText.trim();
                            if (hl.length > 0) {
                                bookManager.addHighlight(hl, "", "yellow", pdfOcrTextSheet.pageNumber - 1, qsTr("Trang %1").arg(pdfOcrTextSheet.pageNumber));
                                ocrToast.body = qsTr("Đã lưu highlight") + Retranslate.onLanguageChanged;
                                ocrToast.show();
                            }
                        }
                    }
                }
            }
        }
    }
}
