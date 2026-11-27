#include "CbzParser.hpp"
#include "ImageUriUtil.hpp"
#include "pugixml.hpp"
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QDebug>
#include <algorithm>

CbzParser::CbzParser()
    : m_seriesIndex(0)
    , m_zipOpen(false)
{
    memset(&m_zip, 0, sizeof(m_zip));
}

CbzParser::~CbzParser()
{
    close();
}

void CbzParser::close()
{
    if (m_zipOpen) {
        mz_zip_reader_end(&m_zip);
        memset(&m_zip, 0, sizeof(m_zip));
        m_zipOpen = false;
    }
    if (m_zipFile.isOpen()) {
        m_zipFile.close();
    }
    m_filePath.clear();
    m_title.clear();
    m_author.clear();
    m_series.clear();
    m_seriesIndex = 0;
    m_description.clear();
    m_genre.clear();
    m_coverPath.clear();
    m_coverEntryName.clear();
    m_chapters.clear();
    m_tocEntries.clear();
    m_contentCache.clear();
    m_cacheLru.clear();
}

bool CbzParser::naturalLessThan(const QString &s1, const QString &s2)
{
    int i1 = 0, i2 = 0;
    int len1 = s1.length(), len2 = s2.length();

    while (i1 < len1 && i2 < len2) {
        QChar c1 = s1.at(i1);
        QChar c2 = s2.at(i2);

        if (c1.isDigit() && c2.isDigit()) {
            int start1 = i1;
            while (i1 < len1 && s1.at(i1).isDigit()) i1++;
            int start2 = i2;
            while (i2 < len2 && s2.at(i2).isDigit()) i2++;

            QString numStr1 = s1.mid(start1, i1 - start1);
            QString numStr2 = s2.mid(start2, i2 - start2);

            bool ok1, ok2;
            qulonglong n1 = numStr1.toULongLong(&ok1);
            qulonglong n2 = numStr2.toULongLong(&ok2);

            if (ok1 && ok2 && n1 != n2) {
                return n1 < n2;
            }
            if (numStr1 != numStr2) {
                if (numStr1.length() != numStr2.length()) {
                    return numStr1.length() < numStr2.length();
                }
                return numStr1 < numStr2;
            }
        } else {
            QChar l1 = c1.toLower();
            QChar l2 = c2.toLower();
            if (l1 != l2) {
                return l1 < l2;
            }
            i1++;
            i2++;
        }
    }
    return len1 < len2;
}

bool CbzParser::open(const QString &filePath)
{
    close();

    m_filePath = filePath;
    if (!QFileInfo(filePath).exists()) {
        qWarning() << "[CbzParser] Cannot open file:" << filePath;
        return false;
    }

    memset(&m_zip, 0, sizeof(m_zip));
    if (!mz_zip_reader_init_file(&m_zip, filePath.toUtf8().constData(), 0)) {
        qWarning() << "[CbzParser] Failed to initialize zip reader for:" << filePath;
        return false;
    }
    m_zipOpen = true;

    mz_uint numFiles = mz_zip_reader_get_num_files(&m_zip);
    QStringList imageEntries;
    QString comicInfoEntry;

    for (mz_uint i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(&m_zip, i, &fileStat)) continue;

        if (mz_zip_reader_is_file_a_directory(&m_zip, i)) continue;

        QString entryName = QString::fromUtf8(fileStat.m_filename);
        if (entryName.isEmpty()) {
            entryName = QString::fromLatin1(fileStat.m_filename);
        }

        // Skip macOS artifacts and hidden files
        if (entryName.startsWith("__MACOSX/") || entryName.contains("/.") || entryName.startsWith(".") || entryName.endsWith(".DS_Store", Qt::CaseInsensitive) || entryName.endsWith("Thumbs.db", Qt::CaseInsensitive)) {
            continue;
        }

        QString fileName = QFileInfo(entryName).fileName().toLower();
        if (fileName == "comicinfo.xml") {
            comicInfoEntry = entryName;
            continue;
        }

        QString ext = QFileInfo(entryName).suffix().toLower();
        if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "webp" || ext == "bmp") {
            imageEntries.append(entryName);
            if (m_coverEntryName.isEmpty() && (fileName.contains("cover") || fileName.contains("front"))) {
                m_coverEntryName = entryName;
            }
        }
    }

    if (imageEntries.isEmpty()) {
        qWarning() << "[CbzParser] No image files found in CBZ:" << filePath;
        close();
        return false;
    }

    // Natural sort image pages
    std::sort(imageEntries.begin(), imageEntries.end(), naturalLessThan);

    if (m_coverEntryName.isEmpty()) {
        m_coverEntryName = imageEntries.first();
    }

    // Parse ComicInfo.xml if available
    if (!comicInfoEntry.isEmpty()) {
        QByteArray xmlData = readZipEntry(comicInfoEntry);
        if (!xmlData.isEmpty()) {
            parseComicInfo(xmlData);
        }
    }

    // Fallbacks for metadata
    QFileInfo fi(filePath);
    if (m_title.isEmpty()) {
        m_title = fi.completeBaseName().replace('_', ' ');
    }
    if (m_author.isEmpty()) {
        m_author = "Truyện tranh CBZ";
    }
    if (m_genre.isEmpty()) {
        m_genre = "Manga / Comic";
    }

    // Populate chapters and TOC
    for (int i = 0; i < imageEntries.size(); ++i) {
        BookChapter ch;
        ch.id = QString("page_%1").arg(i + 1);
        ch.href = imageEntries[i];
        ch.title = QString("Trang %1").arg(i + 1);
        m_chapters.append(ch);

        EpubTocEntry toc;
        toc.title = ch.title;
        toc.chapterIndex = i;
        toc.anchor = QString();
        toc.level = 0;
        toc.href = ch.href;
        m_tocEntries.append(toc);
    }

    return true;
}

