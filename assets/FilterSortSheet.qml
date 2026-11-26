import bb.cascades 1.3

Sheet {
    id: filterSortSheet

    Page {
        titleBar: TitleBar {
            title: qsTr("Lọc & Sắp xếp") + Retranslate.onLanguageChanged
            appearance: TitleBarAppearance.Plain
            dismissAction: ActionItem {
                title: qsTr("Xong") + Retranslate.onLanguageChanged
                onTriggered: {
                    filterSortSheet.close();
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

                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    bottomPadding: bookManager.dp(32)

                    // --- 1. Tìm kiếm nhanh ---
                    Header {
                        title: qsTr("TÌM KIẾM NHANH") + Retranslate.onLanguageChanged
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        topPadding: bookManager.dp(10)
                        bottomPadding: bookManager.dp(16)

                        TextField {
                            id: searchInput
                            hintText: qsTr("Nhập tên sách hoặc tác giả...") + Retranslate.onLanguageChanged
                            clearButtonVisible: true
                            text: bookManager.searchQuery
                            inputMode: TextFieldInputMode.Text
                            onTextChanging: {
                                bookManager.setSearchQuery(text);
                            }
                        }
                    }

                    // --- 2. Sắp xếp danh sách ---
                    Header {
                        title: qsTr("SẮP XẾP SÁCH THEO") + Retranslate.onLanguageChanged
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        topPadding: bookManager.dp(10)
                        bottomPadding: bookManager.dp(16)

                        DropDown {
                            id: sortDropDown
                            title: qsTr("Tiêu chí") + Retranslate.onLanguageChanged

                            Option { text: qsTr("Ngày sửa đổi: Mới nhất") + Retranslate.onLanguageChanged; value: "date_desc"; selected: bookManager.sortBy === "date_desc" }
                            Option { text: qsTr("Ngày sửa đổi: Cũ nhất") + Retranslate.onLanguageChanged; value: "date_asc"; selected: bookManager.sortBy === "date_asc" }
                            Option { text: qsTr("Tên sách: A đến Z") + Retranslate.onLanguageChanged; value: "title_asc"; selected: bookManager.sortBy === "title_asc" }
                            Option { text: qsTr("Tên sách: Z đến A") + Retranslate.onLanguageChanged; value: "title_desc"; selected: bookManager.sortBy === "title_desc" }
                            Option { text: qsTr("Tác giả: A đến Z") + Retranslate.onLanguageChanged; value: "author_asc"; selected: bookManager.sortBy === "author_asc" }
                            Option { text: qsTr("Dung lượng: Lớn nhất") + Retranslate.onLanguageChanged; value: "size_desc"; selected: bookManager.sortBy === "size_desc" }

                            onSelectedValueChanged: {
                                if (selectedValue && selectedValue !== bookManager.sortBy) {
                                    bookManager.setSortBy(selectedValue);
                                }
                            }
                        }
                    }

                    // --- 3. Nhóm sách theo ---
                    Header {
                        title: qsTr("NHÓM SÁCH THEO") + Retranslate.onLanguageChanged
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        topPadding: bookManager.dp(10)
                        bottomPadding: bookManager.dp(16)

                        DropDown {
                            id: groupBySelector
                            title: qsTr("Phân nhóm:") + Retranslate.onLanguageChanged

                            Option { text: qsTr("Không phân nhóm") + Retranslate.onLanguageChanged; value: "none"; selected: bookManager.groupBy === "none" || bookManager.groupBy === "" }
                            Option { text: qsTr("Theo Tác giả") + Retranslate.onLanguageChanged; value: "author"; selected: bookManager.groupBy === "author" }
                            Option { text: qsTr("Theo Series") + Retranslate.onLanguageChanged; value: "series"; selected: bookManager.groupBy === "series" }

                            onSelectedValueChanged: {
                                if (selectedValue && selectedValue !== bookManager.groupBy) {
                                    bookManager.setGroupBy(selectedValue);
                                }
                            }
                        }
                    }

                    // --- 4. Bộ sưu tập & Trạng thái ---
                    Header {
                        title: qsTr("BỘ SƯU TẬP & TRẠNG THÁI") + Retranslate.onLanguageChanged
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        topPadding: bookManager.dp(10)
                        bottomPadding: bookManager.dp(16)

                        DropDown {
                            id: categorySelector
                            title: qsTr("Hiển thị:") + Retranslate.onLanguageChanged

                            Option { text: qsTr("Tủ sách (Đang đọc & Hoàn thành)") + Retranslate.onLanguageChanged; value: "all"; selected: bookManager.categoryFilter === "all" || bookManager.categoryFilter === "" }
                            Option { text: qsTr("Chỉ sách đang đọc") + Retranslate.onLanguageChanged; value: "reading"; selected: bookManager.categoryFilter === "reading" }
                            Option { text: qsTr("Tất cả sách trong máy") + Retranslate.onLanguageChanged; value: "all_scanned"; selected: bookManager.categoryFilter === "all_scanned" }
                            Option { text: qsTr("Sách yêu thích") + Retranslate.onLanguageChanged; value: "favorites"; selected: bookManager.categoryFilter === "favorites" }
                            Option { text: qsTr("Danh sách đọc") + Retranslate.onLanguageChanged; value: "readingList"; selected: bookManager.categoryFilter === "readingList" }
                            Option { text: qsTr("Đã hoàn thành") + Retranslate.onLanguageChanged; value: "completed"; selected: bookManager.categoryFilter === "completed" }
                            Option { text: qsTr("Chưa đọc") + Retranslate.onLanguageChanged; value: "unread"; selected: bookManager.categoryFilter === "unread" }

                            onSelectedValueChanged: {
                                if (selectedValue && selectedValue !== bookManager.categoryFilter) {
                                    bookManager.setCategoryFilter(selectedValue);
                                }
                            }
                        }
                    }

                    // --- 5. Lọc định dạng file ---
                    Header {
                        title: qsTr("LỌC ĐỊNH DẠNG FILE") + Retranslate.onLanguageChanged
                    }

                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        topPadding: bookManager.dp(10)
                        bottomPadding: bookManager.dp(16)

                        DropDown {
                            id: formatSelector
                            title: qsTr("Định dạng:") + Retranslate.onLanguageChanged

                            Option { text: qsTr("Tất cả") + Retranslate.onLanguageChanged; value: "all"; selected: bookManager.formatFilter === "all" || bookManager.formatFilter === "" }
                            Option { text: "EPUB (.epub)"; value: "epub"; selected: bookManager.formatFilter === "epub" }
                            Option { text: "PDF (.pdf)"; value: "pdf"; selected: bookManager.formatFilter === "pdf" }
                            Option { text: "TXT (.txt)"; value: "txt"; selected: bookManager.formatFilter === "txt" }
                            Option { text: "FB2 (.fb2)"; value: "fb2"; selected: bookManager.formatFilter === "fb2" }
                            Option { text: "DJVU (.djvu)"; value: "djvu"; selected: bookManager.formatFilter === "djvu" }
                            Option { text: "CBZ (.cbz)"; value: "cbz"; selected: bookManager.formatFilter === "cbz" }
                            Option { text: "MOBI / AZW3"; value: "mobi"; selected: bookManager.formatFilter === "mobi" }

                            onSelectedValueChanged: {
                                if (selectedValue && selectedValue !== bookManager.formatFilter) {
                                    bookManager.setFormatFilter(selectedValue);
                                }
                            }
                        }
                    }

                    // --- Thống kê kết quả ---
                    Container {
                        horizontalAlignment: HorizontalAlignment.Fill
                        leftPadding: bookManager.dp(16)
                        rightPadding: bookManager.dp(16)
                        topPadding: bookManager.dp(4)

                        Container {
                            horizontalAlignment: HorizontalAlignment.Fill
                            background: Color.create(bookManager.uiCard)
                            leftPadding: bookManager.dp(16)
                            rightPadding: bookManager.dp(16)
                            topPadding: bookManager.dp(14)
                            bottomPadding: bookManager.dp(14)

                            Label {
                                horizontalAlignment: HorizontalAlignment.Center
                                text: ((qsTr("Đang hiển thị") + " " + bookManager.bookCount + " / " + bookManager.totalScannedBooks) + Retranslate.onLanguageChanged)
                                textStyle.base: SystemDefaults.TextStyles.BodyText
                                textStyle.fontWeight: FontWeight.W500
                                textStyle.color: Color.create(bookManager.uiAccent)
                            }
                        }
                    }
                }
            }
        }
    }
}
