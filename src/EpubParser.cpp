#include "EpubParser.hpp"
#include "RegexUtil.hpp"
#include "ImageUriUtil.hpp"
#include "miniz.h"
#include "pugixml.hpp"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QMap>
#include <QUrl>
#include <QRegExp>
#include <QSet>
#include <QCryptographicHash>

EpubParser::EpubParser()
    : m_seriesIndex(0)
    , m_zipOpen(false)
{
    memset(&m_zip, 0, sizeof(m_zip));
}

EpubParser::~EpubParser()
{
    close();
}

void EpubParser::close()
{
    if (m_zipOpen) {
        mz_zip_reader_end(&m_zip);
        m_zipOpen = false;
    }
    if (m_zipFile.isOpen()) {
        m_zipFile.close();
    }
    memset(&m_zip, 0, sizeof(m_zip));
    m_zipIndexMap.clear();
    m_chapterContentCache.clear();
    m_chapterCacheLru.clear();
    m_filePath.clear();
    m_title.clear();
    m_author.clear();
    m_series.clear();
    m_seriesIndex = 0;
    m_description.clear();
    m_genre.clear();
    m_coverHref.clear();
    m_coverPath.clear();
    m_opfDir.clear();
    m_chapters.clear();
    m_tocEntries.clear();
    m_cssFiles.clear();
    m_resolvedBookCss.clear();
}

static size_t qfile_zip_read_cb(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n)
{
    QFile *f = static_cast<QFile*>(pOpaque);
    if (!f || !f->isOpen()) return 0;
    if (!f->seek(file_ofs)) return 0;
    qint64 bytes = f->read(static_cast<char*>(pBuf), static_cast<qint64>(n));
    return (bytes > 0) ? static_cast<size_t>(bytes) : 0;
}

int EpubParser::findZipEntry(const QString &entryName)
{
    if (!m_zipOpen || entryName.isEmpty()) return -1;

    QString norm = entryName;
    norm.replace('\\', '/');
    while (norm.startsWith('/')) norm.remove(0, 1);
    while (norm.startsWith("./")) norm.remove(0, 2);

    if (m_zipIndexMap.contains(norm)) {
        return (int)m_zipIndexMap.value(norm);
    }
    QString lower = norm.toLower();
    if (m_zipIndexMap.contains(lower)) {
        return (int)m_zipIndexMap.value(lower);
    }
    QString base = QFileInfo(norm).fileName().toLower();
    if (m_zipIndexMap.contains(base)) {
        return (int)m_zipIndexMap.value(base);
    }
    return -1;
}

QByteArray EpubParser::readZipEntry(const QString &entryName)
{
    int file_index = findZipEntry(entryName);
    if (file_index < 0) {
        return QByteArray();
    }

    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(&m_zip, (mz_uint)file_index, &stat)) {
        return QByteArray();
    }
    static const mz_uint64 MAX_ENTRY_SIZE = 64ULL * 1024ULL * 1024ULL;
    if (stat.m_uncomp_size > MAX_ENTRY_SIZE) {
        qWarning() << "EpubParser: zip entry too large:" << entryName << stat.m_uncomp_size;
        return QByteArray();
    }

    size_t uncompressed_size = 0;
    void *pData = mz_zip_reader_extract_to_heap(&m_zip, (mz_uint)file_index, &uncompressed_size, 0);
    if (!pData) {
        return QByteArray();
    }
    if (uncompressed_size > MAX_ENTRY_SIZE) {
        mz_free(pData);
        return QByteArray();
    }

    QByteArray result((const char*)pData, (int)uncompressed_size);
    mz_free(pData);
    return result;
}

static QString cleanHtmlEntities(QString text)
{
    text.replace("&nbsp;", " ");
    text.replace("&amp;", "&");
    text.replace("&quot;", "\"");
    text.replace("&apos;", "'");
    text.replace("&#39;", "'");
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");
    text.replace("&#160;", " ");
    text.replace(QChar(0x00A0), ' ');
    text.replace(QRegExp("\\s+"), " ");
    return text.trimmed();
}

static QString extractAllNodeText(const pugi::xml_node &node)
{
    QString result;
    for (pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it) {
        if (it->type() == pugi::node_pcdata || it->type() == pugi::node_cdata) {
            result += QString::fromUtf8(it->value()) + " ";
        } else if (it->type() == pugi::node_element) {
            result += extractAllNodeText(*it) + " ";
        }
    }
    return cleanHtmlEntities(result);
}

static bool isGenericTitle(const QString &title)
{
    QString t = title.trimmed();
    if (t.isEmpty()) return true;

    // Matches file names like "chap1.xhtml", "chapter_01.html", "split_002.html", "text/part001.html"
    if (t.endsWith(".xhtml", Qt::CaseInsensitive) || t.endsWith(".html", Qt::CaseInsensitive) ||
        t.endsWith(".htm", Qt::CaseInsensitive) || t.contains(QRegExp("^(?:text\\/)?part[0-9]+", Qt::CaseInsensitive))) {
        return true;
    }

    // Matches code-like IDs: ch01, chap_01, sec-1, part2
    QRegExp chCodeRe("^(?:ch|chap|chapter|sec|section|part)[_-]?[0-9]+$", Qt::CaseInsensitive);
    if (chCodeRe.exactMatch(t)) return true;

    // Matches just numbers, hashes, or roman numerals: "1", "01", "1.", "1: ", "#1", "#01", "IV."
    QRegExp justNumRe("^#?\\s*([0-9]+|[IVXLCDM]+)\\s*[:.-]?\\s*$", Qt::CaseInsensitive);
    if (justNumRe.exactMatch(t)) return true;

    // Matches generic "Chương 1", "Chương 01", "Chương 1.", "Chương 1:", "Chương I", "Chapter 1", "Hồi 1", "Phần 1", "Tập 1", "Quyển 1", "Mục 1", "Tiết 1", "Bài 1", "Book 1", "Section 1"
    QRegExp genericRe("^\\s*(Chương|Chapter|Phần|Part|Hồi|Quyển|Tập|Mục|Tiết|Bài|Book|Section)\\s+([0-9]+|[IVXLCDM]+)\\s*[:.-]?\\s*$", Qt::CaseInsensitive);
    if (genericRe.exactMatch(t)) return true;

    // Matches redundant patterns like "Chương 1 - Chương 1", "Chương 1 : Chương 1", "Chương 1 - 1"
    QRegExp dupRe("^\\s*(?:Chương|Chapter)\\s+([0-9]+|[IVXLCDM]+)\\s*[:\\-–—]\\s*(?:(?:Chương|Chapter)\\s+)?\\1\\s*$", Qt::CaseInsensitive);
    if (dupRe.exactMatch(t)) return true;

    // Matches just the single keyword by itself
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

    // Clean leading punctuation from subTitle
    while (!s.isEmpty() && (s.startsWith('-') || s.startsWith(QChar(0x2014)) || 
           s.startsWith(QChar(0x2013)) || s.startsWith(':') || s.startsWith(' ') ||
           s.startsWith(QChar(0x2022)) || s.startsWith('*'))) {
        s = s.mid(1).trimmed();
    }
    while (s.endsWith('.')) {
        s = s.left(s.length() - 1).trimmed();
    }
    if (s.isEmpty()) return b;

    // Check if subTitle already has a chapter prefix: e.g. "Chương 1: ..." or "Chapter 2 - ..."
    QRegExp fullChapterRe("^\\s*((?:Chương|Chapter|Hồi|Phần|Quyển|Tập|Mục|Tiết|Bài|Book|Section)\\s+[0-9IVXLCDM]+)\\s*[:\\-–—.]\\s*(.+)$", Qt::CaseInsensitive);
    if (fullChapterRe.indexIn(s) != -1) {
        return QString("%1: %2").arg(fullChapterRe.cap(1).trimmed(), fullChapterRe.cap(2).trimmed());
    }

    // If subTitle is itself generic, do not combine
    if (isGenericTitle(s)) {
        if (!isGenericTitle(b)) return b;
        return s.isEmpty() ? b : s;
    }

    // Check if b is a file name like chap01.xhtml or part001.html
    bool bIsFileName = (b.endsWith(".xhtml", Qt::CaseInsensitive) || b.endsWith(".html", Qt::CaseInsensitive) ||
                        b.endsWith(".htm", Qt::CaseInsensitive) || b.contains(QRegExp("^(?:text\\/)?part[0-9]+", Qt::CaseInsensitive)));

    if (b.isEmpty() || bIsFileName) {
        if (chapterNumber > 0) {
            return QString("Chương %1: %2").arg(chapterNumber).arg(s);
        }
        return s;
    }

    // If b is already a complete non-generic title
    if (!isGenericTitle(b)) {
        return b;
    }

    // b is generic (e.g. "Chương 1" or "1"):
    while (!b.isEmpty() && (b.endsWith(':') || b.endsWith('-') || b.endsWith(QChar(0x2014)) || 
           b.endsWith(QChar(0x2013)) || b.endsWith('.') || b.endsWith(' '))) {
        b = b.left(b.length() - 1).trimmed();
    }

    // If b is just a number
    QRegExp justNumRe("^#?\\s*([0-9]+|[IVXLCDM]+)$", Qt::CaseInsensitive);
    if (justNumRe.exactMatch(b)) {
        return QString("Chương %1: %2").arg(b, s);
    }

    // If b starts with a chapter keyword (e.g. "Chương 1")
    return QString("%1: %2").arg(b, s);
}

