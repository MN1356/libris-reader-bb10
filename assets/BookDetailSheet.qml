import bb.cascades 1.3
import bb.system 1.2

Sheet {
    id: bookDetailSheet

    signal requestOpenBook(string filePath)

    property string bookTitle: ""
    property string bookAuthor: ""
    property string bookCover: ""
    property string bookFormat: ""
    property string bookPath: ""
    property string bookFileSize: ""
    property string bookDateModified: ""
    property string bookDescription: ""
    property bool descExpanded: false
    property bool hideReadButton: false
    property string bookGenre: ""
    property string bookSeries: ""
    property int bookProgress: -1

    property bool isCurrentBook: (bookPath === "" || bookPath === bookManager.currentFilePath)
    property string displayTitle: bookTitle !== "" ? bookTitle : bookManager.currentBookTitle
    property string displayAuthor: bookAuthor !== "" ? bookAuthor : (bookManager.currentBookAuthor !== "" ? bookManager.currentBookAuthor : (qsTr("Tác giả chưa rõ") + Retranslate.onLanguageChanged))
    property string displayCover: bookCover !== "" ? bookCover : (isCurrentBook ? bookManager.currentCoverPath : "")
    property string displayFmt: (bookFormat !== "" ? bookFormat : bookManager.currentFormat).toLowerCase()
    property string displayFileSize: bookFileSize !== "" ? bookFileSize : (isCurrentBook ? bookManager.currentBookFileSize : "")
    property int displayProgress: bookProgress >= 0 ? bookProgress : (isCurrentBook ? bookManager.readingProgress : 0)
    property string displayGenre: bookGenre !== "" ? bookGenre : (isCurrentBook ? bookManager.currentBookGenre : "")
    property string displayDateModified: bookDateModified !== "" ? bookDateModified : (isCurrentBook ? bookManager.currentBookDateModified : "")
    property string displayRawPath: bookPath !== "" ? bookPath : bookManager.currentFilePath
    property string displayDesc: bookDescription !== "" ? bookDescription : (isCurrentBook ? bookManager.currentBookDescription : "")
    property variant nextSeriesInfo: ({})
    property int currentRating: 0
    property bool showMetaEditor: false

    onOpened: {
        nextSeriesInfo = bookManager.nextSeriesBook(displayRawPath);
        currentRating = bookManager.getBookRating(displayRawPath);
        reviewInput.text = bookManager.getBookReview(displayRawPath);
        editTitleField.text = displayTitle;
        editAuthorField.text = bookAuthor !== "" ? bookAuthor : bookManager.currentBookAuthor;
        editSeriesField.text = bookSeries !== "" ? bookSeries : bookManager.currentBookSeries;
        editSeriesIdxField.text = "";
        showMetaEditor = false;
    }

    function cleanDesc(t) {
        if (!t) return "";
        for (var i = 0; i < 2; ++i) {
            t = t.replace(/&quot;/g, '"').replace(/&lt;/g, "<").replace(/&gt;/g, ">").replace(/&#39;/g, "'").replace(/&apos;/g, "'").replace(/&nbsp;/g, " ").replace(/&amp;/g, "&");
            t = t.replace(/<br\s*\/?>/gi, "\n").replace(/<\/(p|div|li)>/gi, "\n").replace(/<[^>]*>/g, "");
        }
        return t.replace(/[ \t\r]+/g, " ").replace(/\n[ ]+/g, "\n").replace(/\n{2,}/g, "\n").trim();
    }

    function friendlyPathOf(rawPath) {
        if (!rawPath) return "";
        if (rawPath.indexOf("/accounts/1000/removable/sdcard") >= 0) {
            return rawPath.replace(new RegExp("^/accounts/1000/removable/sdcard[0-9]*/?"), "SD Card / ");
        }
        if (rawPath.indexOf("/accounts/1000/shared") >= 0) {
            return rawPath.replace(new RegExp("^/accounts/1000/shared/?"), "Internal / ");
        }
        return rawPath;
    }

    function openWithBook(item) {
        descExpanded = false;
        if (item) {
            bookTitle = item.title ? ("" + item.title) : "";
            bookAuthor = item.author ? ("" + item.author) : "";
            bookCover = item.cover ? ("" + item.cover) : "";
            bookFormat = item.format ? ("" + item.format) : "";
            bookPath = item.filePath ? ("" + item.filePath) : (item.path ? ("" + item.path) : "");
            bookFileSize = item.fileSize ? ("" + item.fileSize) : "";
            bookDateModified = item.dateModified ? ("" + item.dateModified) : (item.date ? ("" + item.date) : "");
            bookDescription = item.description ? ("" + item.description) : "";
            bookGenre = item.genre ? ("" + item.genre) : "";
            var isCur = (!item.filePath && !item.path) || (item.filePath === bookManager.currentFilePath) || (item.path === bookManager.currentFilePath);
            var s = item.series ? ("" + item.series) : (isCur && bookManager.currentBookSeries ? bookManager.currentBookSeries : "");
            var sIdx = item.seriesIndex !== undefined ? parseInt(item.seriesIndex) : (isCur && bookManager.currentBookSeriesIndex > 0 ? bookManager.currentBookSeriesIndex : 0);
            bookSeries = s !== "" ? (s + (sIdx > 0 ? (" #" + sIdx) : "")) : "";
            bookProgress = (item.progress !== undefined) ? parseInt(item.progress) : -1;

            if (bookPath !== "" && (bookDescription === "" || bookGenre === "")) {
                var meta = bookManager.getBookMetadata(bookPath);
                if (meta) {
                    if (bookDescription === "" && meta.description) bookDescription = "" + meta.description;
                    if (bookGenre === "" && meta.genre) bookGenre = "" + meta.genre;
                    if (bookSeries === "" && meta.series) {
                        var msIdx = meta.seriesIndex ? parseInt(meta.seriesIndex) : 0;
                        bookSeries = meta.series + (msIdx > 0 ? (" #" + msIdx) : "");
                    }
                }
            }
        }
        nextSeriesInfo = bookManager.nextSeriesBook(bookPath !== "" ? bookPath : bookManager.currentFilePath);
        bookDetailSheet.open();
    }

    onClosed: {
        bookTitle = "";
        bookAuthor = "";
        bookCover = "";
        bookFormat = "";
        bookPath = "";
        bookFileSize = "";
        bookDateModified = "";
        bookDescription = "";
        descExpanded = false;
        bookGenre = "";
        bookSeries = "";
        bookProgress = -1;
        nextSeriesInfo = ({});
    }

    Page {
        titleBar: TitleBar {
            title: qsTr("Thông tin sách") + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain
            dismissAction: ActionItem {
                title: qsTr("Đóng") + Retranslate.onLanguageChanged
                onTriggered: {
                    bookDetailSheet.close();
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
                    bottomPadding: bookManager.dp(32)

                    // =========================================================
                    // HERO CARD (Cover + Title/Metadata + Progress + Read CTAs)
                    // =========================================================
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        topPadding: bookManager.dp(16)
                        bottomPadding: bookManager.dp(16)

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(16)
                            bottomPadding: bookManager.dp(16)
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            // Top row: Cover + Primary Book Metadata
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                Container {
                                    preferredWidth: bookManager.dp(112)
                                    preferredHeight: bookManager.dp(160)
                                    minWidth: bookManager.dp(112)
                                    maxWidth: bookManager.dp(112)
                                    minHeight: bookManager.dp(160)
                                    maxHeight: bookManager.dp(160)
                                    verticalAlignment: VerticalAlignment.Top
                                    background: Color.create(bookManager.uiSurface)
                                    layout: DockLayout {}

                                    ImageView {
                                        visible: bookDetailSheet.displayCover !== ""
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        imageSource: bookDetailSheet.displayCover !== "" ? ("file://" + bookDetailSheet.displayCover) : ""
                                        scalingMethod: ScalingMethod.AspectFill
                                    }

                                    Container {
                                        visible: bookDetailSheet.displayCover === ""
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        verticalAlignment: VerticalAlignment.Fill
                                        layout: DockLayout {}

                                        ImageView {
                                            horizontalAlignment: HorizontalAlignment.Fill
                                            verticalAlignment: VerticalAlignment.Fill
                                            imageSource: {
                                                var fmt = bookDetailSheet.displayFmt ? bookDetailSheet.displayFmt : "epub";
                                                if (fmt === "azw" || fmt === "azw3" || fmt === "kf8" || fmt === "prc") fmt = "mobi";
                                                if (fmt !== "pdf" && fmt !== "epub" && fmt !== "mobi" && fmt !== "fb2" && fmt !== "cbz" && fmt !== "djvu" && fmt !== "txt") fmt = "epub";
                                                return "asset:///icons/default_cover_" + fmt + ".png";
                                            }
                                            scalingMethod: ScalingMethod.AspectFill
                                        }

                                        Container {
                                            horizontalAlignment: HorizontalAlignment.Center
                                            verticalAlignment: VerticalAlignment.Center
                                            leftPadding: bookManager.dp(8)
                                            rightPadding: bookManager.dp(8)
                                            bottomPadding: bookManager.dp(14)

                                            Label {
                                                horizontalAlignment: HorizontalAlignment.Center
                                                verticalAlignment: VerticalAlignment.Center
                                                text: bookDetailSheet.displayTitle
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.fontWeight: FontWeight.W500
                                                textStyle.color: Color.create("#F5EFE6")
                                                multiline: true
                                                autoSize.maxLineCount: 3
                                            }
                                        }
                                    }
                                }

                                // Right Metadata Column
                                Container {
                                    leftMargin: bookManager.dp(16)
                                    verticalAlignment: VerticalAlignment.Top
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Label {
                                        text: bookDetailSheet.displayTitle
                                        textStyle.base: SystemDefaults.TextStyles.TitleText
                                        textStyle.fontWeight: FontWeight.W500
                                        textStyle.color: Color.create(bookManager.uiInk)
                                        multiline: true
                                        autoSize.maxLineCount: 3
                                    }

                                    Label {
                                        topMargin: bookManager.dp(4)
                                        text: bookDetailSheet.displayAuthor
                                        textStyle.base: SystemDefaults.TextStyles.BodyText
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                        multiline: true
                                        autoSize.maxLineCount: 2
                                    }

                                    Container {
                                        topMargin: bookManager.dp(10)
                                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                        Container {
                                            background: Color.create(bookManager.uiSurface)
                                            leftPadding: bookManager.dp(10)
                                            rightPadding: bookManager.dp(10)
                                            topPadding: bookManager.dp(3)
                                            bottomPadding: bookManager.dp(3)
                                            verticalAlignment: VerticalAlignment.Center

                                            Label {
                                                text: bookDetailSheet.displayFmt.toUpperCase()
                                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                                textStyle.fontWeight: FontWeight.W500
                                                textStyle.color: Color.create(bookManager.uiAccent)
                                            }
                                        }

                                        Label {
                                            leftMargin: bookManager.dp(10)
                                            verticalAlignment: VerticalAlignment.Center
                                            text: bookDetailSheet.displayFileSize
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.color: Color.create(bookManager.uiMuted)
                                        }
                                    }

                                    // Reading progress bar inside metadata column
                                    Container {
                                        topMargin: bookManager.dp(12)
                                        horizontalAlignment: HorizontalAlignment.Fill
                                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                        Container {
                                            verticalAlignment: VerticalAlignment.Center
                                            layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                            preferredHeight: bookManager.dp(6)
                                            background: Color.create(bookManager.uiSurface)
                                            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }

                                            Container {
                                                preferredHeight: bookManager.dp(6)
                                                background: Color.create(bookManager.uiAccent)
                                                layoutProperties: StackLayoutProperties { spaceQuota: Math.max(0.001, bookDetailSheet.displayProgress) }
                                            }
                                            Container {
                                                preferredHeight: bookManager.dp(6)
                                                background: Color.Transparent
                                                layoutProperties: StackLayoutProperties { spaceQuota: Math.max(0.001, 100 - bookDetailSheet.displayProgress) }
                                            }
                                        }

                                        Label {
                                            leftMargin: bookManager.dp(8)
                                            verticalAlignment: VerticalAlignment.Center
                                            text: bookDetailSheet.displayProgress + "%"
                                            textStyle.base: SystemDefaults.TextStyles.SmallText
                                            textStyle.fontWeight: FontWeight.W500
                                            textStyle.color: Color.create(bookManager.uiAccent)
                                        }
                                    }
                                }
                            }

                            // Extended Metadata Grid (Series, Genre, Date Modified)
                            Container {
                                visible: bookDetailSheet.bookSeries !== "" || bookDetailSheet.displayGenre !== "" || bookDetailSheet.displayDateModified !== ""
                                topMargin: bookManager.dp(14)
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                Container {
                                    bottomMargin: bookManager.dp(12)
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    preferredHeight: 1
                                    background: Color.create(bookManager.uiHairline)
                                }

                                // Row: Bộ sách
                                Container {
                                    visible: bookDetailSheet.bookSeries !== ""
                                    bottomMargin: bookManager.dp(8)
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Label {
                                        text: qsTr("Bộ sách") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.W500
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                    }
                                    Label {
                                        topMargin: bookManager.dp(2)
                                        text: bookDetailSheet.bookSeries
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiInk)
                                        multiline: true
                                    }
                                }

                                // Row: Thể loại
                                Container {
                                    visible: bookDetailSheet.displayGenre !== ""
                                    bottomMargin: bookManager.dp(8)
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Label {
                                        text: qsTr("Thể loại") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.W500
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                    }
                                    Label {
                                        topMargin: bookManager.dp(2)
                                        text: bookDetailSheet.displayGenre
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiInk)
                                        multiline: true
                                        autoSize.maxLineCount: 3
                                    }
                                }

                                // Row: Ngày cập nhật
                                Container {
                                    visible: bookDetailSheet.displayDateModified !== ""
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                    Label {
                                        text: qsTr("Cập nhật") + Retranslate.onLanguageChanged
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.fontWeight: FontWeight.W500
                                        textStyle.color: Color.create(bookManager.uiMuted)
                                    }
                                    Label {
                                        topMargin: bookManager.dp(2)
                                        text: bookDetailSheet.displayDateModified
                                        textStyle.base: SystemDefaults.TextStyles.SmallText
                                        textStyle.color: Color.create(bookManager.uiInk)
                                    }
                                }
                            }

                            // Primary Read CTA (hidden inside Reader)
                            Button {
                                visible: !bookDetailSheet.hideReadButton
                                topMargin: bookManager.dp(14)
                                horizontalAlignment: HorizontalAlignment.Fill
                                appearance: ControlAppearance.Primary
                                imageSource: "asset:///icons/ic_reading.png"
                                text: qsTr("Đọc sách") + Retranslate.onLanguageChanged
                                onClicked: {
                                    var p = bookDetailSheet.displayRawPath;
                                    bookDetailSheet.close();
                                    bookDetailSheet.requestOpenBook(p);
                                }
                            }

                            // Next Volume in Series Button (when available)
                            Button {
                                visible: bookDetailSheet.nextSeriesInfo && bookDetailSheet.nextSeriesInfo.filePath !== undefined && bookDetailSheet.nextSeriesInfo.filePath !== ""
                                topMargin: bookManager.dp(8)
                                horizontalAlignment: HorizontalAlignment.Fill
                                text: (qsTr("Đọc tiếp tập kế") + (bookDetailSheet.nextSeriesInfo && bookDetailSheet.nextSeriesInfo.seriesIndex > 0 ? (" (#" + bookDetailSheet.nextSeriesInfo.seriesIndex + ")") : "")) + Retranslate.onLanguageChanged
                                onClicked: {
                                    var np = bookDetailSheet.nextSeriesInfo.filePath;
                                    bookDetailSheet.close();
                                    bookDetailSheet.requestOpenBook(np);
                                }
                            }
                        }
                    }

                    // =========================================================
                    // DESCRIPTION SECTION (Edge-to-edge Header + Card)
                    // =========================================================
                    Header {
                        title: qsTr("MÔ TẢ SÁCH") + Retranslate.onLanguageChanged
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        topPadding: bookManager.dp(10)

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(14)
                            bottomPadding: bookManager.dp(14)
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)

                            Label {
                                id: descLabel
                                visible: bookDetailSheet.displayDesc !== ""
                                text: bookDetailSheet.cleanDesc(bookDetailSheet.displayDesc)
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.lineHeight: 1.15
                                textStyle.color: Color.create(bookManager.uiInk)
                                multiline: true
                                autoSize.maxLineCount: bookDetailSheet.descExpanded ? 200 : 8
                            }

                            Label {
                                visible: descLabel.visible && descLabel.text.length > 240
                                topMargin: bookManager.dp(10)
                                text: (bookDetailSheet.descExpanded ? qsTr("Thu gọn") : qsTr("Xem thêm")) + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.fontWeight: FontWeight.W500
                                textStyle.color: Color.create(bookManager.uiAccent)
                                gestureHandlers: [
                                    TapHandler { onTapped: { bookDetailSheet.descExpanded = !bookDetailSheet.descExpanded; } }
                                ]
                            }

                            Label {
                                visible: bookDetailSheet.displayDesc === ""
                                multiline: true
                                text: qsTr("Chưa có mô tả cho cuốn sách này.") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.fontStyle: FontStyle.Italic
                                textStyle.color: Color.create(bookManager.uiMuted)
                            }
                        }

                        // Compact technical file path footer
                        Container {
                            visible: bookDetailSheet.displayRawPath !== ""
                            topMargin: bookManager.dp(12)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(10)
                            bottomPadding: bookManager.dp(10)
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)

                            Label {
                                text: bookDetailSheet.friendlyPathOf(bookDetailSheet.displayRawPath)
                                textStyle.base: SystemDefaults.TextStyles.SmallText
                                textStyle.color: Color.create(bookManager.uiMuted)
                                multiline: true
                            }
                        }

                        // Rating & Review Card
                        Container {
                            topMargin: bookManager.dp(12)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            Label {
                                text: qsTr("Đánh giá & Cảm nhận") + Retranslate.onLanguageChanged
                                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                                textStyle.fontWeight: FontWeight.Bold
                                textStyle.color: Color.create(bookManager.uiAccent)
                            }

                            Container {
                                topMargin: bookManager.dp(8)
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                                Button {
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: bookDetailSheet.currentRating >= 1 ? "★" : "☆"
                                    onClicked: { bookDetailSheet.currentRating = 1; bookManager.setBookRating(bookDetailSheet.displayRawPath, 1); }
                                }
                                Button {
                                    leftMargin: bookManager.dp(4)
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: bookDetailSheet.currentRating >= 2 ? "★" : "☆"
                                    onClicked: { bookDetailSheet.currentRating = 2; bookManager.setBookRating(bookDetailSheet.displayRawPath, 2); }
                                }
                                Button {
                                    leftMargin: bookManager.dp(4)
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: bookDetailSheet.currentRating >= 3 ? "★" : "☆"
                                    onClicked: { bookDetailSheet.currentRating = 3; bookManager.setBookRating(bookDetailSheet.displayRawPath, 3); }
                                }
                                Button {
                                    leftMargin: bookManager.dp(4)
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: bookDetailSheet.currentRating >= 4 ? "★" : "☆"
                                    onClicked: { bookDetailSheet.currentRating = 4; bookManager.setBookRating(bookDetailSheet.displayRawPath, 4); }
                                }
                                Button {
                                    leftMargin: bookManager.dp(4)
                                    layoutProperties: StackLayoutProperties { spaceQuota: 1.0 }
                                    text: bookDetailSheet.currentRating >= 5 ? "★" : "☆"
                                    onClicked: { bookDetailSheet.currentRating = 5; bookManager.setBookRating(bookDetailSheet.displayRawPath, 5); }
                                }
                            }

                            TextArea {
                                id: reviewInput
                                topMargin: bookManager.dp(8)
                                hintText: qsTr("Viết vài dòng cảm nhận về cuốn sách...") + Retranslate.onLanguageChanged
                                minHeight: bookManager.dp(80)
                            }

                            Button {
                                topMargin: bookManager.dp(8)
                                horizontalAlignment: HorizontalAlignment.Right
                                text: qsTr("Lưu cảm nhận") + Retranslate.onLanguageChanged
                                onClicked: {
                                    bookManager.setBookReview(bookDetailSheet.displayRawPath, reviewInput.text);
                                    detailToast.body = qsTr("Đã lưu đánh giá & cảm nhận") + Retranslate.onLanguageChanged;
                                    detailToast.show();
                                }
                            }
                        }

                        // Metadata Editor Card
                        Container {
                            topMargin: bookManager.dp(12)
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            topPadding: bookManager.dp(12)
                            bottomPadding: bookManager.dp(12)
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                            Button {
                                horizontalAlignment: HorizontalAlignment.Fill
                                text: (bookDetailSheet.showMetaEditor ? qsTr("Ẩn trình sửa thông tin") : qsTr("Sửa thông tin sách (Tên / Tác giả / Bộ)")) + Retranslate.onLanguageChanged
                                onClicked: { bookDetailSheet.showMetaEditor = !bookDetailSheet.showMetaEditor; }
                            }

                            Container {
                                visible: bookDetailSheet.showMetaEditor
                                topMargin: bookManager.dp(10)
                                horizontalAlignment: HorizontalAlignment.Fill
                                layout: StackLayout { orientation: LayoutOrientation.TopToBottom }

                                TextField {
                                    id: editTitleField
                                    hintText: qsTr("Tên sách") + Retranslate.onLanguageChanged
                                }
                                TextField {
                                    id: editAuthorField
                                    topMargin: bookManager.dp(6)
                                    hintText: qsTr("Tác giả") + Retranslate.onLanguageChanged
                                }
                                TextField {
                                    id: editSeriesField
                                    topMargin: bookManager.dp(6)
                                    hintText: qsTr("Bộ truyện (Series)") + Retranslate.onLanguageChanged
                                }
                                TextField {
                                    id: editSeriesIdxField
                                    topMargin: bookManager.dp(6)
                                    hintText: qsTr("Số tập (VD: 1)") + Retranslate.onLanguageChanged
                                    inputMode: TextFieldInputMode.NumbersAndPunctuation
                                }
                                Button {
                                    topMargin: bookManager.dp(10)
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    appearance: ControlAppearance.Primary
                                    text: qsTr("Lưu thay đổi") + Retranslate.onLanguageChanged
                                    onClicked: {
                                        var idxVal = parseInt(editSeriesIdxField.text, 10);
                                        if (isNaN(idxVal)) idxVal = 0;
                                        if (bookManager.updateBookMetadata(bookDetailSheet.displayRawPath, editTitleField.text, editAuthorField.text, editSeriesField.text, idxVal)) {
                                            if (editTitleField.text.trim() !== "") bookDetailSheet.bookTitle = editTitleField.text.trim();
                                            if (editAuthorField.text.trim() !== "") bookDetailSheet.bookAuthor = editAuthorField.text.trim();
                                            bookDetailSheet.bookSeries = editSeriesField.text.trim();
                                            bookDetailSheet.showMetaEditor = false;
                                            detailToast.body = qsTr("Đã cập nhật thông tin sách") + Retranslate.onLanguageChanged;
                                            detailToast.show();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    attachedObjects: [
        SystemToast {
            id: detailToast
            body: ""
        }
    ]
}
