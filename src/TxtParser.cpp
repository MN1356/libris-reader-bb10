#include "TxtParser.hpp"
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QRegExp>
#include <QDebug>

static bool isGenericTitle(const QString &title)
{
    QString t = title.trimmed();
    if (t.isEmpty()) return true;

    QRegExp justNumRe("^#?\\s*([0-9]+|[IVXLCDM]+)\\s*[:.-]?\\s*$", Qt::CaseInsensitive);
    if (justNumRe.exactMatch(t)) return true;

    QRegExp genericRe("^\\s*(Chương|Chapter|Phần|Part|Hồi|Quyển|Tập|Mục|Tiết|Bài|Book|Section)\\s+([0-9]+|[IVXLCDM]+)\\s*[:.-]?\\s*$", Qt::CaseInsensitive);
    if (genericRe.exactMatch(t)) return true;

    QRegExp dupRe("^\\s*(?:Chương|Chapter)\\s+([0-9]+|[IVXLCDM]+)\\s*[:\\-–—]\\s*(?:(?:Chương|Chapter)\\s+)?\\1\\s*$", Qt::CaseInsensitive);
    if (dupRe.exactMatch(t)) return true;

    QRegExp chCodeRe("^(?:ch|chap|chapter|sec|section|part)[_-]?[0-9]+$", Qt::CaseInsensitive);
    if (chCodeRe.exactMatch(t)) return true;

    QRegExp singleWordRe("^(Chương|Chapter|Phần|Part|Hồi|Quyển|Tập|Mục|Tiết|Bài|Book|Section|Untitled|Page|Trang)\\s*$", Qt::CaseInsensitive);
    if (singleWordRe.exactMatch(t)) return true;

    return false;
}

static bool isSeparatorOrCreditLine(const QString &line)
{
    QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) return true;

    // Pure separators / symbols / dashes / stars
    QRegExp sepRe("^[\\s*\\-_=~#•·+*o0O–—.…]+$");
    if (sepRe.exactMatch(trimmed)) return true;

    // Common metadata / credit headers in Vietnamese and English ebooks
    QRegExp creditRe("^(?:tác giả|dịch giả|người dịch|biên tập|editor|convert|nguồn|nhóm dịch|thực hiện|website|link|trang|author|translator|source|credits?)\\s*[:\\-–—]", Qt::CaseInsensitive);
    if (creditRe.indexIn(trimmed) != -1) return true;

    // URLs and novel website watermarks
    if (trimmed.contains(QRegExp("https?:\\/\\/|www\\.|(?:truyen|tangthuvien|bachngocsach|metruyenchu|wikidich|wattpad|sstruyen|dtruyen)\\.(?:com|vn|net|org)", Qt::CaseInsensitive))) {
        return true;
    }

    return false;
}

TxtParser::TxtParser()
    : m_title("Tài liệu Text")
    , m_author("Văn bản thường")
{
}

TxtParser::~TxtParser()
{
    close();
}

void TxtParser::close()
{
    m_filePath.clear();
    m_title.clear();
    m_author.clear();
    m_coverPath.clear();
    m_chapters.clear();
    m_chapterTexts.clear();
}

QString TxtParser::escapeHtml(const QString &text)
{
    QString escaped = text;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\"", "&quot;");
    return escaped;
}