QByteArray CbzParser::readZipEntry(const QString &entryName)
{
    if (!m_zipOpen) return QByteArray();

    int entryIndex = mz_zip_reader_locate_file(&m_zip, entryName.toUtf8().constData(), NULL, 0);
    if (entryIndex < 0) {
        // Fallback case-insensitive
        mz_uint numFiles = mz_zip_reader_get_num_files(&m_zip);
        for (mz_uint i = 0; i < numFiles; ++i) {
            mz_zip_archive_file_stat s;
            if (mz_zip_reader_file_stat(&m_zip, i, &s)) {
                if (QString::fromUtf8(s.m_filename).compare(entryName, Qt::CaseInsensitive) == 0) {
                    entryIndex = i;
                    break;
                }
            }
        }
    }

    if (entryIndex < 0) {
        return QByteArray();
    }

    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(&m_zip, (mz_uint)entryIndex, &stat)) {
        return QByteArray();
    }
    static const mz_uint64 MAX_ENTRY_SIZE = 64ULL * 1024ULL * 1024ULL;
    if (stat.m_uncomp_size > MAX_ENTRY_SIZE) {
        qWarning() << "CbzParser: zip entry too large:" << entryName << stat.m_uncomp_size;
        return QByteArray();
    }

    size_t uncompSize = 0;
    void *pData = mz_zip_reader_extract_to_heap(&m_zip, (mz_uint)entryIndex, &uncompSize, 0);
    if (!pData) {
        return QByteArray();
    }
    if (uncompSize > MAX_ENTRY_SIZE) {
        mz_free(pData);
        return QByteArray();
    }

    QByteArray result((const char*)pData, (int)uncompSize);
    mz_free(pData);
    return result;
}

void CbzParser::parseComicInfo(const QByteArray &xmlData)
{
    pugi::xml_document doc;
    pugi::xml_parse_result res = doc.load_buffer(xmlData.constData(), xmlData.size());
    if (!res) return;

    pugi::xml_node root = doc.child("ComicInfo");
    if (!root) return;

    pugi::xml_node titleNode = root.child("Title");
    if (titleNode && titleNode.text()) {
        m_title = QString::fromUtf8(titleNode.text().as_string()).trimmed();
    }

    pugi::xml_node seriesNode = root.child("Series");
    if (seriesNode && seriesNode.text()) {
        m_series = QString::fromUtf8(seriesNode.text().as_string()).trimmed();
    }

    pugi::xml_node numberNode = root.child("Number");
    if (numberNode && numberNode.text()) {
        m_seriesIndex = numberNode.text().as_int();
    }

    pugi::xml_node writerNode = root.child("Writer");
    if (writerNode && writerNode.text()) {
        m_author = QString::fromUtf8(writerNode.text().as_string()).trimmed();
    } else {
        pugi::xml_node pencillerNode = root.child("Penciller");
        if (pencillerNode && pencillerNode.text()) {
            m_author = QString::fromUtf8(pencillerNode.text().as_string()).trimmed();
        }
    }

    pugi::xml_node summaryNode = root.child("Summary");
    if (summaryNode && summaryNode.text()) {
        m_description = QString::fromUtf8(summaryNode.text().as_string()).trimmed();
    }

    pugi::xml_node genreNode = root.child("Genre");
    if (genreNode && genreNode.text()) {
        m_genre = QString::fromUtf8(genreNode.text().as_string()).trimmed();
    }
}