static QString extractTitleFromHtml(const QString &rawHtml, const QString &bookTitle = QString(), const QString &anchor = QString())
{
    QString workHtml = rawHtml;

    // If anchor is specified, focus from the anchor occurrence
    if (!anchor.isEmpty()) {
        int aIdx = workHtml.indexOf(anchor, 0, Qt::CaseInsensitive);
        if (aIdx != -1) {
            workHtml = workHtml.mid(aIdx, 32768);
        }
    }

    // Strip scripts, styles, comments to prevent matching CSS styles or script code
    workHtml.remove(minimalRx("<style[^>]*>[\\s\\S]*<\\/style>", Qt::CaseInsensitive));
    workHtml.remove(minimalRx("<script[^>]*>[\\s\\S]*<\\/script>", Qt::CaseInsensitive));
    workHtml.remove(minimalRx("<!--[\\s\\S]*-->"));

    QString detectedChapterPrefix;

    // Prepare line extraction: replace line-breaking tags with newline
    QString lineHtml = workHtml;
    lineHtml.replace(QRegExp("<br\\s*\\/?>", Qt::CaseInsensitive), "\n");
    lineHtml.replace(QRegExp("<\\/(?:p|div|h[1-6]|li|tr|blockquote|section|header)>", Qt::CaseInsensitive), "\n");
    lineHtml.replace(QRegExp("<(?:hr)[^>]*>", Qt::CaseInsensitive), "\n");
    lineHtml.remove(QRegExp("<[^>]*>"));
    lineHtml = cleanHtmlEntities(lineHtml);

    QStringList lines = lineHtml.split('\n');
    QStringList cleanLines;
    for (int i = 0; i < lines.size(); ++i) {
        QString l = lines[i].trimmed();
        if (!l.isEmpty()) {
            cleanLines.append(l);
            if (cleanLines.size() >= 30) break;
        }
    }

    // Regex to match chapter headings at line start
    QRegExp chapterRe("^\\s*(Chương\\s+[0-9IVXLCDM]+|Chapter\\s+[0-9IVXLCDM]+|Hồi\\s+[0-9IVXLCDM]+|Phần\\s+[0-9IVXLCDM]+|Quyển\\s+[0-9IVXLCDM]+|Tập\\s+[0-9IVXLCDM]+|Mục\\s+[0-9IVXLCDM]+|Bài\\s+[0-9IVXLCDM]+|Tiết\\s+[0-9IVXLCDM]+|Book\\s+[0-9IVXLCDM]+|Section\\s+[0-9IVXLCDM]+|Lời\\s+nói\\s+đầu|Lời\\s+mở\\s+đầu|Mở\\s+đầu|Vĩ\\s+thanh|Ngoại\\s+truyện|Kết\\s+thúc|Prologue|Epilogue|[0-9]+\\.|[IVXLCDM]+\\.)", Qt::CaseInsensitive);

    // Pass 1: Scan cleanLines for chapter heading and subtitle
    for (int i = 0; i < cleanLines.size() && i < 15; ++i) {
        QString line = cleanLines[i];
        if (!bookTitle.isEmpty() && line.compare(bookTitle, Qt::CaseInsensitive) == 0) continue;

        if (chapterRe.indexIn(line) != -1) {
            // Found a chapter line!
            // Check if this line already contains a descriptive subtitle on the same line
            if (!isGenericTitle(line)) {
                line.replace(QRegExp("^\\s*((?:Chương|Chapter|Hồi|Phần|Quyển|Tập|Mục|Tiết|Bài|Book|Section)\\s+[0-9IVXLCDM]+)\\s*[:\\-–—.]\\s*(.+)$", Qt::CaseInsensitive), "\\1: \\2");
                return line;
            }

            // Line is generic (e.g. "Chương 1"). Remember as prefix.
            detectedChapterPrefix = line;

            // Look for subtitle in subsequent lines (up to 6 lines ahead)
            for (int next = i + 1; next < cleanLines.size() && next <= i + 6; ++next) {
                QString nextLine = cleanLines[next];
                if (chapterRe.indexIn(nextLine) != -1) break;
                if (!bookTitle.isEmpty() && nextLine.compare(bookTitle, Qt::CaseInsensitive) == 0) continue;

                if (isSeparatorOrCreditLine(nextLine)) continue;

                // Clean leading hyphens, dashes, colons from subtitle
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

                if (!sub.isEmpty() && sub.length() >= 2 && sub.length() < 120 && !isGenericTitle(sub)) {
                    while (line.endsWith(':') || line.endsWith('-') || line.endsWith('.') || line.endsWith(' ')) {
                        line = line.left(line.length() - 1).trimmed();
                    }
                    return line + ": " + sub;
                }
            }
            break;
        }
    }

    // Secondary pass: Check explicit <h1-h4> tags
    QRegExp hTagRe = minimalRx("<(h[1-4])[^>]*>([\\s\\S]*)<\\/\\1>", Qt::CaseInsensitive);
    int hPos = 0;
    QString firstHGeneric;
    while ((hPos = hTagRe.indexIn(workHtml, hPos)) != -1) {
        QString hContent = hTagRe.cap(2);
        hContent.replace(QRegExp("<br\\s*\\/?>", Qt::CaseInsensitive), " : ");
        hContent.remove(QRegExp("<[^>]*>"));
        hContent = cleanHtmlEntities(hContent);
        hPos += hTagRe.matchedLength();

        if (hContent.isEmpty() || (!bookTitle.isEmpty() && hContent.compare(bookTitle, Qt::CaseInsensitive) == 0)) {
            continue;
        }

        if (!isGenericTitle(hContent) && hContent.length() < 120) {
            if (!firstHGeneric.isEmpty()) {
                return combineChapterTitle(firstHGeneric, hContent);
            }
            if (!detectedChapterPrefix.isEmpty()) {
                return combineChapterTitle(detectedChapterPrefix, hContent);
            }
            return hContent;
        } else if (firstHGeneric.isEmpty() && (chapterRe.indexIn(hContent) != -1 || isGenericTitle(hContent))) {
            firstHGeneric = hContent;
            if (detectedChapterPrefix.isEmpty()) {
                detectedChapterPrefix = hContent;
            }
        }
    }

    // Tertiary pass: Check class="...title..."
    QRegExp classTitleRe = minimalRx("<(?:div|p|span|h[1-6])[^>]*class=[\"'][^\"']*(?:chapter-title|chap-title|chapter-name|chap_title|book-title|c-title|title)[^\"']*[\"'][^>]*>([\\s\\S]*)<\\/(?:div|p|span|h[1-6])>", Qt::CaseInsensitive);
    if (classTitleRe.indexIn(workHtml) != -1) {
        QString ct = classTitleRe.cap(1);
        ct.replace(QRegExp("<br\\s*\\/?>", Qt::CaseInsensitive), " : ");
        ct.remove(QRegExp("<[^>]*>"));
        ct = cleanHtmlEntities(ct);
        if (!ct.isEmpty() && ct.length() >= 2 && ct.length() < 120 &&
            (!bookTitle.isEmpty() ? ct.compare(bookTitle, Qt::CaseInsensitive) != 0 : true)) {
            if (!isGenericTitle(ct)) {
                if (!detectedChapterPrefix.isEmpty()) {
                    return combineChapterTitle(detectedChapterPrefix, ct);
                }
                return ct;
            } else if (detectedChapterPrefix.isEmpty()) {
                detectedChapterPrefix = ct;
            }
        }
    }

    // Quaternary pass: Check bold/strong paragraph
    QRegExp boldPRe = minimalRx("<p[^>]*>\\s*(?:<b[^>]*>|<strong[^>]*>|)(?:<span[^>]*class=[\"'][^\"']*(?:bold|title)[^\"']*[\"'][^>]*>)?([\\s\\S]*)(?:<\\/span>)?(?:<\\/b>|<\\/strong>|)\\s*<\\/p>", Qt::CaseInsensitive);
    if (boldPRe.indexIn(workHtml) != -1) {
        QString bp = boldPRe.cap(1);
        bp.remove(QRegExp("<[^>]*>"));
        bp = cleanHtmlEntities(bp);
        if (!bp.isEmpty() && bp.length() >= 2 && bp.length() < 120 &&
            (!bookTitle.isEmpty() ? bp.compare(bookTitle, Qt::CaseInsensitive) != 0 : true)) {
            if (!isGenericTitle(bp)) {
                if (!detectedChapterPrefix.isEmpty()) {
                    return combineChapterTitle(detectedChapterPrefix, bp);
                }
                return bp;
            } else if (detectedChapterPrefix.isEmpty()) {
                detectedChapterPrefix = bp;
            }
        }
    }

    // Quinary pass: Check <title>...</title> in HTML head
    QRegExp titleRe = minimalRx("<title[^>]*>([\\s\\S]*)<\\/title>", Qt::CaseInsensitive);
    if (titleRe.indexIn(workHtml) != -1) {
        QString rawT = titleRe.cap(1);
        rawT.remove(QRegExp("<[^>]*>"));
        rawT = cleanHtmlEntities(rawT);

        QStringList parts;
        if (rawT.contains(" - ")) parts = rawT.split(" - ");
        else if (rawT.contains(" | ")) parts = rawT.split(" | ");
        else if (rawT.contains(QChar(0x2014))) parts = rawT.split(QChar(0x2014));
        else if (rawT.contains(QChar(0x2013))) parts = rawT.split(QChar(0x2013));
        else parts.append(rawT);

        for (int p = 0; p < parts.size(); ++p) {
            QString part = parts[p].trimmed();
            if (part.isEmpty()) continue;
            if (!bookTitle.isEmpty() && part.compare(bookTitle, Qt::CaseInsensitive) == 0) continue;

            if (!isGenericTitle(part) && part.length() >= 2 && part.length() < 120) {
                if (!detectedChapterPrefix.isEmpty()) {
                    return combineChapterTitle(detectedChapterPrefix, part);
                }
                return part;
            } else if (chapterRe.indexIn(part) != -1 && detectedChapterPrefix.isEmpty()) {
                detectedChapterPrefix = part;
            }
        }
    }

    if (!detectedChapterPrefix.isEmpty() && !isGenericTitle(detectedChapterPrefix)) {
        return detectedChapterPrefix;
    }

    return QString();
}