static ushort mapTcvn3Byte(unsigned char b)
{
    switch (b) {
        case 0xA1: return 0x0102; // Ă
        case 0xA2: return 0x00C2; // Â
        case 0xA3: return 0x00CA; // Ê
        case 0xA4: return 0x00D4; // Ô
        case 0xA5: return 0x01A0; // Ơ
        case 0xA6: return 0x01AF; // Ư
        case 0xA7: return 0x0110; // Đ
        case 0xA8: return 0x0103; // ă
        case 0xA9: return 0x00E2; // â
        case 0xAA: return 0x00EA; // ê
        case 0xAB: return 0x00F4; // ô
        case 0xAC: return 0x01A1; // ơ
        case 0xAD: return 0x01B0; // ư
        case 0xAE: return 0x0111; // đ
        case 0xB5: return 0x00E0; // à
        case 0xB6: return 0x1EA3; // ả
        case 0xB7: return 0x00E3; // ã
        case 0xB8: return 0x00E1; // á
        case 0xB9: return 0x1EA1; // ạ
        case 0xBB: return 0x1EB1; // ằ
        case 0xBC: return 0x1EB3; // ẳ
        case 0xBD: return 0x1EB5; // ẵ
        case 0xBE: return 0x1EAF; // ắ
        case 0xC6: return 0x1EB7; // ặ
        case 0xC7: return 0x1EA7; // ầ
        case 0xC8: return 0x1EA9; // ẩ
        case 0xC9: return 0x1EAB; // ẫ
        case 0xCA: return 0x1EA5; // ấ
        case 0xCB: return 0x1EAD; // ậ
        case 0xCC: return 0x00E8; // è
        case 0xCE: return 0x1EBB; // ẻ
        case 0xCF: return 0x1EBD; // ẽ
        case 0xD0: return 0x00E9; // é
        case 0xD1: return 0x1EB9; // ẹ
        case 0xD2: return 0x1EC1; // ề
        case 0xD3: return 0x1EC3; // ể
        case 0xD4: return 0x1EC5; // ễ
        case 0xD5: return 0x1EBF; // ế
        case 0xD6: return 0x1EC7; // ệ
        case 0xD7: return 0x00EC; // ì
        case 0xD8: return 0x1EC9; // ỉ
        case 0xDC: return 0x0129; // ĩ
        case 0xDD: return 0x00ED; // í
        case 0xDE: return 0x1ECB; // ị
        case 0xDF: return 0x00F2; // ò
        case 0xE1: return 0x1ECF; // ỏ
        case 0xE2: return 0x00F5; // õ
        case 0xE3: return 0x00F3; // ó
        case 0xE4: return 0x1ECD; // ọ
        case 0xE5: return 0x1ED3; // ồ
        case 0xE6: return 0x1ED5; // ổ
        case 0xE7: return 0x1ED7; // ỗ
        case 0xE8: return 0x1ED1; // ố
        case 0xE9: return 0x1ED9; // ộ
        case 0xEA: return 0x1EDD; // ờ
        case 0xEB: return 0x1EDF; // ở
        case 0xEC: return 0x1EE1; // ỡ
        case 0xED: return 0x1EDB; // ớ
        case 0xEE: return 0x1EE3; // ợ
        case 0xEF: return 0x00F9; // ù
        case 0xF1: return 0x1EE7; // ủ
        case 0xF2: return 0x0169; // ũ
        case 0xF3: return 0x00FA; // ú
        case 0xF4: return 0x1EE5; // ụ
        case 0xF5: return 0x1EEB; // ừ
        case 0xF6: return 0x1EED; // ử
        case 0xF7: return 0x1EEF; // ữ
        case 0xF8: return 0x1EE9; // ứ
        case 0xF9: return 0x1EF1; // ự
        case 0xFA: return 0x1EF3; // ỳ
        case 0xFB: return 0x1EF7; // ỷ
        case 0xFC: return 0x1EF9; // ỹ
        case 0xFD: return 0x00FD; // ý
        case 0xFE: return 0x1EF5; // ỵ
        default: return b;
    }
}

static bool looksLikeTcvn3(const QByteArray &data)
{
    int sampleLen = qMin(data.size(), 16384);
    int tcvnHits = 0;
    int win1258Combines = 0;
    const unsigned char *p = reinterpret_cast<const unsigned char*>(data.constData());
    for (int i = 0; i < sampleLen; ++i) {
        unsigned char c = p[i];
        if ((c >= 0xA8 && c <= 0xAE) || c == 0xB8 || c == 0xEA || c == 0xEE || c == 0xF5 || c == 0xF8) {
            tcvnHits++;
        }
        if (i > 0 && (c == 0xCC || c == 0xD2 || c == 0xDE || c == 0xEC || c == 0xF2)) {
            unsigned char prev = p[i - 1];
            if ((prev >= 'A' && prev <= 'z') || prev >= 0xC0) {
                win1258Combines++;
            }
        }
    }
    return tcvnHits > 4 && tcvnHits > win1258Combines * 2;
}

