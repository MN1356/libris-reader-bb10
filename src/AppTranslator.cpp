#include "AppTranslator.hpp"

AppTranslator::AppTranslator(QObject *parent)
    : QTranslator(parent)
    , m_language("en")
{
    initTranslations();
}

void AppTranslator::add(const char *source, const char *en, const char *vi)
{
    QString s = QString::fromUtf8(source);
    QString e = QString::fromUtf8(en);
    QString v = QString::fromUtf8(vi);
    m_en[s] = e;
    m_vi[s] = v;
    // Also allow reverse lookup if source is in English or Vietnamese
    m_en[e] = e;
    m_vi[e] = v;
    m_en[v] = e;
    m_vi[v] = v;

    // Also store Latin-1 raw byte keys so lookups succeed regardless of Qt QML caller encoding
    QString s_lat = QString::fromLatin1(source);
    QString e_lat = QString::fromLatin1(en);
    QString v_lat = QString::fromLatin1(vi);
    m_en[s_lat] = e;
    m_vi[s_lat] = v;
    m_en[e_lat] = e;
    m_vi[e_lat] = v;
    m_en[v_lat] = e;
    m_vi[v_lat] = v;
}

void AppTranslator::initTranslations()
{
    // Navigation & General
    add("Tủ Sách", "Bookshelf", "Tủ Sách");
    add("Kệ Sách", "Bookshelf", "Kệ Sách");
    add("Hoàn thành", "Finished", "Hoàn thành");
    add("Finished", "Finished", "Hoàn thành");
    add("Chưa có sách trong Tủ Sách", "No books in Bookshelf", "Chưa có sách trong Tủ Sách");
    add("Tổng Quan", "Overview", "Tổng Quan");
    add("Tổng Quan Đã Đọc", "Reading Overview", "Tổng Quan Đã Đọc");
    add("Duyệt Thư Mục", "File Browser", "Duyệt Thư Mục");
    add("Sách Đang Đọc", "Currently Reading", "Sách Đang Đọc");
    add("Sách đang đọc", "Currently reading", "Sách đang đọc");
    add("Chưa có sách nào đang đọc", "No books currently being read", "Chưa có sách nào đang đọc");
    add("Hãy chọn sách từ mục 'Duyệt Thư Mục' để bắt đầu đọc.", "Choose a book from 'File Browser' to start reading.", "Hãy chọn sách từ mục 'Duyệt Thư Mục' để bắt đầu đọc.");
    add("Danh mục", "Menu", "Danh mục");
    add("DANH MỤC", "MENU", "DANH MỤC");
    add("CÔNG CỤ & TIỆN ÍCH", "TOOLS & UTILITIES", "CÔNG CỤ & TIỆN ÍCH");
    add("ĐIỀU HƯỚNG", "NAVIGATION", "ĐIỀU HƯỚNG");
    add("CHẾ ĐỘ XEM", "VIEW MODE", "CHẾ ĐỘ XEM");
    add("TIỆN ÍCH", "UTILITIES", "TIỆN ÍCH");
    add("Tìm kiếm sách", "Search Books", "Tìm kiếm sách");
    add("Tổng Quan Đọc", "Reading Overview", "Tổng Quan Đọc");
    add("Tìm theo tên sách hoặc tác giả...", "Search by title or author...", "Tìm theo tên sách hoặc tác giả...");
    add("Kết quả", "Results", "Kết quả");
    add("Trình đọc sách BlackBerry 10", "BlackBerry 10 E-Book Reader", "Trình đọc sách BlackBerry 10");
    add("Native e-Book Reader", "Native e-Book Reader", "Trình đọc sách Native");
    add("Bộ nhớ & Thẻ nhớ", "Internal & SD Card", "Bộ nhớ & Thẻ nhớ");
    add("Bộ nhớ trong & Thẻ nhớ", "Internal & SD Card", "Bộ nhớ trong & Thẻ nhớ");
    add("← Lùi lại", "← Up", "← Lùi lại");
    add("Lùi lại", "Back", "Lùi lại");
    add("mục", "items", "mục");
    add("cuốn", "books", "cuốn");
    add("Thẻ nhớ ngoài", "MicroSD Card", "Thẻ nhớ ngoài");
    add("Thẻ nhớ ngoài (MicroSD)", "MicroSD Card Storage", "Thẻ nhớ ngoài (MicroSD)");
    add("Thẻ nhớ cắm ngoài", "External memory card", "Thẻ nhớ cắm ngoài");
    add("Thư mục Sách", "Books Folder", "Thư mục Sách");
    add("Tài liệu", "Documents", "Tài liệu");
    add("Tải về", "Downloads", "Tải về");
    add("Bộ nhớ trong", "Internal Storage", "Bộ nhớ trong");
    add("Bộ nhớ thiết bị (Internal storage)", "Internal Device Storage", "Bộ nhớ thiết bị (Internal storage)");
    add("Bộ nhớ máy", "Device Storage", "Bộ nhớ máy");
    add("Trở về thư mục trước", "Go up one level", "Trở về thư mục trước");
    add("Toàn bộ thư mục chia sẻ", "All shared storage", "Toàn bộ thư mục chia sẻ");
    add("Đang nạp và quét thư viện sách...", "Loading and scanning library...", "Đang nạp và quét thư viện sách...");
    add("Đang mở sách...", "Opening book...", "Đang mở sách...");
    add("Vui lòng chờ trong giây lát...", "Please wait a moment...", "Vui lòng chờ trong giây lát...");
    add("Đang hiển thị nội dung...", "Rendering content...", "Đang hiển thị nội dung...");
    add("Đang hiển thị trang sách...", "Rendering page...", "Đang hiển thị trang sách...");
    add("Cài Đặt", "Settings", "Cài Đặt");
    add("Cài Đặt Ứng Dụng", "App Settings", "Cài Đặt Ứng Dụng");
    add("Giới thiệu", "About", "Giới thiệu");
    add("Giới Thiệu", "About", "Giới Thiệu");
    add("Tải Offline", "Download Offline", "Tải Offline");
    add("Xong", "Done", "Xong");
    add("Hủy", "Cancel", "Hủy");
    add("Thoát", "Exit", "Thoát");
    add("Thoát ứng dụng", "Exit application", "Thoát ứng dụng");
    add("Thoát Ứng Dụng", "Exit Application", "Thoát Ứng Dụng");
    add("Thoát Libris", "Exit Libris", "Thoát Libris");
    add("Bạn có muốn lưu tiến độ đọc và thoát hoàn toàn ứng dụng không?", "Do you want to save reading progress and exit the application?", "Bạn có muốn lưu tiến độ đọc và thoát hoàn toàn ứng dụng không?");
    add("Lưu tiến độ và đóng hoàn toàn Libris (tránh chạy nền)", "Save progress and completely close Libris (prevent background running)", "Lưu tiến độ và đóng hoàn toàn Libris (tránh chạy nền)");
    add("Lưu tiến độ và đóng hoàn toàn ứng dụng?", "Save progress and completely close application?", "Lưu tiến độ và đóng hoàn toàn ứng dụng?");
    add("Đóng", "Close", "Đóng");
    add("Quay lại", "Back", "Quay lại");
    add("Lưu", "Save", "Lưu");
    add("Chạm màn hình để hủy", "Tap to dismiss", "Chạm màn hình để hủy");

    // Library Actions & Views
    add("Xem Chi Tiết", "Detailed View", "Xem Chi Tiết");
    add("Xem Lưới", "Mosaic View", "Xem Lưới");
    add("Xem: Danh sách", "View: Detailed List", "Xem: Danh sách");
    add("Xem: Lưới bìa", "View: Mosaic Grid", "Xem: Lưới bìa");
    add("Chế độ: Danh sách (chạm đổi)", "View: Detailed List (tap to toggle)", "Chế độ: Danh sách (chạm đổi)");
    add("Chế độ: Lưới bìa (chạm đổi)", "View: Mosaic Grid (tap to toggle)", "Chế độ: Lưới bìa (chạm đổi)");
    add("Lưới", "Mosaic", "Lưới");
    add("Chi tiết", "Detailed", "Chi tiết");
    add("Danh sách", "List", "Danh sách");
    add("Lưới bìa", "Mosaic", "Lưới bìa");
    add("Tất cả sách", "All Books", "Tất cả sách");
    add("Mới", "New", "Mới");
    add("Lọc & Sắp xếp", "Filter & Sort", "Lọc & Sắp xếp");
    add("Mở file...", "Open file...", "Mở file...");
    add("Mở file từ thiết bị...", "Open file from device...", "Mở file từ thiết bị...");
    add("Quét lại", "Rescan", "Quét lại");
    add("Quét lại sách", "Rescan library", "Quét lại sách");
    add("Chưa có sách nào", "No books found", "Chưa có sách nào");
    add("Hãy sao chép file sách vào thẻ nhớ microSD hoặc bộ nhớ trong, sau đó bấm 'Quét lại' hoặc chuyển sang tab 'Duyệt Thư Mục'.",
        "Copy e-books into microSD card or internal storage, then tap 'Rescan' or switch to 'File Browser'.",
        "Hãy sao chép file sách vào thẻ nhớ microSD hoặc bộ nhớ trong, sau đó bấm 'Quét lại' hoặc chuyển sang tab 'Duyệt Thư Mục'.");
    add("Hãy sao chép file sách (.epub, .txt, .fb2, .pdf, .djvu, .cbz, .mobi) vào thẻ nhớ microSD (thư mục Book, books...) hoặc thư mục documents, sau đó bấm 'Quét lại' hoặc 'Mở file...'.",
        "Copy e-books (.epub, .txt, .fb2, .pdf, .djvu, .cbz, .mobi) to microSD card or documents folder, then tap 'Rescan' or 'Open file...'.",
        "Hãy sao chép file sách (.epub, .txt, .fb2, .pdf, .djvu, .cbz, .mobi) vào thẻ nhớ microSD (thư mục Book, books...) hoặc thư mục documents, sau đó bấm 'Quét lại' hoặc 'Mở file...'.");
    add("Không tìm thấy sách phù hợp", "No matching books", "Không tìm thấy sách phù hợp");
    add("Không có cuốn sách nào khớp với từ khóa tìm kiếm hoặc bộ lọc định dạng hiện tại.",
        "No books match the current search keyword or format filter.",
        "Không có cuốn sách nào khớp với từ khóa tìm kiếm hoặc bộ lọc định dạng hiện tại.");
    add("Đặt lại bộ lọc & tìm kiếm", "Reset filters & search", "Đặt lại bộ lọc & tìm kiếm");
    add("Sửa đổi: ", "Modified: ", "Sửa đổi: ");
    add("Không rõ", "Unknown", "Không rõ");
    add("Văn bản TXT", "TXT Document", "Văn bản TXT");
    add("Tài liệu PDF", "PDF Document", "Tài liệu PDF");
    add("Tài liệu DjVu", "DjVu Document", "Tài liệu DjVu");
    add("Truyện tranh CBZ", "CBZ Comic Book", "Truyện tranh CBZ");
    add("Sách điện tử MOBI", "MOBI eBook", "Sách điện tử MOBI");
    add("EPUB eBook", "EPUB eBook", "EPUB eBook");
    add("FB2 FictionBook", "FB2 FictionBook", "FB2 FictionBook");

    // Filter & Sort
    add("Filter & Sort", "Filter & Sort", "Lọc & Sắp xếp");
    add("Lọc & Sắp Xếp Tủ Sách", "Filter & Sort Books", "Lọc & Sắp Xếp Tủ Sách");
    add("Tủ sách (Đang đọc & Hoàn thành)", "Bookshelf (Reading & Finished)", "Tủ sách (Đang đọc & Hoàn thành)");
    add("Chỉ sách đang đọc", "Currently Reading only", "Chỉ sách đang đọc");
    add("NHÓM SÁCH THEO", "GROUP BOOKS BY", "NHÓM SÁCH THEO");
    add("Phân nhóm:", "Group by:", "Phân nhóm:");
    add("Không phân nhóm (Mặc định)", "No grouping (Default)", "Không phân nhóm (Mặc định)");
    add("Theo Tác giả (Author)", "By Author", "Theo Tác giả (Author)");
    add("Theo Series (Bộ sách)", "By Series", "Theo Series (Bộ sách)");
    add("Theo Tác giả", "By Author", "Theo Tác giả");
    add("Theo Series", "By Series", "Theo Series");
    add(" • Theo Tác giả", " • By Author", " • Theo Tác giả");
    add(" • Theo Series", " • By Series", " • Theo Series");
    add("Sách đơn (Không thuộc series)", "Standalone Books", "Sách đơn (Không thuộc series)");
    add("Bộ sách", "Series", "Bộ sách");
    add("TÌM KIẾM NHANH", "QUICK SEARCH", "TÌM KIẾM NHANH");
    add("Nhập tên sách hoặc tác giả...", "Search title or author...", "Nhập tên sách hoặc tác giả...");
    add("Xóa tìm kiếm", "Clear search", "Xóa tìm kiếm");
    add("SẮP XẾP SÁCH THEO", "SORT BOOKS BY", "SẮP XẾP SÁCH THEO");
    add("Tiêu chí", "Criteria", "Tiêu chí");
    add("Ngày sửa đổi (Mới nhất trước)", "Date modified (Newest first)", "Ngày sửa đổi (Mới nhất trước)");
    add("Ngày sửa đổi (Cũ nhất trước)", "Date modified (Oldest first)", "Ngày sửa đổi (Cũ nhất trước)");
    add("Tên sách (A → Z)", "Title (A → Z)", "Tên sách (A → Z)");
    add("Tên sách (Z → A)", "Title (Z → A)", "Tên sách (Z → A)");
    add("Tác giả (A → Z)", "Author (A → Z)", "Tác giả (A → Z)");
    add("Dung lượng file (Lớn → Nhỏ)", "File size (Largest first)", "Dung lượng file (Lớn → Nhỏ)");
    add("LỌC ĐỊNH DẠNG FILE", "FILTER BY FORMAT", "LỌC ĐỊNH DẠNG FILE");
    add("Tất cả", "All", "Tất cả");
    add("CHẾ ĐỘ HIỂN THỊ", "DISPLAY MODE", "CHẾ ĐỘ HIỂN THỊ");
    add("Lưới bìa (Mosaic)", "Mosaic (Grid)", "Lưới bìa");
    add("Chi tiết (Detailed)", "Detailed List", "Chi tiết (Detailed)");
    add("Danh sách chi tiết (Detailed)", "Detailed List", "Danh sách chi tiết (Detailed)");
    add("Bộ lọc và thứ tự sắp xếp được lưu tự động cho các lần mở sau.",
        "Filters and sort preferences are automatically saved.",
        "Bộ lọc và thứ tự sắp xếp được lưu tự động cho các lần mở sau.");

    // Stats & Overview
    add("THỐNG KÊ THỜI GIAN ĐỌC", "READING TIME STATS", "THỐNG KÊ THỜI GIAN ĐỌC");
    add("Hôm nay", "Today", "Hôm nay");
    add("Tổng thời gian", "Total Time", "Tổng thời gian");
    add("Thời gian đọc hôm nay", "Today's Reading Time", "Thời gian đọc hôm nay");
    add("Tổng thời gian đọc", "Total Reading Time", "Tổng thời gian đọc");
    add("Duy trì thói quen đọc sách mỗi ngày", "Keep up your daily reading habit", "Duy trì thói quen đọc sách mỗi ngày");
    add("LƯỢNG ĐỌC & TIẾN ĐỘ", "READING VOLUME & PROGRESS", "LƯỢNG ĐỌC & TIẾN ĐỘ");
    add("Trong thư viện", "In Library", "Trong thư viện");
    add("Đang đọc", "Reading", "Đang đọc");
    add("Đã đọc xong", "Completed", "Đã đọc xong");
    add("Tiến độ trung bình", "Average Progress", "Tiến độ trung bình");
    add("Tỷ lệ hoàn thành tủ sách", "Library Completion Rate", "Tỷ lệ hoàn thành tủ sách");
    add("DANH SÁCH SÁCH ĐÃ ĐỌC", "READING HISTORY", "DANH SÁCH SÁCH ĐÃ ĐỌC");
    add("Chưa có lịch sử đọc sách", "No reading history yet", "Chưa có lịch sử đọc sách");
    add("Hãy mở một cuốn sách từ 'Kệ Sách' để bắt đầu ghi nhận thời gian và tiến độ đọc!",
        "Open a book from 'Bookshelf' to start tracking your reading time and progress!",
        "Hãy mở một cuốn sách từ 'Kệ Sách' để bắt đầu ghi nhận thời gian và tiến độ đọc!");
    add("Thời gian đọc: ", "Time read: ", "Thời gian đọc: ");
    add("Đọc tiếp", "Continue", "Đọc tiếp");
    add("giờ", "hours", "giờ");
    add("phút", "minutes", "phút");
    add("giây", "seconds", "giây");
    add("Dưới 1 phút", "Less than 1 min", "Dưới 1 phút");

    // Book Details & Info
    add("Thông tin sách", "Book Details", "Thông tin sách");
    add("Thông tin chi tiết", "Details", "Thông tin chi tiết");
    add("Thể loại", "Genre", "Thể loại");
    add("Mô tả sách", "Description", "Mô tả sách");
    add("Xem thêm", "Show more", "Xem thêm");
    add("Thu gọn", "Show less", "Thu gọn");
    add("Chưa xác định", "Not specified", "Chưa xác định");
    add("Chưa có mô tả cho cuốn sách này.", "No description available for this book.", "Chưa có mô tả cho cuốn sách này.");
    add("Đường dẫn file", "File Path", "Đường dẫn file");
    add("chương", "chapters", "chương");
    add("trang", "pages", "trang");
    add("Tiến độ đọc", "Reading Progress", "Tiến độ đọc");
    add("THÔNG TIN XUẤT BẢN & TẬP TIN", "PUBLICATION & FILE INFO", "THÔNG TIN XUẤT BẢN & TẬP TIN");

    // Reading & Reader actions
    add("Trang trước", "Previous", "Trang trước");
    add("Trang sau", "Next", "Trang sau");
    add("Chương trước", "Previous Chapter", "Chương trước");
    add("Chương sau", "Next Chapter", "Chương sau");
    add("Cài đặt", "Settings", "Cài đặt");
    add("Cài đặt đọc sách", "Reader Settings", "Cài đặt đọc sách");
    add("Đổi màu nền", "Theme", "Đổi màu nền");
    add("Đổi giao diện", "Theme", "Đổi giao diện");
    add("Toàn màn hình", "Full screen", "Toàn màn hình");
    add("Hiện thanh công cụ", "Exit full screen", "Hiện thanh công cụ");
    add("Màu sáng", "Light", "Màu sáng");
    add("Màu tối (Đảo màu)", "Invert Colors", "Màu tối (Đảo màu)");
    add("Chế độ: Lật trang", "Mode: Paged", "Chế độ: Lật trang");
    add("Chế độ: Cuộn dọc", "Mode: Scroll", "Chế độ: Cuộn dọc");
    add("Chế độ văn bản (Reflow OCR)", "Text Reflow (OCR)", "PDF Reflow (Văn bản)");
    add("Chế độ xem PDF gốc", "Original PDF View", "PDF gốc");
    add("Xoay 90°", "Rotate 90°", "Xoay 90°");
    add("Phóng to +", "Zoom in +", "Phóng to +");
    add("Thu nhỏ -", "Zoom out -", "Thu nhỏ -");
    add("Cỡ chữ +", "Font size +", "Cỡ chữ +");
    add("Cỡ chữ -", "Font size -", "Cỡ chữ -");
    add("Chương", "Chapter", "Chương");
    add("Chương ", "Chapter ", "Chương ");
    add("Trang", "Page", "Trang");
    add("Trang ", "Page ", "Trang ");
    add("Bìa sách", "Cover Page", "Bìa sách");
    add("Tìm", "Find", "Tìm");

    // TTS Voice Reader
    add("Đọc sách", "Read aloud", "Đọc sách");
    add("Đọc sách (TTS)", "Read Aloud (TTS)", "Đọc sách (TTS)");
    add("Dừng đọc", "Stop reading", "Dừng đọc");
    add("Dừng đọc TTS", "Stop TTS", "Dừng đọc TTS");
    add("Tải nghe Offline", "Download Offline", "Tải nghe Offline");
    add("Đang tải dữ liệu...", "Loading audio...", "Đang tải dữ liệu...");
    add("Tốc độ", "Speed", "Tốc độ");
    add("GIỌNG ĐỌC SÁCH", "VOICE READER", "GIỌNG ĐỌC SÁCH");
    add("Tốc độ giọng đọc", "Speech speed", "Tốc độ giọng đọc");
    add("Cao độ giọng đọc (Pitch)", "Voice Pitch", "Cao độ giọng đọc (Pitch)");
    add("Cao độ giọng đọc", "Voice Pitch", "Cao độ giọng đọc");
    add("Cao độ", "Pitch", "Cao độ");
    add("Trầm", "Deep / Low", "Trầm");
    add("Bổng", "High", "Bổng");
    add("Rất bổng", "Very High", "Rất bổng");
    add("Nguồn giọng đọc Online", "Online TTS Source", "Nguồn giọng đọc Online");
    add("Google TTS - Tự nhiên (Khuyên dùng)", "Google TTS - Natural (Recommended)", "Google TTS - Tự nhiên (Khuyên dùng)");
    add("Google TTS - Nhanh (Mobile)", "Google TTS - Fast (Mobile)", "Google TTS - Nhanh (Mobile)");
    add("Google APIs TTS", "Google APIs TTS", "Google APIs TTS");
    add("Nghe thử giọng đọc", "Test Voice", "Nghe thử giọng đọc");
    add("Xóa bộ nhớ đệm âm thanh", "Clear audio cache", "Xóa cache âm thanh");
    add("Đã dọn dẹp xong bộ nhớ đệm âm thanh.", "Audio cache cleared.", "Đã xóa cache âm thanh.");
    add("Đã dọn dẹp xong toàn bộ bộ nhớ đệm âm thanh.", "All audio cache cleared.", "Đã xóa toàn bộ cache âm thanh.");

    // Settings & Margins Presentation
    add("CÀI ĐẶT ĐỌC SÁCH", "READING SETTINGS", "CÀI ĐẶT ĐỌC SÁCH");
    add("Cài Đặt Đọc Sách", "Reading Settings", "Cài Đặt Đọc Sách");
    add("Áp Dụng", "Apply", "Áp Dụng");
    add("THÔNG TIN CHÂN TRANG", "FOOTER INFORMATION", "THÔNG TIN CHÂN TRANG");
    add("Chương hiện tại", "Current chapter", "Chương hiện tại");
    add("Tiến độ đọc toàn sách", "Book progress", "Tiến độ đọc toàn sách");
    add("Mức pin", "Battery level", "Mức pin");
    add("Thời gian", "Time", "Thời gian");
    add("Cách lật trang", "Page turning", "Cách lật trang");
    add("Vuốt ngang", "Swipe", "Vuốt ngang");
    add("Cuộn dọc", "Scroll", "Cuộn dọc");
    add("Vuốt hoặc chạm cạnh màn hình để chuyển trang. Chạm giữa để mở menu.",
        "Swipe or tap screen edges to turn page. Tap center for menu.",
        "Vuốt hoặc chạm cạnh màn hình để chuyển trang. Chạm giữa để mở menu.");
    add("Màu nền", "Background color", "Màu nền");
    add("Màu nền đọc sách", "Background color", "Màu nền đọc sách");
    add("Reading background", "Background color", "Màu nền");
    add("Tối", "Dark", "Tối");
    add("Vàng dịu", "Sepia", "Vàng dịu");
    add("Sáng", "Light", "Sáng");
    add("Cỡ chữ:", "Font size:", "Cỡ chữ:");
    add("Cỡ chữ", "Font size", "Cỡ chữ");
    add("Cỡ chữ nội dung", "Font size", "Cỡ chữ nội dung");
    add("Cỡ chữ nội dung:", "Font size:", "Cỡ chữ nội dung:");
    add("Mức cỡ chữ nhanh", "Quick Font Size Presets", "Mức cỡ chữ nhanh");
    add("Rất nhỏ (14px)", "Very Small (14px)", "Rất nhỏ (14px)");
    add("Nhỏ (16px)", "Small (16px)", "Nhỏ (16px)");
    add("Chuẩn (18px)", "Normal (18px)", "Chuẩn (18px)");
    add("Lớn (22px)", "Large (22px)", "Lớn (22px)");
    add("Rất lớn (26px)", "Very Large (26px)", "Rất lớn (26px)");
    add("Rất nhỏ", "Very small", "Rất nhỏ");
    add("Nhỏ", "Small", "Nhỏ");
    add("Chuẩn", "Standard", "Chuẩn");
    add("Lớn", "Large", "Lớn");
    add("Rất lớn", "Very large", "Rất lớn");
    add("Giãn dòng", "Line spacing", "Giãn dòng");
    add("Hẹp", "Tight", "Hẹp");
    add("Rộng", "Loose", "Rộng");
    add("CĂN LỀ VIỀN TRANG SÁCH", "PAGE MARGINS", "CĂN LỀ VIỀN TRANG SÁCH");
    add("CĂN LỀ TRANG", "PAGE MARGINS", "CĂN LỀ TRANG");
    add("Trên:", "Top:", "Trên:");
    add("Dưới:", "Bottom:", "Dưới:");
    add("Trái:", "Left:", "Trái:");
    add("Phải:", "Right:", "Phải:");
    add("Mép trên:", "Top:", "Trên:");
    add("Mép dưới:", "Bottom:", "Dưới:");
    add("Mép trái:", "Left:", "Trái:");
    add("Mép phải:", "Right:", "Phải:");
    add("Đặt lại viền cân bằng (16px)", "Reset margins (16px)", "Đặt lại viền cân bằng (16px)");
    add("Phông chữ đọc sách", "Reading Font Family", "Phông chữ đọc sách");
    add("Mặc định (Slate Pro)", "Default (Slate Pro)", "Mặc định (Slate Pro)");
    add("Không chân (Sans-Serif)", "Clean (Sans-Serif)", "Không chân (Sans-Serif)");
    add("Có chân sách chuẩn (Serif)", "Classic Book (Serif)", "Có chân sách chuẩn (Serif)");
    add("Báo chí & Tài liệu", "News & Editorial", "Báo chí & Tài liệu");
    add("Tiểu thuyết cổ điển", "Classic Novel", "Tiểu thuyết cổ điển");
    add("Máy đánh chữ (Monospace)", "Typewriter (Monospace)", "Máy đánh chữ (Monospace)");
    add("Nguyên bản sách", "Book Default", "Nguyên bản sách");
    add("Theo định dạng gốc của file", "Original embedded stylesheet", "Theo định dạng gốc của file");
    add("Xoay trang PDF:", "Rotate PDF:", "Xoay trang PDF:");
    add("Đảo màu tối PDF:", "Invert PDF colors:", "Đảo màu tối PDF:");
    add("Văn bản mẫu: Cỡ chữ, màu nền và khoảng cách viền 4 mép hiển thị khi đọc sách.",
        "Sample preview: Font size, background color, and page margins when reading.",
        "Văn bản mẫu: Cỡ chữ, màu nền và khoảng cách viền 4 mép hiển thị khi đọc sách.");
    add("Văn bản mẫu: Cỡ chữ, màu nền và khoảng cách dòng hiển thị khi đọc sách.",
        "Sample text: Font size, background color and line spacing in reader.",
        "Văn bản mẫu: Cỡ chữ, màu nền và khoảng cách dòng hiển thị khi đọc sách.");
    add("TỦ SÁCH & NGÔN NGỮ", "BOOKSHELF & LANGUAGE", "TỦ SÁCH & NGÔN NGỮ");
    add("GIAO DIỆN ỨNG DỤNG", "THEME", "CHỦ ĐỀ");
    add("Giao diện ứng dụng", "Theme", "Chủ đề");
    add("Chủ đề", "Theme", "Chủ đề");
    add("Kiểu hiển thị tủ sách", "Bookshelf display style", "Kiểu hiển thị tủ sách");
    add("Lưới bìa", "Mosaic", "Lưới bìa");
    add("Danh sách chi tiết", "Detailed list", "Danh sách chi tiết");
    add("Ngôn ngữ ứng dụng", "App language", "Ngôn ngữ ứng dụng");
    add("GIỌNG ĐỌC SÁCH", "VOICE READER", "GIỌNG ĐỌC SÁCH");
    add("Ngôn ngữ giọng đọc", "TTS Voice Language", "Ngôn ngữ giọng đọc");
    add("Tự động nhận diện (Khuyên dùng)", "Auto Detect (Recommended)", "Tự động nhận diện (Khuyên dùng)");
    add("Chậm 0.8x", "Slow 0.8x", "Chậm 0.8x");
    add("Chuẩn 1.0x", "Normal 1.0x", "Chuẩn 1.0x");
    add("Nhanh 1.25x", "Fast 1.25x", "Nhanh 1.25x");
    add("Rất nhanh 1.5x", "Very fast 1.5x", "Rất nhanh 1.5x");
    add("Xóa bộ nhớ đệm âm thanh", "Clear audio cache", "Xóa cache âm thanh");
    add("Quét lại toàn bộ thẻ nhớ và thiết bị", "Rescan storage & SD card", "Quét lại toàn bộ thẻ nhớ và thiết bị");
    add("Tự động tìm kiếm sách trên thẻ nhớ và bộ nhớ trong.",
        "Automatically scans for books on SD card and storage.",
        "Tự động tìm kiếm sách trên thẻ nhớ và bộ nhớ trong.");
    add("Ứng dụng đọc sách Native dành cho BlackBerry 10.\n• Hỗ trợ định dạng: EPUB, TXT, FB2, PDF, DJVU, CBZ, MOBI\n• Engine PDF và DJVU: Poppler và Libris Vector Engine C++\n• Tối ưu cho màn hình cảm ứng và bàn phím cứng Q10, Classic, Passport.",
        "Native E-Book Reader for BlackBerry 10.\n• Supported formats: EPUB, TXT, FB2, PDF, DJVU, CBZ, MOBI\n• PDF & DJVU engine: Poppler & Libris Vector Engine C++\n• Optimized for touchscreens and keyboards (Q10, Classic, Passport).",
        "Ứng dụng đọc sách Native dành cho BlackBerry 10.\n• Hỗ trợ định dạng: EPUB, TXT, FB2, PDF, DJVU, CBZ, MOBI\n• Engine PDF và DJVU: Poppler và Libris Vector Engine C++\n• Tối ưu cho màn hình cảm ứng và bàn phím cứng Q10, Classic, Passport.");

    // Hero Card & Detailed View Progress
    add("Đang đọc", "Reading", "Đang đọc");
    add("Tiếp tục đọc ›", "Continue Reading ›", "Tiếp tục đọc ›");
    add("Continue Reading", "Continue Reading", "Tiếp tục đọc");
    add("Continue Reading ›", "Continue Reading ›", "Tiếp tục đọc ›");
    add("Đọc tiếp", "Continue Reading", "Đọc tiếp");
    add("Đọc tiếp ›", "Continue Reading ›", "Đọc tiếp ›");
    add("Tác giả không rõ", "Unknown Author", "Tác giả không rõ");
    add("Đã đọc ", "Read ", "Đã đọc ");
    add("Chưa đọc", "Unread", "Chưa đọc");
    add("Chọn sách để mở", "Select book to open", "Chọn sách để mở");

    // Filter & Sort without parentheses
    add("Ngày sửa đổi: Mới nhất", "Date modified: Newest", "Ngày sửa đổi: Mới nhất");
    add("Ngày sửa đổi: Cũ nhất", "Date modified: Oldest", "Ngày sửa đổi: Cũ nhất");
    add("Tên sách: A đến Z", "Title: A to Z", "Tên sách: A đến Z");
    add("Tên sách: Z đến A", "Title: Z to A", "Tên sách: Z đến A");
    add("Tác giả: A đến Z", "Author: A to Z", "Tác giả: A đến Z");
    add("Dung lượng: Lớn nhất", "File size: Largest", "Dung lượng: Lớn nhất");

    // Legacy with parentheses (kept for backward lookup)
    add("NGÔN NGỮ / LANGUAGE", "LANGUAGE / NGÔN NGỮ", "NGÔN NGỮ / LANGUAGE");
    add("MÀU NỀN & GIAO DIỆN", "THEME & COLORS", "MÀU NỀN & GIAO DIỆN");
    add("Tối (OLED)", "Dark (OLED)", "Tối (OLED)");
    add("Vàng dịu (Sepia)", "Sepia", "Vàng dịu (Sepia)");
    add("Sáng (Light)", "Light", "Sáng (Light)");
    add("CHẾ ĐỘ ĐỌC (EPUB, TXT, FB2)", "READING MODE (EPUB, TXT, FB2)", "CHẾ ĐỘ ĐỌC (EPUB, TXT, FB2)");
    add("Lật trang (Vuốt ngang)", "Paged (Horizontal swipe)", "Lật trang (Vuốt ngang)");
    add("Cuộn liên tục (Dọc)", "Continuous scroll (Vertical)", "Cuộn liên tục (Dọc)");
    add("CỠ CHỮ NỘI DUNG", "CONTENT FONT SIZE", "CỠ CHỮ NỘI DUNG");
    add("A - (Nhỏ)", "A - (Smaller)", "A - (Nhỏ)");
    add("A + (Lớn)", "A + (Larger)", "A + (Lớn)");
    add("Văn bản mẫu: Cỡ chữ và màu nền này sẽ được áp dụng trực tiếp khi đọc sách.",
        "Sample text: This font size and color scheme will be applied while reading.",
        "Văn bản mẫu: Cỡ chữ và màu nền này sẽ được áp dụng trực tiếp khi đọc sách.");
    add("KIỂU HIỂN THỊ TRANG CHỦ", "HOME DISPLAY MODE", "KIỂU HIỂN THỊ TRANG CHỦ");
    add("QUẢN LÝ THƯ VIỆN", "LIBRARY MANAGEMENT", "QUẢN LÝ THƯ VIỆN");
    add("Quét lại toàn bộ thẻ nhớ & thiết bị", "Rescan storage & SD card", "Quét lại toàn bộ thẻ nhớ & thiết bị");
    add("Tự động quét sâu toàn bộ thư mục trên thẻ nhớ microSD (Book, Books, Download, Documents, v.v.) và bộ nhớ trong.",
        "Deep scans all folders on microSD (Book, Books, Download, Documents, etc.) and internal storage.",
        "Tự động quét sâu toàn bộ thư mục trên thẻ nhớ microSD (Book, Books, Download, Documents, v.v.) và bộ nhớ trong.");
    add("THÔNG TIN ỨNG DỤNG", "ABOUT APPLICATION", "THÔNG TIN ỨNG DỤNG");
    add("Ứng dụng đọc sách Native dành cho BlackBerry 10.\n• Hỗ trợ định dạng: EPUB, TXT, FB2, PDF, DJVU, CBZ, MOBI\n• Engine PDF & DJVU: Poppler & Libris Vector Engine C++\n• Tối ưu cho màn hình cảm ứng & bàn phím cứng Q10/Classic/Passport.",
        "Native E-Book Reader for BlackBerry 10.\n• Supported formats: EPUB, TXT, FB2, PDF, DJVU, CBZ, MOBI\n• PDF & DJVU Engine: Poppler & Libris Vector Engine C++\n• Optimized for touchscreens & hardware keyboards (Q10/Classic/Passport).",
        "Ứng dụng đọc sách Native dành cho BlackBerry 10.\n• Hỗ trợ định dạng: EPUB, TXT, FB2, PDF, DJVU, CBZ, MOBI\n• Engine PDF & DJVU: Poppler & Libris Vector Engine C++\n• Tối ưu cho màn hình cảm ứng & bàn phím cứng Q10/Classic/Passport.");
    add("Chọn sách (EPUB, TXT, FB2, PDF, DJVU, CBZ, MOBI)", "Select Book (EPUB, TXT, FB2, PDF, DJVU, CBZ, MOBI)", "Chọn sách (EPUB, TXT, FB2, PDF, DJVU, CBZ, MOBI)");

    // TOC, Search, Reflow, Dictionary, Translate
    add("Mục lục", "Table of Contents", "Mục lục");
    add("Lọc tên chương...", "Filter chapters...", "Lọc tên chương...");
    add("Tìm kiếm", "Search", "Tìm kiếm");
    add("Tìm kiếm trong sách", "Search in Book", "Tìm kiếm trong sách");
    add("Tìm trong chương", "Find in Chapter", "Tìm trong chương");
    add("Nhập từ khóa tìm kiếm...", "Enter search keyword...", "Nhập từ khóa tìm kiếm...");
    add("Nhập từ để tìm trong chương", "Enter word to find in chapter", "Nhập từ để tìm trong chương");
    add("Toàn bộ sách", "Full Book", "Toàn bộ sách");
    add("Trong chương", "In Chapter", "Trong chương");
    add("Kết quả tìm kiếm", "Search Results", "Kết quả tìm kiếm");
    add("Kết quả: ", "Results: ", "Kết quả: ");
    add("Tìm thấy %1 kết quả trong chương", "Found %1 results in chapter", "Tìm thấy %1 kết quả trong chương");
    add("Kết quả %1 trong chương", "Result %1 in chapter", "Kết quả %1 trong chương");
    add("Không tìm thấy kết quả nào.", "No search results found.", "Không tìm thấy kết quả nào.");
    add("Không tìm thấy kết quả trong chương", "No results found in chapter", "Không tìm thấy kết quả trong chương");
    add("Không tìm thấy kết quả nào trong chương.", "No results found in chapter.", "Không tìm thấy kết quả nào trong chương.");
    add("Không tìm thấy kết quả nào trong sách.", "No results found in book.", "Không tìm thấy kết quả nào trong sách.");
    add("Hãy thử tìm kiếm với từ khóa khác ngắn hơn.", "Try searching with a shorter keyword.", "Hãy thử tìm kiếm với từ khóa khác ngắn hơn.");
    add("Đang tìm kiếm trong sách...", "Searching across book...", "Đang tìm kiếm trong sách...");
    add("▲ Trước", "▲ Prev", "▲ Trước");
    add("▼ Tiếp", "▼ Next", "▼ Tiếp");

    // Book Detail
    add("Số chương", "Chapters", "Số chương");
    add("Số trang", "Pages", "Số trang");
    add("Sửa đổi", "Modified", "Sửa đổi");
    add("Thời gian đọc", "Reading Time", "Thời gian đọc");

    // Dictionary & Translation
    add("Tra từ điển", "Dictionary", "Tra từ điển");
    add("Dịch văn bản", "Translate", "Dịch văn bản");
    add("Từ điển & Dịch", "Dictionary & Translate", "Từ điển & Dịch");
    add("Từ điển & Dịch thuật", "Dictionary & Translation", "Từ điển & Dịch");
    add("Từ điển Offline", "Offline Dict", "Từ điển Offline");
    add("Từ điển Online", "Online Dict", "Từ điển Online");
    add("Công cụ Dịch", "Translate Tools", "Công cụ Dịch");
    add("Nhập từ cần tra...", "Enter word to lookup...", "Nhập từ cần tra...");
    add("Nhập đoạn văn bản cần dịch...", "Enter text to translate...", "Nhập đoạn văn bản cần dịch...");
    add("Tra từ", "Lookup", "Tra từ");
    add("Tra Trực Tuyến", "Online Lookup", "Tra Trực Tuyến");
    add("Dịch", "Translate", "Dịch");
    add("Dịch Ngay", "Translate Now", "Dịch Ngay");
    add("Sao chép", "Copy", "Sao chép");
    add("Đọc (TTS)", "Speak (TTS)", "Đọc (TTS)");
    add("Đã sao chép vào bộ nhớ tạm", "Copied to clipboard", "Đã sao chép vào bộ nhớ tạm");
    add("Đang tra từ trực tuyến...", "Looking up online...", "Đang tra từ online...");
    add("Đang tra từ điển trực tuyến...", "Looking up online dictionary...", "Đang tra từ điển online...");
    add("Đang dịch văn bản...", "Translating text...", "Đang dịch văn bản...");
    add("Văn bản gốc", "Original Text", "Văn bản gốc");
    add("Văn bản gốc:", "Source text:", "Văn bản gốc:");
    add("Bản dịch", "Translation", "Bản dịch");
    add("Bản dịch:", "Translation:", "Bản dịch:");
    add("Từ:", "From:", "Từ:");
    add("Từ ngôn ngữ:", "From:", "Từ:");
    add("Dịch nghĩa sang:", "Translate to:", "Dịch nghĩa sang:");
    add("Sang:", "To:", "Sang:");
    add("Tự động", "Auto Detect", "Tự động");
    add("Gợi ý từ tương tự: ", "Suggestions: ", "Gợi ý từ tương tự: ");
    add("Không tìm thấy kết quả từ điển trực tuyến.", "No online dictionary definitions found.", "Không tìm thấy kết quả từ điển online.");
    add("Kết nối công cụ dịch thuật hàng đầu:", "Top translation engines:", "Các công cụ dịch hỗ trợ:");
    add("Chế độ Reflow (Văn bản)", "Text Reflow Mode", "PDF Reflow (Văn bản)");
    add("Chế độ Trang ảnh", "Original Page Mode", "Chế độ Trang ảnh");
    add("Không tìm thấy lớp chữ OCR", "No OCR text layer found", "Không tìm thấy lớp chữ OCR");
    add("Trang PDF này được quét dạng hình ảnh và chưa có lớp văn bản (OCR text layer).",
        "This PDF page contains scanned images with no OCR text layer.",
        "Trang PDF này được quét dạng hình ảnh và chưa có lớp văn bản (OCR text layer).");
    add("Bấm nút Reflow trên thanh công cụ để trở về chế độ xem trang ảnh gốc.",
        "Tap Reflow on the toolbar to return to original page view.",
        "Bấm nút Reflow trên thanh công cụ để trở về chế độ xem trang ảnh gốc.");
    add("Đang đọc ", "Reading ", "Đang đọc ");
    add("Mở trong Trình duyệt", "Open in Browser", "Mở trong Trình duyệt");
    add("Google Dịch", "Google Translate", "Google Dịch");
    add("Bing Translator", "Bing Translator", "Bing Translator");
    add("DeepL Translator", "DeepL Translator", "DeepL Translator");
    add("Từ điển Cambridge", "Cambridge Dictionary", "Từ điển Cambridge");
    add("Từ điển Soha", "Soha Dictionary", "Từ điển Soha");
    add("Tiếng Anh", "English", "Tiếng Anh");
    add("Tiếng Việt", "Vietnamese", "Tiếng Việt");
    add("Tiếng Pháp", "French", "Tiếng Pháp");
    add("Tiếng Đức", "German", "Tiếng Đức");
    add("Tiếng Nga", "Russian", "Tiếng Nga");
    add("Tiếng Trung (GT)", "Chinese (Simplified)", "Tiếng Trung (GT)");
    add("Tiếng Trung (PT)", "Chinese (Traditional)", "Tiếng Trung (PT)");
    add("Tiếng Nhật", "Japanese", "Tiếng Nhật");
    add("Tiếng Hàn", "Korean", "Tiếng Hàn");
    add("Tiếng T.B.Nha", "Spanish", "Tiếng T.B.Nha");
    add("Tiếng B.Đ.Nha", "Portuguese", "Tiếng B.Đ.Nha");
    add("Tiếng Ý", "Italian", "Tiếng Ý");
    add("Tiếng Thái", "Thai", "Tiếng Thái");
    add("Tiếng Indo", "Indonesian", "Tiếng Indo");
    add("Tiếng Ả Rập", "Arabic", "Tiếng Ả Rập");
    add("Thêm sách...", "Import books...", "Thêm sách...");
    add("Tiếp tục đọc", "Continue Reading", "Tiếp tục đọc");
    add("Tiếp tục đọc ›", "Continue Reading ›", "Tiếp tục đọc ›");
    add("Đã đọc %1%", "Read %1%", "Đã đọc %1%");
    add("Ứng dụng đọc sách Native dành cho BlackBerry 10.\n• Phiên bản: 2.0.0\n• Hỗ trợ: EPUB, TXT, FB2, PDF, DJVU, CBZ, MOBI\n• Phát triển với BlackBerry Cascades Qt C++.",
        "Native e-Book reader for BlackBerry 10.\n• Version: 2.0.0\n• Formats: EPUB, TXT, FB2, PDF, DJVU, CBZ, MOBI\n• Built with BlackBerry Cascades Qt C++.",
        "Ứng dụng đọc sách Native dành cho BlackBerry 10.\n• Phiên bản: 2.0.0\n• Hỗ trợ: EPUB, TXT, FB2, PDF, DJVU, CBZ, MOBI\n• Phát triển với BlackBerry Cascades Qt C++.");

    // =========================================================================
    // UNIFIED FULL TRANSLATIONS (All Screens, Settings, Actions, Dashboards)
    // =========================================================================

    // --- Main Navigation, Drawer & Common ---
    add("Tiến Độ Đọc", "Reading Progress", "Tiến Độ Đọc");
    add("Recent", "Recent", "Sách Gần Đây");
    add("Contents", "Contents", "Mục Lục");
    add("Reading", "Reading", "Đang Đọc");
    add("About", "About", "Giới thiệu");
    add("Settings", "Settings", "Cài đặt");
    add("Open", "Open", "Mở Tệp");
    add("Mở Tệp", "Open File", "Mở Tệp");
    add("Mở Tủ Sách", "Open Bookshelf", "Mở Tủ Sách");
    add("Lên Thư Mục Cha", "Go to Parent Folder", "Lên Thư Mục Cha");
    add("Làm Mới", "Refresh", "Làm Mới");
    add("Mục Lục", "Table of Contents", "Mục Lục");
    add("Mục Lục Sách", "Table of Contents", "Mục Lục Sách");
    add("Sách Gần Đây", "Recent Books", "Sách Gần Đây");
    add("SÁCH ĐÃ ĐỌC GẦN ĐÂY", "RECENTLY READ BOOKS", "SÁCH ĐÃ ĐỌC GẦN ĐÂY");
    add("Chưa có sách nào mở gần đây", "No recently opened books", "Chưa có sách nào mở gần đây");
    add("Chưa có tiêu đề", "Untitled", "Chưa có tiêu đề");
    add("Chưa rõ tác giả", "Unknown Author", "Chưa rõ tác giả");
    add("Tác giả chưa rõ", "Unknown Author", "Tác giả chưa rõ");
    add("Hãy mở sách từ 'Tủ Sách' hoặc bấm 'Mở Tệp' để bắt đầu đọc!", "Open a book from 'Bookshelf' or tap 'Open File' to start reading!", "Hãy mở sách từ 'Tủ Sách' hoặc bấm 'Mở Tệp' để bắt đầu đọc!");
    add("Chưa có mục lục khả dụng", "No table of contents available", "Chưa có mục lục khả dụng");
    add("Hãy mở một cuốn sách từ 'Tủ Sách' hoặc 'Gần Đây' để xem danh sách chương và điều hướng nhanh.", "Open a book from 'Bookshelf' or 'Recent' to view chapters and navigate quickly.", "Hãy mở một cuốn sách từ 'Tủ Sách' hoặc 'Gần Đây' để xem danh sách chương và điều hướng nhanh.");
    add("Đến Tủ Sách", "Go to Bookshelf", "Đến Tủ Sách");
    add("Lọc theo tên chương...", "Filter by chapter title...", "Lọc theo tên chương...");
    add("Thư mục trống hoặc không có sách phù hợp", "Folder is empty or contains no compatible books", "Thư mục trống hoặc không có sách phù hợp");

    // --- Reading Progress Dashboard ---
    add("Đang đọc dở", "Currently reading", "Đang đọc dở");
    add("Hôm nay", "Today", "Hôm nay");
    add("Tổng thời gian", "Total time", "Tổng thời gian");
    add("THỐNG KÊ THỜI GIAN ĐỌC", "READING TIME STATISTICS", "THỐNG KÊ THỜI GIAN ĐỌC");
    add("THỜI GIAN ĐỌC", "READING TIME", "THỜI GIAN ĐỌC");
    add("LƯỢNG ĐỌC & TIẾN ĐỘ", "READING VOLUME & PROGRESS", "LƯỢNG ĐỌC & TIẾN ĐỘ");
    add("Trong thư viện", "In library", "Trong thư viện");
    add("Đã hoàn thành", "Finished", "Đã hoàn thành");
    add("Tiến độ trung bình", "Average progress", "Tiến độ trung bình");
    add("Duy trì thói quen đọc sách mỗi ngày để tăng vốn tri thức!", "Maintain a daily reading habit to expand your knowledge!", "Duy trì thói quen đọc sách mỗi ngày để tăng vốn tri thức!");
    add("Xem chi tiết tủ sách ›", "View full library ›", "Xem chi tiết tủ sách ›");
    add("TỔNG QUAN", "OVERVIEW", "TỔNG QUAN");
    add("TIẾN ĐỘ TỦ SÁCH", "BOOKSHELF PROGRESS", "TIẾN ĐỘ TỦ SÁCH");
    add("Bắt Đầu Đọc Sách", "Start Reading", "Bắt Đầu Đọc Sách");
    add("Mở một tệp sách từ thẻ nhớ hoặc duyệt tủ sách để bắt đầu theo dõi tiến độ đọc của bạn.", "Open a book from memory card or browse bookshelf to start tracking your reading progress.", "Mở một tệp sách từ thẻ nhớ hoặc duyệt tủ sách để bắt đầu theo dõi tiến độ đọc của bạn.");
    add("Tổng tích lũy", "Total accumulated", "Tổng tích lũy");
    add("Tiến độ hoàn thành trung bình", "Average completion progress", "Tiến độ hoàn thành trung bình");
    add("Xem tất cả trong mục Gần Đây", "View all in Recent", "Xem tất cả trong mục Gần Đây");

    // --- Settings Flat List ---
    add("Hiển thị & Giao diện", "Display & Appearance", "Hiển thị & Giao diện");
    add("Phông chữ, cỡ chữ, khoảng cách dòng, chủ đề sách", "Font, font size, line spacing, reading theme", "Phông chữ, cỡ chữ, khoảng cách dòng, chủ đề sách");
    add("Đọc sách & Thao tác cuộn", "Reading & Scrolling", "Đọc sách & Thao tác cuộn");
    add("Đọc sách & Thao tác", "Reading & Navigation", "Đọc sách & Thao tác");
    add("Cách lật trang, cuộn dọc, bàn rê Q20, cảm ứng Passport", "Page turning, scroll mode, Q20 trackpad, Passport touch", "Cách lật trang, cuộn dọc, bàn rê Q20, cảm ứng Passport");
    add("Thông tin chân trang", "Footer Information", "Thông tin chân trang");
    add("Tên chương, tiến độ %, pin và đồng hồ thời gian thực", "Chapter title, progress %, battery and real-time clock", "Tên chương, tiến độ %, pin và đồng hồ thời gian thực");
    add("Giọng đọc sách", "Text-To-Speech (TTS)", "Giọng đọc sách");
    add("Giọng đọc sách (TTS)", "Voice Reader (TTS)", "Giọng đọc sách (TTS)");
    add("Ngôn ngữ giọng đọc, tốc độ phát và bộ nhớ đệm âm thanh", "Voice language, playback speed and audio cache", "Ngôn ngữ giọng đọc, tốc độ và cache âm thanh");
    add("Tủ sách & Quản lý", "Bookshelf & Management", "Tủ sách & Quản lý");
    add("Tủ sách & Quản lý dữ liệu", "Bookshelf & Management", "Tủ sách & Quản lý");
    add("Chế độ xem Lưới/Danh sách, gom nhóm, quét thư mục sách", "Mosaic/List view, grouping, book folder scanning", "Chế độ xem Lưới/Danh sách, gom nhóm, quét thư mục sách");
    add("Thông tin ứng dụng", "About Application", "Thông tin ứng dụng");
    add("Phiên bản Libris v2.0, công nghệ và hỗ trợ thiết bị", "Libris v2.0, technology and device support", "Phiên bản Libris v2.0, công nghệ và hỗ trợ thiết bị");

    // --- DisplaySettingsPage ---
    add("GIAO DIỆN CHỦ ĐỀ", "THEME & APPEARANCE", "GIAO DIỆN CHỦ ĐỀ");
    add("Tối (Dark Mode)", "Dark", "Tối");
    add("Sáng (Light Mode)", "Light", "Sáng");
    add("Màu nền đọc sách", "Background color", "Màu nền");
    add("Màu nền", "Background color", "Màu nền");
    add("Tối", "Dark", "Tối");
    add("Sáng", "Light", "Sáng");
    add("CỠ CHỮ & GIÃN DÒNG", "FONT SIZE & SPACING", "CỠ CHỮ & GIÃN DÒNG");
    add("Cỡ chữ nội dung:", "Font size:", "Cỡ chữ:");
    add("Cỡ chữ:", "Font size:", "Cỡ chữ:");
    add("Khoảng cách dòng", "Line Spacing", "Khoảng cách dòng");
    add("Hẹp", "Tight", "Hẹp");
    add("Vừa", "Normal", "Vừa");
    add("Rộng", "Wide", "Rộng");
    add("CANH LỀ MÀN HÌNH (PX)", "PAGE MARGINS", "CĂN LỀ TRANG");
    add("CĂN LỀ VIỀN TRANG SÁCH", "PAGE MARGINS", "CĂN LỀ TRANG");
    add("CĂN LỀ TRANG", "PAGE MARGINS", "CĂN LỀ TRANG");
    add("Lề trên:", "Top:", "Trên:");
    add("Lề dưới:", "Bottom:", "Dưới:");
    add("Lề trái:", "Left:", "Trái:");
    add("Lề phải:", "Right:", "Phải:");
    add("Mép trên:", "Top:", "Trên:");
    add("Mép dưới:", "Bottom:", "Dưới:");
    add("Mép trái:", "Left:", "Trái:");
    add("Mép phải:", "Right:", "Phải:");
    add("Trên:", "Top:", "Trên:");
    add("Dưới:", "Bottom:", "Dưới:");
    add("Trái:", "Left:", "Trái:");
    add("Phải:", "Right:", "Phải:");
    add("XEM TRƯỚC VĂN BẢN", "TEXT PREVIEW", "XEM TRƯỚC VĂN BẢN");
    add("Văn bản mẫu: Cỡ chữ, màu nền và khoảng cách viền 4 mép hiển thị khi đọc sách.", "Sample text: Font size, background color and margins applied while reading.", "Văn bản mẫu: Cỡ chữ, màu nền và khoảng cách viền 4 mép hiển thị khi đọc sách.");

    // --- ReadingSettingsPage ---
    add("CHẾ ĐỘ ĐỌC SÁCH", "READING MODE", "CHẾ ĐỘ ĐỌC SÁCH");
    add("Phương thức chuyển trang", "Page turn method", "Phương thức chuyển trang");
    add("Page turn method", "Page turn method", "Phương thức chuyển trang");
    add("Page Turn Method", "Page turn method", "Phương thức chuyển trang");
    add("Vuốt", "Swipe", "Vuốt");
    add("Cuộn", "Scroll", "Cuộn");
    add("Swipe", "Swipe", "Vuốt");
    add("Scroll", "Scroll", "Cuộn");
    add("Lật trang ngang", "Swipe", "Vuốt");
    add("Cuộn dọc liên tục", "Scroll", "Cuộn");
    add("Lật trang ngang (Paged Mode)", "Swipe", "Vuốt");
    add("Cuộn dọc liên tục (Scroll Mode)", "Scroll", "Cuộn");
    add("Đọc sách & Thao tác cuộn", "Reading & Navigation", "Đọc sách & Thao tác");
    add("Đọc sách & Thao tác", "Reading & Navigation", "Đọc sách & Thao tác");
    add("Phân trang, cuộn dọc & bàn phím", "Page turn method", "Phương thức chuyển trang");

    // --- FooterSettingsPage ---
    add("MỤC HIỂN THỊ DƯỚI ĐÁY TRANG", "FOOTER DISPLAY ITEMS", "MỤC HIỂN THỊ DƯỚI ĐÁY TRANG");
    add("Tên chương hiện tại", "Current chapter title", "Tên chương hiện tại");
    add("Hiển thị tên chương đang đọc ở góc chân trang", "Show reading chapter title in the footer", "Hiển thị tên chương đang đọc ở góc chân trang");
    add("Tiến độ đọc toàn sách", "Overall book progress", "Tiến độ đọc toàn sách");
    add("Tiến độ đọc (% và trang)", "Reading progress (% and page)", "Tiến độ đọc (% và trang)");
    add("Hiển thị số trang hiện tại và phần trăm (%) hoàn thành", "Show current page and percentage (%) completed", "Hiển thị số trang hiện tại và phần trăm (%) hoàn thành");
    add("Mức pin thiết bị", "Device battery level", "Mức pin thiết bị");
    add("Theo dõi phần trăm pin điện thoại trong khi đọc sách", "Monitor phone battery percentage while reading", "Theo dõi phần trăm pin điện thoại trong khi đọc sách");
    add("Đồng hồ thời gian thực", "Real-time clock", "Đồng hồ thời gian thực");
    add("Đồng hồ thời gian", "Clock", "Đồng hồ thời gian");
    add("Hiển thị giờ:phút hiện tại mà không cần vuốt thanh thông báo", "Show current hour:minute without pulling down notification shade", "Hiển thị giờ:phút hiện tại mà không cần vuốt thanh thông báo");
    add("THAO TÁC NHANH", "QUICK ACTIONS", "THAO TÁC NHANH");
    add("Bật tất cả", "Enable All", "Bật tất cả");
    add("Tắt tất cả", "Disable All", "Tắt tất cả");
    add("Mẹo: Thanh chân trang được tích hợp kín đáo ở đáy màn hình, sử dụng màu chữ dịu mắt tương thích với nền sáng, sepia và dark mode.", "Tip: The footer is discreetly integrated at the bottom of the screen with subtle text colors matching light, sepia and dark modes.", "Mẹo: Thanh chân trang được tích hợp kín đáo ở đáy màn hình, sử dụng màu chữ dịu mắt tương thích với nền sáng, sepia và dark mode.");

    // --- TtsSettingsPage ---
    add("CẤU HÌNH GIỌNG ĐỌC", "VOICE CONFIGURATION", "CẤU HÌNH GIỌNG ĐỌC");
    add("Tốc độ giọng đọc", "Voice Speed", "Tốc độ giọng đọc");
    add("BỘ NHỚ ĐỆM ÂM THANH", "AUDIO CACHE", "BỘ NHỚ ĐỆM ÂM THANH");
    add("Dọn dẹp các tệp âm thanh giọng đọc tạm thời đã lưu để giải phóng dung lượng bộ nhớ thiết bị.", "Clear cached temporary audio files to free up device storage.", "Dọn dẹp các tệp âm thanh giọng đọc tạm thời đã lưu để giải phóng dung lượng bộ nhớ thiết bị.");
    add("TÍNH NĂNG ĐỌC THÔNG MINH", "SMART READING FEATURES", "TÍNH NĂNG ĐỌC THÔNG MINH");
    add("• Tự động ngắt câu thông minh theo ngữ điệu văn bản.\n• Hỗ trợ nghe tiếp câu sau khi chuyển trang.\n• Tích hợp bảng điều khiển nổi ngay trong trang sách.", "• Intelligent sentence boundary detection matching natural cadence.\n• Continuous playback across chapter transitions.\n• Integrated floating audio controls inside reader canvas.", "• Tự động ngắt câu thông minh theo ngữ điệu văn bản.\n• Hỗ trợ nghe tiếp câu sau khi chuyển trang.\n• Tích hợp bảng điều khiển nổi ngay trong trang sách.");

    // --- LibrarySettingsPage ---
    add("GIAO DIỆN TỦ SÁCH", "BOOKSHELF VIEW", "GIAO DIỆN TỦ SÁCH");
    add("Lưới bìa sách", "Mosaic", "Lưới bìa sách");
    add("Lưới bìa sách (Mosaic Grid)", "Mosaic", "Lưới bìa sách");
    add("Danh sách chi tiết (Detailed List)", "Detailed List", "Danh sách chi tiết");
    add("QUÉT LẠI DỮ LIỆU SÁCH", "RESCAN BOOKS", "QUÉT LẠI DỮ LIỆU SÁCH");
    add("Quét lại toàn bộ thẻ nhớ và thiết bị", "Rescan all storage and MicroSD card", "Quét lại toàn bộ thẻ nhớ và thiết bị");

    // --- AboutPage & AboutSettingsPage ---
    add("Thông Tin Ứng Dụng",
        "About Application",
        "Thông Tin Ứng Dụng");
    add("Đóng",
        "Close",
        "Đóng");
    add("Cài Đặt",
        "Settings",
        "Cài Đặt");
    add("Cài đặt",
        "Settings",
        "Cài đặt");
    add("Giới thiệu",
        "About",
        "Giới thiệu");
    add("Hướng dẫn",
        "Help",
        "Hướng dẫn");
    add("Thay đổi",
        "Changelog",
        "Thay đổi");
    add("Phiên bản 2.0.0 (Build 66) • BlackBerry 10",
        "Version 2.0.0 (Build 66) • BlackBerry 10",
        "Phiên bản 2.0.0 (Build 66) • BlackBerry 10");
    add("Trình đọc sách Native thuần C++ & Cascades dành cho BlackBerry 10.",
        "Native C++ & Cascades e-Book Reader handcrafted for BlackBerry 10.",
        "Trình đọc sách Native thuần C++ & Cascades dành cho BlackBerry 10.");
    add("TỔNG QUAN",
        "OVERVIEW",
        "TỔNG QUAN");
    add("Libris mang lại trải nghiệm đọc sách nguyên bản, mượt mà và tối ưu nhất trên BlackBerry 10. Tích hợp thanh bên vuốt trái (Navigation Drawer), hỗ trợ đa dạng định dạng tệp sách, tra từ điển ngoại tuyến tức thì và đọc sách bằng giọng nói (TTS).",
        "Libris delivers a native, fluid, and optimized e-reading experience for BlackBerry 10. Features a slide-out Navigation Drawer, comprehensive e-book format support, instant offline dictionary definitions, and Text-To-Speech (TTS) voice reading.",
        "Libris mang lại trải nghiệm đọc sách nguyên bản, mượt mà và tối ưu nhất trên BlackBerry 10. Tích hợp thanh bên vuốt trái (Navigation Drawer), hỗ trợ đa dạng định dạng tệp sách, tra từ điển ngoại tuyến tức thì và đọc sách bằng giọng nói (TTS).");
    add("CÔNG NGHỆ & BỘ XỬ LÝ LÕI",
        "CORE ENGINES & TECHNOLOGY",
        "CÔNG NGHỆ & BỘ XỬ LÝ LÕI");
    add("• Lõi PDF & DJVU: Động cơ Libris Vector Engine kết hợp Poppler C++ và DjVuLibre kết xuất trang nhanh, sắc nét.\n• Lõi EPUB & FB2: Bộ giải mã pugixml DOM C++ siêu nhẹ và thư viện nén miniz tốc độ cao.\n• Bảng mã Tiếng Việt: Tự động nhận diện bảng mã (UTF-8, UTF-16, TCVN3, Windows-1258, VNI) cho tệp TXT.\n• Đọc Giọng Nói (TTS): Tích hợp trực tiếp hệ thống giọng đọc BlackBerry Multimedia Services.\n• Từ Điển Ngoại Tuyến: Cơ sở dữ liệu SQLite tích hợp trong máy, tra từ tức thì không cần mạng.",
        "• PDF & DJVU Engine: Libris Vector Engine combining Poppler C++ and DjVuLibre for crisp, fast rendering.\n• EPUB & FB2 Parser: Lightweight pugixml DOM parser C++ and high-speed miniz decompression.\n• Vietnamese Charsets: Automatic encoding detection (UTF-8, UTF-16, TCVN3, Windows-1258, VNI) for TXT files.\n• Voice Engine (TTS): Native integration with BlackBerry Multimedia Services speech synthesis.\n• Offline Dictionary: Embedded SQLite database for instant word definition lookups without internet.",
        "• Lõi PDF & DJVU: Động cơ Libris Vector Engine kết hợp Poppler C++ và DjVuLibre kết xuất trang nhanh, sắc nét.\n• Lõi EPUB & FB2: Bộ giải mã pugixml DOM C++ siêu nhẹ và thư viện nén miniz tốc độ cao.\n• Bảng mã Tiếng Việt: Tự động nhận diện bảng mã (UTF-8, UTF-16, TCVN3, Windows-1258, VNI) cho tệp TXT.\n• Đọc Giọng Nói (TTS): Tích hợp trực tiếp hệ thống giọng đọc BlackBerry Multimedia Services.\n• Từ Điển Ngoại Tuyến: Cơ sở dữ liệu SQLite tích hợp trong máy, tra từ tức thì không cần mạng.");
    add("ĐỊNH DẠNG TÀI LIỆU HỖ TRỢ",
        "SUPPORTED DOCUMENT FORMATS",
        "ĐỊNH DẠNG TÀI LIỆU HỖ TRỢ");
    add("• EPUB (.epub): Chuẩn EPUB 2.0 & 3.0 với phông chữ nhúng, hình ảnh và mục lục NCX.\n• MOBI (.mobi, .prc): Sách điện tử Mobipocket / Kindle với mục lục và ảnh nhúng.\n• CBZ (.cbz): Truyện tranh nén với lật trang mượt mà và trích xuất ComicInfo.\n• PDF (.pdf): Chế độ đồ họa nguyên bản và cuộn văn bản Reflow OCR.\n• DJVU (.djvu, .djv): Tài liệu scan nén chất lượng cao.\n• FB2 (.fb2): Định dạng FictionBook kèm chú thích chân trang.\n• TXT (.txt): Văn bản thuần tự động phát hiện mã hóa tiếng Việt.",
        "• EPUB (.epub): EPUB 2.0 & 3.0 standards with embedded fonts, images, and NCX TOC.\n• MOBI (.mobi, .prc): Mobipocket / Kindle eBook format with embedded images & TOC.\n• CBZ (.cbz): Comic book archive with smooth page flipping and ComicInfo parsing.\n• PDF (.pdf): Native vector graphic rendering and Text Reflow OCR mode.\n• DJVU (.djvu, .djv): High-compression scanned books and documents.\n• FB2 (.fb2): FictionBook format with full footnote annotations.\n• TXT (.txt): Plain text with automatic Vietnamese charset decoding.",
        "• EPUB (.epub): Chuẩn EPUB 2.0 & EPUB 3.0 với phông chữ nhúng, hình ảnh và mục lục NCX.\n• MOBI (.mobi, .prc): Sách điện tử Mobipocket / Kindle với mục lục và ảnh nhúng.\n• CBZ (.cbz): Truyện tranh nén với lật trang mượt mà và trích xuất ComicInfo.\n• PDF (.pdf): Chế độ đồ họa nguyên bản và cuộn văn bản Reflow OCR.\n• DJVU (.djvu, .djv): Tài liệu scan nén chất lượng cao.\n• FB2 (.fb2): Định dạng FictionBook kèm chú thích chân trang.\n• TXT (.txt): Văn bản thuần tự động phát hiện mã hóa tiếng Việt.");
    add("BẢN QUYỀN & GIẤY PHÉP",
        "COPYRIGHT & LICENSE",
        "BẢN QUYỀN & GIẤY PHÉP");
    add("• Tác giả: NhatLee (Libris Developer Team)\n• Bộ dựng Vector PDF & DjVu: Libris Engine\n• Giấy phép: Phần mềm nguồn mở theo giấy phép GNU General Public License v2 (GPLv2)\n• Môi trường: BlackBerry 10 Native SDK 10.3.1, QNX Momentics IDE.",
        "• Author: NhatLee (Libris Developer Team)\n• PDF & DjVu Vector Engine: Libris Engine\n• License: Open-source software under GNU General Public License v2 (GPLv2)\n• Environment: BlackBerry 10 Native SDK 10.3.1, QNX Momentics IDE.",
        "• Tác giả: NhatLee (Libris Developer Team)\n• Bộ dựng Vector PDF & DjVu: Libris Engine\n• Giấy phép: Phần mềm nguồn mở theo giấy phép GNU General Public License v2 (GPLv2)\n• Môi trường: BlackBerry 10 Native SDK 10.3.1, QNX Momentics IDE.");
    add("CỬ CHỈ ĐIỀU HƯỚNG NATIVE (BB10 GESTURES)",
        "NATIVE BB10 GESTURES",
        "CỬ CHỈ ĐIỀU HƯỚNG NATIVE (BB10 GESTURES)");
    add("• Vuốt từ mép viền trái sang phải (Left Bezel Swipe): Mở ngăn kéo Sidebar trượt trái (Đang đọc, Gần đây, Mục lục, Tủ sách, Duyệt tệp). Hoặc bấm icon Menu góc dưới thanh công cụ.\n• Vuốt từ mép viền trên xuống (Top Bezel Swipe): Mở Menu ứng dụng BlackBerry 10 để truy cập nhanh Cài Đặt (Settings) và Giới Thiệu (About).\n• Chạm vào giữa màn hình đọc: Bật/Tắt chế độ đọc toàn màn hình (ẩn/hiện thanh tiêu đề và thanh công cụ đáy).\n• Vuốt từ mép phải sang trái: Mở danh sách tác vụ mở rộng (Action Overflow).",
        "• Left Bezel Swipe: Slide out native Navigation Drawer (Reading, Recent, Contents, Bookshelf, File Browser). You can also tap the Menu icon on the bottom action bar.\n• Top Bezel Swipe: Pull down standard BlackBerry 10 application menu for quick access to Settings and About.\n• Tap Center Canvas: Toggle full-screen reading mode (hide or show top title bar and bottom toolbar).\n• Swipe Right Edge: Open Action Overflow sheet.",
        "• Vuốt từ mép viền trái sang phải (Left Bezel Swipe): Mở ngăn kéo Sidebar trượt trái (Đang đọc, Gần đây, Mục lục, Tủ sách, Duyệt tệp). Hoặc bấm icon Menu góc dưới thanh công cụ.\n• Vuốt từ mép viền trên xuống (Top Bezel Swipe): Mở Menu ứng dụng BlackBerry 10 để truy cập nhanh Cài Đặt (Settings) và Giới Thiệu (About).\n• Chạm vào giữa màn hình đọc: Bật/Tắt chế độ đọc toàn màn hình (ẩn/hiện thanh tiêu đề và thanh công cụ đáy).\n• Vuốt từ mép phải sang trái: Mở danh sách tác vụ mở rộng (Action Overflow).");
    add("PHÍM TẮT BÀN PHÍM CỨNG (PASSPORT, CLASSIC, Q10)",
        "HARDWARE KEYBOARD SHORTCUTS",
        "PHÍM TẮT BÀN PHÍM CỨNG (PASSPORT, CLASSIC, Q10)");
    add("• Space (Phím cách): Sang trang tiếp theo / Tạm dừng hoặc tiếp tục đọc TTS.\n• N: Chuyển đến trang kế tiếp (Next page).\n• P / U: Quay lại trang trước (Previous page).\n• J / K: Cuộn từng dòng xuống / lên (Scroll mode).\n• Z / W: Phóng to / Thu nhỏ cỡ chữ đọc sách.\n• G / M: Mở mục lục sách để đi đến chương mong muốn (Go to / Menu).\n• T / B: Nhảy về đầu sách (Top) / cuối sách (Bottom).\n• R: Xoay trang tài liệu PDF 90° (Rotate).\n• V: Đổi màu nền Đêm / Ngày (Invert Colors).\n• F: Tìm kiếm từ khóa trong cuốn sách / Toàn màn hình.\n• A / X: Bật/Tắt đọc sách bằng giọng nói (Text-To-Speech).\n• D / L: Tra từ điển & dịch nghĩa ngoại tuyến.\n• H: Đánh dấu trang hiện tại (Add Bookmark).\n• O: Mở tệp sách mới từ bộ nhớ máy hoặc thẻ nhớ.\n• S: Mở bảng cài đặt hiển thị và phông chữ (Settings).\n• I: Xem thông tin chi tiết cuốn sách (Book Info).",
        "• Space: Turn to next page / Pause or Resume Voice (TTS).\n• N: Turn to next page.\n• P / U: Turn to previous page.\n• J / K: Scroll down / up by line (Scroll mode).\n• Z / W: Zoom In / Out font size.\n• G / M: Open Table of Contents (Go to / Menu).\n• T / B: Jump to Top / Bottom of book.\n• R: Rotate PDF page 90°.\n• V: Invert Day / Night reading colors.\n• F: Search keywords in book / Toggle Fullscreen.\n• A / X: Toggle Text-To-Speech voice reading.\n• D / L: Offline dictionary definition & translation lookup.\n• H: Bookmark current page.\n• O: Open new book file from storage or SD card.\n• S: Open Display & Font Settings.\n• I: View Book Information.",
        "• Space (Phím cách): Sang trang tiếp theo / Tạm dừng hoặc tiếp tục đọc TTS.\n• N: Chuyển đến trang kế tiếp (Next page).\n• P / U: Quay lại trang trước (Previous page).\n• J / K: Cuộn từng dòng xuống / lên (Scroll mode).\n• Z / W: Phóng to / Thu nhỏ cỡ chữ đọc sách.\n• G / M: Mở mục lục sách để đi đến chương mong muốn (Go to / Menu).\n• T / B: Nhảy về đầu sách (Top) / cuối sách (Bottom).\n• R: Xoay trang tài liệu PDF 90° (Rotate).\n• V: Đổi màu nền Đêm / Ngày (Invert Colors).\n• F: Tìm kiếm từ khóa trong cuốn sách / Toàn màn hình.\n• A / X: Bật/Tắt đọc sách bằng giọng nói (Text-To-Speech).\n• D / L: Tra từ điển & dịch nghĩa ngoại tuyến.\n• H: Đánh dấu trang hiện tại (Add Bookmark).\n• O: Mở tệp sách mới từ bộ nhớ máy hoặc thẻ nhớ.\n• S: Mở bảng cài đặt hiển thị và phông chữ (Settings).\n• I: Xem thông tin chi tiết cuốn sách (Book Info).");
    add("HỖ TRỢ THIẾT BỊ ĐẶC BIỆT",
        "SPECIAL DEVICE HARDWARE",
        "HỖ TRỢ THIẾT BỊ ĐẶC BIỆT");
    add("• BlackBerry Passport: Hỗ trợ vuốt trượt ngón tay trực tiếp trên bàn phím cảm ứng vật lý (Touch-enabled Keyboard) để lật trang hoặc cuộn dòng mượt mà không che khuất màn hình.\n• BlackBerry Classic (Q20): Hỗ trợ bàn rê quang học (Optical Trackpad) di chuyển chính xác và nhấn phím điều hướng để cuộn trang.",
        "• BlackBerry Passport: Touch-enabled physical keyboard allows swiping directly across key rows to turn pages or scroll smoothly without obstructing the screen.\n• BlackBerry Classic (Q20): Optical Trackpad navigation provides precision line scrolling and physical navigation button support.",
        "• BlackBerry Passport: Hỗ trợ vuốt trượt ngón tay trực tiếp trên bàn phím cảm ứng vật lý (Touch-enabled Keyboard) để lật trang hoặc cuộn dòng mượt mà không che khuất màn hình.\n• BlackBerry Classic (Q20): Hỗ trợ bàn rê quang học (Optical Trackpad) di chuyển chính xác và nhấn phím điều hướng để cuộn trang.");
    add("NHẬT KÝ THAY ĐỔI THEO BẢN BUILD",
        "CHANGELOG BY BUILD",
        "NHẬT KÝ THAY ĐỔI THEO BẢN BUILD");
    add("• Bản dựng Build 65 - 66 (Mới nhất):\n  - Icon Thư Viện Đơn Sắc: Chuyển icon Thư viện sang màu đen trắng (monochrome) đồng bộ với toàn bộ thanh Tab và Action Bar chuẩn BlackBerry 10.\n  - Đồng Bộ Ngôn Ngữ Toàn Diện: Chuẩn hóa 100% tiếng Việt & tiếng Anh cho Giới Thiệu (About), Hướng Dẫn (Help), Cài Đặt (Settings) và các menu.\n  - Khối Đang Đọc Tinh Gọn: Loại bỏ viền xanh và icon thừa, mang lại diện mạo hiện đại, sạch sẽ cho thẻ sách đang đọc.\n  - Đọc Giọng Nói (TTS) Hoàn Thiện: Tối ưu đọc TTS trên trang sách, hỗ trợ tạm dừng bằng phím Space và tự động cuộn trang theo giọng đọc.\n  - Tối Ưu Hóa Giao Diện: Rút gọn mô tả các mục menu và cài đặt, giúp thao tác nhanh trên màn hình nhỏ.\n\n• Bản dựng Build 50 - 64:\n  - Bảo Toàn Dữ Liệu Sau Cài Lại: Tự động lưu và khôi phục lịch sử đọc, tiến độ và ghi chú vào bộ nhớ ngoài (/accounts/1000/shared/documents/Libris).\n  - Từ Điển Ngoại Tuyến (Offline Dictionary): Tra cứu từ điển SQLite tức thì khi bôi đen từ trong sách mà không cần kết nối mạng.\n  - Bảng Thống Kê Đọc Sách (Reading Stats): Theo dõi thời gian đọc, tốc độ đọc và tiến độ hoàn thành sách chi tiết.\n  - Bộ Phím Tắt Toàn Diện: Tối ưu hóa tối đa cho bàn phím vật lý BlackBerry Passport, Classic Q20 và Q10.\n\n• Bản dựng Build 30 - 49 (Phiên bản 2.0.0):\n  - Navigation Drawer 5 Tab Native: Giao diện chuẩn 5 Tab Native với ngăn kéo trượt trái: Đang đọc, Gần đây, Mục lục, Tủ sách, Duyệt tệp.\n  - Nâng Cấp Lõi Libris: Tối ưu hiệu năng kết xuất PDF và DJVU, hỗ trợ bộ nhớ đệm trang thông minh.\n  - Tự Động Nhận Diện Bảng Mã Tiếng Việt: Đọc mượt mà mọi tệp TXT (UTF-8, UTF-16, TCVN3, Windows-1258, VNI) không lỗi phông.\n  - Trang Thông Tin Toàn Diện: Màn hình thông tin chuẩn BB10 với 3 tab: Giới thiệu, Hướng dẫn phím tắt và Nhật ký phiên bản.\n\n• Phiên bản 1.0.0 (Bản dựng ban đầu):\n  - Phát hành nền tảng đọc sách Cascades thuần C++ / Qt 4.8.\n  - Hỗ trợ định dạng EPUB 2/3 với trích xuất ảnh bìa và mục lục.\n  - Hỗ trợ Active Frame hiển thị tiến độ đọc ngoài màn hình chủ.",
        "• Build 65 - 66 (Latest):\n  - Monochrome Library Icon: Converted Library tab icon to clean black-and-white, matching the native BlackBerry 10 design.\n  - Complete Bilingual Sync: 100% unified English and Vietnamese for About, Help, Settings, and Action menus.\n  - Minimalist Reading Card: Removed distracting borders and redundant badges for a clean reading card.\n  - Polished Voice Reading (TTS): Refined text-to-speech reading on book pages, with Spacebar pause/play and auto-page turning.\n  - Streamlined UI Descriptions: Condensed menu labels and settings descriptions for quick glancing on compact BB10 screens.\n\n• Build 50 - 64:\n  - Persistent Data Storage: Automatically saves and restores reading history and progress in shared documents across reinstalls.\n  - Offline SQLite Dictionary: Instant word definition and translation popup when selecting text, no internet required.\n  - Reading Statistics Sheet: Detailed tracking of reading sessions, reading speed, and completion estimates.\n  - Comprehensive Keyboard Shortcuts: Full physical keyboard navigation for BlackBerry Passport, Classic Q20, and Q10.\n\n• Build 30 - 49 (Version 2.0.0):\n  - 5-Tab Navigation Drawer: Native slide-out menu with Reading, Recent, Contents, Bookshelf, and File Browser.\n  - Optimized Libris Engine: High-performance rendering for PDF and DJVU with intelligent memory page caching.\n  - Automatic Vietnamese Charset Detection: Seamlessly reads TXT files in UTF-8, UTF-16, TCVN3, Windows-1258, and VNI.\n  - Full Info Screen: Standard BB10 page layout featuring About, Keyboard Guides, and Version Changelog.\n\n• Version 1.0.0 (Initial Release):\n  - Native BlackBerry Cascades C++ / Qt 4.8 e-book reader launch.\n  - EPUB 2/3 parser with cover image extraction and NCX TOC navigation.\n  - Active Frame cover displaying reading progress on the homescreen.",
        "• Bản dựng Build 65 - 66 (Mới nhất):\n  - Icon Thư Viện Đơn Sắc: Chuyển icon Thư viện sang màu đen trắng (monochrome) đồng bộ với toàn bộ thanh Tab và Action Bar chuẩn BlackBerry 10.\n  - Đồng Bộ Ngôn Ngữ Toàn Diện: Chuẩn hóa 100% tiếng Việt & tiếng Anh cho Giới Thiệu (About), Hướng Dẫn (Help), Cài Đặt (Settings) và các menu.\n  - Khối Đang Đọc Tinh Gọn: Loại bỏ viền xanh và icon thừa, mang lại diện mạo hiện đại, sạch sẽ cho thẻ sách đang đọc.\n  - Đọc Giọng Nói (TTS) Hoàn Thiện: Tối ưu đọc TTS trên trang sách, hỗ trợ tạm dừng bằng phím Space và tự động cuộn trang theo giọng đọc.\n  - Tối Ưu Hóa Giao Diện: Rút gọn mô tả các mục menu và cài đặt, giúp thao tác nhanh trên màn hình nhỏ.\n\n• Bản dựng Build 50 - 64:\n  - Bảo Toàn Dữ Liệu Sau Cài Lại: Tự động lưu và khôi phục lịch sử đọc, tiến độ và ghi chú vào bộ nhớ ngoài (/accounts/1000/shared/documents/Libris).\n  - Từ Điển Ngoại Tuyến (Offline Dictionary): Tra cứu từ điển SQLite tức thì khi bôi đen từ trong sách mà không cần kết nối mạng.\n  - Bảng Thống Kê Đọc Sách (Reading Stats): Theo dõi thời gian đọc, tốc độ đọc và tiến độ hoàn thành sách chi tiết.\n  - Bộ Phím Tắt Toàn Diện: Tối ưu hóa tối đa cho bàn phím vật lý BlackBerry Passport, Classic Q20 và Q10.\n\n• Bản dựng Build 30 - 49 (Phiên bản 2.0.0):\n  - Navigation Drawer 5 Tab Native: Giao diện chuẩn 5 Tab Native với ngăn kéo trượt trái: Đang đọc, Gần đây, Mục lục, Tủ sách, Duyệt tệp.\n  - Nâng Cấp Lõi Libris: Tối ưu hiệu năng kết xuất PDF và DJVU, hỗ trợ bộ nhớ đệm trang thông minh.\n  - Tự Động Nhận Diện Bảng Mã Tiếng Việt: Đọc mượt mà mọi tệp TXT (UTF-8, UTF-16, TCVN3, Windows-1258, VNI) không lỗi phông.\n  - Trang Thông Tin Toàn Diện: Màn hình thông tin chuẩn BB10 với 3 tab: Giới thiệu, Hướng dẫn phím tắt và Nhật ký phiên bản.\n\n• Phiên bản 1.0.0 (Bản dựng ban đầu):\n  - Phát hành nền tảng đọc sách Cascades thuần C++ / Qt 4.8.\n  - Hỗ trợ định dạng EPUB 2/3 với trích xuất ảnh bìa và mục lục.\n  - Hỗ trợ Active Frame hiển thị tiến độ đọc ngoài màn hình chủ.");
    
    // --- ReaderPage Actions & Overlays ---
    add("Đi Đến (Mục Lục)", "Go to (Contents)", "Đi Đến (Mục Lục)");
    add("Đánh Dấu Sách", "Add Bookmark", "Đánh Dấu Sách");
    add("Bỏ Đánh Dấu", "Remove Bookmark", "Bỏ Đánh Dấu");
    add("Chế độ Reflow", "Text Reflow", "Chế độ Reflow");
    add("Đảo Màu Nền", "Invert Colors", "Đảo Màu Nền");
    add("Đổi Màu (Đêm / Ngày)", "Invert Colors (Night / Day)", "Đổi Màu (Đêm / Ngày)");
    add("Đọc Sách (TTS)", "Read Aloud (TTS)", "Đọc Sách (TTS)");
    add("Dừng Giọng Đọc", "Stop Voice", "Dừng Giọng Đọc");
    add("Xoay trang PDF:", "Rotate PDF:", "Xoay trang PDF:");
    add("Xoay 90°", "Rotate 90°", "Xoay 90°");
    add("TÙY CHỌN TÀI LIỆU PDF", "PDF OPTIONS", "TÙY CHỌN TÀI LIỆU PDF");
    add("Về Tủ Sách", "Back to Bookshelf", "Về Tủ Sách");
    add("Thông Tin Sách", "Book Information", "Thông Tin Sách");
    add("MỤC LỤC & ĐIỀU HƯỚNG", "CONTENTS & NAVIGATION", "MỤC LỤC & ĐIỀU HƯỚNG");
    add("Đọc cuốn sách này", "Read this book", "Đọc cuốn sách này");

    // --- FilterSortSheet & Library Options ---
    add("BỘ SƯU TẬP & TRẠNG THÁI", "COLLECTION & STATUS", "BỘ SƯU TẬP & TRẠNG THÁI");
    add("Lọc & Sắp Xếp", "Filter & Sort", "Lọc & Sắp Xếp");
    add("Filter & Sort", "Filter & Sort", "Lọc & Sắp Xếp");
    add("Hiển thị:", "Show:", "Hiển thị:");
    add("Tủ sách (Đang đọc & Hoành thành)", "Bookshelf (Reading & Finished)", "Tủ sách (Đang đọc & Hoàn thành)");
    add("Tủ sách (Đang đọc & Hoàn thành)", "Bookshelf (Reading & Finished)", "Tủ sách (Đang đọc & Hoàn thành)");
    add("Chỉ sách đang đọc", "Currently reading only", "Chỉ sách đang đọc");
    add("Tất cả sách trong máy", "All books on device", "Tất cả sách trong máy");
    add("★ Sách yêu thích", "★ Favorites", "★ Sách yêu thích");
    add("📚 Danh sách đọc", "📚 Reading List", "📚 Danh sách đọc");
    add("✓ Đã hoàn thành", "✓ Finished", "✓ Đã hoàn thành");
    add("○ Chưa đọc", "○ Unread", "○ Chưa đọc");
    add("NHÓM SÁCH THEO", "GROUP BOOKS BY", "NHÓM SÁCH THEO");
    add("Phân nhóm:", "Group by:", "Phân nhóm:");
    add("Không phân nhóm (Mặc định)", "No grouping (Default)", "Không phân nhóm (Mặc định)");
    add("Theo Tác giả (Author)", "By Author", "Theo Tác giả (Author)");
    add("Theo Series (Bộ sách)", "By Series", "Theo Series (Bộ sách)");
    add(" • Theo Tác giả", " • By Author", " • Theo Tác giả");
    add(" • Theo Series", " • By Series", " • Theo Series");
    add("LỌC ĐỊNH DẠNG FILE", "FILTER BY FORMAT", "LỌC ĐỊNH DẠNG FILE");
    add("Định dạng:", "Format:", "Định dạng:");
    add("Bộ lọc và thứ tự sắp xếp được lưu tự động cho các lần mở sau.", "Filter and sort preferences are saved automatically.", "Bộ lọc và thứ tự sắp xếp được lưu tự động cho các lần mở sau.");
    add("Bỏ yêu thích", "Remove from Favorites", "Bỏ yêu thích");
    add("Yêu thích", "Add to Favorites", "Yêu thích");
    add("Bỏ khỏi DS đọc", "Remove from Reading List", "Bỏ khỏi DS đọc");
    add("Thêm vào danh sách đọc", "Add to Reading List", "Thêm vào danh sách đọc");
    add("Đánh dấu chưa đọc", "Mark as Unread", "Đánh dấu chưa đọc");
    add("Đánh dấu đã đọc xong", "Mark as Finished", "Đánh dấu đã đọc xong");
    add("Xóa khỏi thư viện", "Remove from Library", "Xóa khỏi thư viện");
    add("Quét Lại Thư Viện", "Rescan Library", "Quét Lại Thư Viện");
    add("Đã thêm vào yêu thích: ", "Added to favorites: ", "Đã thêm vào yêu thích: ");
    add("Đã bỏ yêu thích: ", "Removed from favorites: ", "Đã bỏ yêu thích: ");
    add("Đã thêm vào DS đọc: ", "Added to reading list: ", "Đã thêm vào DS đọc: ");
    add("Đã bỏ khỏi DS đọc: ", "Removed from reading list: ", "Đã bỏ khỏi DS đọc: ");
    add("Đã đánh dấu đã đọc xong: ", "Marked as finished: ", "Đã đánh dấu đã đọc xong: ");
    add("Đã đánh dấu chưa đọc: ", "Marked as unread: ", "Đã đánh dấu chưa đọc: ");
    add("Đã xóa khỏi thư viện: ", "Removed from library: ", "Đã xóa khỏi thư viện: ");
    add("Đã đọc: ", "Read: ", "Đã đọc: ");

    // --- BookDetailSheet ---
    add("MÔ TẢ SÁCH", "BOOK DESCRIPTION", "MÔ TẢ SÁCH");
    add("THÔNG TIN XUẤT BẢN & TẬP TIN", "PUBLICATION & FILE INFO", "THÔNG TIN XUẤT BẢN & TẬP TIN");
    add("Thông tin sách", "Book Information", "Thông tin sách");
    add("Đang Đọc", "Currently Reading", "Đang Đọc");
    add("Đọc ngay", "Read Now", "Đọc ngay");
    add("Mở Tệp Sách", "Open Book File", "Mở Tệp Sách");
    add("Bìa sách", "Cover", "Bìa sách");
    add("Thể loại", "Genre", "Thể loại");
    add("Bộ sách", "Series", "Bộ sách");
    add("Ngày sửa đổi", "Date Modified", "Ngày sửa đổi");
    add("Đường dẫn", "Path", "Đường dẫn");
    add("Đường dẫn tệp", "File path", "Đường dẫn tệp");
    add("Dung lượng", "File Size", "Dung lượng");
    add("Tiến độ", "Progress", "Tiến độ");
    add("ĐẶC TÍNH NỔI BẬT", "KEY FEATURES", "ĐẶC TÍNH NỔI BẬT");
    add("Tr. ", "p. ", "Tr. ");
    add("Trang ", "Page ", "Trang ");
    add("Trang trước", "Previous page", "Trang trước");
    add("Trang sau", "Next page", "Trang sau");
    add("Tiếp Tục Đọc", "Continue Reading", "Tiếp Tục Đọc");
    add("Nhỏ (16px)", "Small (16px)", "Nhỏ (16px)");
    add("Rất nhỏ (14px)", "Very small (14px)", "Rất nhỏ (14px)");
    add("Rất lớn (26px)", "Very large (26px)", "Rất lớn (26px)");

    // --- Highlights & Notes ---
    add("Dấu trang & Ghi chú", "Bookmarks & Notes", "Dấu trang & Ghi chú");
    add("Dấu Trang & Ghi Chú", "Bookmarks & Notes", "Dấu Trang & Ghi Chú");
    add("Ghi chú & Dấu trang", "Notes & Bookmarks", "Ghi chú & Dấu trang");
    add("Dấu trang", "Bookmarks", "Dấu trang");
    add("Dấu Trang", "Bookmarks", "Dấu Trang");
    add("Đánh dấu trang", "Bookmark Page", "Đánh dấu trang");
    add("Bỏ dấu trang", "Remove Bookmark", "Bỏ dấu trang");
    add("Đã đánh dấu trang: ", "Bookmarked: ", "Đã đánh dấu trang: ");
    add("Đã bỏ dấu trang", "Bookmark removed", "Đã bỏ dấu trang");
    add("+ Đánh dấu trang hiện tại", "+ Bookmark Current Page", "+ Đánh dấu trang hiện tại");
    add("Đã đánh dấu trang này (Bấm để bỏ)", "Page Bookmarked (Tap to remove)", "Đã đánh dấu trang này (Bấm để bỏ)");
    add("Chưa có dấu trang nào", "No bookmarks yet", "Chưa có dấu trang nào");
    add("Chưa có dấu trang hoặc ghi chú nào", "No bookmarks or notes yet", "Chưa có dấu trang hoặc ghi chú nào");
    add("Đánh dấu trang hoặc lưu đoạn trích yêu thích khi đọc để xem lại tại đây.", "Bookmark pages or save quotes while reading to revisit them here.", "Đánh dấu trang hoặc lưu đoạn trích yêu thích khi đọc để xem lại tại đây.");
    add("Bấm vào '+ Đánh dấu trang' hoặc phím 'H' khi đọc sách để lưu lại trang này.", "Tap '+ Bookmark Current Page' or press 'H' while reading to save this page.", "Bấm vào '+ Đánh dấu trang' hoặc phím 'H' khi đọc sách để lưu lại trang này.");
    add("Đến trang này", "Go to page", "Đến trang này");
    add("Ghi Chú Dấu Trang", "Bookmark Note", "Ghi Chú Dấu Trang");
    add("Dấu trang này:", "This bookmark:", "Dấu trang này:");
    add("Lọc dấu trang hoặc ghi chú...", "Filter bookmarks or notes...", "Lọc dấu trang hoặc ghi chú...");
    add("Highlight", "Highlights", "Ghi Chú");
    add("Highlights", "Highlights", "Ghi Chú");
    add("Ghi Chú", "Notes", "Ghi Chú");
    add("Ghi chú", "Notes", "Ghi chú");
    add("Ghi Chú Highlight", "Highlight Note", "Ghi Chú Highlight");
    add("HIGHLIGHT & GHI CHÚ", "HIGHLIGHTS & NOTES", "HIGHLIGHT & GHI CHÚ");
    add("Điểm nổi bật & Ghi chú", "Highlights & Notes", "Điểm nổi bật & Ghi chú");
    add("Chưa có đoạn highlight nào", "No highlights yet", "Chưa có đoạn highlight nào");
    add("Hãy bôi đen văn bản khi đọc sách và chọn 'Highlight' để lưu lại và thêm ghi chú.", "Select text while reading and tap 'Highlight' to save and add notes.", "Hãy bôi đen văn bản khi đọc sách và chọn 'Highlight' để lưu lại và thêm ghi chú.");
    add("Cuốn sách này", "This book", "Cuốn sách này");
    add("Tất cả sách", "All books", "Tất cả sách");
    add("Lọc đoạn trích hoặc ghi chú...", "Filter quotes or notes...", "Lọc đoạn trích hoặc ghi chú...");
    add("Đoạn trích:", "Quote:", "Đoạn trích:");
    add("Ghi chú của bạn:", "Your note:", "Ghi chú của bạn:");
    add("Ghi chú:", "Note:", "Ghi chú:");
    add("Nhập ghi chú cho đoạn này...", "Enter note for this quote...", "Nhập ghi chú cho đoạn này...");
    add("Màu sắc:", "Color:", "Màu sắc:");
    add("Vàng", "Yellow", "Vàng");
    add("Xanh lá", "Green", "Xanh lá");
    add("Xanh dương", "Blue", "Xanh dương");
    add("Hồng", "Pink", "Hồng");
    add("Lưu Ghi Chú", "Save Note", "Lưu Ghi Chú");
    add("Sửa ghi chú", "Edit note", "Sửa ghi chú");
    add("Thêm ghi chú", "Add note", "Thêm ghi chú");
    add("Đọc đoạn này", "Read quote", "Đọc đoạn này");
    add("Xóa Highlight", "Delete Highlight", "Xóa Highlight");
    add("Xóa", "Delete", "Xóa");
    add("Xem", "View", "Xem");
    add("đoạn trích đã lưu", "saved quotes", "đoạn trích đã lưu");

    // --- Reading Statistics & Calendar (KOReader style) ---
    add("Thống Kê Đọc Sách", "Reading Statistics", "Thống Kê Đọc Sách");
    add("Lịch Thống Kê", "Reading Stats", "Lịch Thống Kê");
    add("Lịch Thống Kê Đọc Sách", "Statistics Calendar", "Lịch Thống Kê");
    add("Xem lịch đọc hàng tháng, chuỗi ngày & chi tiết từng ngày", "View monthly reading calendar, streaks & daily details", "Xem lịch đọc hàng tháng, chuỗi ngày & chi tiết từng ngày");
    add("LỊCH ĐỌC THEO THÁNG", "MONTHLY READING CALENDAR", "LỊCH ĐỌC THEO THÁNG");
    add("TỔNG THỜI GIAN THÁNG", "MONTHLY TOTAL TIME", "TỔNG THỜI GIAN THÁNG");
    add("SỐ NGÀY ĐỌC", "READING DAYS", "SỐ NGÀY ĐỌC");
    add("TRUNG BÌNH / NGÀY", "DAILY AVERAGE", "TRUNG BÌNH / NGÀY");
    add("CHUỖI DÀI NHẤT", "LONGEST STREAK", "CHUỖI DÀI NHẤT");
    add("CHI TIẾT NGÀY ĐÃ CHỌN", "SELECTED DAY DETAILS", "CHI TIẾT NGÀY ĐÃ CHỌN");
    add("TỔNG TÍCH LŨY TOÀN THỜI GIAN", "ALL-TIME READING SUMMARY", "TỔNG THỜI GIAN ĐỌC");
    add("Tổng thời gian đọc tích lũy", "Total cumulative reading time", "Tổng thời gian đọc tích lũy");
    add("Tổng số ngày đã đọc sách", "Total days with reading", "Tổng số ngày đã đọc sách");
    add("Chuỗi ngày đọc hiện tại", "Current reading streak", "Chuỗi ngày đọc hiện tại");
    add("Kỷ lục ngày đọc nhiều nhất", "Single day reading record", "Kỷ lục ngày đọc nhiều nhất");
    add("Tổng kết tủ sách", "Bookshelf reading summary", "Tổng kết tủ sách");
    add("Không có hoạt động đọc sách trong ngày này.", "No reading activity on this day.", "Không có hoạt động đọc sách trong ngày này.");
    add("Chưa có thời gian đọc", "No reading time recorded", "Chưa có thời gian đọc");
    add("Tổng thời gian đọc: ", "Total reading time: ", "Tổng thời gian đọc: ");
    add("Ít hơn", "Less", "Ít hơn");
    add("Nhiều hơn", "More", "Nhiều hơn");
    add("ngày liên tục", "consecutive days", "ngày liên tục");
    add("ngày", "days", "ngày");
    add("đã xong", "finished", "đã xong");
    add("đang đọc", "reading", "đang đọc");
    add("Chưa có", "None", "Chưa có");
    add("Đọc: ", "Read: ", "Đọc: ");
    add("Tiến độ: ", "Progress: ", "Tiến độ: ");
    add("T2", "Mon", "T2");
    add("T3", "Tue", "T3");
    add("T4", "Wed", "T4");
    add("T5", "Thu", "T5");
    add("T6", "Fri", "T6");
    add("T7", "Sat", "T7");
    add("CN", "Sun", "CN");
    add("Ngày", "Date", "Ngày");

    // KOReader-style Reading Stats Modes
    add("Lịch đọc", "Calendar", "Lịch đọc");
    add("Theo sách", "By Book", "Theo sách");
    add("Xu hướng", "Trends", "Xu hướng");
    add("Tổng quan", "Overview", "Tổng quan");
    add("SÁCH ĐANG CHỌN", "SELECTED BOOK", "SÁCH ĐANG CHỌN");
    add("TẤT CẢ SÁCH TRONG THỐNG KÊ", "ALL BOOKS IN STATS", "TẤT CẢ SÁCH TRONG THỐNG KÊ");
    add("XU HƯỚNG 7 NGÀY GẦN NHẤT", "LAST 7 DAYS TREND", "XU HƯỚNG 7 NGÀY GẦN NHẤT");
    add("PHÂN BỔ THEO ĐỊNH DẠNG TÀI LIỆU", "DOCUMENT FORMAT DISTRIBUTION", "PHÂN BỔ THEO ĐỊNH DẠNG TÀI LIỆU");
    add("Đọc tiếp cuốn sách này", "Continue Reading This Book", "Đọc tiếp cuốn sách này");
    add("THỜI GIAN ĐÃ ĐỌC", "TIME READ", "THỜI GIAN ĐÃ ĐỌC");
    add("ƯỚC TÍNH CÒN LẠI", "ESTIMATED REMAINING", "ƯỚC TÍNH CÒN LẠI");
    add("TỐC ĐỘ ĐỌC", "READING SPEED", "TỐC ĐỘ ĐỌC");
    add("TRANG ĐÃ ĐỌC", "PAGES READ", "TRANG ĐÃ ĐỌC");
    add("TỔNG TUẦN NÀY", "THIS WEEK TOTAL", "TỔNG TUẦN NÀY");
    add("TRUNG BÌNH/NGÀY", "DAILY AVERAGE", "TRUNG BÌNH/NGÀY");
    add("CAO NHẤT", "RECORD", "CAO NHẤT");
    add("TỔNG QUAN TÍCH LŨY", "ALL-TIME OVERVIEW", "TỔNG QUAN");
    add("TỔNG THỜI GIAN ĐÃ ĐỌC", "TOTAL READING TIME", "TỔNG THỜI GIAN ĐÃ ĐỌC");
    add("TỔNG SỐ NGÀY ĐÃ ĐỌC", "TOTAL READING DAYS", "TỔNG SỐ NGÀY ĐÃ ĐỌC");
    add("CHUỖI HIỆN TẠI", "CURRENT STREAK", "CHUỖI HIỆN TẠI");
    add("CHUỖI KỶ LỤC", "LONGEST STREAK", "CHUỖI KỶ LỤC");
    add("Kỷ lục chuỗi ngày dài nhất", "Longest reading streak record", "Kỷ lục chuỗi ngày dài nhất");
    add("NGÀY ĐỌC NHIỀU NHẤT", "BEST READING DAY", "NGÀY ĐỌC NHIỀU NHẤT");
    add("TIẾN ĐỘ TỦ SÁCH", "BOOKSHELF PROGRESS", "TIẾN ĐỘ TỦ SÁCH");
    add("cuốn", "books", "cuốn");
    add("Chưa có dữ liệu sách đọc. Hãy mở một cuốn sách để bắt đầu ghi nhận thống kê!",
        "No book reading data yet. Open a book to start tracking statistics!",
        "Chưa có dữ liệu sách đọc. Hãy mở một cuốn sách để bắt đầu ghi nhận thống kê!");

    // Persistent Shared Storage & Backup
    add("LƯU TRỮ & BẢO TOÀN DỮ LIỆU", "DATA STORAGE & BACKUP", "LƯU TRỮ & BẢO TOÀN DỮ LIỆU");
    add("Thư mục lưu trữ bảo toàn:", "Persistent storage directory:", "Thư mục lưu trữ bảo toàn:");
    add("Sao Lưu Ngay", "Backup Now", "Sao Lưu Ngay");
    add("Khôi Phục", "Restore", "Khôi Phục");
    add("Đã đồng bộ dữ liệu vào ", "Data synced to ", "Đã đồng bộ dữ liệu vào ");
    add("Đã khôi phục toàn bộ dữ liệu từ bộ nhớ ngoài.", "Successfully restored all data from external storage.", "Đã khôi phục toàn bộ dữ liệu từ bộ nhớ ngoài.");
    add("Dữ liệu đã được đồng bộ với bộ nhớ ngoài.", "Data is synchronized with external storage.", "Dữ liệu đã được đồng bộ với bộ nhớ ngoài.");
    add("Đã khôi phục từ tệp JSON.", "Restored data from JSON file.", "Đã khôi phục từ tệp JSON.");
    add("Bộ nhớ ngoài sẵn sàng: ", "External storage ready: ", "Bộ nhớ ngoài sẵn sàng: ");
    add("Libris tự động lưu toàn bộ lịch sử đọc, tiến độ đọc của từng cuốn sách, sách đang đọc và ghi chú vào bộ nhớ ngoài dùng chung. Khi bạn cài lại ứng dụng hoặc cập nhật bản mới, toàn bộ dữ liệu đọc sẽ được tự động bảo toàn 100%.",
        "Libris automatically saves your reading history, per-book progress, currently reading book, and highlights into shared external storage. When you reinstall or update the app, your reading data will be 100% preserved.",
        "Libris tự động lưu toàn bộ lịch sử đọc, tiến độ đọc của từng cuốn sách, sách đang đọc và ghi chú vào bộ nhớ ngoài dùng chung. Khi bạn cài lại ứng dụng hoặc cập nhật bản mới, toàn bộ dữ liệu đọc sẽ được tự động bảo toàn 100%.");

    // --- Navigation Tabs & Main App ---
    add("Gần đây", "Recent", "Gần đây");
    add("Thư viện", "Library", "Thư viện");
    add("Thư Viện", "Library", "Thư Viện");
    add("Thư mục", "Files", "Thư mục");
    add("Mục lục", "Contents", "Mục lục");
    add("Mục Lục", "Table of Contents", "Mục Lục");
    add("Mục Lục Sách", "Book Table of Contents", "Mục Lục Sách");
    add("Tiến Độ", "Reading Stats", "Tiến Độ");
    add("SÁCH GẦN ĐÂY", "RECENT BOOKS", "SÁCH GẦN ĐÂY");
    add("Hãy đóng sách trước khi mở sách mới", "Please close the book before opening a new one", "Hãy đóng sách trước khi mở sách mới");
    add("Không thể khởi tạo giao diện đọc sách", "Failed to initialize reading interface", "Không thể khởi tạo giao diện đọc sách");
    add("Không thể khởi tạo giao diện đọc sách:\n", "Failed to initialize reading interface:\n", "Không thể khởi tạo giao diện đọc sách:\n");
    add("Không thể mở sách. File có thể bị hỏng hoặc không được hỗ trợ.", "Cannot open book. File may be corrupted or unsupported.", "Không thể mở sách. File có thể bị hỏng hoặc không được hỗ trợ.");
    add("Hãy mở sách từ 'Thư Viện' hoặc bấm 'Mở Tệp' để bắt đầu đọc cùng Libris!", "Open a book from 'Library' or tap 'Open File' to start reading with Libris!", "Hãy mở sách từ 'Thư Viện' hoặc bấm 'Mở Tệp' để bắt đầu đọc cùng Libris!");
    add("Libris", "Libris", "Libris");
    add("Libris TTS", "Libris TTS", "Libris TTS");

    // --- Library Page & Bookshelf ---
    add("Xem Danh Sách", "List View", "Xem Danh Sách");
    add("Xem Dạng Lưới", "Grid View", "Xem Dạng Lưới");
    add("Bắt đầu đọc", "Start Reading", "Bắt đầu đọc");
    add("Đọc lại từ đầu", "Read from Start", "Đọc lại từ đầu");
    add("Chưa có sách yêu thích", "No favorite books", "Chưa có sách yêu thích");
    add("Chưa có sách nào yêu thích", "No favorite books yet", "Chưa có sách nào yêu thích");
    add("Hãy bấm vào biểu tượng đánh dấu trong trình đọc để thêm sách vào mục Yêu Thích.", "Tap the bookmark icon in the reader to add books to Favorites.", "Hãy bấm vào biểu tượng đánh dấu trong trình đọc để thêm sách vào mục Yêu Thích.");
    add("Tủ sách của bạn đang trống hoặc bạn chưa đánh dấu sách yêu thích nào.\nHãy mở tệp hoặc duyệt thư mục để bắt đầu đọc và đánh dấu yêu thích!",
        "Your bookshelf is empty or you haven't marked any favorite books.\nOpen a file or browse folders to start reading and add favorites!",
        "Tủ sách của bạn đang trống hoặc bạn chưa đánh dấu sách yêu thích nào.\nHãy mở tệp hoặc duyệt thư mục để bắt đầu đọc và đánh dấu yêu thích!");

    // --- Reader Mode & Overflows ---
    add("Sepia", "Sepia", "Nâu ấm (Sepia)");
    add("Nâu ấm (Sepia)", "Sepia", "Nâu ấm (Sepia)");
    add("Chế độ văn bản (Reflow OCR)", "Text Mode (Reflow OCR)", "PDF Reflow (Văn bản)");
    add("Chế độ xem PDF gốc", "Original PDF View", "PDF gốc");
    add("Đọc sách (TTS)", "Read Aloud (TTS)", "Đọc sách (TTS)");
    add("Dừng đọc TTS", "Stop TTS", "Dừng đọc TTS");
    add("Đánh Dấu Sách", "Bookmark Book", "Đánh Dấu Sách");
    add("Bỏ Đánh Dấu", "Remove Bookmark", "Bỏ Đánh Dấu");
    add("Về Tủ Sách", "To Bookshelf", "Về Tủ Sách");
    add("Thông tin sách", "Book info", "Thông tin sách");
    add("Từ điển & Dịch", "Dictionary & Translate", "Từ điển & Dịch");

    // --- Settings Categories & Descriptions ---
    add("Chủ đề, phông chữ & lề trang", "Themes, fonts & page margins", "Chủ đề, phông chữ & lề trang");
    add("Phương thức chuyển trang", "Page turn method", "Phương thức chuyển trang");
    add("Tiến độ đọc, pin & đồng hồ", "Reading progress, battery & clock", "Tiến độ đọc, pin & đồng hồ");
    add("Giọng đọc, ngôn ngữ & ngoại tuyến", "Voices, languages & offline playback", "Giọng đọc, ngôn ngữ & phát Offline");
    add("Bố cục tủ sách & quét lại dữ liệu", "Bookshelf layout & rescan data", "Bố cục tủ sách & quét lại dữ liệu");
    add("Phiên bản v2.0 & trợ giúp", "Version 2.0 & help guide", "Phiên bản v2.0 & trợ giúp");

    // --- Display & Font Settings ---
    add("Phông chữ", "Font Family", "Phông chữ");
    add("Font", "Font Family", "Phông chữ");
    add("Văn bản mẫu: Trực quan cỡ chữ, màu nền và lề trang.", "Sample text: Preview font size, theme color, and page margins.", "Văn bản mẫu: Trực quan cỡ chữ, màu nền và lề trang.");
    add("Đảo màu tối PDF:", "Invert PDF colors:", "Đảo màu tối PDF:");
    add("Xoay trang PDF:", "Rotate PDF page:", "Xoay trang PDF:");
    add("Xoay 90°", "Rotate 90°", "Xoay 90°");
    add("TÙY CHỌN TÀI LIỆU PDF", "PDF DOCUMENT OPTIONS", "TÙY CHỌN TÀI LIỆU PDF");
    add("Đặt lại viền cân bằng (16px)", "Reset margins (16px)", "Đặt lại viền cân bằng (16px)");

    // --- Reading & Footer Settings ---
    add("CHẾ ĐỘ ĐỌC SÁCH", "READING MODE", "CHẾ ĐỘ ĐỌC SÁCH");
    add("Vuốt", "Paged", "Vuốt");
    add("Cuộn", "Scroll", "Cuộn");
    add("MỤC HIỂN THỊ DƯỚI ĐÁY TRANG", "ITEMS SHOWN AT PAGE BOTTOM", "MỤC HIỂN THỊ DƯỚI ĐÁY TRANG");
    add("Tên chương hiện tại", "Current chapter title", "Tên chương hiện tại");
    add("Tiến độ đọc (% và trang)", "Reading progress (% and page)", "Tiến độ đọc (% và trang)");
    add("Mức pin thiết bị", "Device battery level", "Mức pin thiết bị");
    add("Đồng hồ thời gian", "Time clock", "Đồng hồ thời gian");

    // --- Library & Storage Settings ---
    add("GIAO DIỆN TỦ SÁCH", "BOOKSHELF VIEW", "GIAO DIỆN TỦ SÁCH");
    add("Kiểu hiển thị tủ sách", "Bookshelf view style", "Kiểu hiển thị tủ sách");
    add("Lưới bìa sách", "Mosaic Grid", "Lưới bìa sách");
    add("Danh sách chi tiết", "Detailed List", "Danh sách chi tiết");
    add("QUÉT LẠI DỮ LIỆU SÁCH", "RESCAN BOOK DATA", "QUÉT LẠI DỮ LIỆU SÁCH");
    add("Quét lại toàn bộ thẻ nhớ và thiết bị", "Rescan all storage and memory card", "Quét lại toàn bộ thẻ nhớ và thiết bị");
    add("Libris tự động lưu toàn bộ lịch sử đọc, tiến độ đọc của từng cuốn sách, sách đang đọc và ghi chú vào bộ nhớ ngoài dùng chung. Khi bạn cài lại ứng dụng hoặc cập nhật bản mới, toàn bộ dữ liệu đọc sẽ được tự động bảo toàn 100%.",
        "Libris automatically saves your entire reading history, per-book progress, current reading book, and highlights to shared external storage. When you reinstall or update the app, all reading data is 100% preserved.",
        "Libris tự động lưu toàn bộ lịch sử đọc, tiến độ đọc của từng cuốn sách, sách đang đọc và ghi chú vào bộ nhớ ngoài dùng chung. Khi bạn cài lại ứng dụng hoặc cập nhật bản mới, toàn bộ dữ liệu đọc sẽ được tự động bảo toàn 100%.");

    // --- TTS & Voice Settings ---
    add("BỘ MÁY & GIỌNG ĐỌC", "VOICE ENGINE & VOICES", "BỘ MÁY & GIỌNG ĐỌC");
    add("Bộ máy đọc", "Voice engine", "Bộ máy đọc");
    add("Google Dịch", "Google Translate TTS", "Google Dịch");
    add("Google Gemini AI", "Google Gemini AI", "Google Gemini AI");
    add("Khóa API Google AI Studio:", "Google AI Studio API Key:", "Khóa API Google AI Studio:");
    add("Kiểm tra API Key", "Test API Key", "Kiểm tra API Key");
    add("Đang kiểm tra kết nối...", "Testing connection...", "Đang kiểm tra kết nối...");
    add("Mô hình AI", "AI Model", "Mô hình AI");
    add("Giọng đọc AI", "AI Voice", "Giọng đọc AI");
    add("Phong cách đọc", "Reading style", "Phong cách đọc");
    add("Tự động theo sách", "Auto by book genre", "Tự động theo sách");
    add("Kể chuyện, truyền cảm", "Storytelling, expressive", "Kể chuyện, truyền cảm");
    add("Trang trọng, chuẩn mực", "Formal, authoritative", "Trang trọng, chuẩn mực");
    add("Thư thái, nhẹ nhàng", "Relaxed, soothing", "Thư thái, nhẹ nhàng");
    add("Tùy chỉnh", "Custom", "Tùy chỉnh");
    add("NGÔN NGỮ & TỐC ĐỘ", "LANGUAGE & SPEED", "NGÔN NGỮ & TỐC ĐỘ");
    add("Ngôn ngữ nội dung", "Content language", "Ngôn ngữ nội dung");
    add("Tự động nhận diện (Khuyên dùng)", "Auto Detect (Recommended)", "Tự động nhận diện (Khuyên dùng)");
    add("Chậm", "Slow", "Chậm");
    add("Nhanh", "Fast", "Nhanh");
    add("Rất nhanh", "Very fast", "Rất nhanh");
    add("Nghe thử giọng đọc đã chọn", "Preview Selected Voice", "Nghe thử giọng đọc đã chọn");
    add("TÙY CHỌN PHÁT & NGOẠI TUYẾN", "PLAYBACK & OFFLINE OPTIONS", "TÙY CHỌN PHÁT & NGOẠI TUYẾN");
    add("Chế độ đọc", "Playback mode", "Chế độ đọc");
    add("Trực tuyến", "Online", "Online");
    add("Ngoại tuyến", "Offline", "Offline");
    add("Tự động chuyển chương", "Auto next chapter", "Tự động chuyển chương");
    add("Tải toàn bộ chương về máy để nghe Offline", "Download entire chapter for Offline listening", "Tải toàn bộ chương về máy để nghe Offline");
    add("Đang tải: ", "Downloading: ", "Đang tải: ");
    add("Puck (Nam - Ấm áp, tự nhiên)", "Puck (Male - Warm, Natural)", "Puck (Nam - Ấm áp, tự nhiên)");
    add("Charon (Nam - Trầm tĩnh, truyền cảm)", "Charon (Male - Deep, Expressive)", "Charon (Nam - Trầm tĩnh, truyền cảm)");
    add("Kore (Nữ - Trong trẻo, thanh lịch)", "Kore (Female - Clear, Elegant)", "Kore (Nữ - Trong trẻo, thanh lịch)");
    add("Fenrir (Nam - Dứt khoát, uy lực)", "Fenrir (Male - Decisive, Authoritative)", "Fenrir (Nam - Dứt khoát, uy lực)");
    add("Aoede (Nữ - Nhẹ nhàng, êm dịu)", "Aoede (Female - Gentle, Soothing)", "Aoede (Nữ - Nhẹ nhàng, êm dịu)");
    add("Zephyr (Nữ - Truyền cảm, sống động)", "Zephyr (Female - Dynamic, Expressive)", "Zephyr (Nữ - Truyền cảm, sống động)");
    add("Leda (Nữ - Điềm đạm, ấm áp)", "Leda (Female - Calm, Warm)", "Leda (Nữ - Điềm đạm, ấm áp)");
    add("Gemini 2.5 Flash (Khuyên dùng)", "Gemini 2.5 Flash (Recommended)", "Gemini 2.5 Flash (Khuyên dùng)");
    add("Gemini 2.5 Flash (Âm thanh gốc)", "Gemini 2.5 Flash (Native Audio)", "Gemini 2.5 Flash (Âm thanh gốc)");
    add("Gemini 3.8 Flash TTS", "Gemini 3.8 Flash TTS", "Gemini 3.8 Flash TTS");
    add("Tiếng Việt chuẩn (vi)", "Vietnamese (vi)", "Tiếng Việt chuẩn (vi)");
    add("English - Giọng Mỹ (en-US)", "English - US (en-US)", "English - Giọng Mỹ (en-US)");
    add("English - Giọng Anh (en-GB)", "English - UK (en-GB)", "English - Giọng Anh (en-GB)");
    add("English - Giọng Úc (en-AU)", "English - Australia (en-AU)", "English - Giọng Úc (en-AU)");
    add("English - Giọng Ấn Độ (en-IN)", "English - India (en-IN)", "English - Giọng Ấn Độ (en-IN)");
    add("Français - Pháp (fr-FR)", "French - France (fr-FR)", "Français - Pháp (fr-FR)");
    add("Français - Canada (fr-CA)", "French - Canada (fr-CA)", "Français - Canada (fr-CA)");
    add("Español - Tây Ban Nha (es-ES)", "Spanish - Spain (es-ES)", "Español - Tây Ban Nha (es-ES)");
    add("Español - Mỹ Latinh (es-US)", "Spanish - Latin America (es-US)", "Español - Mỹ Latinh (es-US)");
    add("Deutsch (de)", "German (de)", "Deutsch (de)");
    add("Русский (ru)", "Russian (ru)", "Русский (ru)");
    add("中文 - Giản thể (zh-CN)", "Chinese - Simplified (zh-CN)", "中文 - Giản thể (zh-CN)");
    add("中文 - Đài Loan (zh-TW)", "Chinese - Taiwan (zh-TW)", "中文 - Đài Loan (zh-TW)");
    add("中文 - Quảng Đông / HK (zh-HK)", "Chinese - Cantonese/HK (zh-HK)", "中文 - Quảng Đông / HK (zh-HK)");
    add("日本語 (ja)", "Japanese (ja)", "日本語 (ja)");
    add("한국어 (ko)", "Korean (ko)", "한국어 (ko)");

    // --- Reading Statistics ---
    add("Theo Tháng", "Monthly", "Theo Tháng");
    add("Theo Tuần", "Weekly", "Theo Tuần");
    add("HOẠT ĐỘNG ĐỌC TỪNG NGÀY TRONG TUẦN", "WEEKLY READING ACTIVITY", "HOẠT ĐỘNG ĐỌC TỪNG NGÀY TRONG TUẦN");
    add("TỔNG THỜI GIAN TUẦN", "TOTAL WEEK TIME", "TỔNG THỜI GIAN TUẦN");
    add("Tuần này", "This week", "Tuần này");
    add("ngày đọc liên tục", "consecutive reading days", "ngày đọc liên tục");
    add("mỗi ngày có đọc", "per active reading day", "mỗi ngày có đọc");
    add("ngày liên tiếp", "consecutive days", "ngày liên tiếp");
    add("thời gian đọc trong tháng", "reading time this month", "thời gian đọc trong tháng");
    add("tỷ lệ hoàn thành", "completion rate", "tỷ lệ hoàn thành");
    add("Đọc", "Read", "Đọc");
    add("Không có hoạt động đọc sách", "No reading activity", "Không có hoạt động đọc sách");
    add("LỊCH ĐỌC THEO THÁNG", "MONTHLY READING CALENDAR", "LỊCH ĐỌC THEO THÁNG");
    add("TỔNG TÍCH LŨY TOÀN THỜI GIAN", "ALL-TIME TOTAL READING", "TỔNG THỜI GIAN ĐỌC");

    // --- Filter & Sort Sheet ---
    add("Lưới bìa sách (Mosaic)", "Mosaic Grid", "Lưới bìa");
    add("✦ Danh sách đọc", "✦ Reading List", "✦ Danh sách đọc");
    add("Chế độ xem:", "View mode:", "Chế độ xem:");
    add("Không phân nhóm", "No grouping", "Không phân nhóm");
    add("KIỂU HIỂN THỊ TỦ SÁCH", "BOOKSHELF VIEW STYLE", "KIỂU HIỂN THỊ TỦ SÁCH");

    // --- Browser & Cover ---
    add("Bộ nhớ thiết bị", "Device Storage", "Bộ nhớ thiết bị");
    add("Mở thư mục", "Open folder", "Mở thư mục");
    add("Chưa có sách đang đọc", "No book currently open", "Chưa có sách đang đọc");
    add("Trang bìa", "Cover", "Trang bìa");
    add("Hôm nay: ", "Today: ", "Hôm nay: ");

    // --- Reading & Actions ---
    add("Tác phẩm", "Book", "Tác phẩm");
    add("Thêm vào Yêu thích", "Add to Favorites", "Thêm vào Yêu thích");
    add("Bỏ khỏi Yêu thích", "Remove from Favorites", "Bỏ khỏi Yêu thích");
    add("Đánh dấu là chưa đọc", "Mark as unread", "Đánh dấu là chưa đọc");
    add("Xóa khỏi Đọc gần đây", "Remove from Recent", "Xóa khỏi Đọc gần đây");
    add("Xóa khỏi gần đây", "Remove from Recent", "Xóa khỏi gần đây");
    add("Đã xóa khỏi gần đây: ", "Removed from recent: ", "Đã xóa khỏi gần đây: ");
    add("Đã xóa khỏi gần đây", "Removed from recent", "Đã xóa khỏi gần đây");
    add("Đã thêm vào yêu thích", "Added to favorites", "Đã thêm vào yêu thích");
    add("Đã bỏ yêu thích", "Removed from favorites", "Đã bỏ yêu thích");
    add("Đã thêm vào DS đọc", "Added to reading list", "Đã thêm vào DS đọc");
    add("Đã bỏ khỏi DS đọc", "Removed from reading list", "Đã bỏ khỏi DS đọc");

    // --- BookManager / C++ Engine UI Strings ---
    add("Không rõ tác giả", "Unknown author", "Không rõ tác giả");
    add("Trang %1 / %2", "Page %1 / %2", "Trang %1 / %2");
    add("%1 trang", "%1 pages", "%1 trang");
    add("Chương %1", "Chapter %1", "Chương %1");
    add("Chương %1 / %2", "Chapter %1 / %2", "Chương %1 / %2");
    add("%1 chương", "%1 chapters", "%1 chương");
    add("Đã khôi phục dữ liệu từ tệp JSON.", "Restored data from JSON file.", "Đã khôi phục dữ liệu từ tệp JSON.");
    add("Không thể kết nối máy chủ nội bộ. Đang tải lên đám mây (dpaste)...",
        "Cannot connect to local server. Uploading to cloud (dpaste)...",
        "Không thể kết nối máy chủ nội bộ. Đang tải lên đám mây (dpaste)...");
    add("Đã tải log lên đám mây và sao chép link vào bộ nhớ tạm: %1",
        "Uploaded log to cloud and copied link: %1",
        "Đã tải log lên đám mây và sao chép link vào bộ nhớ tạm: %1");
    add("Không thể gửi qua mạng. Đã lưu báo cáo tại: Documents/Libris_Debug.log",
        "Cannot send over network. Saved report to: Documents/Libris_Debug.log",
        "Không thể gửi qua mạng. Đã lưu báo cáo tại: Documents/Libris_Debug.log");
    add("Hết thời gian chờ kết nối. Đã lưu báo cáo tại: Documents/Libris_Debug.log",
        "Connection timed out. Saved report to: Documents/Libris_Debug.log",
        "Hết thời gian chờ kết nối. Đã lưu báo cáo tại: Documents/Libris_Debug.log");
    add("Đã gửi trực tiếp tới máy chủ phát triển (%1:8080) thành công!",
        "Successfully sent directly to dev server (%1:8080)!",
        "Đã gửi trực tiếp tới máy chủ phát triển (%1:8080) thành công!");

    // --- TtsManager Pitch Strings ---
    add("Trầm (0.8x)", "Low (0.8x)", "Trầm (0.8x)");
    add("Chuẩn (1.0x)", "Normal (1.0x)", "Chuẩn (1.0x)");
    add("Bổng (1.2x)", "High (1.2x)", "Bổng (1.2x)");
    add("Rất bổng (1.4x)", "Very high (1.4x)", "Rất bổng (1.4x)");

    // --- DictManager Strings ---
    add("Lỗi dịch thuật: ", "Translation error: ", "Lỗi dịch: ");
    add("Không thể kết nối đến từ điển trực tuyến: ", "Cannot connect to online dictionary: ", "Không thể kết nối từ điển online: ");
    add("Không tìm thấy định nghĩa cho từ này.", "No definition found for this word.", "Không tìm thấy định nghĩa cho từ này.");
    add("Không thể phân tích kết quả dịch thuật.", "Cannot parse translation results.", "Không thể đọc kết quả dịch.");

    // --- Concise UI & Menu Strings ---
    add("Hiển thị", "Display", "Hiển thị");
    add("Chủ đề & phông chữ", "Theme & font", "Chủ đề & phông chữ");
    add("Thao tác đọc", "Reading", "Thao tác đọc");
    add("Chế độ chuyển trang", "Page turn mode", "Chế độ chuyển trang");
    add("Chân trang", "Footer", "Chân trang");
    add("Tiến độ, pin & giờ", "Progress, battery & clock", "Tiến độ, pin & giờ");
    add("Bộ máy & giọng đọc", "Engine & voices", "Bộ máy & giọng đọc");
    add("Tủ sách", "Bookshelf", "Tủ sách");
    add("Bố cục & quét tệp", "Layout & scan", "Bố cục & quét tệp");
    add("Phiên bản & trợ giúp", "Version & help", "Phiên bản & trợ giúp");

    // Reader concise actions
    add("Đánh dấu", "Bookmark", "Đánh dấu");
    add("Bỏ đánh dấu", "Unmark", "Bỏ đánh dấu");
    add("PDF gốc", "Original PDF", "PDF gốc");
    add("Dàn chữ (OCR)", "Text Reflow", "Dàn chữ (OCR)");
    add("Đọc (TTS)", "Read (TTS)", "Đọc (TTS)");
    add("Dừng (TTS)", "Stop (TTS)", "Dừng (TTS)");
    add("Từ điển", "Dictionary", "Từ điển");
    add("Thông tin", "Info", "Thông tin");
    add("Đọc tiếp", "Continue", "Đọc tiếp");
    add("Chưa có sách gần đây", "No recent books", "Chưa có sách gần đây");
    add("Mở sách từ Thư viện hoặc chọn Mở tệp để đọc.", "Open a book from Library or select Open File to read.", "Mở sách từ Thư viện hoặc chọn Mở tệp để đọc.");

    // Library concise actions
    add("Bộ lọc", "Filter", "Bộ lọc");
    add("Dạng lưới", "Grid", "Dạng lưới");
    add("Quét lại", "Rescan", "Quét lại");
    add("Tủ sách đang trống. Hãy mở tệp hoặc duyệt thư mục để bắt đầu đọc!",
        "Bookshelf is empty. Open a file or browse folders to start reading!",
        "Tủ sách đang trống. Hãy mở tệp hoặc duyệt thư mục để bắt đầu đọc!");

    // TTS concise options
    add("Bộ máy", "Engine", "Bộ máy");
    add("Google API Key:", "Google API Key:", "Google API Key:");
    add("Kiểm tra", "Test", "Kiểm tra");
    add("Đang kiểm tra...", "Testing...", "Đang kiểm tra...");
    add("Giọng đọc", "Voice", "Giọng đọc");
    add("Puck (Nam - Ấm áp)", "Puck (Male - Warm)", "Puck (Nam - Ấm áp)");
    add("Charon (Nam - Trầm tĩnh)", "Charon (Male - Deep)", "Charon (Nam - Trầm tĩnh)");
    add("Kore (Nữ - Trong trẻo)", "Kore (Female - Clear)", "Kore (Nữ - Trong trẻo)");
    add("Fenrir (Nam - Uy lực)", "Fenrir (Male - Strong)", "Fenrir (Nam - Uy lực)");
    add("Aoede (Nữ - Êm dịu)", "Aoede (Female - Gentle)", "Aoede (Nữ - Êm dịu)");
    add("Zephyr (Nữ - Sống động)", "Zephyr (Female - Dynamic)", "Zephyr (Nữ - Sống động)");
    add("Leda (Nữ - Điềm đạm)", "Leda (Female - Calm)", "Leda (Nữ - Điềm đạm)");
    add("Mô hình", "Model", "Mô hình");
    add("Gemini 2.5 Flash", "Gemini 2.5 Flash", "Gemini 2.5 Flash");
    add("Phong cách", "Style", "Phong cách");
    add("Tự động", "Auto", "Tự động");
    add("Kể chuyện", "Story", "Kể chuyện");
    add("Trang trọng", "Formal", "Trang trọng");
    add("Thư thái", "Relaxed", "Thư thái");
    add("Ngôn ngữ", "Language", "Ngôn ngữ");
    add("Tự động nhận diện", "Auto Detect", "Tự động nhận diện");
    add("Tốc độ", "Speed", "Tốc độ");
    add("Cao độ", "Pitch", "Cao độ");
    add("Nghe thử giọng", "Test Voice", "Nghe thử giọng");
    add("PHÁT & NGOẠI TUYẾN", "PLAYBACK & OFFLINE", "PHÁT & NGOẠI TUYẾN");
    add("Chế độ", "Mode", "Chế độ");
    add("Tự chuyển chương", "Auto next chapter", "Tự chuyển chương");
    add("Tải chương để nghe Offline", "Download chapter for offline", "Tải chương để nghe Offline");
    add("Xóa bộ nhớ đệm", "Clear cache", "Xóa cache");

    // Display & Fonts concise
    add("GIAO DIỆN", "INTERFACE", "GIAO DIỆN");
    add("Cỡ chữ mẫu", "Preset Size", "Cỡ chữ mẫu");
    add("Cách dòng", "Line Spacing", "Cách dòng");
    add("CĂN LỀ", "MARGINS", "CĂN LỀ");
    add("Đặt lại viền (16px)", "Reset margins (16px)", "Đặt lại viền (16px)");
    add("Văn bản mẫu xem trước hiển thị.", "Sample preview text.", "Văn bản mẫu xem trước hiển thị.");
    add("TÙY CHỌN PDF", "PDF OPTIONS", "TÙY CHỌN PDF");
    add("Đảo màu:", "Invert:", "Đảo màu:");
    add("Xoay trang:", "Rotate:", "Xoay trang:");

    // Reading & Footer concise
    add("CHẾ ĐỘ ĐỌC", "READING MODE", "CHẾ ĐỘ ĐỌC");
    add("Chuyển trang", "Page Turn", "Chuyển trang");
    add("MỤC CHÂN TRANG", "FOOTER ITEMS", "MỤC CHÂN TRANG");
    add("Tên chương", "Chapter", "Tên chương");
    add("Tiến độ đọc", "Progress", "Tiến độ đọc");
    add("Mức pin", "Battery", "Mức pin");
    add("Đồng hồ", "Clock", "Đồng hồ");

    // Library & Storage concise
    add("Kiểu hiển thị", "Display style", "Kiểu hiển thị");
    add("Lưới bìa", "Grid", "Lưới bìa");
    add("QUÉT DỮ LIỆU", "SCAN DATA", "QUÉT DỮ LIỆU");
    add("Quét lại bộ nhớ & thẻ nhớ", "Rescan all storage", "Quét lại bộ nhớ & thẻ nhớ");
    add("BẢO TOÀN DỮ LIỆU", "DATA BACKUP", "BẢO TOÀN DỮ LIỆU");
    add("Tự động sao lưu lịch sử, tiến độ và ghi chú vào bộ nhớ chung khi cài lại app.",
        "Auto-backup history, progress and notes to shared storage across reinstalls.",
        "Tự động sao lưu lịch sử, tiến độ và ghi chú vào bộ nhớ chung khi cài lại app.");

    // Reading Stats concise
    add("Tháng", "Month", "Tháng");
    add("Tuần", "Week", "Tuần");
    add("LỊCH THÁNG", "MONTHLY CALENDAR", "LỊCH THÁNG");
    add("HOẠT ĐỘNG TUẦN", "WEEKLY ACTIVITY", "HOẠT ĐỘNG TUẦN");
    add("TỔNG TÍCH LŨY", "ALL-TIME STATS", "TỔNG THỐNG KÊ");

    // --- Settings Menu & Subtitles ---
    add("Chủ đề, phông chữ & màu nền", "Themes, fonts & color schemes", "Chủ đề, phông chữ & màu nền");
    add("Bố cục, quét tệp & lịch sử đọc", "Layout, file scan & reading history", "Bố cục, quét tệp & lịch sử đọc");
    add("Bộ máy đọc, giọng nói AI & tốc độ", "Engine, AI voice & reading speed", "Bộ máy đọc, giọng nói AI & tốc độ");
    add("Tên chương, tiến độ, pin & đồng hồ", "Chapter title, progress, battery & clock", "Tên chương, tiến độ, pin & đồng hồ");
    add("Chế độ cuộn & lật trang", "Scroll & page turn mode", "Chế độ cuộn & lật trang");
    add("Phiên bản Libris & hướng dẫn", "Libris version & user guide", "Phiên bản Libris & hướng dẫn");

    // --- Reading & Display Modes ---
    add("Chế độ Cuộn (Scroll)", "Continuous Scroll Mode", "Chế độ Cuộn");
    add("Chế độ Lật trang (Paged)", "Page Turn Mode", "Chế độ Lật trang");
    add("Cuộn liên tục (Scroll)", "Continuous Scroll", "Cuộn liên tục");
    add("Vuốt lật trang (Paged)", "Swipe Page Turn", "Vuốt lật trang");
    add("Đã chuyển sang chế độ Cuộn liên tục", "Switched to Continuous Scroll Mode", "Đã chuyển sang chế độ Cuộn liên tục");
    add("Đã chuyển sang chế độ Vuốt lật trang", "Switched to Page Turn Mode", "Đã chuyển sang chế độ Vuốt lật trang");
    add("Đảo màu trang", "Invert Page Colors", "Đảo màu trang");
    add("Đảo màu văn bản và nền tài liệu PDF", "Invert PDF text and page background colors", "Đảo màu văn bản và nền tài liệu PDF");
    add("Tự động chuyển sang chương tiếp theo khi đọc hết", "Auto-advance to next chapter when finished", "Tự động chuyển sang chương tiếp theo khi đọc hết");

    // --- Footer Settings ---
    add("MỤC HIỂN THỊ CHÂN TRANG", "FOOTER DISPLAY ITEMS", "MỤC HIỂN THỊ CHÂN TRANG");
    add("Hiển thị tiêu đề chương đang đọc ở chân trang", "Show current chapter title in footer", "Hiển thị tiêu đề chương đang đọc ở chân trang");
    add("Hiển thị số trang và phần trăm đọc sách", "Show page numbers and reading progress", "Hiển thị số trang và phần trăm đọc sách");
    add("Hiển thị thời gian hiện tại", "Show current time in footer", "Hiển thị thời gian hiện tại");
    add("Hiển thị phần trăm pin của thiết bị", "Show device battery percentage", "Hiển thị phần trăm pin của thiết bị");

    // --- Library Settings & Reading History ---
    add("LỊCH SỬ ĐỌC", "READING HISTORY", "LỊCH SỬ ĐỌC");
    add("Xóa Lịch Sử Đọc", "Clear Reading History", "Xóa Lịch Sử Đọc");
    add("Xóa lịch sử", "Clear History", "Xóa lịch sử");
    add("Xóa toàn bộ lịch sử đọc", "Clear All Reading History", "Xóa toàn bộ lịch sử đọc");
    add("Toàn bộ danh sách sách đã đọc gần đây và tiến độ đọc sẽ được xóa. Các đánh dấu (Bookmark) và ghi chú (Highlight) vẫn sẽ được giữ nguyên.\n\nBạn có chắc chắn muốn xóa không?",
        "All recent books and reading progress will be cleared. Bookmarks and highlights will be preserved.\n\nAre you sure you want to proceed?",
        "Toàn bộ danh sách sách đã đọc gần đây và tiến độ đọc sẽ được xóa. Các dấu trang và ghi chú vẫn sẽ được giữ nguyên.\n\nBạn có chắc chắn muốn xóa không?");
    add("Đã xóa sạch lịch sử đọc gần đây và đặt lại tiến độ.", "Cleared recent reading history and reset progress.", "Đã xóa sạch lịch sử đọc gần đây và đặt lại tiến độ.");
    add("Đã xóa toàn bộ lịch sử đọc.", "Cleared all reading history.", "Đã xóa toàn bộ lịch sử đọc.");

    // --- OPDS Catalogs & OPDS Book Details ---
    add("Kho Sách OPDS", "OPDS Book Catalogs", "Kho Sách OPDS");
    add("Kho sách OPDS", "OPDS Catalogs", "Kho sách OPDS");
    add("Chi Tiết Sách", "Book Details", "Chi Tiết Sách");
    add("Thêm Kho Sách OPDS", "Add OPDS Catalog", "Thêm Kho Sách OPDS");
    add("Chỉnh Sửa Kho", "Edit Catalog", "Chỉnh Sửa Kho");
    add("Chỉnh sửa", "Edit", "Chỉnh sửa");
    add("Xóa kho sách", "Delete Catalog", "Xóa kho sách");
    add("Làm mới", "Refresh", "Làm mới");
    add("Khôi phục mặc định", "Restore Defaults", "Khôi phục mặc định");
    add("Mở kho sách", "Open Catalog", "Mở kho sách");
    add("Trang chủ", "Home", "Trang chủ");
    add("Thêm kho", "Add Catalog", "Thêm kho");
    add("Thêm kho mới", "Add New Catalog", "Thêm kho mới");
    add("Thử lại", "Try Again", "Thử lại");
    add("Danh mục sách", "Book Catalog", "Danh mục sách");
    add("Tải sách về máy", "Download Book", "Tải sách về máy");
    add("Tải Sách Về Thiết Bị (", "Download Book to Device (", "Tải Sách Về Thiết Bị (");
    add("TÓM TẮT NỘI DUNG", "SUMMARY", "TÓM TẮT NỘI DUNG");
    add("Chưa có phần tóm tắt nội dung giới thiệu cho tác phẩm này.", "No summary or description available for this book.", "Chưa có phần tóm tắt nội dung giới thiệu cho tác phẩm này.");
    add("Định dạng: ", "Format: ", "Định dạng: ");
    add("Đọc ngay ›", "Read Now ›", "Đọc ngay ›");
    add("Xem tóm tắt & chi tiết", "View Summary & Details", "Xem tóm tắt & chi tiết");
    add("Tìm kiếm trong kho sách này...", "Search in this catalog...", "Tìm kiếm trong kho sách này...");
    add("Đã thêm kho sách thành công!", "Catalog added successfully!", "Đã thêm kho sách thành công!");
    add("Đã cập nhật kho sách!", "Catalog updated successfully!", "Đã cập nhật kho sách!");
    add("Đã tải xong: ", "Download complete: ", "Đã tải xong: ");
    add("Đã khôi phục các kho sách mặc định.", "Restored default catalogs.", "Đã khôi phục các kho sách mặc định.");
    add("Lỗi chuyển hướng quá nhiều lần (Redirect loop)!", "Too many redirects (Redirect loop)!", "Lỗi chuyển hướng quá nhiều lần (Redirect loop)!");
    add("Máy chủ bắt buộc dùng HTTPS với chuẩn TLS/mã hóa mới mà thiết bị BB10 chưa hỗ trợ.", "Server requires modern HTTPS TLS/ciphers unsupported by BlackBerry 10.", "Máy chủ bắt buộc dùng HTTPS với chuẩn TLS/mã hóa mới mà thiết bị BB10 chưa hỗ trợ.");
    add("Máy chủ từ chối kết nối (HTTP 403 / Bảo vệ chống Bot).", "Server denied access (HTTP 403 / Bot Protection).", "Máy chủ từ chối kết nối (HTTP 403 / Bảo vệ chống Bot).");
    add("Lỗi bắt tay SSL/TLS: Máy chủ yêu cầu phiên bản TLS hoặc thuật toán mã hóa (cipher) mới.", "SSL/TLS handshake error: Server requires a newer TLS version or cipher.", "Lỗi bắt tay SSL/TLS: Máy chủ yêu cầu phiên bản TLS hoặc thuật toán mã hóa (cipher) mới.");
    add("Lỗi chuyển hướng tải sách quá nhiều lần (Redirect loop)!", "Too many download redirects (Redirect loop)!", "Lỗi chuyển hướng tải sách quá nhiều lần (Redirect loop)!");
    add("Máy chủ chặn tải sách (HTTP 403 / Cloudflare Protection).", "Server blocked download (HTTP 403 / Cloudflare Protection).", "Máy chủ chặn tải sách (HTTP 403 / Cloudflare Protection).");
    add("Lỗi bắt tay SSL khi tải sách. Máy chủ yêu cầu chuẩn TLS mới hơn.", "SSL handshake error while downloading book. Server requires newer TLS.", "Lỗi bắt tay SSL khi tải sách. Máy chủ yêu cầu chuẩn TLS mới hơn.");

    // --- Add/Edit Catalog Dialog ---
    add("THÔNG TIN KHO SÁCH", "CATALOG INFORMATION", "THÔNG TIN KHO SÁCH");
    add("Tên hiển thị:", "Display Name:", "Tên hiển thị:");
    add("Ví dụ: Calibre Của Tôi, Kavita Server...", "e.g., My Calibre, Kavita Server...", "Ví dụ: Calibre Của Tôi, Kavita Server...");
    add("Địa chỉ OPDS URL (*):", "OPDS URL (*):", "Địa chỉ OPDS URL (*):");
    add("Mô tả / Ghi chú (tùy chọn):", "Description / Notes (optional):", "Mô tả / Ghi chú (tùy chọn):");
    add("Thư viện sách cá nhân trong gia đình", "Personal home book library", "Thư viện sách cá nhân trong gia đình");
    add("GỢI Ý MẪU KHO SÁCH (CHẠM ĐỂ ĐIỀN NHANH)", "SUGGESTED CATALOG TEMPLATES (TAP TO AUTO-FILL)", "GỢI Ý MẪU KHO SÁCH (CHẠM ĐỂ ĐIỀN NHANH)");
    add("Calibre Máy Chủ", "Calibre Server", "Calibre Máy Chủ");
    add("Máy chủ Calibre trong mạng Wi-Fi", "Calibre server on local Wi-Fi", "Máy chủ Calibre trong mạng Wi-Fi");
    add("Máy chủ đọc sách Kavita", "Kavita reading server", "Máy chủ đọc sách Kavita");
    add("Thư viện truyện và sách Komga", "Komga comics and book library", "Thư viện truyện và sách Komga");
    add("http://<IP-máy-tính>:8080/opds", "http://<PC-IP-Address>:8080/opds", "http://<IP-máy-tính>:8080/opds");
    add("XÁC THỰC TÀI KHOẢN (NẾU CÓ)", "ACCOUNT AUTHENTICATION (IF REQUIRED)", "XÁC THỰC TÀI KHOẢN (NẾU CÓ)");
    add("Nếu máy chủ yêu cầu đăng nhập (HTTP Basic Auth), hãy nhập thông tin bên dưới:", "If server requires login (HTTP Basic Auth), enter credentials below:", "Nếu máy chủ yêu cầu đăng nhập (HTTP Basic Auth), hãy nhập thông tin bên dưới:");
    add("Tên đăng nhập (Username):", "Username:", "Tên đăng nhập:");
    add("Mật khẩu (Password):", "Password:", "Mật khẩu:");
    add("Để trống nếu không yêu cầu mật khẩu", "Leave blank if no password required", "Để trống nếu không yêu cầu mật khẩu");
    add("Mật khẩu truy cập máy chủ", "Server access password", "Mật khẩu truy cập máy chủ");
    add("HƯỚNG DẪN KẾT NỐI CALIBRE NHANH", "QUICK CALIBRE CONNECTION GUIDE", "HƯỚNG DẪN KẾT NỐI CALIBRE NHANH");
    add("Các bước kết nối với máy tính:", "Steps to connect to your PC:", "Các bước kết nối với máy tính:");
    add("1. Mở Calibre trên máy tính -> Chọn 'Kết nối/chia sẻ' -> 'Khởi động máy chủ nội dung'.\n",
        "1. Open Calibre on PC -> Click 'Connect/share' -> 'Start content server'.\n",
        "1. Mở Calibre trên máy tính -> Chọn 'Kết nối/chia sẻ' -> 'Khởi động máy chủ nội dung'.\n");
    add("2. Đảm bảo thiết bị BlackBerry và máy tính đang cùng kết nối một mạng Wi-Fi.\n",
        "2. Ensure BlackBerry device and PC are connected to the same Wi-Fi.\n",
        "2. Đảm bảo thiết bị BlackBerry và máy tính đang cùng kết nối một mạng Wi-Fi.\n");
    add("3. Xem địa chỉ IP của máy tính (ví dụ: 192.168.1.15) và nhập dạng: http://192.168.1.15:8080/opds vào ô ở trên.\n",
        "3. Find PC IP address (e.g. 192.168.1.15) and enter: http://192.168.1.15:8080/opds above.\n",
        "3. Xem địa chỉ IP của máy tính (ví dụ: 192.168.1.15) và nhập dạng: http://192.168.1.15:8080/opds vào ô ở trên.\n");
    add("4. Nhấn 'Lưu kho sách' để bắt đầu duyệt toàn bộ thư viện sách!",
        "4. Tap 'Save Catalog' to start browsing your full book collection!",
        "4. Nhấn 'Lưu kho sách' để bắt đầu duyệt toàn bộ thư viện sách!");
    add("Áp dụng ›", "Apply ›", "Áp dụng ›");
    add("Hủy bỏ", "Cancel", "Hủy bỏ");
    add("Lưu kho sách", "Save Catalog", "Lưu kho sách");

    // --- Browser & Search & Empty States ---
    add("Tìm & Lọc", "Search & Filter", "Tìm & Lọc");
    add("Tìm theo tên file, tên sách, tác giả...", "Search by filename, title, author...", "Tìm theo tên file, tên sách, tác giả...");
    add("Xóa bộ lọc & tìm kiếm", "Clear Filter & Search", "Xóa bộ lọc & tìm kiếm");
    add("Không tìm thấy tệp hoặc thư mục phù hợp", "No matching files or folders found", "Không tìm thấy tệp hoặc thư mục phù hợp");
    add("Chạm vào màn hình để bỏ qua", "Tap anywhere to dismiss", "Chạm vào màn hình để bỏ qua");
    add("Hoàn thành", "Finished", "Hoàn thành");
    add("Tác phẩm", "Book", "Tác phẩm");
    add("Đang nạp và quét thư viện sách...", "Loading and scanning book library...", "Đang nạp và quét thư viện sách...");

    // --- Version & Changelog (Build 67-69) ---
    add("Phiên bản 2.0.0 (Build 200) • BlackBerry 10", "Version 2.0.0 (Build 200) • BlackBerry 10", "Phiên bản 2.0.0 (Build 200) • BlackBerry 10");
    add("• Bản dựng Build 67 - 69 (Mới nhất):\n  - Nhận Diện Tên Chương Mục Lục: Tự động trích xuất tên chương thực tế từ nội dung sách EPUB, TXT, FB2 thay vì chỉ hiển thị Chương 1, Chương 2...\n  - Tinh Chỉnh Số Thứ Tự & Số Trang: Bỏ ký tự '#' ở huy hiệu số thứ tự/số trang, hiển thị số thuần gọn gàng.\n  - Tăng Cỡ Chữ Mục Lục: Tăng kích thước phông chữ các dòng mục lục giúp đọc dễ dàng và rõ ràng hơn trên màn hình BlackBerry.\n  - Tinh Chỉnh Tra Cứu & Dịch Thuật: Rút gọn nhãn 'From:', bỏ icon thừa trong hộp thoại tìm kiếm từ điển.\n  - Icon Thư Viện Đơn Sắc: Chuyển icon Thư viện sang màu đen trắng (monochrome) đồng bộ với toàn bộ thanh Tab và Action Bar chuẩn BlackBerry 10.\n  - Khối Đang Đọc Tinh Gọn: Loại bỏ viền xanh và icon thừa, mang lại diện mạo hiện đại, sạch sẽ cho thẻ sách đang đọc.\n\n• Bản dựng Build 65 - 66:\n  - Đồng Bộ Ngôn Ngữ Toàn Diện: Chuẩn hóa 100% tiếng Việt & tiếng Anh cho Giới Thiệu (About), Hướng Dẫn (Help), Cài Đặt (Settings) và các menu.\n  - Đọc Giọng Nói (TTS) Hoàn Thiện: Tối ưu đọc TTS trên trang sách, hỗ trợ tạm dừng bằng phím Space và tự động cuộn trang theo giọng đọc.\n  - Tối Ưu Hóa Giao Diện: Rút gọn mô tả các mục menu và cài đặt, giúp thao tác nhanh trên màn hình nhỏ.\n\n• Bản dựng Build 50 - 64:\n  - Bảo Toàn Dữ Liệu Sau Cài Lại: Tự động lưu và khôi phục lịch sử đọc, tiến độ và ghi chú vào bộ nhớ ngoài (/accounts/1000/shared/documents/Libris).\n  - Từ Điển Ngoại Tuyến (Offline Dictionary): Tra cứu từ điển SQLite tức thì khi bôi đen từ trong sách mà không cần kết nối mạng.\n  - Bảng Thống Kê Đọc Sách (Reading Stats): Theo dõi thời gian đọc, tốc độ đọc và tiến độ hoàn thành sách chi tiết.\n  - Bộ Phím Tắt Toàn Diện: Tối ưu hóa tối đa cho bàn phím vật lý BlackBerry Passport, Classic Q20 và Q10.\n\n• Bản dựng Build 30 - 49 (Phiên bản 2.0.0):\n  - Navigation Drawer 5 Tab Native: Giao diện chuẩn 5 Tab Native với ngăn kéo trượt trái: Đang đọc, Gần đây, Mục lục, Tủ sách, Duyệt tệp.\n  - Nâng Cấp Lõi Libris: Tối ưu hiệu năng kết xuất PDF và DJVU, hỗ trợ bộ nhớ đệm trang thông minh.\n  - Tự Động Nhận Diện Bảng Mã Tiếng Việt: Đọc mượt mà mọi tệp TXT (UTF-8, UTF-16, TCVN3, Windows-1258, VNI) không lỗi phông.\n  - Trang Thông Tin Toàn Diện: Màn hình thông tin chuẩn BB10 với 3 tab: Giới thiệu, Hướng dẫn phím tắt và Nhật ký phiên bản.\n\n• Phiên bản 1.0.0 (Bản dựng ban đầu):\n  - Phát hành nền tảng đọc sách Cascades thuần C++ / Qt 4.8.\n  - Hỗ trợ định dạng EPUB 2/3 với trích xuất ảnh bìa và mục lục.\n  - Hỗ trợ Active Frame hiển thị tiến độ đọc ngoài màn hình chủ.",
        "• Build 67 - 69 (Latest):\n  - Smart Chapter Name Detection: Automatically extracts actual chapter titles from EPUB, TXT, and FB2 content instead of generic 'Chapter 1, Chapter 2...'\n  - Clean Badges: Removed '#' prefix from index/page count badges for a clean look.\n  - Larger TOC Font Size: Increased font size in Table of Contents for effortless readability on compact BlackBerry screens.\n  - Streamlined Translation & Dictionary: Condensed 'From:' labels and removed redundant icons in dictionary search dialog.\n  - Monochrome Library Icon: Clean black-and-white icon matching the native BlackBerry 10 design.\n  - Minimalist Reading Card: Removed borders and badges for a sleek currently-reading card.\n\n• Build 65 - 66:\n  - Complete Bilingual Sync: 100% unified English and Vietnamese for About, Help, Settings, and Action menus.\n  - Polished Voice Reading (TTS): Refined text-to-speech reading on book pages, with Spacebar pause/play and auto-page turning.\n  - Streamlined UI Descriptions: Condensed menu labels and settings descriptions for quick glancing on compact BB10 screens.\n\n• Build 50 - 64:\n  - Persistent Data Storage: Automatically saves and restores reading history and progress in shared documents across reinstalls.\n  - Offline SQLite Dictionary: Instant word definition and translation popup when selecting text, no internet required.\n  - Reading Statistics Sheet: Detailed tracking of reading sessions, reading speed, and completion estimates.\n  - Comprehensive Keyboard Shortcuts: Full physical keyboard navigation for BlackBerry Passport, Classic Q20, and Q10.\n\n• Build 30 - 49 (Version 2.0.0):\n  - 5-Tab Navigation Drawer: Native slide-out menu with Reading, Recent, Contents, Bookshelf, and File Browser.\n  - Optimized Libris Engine: High-performance rendering for PDF and DJVU with intelligent memory page caching.\n  - Automatic Vietnamese Charset Detection: Seamlessly reads TXT files in UTF-8, UTF-16, TCVN3, Windows-1258, and VNI.\n  - Full Info Screen: Standard BB10 page layout featuring About, Keyboard Guides, and Version Changelog.\n\n• Version 1.0.0 (Initial Release):\n  - Native BlackBerry Cascades C++ / Qt 4.8 e-book reader launch.\n  - EPUB 2/3 parser with cover image extraction and NCX TOC navigation.\n  - Active Frame cover displaying reading progress on the homescreen.",
        "• Bản dựng Build 67 - 69 (Mới nhất):\n  - Nhận Diện Tên Chương Mục Lục: Tự động trích xuất tên chương thực tế từ nội dung sách EPUB, TXT, FB2 thay vì chỉ hiển thị Chương 1, Chương 2...\n  - Tinh Chỉnh Số Thứ Tự & Số Trang: Bỏ ký tự '#' ở huy hiệu số thứ tự/số trang, hiển thị số thuần gọn gàng.\n  - Tăng Cỡ Chữ Mục Lục: Tăng kích thước phông chữ các dòng mục lục giúp đọc dễ dàng và rõ ràng hơn trên màn hình BlackBerry.\n  - Tinh Chỉnh Tra Cứu & Dịch Thuật: Rút gọn nhãn 'From:', bỏ icon thừa trong hộp thoại tìm kiếm từ điển.\n  - Icon Thư Viện Đơn Sắc: Chuyển icon Thư viện sang màu đen trắng (monochrome) đồng bộ với toàn bộ thanh Tab và Action Bar chuẩn BlackBerry 10.\n  - Khối Đang Đọc Tinh Gọn: Loại bỏ viền xanh và icon thừa, mang lại diện mạo hiện đại, sạch sẽ cho thẻ sách đang đọc.\n\n• Bản dựng Build 65 - 66:\n  - Đồng Bộ Ngôn Ngữ Toàn Diện: Chuẩn hóa 100% tiếng Việt & tiếng Anh cho Giới Thiệu (About), Hướng Dẫn (Help), Cài Đặt (Settings) và các menu.\n  - Đọc Giọng Nói (TTS) Hoàn Thiện: Tối ưu đọc TTS trên trang sách, hỗ trợ tạm dừng bằng phím Space và tự động cuộn trang theo giọng đọc.\n  - Tối Ưu Hóa Giao Diện: Rút gọn mô tả các mục menu và cài đặt, giúp thao tác nhanh trên màn hình nhỏ.\n\n• Bản dựng Build 50 - 64:\n  - Bảo Toàn Dữ Liệu Sau Cài Lại: Tự động lưu và khôi phục lịch sử đọc, tiến độ và ghi chú vào bộ nhớ ngoài (/accounts/1000/shared/documents/Libris).\n  - Từ Điển Ngoại Tuyến (Offline Dictionary): Tra cứu từ điển SQLite tức thì khi bôi đen từ trong sách mà không cần kết nối mạng.\n  - Bảng Thống Kê Đọc Sách (Reading Stats): Theo dõi thời gian đọc, tốc độ đọc và tiến độ hoàn thành sách chi tiết.\n  - Bộ Phím Tắt Toàn Diện: Tối ưu hóa tối đa cho bàn phím vật lý BlackBerry Passport, Classic Q20 và Q10.\n\n• Bản dựng Build 30 - 49 (Phiên bản 2.0.0):\n  - Navigation Drawer 5 Tab Native: Giao diện chuẩn 5 Tab Native với ngăn kéo trượt trái: Đang đọc, Gần đây, Mục lục, Tủ sách, Duyệt tệp.\n  - Nâng Cấp Lõi Libris: Tối ưu hiệu năng kết xuất PDF và DJVU, hỗ trợ bộ nhớ đệm trang thông minh.\n  - Tự Động Nhận Diện Bảng Mã Tiếng Việt: Đọc mượt mà mọi tệp TXT (UTF-8, UTF-16, TCVN3, Windows-1258, VNI) không lỗi phông.\n  - Trang Thông Tin Toàn Diện: Màn hình thông tin chuẩn BB10 với 3 tab: Giới thiệu, Hướng dẫn phím tắt và Nhật ký phiên bản.\n\n• Phiên bản 1.0.0 (Bản dựng ban đầu):\n  - Phát hành nền tảng đọc sách Cascades thuần C++ / Qt 4.8.\n  - Hỗ trợ định dạng EPUB 2/3 với trích xuất ảnh bìa và mục lục.\n  - Hỗ trợ Active Frame hiển thị tiến độ đọc ngoài màn hình chủ.");
    // --- OPDS C++ Backend & Download Statuses ---
    add("DANH MỤC CÔNG CỘNG", "PUBLIC CATALOGS", "DANH MỤC CÔNG CỘNG");
    add("KHO SÁCH CÁ NHÂN (CALIBRE, KAVITA...)", "PERSONAL CATALOGS (CALIBRE, KAVITA...)", "KHO SÁCH CÁ NHÂN (CALIBRE, KAVITA...)");
    add("Calibre Máy Chủ (Mẫu)", "Calibre Server (Sample)", "Calibre Máy Chủ (Mẫu)");
    add("Ví dụ kết nối Calibre trong mạng Wi-Fi (nhấn giữ để sửa IP)", "Local Wi-Fi Calibre sample (press & hold to edit IP)", "Ví dụ kết nối Calibre trong mạng Wi-Fi (nhấn giữ để sửa IP)");
    add("Hơn 70.000 tác phẩm sách miễn phí toàn cầu", "Over 70,000 free books worldwide", "Hơn 70.000 tác phẩm sách miễn phí toàn cầu");
    add("Sách kinh điển công cộng định dạng EPUB chuẩn đẹp", "Beautifully formatted public domain classic books", "Sách kinh điển công cộng định dạng EPUB chuẩn đẹp");
    add("Kho sách miễn phí phổ biến nhiều thể loại", "Popular free books across many genres", "Kho sách miễn phí phổ biến nhiều thể loại");
    add("Kho sách mới", "New Catalogs", "Kho sách mới");
    add("Kho sách người dùng thêm", "Custom user catalogs", "Kho sách người dùng thêm");
    add("Danh mục con", "Subcategory", "Danh mục con");
    add("Trang tiếp theo ›", "Next Page ›", "Trang tiếp theo ›");
    add("Nhấn để xem thêm các tác phẩm tiếp theo", "Tap to view more books", "Nhấn để xem thêm các tác phẩm tiếp theo");
    add("Đang kết nối tới máy chủ...", "Connecting to server...", "Đang kết nối tới máy chủ...");
    add("Đang tải", "Downloading", "Đang tải");
    add("Không thể tạo file lưu sách trên thiết bị!", "Cannot create file to save book on device!", "Không thể tạo file lưu sách trên thiết bị!");
    add("Không thể đọc cấu trúc danh mục OPDS XML!", "Cannot parse OPDS XML catalog structure!", "Không thể đọc cấu trúc danh mục OPDS XML!");
    add("Không tìm thấy tác phẩm hoặc mục nào.", "No books or items found.", "Không tìm thấy tác phẩm hoặc mục nào.");
    add("Liên kết tải sách không hợp lệ!", "Invalid book download link!", "Liên kết tải sách không hợp lệ!");
    add("Lỗi kết nối: %1", "Connection error: %1", "Lỗi kết nối: %1");
    add("Tải sách thất bại: %1", "Download failed: %1", "Tải sách thất bại: %1");
    add("Phản hồi không phải là nguồn OPDS Atom hợp lệ.", "Response is not a valid OPDS Atom feed.", "Phản hồi không phải là nguồn OPDS Atom hợp lệ.");
    add("Sách chưa đặt tên", "Untitled Book", "Sách chưa đặt tên");
    add("Đang có tác phẩm khác đang tải, vui lòng đợi hoàn tất!", "Another download is in progress, please wait!", "Đang có tác phẩm khác đang tải, vui lòng đợi hoàn tất!");
    add("Đã hủy tải sách", "Download cancelled", "Đã hủy tải sách");
    add("Trang %1", "Page %1", "Trang %1");
    add("Tìm: \"%1\"", "Search: \"%1\"", "Tìm: \"%1\"");
    add("%1 mục", "%1 items", "%1 mục");
    add("Bộ lọc danh mục", "Catalog filter", "Bộ lọc danh mục");
    add("Nhóm danh mục", "Catalog group", "Nhóm danh mục");
    add("Không thể đọc cấu trúc danh mục OPDS 2.0 JSON!", "Cannot parse OPDS 2.0 JSON catalog structure!", "Không thể đọc cấu trúc danh mục OPDS 2.0 JSON!");
    add("Không tìm thấy đường dẫn danh mục OPDS (HTTP 404).", "OPDS catalog URL not found (HTTP 404).", "Không tìm thấy đường dẫn danh mục OPDS (HTTP 404).");
    add("Máy chủ chặn tải sách (HTTP 403 / Bảo vệ chống Bot).", "Server blocked book download (HTTP 403 / Bot protection).", "Máy chủ chặn tải sách (HTTP 403 / Bảo vệ chống Bot).");
    add("Máy chủ gặp lỗi nội bộ (HTTP %1).", "Server internal error (HTTP %1).", "Máy chủ gặp lỗi nội bộ (HTTP %1).");
    add("Máy chủ phản hồi chậm hoặc cổng kết nối gặp sự cố (HTTP %1).", "Server timed out or gateway error (HTTP %1).", "Máy chủ phản hồi chậm hoặc cổng kết nối gặp sự cố (HTTP %1).");
    add("Máy chủ trả về dữ liệu trống.", "Server returned an empty response.", "Máy chủ trả về dữ liệu trống.");
    add("Máy chủ trả về trang web HTML thay vì danh mục OPDS (%1).", "Server returned an HTML web page instead of an OPDS feed (%1).", "Máy chủ trả về trang web HTML thay vì danh mục OPDS (%1).");
    add("Máy chủ trả về trang web HTML thay vì danh mục OPDS.", "Server returned an HTML web page instead of an OPDS feed.", "Máy chủ trả về trang web HTML thay vì danh mục OPDS.");
    add("Máy chủ tải sách đang bảo trì hoặc quá tải (HTTP 503 Service Unavailable). Vui lòng thử lại sau.",
        "Download server is down for maintenance or overloaded (HTTP 503 Service Unavailable). Please try again later.",
        "Máy chủ tải sách đang bảo trì hoặc quá tải (HTTP 503 Service Unavailable). Vui lòng thử lại sau.");
    add("Máy chủ yêu cầu đăng nhập (HTTP 401). Hãy kiểm tra tên người dùng và mật khẩu.",
        "Server requires authentication (HTTP 401). Check your username and password.",
        "Máy chủ yêu cầu đăng nhập (HTTP 401). Hãy kiểm tra tên người dùng và mật khẩu.");
    add("Máy chủ đang bảo trì hoặc tạm ngừng phục vụ (HTTP 503 Service Unavailable). Vui lòng thử lại sau.",
        "Server is down for maintenance or temporarily unavailable (HTTP 503 Service Unavailable). Please try again later.",
        "Máy chủ đang bảo trì hoặc tạm ngừng phục vụ (HTTP 503 Service Unavailable). Vui lòng thử lại sau.");
    add("Quá nhiều yêu cầu tới máy chủ (HTTP 429). Vui lòng đợi một lát rồi thử lại.",
        "Too many requests to server (HTTP 429). Please wait a moment and try again.",
        "Quá nhiều yêu cầu tới máy chủ (HTTP 429). Vui lòng đợi một lát rồi thử lại.");

    // --- TTS Backend Status & Dialogs ---
    add("Google Translate (Tự động)", "Google Translate (Auto)", "Google Translate (Tự động)");
    add("Libris xin chào! Kết nối API thành công.", "Libris greeting! API connection successful.", "Libris xin chào! Kết nối API thành công.");
    add("Chế độ Ngoại tuyến: Câu này chưa có trong bộ nhớ máy. Hãy bấm nút 'Tải về' để lưu trước toàn bộ chương.",
        "Offline Mode: This sentence is not cached yet. Tap 'Download' to pre-cache the whole chapter.",
        "Chế độ Ngoại tuyến: Câu này chưa có trong bộ nhớ máy. Hãy bấm nút 'Tải về' để lưu trước toàn bộ chương.");
    add("Dữ liệu âm thanh Base64 bị cắt ngắn.", "Base64 audio data truncated.", "Dữ liệu âm thanh Base64 bị cắt ngắn.");
    add("Dữ liệu âm thanh Base64 không đúng định dạng.", "Base64 audio format invalid.", "Dữ liệu âm thanh Base64 không đúng định dạng.");
    add("Dữ liệu âm thanh nhận từ Gemini quá ngắn.", "Audio data from Gemini is too short.", "Dữ liệu âm thanh nhận từ Gemini quá ngắn.");
    add("Dữ liệu âm thanh nhận về không hợp lệ. Hãy kiểm tra kết nối mạng hoặc API Key.",
        "Received audio data is invalid. Please check network connection or API Key.",
        "Dữ liệu âm thanh nhận về không hợp lệ. Hãy kiểm tra kết nối mạng hoặc API Key.");
    add("Không thể tải file âm thanh từ máy chủ: ", "Cannot download audio file from server: ", "Không thể tải file âm thanh từ máy chủ: ");
    add("Lỗi Gemini API (%1): %2", "Gemini API Error (%1): %2", "Lỗi Gemini API (%1): %2");
    add("Lỗi Gemini: ", "Gemini Error: ", "Lỗi Gemini: ");
    add("Lỗi kết nối khi tải giọng đọc. Hãy kiểm tra kết nối mạng hoặc máy chủ TTS.",
        "Connection error while fetching voice. Check network connection or TTS server.",
        "Lỗi kết nối khi tải giọng đọc. Hãy kiểm tra kết nối mạng hoặc máy chủ TTS.");
    add("Lỗi phản hồi từ Gemini API.", "Error response from Gemini API.", "Lỗi phản hồi từ Gemini API.");
    add("Phản hồi từ Gemini không chứa dữ liệu âm thanh.", "Gemini response contains no audio data.", "Phản hồi từ Gemini không chứa dữ liệu âm thanh.");
    add("Vui lòng nhập Gemini API Key trong Cài đặt Giọng đọc (lấy miễn phí tại aistudio.google.com).",
        "Please enter your Gemini API Key in Voice Settings (get free at aistudio.google.com).",
        "Vui lòng nhập Gemini API Key trong Cài đặt Giọng đọc (lấy miễn phí tại aistudio.google.com).");
    add("Vui lòng nhập Google AI Studio API Key trước khi kiểm tra.",
        "Please enter Google AI Studio API Key before testing.",
        "Vui lòng nhập Google AI Studio API Key trước khi kiểm tra.");
    add("Xin chào, đây là giọng đọc sách trực tuyến của Libris.",
        "Hello, this is Libris online voice reading.",
        "Xin chào, đây là giọng đọc sách trực tuyến của Libris.");
    add("Đang kết nối và kiểm tra API Key với Google AI Studio...",
        "Connecting and verifying API Key with Google AI Studio...",
        "Đang kết nối và kiểm tra API Key với Google AI Studio...");
    add("Đã hủy kiểm tra.", "Test cancelled.", "Đã hủy kiểm tra.");
    add("Đã vượt quá giới hạn lượt gọi (Rate Limit) của Gemini API. Vui lòng chờ vài giây rồi thử lại.",
        "Gemini API Rate Limit exceeded. Please wait a few seconds and try again.",
        "Đã vượt quá giới hạn lượt gọi (Rate Limit) của Gemini API. Vui lòng chờ vài giây rồi thử lại.");
    add("✓ Kết nối thành công (200 OK - %1 ms)! API Key hợp lệ (Mô hình '%2' phản hồi dạng văn bản).",
        "✓ Connected successfully (200 OK - %1 ms)! Valid API Key ('%2' text response).",
        "✓ Kết nối thành công (200 OK - %1 ms)! API Key hợp lệ (Mô hình '%2' phản hồi dạng văn bản).");
    add("✓ Kết nối thành công (200 OK - %1 ms)! API Key hợp lệ và đang phát thử giọng đọc.",
        "✓ Connected successfully (200 OK - %1 ms)! Valid API Key, playing sample audio.",
        "✓ Kết nối thành công (200 OK - %1 ms)! API Key hợp lệ và đang phát thử giọng đọc.");
    add("✗ Lỗi 400 (Bad Request): API Key không hợp lệ hoặc model không hỗ trợ.\nChi tiết: %1",
        "✗ Error 400 (Bad Request): Invalid API Key or unsupported model.\nDetails: %1",
        "✗ Lỗi 400 (Bad Request): API Key không hợp lệ hoặc model không hỗ trợ.\nChi tiết: %1");
    add("✗ Lỗi 403 (Forbidden): API Key bị từ chối truy cập hoặc chưa được cấp quyền.\nChi tiết: %1",
        "✗ Error 403 (Forbidden): API Key rejected or lacks permissions.\nDetails: %1",
        "✗ Lỗi 403 (Forbidden): API Key bị từ chối truy cập hoặc chưa được cấp quyền.\nChi tiết: %1");
    add("✗ Lỗi 404 (Not Found): Mô hình '%1' không tồn tại trên API v1beta.\nChi tiết: %2",
        "✗ Error 404 (Not Found): Model '%1' does not exist on v1beta API.\nDetails: %2",
        "✗ Lỗi 404 (Not Found): Mô hình '%1' không tồn tại trên API v1beta.\nChi tiết: %2");
    add("✗ Lỗi 429 (Rate Limit): Đã vượt quá hạn mức gọi (Quota / Rate limit). Vui lòng chờ 1 lát rồi thử lại.",
        "✗ Error 429 (Rate Limit): Quota / rate limit exceeded. Please wait a moment and retry.",
        "✗ Lỗi 429 (Rate Limit): Đã vượt quá hạn mức gọi (Quota / Rate limit). Vui lòng chờ 1 lát rồi thử lại.");
    add("✗ Lỗi HTTP %1: %2", "✗ HTTP Error %1: %2", "✗ Lỗi HTTP %1: %2");
    add("✗ Lỗi kết nối: %1 (Vui lòng kiểm tra kết nối mạng Wi-Fi / 3G)",
        "✗ Connection error: %1 (Please check Wi-Fi / 3G connection)",
        "✗ Lỗi kết nối: %1 (Vui lòng kiểm tra kết nối mạng Wi-Fi / 3G)");
    add("✗ Phản hồi không có âm thanh: %1", "✗ Response has no audio: %1", "✗ Phản hồi không có âm thanh: %1");

    // --- Dictionary Parts of Speech & Meaning ---
    add("Danh từ (noun)", "Noun", "Danh từ");
    add("Động từ (verb)", "Verb", "Động từ");
    add("Tính từ (adjective)", "Adjective", "Tính từ");
    add("Phó từ (adverb)", "Adverb", "Phó từ");
    add("Giới từ (preposition)", "Preposition", "Giới từ");
    add("Liên từ (conjunction)", "Conjunction", "Liên từ");
    add("Đại từ (pronoun)", "Pronoun", "Đại từ");
    add("Thán từ (interjection)", "Interjection", "Thán từ");
    add("★ Nghĩa: ", "★ Meaning: ", "★ Nghĩa: ");

    // --- Book Workers & App Startup ---
    add("Trang %1 (Trang hiện tại)", "Page %1 (Current page)", "Trang %1 (Trang hiện tại)");
    add("Phần %1", "Part %1", "Phần %1");
    add("Tài liệu Text", "Text Document", "Tài liệu Text");
    add("Tài liệu FB2", "FB2 Document", "Tài liệu FB2");
    add("Lời giới thiệu", "Introduction", "Lời giới thiệu");
    add("Văn bản thường", "Plain Text", "Văn bản thường");
    add("Libris - Khởi Động Thất Bại", "Libris - Startup Failed", "Libris - Khởi Động Thất Bại");
    add("Không thể tải giao diện chính asset:///main.qml:\n\n", "Cannot load main UI asset:///main.qml:\n\n", "Không thể tải giao diện chính asset:///main.qml:\n\n");

    // --- Book Stats and Details ---
    add("%1 cuốn sách", "%1 books", "%1 cuốn sách");
    add("Chưa mở", "Not opened", "Chưa mở");
    add("Đã đọc xong cuốn sách này", "Book completed", "Đã đọc xong cuốn sách này");
    add("Còn khoảng %1", "~%1 remaining", "Còn khoảng %1");
    add("Cần đọc thêm để ước tính", "Read more to estimate", "Cần đọc thêm để ước tính");
    add("Đang tính...", "Calculating...", "Đang tính...");
    add("giây / trang", "sec / page", "giây / trang");
    add("phút / trang", "min / page", "phút / trang");
    add("trang / giờ", "pages / hr", "trang / giờ");
    add("tác giả không rõ", "unknown author", "tác giả không rõ");
    add("Không rõ tác giả", "Unknown author", "Không rõ tác giả");
    add("Đang hiển thị", "Showing", "Đang hiển thị");
    add("PIN: ", "BAT: ", "PIN: ");
    add("Đã quét xong! Tổng cộng: %1 cuốn sách.", "Scan complete! Total: %1 books found.", "Đã quét xong! Tổng cộng: %1 cuốn sách.");
    add("Đã cập nhật kho sách!", "Catalog updated successfully!", "Đã cập nhật kho sách!");
    add("Đã thêm kho sách thành công!", "Catalog added successfully!", "Đã thêm kho sách thành công!");

    // Sleep Timer
    add("HẸN GIỜ TẮT", "SLEEP TIMER", "HẸN GIỜ TẮT");
    add("Tự động dừng sau", "Auto-stop after", "Tự động dừng sau");
    add("Tắt (Không hẹn giờ)", "Off (No timer)", "Tắt (Không hẹn giờ)");
    add("15 phút", "15 minutes", "15 phút");
    add("30 phút", "30 minutes", "30 phút");
    add("45 phút", "45 minutes", "45 phút");
    add("60 phút", "60 minutes", "60 phút");
    add("Khi đọc xong chương này", "At the end of this chapter", "Khi đọc xong chương này");
    add("Trạng thái: Sẽ tự động dừng khi đọc hết chương.", "Status: Will stop at the end of chapter.", "Trạng thái: Sẽ tự động dừng khi đọc hết chương.");
    add("Thời gian còn lại: ", "Remaining time: ", "Thời gian còn lại: ");
    add("Hẹn giờ", "Timer", "Hẹn giờ");
    add("Hẹn giờ tắt: ", "Sleep timer: ", "Hẹn giờ tắt: ");
    add("Đã tắt hẹn giờ", "Sleep timer cancelled", "Đã tắt hẹn giờ");
    add("Hẹn giờ: Dừng khi hết chương", "Timer: Stop at end of chapter", "Hẹn giờ: Dừng khi hết chương");

    // Column Layout
    add("Bố cục cột", "Column layout", "Bố cục cột");
    add("Tự động (2 cột trên Passport)", "Auto (2 columns on Passport)", "Tự động (2 cột trên Passport)");
    add("1 cột (Toàn màn hình)", "1 column (Full screen)", "1 cột (Toàn màn hình)");
    add("2 cột (Trang sách đôi)", "2 columns (Book spread)", "2 cột (Trang sách đôi)");

    // Comics & Manga RTL
    add("TRUYỆN TRANH & MANGA", "COMICS & MANGA", "TRUYỆN TRANH & MANGA");
    add("Chế độ Manga (Phải qua Trái)", "Manga Mode (Right-to-Left)", "Chế độ Manga (Phải qua Trái)");
    add("Chạm bên trái để lật trang tiếp theo, vuốt phải sang trái để đọc", "Tap left edge to turn next, swipe right-to-left to read", "Chạm bên trái để lật trang tiếp theo, vuốt phải sang trái để đọc");
    add("Manga RTL (Đang bật)", "Manga RTL (On)", "Manga RTL (Đang bật)");
    add("Manga RTL (Đang tắt)", "Manga RTL (Off)", "Manga RTL (Đang tắt)");
    add("Đã bật chế độ Manga (Phải qua Trái)", "Manga Mode enabled (Right-to-Left)", "Đã bật chế độ Manga (Phải qua Trái)");
    add("Đã tắt chế độ Manga (Trái qua Phải)", "Manga Mode disabled (Left-to-Right)", "Đã tắt chế độ Manga (Trái qua Phải)");

    // Footnotes & Markdown Export
    add("Chú thích", "Footnote", "Chú thích");
    add("Xuất .MD", "Export .MD", "Xuất .MD");
    add("Đã lưu vào Documents: ", "Saved to Documents: ", "Đã lưu vào Documents: ");
    add("Không có ghi chú nào để xuất.", "No notes or bookmarks to export.", "Không có ghi chú nào để xuất.");

    // WiFi Drop, Reader, Highlights, About, & Misc Missing Strings
    add("Truyền sách Wi-Fi", "Wi-Fi Book Drop", "Wi-Fi Drop");
    add("Máy chủ WiFi Book Drop", "WiFi Book Drop Server", "Máy chủ WiFi Book Drop");
    add("● MÁY CHỦ ĐANG HOẠT ĐỘNG", "● SERVER ACTIVE", "● MÁY CHỦ ĐANG HOẠT ĐỘNG");
    add("○ MÁY CHỦ ĐÃ TẠM DỪNG", "○ SERVER STOPPED", "○ MÁY CHỦ ĐÃ TẠM DỪNG");
    add("Bật công tắc phía trên để kích hoạt máy chủ nhận sách không dây.", "Turn on the switch above to start the wireless book upload server.", "Bật công tắc phía trên để kích hoạt máy chủ nhận sách không dây.");
    add("Mở trình duyệt trên máy tính hoặc điện thoại trong cùng mạng Wi-Fi và truy cập địa chỉ trên để truyền sách không dây.", "Open a browser on your PC or phone on the same Wi-Fi network and visit the address above to transfer books wirelessly.", "Mở trình duyệt trên máy tính hoặc điện thoại trong cùng mạng Wi-Fi và truy cập địa chỉ trên để truyền sách không dây.");
    add("Chưa phát hiện địa chỉ Wi-Fi. Vui lòng kết nối vào mạng Wi-Fi.", "No Wi-Fi IP detected. Please connect to a Wi-Fi network.", "Chưa phát hiện địa chỉ Wi-Fi. Vui lòng kết nối vào mạng Wi-Fi.");
    add("Thư mục lưu: /accounts/1000/shared/books (Tự động cập nhật thư viện)", "Save folder: /accounts/1000/shared/books (Auto-updates library)", "Thư mục lưu: /accounts/1000/shared/books (Tự động cập nhật thư viện)");
    add("SÁCH ĐÃ NHẬN TRONG PHIÊN", "BOOKS RECEIVED IN SESSION", "SÁCH ĐÃ NHẬN TRONG PHIÊN");
    add("Chưa có sách nào được truyền", "No books transferred yet", "Chưa có sách nào được truyền");
    add("tệp", "files", "tệp");
    add(" phút", " min", " phút");
    add("0 phút", "0 min", "0 phút");
    add(" • Yêu thích", " • Favorites", " • Yêu thích");
    add("Tài liệu đã đọc", "Document Read", "Tài liệu đã đọc");
    add("Hết chương", "End of Chapter", "Hết chương");
    add("Tắt", "Off", "Tắt");
    add("Dừng", "Stop", "Dừng");
    add("Đổi", "Swap", "Đổi");
    add("Trước", "Prev", "Trước");
    add("Tiếp", "Next", "Tiếp");
    add("Sách yêu thích", "Favorite Books", "Sách yêu thích");
    add("Danh sách đọc", "Reading List", "Danh sách đọc");
    add("Sách hiện tại", "Current Book", "Sách hiện tại");
    add("Sách này", "This Book", "Sách này");
    add("Phạm vi: Sách hiện tại", "Scope: Current Book", "Phạm vi: Sách hiện tại");
    add("Phạm vi: Toàn bộ thư viện", "Scope: Entire Library", "Phạm vi: Toàn bộ thư viện");
    add("Chưa có dấu trang hoặc ghi chú", "No bookmarks or notes yet", "Chưa có dấu trang hoặc ghi chú");
    add("Chưa có ghi chú nào", "No notes yet", "Chưa có ghi chú nào");
    add("Bôi đen văn bản khi đọc sách để lưu trích dẫn và ghi chú.", "Highlight text while reading to save quotes and notes.", "Bôi đen văn bản khi đọc sách để lưu trích dẫn và ghi chú.");
    add("Chạm phím 'H' hoặc nút đánh dấu trong trình đọc để lưu trang.", "Press 'H' or tap the bookmark button in the reader to save a page.", "Chạm phím 'H' hoặc nút đánh dấu trong trình đọc để lưu trang.");
    add("Đánh dấu trang hoặc lưu trích dẫn khi đọc sách để xem lại tại đây.", "Bookmark pages or highlight passages while reading to review them here.", "Đánh dấu trang hoặc lưu trích dẫn khi đọc sách để xem lại tại đây.");
    add("Hãy bấm vào biểu tượng ngôi sao để thêm sách vào mục Yêu Thích.", "Tap the star icon to add a book to Favorites.", "Hãy bấm vào biểu tượng ngôi sao để thêm sách vào mục Yêu Thích.");
    add("Tủ sách đang trống", "Bookshelf is Empty", "Tủ sách đang trống");
    add("Tủ sách của bạn đang trống. Hãy mở tệp hoặc duyệt thư mục để bắt đầu đọc sách!", "Your bookshelf is empty. Open a file or browse folders to start reading!", "Tủ sách của bạn đang trống. Hãy mở tệp hoặc duyệt thư mục để bắt đầu đọc sách!");
    add("KẾT QUẢ TÌM KIẾM (%1)", "SEARCH RESULTS (%1)", "KẾT QUẢ TÌM KIẾM (%1)");
    add("Thử tìm kiếm với từ khóa khác hoặc xóa bộ lọc.", "Try another keyword or clear your filters.", "Thử tìm kiếm với từ khóa khác hoặc xóa bộ lọc.");
    add("Chuyển đến trang ", "Go to page ", "Chuyển đến trang ");
    add("Cỡ chữ: ", "Font size: ", "Cỡ chữ: ");
    add("Chế độ Vuốt trang (Swipe)", "Swipe Page Mode", "Chế độ Vuốt trang");
    add("Giao diện: Sáng", "Theme: Light", "Giao diện: Sáng");
    add("Giao diện: Tối (AMOLED)", "Theme: Dark (AMOLED)", "Giao diện: Tối (AMOLED)");
    add("Giao diện: Vàng dịu (Sepia)", "Theme: Sepia", "Giao diện: Vàng dịu (Sepia)");
    add("Đang thử lại kết nối HTTP...", "Retrying over HTTP...", "Đang thử lại kết nối HTTP...");

    // About & Help
    add("Phiên bản 2.0.0 (Build 200) - BlackBerry 10", "Version 2.0.0 (Build 200) - BlackBerry 10", "Phiên bản 2.0.0 (Build 200) - BlackBerry 10");
    add("Trình đọc sách Native thuần C++ và Cascades cho BlackBerry 10.", "Pure C++ and Cascades Native eBook Reader for BlackBerry 10.", "Trình đọc sách Native thuần C++ và Cascades cho BlackBerry 10.");
    add("Libris là ứng dụng đọc sách chuẩn Native Cascades dành cho BlackBerry 10. Hỗ trợ nhiều định dạng, cử chỉ vuốt mượt mà, tra từ điển ngoại tuyến và đọc giọng nói TTS.", "Libris is a Native Cascades eBook reader for BlackBerry 10 supporting multiple formats, smooth swipe gestures, offline dictionary, and TTS voice reading.", "Libris là ứng dụng đọc sách Native Cascades dành cho BlackBerry 10, hỗ trợ nhiều định dạng, vuốt mượt mà, từ điển Offline và đọc giọng nói TTS.");
    add("Hỗ trợ EPUB, PDF, MOBI, AZW3, PRC, FB2, TXT, CBZ, DJVU.", "Supports EPUB, PDF, MOBI, AZW3, PRC, FB2, TXT, CBZ, DJVU.", "Hỗ trợ EPUB, PDF, MOBI, AZW3, PRC, FB2, TXT, CBZ, DJVU.");
    add("CÔNG NGHỆ VÀ BỘ XỬ LÝ LÕI", "CORE TECHNOLOGIES & ENGINES", "CÔNG NGHỆ VÀ BỘ XỬ LÝ LÕI");
    add("BẢN QUYỀN VÀ GIẤY PHÉP", "COPYRIGHT & LICENSE", "BẢN QUYỀN VÀ GIẤY PHÉP");
    add("CỬ CHỈ ĐIỀU HƯỚNG NATIVE", "NATIVE NAVIGATION GESTURES", "CỬ CHỈ ĐIỀU HƯỚNG NATIVE");
    add("PHÍM TẮT BÀN PHÍM VẬT LÝ", "PHYSICAL KEYBOARD SHORTCUTS", "PHÍM TẮT BÀN PHÍM VẬT LÝ");
    add("NHẬT KÝ PHIÊN BẢN", "RELEASE CHANGELOG", "NHẬT KÝ PHIÊN BẢN");
    add("Lõi tài liệu: Poppler C++, DjVuLibre, pugixml DOM và miniz.\nBảng mã Tiếng Việt: Nhận diện tự động UTF-8, UTF-16, TCVN3, Windows-1258, VNI.\nĐọc văn bản TTS: BlackBerry Multimedia Services, Google TTS và Gemini AI.\nTừ điển ngoại tuyến: Cơ sở dữ liệu SQLite tích hợp trên thiết bị.",
        "Document Core: Poppler C++, DjVuLibre, pugixml DOM, and miniz.\nVietnamese Encodings: Auto-detects UTF-8, UTF-16, TCVN3, Windows-1258, VNI.\nVoice Reading (TTS): BlackBerry Multimedia Services, Google TTS, and Gemini AI.\nOffline Dictionary: Embedded on-device SQLite database.",
        "Lõi tài liệu: Poppler C++, DjVuLibre, pugixml DOM và miniz.\nBảng mã Tiếng Việt: Nhận diện tự động UTF-8, UTF-16, TCVN3, Windows-1258, VNI.\nĐọc văn bản TTS: BlackBerry Multimedia Services, Google TTS và Gemini AI.\nTừ điển ngoại tuyến: Cơ sở dữ liệu SQLite tích hợp trên thiết bị.");
    add("EPUB (.epub): Chuẩn EPUB 2.0 và 3.0 với phông chữ nhúng và mục lục NCX.\nMOBI / AZW3 (.mobi, .azw, .azw3, .kf8, .prc): Sách điện tử Mobipocket / Kindle KF8 với mục lục, CSS và ảnh nhúng.\nCBZ (.cbz): Truyện tranh nén với lật trang mượt mà và trích xuất ComicInfo.\nPDF (.pdf): Trang đồ họa nguyên bản và chế độ cuộn văn bản Reflow.\nDJVU (.djvu, .djv): Tài liệu scan nén chất lượng cao.\nFB2 (.fb2): Định dạng FictionBook kèm chú thích chân trang.\nTXT (.txt): Văn bản thuần tự động phát hiện mã hóa.",
        "EPUB (.epub): EPUB 2.0 & 3.0 standard with embedded fonts and NCX Table of Contents.\nMOBI / AZW3 (.mobi, .azw, .azw3, .kf8, .prc): Mobipocket / Kindle KF8 eBooks with TOC, CSS, and embedded images.\nCBZ (.cbz): Compressed comic books with smooth page flipping and ComicInfo metadata.\nPDF (.pdf): Native graphic pages and Reflow text reading mode.\nDJVU (.djvu, .djv): High-compression scanned documents.\nFB2 (.fb2): FictionBook XML with footnotes.\nTXT (.txt): Plain text with automatic encoding detection.",
        "EPUB (.epub): Chuẩn EPUB 2.0 và 3.0 với phông chữ nhúng và mục lục NCX.\nMOBI / AZW3 (.mobi, .azw, .azw3, .kf8, .prc): Sách điện tử Mobipocket / Kindle KF8 với mục lục, CSS và ảnh nhúng.\nCBZ (.cbz): Truyện tranh nén với lật trang mượt mà và trích xuất ComicInfo.\nPDF (.pdf): Trang đồ họa nguyên bản và chế độ cuộn văn bản Reflow.\nDJVU (.djvu, .djv): Tài liệu scan nén chất lượng cao.\nFB2 (.fb2): Định dạng FictionBook kèm chú thích chân trang.\nTXT (.txt): Văn bản thuần tự động phát hiện mã hóa.");
    add("Tác giả: NhatLee (Libris Developer Team)\nBộ dựng Vector PDF & DjVu: Libris PDF/DjVu Engine\nGiấy phép: GNU General Public License v2 (GPLv2)\nMôi trường: BlackBerry 10 Native SDK 10.3.1",
        "Author: NhatLee (Libris Developer Team)\nPDF & DjVu Vector Renderer: Libris PDF/DjVu Engine\nLicense: GNU General Public License v2 (GPLv2)\nEnvironment: BlackBerry 10 Native SDK 10.3.1",
        "Tác giả: NhatLee (Libris Developer Team)\nBộ dựng Vector PDF & DjVu: Libris PDF/DjVu Engine\nGiấy phép: GNU General Public License v2 (GPLv2)\nMôi trường: BlackBerry 10 Native SDK 10.3.1");
    add("Vuốt mép viền trái: Mở thanh điều hướng Sidebar trượt trái.\nVuốt mép viền trên: Mở Menu ứng dụng BlackBerry 10.\nChạm giữa màn hình: Bật hoặc tắt thanh tiêu đề và thanh công cụ.\nVuốt mép viền phải: Mở danh sách tác vụ mở rộng.",
        "Swipe from left edge: Open the sliding Sidebar drawer.\nSwipe from top edge: Open BlackBerry 10 Application Menu.\nTap center of screen: Toggle title bar and action toolbar.\nSwipe from right edge: Open extended overflow action menu.",
        "Vuốt mép viền trái: Mở thanh điều hướng Sidebar trượt trái.\nVuốt mép viền trên: Mở Menu ứng dụng BlackBerry 10.\nChạm giữa màn hình: Bật hoặc tắt thanh tiêu đề và thanh công cụ.\nVuốt mép viền phải: Mở danh sách tác vụ mở rộng.");
    add("Space: Sang trang kế tiếp / Tạm dừng đọc TTS\nN: Sang trang kế tiếp\nP / U: Về trang trước đó\nJ / K: Cuộn trang xuống / lên\nZ / W: Phóng to / Thu nhỏ cỡ chữ\nG / M: Mở mục lục sách\nT / B: Nhảy về đầu sách / cuối sách\nV: Đổi màu nền Ngày / Đêm\nF: Tìm kiếm trong sách\nA: Bật hoặc tắt đọc giọng nói TTS\nD: Tra từ điển ngoại tuyến\nH: Đánh dấu trang hiện tại\nO: Mở tệp sách mới\nS: Mở bảng cài đặt\nQ: Thoát hoàn toàn ứng dụng",
        "Space: Next page / Pause or Resume TTS\nN: Next page\nP / U: Previous page\nJ / K: Scroll down / up\nZ / W: Increase / Decrease font size\nG / M: Open Table of Contents\nT / B: Jump to start / end of book\nV: Switch Day / Night background\nF: Search inside book\nA: Toggle Text-to-Speech reading\nD: Offline dictionary lookup\nH: Bookmark current page\nO: Open a new book file\nS: Open settings panel\nQ: Quit application completely",
        "Space: Sang trang kế tiếp / Tạm dừng đọc TTS\nN: Sang trang kế tiếp\nP / U: Về trang trước đó\nJ / K: Cuộn trang xuống / lên\nZ / W: Phóng to / Thu nhỏ cỡ chữ\nG / M: Mở mục lục sách\nT / B: Nhảy về đầu sách / cuối sách\nV: Đổi màu nền Ngày / Đêm\nF: Tìm kiếm trong sách\nA: Bật hoặc tắt đọc giọng nói TTS\nD: Tra từ điển ngoại tuyến\nH: Đánh dấu trang hiện tại\nO: Mở tệp sách mới\nS: Mở bảng cài đặt\nQ: Thoát hoàn toàn ứng dụng");
    add("BlackBerry Passport: Vuốt trượt trên bàn phím cảm ứng để lật trang hoặc cuộn dòng.\nBlackBerry Classic: Sử dụng bàn rê quang học Trackpad để điều hướng và cuộn trang.",
        "BlackBerry Passport: Swipe across the touch-enabled keyboard to turn pages or scroll.\nBlackBerry Classic: Use the optical Trackpad to navigate and scroll pages.",
        "BlackBerry Passport: Vuốt trượt trên bàn phím cảm ứng để lật trang hoặc cuộn dòng.\nBlackBerry Classic: Sử dụng bàn rê quang học Trackpad để điều hướng và cuộn trang.");
    add("Phiên bản 2.0.0 (Build 200):\n- Bảng màu đơn sắc Đen, Trắng và điểm nhấn Emerald theo chuẩn BlackBerry 10.\n- Loại bỏ toàn bộ thẻ xanh đậm và ký tự rườm rà.\n- Nhận diện tên chương thực tế từ nội dung sách EPUB, TXT, FB2.\n- Tối ưu tra từ điển ngoại tuyến và đọc giọng nói TTS.\n- Điều hướng mượt mà với bàn phím và bàn rê quang học Passport, Classic, Q10.\n\nPhiên bản 2.0.0 (Bản dựng trước):\n- Cấu trúc Navigation Drawer 5 tab Native: Đang đọc, Gần đây, Mục lục, Tủ sách, Duyệt tệp.\n- Tự động nhận diện bảng mã tiếng Việt TCVN3, Windows-1258, VNI, UTF-8.\n- Đồng bộ dữ liệu lịch sử đọc và cấu hình vào bộ nhớ lưu trữ an toàn.",
        "Version 2.0.0 (Build 200):\n- Monochrome Black & White palette with Emerald accents following BlackBerry 10 design.\n- Removed cluttered labels and redundant badges.\n- Real chapter title extraction from EPUB, TXT, and FB2 content.\n- Optimized offline dictionary and TTS voice reading.\n- Smooth keyboard & optical trackpad navigation for Passport, Classic, and Q10.\n\nVersion 2.0.0 (Previous Builds):\n- Native 5-tab Navigation Drawer: Reading, Recent, TOC, Library, Files.\n- Automatic Vietnamese charset detection (TCVN3, Windows-1258, VNI, UTF-8).\n- Persistent sync of reading history and settings to safe storage.",
        "Phiên bản 2.0.0 (Build 200):\n- Bảng màu đơn sắc Đen, Trắng và điểm nhấn Emerald theo chuẩn BlackBerry 10.\n- Loại bỏ toàn bộ thẻ xanh đậm và ký tự rườm rà.\n- Nhận diện tên chương thực tế từ nội dung sách EPUB, TXT, FB2.\n- Tối ưu tra từ điển ngoại tuyến và đọc giọng nói TTS.\n- Điều hướng mượt mà với bàn phím và bàn rê quang học Passport, Classic, Q10.\n\nPhiên bản 2.0.0 (Bản dựng trước):\n- Cấu trúc Navigation Drawer 5 tab Native: Đang đọc, Gần đây, Mục lục, Tủ sách, Duyệt tệp.\n- Tự động nhận diện bảng mã tiếng Việt TCVN3, Windows-1258, VNI, UTF-8.\n- Đồng bộ dữ liệu lịch sử đọc và cấu hình vào bộ nhớ lưu trữ an toàn.");
    add("Không thể mở tệp PDF/DjVu.", "Unable to open PDF/DjVu file.", "Không thể mở tệp PDF/DjVu.");
    add("Không thể mở hoặc giải mã tệp PDF/DjVu (mã lỗi %1).", "Unable to open or decode PDF/DjVu file (error %1).", "Không thể mở tệp PDF/DjVu (mã lỗi %1).");
    add("Lỗi dịch thuật:", "Translation error:", "Lỗi dịch:");
    add("Phiên bản %1 (Build %2) - BlackBerry 10", "Version %1 (Build %2) - BlackBerry 10", "Phiên bản %1 (Build %2) - BlackBerry 10");
    add("Phiên bản hiện tại: %1 (Build %2)", "Current version: %1 (Build %2)", "Phiên bản hiện tại: %1 (Build %2)");
    add("TỔNG 7 NGÀY", "7-DAY TOTAL", "TỔNG 7 NGÀY");
    add("Chưa đọc sách", "No reading", "Chưa đọc sách");

    // Libris Editorial & 5-Tab Drawer Strings
    add("Đang đọc", "Reading", "Đang đọc");
    add("Ghi chú", "Notes", "Ghi chú");
    add("Nhập sách", "Import", "Nhập sách");
    add("Nhập Sách", "Import Books", "Nhập Sách");
    add("NHẬP SÁCH VÀO LIBRIS", "IMPORT BOOKS TO LIBRIS", "NHẬP SÁCH VÀO LIBRIS");
    add("Hiển thị", "Display", "Hiển thị");
    add("Cắt lề PDF", "Auto-Crop PDF", "Cắt lề PDF");
    add("Đảo màu Đêm", "Night Invert", "Đảo màu Đêm");
    add("Xoay 90°", "Rotate 90°", "Xoay 90°");
    add("Mục tiêu hôm nay", "Today's Goal", "Mục tiêu hôm nay");
    add("ĐANG ĐỌC", "CURRENTLY READING", "ĐANG ĐỌC");
    add("Xuất Markdown", "Export Markdown", "Xuất Markdown");
    add("Tất cả file", "All Files", "Tất cả file");
    add("DS đọc", "Reading List", "DS đọc");
    add("Tối (AMOLED)", "AMOLED Black", "Tối (AMOLED)");
    add("Cài đặt thêm...", "More Settings...", "Cài đặt thêm...");
    add("Gọn", "Tight", "Gọn");
    add("Vừa", "Normal", "Vừa");
    add("Thoáng", "Loose", "Thoáng");
    add("1 cột", "1 Col", "1 cột");
    add("2 cột", "2 Cols", "2 cột");
    add("Lật trang", "Paged", "Lật trang");
    add("MÀU NỀN ĐỌC", "READING THEME", "MÀU NỀN ĐỌC");
    add("FONT CHỮ", "FONT FAMILY", "FONT CHỮ");
    add("CỠ CHỮ", "FONT SIZE", "CỠ CHỮ");
    add("SỐ CỘT", "COLUMNS", "SỐ CỘT");
    add("DÃN DÒNG", "LINE SPACING", "GIÃN DÒNG");
    add("Đọc sách", "Read book", "Đọc sách");
    add("Cập nhật", "Updated", "Cập nhật");
    add("Sách khác gần đây", "Other Recent Books", "Sách khác gần đây");
    add("Mục tiêu", "Goal", "Mục tiêu");
    add("BẬT", "ON", "BẬT");
    add("TẮT", "OFF", "TẮT");
    add("Đã sẵn sàng", "Ready", "Đã sẵn sàng");
    add("Bộ lọc & Sắp xếp", "Filter & Sort", "Bộ lọc & Sắp xếp");
    add("Không", "None", "Không");
    add("Tác giả", "Author", "Tác giả");
    add("Trong máy", "Internal", "Trong máy");
    add("Thẻ SD", "SD Card", "Thẻ SD");
    add("Đặt lại tất cả", "Reset All", "Đặt lại tất cả");
    add("Lọc phần / chương...", "Filter parts / chapters...", "Lọc phần / chương...");
    add("TRẢI NGHIỆM ĐỌC", "READING EXPERIENCE", "TRẢI NGHIỆM ĐỌC");
    add("TÍNH NĂNG & KHO SÁCH", "FEATURES & LIBRARY", "TÍNH NĂNG & KHO SÁCH");
    add("ỨNG DỤNG", "APPLICATION", "ỨNG DỤNG");
    add("Đã bật cắt lề trắng PDF", "PDF Auto-Crop enabled", "Đã bật cắt lề trắng PDF");
    add("Đã tắt cắt lề trắng PDF", "PDF Auto-Crop disabled", "Đã tắt cắt lề trắng PDF");
    add("Đã xuất ghi chú: ", "Notes exported: ", "Đã xuất ghi chú: ");
    add("Không có ghi chú để xuất", "No notes to export", "Không có ghi chú để xuất");
    add("Truyền Wi-Fi", "Wi-Fi Drop", "Wi-Fi Drop");
    add("Kho OPDS", "OPDS", "Kho OPDS");
    add("Libris TTS", "Libris TTS", "Libris TTS");
    add("Phiên bản Libris & hướng dẫn", "Libris version & guide", "Phiên bản Libris & hướng dẫn");
    add("Chạm để bật máy chủ", "Tap to start server", "Chạm để bật máy chủ");
    add("Tải sách từ thư viện công cộng & Calibre", "Download from public & Calibre OPDS feeds", "Tải sách từ thư viện công cộng & Calibre");
    add("Đồng bộ & Quét thư viện", "Sync & Scan Library", "Đồng bộ & Quét thư viện");
    add("Duyệt thư mục", "Browse Folders", "Duyệt thư mục");
    add("Đặt lại", "Reset", "Đặt lại");

    // Phase 2 Libris Features
    add("Kệ sách", "Shelves", "Kệ sách");
    add("Quản lý Kệ sách", "Manage Shelves", "Quản lý Kệ sách");
    add("Thêm vào kệ sách", "Add to Shelf", "Thêm vào kệ sách");
    add("Thêm vào kệ...", "Add to shelf...", "Thêm vào kệ...");
    add("Tạo kệ mới", "Create Shelf", "Tạo kệ mới");
    add("Tên kệ sách mới...", "New shelf name...", "Tên kệ sách mới...");
    add("Tạo", "Create", "Tạo");
    add("Chưa có kệ sách tùy chỉnh nào", "No custom shelves yet", "Chưa có kệ sách tùy chỉnh nào");
    add("Đã tạo kệ sách: ", "Created shelf: ", "Đã tạo kệ sách: ");
    add("Đã xóa kệ sách: ", "Deleted shelf: ", "Đã xóa kệ sách: ");
    add("Xóa kệ", "Delete Shelf", "Xóa kệ");
    add("Phông chữ tùy chỉnh (.ttf / .otf)", "Custom Fonts (.ttf / .otf)", "Phông chữ tùy chỉnh (.ttf / .otf)");
    add("Quét lại phông chữ", "Rescan Fonts", "Quét lại phông chữ");
    add("Đã tìm thấy %1 phông chữ tùy chỉnh", "Found %1 custom font(s)", "Đã tìm thấy %1 phông chữ tùy chỉnh");
    add("Thư mục phông chữ:", "Fonts folder:", "Thư mục phông chữ:");
    add("BẢO VỆ MẮT & TỰ ĐỘNG", "EYE CARE & AUTOMATION", "BẢO VỆ MẮT & TỰ ĐỘNG");
    add("Lọc dịu mắt (Ánh sáng ấm)", "Warm Amber Night Filter", "Lọc dịu mắt (Ánh sáng ấm)");
    add("Cường độ lọc ấm", "Warm Filter Intensity", "Cường độ lọc ấm");
    add("Tự động chế độ Đêm (20:00 - 06:00)", "Auto Night Theme (20:00 - 06:00)", "Tự động chế độ Đêm (20:00 - 06:00)");
    add("Lọc ấm", "Warm Filter", "Lọc ấm");
    add("MỤC TIÊU & PHÍM CỨNG", "DAILY GOAL & HARDWARE KEYS", "MỤC TIÊU & PHÍM CỨNG");
    add("Mục tiêu đọc hằng ngày", "Daily Reading Goal", "Mục tiêu đọc hằng ngày");
    add("Lật trang bằng phím âm lượng", "Volume Key Page Turn", "Lật trang bằng phím âm lượng");
    add("Nhấn Giảm âm lượng để sang trang tiếp, Tăng âm lượng để lùi trang (tự nhả khi bật TTS)",
        "Press Volume Down for next page, Volume Up for previous page (auto-releases during TTS)",
        "Nhấn Giảm âm lượng để sang trang tiếp, Tăng âm lượng để lùi trang (tự nhả khi bật TTS)");
    add("TRÍCH DẪN HÔM NAY", "TODAY'S HIGHLIGHT", "TRÍCH DẪN HÔM NAY");
    add("Xuất ảnh Quote Card", "Export Quote Card PNG", "Xuất ảnh Quote Card");
    add("Đã lưu ảnh Quote Card: ", "Saved Quote Card: ", "Đã lưu ảnh Quote Card: ");
    add("Không thể xuất ảnh Quote Card", "Unable to export Quote Card", "Không thể xuất ảnh Quote Card");
    add("Tự cuộn", "Auto-Scroll", "Tự cuộn");
    add("Tự cuộn (Teleprompter)", "Auto-Scroll", "Tự cuộn");
    add("Kho máy", "On Device", "Kho máy");
    add("Kệ", "Shelf", "Kệ");
    add("CHUỖI MAX", "MAX STREAK", "CHUỖI DÀI NHẤT");
    add("MỤC TIÊU HÔM NAY", "TODAY'S GOAL", "MỤC TIÊU HÔM NAY");
    add("Mục tiêu: ", "Goal: ", "Mục tiêu: ");
    add("NGÀY ĐỌC", "READ DAYS", "NGÀY ĐỌC");
    add("TB / NGÀY", "AVG / DAY", "TB / NGÀY");
    add("TỔNG THÁNG", "MONTH TOTAL", "TỔNG THÁNG");
    add("liên tục", "streak", "liên tục");
    add("ngày có đọc", "active days", "ngày có đọc");
    add("phút/ngày", "min/day", "phút/ngày");
    add("tháng", "month", "tháng");
    add("thời gian đọc", "reading time", "thời gian đọc");
    add("cuốn khác", "other books", "cuốn khác");
    add("Tự chuyển nền đọc Tối AMOLED vào buổi tối và Sepia ban ngày",
        "Auto-switch to AMOLED Dark at night and Sepia in daytime",
        "Tự chuyển nền đọc Tối AMOLED vào buổi tối và Sepia ban ngày");
    add("Áp lớp lọc ánh sáng ấm dịu mắt khi đọc ban đêm",
        "Apply a warm eye-comfort tint for night reading",
        "Áp lớp lọc ánh sáng ấm dịu mắt khi đọc ban đêm");
    add("1. Mở Calibre trên máy tính -> Chọn 'Kết nối/chia sẻ' -> 'Khởi động máy chủ nội dung'.",
        "1. Open Calibre on PC -> 'Connect/share' -> 'Start Content server'.",
        "1. Mở Calibre trên máy tính -> Chọn 'Kết nối/chia sẻ' -> 'Khởi động máy chủ nội dung'.");
    add("2. Đảm bảo thiết bị BlackBerry và máy tính đang cùng kết nối một mạng Wi-Fi.",
        "2. Ensure your BlackBerry and PC are on the same Wi-Fi network.",
        "2. Đảm bảo thiết bị BlackBerry và máy tính đang cùng kết nối một mạng Wi-Fi.");
    add("3. Xem địa chỉ IP của máy tính (ví dụ: 192.168.1.15) và nhập dạng: http://192.168.1.15:8080/opds vào ô ở trên.",
        "3. Find your PC IP (e.g. 192.168.1.15) and enter: http://192.168.1.15:8080/opds above.",
        "3. Xem địa chỉ IP của máy tính (ví dụ: 192.168.1.15) và nhập dạng: http://192.168.1.15:8080/opds vào ô ở trên.");
    add("Dừng", "Stop", "Dừng");

    // --- Focus Mode, Next Series, Quote Share, Flashcards, OPDS Queue & .librisbackup ---
    add("Chế độ Tập trung (Thước đọc)", "Focus Mode (Reading Ruler)", "Focus Mode (Reading Ruler)");
    add("Làm mờ vùng chữ xung quanh, chạm để dời dải đọc", "Dim surrounding text, tap to move reading band", "Làm mờ chữ xung quanh, chạm để di chuyển vùng đọc");
    add("Chế độ Tập trung", "Focus Mode", "Focus Mode");
    add("Chế độ Tập trung (Đang bật)", "Focus Mode (On)", "Focus Mode (Đang bật)");
    add("Đã bật Chế độ Tập trung", "Focus Mode enabled", "Đã bật Focus Mode");
    add("Đã tắt Chế độ Tập trung", "Focus Mode disabled", "Đã tắt Focus Mode");
    add("Đọc tiếp tập kế", "Read Next Volume", "Đọc tiếp tập kế");
    add("Chia sẻ Quote Card", "Share Quote Card", "Chia sẻ Quote Card");
    add("Lưu từ", "Save Word", "Lưu từ");
    add("Lưu vào Flashcard", "Save to Flashcard", "Lưu vào Flashcard");
    add("Đã lưu vào sổ từ vựng!", "Saved to vocabulary book!", "Đã lưu vào sổ từ vựng!");
    add("Flashcard", "Flashcard", "Flashcard");
    add("Tổng số thẻ:", "Total cards:", "Tổng số thẻ:");
    add("Xuất CSV (Anki)", "Export CSV (Anki)", "Xuất CSV (Anki)");
    add("Đã xuất CSV: ", "Exported CSV: ", "Đã xuất CSV: ");
    add("Không thể xuất CSV", "Unable to export CSV", "Không thể xuất CSV");
    add("Chưa có từ vựng nào.\nHãy tra từ hoặc dịch rồi bấm 'Lưu từ' để ôn tập Flashcard.",
        "No vocabulary cards yet.\nLook up or translate a word and tap 'Save Word' to practice with Flashcards.",
        "Chưa có từ vựng nào.\nHãy tra từ hoặc dịch rồi bấm 'Lưu từ' để ôn tập Flashcard.");
    add("Chạm thẻ để lật", "Tap card to flip", "Chạm thẻ để lật");
    add("Đã thuộc ✓", "Mastered ✓", "Đã thuộc ✓");
    add("Đánh dấu chưa thuộc", "Mark as Learning", "Đánh dấu chưa thuộc");
    add("Nguồn:", "Source:", "Nguồn:");
    add("‹ Trước", "‹ Prev", "‹ Trước");
    add("Lật thẻ", "Flip Card", "Lật thẻ");
    add("Tiếp ›", "Next ›", "Tiếp ›");
    add("chờ", "queued", "chờ");
    add("Đã thêm vào hàng đợi", "Added to download queue", "Đã thêm vào hàng đợi");
    add("Xuất .librisbackup", "Export .librisbackup", "Xuất .librisbackup");
    add("Nhập .librisbackup", "Import .librisbackup", "Nhập .librisbackup");
    add("Đã tạo gói sao lưu: ", "Backup created: ", "Đã tạo gói sao lưu: ");
    add("Không thể tạo gói sao lưu", "Unable to create backup archive", "Không thể tạo gói sao lưu");
    add("Chọn tệp sao lưu .librisbackup", "Select .librisbackup file", "Chọn tệp sao lưu .librisbackup");
    add("Đã khôi phục dữ liệu từ gói sao lưu!", "Restored data from backup archive!", "Đã khôi phục dữ liệu từ gói sao lưu!");
    add("Gói sao lưu không hợp lệ hoặc bị lỗi", "Invalid or corrupted backup archive", "Gói sao lưu không hợp lệ hoặc bị lỗi");

    // --- UI Overhaul Phase 2/3 Strings ---
    add("NGUỒN NHẬP SÁCH", "IMPORT SOURCES", "NGUỒN NHẬP SÁCH");
    add("THƯ VIỆN MÁY", "DEVICE LIBRARY", "THƯ VIỆN MÁY");
    add("Chọn tệp từ bộ nhớ máy hoặc thẻ SD", "Choose files from device storage or SD card", "Chọn tệp từ bộ nhớ máy hoặc thẻ SD");
    add("Không tìm thấy chương phù hợp", "No matching chapters found", "Không tìm thấy chương phù hợp");
    add("Không tìm thấy kết quả phù hợp", "No matching results found", "Không tìm thấy kết quả phù hợp");
    add("Toàn bộ thư viện", "Entire library", "Toàn bộ thư viện");
    add("dấu trang", "bookmarks", "dấu trang");
    add("ghi chú", "notes", "ghi chú");
    add("Xóa Dấu Trang", "Delete Bookmark", "Xóa Dấu Trang");
    add("Xác nhận xóa", "Confirm Delete", "Xác nhận xóa");
    add("Bạn có chắc muốn xóa dấu trang này?", "Are you sure you want to delete this bookmark?", "Bạn có chắc muốn xóa dấu trang này?");
    add("Bạn có chắc muốn xóa highlight này?", "Are you sure you want to delete this highlight?", "Bạn có chắc muốn xóa highlight này?");
    add("XEM TRƯỚC CHÂN TRANG", "FOOTER PREVIEW", "XEM TRƯỚC CHÂN TRANG");
    add("Chương 3: Đêm tĩnh lặng", "Chapter 3: The Quiet Night", "Chương 3: Đêm tĩnh lặng");
    add("Tất cả mục chân trang đang ẩn", "All footer items are hidden", "Tất cả mục chân trang đang ẩn");

    // --- Reading Statistics Redesign ---
    add("7 NGÀY GẦN NHẤT", "LAST 7 DAYS", "7 NGÀY GẦN NHẤT");
    add("TÍCH LŨY", "ALL-TIME", "TỔNG CỘNG");
    add("THEO ĐỊNH DẠNG", "BY FORMAT", "THEO ĐỊNH DẠNG");
    add("TỔNG TUẦN", "WEEK TOTAL", "TỔNG TUẦN");
    add("TIẾN ĐỘ CHƯƠNG", "CHAPTER PROGRESS", "TIẾN ĐỘ CHƯƠNG");
    add("Thư viện còn:", "Also in library:", "Thư viện còn:");
    add("Chưa có dữ liệu đọc sách", "No reading data yet", "Chưa có dữ liệu đọc sách");

    // --- eSpeak-NG ---
    add("eSpeak-NG (Ngoại tuyến)", "eSpeak-NG (Offline)", "eSpeak-NG (Offline)");
    add("Giọng vùng miền (Tiếng Việt)", "Vietnamese regional accent", "Giọng vùng miền (Tiếng Việt)");
    add("Miền Bắc", "Northern", "Miền Bắc");
    add("Miền Nam", "Southern", "Miền Nam");
    add("Miền Trung", "Central", "Miền Trung");
    add("Chất giọng eSpeak-NG", "eSpeak-NG voice style", "Chất giọng eSpeak-NG");
    add("Nữ êm dịu", "Female - Smooth", "Nữ êm dịu");
    add("Nữ sáng", "Female - Bright", "Nữ sáng");
    add("Nam chuẩn", "Male - Standard", "Nam chuẩn");
    add("Nam trầm ấm", "Male - Warm", "Nam trầm ấm");
    add("Klatt rõ chữ", "Klatt - Clear", "Klatt rõ chữ");
    add("Giọng Nguyễn", "Nguyen voice", "Giọng Nguyễn");
    add("Chế độ ngoại tuyến: ưu tiên phát âm thanh đã tải, câu chưa có sẽ tự đọc bằng eSpeak-NG tích hợp sẵn.", "Offline mode: downloaded audio plays first; uncached sentences are read by the built-in eSpeak-NG.", "Chế độ Offline: ưu tiên phát âm thanh đã tải, câu chưa có sẽ đọc bằng eSpeak-NG tích hợp sẵn.");
    // --- TTS quick HUD ---
    add("Tạm dừng", "Pause", "Tạm dừng");
    add("Phát tiếp", "Play", "Phát tiếp");
    add("Nguồn", "Source", "Nguồn");
    add("Giọng", "Voice", "Giọng");
    add("Chuẩn (mượt nhất)", "Standard (smoothest)", "Chuẩn (mượt nhất)");
    add("Chế độ ngoại tuyến: đọc âm thanh đã tải hoặc eSpeak-NG", "Offline mode: cached audio or eSpeak-NG", "Chế độ Offline: dùng âm thanh đã tải hoặc eSpeak-NG");
    add("Chế độ trực tuyến", "Online mode", "Chế độ Online");

    // --- Onboarding Guide ---
    add("Skip", "Skip", "Bỏ qua");
    add("Chào mừng bạn", "Welcome", "Chào mừng bạn");
    add("Trình đọc sách bản địa cho BlackBerry 10", "Native E-Book Reader for BlackBerry 10", "Ứng dụng đọc sách cho BlackBerry 10");
    add("Chọn ngôn ngữ", "Choose Language", "Chọn ngôn ngữ");
    add("Chọn ngôn ngữ hiển thị", "Select your preferred language", "Chọn ngôn ngữ hiển thị");
    add("Chọn giao diện", "Choose Theme", "Chọn giao diện");
    add("Thêm sách vào tủ", "Add Books", "Thêm sách vào tủ");
    add("Bộ nhớ & Thẻ SD", "Storage & SD Card", "Bộ nhớ & Thẻ SD");
    add("/shared/books & MicroSD", "/shared/books & MicroSD", "/shared/books & Thẻ MicroSD");
    add("Kéo thả sách & bài web qua Wi-Fi", "Drop books & web articles over Wi-Fi", "Gửi sách & bài web qua Wi-Fi");
    add("Tải sách trực tuyến miễn phí", "Browse & download free e-books online", "Tải sách online miễn phí");
    add("Cử chỉ & Phím tắt", "Gestures & Shortcuts", "Cử chỉ & Phím tắt");
    add("Chạm giữa", "Tap center", "Chạm giữa");
    add("Vuốt cạnh trái: Mở menu 5 tab", "Swipe left edge: Open 5-tab menu", "Vuốt cạnh trái: Mở menu chính");
    add("Chạm giữa: Ẩn / hiện thanh công cụ", "Tap center: Toggle toolbars", "Chạm giữa: Ẩn / hiện thanh công cụ");
    add("Chạm / vuốt cạnh: Lật trang", "Tap / swipe edge: Turn page", "Chạm hoặc vuốt cạnh: Lật trang");
    add("Chạm vào các vùng trên để thử cử chỉ", "Tap any zone above to try gestures", "Chạm vào các vùng trên để thử");
    add("Space / J, K: Lật trang mượt mà", "Space / J, K: Turn pages smoothly", "Space / J, K: Lật trang");
    add("Z / W: Tăng hoặc giảm cỡ chữ tức thì", "Z / W: Increase or decrease font size", "Z / W: Tăng / giảm cỡ chữ nhanh");
    add("D: Chuyển nhanh nền Tối / Sepia / Sáng", "D: Cycle Dark / Sepia / Light theme", "D: Chuyển nhanh nền Tối / Sepia / Sáng");
    add("R: Bật hoặc tắt Thước kẻ đọc", "R: Toggle Reading Ruler", "R: Bật / tắt Reading Ruler");
    add("V: Bật hoặc tắt Bionic Reading", "V: Toggle Bionic Reading", "V: Bật / tắt Bionic Reading");
    add("?: Mở bảng tất cả phím tắt", "?: Show all keyboard shortcuts", "?: Xem tất cả phím tắt");
    add("Ghi chú, Từ điển & TTS", "Notes, Dictionary & TTS", "Ghi chú, Từ điển & TTS");
    add("Ghi chú & Quote Card", "Highlights & Quote Card", "Highlight & Quote Card");
    add("Tô màu, ghi chú & xuất ảnh PNG", "Highlight, annotate & export PNG cards", "Highlight, ghi chú & xuất ảnh PNG");
    add("Từ điển & Dịch nhanh", "Dictionary & Translate", "Từ điển & Dịch nhanh");
    add("Bôi đen từ để tra ngay trên trang", "Select text for instant lookup", "Bôi đen từ để tra ngay trên trang");
    add("Nghe đọc (TTS)", "Text-to-Speech (TTS)", "Nghe sách (TTS)");
    add("eSpeak-NG ngoại tuyến & Cloud TTS", "Offline eSpeak-NG & Cloud TTS", "eSpeak-NG Offline & Cloud TTS");
    add("Sẵn sàng đọc sách!", "Ready to Read!", "Sẵn sàng đọc sách!");
    add("Mở lại bất cứ lúc nào trong Giới thiệu → Hướng dẫn.", "Replay anytime in About → Guide.", "Xem lại bất cứ lúc nào ở Giới thiệu → Hướng dẫn.");
    add("Bắt đầu đọc ngay", "Start Reading", "Bắt đầu đọc ngay");

    // --- Recent UI & Settings ---
    add("Xem lại hướng dẫn từng bước", "Replay step-by-step walkthrough", "Xem lại hướng dẫn từng bước");
    add("TRONG SÁCH", "IN BOOKS", "TRONG SÁCH");
    add("Màu nền trang đọc", "Reader background color", "Màu nền trang đọc");
    add("Báo lỗi & Góp ý", "Bug Report & Feedback", "Báo lỗi & Góp ý");
    add("Gửi email tới nhatlm0910@gmail.com (log tùy chọn)", "Send email to nhatlm0910@gmail.com (optional log)", "Gửi email tới nhatlm0910@gmail.com (log tùy chọn)");
    add("Chỉnh mục tiêu đọc mỗi ngày", "Edit daily reading goal", "Chỉnh mục tiêu đọc mỗi ngày");
    add("Sửa", "Edit", "Sửa");

    // --- Remaining QML UI Strings (100% Coverage) ---
    add("Đánh giá & Cảm nhận", "Review & Notes", "Đánh giá & Cảm nhận");
    add("Viết vài dòng cảm nhận về cuốn sách...", "Write a short review or impression...", "Viết vài dòng cảm nhận về cuốn sách...");
    add("Lưu cảm nhận", "Save Review", "Lưu cảm nhận");
    add("Đã lưu đánh giá & cảm nhận", "Review saved", "Đã lưu đánh giá & cảm nhận");
    add("Ẩn trình sửa thông tin", "Hide metadata editor", "Ẩn trình sửa thông tin");
    add("Sửa thông tin sách (Tên / Tác giả / Bộ)", "Edit Book Info (Title / Author / Series)", "Sửa thông tin sách (Tên / Tác giả / Bộ)");
    add("Tên sách", "Book title", "Tên sách");
    add("Bộ truyện (Series)", "Series", "Bộ truyện (Series)");
    add("Số tập (VD: 1)", "Volume (e.g. 1)", "Số tập (VD: 1)");
    add("Lưu thay đổi", "Save Changes", "Lưu thay đổi");
    add("Đã cập nhật thông tin sách", "Book info updated", "Đã cập nhật thông tin sách");
    add("Mục tiêu:", "Goal:", "Mục tiêu:");
    add("Ngẫu nhiên", "Shuffle", "Ngẫu nhiên");
    add("Xóa từ vựng?", "Delete vocabulary word?", "Xóa từ vựng?");
    add("Bạn có chắc muốn xóa từ này khỏi sổ từ vựng?", "Are you sure you want to remove this word?", "Bạn có chắc muốn xóa từ này khỏi sổ từ vựng?");
    add("Lưu ảnh trích dẫn", "Save Quote Card", "Lưu Quote Card");
    add("Đã lưu ảnh trích dẫn: ", "Saved Quote Card: ", "Đã lưu Quote Card: ");
    add("Không thể tạo ảnh trích dẫn", "Unable to create Quote Card", "Không thể tạo Quote Card");
    add("Chưa có vị trí trước đó", "No previous position", "Chưa có vị trí trước đó");
    add("Đã quay lại vị trí trước", "Returned to previous position", "Đã quay lại vị trí trước");
    add("1x · Cuộn liên tục", "1x · Continuous Scroll", "1x · Cuộn liên tục");
    add("Phím tắt bàn phím BB10", "BB10 Keyboard Shortcuts", "Phím tắt bàn phím BB10");
    add("Đã bật thước kẻ đọc", "Reading Ruler enabled", "Đã bật Reading Ruler");
    add("Đã tắt thước kẻ đọc", "Reading Ruler disabled", "Đã tắt Reading Ruler");
    add("Đã khóa cuộn ngang PDF", "PDF horizontal scroll locked", "Đã khóa cuộn ngang PDF");
    add("Đã mở khóa cuộn ngang PDF", "PDF horizontal scroll unlocked", "Đã mở khóa cuộn ngang PDF");
    add("Đã bật Bionic Reading", "Bionic Reading enabled", "Đã bật Bionic Reading");
    add("Đã tắt Bionic Reading", "Bionic Reading disabled", "Đã tắt Bionic Reading");
    add("Bạn đã đọc liên tục một chặng — hãy nghỉ mắt 20 giây nhé!", "You've been reading for a stretch — rest your eyes for 20 seconds!", "Bạn đã đọc khá lâu rồi — nghỉ mắt 20 giây nhé!");
    add("Xoay trang 90°", "Rotate Page 90°", "Xoay trang 90°");
    add("Đảo màu đêm", "Night Invert", "Đảo màu Đêm");
    add("Cuộn liên tục", "Continuous Scroll", "Cuộn liên tục");
    add("Phóng to trang", "Zoom Page", "Phóng to trang");
    add("Bionic Reading", "Bionic Reading", "Bionic Reading");
    add("Nhập số phút mục tiêu mỗi ngày (5 - 240 phút):", "Enter daily goal in minutes (5 - 240 min):", "Nhập số phút mục tiêu mỗi ngày (5 - 240 phút):");
    add("Báo lỗi / Góp ý (nhatlm0910@gmail.com)", "Bug Report / Feedback (nhatlm0910@gmail.com)", "Báo lỗi / Góp ý (nhatlm0910@gmail.com)");
    add("Gửi báo lỗi hoặc góp ý qua Email BlackBerry Hub tới nhatlm0910@gmail.com. Đính kèm nhật ký kỹ thuật là tùy chọn.", "Send a bug report or feedback via BlackBerry Hub Email to nhatlm0910@gmail.com. Attaching logs is optional.", "Gửi báo lỗi hoặc góp ý qua Email BlackBerry Hub tới nhatlm0910@gmail.com. Đính kèm file log là tùy chọn.");
    add("NỘI DUNG BÁO LỖI", "REPORT DETAILS", "NỘI DUNG BÁO LỖI");
    add("Phân loại", "Category", "Phân loại");
    add("Báo lỗi (Bug)", "Bug Report", "Báo lỗi (Bug)");
    add("Lỗi mở/hiển thị sách", "Book open / render issue", "Lỗi mở/hiển thị sách");
    add("Góp ý tính năng", "Feature suggestion", "Góp ý tính năng");
    add("Tiêu đề ngắn gọn (ví dụ: Lỗi mở file EPUB)...", "Short subject (e.g. EPUB open issue)...", "Tiêu đề ngắn gọn (ví dụ: Lỗi mở file EPUB)...");
    add("Mô tả chi tiết lỗi bạn gặp phải hoặc góp ý tính năng...", "Describe the bug or feature suggestion in detail...", "Mô tả chi tiết lỗi bạn gặp phải hoặc góp ý tính năng...");
    add("TÙY CHỌN KỸ THUẬT", "TECHNICAL OPTIONS", "TÙY CHỌN KỸ THUẬT");
    add("Đính kèm nhật ký kỹ thuật (Log)", "Attach technical log", "Đính kèm file log");
    add("Không bắt buộc — gửi kèm ~60 dòng log gần nhất để hỗ trợ sửa lỗi", "Optional — attach ~60 recent log lines to help diagnose issues", "Tùy chọn — gửi kèm ~60 dòng log gần nhất để dễ sửa lỗi");
    add("Gửi Email tới nhatlm0910@gmail.com", "Send Email to nhatlm0910@gmail.com", "Gửi Email tới nhatlm0910@gmail.com");
    add("Vui lòng nhập tiêu đề hoặc mô tả trước khi gửi.", "Please enter a subject or description before sending.", "Vui lòng nhập tiêu đề hoặc mô tả trước khi gửi.");
    add("Đang mở trình soạn Email tới nhatlm0910@gmail.com...", "Opening Email composer to nhatlm0910@gmail.com...", "Đang mở trình soạn Email tới nhatlm0910@gmail.com...");
    add("Không thể mở ứng dụng Email trên thiết bị.", "Unable to open the Email app on this device.", "Không thể mở ứng dụng Email trên thiết bị.");
    add("Căn lề văn bản", "Text Alignment", "Căn lề văn bản");
    add("Căn đều hai bên", "Justify", "Căn đều hai bên");
    add("Căn trái", "Align Left", "Căn trái");
    add("Bỏ CSS gốc của sách", "Ignore Book CSS", "Tắt CSS gốc của sách");
    add("Dùng định dạng chuẩn của Libris thay cho CSS nhúng", "Use Libris styling instead of embedded book CSS", "Dùng định dạng của Libris thay cho CSS nhúng");
    add("Thụt đầu dòng", "Paragraph Indent", "Thụt đầu dòng");
    add("Không thụt (0)", "None (0)", "Không thụt (0)");
    add("Chuẩn (1.5em)", "Standard (1.5em)", "Chuẩn (1.5em)");
    add("Sâu (2.2em)", "Deep (2.2em)", "Sâu (2.2em)");
    add("Khoảng cách đoạn", "Paragraph Spacing", "Khoảng cách đoạn");
    add("Độ đậm chữ", "Font Weight", "Độ đậm chữ");
    add("Thường (400)", "Regular (400)", "Thường (400)");
    add("Vừa (500)", "Medium (500)", "Vừa (500)");
    add("Đậm (600)", "SemiBold (600)", "Đậm (600)");
    add("Độ sáng chữ nền Tối", "Dark Mode Text Brightness", "Độ sáng chữ nền Tối");
    add("Sáng rõ", "Crisp", "Sáng rõ");
    add("Dịu mắt", "Soft", "Dịu mắt");
    add("Rất dịu (đọc đêm)", "Ultra Soft (Night)", "Rất dịu (đọc đêm)");
    add("Bionic Reading (Đọc nhanh)", "Bionic Reading", "Bionic Reading");
    add("Làm đậm nửa đầu mỗi từ giúp mắt bắt nhịp đọc nhanh hơn", "Bold the first half of each word for faster reading", "In đậm nửa đầu mỗi từ giúp mắt bắt nhịp đọc nhanh hơn");
    add("Thanh tiến độ mảnh", "Thin Progress Bar", "Thanh tiến độ mảnh");
    add("Hiển thị vạch tiến độ 3px phía trên chân trang", "Show a 3px progress bar above the footer", "Hiển thị vạch tiến độ 3px phía trên chân trang");
    add("Ước tính thời gian còn lại", "Remaining Time Estimate", "Ước tính thời gian còn lại");
    add("Hiển thị số phút dự kiến hết chương ở chân trang", "Show estimated minutes left in chapter in footer", "Hiển thị số phút dự kiến hết chương ở chân trang");
    add("Khôi phục dữ liệu?", "Restore backup?", "Khôi phục dữ liệu?");
    add("Thao tác này sẽ ghi đè tiến độ đọc, ghi chú và cài đặt hiện tại bằng bản sao lưu. Tiếp tục?", "This will overwrite your current reading progress, notes, and settings with the backup. Continue?", "Thao tác này sẽ ghi đè tiến độ đọc, ghi chú và cài đặt hiện tại bằng bản sao lưu. Tiếp tục?");
    add("Đảo chiều phím âm lượng", "Invert Volume Keys", "Đảo chiều phím âm lượng");
    add("Nhấn Tăng âm lượng để sang trang tiếp, Giảm âm lượng để lùi trang", "Press Volume Up for next page, Volume Down for previous page", "Nhấn Tăng âm lượng để sang trang tiếp, Giảm âm lượng để lùi trang");
    add("Nhắc nghỉ mắt (Pomodoro)", "Eye Break Reminder (Pomodoro)", "Nhắc nghỉ mắt (Pomodoro)");

    // --- Minimal Onboarding Guide ---
    add("Chào mừng", "Welcome", "Chào mừng");
    add("Giao diện", "Theme", "Giao diện");
    add("Thêm sách", "Add Books", "Thêm sách");
    add("Trình duyệt → BB10", "Browser → BB10", "Trình duyệt → BB10");
    add("Kho sách online", "Online catalogs", "Kho sách online");
    add("Điều khiển", "Controls", "Điều khiển");
    add("Chạm để thử", "Tap to try", "Chạm để thử");
    add("Vuốt trái · Menu", "Swipe left · Menu", "Vuốt trái · Menu");
    add("Chạm giữa · Ẩn/hiện thanh", "Tap center · Toggle bars", "Chạm giữa · Ẩn/hiện thanh");
    add("Chạm cạnh · Lật trang", "Tap edge · Turn page", "Chạm cạnh · Lật trang");
    add("Space · Lật trang", "Space · Turn page", "Space · Lật trang");
    add("Z / W · Cỡ chữ", "Z / W · Font size", "Z / W · Cỡ chữ");
    add("D · Đổi màu nền", "D · Switch theme", "D · Đổi màu nền");
    add("R · Reading Ruler", "R · Reading Ruler", "R · Reading Ruler");
    add("V · Bionic Reading", "V · Bionic Reading", "V · Bionic Reading");
    add("? · Tất cả phím tắt", "? · All shortcuts", "? · Tất cả phím tắt");
    add("Công cụ đọc", "Reading Tools", "Công cụ đọc");
    add("Sẵn sàng!", "Ready!", "Sẵn sàng!");

    // --- Interactive BB10 Help Guide ---
    add("Vuốt trái để tiếp tục ›", "Swipe left to continue ›", "Vuốt trái để tiếp tục ›");
    add("Tự động quét sách trong máy & thẻ nhớ", "Auto-scans internal storage & SD card", "Tự động quét sách trong máy & thẻ nhớ");
    add("Mở trình duyệt máy tính để gửi sách không dây", "Open any browser to send books wirelessly", "Mở trình duyệt máy tính để gửi sách không dây");
    add("Tải sách miễn phí trực tiếp từ kho OPDS", "Browse & download books from OPDS catalogs", "Tải sách miễn phí trực tiếp từ kho OPDS");
    add("Sự tình cờ may mắn", "Happy accidental discovery", "Sự tình cờ may mắn");

    // --- Letter-only BB10 Passport Shortcuts ---
    add("Z · W · Cỡ chữ", "Z · W · Font size", "Z · W · Cỡ chữ");
    add("O · Tất cả phím tắt", "O · All shortcuts", "O · Tất cả phím tắt");
    add("Space: Lật trang / TTS\nJ / K hoặc N / P: Trang / Chương trước sau\nZ / W: Tăng / giảm cỡ chữ\nD / C / A: Đổi theme / Cuộn / Đọc TTS\nM / H / F / G: Đánh dấu / Ghi chú / Tìm / Mục lục\nL / I / S: Từ điển / Thông tin / Cài đặt\nR / V: Reading Ruler (hoặc xoay PDF) / Bionic Reading\nU / Backspace: Quay lại vị trí trước\nO: Hiện / ẩn bảng phím tắt", "Space: Turn page / TTS\nJ / K or N / P: Page / Chapter prev-next\nZ / W: Increase / decrease font size\nD / C / A: Theme / Scroll / Read aloud (TTS)\nM / H / F / G: Bookmark / Notes / Search / TOC\nL / I / S: Dictionary / Info / Settings\nR / V: Reading Ruler (or Rotate PDF) / Bionic Reading\nU / Backspace: Jump back\nO: Show / hide shortcuts", "Space: Lật trang / TTS\nJ / K hoặc N / P: Trang / Chương trước sau\nZ / W: Tăng / giảm cỡ chữ\nD / C / A: Đổi theme / Cuộn / Đọc TTS\nM / H / F / G: Đánh dấu / Ghi chú / Tìm / Mục lục\nL / I / S: Từ điển / Thông tin / Cài đặt\nR / V: Reading Ruler (hoặc xoay PDF) / Bionic Reading\nU / Backspace: Quay lại vị trí trước\nO: Hiện / ẩn bảng phím tắt");

    // --- Ergonomic BB10 Passport Shortcuts ---
    add("Z · X · Cỡ chữ", "Z · X · Font size", "Z · X · Cỡ chữ");
    add("Space / Backspace: Trang sau / trước (hoặc TTS)\nJ / K: Trang sau / trước · N / P: Chương sau / trước\nT / B: Đầu / Cuối sách · U: Quay lại · Enter: Toàn màn hình\nZ / X: Tăng / giảm cỡ chữ · D: Đổi màu nền\nC / R / V: Cuộn / Reading Ruler / Bionic Reading\nG / F / M / H: Mục lục / Tìm / Đánh dấu / Ghi chú\nL / A / I / S: Từ điển / Đọc TTS / Thông tin / Cài đặt\nW / Q: PDF Reflow / Khóa cuộn ngang PDF · O: Bảng này", "Space / Backspace: Next / Prev page (or TTS)\nJ / K: Next / Prev page · N / P: Next / Prev chapter\nT / B: Top / Bottom · U: Jump back · Enter: Fullscreen\nZ / X: Font size + / - · D: Cycle reading theme\nC / R / V: Scroll / Reading Ruler / Bionic Reading\nG / F / M / H: TOC / Search / Bookmark / Notes\nL / A / I / S: Dictionary / Read TTS / Info / Settings\nW / Q: PDF Reflow / Lock PDF scroll · O: This help", "Space / Backspace: Trang sau / trước (hoặc TTS)\nJ / K: Trang sau / trước · N / P: Chương sau / trước\nT / B: Đầu / Cuối sách · U: Quay lại · Enter: Toàn màn hình\nZ / X: Tăng / giảm cỡ chữ · D: Đổi màu nền\nC / R / V: Cuộn / Reading Ruler / Bionic Reading\nG / F / M / H: Mục lục / Tìm / Đánh dấu / Ghi chú\nL / A / I / S: Từ điển / Đọc TTS / Thông tin / Cài đặt\nW / Q: PDF Reflow / Khóa cuộn ngang PDF · O: Bảng này");

    // --- Guided Animated Reader Walkthrough ---
    add("Lật trang trước / sau (hoặc phát TTS)", "Turn pages prev / next (or play TTS)", "Lật trang trước / sau (hoặc phát TTS)");
    add("Tăng / giảm cỡ chữ tức thì", "Increase / decrease font size", "Tăng / giảm cỡ chữ nhanh");
    add("Đổi nền đọc: Tối · Sepia · Sáng", "Cycle theme: Dark · Sepia · Light", "Đổi nền đọc: Tối · Sepia · Sáng");
    add("Bật Reading Ruler tập trung dòng đọc", "Toggle Reading Ruler focus band", "Bật Reading Ruler tập trung dòng đọc");
    add("Bật Bionic Reading giúp đọc nhanh hơn", "Toggle Bionic Reading for faster focus", "Bật Bionic Reading giúp đọc nhanh hơn");
    add("Vuốt trái mở menu · Bấm O xem tất cả phím tắt", "Swipe left for menu · Press O for all shortcuts", "Vuốt trái mở menu · Bấm O xem tất cả phím tắt");

    // --- Visual BlackBerry Passport Keymap (Onboarding Page 4) ---
    add("ĐIỀU KHIỂN", "CONTROLS", "ĐIỀU KHIỂN");
    add("Bàn phím Passport & cử chỉ", "Passport Keyboard & Gestures", "Bàn phím Passport & cử chỉ");
    add("Chạm phím trên hình hoặc bấm trực tiếp phím cứng để xem chức năng", "Tap any key below or press a physical key to inspect", "Chạm phím trên hình hoặc bấm trực tiếp phím cứng để xem chức năng");
    add("Cử chỉ chạm: Trái / Phải lật trang · Giữa mở thanh công cụ", "Touch gestures: Tap Left / Right to turn page · Center for menu", "Cử chỉ chạm: Trái / Phải lật trang · Giữa mở thanh công cụ");
    add("PDF", "PDF", "PDF");
    add("GIAO DIỆN", "APPEARANCE", "GIAO DIỆN");
    add("CÔNG CỤ", "TOOLS", "CÔNG CỤ");
    add("ĐIỀU HƯỚNG", "NAVIGATION", "ĐIỀU HƯỚNG");

    add("Khóa cuộn ngang PDF", "Lock PDF horizontal scroll", "Khóa cuộn ngang PDF");
    add("Bật / tắt PDF Reflow", "Toggle PDF Reflow", "Bật / tắt PDF Reflow");
    add("Reading Ruler / Xoay PDF", "Reading Ruler / Rotate PDF", "Reading Ruler / Xoay PDF");
    add("Lên đầu trang / chương", "Jump to top", "Lên đầu trang / chương");
    add("Quay lại vị trí trước", "Jump back", "Quay lại vị trí trước");
    add("Thông tin sách", "Book info", "Thông tin sách");
    add("Bảng phím tắt", "Shortcuts overlay", "Bảng phím tắt");
    add("Chương trước", "Previous chapter", "Chương trước");
    add("Đọc thành tiếng (TTS)", "Read aloud (TTS)", "Đọc thành tiếng (TTS)");
    add("Mở Cài đặt", "Open Settings", "Mở Cài đặt");
    add("Đổi màu nền đọc", "Cycle reading theme", "Đổi màu nền đọc");
    add("Tìm kiếm trong sách", "Search in book", "Tìm kiếm trong sách");
    add("Mở Mục lục (TOC)", "Open Table of Contents", "Mở Mục lục (TOC)");
    add("Đánh dấu & Ghi chú", "Bookmarks & Notes", "Đánh dấu & Ghi chú");
    add("Trang sau / Cuộn xuống", "Next page / Scroll down", "Trang sau / Cuộn xuống");
    add("Trang trước / Cuộn lên", "Prev page / Scroll up", "Trang trước / Cuộn lên");
    add("Tra từ điển", "Dictionary lookup", "Tra từ điển");
    add("Trang trước", "Previous page", "Trang trước");
    add("Tăng cỡ chữ", "Increase font size", "Tăng cỡ chữ");
    add("Giảm cỡ chữ", "Decrease font size", "Giảm cỡ chữ");
    add("Đổi kiểu đọc (Trang / Cuộn)", "Toggle Paged / Scroll mode", "Đổi kiểu đọc (Trang / Cuộn)");
    add("Trang sau / Tạm dừng TTS", "Next page / Play-Pause TTS", "Trang sau / Tạm dừng TTS");
    add("Xuống cuối trang / chương", "Jump to bottom", "Xuống cuối trang / chương");
    add("Chương sau", "Next chapter", "Chương sau");
    add("Đánh dấu trang (Bookmark)", "Toggle Bookmark", "Đánh dấu trang (Bookmark)");
    add("Chế độ toàn màn hình", "Fullscreen mode", "Chế độ toàn màn hình");

    add("Giữ cố định chiều ngang khi phóng to trang PDF.", "Keep horizontal position locked while zooming PDF pages.", "Giữ cố định chiều ngang khi phóng to trang PDF.");
    add("Chuyển PDF sang dạng văn bản cuộn dễ đọc hơn.", "Reflow PDF into clean, readable flowing text.", "Chuyển PDF sang dạng văn bản cuộn dễ đọc hơn.");
    add("Bật thước tập trung dòng đọc hoặc xoay ngang PDF.", "Toggle Reading Ruler focus band, or rotate PDF pages.", "Bật thước tập trung dòng đọc hoặc xoay ngang PDF.");
    add("Nhảy nhanh về đầu chương hoặc đầu trang hiện tại.", "Jump straight to the top of the current chapter or page.", "Nhảy nhanh về đầu chương hoặc đầu trang hiện tại.");
    add("Trở về vị trí vừa đọc trước khi nhảy mục lục.", "Return to your previous reading position after jumping.", "Trở về vị trí vừa đọc trước khi nhảy mục lục.");
    add("Xem chi tiết sách, tiến độ và thống kê đọc.", "View book metadata, progress, and reading statistics.", "Xem chi tiết sách, tiến độ và thống kê đọc.");
    add("Mở hoặc ẩn nhanh danh sách toàn bộ phím tắt.", "Quickly show or hide the full shortcuts reference.", "Mở hoặc ẩn nhanh danh sách toàn bộ phím tắt.");
    add("Chuyển lùi về chương trước đó trong sách.", "Move back to the previous chapter in the book.", "Chuyển lùi về chương trước đó trong sách.");
    add("Bật hoặc tắt đọc sách tự động bằng giọng nói.", "Start or stop automatic text-to-speech playback.", "Bật hoặc tắt đọc sách tự động bằng giọng nói.");
    add("Mở nhanh trang cài đặt giao diện và đọc sách.", "Open reader appearance and behavior settings.", "Mở nhanh trang cài đặt giao diện và đọc sách.");
    add("Chuyển qua lại giữa nền Tối, Sepia và Sáng.", "Cycle between Dark, Sepia, and Light reading themes.", "Chuyển qua lại giữa nền Tối, Sepia và Sáng.");
    add("Tìm từ khóa hoặc cụm từ trong toàn bộ cuốn sách.", "Search for keywords or phrases across the whole book.", "Tìm từ khóa hoặc cụm từ trong toàn bộ cuốn sách.");
    add("Mở danh sách chương để nhảy đến phần mong muốn.", "Open the chapter list to jump to any section.", "Mở danh sách chương để nhảy đến phần mong muốn.");
    add("Mở danh sách bookmark, highlight và ghi chú.", "Open your saved bookmarks, highlights, and notes.", "Mở danh sách bookmark, highlight và ghi chú.");
    add("Sang trang tiếp theo hoặc cuộn xuống một nhịp.", "Advance to the next page or scroll down one step.", "Sang trang tiếp theo hoặc cuộn xuống một nhịp.");
    add("Quay lại trang trước hoặc cuộn lên một nhịp.", "Go back one page or scroll up one step.", "Quay lại trang trước hoặc cuộn lên một nhịp.");
    add("Mở từ điển tra nhanh nghĩa từ ngay khi đang đọc.", "Look up word definitions while reading.", "Mở từ điển tra nhanh nghĩa từ ngay khi đang đọc.");
    add("Lùi về trang trước hoặc cuộn lên một nhịp.", "Go back to the previous page or scroll up one step.", "Lùi về trang trước hoặc cuộn lên một nhịp.");
    add("Phóng to chữ đọc thêm 1 nấc ngay lập tức.", "Increase reading font size by one step immediately.", "Phóng to chữ đọc thêm 1 nấc ngay lập tức.");
    add("Thu nhỏ chữ đọc đi 1 nấc ngay lập tức.", "Decrease reading font size by one step immediately.", "Thu nhỏ chữ đọc đi 1 nấc ngay lập tức.");
    add("Chuyển giữa lật từng trang và cuộn liên tục.", "Switch between paginated turning and continuous scroll.", "Chuyển giữa lật từng trang và cuộn liên tục.");
    add("Tô đậm đầu từ giúp mắt lướt và đọc nhanh hơn.", "Bold word prefixes to help your eyes scan faster.", "Tô đậm đầu từ giúp mắt lướt và đọc nhanh hơn.");
    add("Sang trang tiếp theo, hoặc phát / tạm dừng TTS.", "Turn to next page, or play / pause TTS when active.", "Sang trang tiếp theo, hoặc phát / tạm dừng TTS.");
    add("Nhảy nhanh xuống cuối chương hoặc cuối trang hiện tại.", "Jump straight to the bottom of the current chapter or page.", "Nhảy nhanh xuống cuối chương hoặc cuối trang hiện tại.");
    add("Chuyển nhanh sang chương kế tiếp trong sách.", "Advance quickly to the next chapter in the book.", "Chuyển nhanh sang chương kế tiếp trong sách.");
    add("Lưu hoặc bỏ đánh dấu trang đang đọc hiện tại.", "Save or remove a bookmark on the current page.", "Lưu hoặc bỏ đánh dấu trang đang đọc hiện tại.");
    add("Ẩn hoặc hiện thanh trạng thái để tập trung đọc.", "Hide or show the status bar for distraction-free reading.", "Ẩn hoặc hiện thanh trạng thái để tập trung đọc.");

    // --- Hybrid Reading Statistics UI ---
    add("Đã hoàn thành mục tiêu hôm nay", "Daily reading goal completed!", "Đã hoàn thành mục tiêu hôm nay");
    add("Còn", "Need", "Còn");
    add("phút nữa", "min left", "phút nữa");

    // --- Multi-Device Onboarding Keymap & Per-Format Brightness ---
    add("Bàn phím & cử chỉ", "Keyboard & Gestures", "Bàn phím & cử chỉ");
    add("Độ sáng", "Brightness", "Độ sáng");
    add("Độ sáng màn hình đọc", "Reader Screen Brightness", "Độ sáng màn hình đọc");
    add("Lưu riêng cho từng định dạng sách (EPUB, PDF, CBZ...)", "Saved independently per book format (EPUB, PDF, CBZ...)", "Lưu riêng cho từng định dạng sách (EPUB, PDF, CBZ...)");
}

void AppTranslator::setLanguage(const QString &lang)
{
    if (lang == "en" || lang == "vi") {
        m_language = lang;
    }
}

QString AppTranslator::currentLanguage() const
{
    return m_language;
}

QString AppTranslator::translate(const char *context, const char *sourceText,
                                const char *disambiguation) const
{
    Q_UNUSED(context);
    Q_UNUSED(disambiguation);

    if (!sourceText) return QString();

    QString keyUtf8 = QString::fromUtf8(sourceText);
    QString keyLatin = QString::fromLatin1(sourceText);

    if (m_language == "en") {
        if (m_en.contains(keyUtf8)) {
            return m_en.value(keyUtf8);
        }
        if (m_en.contains(keyLatin)) {
            return m_en.value(keyLatin);
        }
        return keyUtf8;
    } else {
        if (m_vi.contains(keyUtf8)) {
            return m_vi.value(keyUtf8);
        }
        if (m_vi.contains(keyLatin)) {
            return m_vi.value(keyLatin);
        }
        // Always return proper UTF-8 rather than empty (which triggers Latin-1 fallback in Qt 4!)
        return keyUtf8;
    }
}

bool AppTranslator::isEmpty() const
{
    return false;
}