static pugi::xml_node findDirectAnchor(const pugi::xml_node &li)
{
    pugi::xml_node aNode = li.child("a");
    if (aNode) return aNode;
    for (pugi::xml_node c = li.first_child(); c; c = c.next_sibling()) {
        if (strcmp(c.name(), "ol") == 0 || strcmp(c.name(), "ul") == 0) continue;
        if (strcmp(c.name(), "a") == 0) return c;
        pugi::xml_node subA = c.child("a");
        if (subA) return subA;
    }
    return pugi::xml_node();
}

static void parseNcxNavPoint(const pugi::xml_node &nav, int level, const QString &ncxDir,
                             const QHash<QString, int> &hrefMap, const QHash<QString, int> &baseMap,
                             QList<EpubTocEntry> &outToc, QSet<int> &titledChapters,
                             QList<EpubChapter> &chapters)
{
    QString navLabel;
    pugi::xml_node textNode = nav.child("navLabel").child("text");
    if (textNode) navLabel = extractAllNodeText(textNode);
    if (navLabel.isEmpty()) {
        pugi::xml_node nlNode = nav.child("navLabel");
        if (nlNode) navLabel = extractAllNodeText(nlNode);
    }
    if (navLabel.isEmpty()) {
        navLabel = QString::fromUtf8(nav.child("navLabel").text().get()).trimmed();
        navLabel = cleanHtmlEntities(navLabel);
    }

    QString rawSrc = QString::fromUtf8(nav.child("content").attribute("src").value()).trimmed();
    if (rawSrc.isEmpty()) {
        rawSrc = QString::fromUtf8(nav.select_node("content").node().attribute("src").value()).trimmed();
    }

    int targetIdx = -1;
    QString anchor;
    QString cleanPath;

    if (!rawSrc.isEmpty()) {
        QString decodedSrc = QString::fromUtf8(QUrl::fromPercentEncoding(rawSrc.toUtf8()).toUtf8());
        decodedSrc.replace('\\', '/');
        int hashIdx = decodedSrc.indexOf('#');
        if (hashIdx >= 0) {
            cleanPath = decodedSrc.left(hashIdx);
            anchor = decodedSrc.mid(hashIdx + 1);
        } else {
            cleanPath = decodedSrc;
        }

        while (cleanPath.startsWith("./")) cleanPath.remove(0, 2);
        QString fullSrc = QDir::cleanPath(ncxDir + cleanPath);
        while (fullSrc.startsWith('/')) fullSrc.remove(0, 1);
        QString navBase = QFileInfo(cleanPath).fileName().toLower();

        targetIdx = hrefMap.value(fullSrc, -1);
        if (targetIdx == -1) targetIdx = hrefMap.value(fullSrc.toLower(), -1);
        if (targetIdx == -1) targetIdx = hrefMap.value(cleanPath, -1);
        if (targetIdx == -1) targetIdx = hrefMap.value(cleanPath.toLower(), -1);
        if (targetIdx == -1) targetIdx = baseMap.value(navBase, -1);
    }

    if (targetIdx >= 0 && targetIdx < chapters.size()) {
        if (navLabel.isEmpty()) {
            navLabel = chapters[targetIdx].title;
        }
        if (targetIdx == 0 && (navLabel.toLower() == "cover" || navLabel.toLower() == "bìa")) {
            navLabel = "Bìa sách";
        }

        EpubTocEntry entry;
        entry.title = navLabel;
        entry.chapterIndex = targetIdx;
        entry.anchor = anchor;
        entry.level = level;
        entry.href = cleanPath;
        outToc.append(entry);

        if (!titledChapters.contains(targetIdx) || (isGenericTitle(chapters[targetIdx].title) && !isGenericTitle(navLabel))) {
            titledChapters.insert(targetIdx);
            chapters[targetIdx].title = combineChapterTitle(chapters[targetIdx].title, navLabel);
        }
    }

    for (pugi::xml_node child = nav.child("navPoint"); child; child = child.next_sibling("navPoint")) {
        parseNcxNavPoint(child, level + 1, ncxDir, hrefMap, baseMap, outToc, titledChapters, chapters);
    }
}

static void parseNavList(const pugi::xml_node &listNode, int level, const QString &navDir,
                         const QHash<QString, int> &hrefMap, const QHash<QString, int> &baseMap,
                         QList<EpubTocEntry> &outToc, QSet<int> &titledChapters,
                         QList<EpubChapter> &chapters)
{
    if (!listNode) return;

    for (pugi::xml_node li = listNode.child("li"); li; li = li.next_sibling("li")) {
        pugi::xml_node aNode = findDirectAnchor(li);

        if (aNode) {
            QString aHref = QString::fromUtf8(aNode.attribute("href").value()).trimmed();
            QString aText = extractAllNodeText(aNode);
            if (aText.isEmpty()) {
                aText = extractAllNodeText(li);
            }

            int targetIdx = -1;
            QString anchor;
            QString cleanPath;

            if (!aHref.isEmpty()) {
                QString decodedHref = QString::fromUtf8(QUrl::fromPercentEncoding(aHref.toUtf8()).toUtf8());
                decodedHref.replace('\\', '/');
                int hashIdx = decodedHref.indexOf('#');
                if (hashIdx >= 0) {
                    cleanPath = decodedHref.left(hashIdx);
                    anchor = decodedHref.mid(hashIdx + 1);
                } else {
                    cleanPath = decodedHref;
                }

                while (cleanPath.startsWith("./")) cleanPath.remove(0, 2);
                QString fullHref = QDir::cleanPath(navDir + cleanPath);
                while (fullHref.startsWith('/')) fullHref.remove(0, 1);
                QString aBase = QFileInfo(cleanPath).fileName().toLower();

                targetIdx = hrefMap.value(fullHref, -1);
                if (targetIdx == -1) targetIdx = hrefMap.value(fullHref.toLower(), -1);
                if (targetIdx == -1) targetIdx = hrefMap.value(cleanPath, -1);
                if (targetIdx == -1) targetIdx = hrefMap.value(cleanPath.toLower(), -1);
                if (targetIdx == -1) targetIdx = baseMap.value(aBase, -1);
            }

            if (targetIdx >= 0 && targetIdx < chapters.size()) {
                if (aText.isEmpty()) {
                    aText = chapters[targetIdx].title;
                }
                if (targetIdx == 0 && (aText.toLower() == "cover" || aText.toLower() == "bìa")) {
                    aText = "Bìa sách";
                }

                EpubTocEntry entry;
                entry.title = aText;
                entry.chapterIndex = targetIdx;
                entry.anchor = anchor;
                entry.level = level;
                entry.href = cleanPath;
                outToc.append(entry);

                if (!titledChapters.contains(targetIdx) || (isGenericTitle(chapters[targetIdx].title) && !isGenericTitle(aText))) {
                    titledChapters.insert(targetIdx);
                    chapters[targetIdx].title = combineChapterTitle(chapters[targetIdx].title, aText);
                }
            }
        }

        pugi::xml_node subList = li.child("ol");
        if (!subList) subList = li.child("ul");
        if (subList) {
            parseNavList(subList, level + 1, navDir, hrefMap, baseMap, outToc, titledChapters, chapters);
        }
    }
}