static ushort mapVniSingleByte(unsigned char b)
{
    switch (b) {
    case 0xF1: return 0x0111; // ñ -> đ
    case 0xD1: return 0x0110; // Ñ -> Đ
    case 0xF4: return 0x01A1; // ô -> ơ
    case 0xD4: return 0x01A0; // Ô -> Ơ
    case 0xF6: return 0x01B0; // ö -> ư
    case 0xD6: return 0x01AF; // Ö -> Ư
    case 0xEC: return 0x00EC; // ì
    case 0xCC: return 0x00CC; // Ì
    case 0xED: return 0x00ED; // í
    case 0xCD: return 0x00CD; // Í
    case 0xE6: return 0x1EC9; // æ -> ỉ
    case 0xC6: return 0x1EC8; // Æ -> Ỉ
    case 0xF3: return 0x0129; // ó -> ĩ
    case 0xD3: return 0x0128; // Ó -> Ĩ
    case 0xF2: return 0x1ECB; // ò -> ị
    case 0xD2: return 0x1ECA; // Ò -> Ị
    case 0xEE: return 0x1EF5; // î -> ỵ
    case 0xCE: return 0x1EF4; // Î -> Ỵ
    default: return b;
    }
}

static bool mapVniPair(unsigned char b1, unsigned char b2, ushort &outChar)
{
    bool upper = (b1 >= 'A' && b1 <= 'Z');
    unsigned char base = upper ? (b1 + 32) : b1;
    // Tone/diacritic byte in VNI-Windows
    if (base == 'a') {
        switch (b2) {
        case 0xF8: outChar = upper ? 0x00C0 : 0x00E0; return true; // à
        case 0xF9: outChar = upper ? 0x00C1 : 0x00E1; return true; // á
        case 0xFB: outChar = upper ? 0x1EA2 : 0x1EA3; return true; // ả
        case 0xF5: outChar = upper ? 0x00C3 : 0x00E3; return true; // ã
        case 0xEF: outChar = upper ? 0x1EA0 : 0x1EA1; return true; // ạ
        case 0xE2: outChar = upper ? 0x00C2 : 0x00E2; return true; // â
        case 0xE0: outChar = upper ? 0x1EA6 : 0x1EA7; return true; // ầ
        case 0xE1: outChar = upper ? 0x1EA4 : 0x1EA5; return true; // ấ
        case 0xE5: outChar = upper ? 0x1EA8 : 0x1EA9; return true; // ẩ
        case 0xE3: outChar = upper ? 0x1EAA : 0x1EAB; return true; // ẫ
        case 0xE4: outChar = upper ? 0x1EAC : 0x1EAD; return true; // ậ
        case 0xEA: outChar = upper ? 0x0102 : 0x0103; return true; // ă
        case 0xE8: outChar = upper ? 0x1EB0 : 0x1EB1; return true; // ằ
        case 0xE9: outChar = upper ? 0x1EAE : 0x1EAF; return true; // ắ
        case 0xFA: outChar = upper ? 0x1EB2 : 0x1EB3; return true; // ẳ
        case 0xFC: outChar = upper ? 0x1EB4 : 0x1EB5; return true; // ẵ
        case 0xEB: outChar = upper ? 0x1EB6 : 0x1EB7; return true; // ặ
        default: break;
        }
    } else if (base == 'e') {
        switch (b2) {
        case 0xF8: outChar = upper ? 0x00C8 : 0x00E8; return true; // è
        case 0xF9: outChar = upper ? 0x00C9 : 0x00E9; return true; // é
        case 0xFB: outChar = upper ? 0x1EBA : 0x1EBB; return true; // ẻ
        case 0xF5: outChar = upper ? 0x1EBC : 0x1EBD; return true; // ẽ
        case 0xEF: outChar = upper ? 0x1EB8 : 0x1EB9; return true; // ẹ
        case 0xE2: outChar = upper ? 0x00CA : 0x00EA; return true; // ê
        case 0xE0: outChar = upper ? 0x1EC0 : 0x1EC1; return true; // ề
        case 0xE1: outChar = upper ? 0x1EBE : 0x1ECF; return true; // ế (fix below)
        case 0xE5: outChar = upper ? 0x1EC2 : 0x1EC3; return true; // ể
        case 0xE3: outChar = upper ? 0x1EC4 : 0x1EC5; return true; // ễ
        case 0xE4: outChar = upper ? 0x1EC6 : 0x1EC7; return true; // ệ
        default: break;
        }
    } else if (base == 'o') {
        switch (b2) {
        case 0xF8: outChar = upper ? 0x00D2 : 0x00F2; return true; // ò
        case 0xF9: outChar = upper ? 0x00D3 : 0x00F3; return true; // ó
        case 0xFB: outChar = upper ? 0x1ECE : 0x1ECF; return true; // ỏ
        case 0xF5: outChar = upper ? 0x00D5 : 0x00F5; return true; // õ
        case 0xEF: outChar = upper ? 0x1ECC : 0x1ECD; return true; // ọ
        case 0xE2: outChar = upper ? 0x00D4 : 0x00F4; return true; // ô
        case 0xE0: outChar = upper ? 0x1ED2 : 0x1ED3; return true; // ồ
        case 0xE1: outChar = upper ? 0x1ED0 : 0x1ED1; return true; // ố
        case 0xE5: outChar = upper ? 0x1ED4 : 0x1ED5; return true; // ổ
        case 0xE3: outChar = upper ? 0x1ED6 : 0x1ED7; return true; // ỗ
        case 0xE4: outChar = upper ? 0x1ED8 : 0x1ED9; return true; // ộ
        default: break;
        }
    } else if (b1 == 0xF4 || b1 == 0xD4) { // ơ / Ơ + tone
        bool up = (b1 == 0xD4);
        switch (b2) {
        case 0xF8: outChar = up ? 0x1EDC : 0x1EDD; return true; // ờ
        case 0xF9: outChar = up ? 0x1EDA : 0x1EDB; return true; // ớ
        case 0xFB: outChar = up ? 0x1EDE : 0x1EDF; return true; // ở
        case 0xF5: outChar = up ? 0x1EE0 : 0x1EE1; return true; // ỡ
        case 0xEF: outChar = up ? 0x1EE2 : 0x1EE3; return true; // ợ
        default: break;
        }
    } else if (base == 'u') {
        switch (b2) {
        case 0xF8: outChar = upper ? 0x00D9 : 0x00F9; return true; // ù
        case 0xF9: outChar = upper ? 0x00DA : 0x00FA; return true; // ú
        case 0xFB: outChar = upper ? 0x1EE6 : 0x1EE7; return true; // ủ
        case 0xF5: outChar = upper ? 0x0168 : 0x0169; return true; // ũ
        case 0xEF: outChar = upper ? 0x1EE4 : 0x1EE5; return true; // ụ
        default: break;
        }
    } else if (b1 == 0xF6 || b1 == 0xD6) { // ư / Ư + tone
        bool up = (b1 == 0xD6);
        switch (b2) {
        case 0xF8: outChar = up ? 0x1EEA : 0x1EEB; return true; // ừ
        case 0xF9: outChar = up ? 0x1EE8 : 0x1EE9; return true; // ứ
        case 0xFB: outChar = up ? 0x1EEC : 0x1EED; return true; // ử
        case 0xF5: outChar = up ? 0x1EEE : 0x1EEF; return true; // ữ
        case 0xEF: outChar = up ? 0x1EF0 : 0x1EF1; return true; // ự
        default: break;
        }
    } else if (base == 'y') {
        switch (b2) {
        case 0xF8: outChar = upper ? 0x1EF2 : 0x1EF3; return true; // ỳ
        case 0xF9: outChar = upper ? 0x00DD : 0x00FD; return true; // ý
        case 0xFB: outChar = upper ? 0x1EF6 : 0x1EF7; return true; // ỷ
        case 0xF5: outChar = upper ? 0x1EF8 : 0x1EF9; return true; // ỹ
        default: break;
        }
    }
    return false;
}

