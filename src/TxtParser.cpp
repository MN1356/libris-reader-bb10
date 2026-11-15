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

static QString combineChapterTitle(const QString &baseTitle, const QString &subTitle, int chapterNumber = -1)
{
    QString b = baseTitle.trimmed();
    QString s = subTitle.trimmed();

    if (s.isEmpty()) return b;

    while (!s.isEmpty() && (s.startsWith('-') || s.startsWith(QChar(0x2014)) || 
           s.startsWith(QChar(0x2013)) || s.startsWith(':') || s.startsWith(' ') ||
           s.startsWith(QChar(0x2022)) || s.startsWith('*'))) {
        s = s.mid(1).trimmed();
    }
    while (s.endsWith('.')) {
        s = s.left(s.length() - 1).trimmed();
    }
    if (s.isEmpty()) return b;

    QRegExp fullChapterRe("^\\s*((?:Chương|Chapter|Hồi|Phần|Quyển|Tập|Mục|Tiết|Bài|Book|Section)\\s+[0-9IVXLCDM]+)\\s*[:\\-–—.]\\s*(.+)$", Qt::CaseInsensitive);
    if (fullChapterRe.indexIn(s) != -1) {
        return QString("%1: %2").arg(fullChapterRe.cap(1).trimmed(), fullChapterRe.cap(2).trimmed());
    }

    if (isGenericTitle(s)) {
        if (!isGenericTitle(b)) return b;
        return s.isEmpty() ? b : s;
    }

    if (b.isEmpty()) {
        if (chapterNumber > 0) {
            return QString("Chương %1: %2").arg(chapterNumber).arg(s);
        }
        return s;
    }

    if (!isGenericTitle(b)) {
        return b;
    }

    while (!b.isEmpty() && (b.endsWith(':') || b.endsWith('-') || b.endsWith(QChar(0x2014)) || 
           b.endsWith(QChar(0x2013)) || b.endsWith('.') || b.endsWith(' '))) {
        b = b.left(b.length() - 1).trimmed();
    }

    QRegExp justNumRe("^#?\\s*([0-9]+|[IVXLCDM]+)$", Qt::CaseInsensitive);
    if (justNumRe.exactMatch(b)) {
        return QString("Chương %1: %2").arg(b, s);
    }

    return QString("%1: %2").arg(b, s);
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
    QTextCodec::ConverterState state;
    QString text = utf8Codec->toUnicode(data.constData(), data.size(), &state);

    if (state.invalidChars == 0) {
        return text;
    }

    // Fallback: try Windows-1258 (Vietnamese) or Windows-1252 (Western)
    QTextCodec *win1258 = QTextCodec::codecForName("Windows-1258");
    if (win1258) {
        return win1258->toUnicode(data);
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
    QRegExp chapterRe("^\\s*(Chương\\s+[0-9IVXLCDM]+|Chapter\\s+[0-9IVXLCDM]+|Hồi\\s+[0-9IVXLCDM]+|Phần\\s+[0-9IVXLCDM]+|Quyển\\s+[0-9IVXLCDM]+|Tập\\s+[0-9IVXLCDM]+|Mục\\s+[0-9IVXLCDM]+|Bài\\s+[0-9IVXLCDM]+|Tiết\\s+[0-9IVXLCDM]+|Book\\s+[0-9IVXLCDM]+|Section\\s+[0-9IVXLCDM]+|Lời\\s+nói\\s+đầu|Mở\\s+đầu|Vĩ\\s+thanh|Ngoại\\s+truyện|Kết\\s+thúc|Prologue|Epilogue|[0-9]+\\.|I\\.|II\\.|III\\.|IV\\.|V\\.)", Qt::CaseInsensitive);

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
            QString coverSrc = QString("file://%1").arg(m_coverPath);
            QFile f(m_coverPath);
            if (f.open(QIODevice::ReadOnly)) {
                QByteArray b = f.readAll();
                f.close();
                QString ext = QFileInfo(m_coverPath).suffix().toLower();
                QString mime = (ext == "png") ? "image/png" : "image/jpeg";
                coverSrc = QString("data:%1;base64,%2").arg(mime).arg(QString::fromLatin1(b.toBase64().constData()));
            }
            html += QString("<img class=\"cover-img\" src=\"%1\" alt=\"Cover\"/>").arg(coverSrc);
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

    QString html;
    html += "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/>";
    html += "<style>";
    html += "body { font-family: -apple-system, sans-serif; line-height: 1.6; padding: 12px; margin: 0; word-wrap: break-word; }";
    html += "h2.txt-chapter-title { text-align: center; margin: 1.2em 0 1em 0; font-size: 1.3em; border-bottom: 1px solid rgba(128,128,128,0.2); padding-bottom: 0.5em; }";
    html += "p { text-indent: 1.8em; margin: 0.6em 0; }";
    html += "</style></head><body>";

    html += QString("<h2 class=\"txt-chapter-title\">%1</h2>").arg(escapeHtml(chTitle));

    QStringList lines = rawText.split('\n');
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