bool EpubParser::open(const QString &filePath)
{
    close();
    m_filePath = filePath;

    // Fast stdio fopen directly using UTF-8 path
    m_zipOpen = mz_zip_reader_init_file(&m_zip, m_filePath.toUtf8().constData(), 0);
    if (!m_zipOpen) {
        // Fallback to persistent QFile callback if direct C fopen failed
        m_zipFile.setFileName(m_filePath);
        if (m_zipFile.open(QIODevice::ReadOnly)) {
            m_zip.m_pRead = qfile_zip_read_cb;
            m_zip.m_pIO_opaque = &m_zipFile;
            m_zipOpen = mz_zip_reader_init(&m_zip, m_zipFile.size(), 0);
        }
    }

    if (!m_zipOpen) {
        qWarning() << "[EpubParser] Failed to open zip archive:" << m_filePath
                   << "miniz error:" << mz_zip_get_last_error(&m_zip);
        return false;
    }

    // Build O(1) in-memory index for all entries in the archive
    mz_uint total = mz_zip_reader_get_num_files(&m_zip);
    m_zipIndexMap.reserve(total * 2);
    for (mz_uint i = 0; i < total; ++i) {
        mz_zip_archive_file_stat stat;
        if (mz_zip_reader_file_stat(&m_zip, i, &stat)) {
            QString fn = QString::fromUtf8(stat.m_filename);
            fn.replace('\\', '/');
            while (fn.startsWith('/')) fn.remove(0, 1);
            while (fn.startsWith("./")) fn.remove(0, 2);

            m_zipIndexMap.insert(fn, i);
            m_zipIndexMap.insert(fn.toLower(), i);

            QString base = QFileInfo(fn).fileName().toLower();
            if (!m_zipIndexMap.contains(base)) {
                m_zipIndexMap.insert(base, i);
            }
        }
    }

    // 1. Read META-INF/container.xml
    QByteArray containerData = readZipEntry("META-INF/container.xml");
    if (containerData.isEmpty()) {
        qWarning() << "Failed to find META-INF/container.xml in" << filePath;
        close();
        return false;
    }

    pugi::xml_document docContainer;
    if (!docContainer.load_buffer(containerData.constData(), containerData.size())) {
        qWarning() << "Failed to parse META-INF/container.xml";
        close();
        return false;
    }

    pugi::xml_node rootfileNode = docContainer.select_node("//rootfile").node();
    QString opfPath = QString::fromUtf8(rootfileNode.attribute("full-path").value());
    if (opfPath.isEmpty()) {
        qWarning() << "No full-path in rootfile";
        close();
        return false;
    }

    QFileInfo opfInfo(opfPath);
    m_opfDir = opfInfo.path();
    if (m_opfDir == ".") {
        m_opfDir = "";
    } else if (!m_opfDir.endsWith('/')) {
        m_opfDir += "/";
    }

    // 2. Read OPF file
    QByteArray opfData = readZipEntry(opfPath);
    if (opfData.isEmpty()) {
        qWarning() << "Failed to read OPF file:" << opfPath;
        close();
        return false;
    }

    pugi::xml_document docOpf;
    if (!docOpf.load_buffer(opfData.constData(), opfData.size())) {
        qWarning() << "Failed to parse OPF file";
        close();
        return false;
    }

    // Metadata
    pugi::xml_node titleNode = docOpf.select_node("//dc:title").node();
    if (titleNode) {
        m_title = QString::fromUtf8(titleNode.child_value());
    } else {
        m_title = opfInfo.baseName();
    }

    pugi::xml_node creatorNode = docOpf.select_node("//dc:creator").node();
    if (creatorNode) {
        m_author = QString::fromUtf8(creatorNode.child_value());
    } else {
        m_author = "Unknown Author";
    }

    pugi::xml_node descNode = docOpf.select_node("//dc:description").node();
    if (!descNode) descNode = docOpf.select_node("//description").node();
    if (descNode) {
        QString rawDesc;
        if (descNode.first_child()) {
            struct xml_string_writer : public pugi::xml_writer {
                std::string result;
                virtual void write(const void* data, size_t size) {
                    result.append(static_cast<const char*>(data), size);
                }
            } writer;
            for (pugi::xml_node child = descNode.first_child(); child; child = child.next_sibling()) {
                child.print(writer, "", pugi::format_raw);
            }
            rawDesc = QString::fromUtf8(writer.result.c_str()).trimmed();
        } else {
            rawDesc = QString::fromUtf8(descNode.child_value()).trimmed();
        }

        if (rawDesc.isEmpty()) {
            rawDesc = QString::fromUtf8(descNode.text().get()).trimmed();
        }

        // Entities may be escaped markup (&lt;p&gt;): decode first, then strip tags (2 passes)
        for (int pass = 0; pass < 2; ++pass) {
            rawDesc.replace("&quot;", "\"").replace("&lt;", "<").replace("&gt;", ">").replace("&#39;", "'").replace("&apos;", "'").replace("&nbsp;", " ").replace("&amp;", "&");
            rawDesc.replace(QRegExp("<br\\s*/?>", Qt::CaseInsensitive), "\n");
            rawDesc.replace(QRegExp("</p>", Qt::CaseInsensitive), "\n\n");
            rawDesc.replace(QRegExp("</div>", Qt::CaseInsensitive), "\n");
            rawDesc.replace(QRegExp("</li>", Qt::CaseInsensitive), "\n");
            rawDesc.replace(QRegExp("<li[^>]*>", Qt::CaseInsensitive), QString(QChar(0x2022)) + " ");
            rawDesc.remove(QRegExp("<[^>]*>"));
        }
        rawDesc.replace(QRegExp("[ \t\r]+"), " ");
        rawDesc.replace(QRegExp("\n[ ]+"), "\n");
        rawDesc.replace(QRegExp("\n{3,}"), "\n\n");
        m_description = rawDesc.trimmed();
    }

    pugi::xpath_node_set subjectNodes = docOpf.select_nodes("//dc:subject");
    if (subjectNodes.empty()) subjectNodes = docOpf.select_nodes("//subject");
    QStringList subjects;
    for (size_t i = 0; i < subjectNodes.size(); ++i) {
        QString sub = QString::fromUtf8(subjectNodes[i].node().child_value()).trimmed();
        if (sub.isEmpty()) sub = QString::fromUtf8(subjectNodes[i].node().text().get()).trimmed();
        if (sub.isEmpty()) continue;
        QString subLower = sub.toLower();
        // Filter out URLs, converter watermarks, software names, and advertising spam
        if (subLower.contains("http://") || subLower.contains("https://") ||
            subLower.contains("www.") || subLower.contains(".com") ||
            subLower.contains(".net") || subLower.contains(".org") ||
            subLower.contains("converter") || subLower.contains("converted") ||
            subLower.contains("neevia") || subLower.contains("calibre") ||
            subLower.contains("batch") || subLower.contains("batches") ||
            subLower.contains("adobe") || subLower.contains("watermark")) {
            continue;
        }
        if (!subjects.contains(sub, Qt::CaseInsensitive)) {
            subjects.append(sub);
        }
    }
    m_genre = subjects.join(", ");

    // Series metadata (Calibre, EPUB 3, or generic meta tags)
    m_series.clear();
    m_seriesIndex = 0;
    pugi::xml_node seriesNode = docOpf.select_node("//meta[@name='calibre:series']").node();
    if (seriesNode) {
        m_series = QString::fromUtf8(seriesNode.attribute("content").value()).trimmed();
        pugi::xml_node idxNode = docOpf.select_node("//meta[@name='calibre:series_index']").node();
        if (idxNode) {
            m_seriesIndex = qRound(QString::fromUtf8(idxNode.attribute("content").value()).toDouble());
        }
    }
    if (m_series.isEmpty()) {
        pugi::xml_node collNode = docOpf.select_node("//meta[@property='belongs-to-collection']").node();
        if (collNode) {
            m_series = QString::fromUtf8(collNode.child_value()).trimmed();
            pugi::xml_node posNode = docOpf.select_node("//meta[@property='group-position']").node();
            if (posNode) {
                m_seriesIndex = qRound(QString::fromUtf8(posNode.child_value()).toDouble());
            }
        }
    }
    if (m_series.isEmpty()) {
        pugi::xml_node sNode = docOpf.select_node("//meta[@name='series']").node();
        if (sNode) {
            m_series = QString::fromUtf8(sNode.attribute("content").value()).trimmed();
            pugi::xml_node idxNode = docOpf.select_node("//meta[@name='series_index']").node();
            if (idxNode) {
                m_seriesIndex = qRound(QString::fromUtf8(idxNode.attribute("content").value()).toDouble());
            }
        }
    }

    // Manifest: map id -> href
    QMap<QString, QString> manifestHref;
    QString coverId;

    pugi::xpath_node_set items = docOpf.select_nodes("//manifest/item");
    for (size_t i = 0; i < items.size(); ++i) {
        pugi::xml_node item = items[i].node();
        QString id = QString::fromUtf8(item.attribute("id").value());
        QString href = QString::fromUtf8(item.attribute("href").value());
        QString properties = QString::fromUtf8(item.attribute("properties").value());

        manifestHref[id] = href;

        QString mediaType = QString::fromUtf8(item.attribute("media-type").value());
        if (mediaType == "text/css" || href.endsWith(".css", Qt::CaseInsensitive)) {
            QString cleanHref = QString::fromUtf8(QUrl::fromPercentEncoding(href.toUtf8()).toUtf8());
            cleanHref.replace('\\', '/');
            while (cleanHref.startsWith('/')) cleanHref.remove(0, 1);
            m_cssFiles.append(m_opfDir + cleanHref);
        }

        bool isImageItem = mediaType.startsWith("image/", Qt::CaseInsensitive);
        if (properties.contains("cover-image") ||
            (isImageItem && (id.toLower() == "cover" || id.toLower() == "cover-image"))) {
            coverId = id;
        }
    }

    // Check meta name="cover"
    pugi::xml_node coverMeta = docOpf.select_node("//meta[@name='cover']").node();
    if (coverMeta && coverId.isEmpty()) {
        coverId = QString::fromUtf8(coverMeta.attribute("content").value());
    }

    // Fallback 1: Search for any item with image media-type whose id or href has "cover"
    if (coverId.isEmpty() || !manifestHref.contains(coverId)) {
        for (size_t i = 0; i < items.size(); ++i) {
            pugi::xml_node item = items[i].node();
            QString mediaType = QString::fromUtf8(item.attribute("media-type").value());
            QString id = QString::fromUtf8(item.attribute("id").value());
            QString href = QString::fromUtf8(item.attribute("href").value());
            if (mediaType.startsWith("image/") && (id.contains("cover", Qt::CaseInsensitive) || href.contains("cover", Qt::CaseInsensitive))) {
                coverId = id;
                break;
            }
        }
    }

    // Fallback 2: First image in manifest
    if (coverId.isEmpty() || !manifestHref.contains(coverId)) {
        for (size_t i = 0; i < items.size(); ++i) {
            pugi::xml_node item = items[i].node();
            QString mediaType = QString::fromUtf8(item.attribute("media-type").value());
            QString id = QString::fromUtf8(item.attribute("id").value());
            if (mediaType.startsWith("image/")) {
                coverId = id;
                break;
            }
        }
    }

    if (!coverId.isEmpty() && manifestHref.contains(coverId)) {
        QString href = manifestHref[coverId];
        href = QString::fromUtf8(QUrl::fromPercentEncoding(href.toUtf8()).toUtf8());
        m_coverHref = m_opfDir + href;
    }

    // Spine: order of chapter IDs
    pugi::xpath_node_set spineItems = docOpf.select_nodes("//spine/itemref");
    for (size_t i = 0; i < spineItems.size(); ++i) {
        pugi::xml_node itemref = spineItems[i].node();
        QString idref = QString::fromUtf8(itemref.attribute("idref").value());
        if (manifestHref.contains(idref)) {
            QString rawHref = manifestHref[idref];
            rawHref = QString::fromUtf8(QUrl::fromPercentEncoding(rawHref.toUtf8()).toUtf8());
            rawHref.replace('\\', '/');
            while (rawHref.startsWith('/')) rawHref.remove(0, 1);
            while (rawHref.startsWith("./")) rawHref.remove(0, 2);

            EpubChapter ch;
            ch.id = idref;
            ch.href = m_opfDir + rawHref;
            ch.title = QString("Chương %1").arg(i + 1);
            m_chapters.append(ch);
        }
    }

    // Identify if the first spine chapter is already a cover page
    bool firstIsCover = false;
    if (!m_chapters.isEmpty()) {
        QString firstId = m_chapters[0].id.toLower();
        QString firstHref = m_chapters[0].href.toLower();
        QString firstTitle = m_chapters[0].title.toLower();
        if (firstId.contains("cover") || firstId.contains("titlepage") || firstId.contains("title_page") ||
            firstId.contains("title-page") || firstId.contains("jacket") || firstId.contains("biatruoc") ||
            firstHref.contains("cover") || firstHref.contains("titlepage") || firstHref.contains("title_page") ||
            firstHref.contains("title-page") || firstHref.contains("jacket") || firstHref.contains("biatruoc") ||
            firstHref.contains("cvi_") || firstTitle.contains("cover") || firstTitle.contains("bìa")) {
            firstIsCover = true;
        }
    }

    if (firstIsCover) {
        m_chapters[0].title = "Bìa sách";
        for (int i = 1; i < m_chapters.size(); ++i) {
            m_chapters[i].title = QString("Chương %1").arg(i);
        }
    } else {
        EpubChapter coverCh;
        coverCh.id = "libris_cover_page";
        coverCh.title = "Bìa sách";
        coverCh.href = "libris://cover";
        m_chapters.prepend(coverCh);
    }

    // 1. Try finding NCX file for human-readable chapter titles & TOC
    QString ncxHref;
    pugi::xml_node spineNode = docOpf.select_node("//spine").node();
    QString tocId = QString::fromUtf8(spineNode.attribute("toc").value());
    if (!tocId.isEmpty() && manifestHref.contains(tocId)) {
        QString cleanTocHref = QString::fromUtf8(QUrl::fromPercentEncoding(manifestHref[tocId].toUtf8()).toUtf8());
        cleanTocHref.replace('\\', '/');
        while (cleanTocHref.startsWith('/')) cleanTocHref.remove(0, 1);
        while (cleanTocHref.startsWith("./")) cleanTocHref.remove(0, 2);
        ncxHref = QDir::cleanPath(m_opfDir + cleanTocHref);
        while (ncxHref.startsWith('/')) ncxHref.remove(0, 1);
    } else if (manifestHref.contains("ncx")) {
        QString cleanTocHref = QString::fromUtf8(QUrl::fromPercentEncoding(manifestHref["ncx"].toUtf8()).toUtf8());
        cleanTocHref.replace('\\', '/');
        while (cleanTocHref.startsWith('/')) cleanTocHref.remove(0, 1);
        while (cleanTocHref.startsWith("./")) cleanTocHref.remove(0, 2);
        ncxHref = QDir::cleanPath(m_opfDir + cleanTocHref);
        while (ncxHref.startsWith('/')) ncxHref.remove(0, 1);
    } else {
        // Fallback: search manifest for NCX mime type or extension
        for (size_t i = 0; i < items.size(); ++i) {
            pugi::xml_node it = items[i].node();
            QString mt = QString::fromUtf8(it.attribute("media-type").value()).toLower();
            QString hr = QString::fromUtf8(it.attribute("href").value());
            QString id = QString::fromUtf8(it.attribute("id").value()).toLower();
            if (mt == "application/x-dtbncx+xml" || hr.endsWith(".ncx", Qt::CaseInsensitive) || id.contains("ncx")) {
                QString cleanHr = QString::fromUtf8(QUrl::fromPercentEncoding(hr.toUtf8()).toUtf8());
                cleanHr.replace('\\', '/');
                while (cleanHr.startsWith('/')) cleanHr.remove(0, 1);
                while (cleanHr.startsWith("./")) cleanHr.remove(0, 2);
                ncxHref = QDir::cleanPath(m_opfDir + cleanHr);
                while (ncxHref.startsWith('/')) ncxHref.remove(0, 1);
                break;
            }
        }
    }

    // Build fast lookup index for chapters to avoid O(N*M) linear scans
    QHash<QString, int> hrefMap;
    QHash<QString, int> baseMap;
    for (int j = 0; j < m_chapters.size(); ++j) {
        if (m_chapters[j].id == "libris_cover_page") continue;
        hrefMap.insert(m_chapters[j].href, j);
        hrefMap.insert(m_chapters[j].href.toLower(), j);
        if (!m_opfDir.isEmpty() && m_chapters[j].href.startsWith(m_opfDir)) {
            QString rel = m_chapters[j].href.mid(m_opfDir.length());
            hrefMap.insert(rel, j);
            hrefMap.insert(rel.toLower(), j);
        }
        QString b = QFileInfo(m_chapters[j].href).fileName().toLower();
        if (!baseMap.contains(b)) {
            baseMap.insert(b, j);
        }
    }

    QSet<int> titledChapters;
    QList<EpubTocEntry> ncxToc;
    QList<EpubTocEntry> navToc;

    if (!ncxHref.isEmpty() && readZipEntry(ncxHref).isEmpty()) {
        ncxHref.clear();
    }
    if (ncxHref.isEmpty()) {
        // Fallback: search zip index directly for any .ncx file
        foreach (const QString &key, m_zipIndexMap.keys()) {
            if (key.endsWith(".ncx", Qt::CaseInsensitive)) {
                ncxHref = key;
                break;
            }
        }
    }

    if (!ncxHref.isEmpty()) {
        QByteArray ncxData = readZipEntry(ncxHref);
        if (!ncxData.isEmpty()) {
            pugi::xml_document docNcx;
            if (docNcx.load_buffer(ncxData.constData(), ncxData.size())) {
                QString ncxDir = QFileInfo(ncxHref).path();
                if (ncxDir == ".") ncxDir = "";
                else if (!ncxDir.endsWith('/')) ncxDir += "/";

                pugi::xpath_node_set rootNavPoints = docNcx.select_nodes("//navMap/navPoint | //ncx/navPoint");
                if (rootNavPoints.empty()) {
                    rootNavPoints = docNcx.select_nodes("//navPoint[not(parent::navPoint)]");
                }
                for (size_t i = 0; i < rootNavPoints.size(); ++i) {
                    parseNcxNavPoint(rootNavPoints[i].node(), 0, ncxDir, hrefMap, baseMap, ncxToc, titledChapters, m_chapters);
                }
            }
        }
    }

    // 2. EPUB 3 Navigation Document (nav) support
    QString navHref;
    for (size_t i = 0; i < items.size(); ++i) {
        pugi::xml_node it = items[i].node();
        QString props = QString::fromUtf8(it.attribute("properties").value()).toLower();
        QString hr = QString::fromUtf8(it.attribute("href").value());
        QString id = QString::fromUtf8(it.attribute("id").value()).toLower();
        if (props.contains("nav") || id == "nav" || id == "toc" || hr.endsWith("nav.xhtml", Qt::CaseInsensitive) || hr.endsWith("toc.xhtml", Qt::CaseInsensitive)) {
            QString cleanHr = QString::fromUtf8(QUrl::fromPercentEncoding(hr.toUtf8()).toUtf8());
            cleanHr.replace('\\', '/');
            while (cleanHr.startsWith('/')) cleanHr.remove(0, 1);
            while (cleanHr.startsWith("./")) cleanHr.remove(0, 2);
            navHref = QDir::cleanPath(m_opfDir + cleanHr);
            while (navHref.startsWith('/')) navHref.remove(0, 1);
            break;
        }
    }

    if (navHref.isEmpty() || readZipEntry(navHref).isEmpty()) {
        foreach (const QString &key, m_zipIndexMap.keys()) {
            if (key.endsWith("nav.xhtml", Qt::CaseInsensitive) || key.endsWith("toc.xhtml", Qt::CaseInsensitive) || key.endsWith("nav.html", Qt::CaseInsensitive)) {
                navHref = key;
                break;
            }
        }
    }

    if (!navHref.isEmpty()) {
        QByteArray navData = readZipEntry(navHref);
        if (!navData.isEmpty()) {
            pugi::xml_document docNav;
            if (docNav.load_buffer(navData.constData(), navData.size())) {
                QString navDir = QFileInfo(navHref).path();
                if (navDir == ".") navDir = "";
                else if (!navDir.endsWith('/')) navDir += "/";

                pugi::xml_node tocNav = docNav.select_node("//nav[@*='toc']").node();
                if (!tocNav) tocNav = docNav.select_node("//nav").node();
                pugi::xml_node rootList;
                if (tocNav) {
                    rootList = tocNav.child("ol");
                    if (!rootList) rootList = tocNav.child("ul");
                }
                if (!rootList) {
                    rootList = docNav.select_node("//ol").node();
                    if (!rootList) rootList = docNav.select_node("//ul").node();
                }
                if (rootList) {
                    parseNavList(rootList, 0, navDir, hrefMap, baseMap, navToc, titledChapters, m_chapters);
                }
            }
        }
    }

    // Determine primary TOC entries
    if (navToc.size() >= ncxToc.size() && !navToc.isEmpty()) {
        m_tocEntries = navToc;
    } else if (!ncxToc.isEmpty()) {
        m_tocEntries = ncxToc;
    } else if (!navToc.isEmpty()) {
        m_tocEntries = navToc;
    }

    // 3. Fallback: Chapter Content Heading Inspection
    // For any chapter still missing a title or with a generic "Chương X" title,
    // read the chapter's XHTML/HTML head and extract clean human-readable title.
    for (int i = 0; i < m_chapters.size(); ++i) {
        if (m_chapters[i].id == "libris_cover_page") continue;

        bool needsTitle = isGenericTitle(m_chapters[i].title);
        if (!needsTitle) {
            for (int k = 0; k < m_tocEntries.size(); ++k) {
                if (m_tocEntries[k].chapterIndex == i && isGenericTitle(m_tocEntries[k].title)) {
                    needsTitle = true;
                    break;
                }
            }
        }

        if (needsTitle) {
            QString extracted = extractChapterTitle(i);
            if (!extracted.isEmpty()) {
                QString newTitle = combineChapterTitle(m_chapters[i].title, extracted, i + 1);
                if (!isGenericTitle(newTitle)) {
                    m_chapters[i].title = newTitle;
                    for (int k = 0; k < m_tocEntries.size(); ++k) {
                        if (m_tocEntries[k].chapterIndex == i) {
                            m_tocEntries[k].title = combineChapterTitle(m_tocEntries[k].title, newTitle, i + 1);
                        }
                    }
                }
            }
        }
    }

    // Also check any individual TOC entry that is still generic (especially those with anchors)
    for (int k = 0; k < m_tocEntries.size(); ++k) {
        if (isGenericTitle(m_tocEntries[k].title)) {
            int cIdx = m_tocEntries[k].chapterIndex;
            if (cIdx >= 0 && cIdx < m_chapters.size()) {
                if (!isGenericTitle(m_chapters[cIdx].title) && m_tocEntries[k].anchor.isEmpty()) {
                    m_tocEntries[k].title = combineChapterTitle(m_tocEntries[k].title, m_chapters[cIdx].title, cIdx + 1);
                } else {
                    QString extracted = extractChapterTitle(cIdx, m_tocEntries[k].anchor);
                    if (!extracted.isEmpty()) {
                        QString combined = combineChapterTitle(m_tocEntries[k].title, extracted, cIdx + 1);
                        if (!isGenericTitle(combined)) {
                            m_tocEntries[k].title = combined;
                            if (isGenericTitle(m_chapters[cIdx].title)) {
                                m_chapters[cIdx].title = combined;
                            }
                        }
                    } else if (!isGenericTitle(m_chapters[cIdx].title)) {
                        m_tocEntries[k].title = combineChapterTitle(m_tocEntries[k].title, m_chapters[cIdx].title, cIdx + 1);
                    }
                }
            }
        }
    }

    // Fallback: If no TOC entries were discovered from NCX or NAV, populate from spine chapters
    if (m_tocEntries.isEmpty()) {
        for (int i = 0; i < m_chapters.size(); ++i) {
            EpubTocEntry entry;
            entry.title = m_chapters[i].title;
            entry.chapterIndex = i;
            entry.anchor = QString();
            entry.level = 0;
            entry.href = m_chapters[i].href;
            m_tocEntries.append(entry);
        }
    } else {
        // Sync any remaining generic TOC entry titles with m_chapters if chapter has a better title
        for (int k = 0; k < m_tocEntries.size(); ++k) {
            int cIdx = m_tocEntries[k].chapterIndex;
            if (cIdx >= 0 && cIdx < m_chapters.size()) {
                if (isGenericTitle(m_tocEntries[k].title) && !isGenericTitle(m_chapters[cIdx].title)) {
                    m_tocEntries[k].title = combineChapterTitle(m_tocEntries[k].title, m_chapters[cIdx].title, cIdx + 1);
                }
            }
        }
    }

    // Ensure Cover page is present as the first entry in TOC if chapter 0 is cover
    if (!m_chapters.isEmpty()) {
        bool hasCoverInToc = false;
        for (int i = 0; i < m_tocEntries.size(); ++i) {
            if (m_tocEntries[i].chapterIndex == 0) {
                hasCoverInToc = true;
                break;
            }
        }
        if (!hasCoverInToc && (m_chapters[0].title == "Bìa sách" || m_chapters[0].id == "libris_cover_page")) {
            EpubTocEntry coverEntry;
            coverEntry.title = "Bìa sách";
            coverEntry.chapterIndex = 0;
            coverEntry.anchor = QString();
            coverEntry.level = 0;
            coverEntry.href = m_chapters[0].href;
            m_tocEntries.prepend(coverEntry);
        }
    }

    return !m_chapters.isEmpty();
}