static bool looksLikeVni(const QByteArray &data)
{
    int sample = qMin(data.size(), 8192);
    const unsigned char *p = reinterpret_cast<const unsigned char*>(data.constData());
    int vniPairs = 0;
    for (int i = 0; i + 1 < sample; ++i) {
        ushort ch = 0;
        if (mapVniPair(p[i], p[i + 1], ch)) {
            vniPairs++;
            ++i;
        }
    }
    return vniPairs >= 4;
}

static QString decodeVni(const QByteArray &data)
{
    QString out;
    out.reserve(data.size());
    const unsigned char *p = reinterpret_cast<const unsigned char*>(data.constData());
    for (int i = 0; i < data.size(); ++i) {
        ushort ch = 0;
        if (i + 1 < data.size() && mapVniPair(p[i], p[i + 1], ch)) {
            if (ch == 0x1ECF && (p[i] == 'e' || p[i] == 'E')) ch = (p[i] == 'E') ? 0x1EBE : 0x1EBF; // ế/Ế
            out.append(QChar(ch));
            ++i;
        } else {
            out.append(QChar(mapVniSingleByte(p[i])));
        }
    }
    return out;
}

QString TxtParser::detectAndDecode(const QByteArray &data)
{
    if (data.size() >= 3 && (uchar)data[0] == 0xEF && (uchar)data[1] == 0xBB && (uchar)data[2] == 0xBF) {
        return QString::fromUtf8(data.constData() + 3, data.size() - 3);
    }
    if (data.size() >= 2 && (uchar)data[0] == 0xFF && (uchar)data[1] == 0xFE) {
        QTextCodec *codec = QTextCodec::codecForName("UTF-16LE");
        if (codec) return codec->toUnicode(data.constData() + 2, data.size() - 2);
    }
    if (data.size() >= 2 && (uchar)data[0] == 0xFE && (uchar)data[1] == 0xFF) {
        QTextCodec *codec = QTextCodec::codecForName("UTF-16BE");
        if (codec) return codec->toUnicode(data.constData() + 2, data.size() - 2);
    }

    // Try UTF-8 first
    QTextCodec *utf8Codec = QTextCodec::codecForName("UTF-8");
    if (utf8Codec) {
        QTextCodec::ConverterState state;
        QString text = utf8Codec->toUnicode(data.constData(), data.size(), &state);

        if (state.invalidChars == 0) {
            return text;
        }
    }

    // TCVN3 (ABC) & VNI legacy Vietnamese encodings
    if (looksLikeTcvn3(data)) {
        QString out;
        out.reserve(data.size());
        const unsigned char *p = reinterpret_cast<const unsigned char*>(data.constData());
        for (int i = 0; i < data.size(); ++i) {
            out.append(QChar(mapTcvn3Byte(p[i])));
        }
        return out;
    }
    if (looksLikeVni(data)) {
        return decodeVni(data);
    }

    // Fallback: try Windows-1258 (Vietnamese) normalized to NFC, or Windows-1252 / Latin1
    QTextCodec *win1258 = QTextCodec::codecForName("Windows-1258");
    if (win1258) {
        return win1258->toUnicode(data).normalized(QString::NormalizationForm_C);
    }

    QTextCodec *latin1 = QTextCodec::codecForName("ISO-8859-1");
    if (latin1) {
        return latin1->toUnicode(data);
    }

    return QString::fromUtf8(data);
}

