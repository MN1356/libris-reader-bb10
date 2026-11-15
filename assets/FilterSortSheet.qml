import bb.cascades 1.3

Sheet {
    id: filterSortSheet

    Page {
        titleBar: TitleBar {
            title: qsTr("Filter & Sort") + Retranslate.onLanguageChanged
            dismissAction: ActionItem {
                title: qsTr("Xong") + Retranslate.onLanguageChanged
                onTriggered: {
                    filterSortSheet.close();
                }
            }
        }

        ScrollView {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill

            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: bookManager.appTheme === "dark" ? Color.create("#000000") : Color.create("#FFFFFF")
                topPadding: 20
                leftPadding: 24
                rightPadding: 24
                bottomPadding: 32

                // --- 1. Tìm kiếm nhanh ---
                Header {
                    title: qsTr("TÌM KIẾM NHANH") + Retranslate.onLanguageChanged
                }

                TextField {
                    id: searchInput
                    topMargin: 8
                    hintText: qsTr("Nhập tên sách hoặc tác giả...") + Retranslate.onLanguageChanged
                    text: bookManager.searchQuery
                    inputMode: TextFieldInputMode.Text
                    onTextChanging: {
                        bookManager.setSearchQuery(text);
                    }
                }

                Button {
                    visible: searchInput.text.length > 0
                    topMargin: 6
                    text: qsTr("Xóa tìm kiếm") + Retranslate.onLanguageChanged
                    horizontalAlignment: HorizontalAlignment.Right
                    onClicked: {
                        searchInput.text = "";
                        bookManager.setSearchQuery("");
                    }
                }

                // --- 2. Sắp xếp danh sách ---
                Header {
                    topMargin: 24
                    title: qsTr("SẮP XẾP SÁCH THEO") + Retranslate.onLanguageChanged
                }

                DropDown {
                    id: sortDropDown
                    topMargin: 8
                    title: qsTr("Tiêu chí") + Retranslate.onLanguageChanged

                    Option {
                        text: qsTr("Ngày sửa đổi: Mới nhất") + Retranslate.onLanguageChanged
                        value: "date_desc"
                        selected: bookManager.sortBy === "date_desc"
                    }
                    Option {
                        text: qsTr("Ngày sửa đổi: Cũ nhất") + Retranslate.onLanguageChanged
                        value: "date_asc"
                        selected: bookManager.sortBy === "date_asc"
                    }
                    Option {
                        text: qsTr("Tên sách: A đến Z") + Retranslate.onLanguageChanged
                        value: "title_asc"
                        selected: bookManager.sortBy === "title_asc"
                    }
                    Option {
                        text: qsTr("Tên sách: Z đến A") + Retranslate.onLanguageChanged
                        value: "title_desc"
                        selected: bookManager.sortBy === "title_desc"
                    }
                    Option {
                        text: qsTr("Tác giả: A đến Z") + Retranslate.onLanguageChanged
                        value: "author_asc"
                        selected: bookManager.sortBy === "author_asc"
                    }
                    Option {
                        text: qsTr("Dung lượng: Lớn nhất") + Retranslate.onLanguageChanged
                        value: "size_desc"
                        selected: bookManager.sortBy === "size_desc"
                    }

                    onSelectedValueChanged: {
                        if (selectedValue && selectedValue !== bookManager.sortBy) {
                            bookManager.setSortBy(selectedValue);
                        }
                    }
                }

                // --- 2.1. Phân nhóm sách (Group by Author / Series) ---
                Header {
                    topMargin: 24
                    title: qsTr("NHÓM SÁCH THEO") + Retranslate.onLanguageChanged
                }

                DropDown {
                    id: groupBySelector
                    topMargin: 8
                    title: qsTr("Phân nhóm:") + Retranslate.onLanguageChanged

                    Option {
                        text: qsTr("Không phân nhóm") + Retranslate.onLanguageChanged
                        value: "none"
                        selected: bookManager.groupBy === "none" || bookManager.groupBy === ""
                    }
                    Option {
                        text: qsTr("Theo Tác giả") + Retranslate.onLanguageChanged
                        value: "author"
                        selected: bookManager.groupBy === "author"
                    }
                    Option {
                        text: qsTr("Theo Series") + Retranslate.onLanguageChanged
                        value: "series"
                        selected: bookManager.groupBy === "series"
                    }

                    onSelectedValueChanged: {
                        if (selectedValue && selectedValue !== bookManager.groupBy) {
                            bookManager.setGroupBy(selectedValue);
                        }
                    }
                }


                // --- 3. Lọc theo bộ sưu tập / trạng thái ---
                Header {
                    topMargin: 24
                    title: qsTr("BỘ SƯU TẬP & TRẠNG THÁI") + Retranslate.onLanguageChanged
                }

                DropDown {
                    id: categorySelector
                    topMargin: 8
                    title: qsTr("Hiển thị:") + Retranslate.onLanguageChanged

                    Option {
                        text: qsTr("Tủ sách (Đang đọc & Hoàn thành)") + Retranslate.onLanguageChanged
                        value: "all"
                        selected: bookManager.categoryFilter === "all" || bookManager.categoryFilter === ""
                    }
                    Option {
                        text: qsTr("Chỉ sách đang đọc") + Retranslate.onLanguageChanged
                        value: "reading"
                        selected: bookManager.categoryFilter === "reading"
                    }
                    Option {
                        text: qsTr("Tất cả sách trong máy") + Retranslate.onLanguageChanged
                        value: "all_scanned"
                        selected: bookManager.categoryFilter === "all_scanned"
                    }
                    Option {
                        text: qsTr("Sách yêu thích") + Retranslate.onLanguageChanged
                        value: "favorites"
                        selected: bookManager.categoryFilter === "favorites"
                    }
                    Option {
                        text: qsTr("Danh sách đọc") + Retranslate.onLanguageChanged
                        value: "readingList"
                        selected: bookManager.categoryFilter === "readingList"
                    }
                    Option {
                        text: qsTr("Đã hoàn thành") + Retranslate.onLanguageChanged
                        value: "completed"
                        selected: bookManager.categoryFilter === "completed"
                    }
                    Option {
                        text: qsTr("Chưa đọc") + Retranslate.onLanguageChanged
                        value: "unread"
                        selected: bookManager.categoryFilter === "unread"
                    }

                    onSelectedValueChanged: {
                        if (selectedValue && selectedValue !== bookManager.categoryFilter) {
                            bookManager.setCategoryFilter(selectedValue);
                        }
                    }
                }

                // --- 4. Lọc theo định dạng file ---
                Header {
                    topMargin: 24
                    title: qsTr("LỌC ĐỊNH DẠNG FILE") + Retranslate.onLanguageChanged
                }

                DropDown {
                    id: formatSelector
                    topMargin: 8
                    title: qsTr("Định dạng:") + Retranslate.onLanguageChanged

                    Option {
                        text: qsTr("Tất cả") + Retranslate.onLanguageChanged
                        value: "all"
                        selected: bookManager.formatFilter === "all" || bookManager.formatFilter === ""
                    }
                    Option {
                        text: "EPUB (.epub)"
                        value: "epub"
                        selected: bookManager.formatFilter === "epub"
                    }
                    Option {
                        text: "PDF (.pdf)"
                        value: "pdf"
                        selected: bookManager.formatFilter === "pdf"
                    }
                    Option {
                        text: "TXT (.txt)"
                        value: "txt"
                        selected: bookManager.formatFilter === "txt"
                    }
                    Option {
                        text: "FB2 (.fb2)"
                        value: "fb2"
                        selected: bookManager.formatFilter === "fb2"
                    }

                    onSelectedValueChanged: {
                        if (selectedValue && selectedValue !== bookManager.formatFilter) {
                            bookManager.setFormatFilter(selectedValue);
                        }
                    }
                }

                // --- Thống kê kết quả ---
                Container {
                    topMargin: 20
                    horizontalAlignment: HorizontalAlignment.Fill
                    background: bookManager.appTheme === "dark" ? Color.create("#141414") : Color.create("#F5F5F5")
                    leftPadding: 16
                    rightPadding: 16
                    topPadding: 12
                    bottomPadding: 12

                    Label {
                        horizontalAlignment: HorizontalAlignment.Center
                        text: ((qsTr("Đang hiển thị") + " " + bookManager.bookCount + " / " + bookManager.totalScannedBooks) + Retranslate.onLanguageChanged)
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: bookManager.appTheme === "dark" ? Color.White : Color.create("#000000")
                    }
                }
            }
        }
    }
}