QString EpubParser::extractChapterTitle(int chapterIndex, const QString &anchor)
{
    if (chapterIndex < 0 || chapterIndex >= m_chapters.size()) return QString();
    if (m_chapters[chapterIndex].id == "libris_cover_page") return "Bìa sách";

    QByteArray chunk = readZipEntry(m_chapters[chapterIndex].href);
    if (chunk.isEmpty()) return QString();

    if (!anchor.isEmpty()) {
        int aPos = chunk.indexOf(anchor.toUtf8());
        if (aPos != -1) {
            int startPos = qMax(0, aPos - 256);
            int readLen = qMin(chunk.size() - startPos, 65536);
            QString text = QString::fromUtf8(chunk.mid(startPos, readLen));
            if (text.isEmpty()) text = QString::fromLatin1(chunk.mid(startPos, readLen));
            QString t = extractTitleFromHtml(text, m_title, anchor);
            if (!t.isEmpty() && !isGenericTitle(t)) return t;
        }
    }

    int checkLen = qMin(chunk.size(), 65536);
    QString text = QString::fromUtf8(chunk.left(checkLen));
    if (text.isEmpty()) text = QString::fromLatin1(chunk.left(checkLen));

    return extractTitleFromHtml(text, m_title, QString());
}

QString EpubParser::getChapterContent(int index, const QString &cacheDir)
{
    if (index < 0 || index >= m_chapters.size()) {
        return QString();
    }

    if (m_chapters[index].id == "libris_cover_page") {
        QString localCover = extractCover(cacheDir);
        QString coverSrc = fileToDataUri(localCover);
        if (coverSrc.isEmpty() && !m_coverHref.isEmpty()) {
            QString cleanCov = QDir::cleanPath(m_coverHref);
            while (cleanCov.startsWith('/')) cleanCov.remove(0, 1);
            QByteArray covBytes = readZipEntry(cleanCov);
            if (covBytes.isEmpty()) covBytes = readZipEntry(m_coverHref);
            if (!covBytes.isEmpty() && covBytes.size() <= 8 * 1024 * 1024) {
                coverSrc = bytesToDataUri(covBytes, cleanCov);
            }
        }
        if (coverSrc.isEmpty() && !localCover.isEmpty() && QFile::exists(localCover)) {
            coverSrc = QString("file://%1").arg(localCover);
        }

        QString coverHtml;
        coverHtml += "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/>";
        coverHtml += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"/>";
        coverHtml += "<style>";
        coverHtml += "html, body { width: 100%; height: 100%; margin: 0; padding: 0; overflow: hidden; }";
        coverHtml += ".cover-container { width: 100%; height: 100%; display: -webkit-box; -webkit-box-pack: center; -webkit-box-align: center; display: flex; justify-content: center; align-items: center; box-sizing: border-box; padding: 16px; text-align: center; }";
        coverHtml += ".cover-img { max-width: 90vw !important; max-height: 82vh !important; width: auto !important; height: auto !important; object-fit: contain; border-radius: 6px; box-shadow: 0 4px 18px rgba(0,0,0,0.6); display: block; margin: auto; }";
        coverHtml += ".cover-card { border: 2px solid rgba(128,128,128,0.3); border-radius: 12px; padding: 36px 24px; max-width: 85%; margin: auto; text-align: center; }";
        coverHtml += "</style></head><body>";
        coverHtml += "<div class=\"cover-container\">";
        if (!coverSrc.isEmpty()) {
            coverHtml += QString("<img class=\"cover-img\" src=\"%1\" alt=\"Cover\"/>").arg(coverSrc);
        } else {
            coverHtml += "<div class=\"cover-card\">";
            coverHtml += QString("<h1 style=\"font-size: 1.8em; margin-bottom: 12px;\">%1</h1>").arg(m_title);
            coverHtml += QString("<h3 style=\"font-size: 1.2em; font-weight: normal; opacity: 0.85;\">%1</h3>").arg(m_author);
            coverHtml += "</div>";
        }
        coverHtml += "</div></body></html>";
        return coverHtml;
    }

    if (m_chapterContentCache.contains(index)) {
        m_chapterCacheLru.removeAll(index);
        m_chapterCacheLru.append(index);
        return m_chapterContentCache.value(index);
    }

    QByteArray data = readZipEntry(m_chapters[index].href);
    if (data.isEmpty()) {
        qWarning() << "[EpubParser] Empty chapter data for:" << m_chapters[index].href;
        return QString();
    }

    QString text = QString::fromUtf8(data.constData(), data.size());
    if (text.isEmpty() && !data.isEmpty()) {
        text = QString::fromLatin1(data.constData(), data.size());
    }

    // Resolve chapter images directly to inlined data URIs to guarantee immediate rendering without WebView file:// security blocks
    QString chapterDir = QFileInfo(m_chapters[index].href).path();
    if (chapterDir == ".") chapterDir = "";
    else if (!chapterDir.endsWith('/')) chapterDir += "/";

    QStringList patterns;
    patterns << "<img[^>]+src\\s*=\\s*[\"']([^\"'>]+)[\"']";
    patterns << "<(?:[A-Za-z0-9_-]+:)?image[^>]+xlink:href\\s*=\\s*[\"']([^\"'>]+)[\"']";
    patterns << "<(?:[A-Za-z0-9_-]+:)?image[^>]+href\\s*=\\s*[\"']([^\"'>]+)[\"']";

    QSet<QString> rawUrls;
    for (int p = 0; p < patterns.size(); ++p) {
        QRegExp rx(patterns[p], Qt::CaseInsensitive);
        int pos = 0;
        while ((pos = rx.indexIn(text, pos)) != -1) {
            QString src = rx.cap(1).trimmed();
            if (!src.isEmpty() &&
                !src.startsWith("http:", Qt::CaseInsensitive) &&
                !src.startsWith("https:", Qt::CaseInsensitive) &&
                !src.startsWith("data:", Qt::CaseInsensitive) &&
                !src.startsWith("file:", Qt::CaseInsensitive)) {
                rawUrls.insert(src);
            }
            pos += qMax(1, rx.matchedLength());
        }
    }

    foreach (const QString &src, rawUrls) {
        QString cleanSrc = src;
        cleanSrc.replace("&amp;", "&");
        int fragIdx = cleanSrc.indexOf('#');
        if (fragIdx != -1) cleanSrc = cleanSrc.left(fragIdx);
        int qIdx = cleanSrc.indexOf('?');
        if (qIdx != -1) cleanSrc = cleanSrc.left(qIdx);

        QString decodedSrc = QString::fromUtf8(QUrl::fromPercentEncoding(cleanSrc.toUtf8()).toUtf8());
        QString relPath = decodedSrc;
        relPath.replace('\\', '/');
        while (relPath.startsWith("./")) relPath.remove(0, 2);
        QString fullEntry = QDir::cleanPath(chapterDir + relPath);
        while (fullEntry.startsWith('/')) fullEntry.remove(0, 1);

        QByteArray imgBytes = readZipEntry(fullEntry);
        if (imgBytes.isEmpty()) {
            imgBytes = readZipEntry(decodedSrc);
        }
        if (imgBytes.isEmpty() && !m_opfDir.isEmpty()) {
            imgBytes = readZipEntry(QDir::cleanPath(m_opfDir + relPath));
        }
        if (imgBytes.isEmpty()) {
            imgBytes = readZipEntry(QFileInfo(fullEntry).fileName());
        }
        if (imgBytes.isEmpty()) {
            imgBytes = readZipEntry(relPath);
        }

        if (!imgBytes.isEmpty()) {
            QString resolvedUri;
            if (imgBytes.size() <= 8 * 1024 * 1024) {
                resolvedUri = bytesToDataUri(imgBytes, fullEntry);
            }

            if (resolvedUri.isEmpty()) {
                QString ext = QFileInfo(fullEntry).suffix().toLower();
                if (ext.isEmpty()) ext = "jpg";
                QString targetDir = cacheDir.isEmpty() ? (QDir::tempPath() + "/libris_epub") : (cacheDir + "/epub");
                QString hash = QCryptographicHash::hash(m_filePath.toUtf8(), QCryptographicHash::Md5).toHex().left(10);
                QString imgFileName = QCryptographicHash::hash(fullEntry.toUtf8(), QCryptographicHash::Md5).toHex().left(12) + "." + ext;
                QString imgPath = targetDir + "/" + hash + "_" + imgFileName;
                if (QDir().mkpath(targetDir)) {
                    if (!QFile::exists(imgPath) || QFileInfo(imgPath).size() == 0) {
                        QFile outFile(imgPath);
                        if (outFile.open(QIODevice::WriteOnly)) {
                            outFile.write(imgBytes);
                            outFile.close();
                        }
                    }
                    if (QFile::exists(imgPath) && QFileInfo(imgPath).size() > 0) {
                        resolvedUri = "file://" + imgPath;
                    }
                }
            }

            if (!resolvedUri.isEmpty()) {
                text.replace(QString("\"%1\"").arg(src), QString("\"%1\"").arg(resolvedUri));
                text.replace(QString("'%1'").arg(src), QString("'%1'").arg(resolvedUri));
                if (src != decodedSrc) {
                    text.replace(QString("\"%1\"").arg(decodedSrc), QString("\"%1\"").arg(resolvedUri));
                    text.replace(QString("'%1'").arg(decodedSrc), QString("'%1'").arg(resolvedUri));
                }
            }
        }
    }

    // LRU eviction: limit in-memory cached chapters to 8 to avoid OOM
    while (m_chapterCacheLru.size() >= 8) {
        int oldest = m_chapterCacheLru.takeFirst();
        m_chapterContentCache.remove(oldest);
    }
    m_chapterContentCache.insert(index, text);
    m_chapterCacheLru.append(index);
    return text;
}