bool TxtParser::open(const QString &filePath)
{
    close();
    m_filePath = filePath;

    QFile file(filePath);
    if (file.size() > 40 * 1024 * 1024) {
        qWarning() << "[TxtParser] Text file too large (>40MB):" << filePath << file.size();
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[TxtParser] Cannot open file:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.isEmpty()) {
        qWarning() << "[TxtParser] Empty text file:" << filePath;
        return false;
    }

    QString fullText = detectAndDecode(data);
    fullText.replace("\r\n", "\n");
    fullText.replace('\r', '\n');

    // Title from file basename
    QFileInfo fi(filePath);
    m_title = fi.completeBaseName();
    m_title.replace('_', ' ');
    m_author = "Văn bản TXT";

    // Split text into lines
    QStringList lines = fullText.split('\n');

    // Regex to match chapter headings:
    // e.g. "Chương 1", "Chapter 5", "Hồi 12", "Phần 3", "Quyển 2", "Tập 1", "1. Mở đầu", etc.
    QRegExp chapterRe("^\\s*(Chương\\s+[0-9IVXLCDM]+|Chapter\\s+[0-9IVXLCDM]+|Hồi\\s+[0-9IVXLCDM]+|Phần\\s+[0-9IVXLCDM]+|Quyển\\s+[0-9IVXLCDM]+|Tập\\s+[0-9IVXLCDM]+|Mục\\s+[0-9IVXLCDM]+|Bài\\s+[0-9IVXLCDM]+|Tiết\\s+[0-9IVXLCDM]+|Book\\s+[0-9IVXLCDM]+|Section\\s+[0-9IVXLCDM]+|Lời\\s+nói\\s+đầu|Mở\\s+đầu|Vĩ\\s+thanh|Ngoại\\s+truyện|Kết\\s+thúc|Prologue|Epilogue)", Qt::CaseInsensitive);

    typedef QPair<int, QString> FoundChapter;
    QList<FoundChapter> foundChapters;

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty()) continue;

        if (chapterRe.indexIn(line) != -1 && line.length() < 120) {
            // Check if this line already contains a descriptive subtitle on the same line
            if (!isGenericTitle(line)) {
                line.replace(QRegExp("^\\s*((?:Chương|Chapter|Hồi|Phần|Quyển|Tập|Mục|Tiết|Bài|Book|Section)\\s+[0-9IVXLCDM]+)\\s*[:\\-–—.]\\s*(.+)$", Qt::CaseInsensitive), "\\1: \\2");
            } else {
                for (int nextIdx = i + 1; nextIdx < lines.size() && nextIdx <= i + 6; ++nextIdx) {
                    QString nextLine = lines[nextIdx].trimmed();
                    if (nextLine.isEmpty()) continue;
                    if (chapterRe.indexIn(nextLine) != -1) break;

                    if (isSeparatorOrCreditLine(nextLine)) continue;

                    // Clean leading dashes, colons, spaces from nextLine
                    QString sub = nextLine;
                    while (!sub.isEmpty() && (sub.startsWith('-') || sub.startsWith(QChar(0x2014)) || 
                           sub.startsWith(QChar(0x2013)) || sub.startsWith(':') || sub.startsWith(' ') ||
                           sub.startsWith(QChar(0x2022)) || sub.startsWith('*'))) {
                        sub = sub.mid(1).trimmed();
                    }
                    while (sub.endsWith('.')) {
                        sub = sub.left(sub.length() - 1).trimmed();
                    }

                    if (sub.startsWith('"') || sub.startsWith(QChar(0x201C)) || sub.startsWith(QChar(0x00AB))) {
                        continue;
                    }

                    if (!sub.isEmpty() && sub.length() >= 2 && sub.length() < 100 && !isGenericTitle(sub)) {
                        while (line.endsWith(':') || line.endsWith('-') || line.endsWith('.') || line.endsWith(' ')) {
                            line = line.left(line.length() - 1).trimmed();
                        }
                        line = line + ": " + sub;
                        break;
                    }
                }
            }
            foundChapters.append(qMakePair(i, line));
        }
    }

    // If we found at least 2 distinct chapter headings, split by chapters
    if (foundChapters.size() >= 2) {
        // If there is preamble before the first chapter, add it as "Lời giới thiệu"
        if (foundChapters[0].first > 0) {
            QString preambleText;
            for (int j = 0; j < foundChapters[0].first; ++j) {
                preambleText += lines[j] + "\n";
            }
            if (!preambleText.trimmed().isEmpty()) {
                EpubChapter c;
                c.id = "preamble";
                c.title = "Giới thiệu";
                c.href = "txt://preamble";
                m_chapters.append(c);
                m_chapterTexts.append(preambleText);
            }
        }

        for (int k = 0; k < foundChapters.size(); ++k) {
            int start = foundChapters[k].first;
            int end = (k + 1 < foundChapters.size()) ? foundChapters[k + 1].first : lines.size();

            QString chText;
            for (int l = start; l < end; ++l) {
                chText += lines[l] + "\n";
            }

            EpubChapter c;
            c.id = QString("ch_%1").arg(k + 1);
            c.title = foundChapters[k].second;
            c.href = QString("txt://%1").arg(k + 1);
            m_chapters.append(c);
            m_chapterTexts.append(chText);
        }
    } else {
        // Fallback: Split into readable chunks of ~150 paragraphs or ~25KB each
        int chunkSize = 150;
        int chunkCount = 0;
        QString currentChunk;
        int paragraphCount = 0;

        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines[i];
            currentChunk += line + "\n";
            if (!line.trimmed().isEmpty()) {
                paragraphCount++;
            }

            if (paragraphCount >= chunkSize || i == lines.size() - 1) {
                chunkCount++;
                EpubChapter c;
                c.id = QString("chunk_%1").arg(chunkCount);
                c.title = QString("Phần %1").arg(chunkCount);
                c.href = QString("txt://%1").arg(chunkCount);
                m_chapters.append(c);
                m_chapterTexts.append(currentChunk);

                currentChunk.clear();
                paragraphCount = 0;
            }
        }
    }

    // Prepend dedicated book cover chapter
    EpubChapter coverCh;
    coverCh.id = "cover";
    coverCh.title = "Bìa sách";
    coverCh.href = "txt://cover";
    m_chapters.prepend(coverCh);
    m_chapterTexts.prepend(QString());

    qDebug() << "[TxtParser] Opened:" << m_title << "Chapters count:" << m_chapters.size();
    return !m_chapters.isEmpty();
}