QString CbzParser::extractCover(const QString &cacheDir)
{
    if (!m_coverPath.isEmpty() && QFile::exists(m_coverPath)) {
        return m_coverPath;
    }

    if (m_coverEntryName.isEmpty()) {
        if (!m_chapters.isEmpty()) {
            m_coverEntryName = m_chapters.first().href;
        } else {
            return QString();
        }
    }

    QDir dir(cacheDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString hash = QString(QCryptographicHash::hash(m_filePath.toUtf8(), QCryptographicHash::Md5).toHex());
    QString ext = QFileInfo(m_coverEntryName).suffix().toLower();
    if (ext.isEmpty()) ext = "jpg";
    QString outPath = cacheDir + "/" + hash + "." + ext;

    if (QFile::exists(outPath) && QFileInfo(outPath).size() > 0) {
        m_coverPath = outPath;
        return m_coverPath;
    }

    QByteArray imgData = readZipEntry(m_coverEntryName);
    if (imgData.isEmpty()) {
        return QString();
    }

    QFile outFile(outPath);
    if (outFile.open(QIODevice::WriteOnly)) {
        outFile.write(imgData);
        outFile.close();
        m_coverPath = outPath;
        return m_coverPath;
    }

    return QString();
}

QString CbzParser::getChapterContent(int index, const QString &cacheDir)
{
    if (index < 0 || index >= m_chapters.size()) {
        return QString();
    }

    if (m_contentCache.contains(index)) {
        m_cacheLru.removeAll(index);
        m_cacheLru.append(index);
        return m_contentCache.value(index);
    }

    QByteArray data = readZipEntry(m_chapters[index].href);
    if (data.isEmpty()) {
        qWarning() << "[CbzParser] Empty image data for page:" << m_chapters[index].href;
        return QString();
    }

    QString ext = QFileInfo(m_chapters[index].href).suffix().toLower();
    if (ext.isEmpty()) ext = "jpg";
    QString imgSrc;
    if (data.size() <= 6 * 1024 * 1024) {
        imgSrc = bytesToDataUri(data, m_chapters[index].href);
    }

    // Determine cache target directory if image exceeds inline limit
    QString targetDir = cacheDir;
    if (targetDir.isEmpty()) {
        targetDir = QDir::tempPath() + "/libris_cbz";
    } else {
        targetDir = cacheDir + "/cbz";
    }

    QString hash = QCryptographicHash::hash(m_filePath.toUtf8(), QCryptographicHash::Md5).toHex().left(10);
    QString imgPath = QString("%1/%2_%3.%4").arg(targetDir, hash).arg(index).arg(ext);

    if (imgSrc.isEmpty() && QDir().mkpath(targetDir)) {
        if (!QFile::exists(imgPath) || QFileInfo(imgPath).size() == 0) {
            QFile outFile(imgPath);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(data);
                outFile.close();
            }
        }
        if (QFile::exists(imgPath) && QFileInfo(imgPath).size() > 0) {
            imgSrc = QString("file://%1").arg(imgPath);
        }
    }

    // If writing to primary cache directory failed, fallback to system temp folder
    if (imgSrc.isEmpty()) {
        QString fallbackDir = QDir::tempPath() + "/libris_cbz";
        if (QDir().mkpath(fallbackDir)) {
            QString fallbackPath = QString("%1/%2_%3.%4").arg(fallbackDir, hash).arg(index).arg(ext);
            QFile outFile(fallbackPath);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(data);
                outFile.close();
            }
            if (QFile::exists(fallbackPath) && QFileInfo(fallbackPath).size() > 0) {
                imgSrc = QString("file://%1").arg(fallbackPath);
            }
        }
    }

    if (imgSrc.isEmpty()) {
        qWarning() << "[CbzParser] Failed to cache CBZ image to disk for page:" << index;
        return QString("<!DOCTYPE html><html><body><div style='color:#888;text-align:center;padding:50px;'>Không thể lưu bộ nhớ tạm hình ảnh cho trang này (bộ nhớ đầy).</div></body></html>");
    }

    QString html;
    html += "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no, minimum-scale=1.0, maximum-scale=1.0\"/>";
    html += "<style>";
    html += "html, body { width: 100% !important; height: 100% !important; margin: 0 !important; padding: 0 !important; overflow: hidden !important; background: transparent; }";
    html += ".comic-box { width: 100% !important; height: 100% !important; display: -webkit-box; display: flex; -webkit-box-pack: center; justify-content: center; -webkit-box-align: center; align-items: center; text-align: center; margin: 0 !important; padding: 0 !important; }";
    html += ".comic-img { max-width: 100vw !important; max-height: 98vh !important; width: auto !important; height: auto !important; object-fit: contain; margin: auto !important; display: block !important; -webkit-column-break-inside: avoid; page-break-inside: avoid; }";
    html += "</style></head><body>";
    QString escapedAlt = m_chapters[index].title;
    escapedAlt.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;");
    html += QString("<div class=\"comic-box\"><img class=\"comic-img\" src=\"%1\" alt=\"%2\"/></div>")
            .arg(imgSrc).arg(escapedAlt);
    html += "</body></html>";

    // Manage LRU cache of 6 pages in memory
    if (m_contentCache.size() >= 6 && !m_cacheLru.isEmpty()) {
        int oldest = m_cacheLru.takeFirst();
        m_contentCache.remove(oldest);
    }
    m_contentCache.insert(index, html);
    m_cacheLru.append(index);

    return html;
}