QString EpubParser::readHtmlByPath(const QString &entryPath)
{
    if (!m_zipOpen || entryPath.isEmpty()) return QString();

    QString clean = QString::fromUtf8(QUrl::fromPercentEncoding(entryPath.toUtf8()).toUtf8());
    clean.replace('\\', '/');
    while (clean.startsWith('/')) clean.remove(0, 1);
    while (clean.startsWith("./")) clean.remove(0, 2);

    QByteArray data = readZipEntry(clean);
    if (data.isEmpty() && !m_opfDir.isEmpty()) {
        data = readZipEntry(m_opfDir + clean);
    }
    if (data.isEmpty()) return QString();

    QString text = QString::fromUtf8(data.constData(), data.size());
    if (text.isEmpty() && !data.isEmpty()) {
        text = QString::fromLatin1(data.constData(), data.size());
    }
    return text;
}

QString EpubParser::extractCover(const QString &cacheDir)
{
    if (m_coverHref.isEmpty()) {
        return QString();
    }

    QDir().mkpath(cacheDir);

    QString hash = QString(QCryptographicHash::hash(m_filePath.toUtf8(), QCryptographicHash::Md5).toHex());
    QString cleanHref = QDir::cleanPath(m_coverHref);
    while (cleanHref.startsWith('/')) cleanHref.remove(0, 1);

    QString ext = QFileInfo(cleanHref).suffix().toLower();
    if (ext != "png" && ext != "gif") {
        ext = "jpg";
    }
    QString outPath = cacheDir + "/" + hash + "." + ext;

    if (QFile::exists(outPath)) {
        m_coverPath = outPath;
        return outPath;
    }

    QByteArray data = readZipEntry(cleanHref);
    if (data.isEmpty() && cleanHref != m_coverHref) {
        data = readZipEntry(m_coverHref);
    }

    if (data.isEmpty()) {
        return QString();
    }

    QFile file(outPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
        m_coverPath = outPath;
        return outPath;
    }

    return QString();
}