QString TxtParser::getChapterContent(int index)
{
    if (index < 0 || index >= m_chapterTexts.size()) {
        return QString();
    }

    if (index == 0) {
        QString html;
        html += "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/>";
        html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"/>";
        html += "<style>";
        html += "html, body { width: 100%; height: 100%; margin: 0; padding: 0; overflow: hidden; }";
        html += ".cover-container { width: 100%; height: 100%; display: -webkit-box; -webkit-box-pack: center; -webkit-box-align: center; display: flex; justify-content: center; align-items: center; box-sizing: border-box; padding: 16px; text-align: center; }";
        html += ".cover-img { max-width: 90vw !important; max-height: 82vh !important; width: auto !important; height: auto !important; object-fit: contain; border-radius: 6px; box-shadow: 0 4px 18px rgba(0,0,0,0.6); display: block; margin: auto; }";
        html += ".cover-card { border: 2px solid rgba(128,128,128,0.3); border-radius: 12px; padding: 36px 24px; max-width: 85%; margin: auto; text-align: center; }";
        html += "</style></head><body><div class=\"cover-container\">";
        if (!m_coverPath.isEmpty() && QFile::exists(m_coverPath)) {
            html += QString("<img class=\"cover-img\" src=\"file://%1\" alt=\"Cover\"/>").arg(m_coverPath);
        } else {
            html += "<div class=\"cover-card\">";
            html += QString("<h1 style=\"font-size: 1.8em; margin-bottom: 12px;\">%1</h1>").arg(escapeHtml(m_title));
            html += QString("<h3 style=\"font-size: 1.2em; font-weight: normal; opacity: 0.85;\">%1</h3>").arg(escapeHtml(m_author));
            html += "</div>";
        }
        html += "</div></body></html>";
        return html;
    }

    QString rawText = m_chapterTexts[index];
    QString chTitle = m_chapters[index].title;
    QString ext = QFileInfo(m_filePath).suffix().toLower();

    if (ext == "html" || ext == "htm") {
        if (rawText.contains("<html", Qt::CaseInsensitive) || rawText.contains("<body", Qt::CaseInsensitive)) {
            return rawText;
        }
        return "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/></head><body>" + rawText + "</body></html>";
    }

    QString html;
    html += "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/>";
    html += "<style>";
    html += "body { font-family: -apple-system, sans-serif; line-height: 1.6; padding: 12px; margin: 0; word-wrap: break-word; }";
    html += "h2.txt-chapter-title { text-align: center; margin: 1.2em 0 1em 0; font-size: 1.3em; border-bottom: 1px solid rgba(128,128,128,0.2); padding-bottom: 0.5em; }";
    html += "p { text-indent: 1.8em; margin: 0.6em 0; }";
    html += "code { font-family: monospace; background: rgba(128,128,128,0.16); padding: 1px 4px; border-radius: 3px; }";
    html += "</style></head><body>";

    html += QString("<h2 class=\"txt-chapter-title\">%1</h2>").arg(escapeHtml(chTitle));

    QStringList lines = rawText.split('\n');
    if (ext == "md" || ext == "markdown") {
        QRegExp boldRx("\\*\\*([^*]+)\\*\\*");
        QRegExp italicRx("\\*([^*]+)\\*");
        QRegExp codeRx("`([^`]+)`");
        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            if (line.isEmpty()) continue;

            QString openTag = "<p>";
            QString closeTag = "</p>";
            QString body = line;
            if (line.startsWith("### ")) { openTag = "<h3>"; closeTag = "</h3>"; body = line.mid(4); }
            else if (line.startsWith("## ")) { openTag = "<h2>"; closeTag = "</h2>"; body = line.mid(3); }
            else if (line.startsWith("# ")) { openTag = "<h1>"; closeTag = "</h1>"; body = line.mid(2); }
            else if (line.startsWith("- ") || line.startsWith("* ")) { openTag = "<p style=\"text-indent:0;\">&bull; "; closeTag = "</p>"; body = line.mid(2); }
            else if (line.startsWith("> ")) { openTag = "<blockquote>"; closeTag = "</blockquote>"; body = line.mid(2); }

            QString esc = escapeHtml(body);
            esc.replace(boldRx, "<b>\\1</b>");
            esc.replace(italicRx, "<i>\\1</i>");
            esc.replace(codeRx, "<code>\\1</code>");
            html += openTag + esc + closeTag;
        }
        html += "</body></html>";
        return html;
    }
    QString currentParagraph;

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty()) {
            if (!currentParagraph.isEmpty()) {
                html += QString("<p>%1</p>").arg(escapeHtml(currentParagraph));
                currentParagraph.clear();
            }
        } else {
            // Check if this line is identical to chapter title, skip duplicate
            if (i < 3 && line == chTitle) continue;

            if (!currentParagraph.isEmpty()) {
                currentParagraph += " ";
            }
            currentParagraph += line;
        }
    }

    if (!currentParagraph.isEmpty()) {
        html += QString("<p>%1</p>").arg(escapeHtml(currentParagraph));
    }

    html += "</body></html>";
    return html;
}

QString TxtParser::extractCover(const QString &cacheDir)
{
    Q_UNUSED(cacheDir);
    return QString(); // Use default cover icon
}