QString EpubParser::bookCss(const QString &cacheDir)
{
    if (!m_resolvedBookCss.isEmpty()) {
        return m_resolvedBookCss;
    }

    if (!m_zipOpen || m_cssFiles.isEmpty()) {
        return QString();
    }

    QString combinedCss;
    QRegExp urlRx("url\\s*\\(\\s*[\"']?([^\"'\\)]+)[\"']?\\s*\\)", Qt::CaseInsensitive);

    foreach (const QString &cssPath, m_cssFiles) {
        QByteArray data = readZipEntry(cssPath);
        if (data.isEmpty()) {
            data = readZipEntry(QFileInfo(cssPath).fileName());
        }
        if (data.isEmpty()) continue;

        QString css = QString::fromUtf8(data.constData(), data.size());
        if (css.isEmpty()) css = QString::fromLatin1(data.constData(), data.size());

        QString cssDir = QFileInfo(cssPath).path();
        if (cssDir == ".") cssDir = "";
        else if (!cssDir.endsWith('/')) cssDir += "/";

        if (!cacheDir.isEmpty()) {
            int pos = 0;
            QSet<QString> rawUrls;
            while ((pos = urlRx.indexIn(css, pos)) != -1) {
                QString u = urlRx.cap(1).trimmed();
                if (!u.isEmpty() && !u.startsWith("http:") && !u.startsWith("https:") && !u.startsWith("data:") && !u.startsWith("file:")) {
                    rawUrls.insert(u);
                }
                pos += urlRx.matchedLength();
            }

            foreach (const QString &u, rawUrls) {
                QString decodedU = QString::fromUtf8(QUrl::fromPercentEncoding(u.toUtf8()).toUtf8());
                QString rel = decodedU;
                if (rel.startsWith("./")) rel.remove(0, 2);
                QString fullEntry = QDir::cleanPath(cssDir + rel);
                while (fullEntry.startsWith('/')) fullEntry.remove(0, 1);

                QString hash = QString(QCryptographicHash::hash(fullEntry.toUtf8(), QCryptographicHash::Md5).toHex());
                QString ext = QFileInfo(fullEntry).suffix().toLower();
                if (ext.isEmpty()) ext = "ttf";
                bool isCssImg = (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "gif" || ext == "svg" || ext == "svgz" || ext == "webp");
                QString cssResolvedUrl;
                if (isCssImg) {
                    QByteArray imgBytes = readZipEntry(fullEntry);
                    if (imgBytes.isEmpty()) imgBytes = readZipEntry(QFileInfo(fullEntry).fileName());
                    if (!imgBytes.isEmpty() && imgBytes.size() <= 4 * 1024 * 1024) {
                        cssResolvedUrl = bytesToDataUri(imgBytes, fullEntry);
                    }
                }
                if (cssResolvedUrl.isEmpty()) {
                    QString localPath = cacheDir + "/" + hash + "." + ext;
                    if (!QFile::exists(localPath)) {
                        QByteArray fBytes = readZipEntry(fullEntry);
                        if (fBytes.isEmpty()) {
                            fBytes = readZipEntry(QFileInfo(fullEntry).fileName());
                        }
                        if (!fBytes.isEmpty()) {
                            QFile f(localPath);
                            if (f.open(QIODevice::WriteOnly)) {
                                f.write(fBytes);
                                f.close();
                            }
                        }
                    }
                    if (QFile::exists(localPath)) {
                        cssResolvedUrl = "file://" + localPath;
                    }
                }

                if (!cssResolvedUrl.isEmpty()) {
                    css.replace(QString("url(\"%1\")").arg(u), QString("url('%1')").arg(cssResolvedUrl));
                    css.replace(QString("url('%1')").arg(u), QString("url('%1')").arg(cssResolvedUrl));
                    css.replace(QString("url(%1)").arg(u), QString("url('%1')").arg(cssResolvedUrl));
                    if (u != decodedU) {
                        css.replace(QString("url(\"%1\")").arg(decodedU), QString("url('%1')").arg(cssResolvedUrl));
                        css.replace(QString("url('%1')").arg(decodedU), QString("url('%1')").arg(cssResolvedUrl));
                        css.replace(QString("url(%1)").arg(decodedU), QString("url('%1')").arg(cssResolvedUrl));
                    }
                }
            }
        }

        // Clean out @page rules that might conflict with screen paging
        css.remove(QRegExp("@page\\s*\\{[^\\}]*\\}", Qt::CaseInsensitive));

        combinedCss += "\n/* " + cssPath + " */\n" + css;
    }

    m_resolvedBookCss = combinedCss;
    return m_resolvedBookCss;
}
