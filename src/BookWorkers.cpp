#include "BookWorkers.hpp"
#include "RegexUtil.hpp"
#include "ImageUriUtil.hpp"
#include <QDebug>
#include <QRegExp>

// ============================================================================
// SearchWorker Implementation
// ============================================================================

SearchWorker::SearchWorker(const SearchParams &params, QObject *parent)
    : QThread(parent)
    , m_params(params)
    , m_cancelled(0)
{
}

SearchWorker::~SearchWorker()
{
    cancel();
    wait();
}

void SearchWorker::cancel()
{
    m_cancelled.fetchAndStoreOrdered(1);
}

static QString escapeHtmlSnippet(const QString &str)
{
    QString res = str;
    res.replace("&", "&amp;");
    res.replace("<", "&lt;");
    res.replace(">", "&gt;");
    res.replace("\"", "&quot;");
    return res;
}

QString SearchWorker::extractSnippet(const QString &text, int matchPos, int matchLen, const QString &query)
{
    const int snippetContext = 45;
    int start = qMax(0, matchPos - snippetContext);
    int end = qMin(text.length(), matchPos + matchLen + snippetContext);

    while (start > 0 && !text.at(start).isSpace()) {
        start--;
    }
    while (end < text.length() && !text.at(end).isSpace()) {
        end++;
    }

    QString rawSnippet = text.mid(start, end - start).trimmed();
    rawSnippet.replace(QRegExp("[\\r\\n\\t]+"), " ");

    QString needle = query.trimmed();
    if (needle.isEmpty() && matchLen > 0) {
        needle = text.mid(matchPos, matchLen);
    }
    QString snippet;
    if (!needle.isEmpty()) {
        QRegExp rx(QRegExp::escape(needle), Qt::CaseInsensitive);
        int pos = 0, last = 0;
        while ((pos = rx.indexIn(rawSnippet, last)) != -1) {
            int len = rx.matchedLength();
            if (len <= 0) break;
            snippet += escapeHtmlSnippet(rawSnippet.mid(last, pos - last));
            snippet += "<b>" + escapeHtmlSnippet(rawSnippet.mid(pos, len)) + "</b>";
            last = pos + len;
        }
        snippet += escapeHtmlSnippet(rawSnippet.mid(last));
    } else {
        snippet = escapeHtmlSnippet(rawSnippet);
    }

    if (start > 0) {
        snippet.prepend(QString::fromUtf8("… "));
    }
    if (end < text.length()) {
        snippet.append(QString::fromUtf8(" …"));
    }

    return snippet;
}

void SearchWorker::run()
{
    QVariantList results;
    QString q = m_params.query.trimmed();
    if (q.isEmpty() || m_cancelled) {
        emit searchCompleted(results);
        return;
    }

    int totalFound = 0;
    const int maxTotalResults = m_params.maxTotalResults > 0 ? m_params.maxTotalResults : 150;
    bool isChapterScope = (m_params.scope == "chapter");

    if (m_params.format == "epub") {
        EpubParser ep;
        if (ep.open(m_params.filePath)) {
            int startChapter = (isChapterScope && ep.chapterCount() > 0) ? qBound(0, m_params.currentChapterIndex, ep.chapterCount() - 1) : 0;
            int endChapter = isChapterScope ? (startChapter + 1) : ep.chapterCount();

            for (int i = startChapter; i < endChapter && totalFound < maxTotalResults; ++i) {
                if (m_cancelled) break;

                QString content = ep.getChapterContent(i);
                QString plain = content;
                plain.remove(QRegExp("<[^>]*>"));
                plain.replace("&nbsp;", " ").replace("&quot;", "\"").replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">");

                int pos = 0;
                int foundInChapter = 0;
                while ((pos = plain.indexOf(q, pos, Qt::CaseInsensitive)) != -1 && totalFound < maxTotalResults) {
                    if (m_cancelled) break;
                    foundInChapter++;
                    if (isChapterScope || foundInChapter <= 10) {
                        QString snippet = extractSnippet(plain, pos, q.length(), q);

                        QVariantMap res;
                        res["chapterIndex"] = i;
                        res["matchIndex"] = foundInChapter - 1;
                        res["matchNum"] = foundInChapter;
                        res["chapterTitle"] = isChapterScope ? QString("Kết quả %1 trong chương").arg(foundInChapter)
                                                            : ((i < ep.chapters().size()) ? ep.chapters()[i].title : QString("Chương %1").arg(i + 1));
                        res["snippet"] = snippet;
                        res["matchWord"] = plain.mid(pos, q.length());
                        results.append(res);
                        totalFound++;
                    }
                    pos += q.length();
                }
            }
            ep.close();
        }
    } else if (m_params.format == "fb2") {
        Fb2Parser fb;
        if (fb.open(m_params.filePath)) {
            int startChapter = (isChapterScope && fb.chapterCount() > 0) ? qBound(0, m_params.currentChapterIndex, fb.chapterCount() - 1) : 0;
            int endChapter = isChapterScope ? (startChapter + 1) : fb.chapterCount();

            for (int i = startChapter; i < endChapter && totalFound < maxTotalResults; ++i) {
                if (m_cancelled) break;
                QString content = fb.getChapterContent(i);
                QString plain = content;
                plain.remove(QRegExp("<[^>]*>"));

                int pos = 0;
                int foundInChapter = 0;
                while ((pos = plain.indexOf(q, pos, Qt::CaseInsensitive)) != -1 && totalFound < maxTotalResults) {
                    if (m_cancelled) break;
                    foundInChapter++;
                    if (isChapterScope || foundInChapter <= 10) {
                        QString snippet = extractSnippet(plain, pos, q.length(), q);

                        QVariantMap res;
                        res["chapterIndex"] = i;
                        res["matchIndex"] = foundInChapter - 1;
                        res["matchNum"] = foundInChapter;
                        res["chapterTitle"] = isChapterScope ? QString("Kết quả %1 trong chương").arg(foundInChapter)
                                                            : ((i < fb.chapters().size()) ? fb.chapters()[i].title : QString("Phần %1").arg(i + 1));
                        res["snippet"] = snippet;
                        res["matchWord"] = plain.mid(pos, q.length());
                        results.append(res);
                        totalFound++;
                    }
                    pos += q.length();
                }
            }
            fb.close();
        }
    } else if (m_params.format == "txt" || m_params.format == "md" || m_params.format == "markdown" || m_params.format == "html" || m_params.format == "htm") {
        TxtParser tp;
        if (tp.open(m_params.filePath)) {
            int startChapter = (isChapterScope && tp.chapterCount() > 0) ? qBound(0, m_params.currentChapterIndex, tp.chapterCount() - 1) : 0;
            int endChapter = isChapterScope ? (startChapter + 1) : tp.chapterCount();

            for (int i = startChapter; i < endChapter && totalFound < maxTotalResults; ++i) {
                if (m_cancelled) break;
                QString plain = tp.getChapterContent(i);
                plain.remove(minimalRx("<style[^>]*>[\\s\\S]*</style>", Qt::CaseInsensitive));
                plain.remove(minimalRx("<script[^>]*>[\\s\\S]*</script>", Qt::CaseInsensitive));
                plain.remove(QRegExp("<[^>]*>"));
                plain.replace("&nbsp;", " ").replace("&quot;", "\"").replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">");

                int pos = 0;
                int foundInChapter = 0;
                while ((pos = plain.indexOf(q, pos, Qt::CaseInsensitive)) != -1 && totalFound < maxTotalResults) {
                    if (m_cancelled) break;
                    foundInChapter++;
                    if (isChapterScope || foundInChapter <= 10) {
                        QString snippet = extractSnippet(plain, pos, q.length(), q);

                        QVariantMap res;
                        res["chapterIndex"] = i;
                        res["matchIndex"] = foundInChapter - 1;
                        res["matchNum"] = foundInChapter;
                        res["chapterTitle"] = isChapterScope ? QString("Kết quả %1 trong chương").arg(foundInChapter)
                                                            : ((i < tp.chapters().size()) ? tp.chapters()[i].title : QString("Chương %1").arg(i + 1));
                        res["snippet"] = snippet;
                        res["matchWord"] = plain.mid(pos, q.length());
                        results.append(res);
                        totalFound++;
                    }
                    pos += q.length();
                }
            }
            tp.close();
        }
    } else if (m_params.format == "mobi" || m_params.format == "azw" || m_params.format == "azw3" || m_params.format == "kf8" || m_params.format == "prc") {
        MobiParser mp;
        if (mp.open(m_params.filePath)) {
            int startChapter = (isChapterScope && mp.chapterCount() > 0) ? qBound(0, m_params.currentChapterIndex, mp.chapterCount() - 1) : 0;
            int endChapter = isChapterScope ? (startChapter + 1) : mp.chapterCount();

            for (int i = startChapter; i < endChapter && totalFound < maxTotalResults; ++i) {
                if (m_cancelled) break;
                QString content = mp.getChapterContent(i);
                QString plain = content;
                plain.remove(QRegExp("<[^>]*>"));
                plain.replace("&nbsp;", " ").replace("&quot;", "\"").replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">");

                int pos = 0;
                int foundInChapter = 0;
                while ((pos = plain.indexOf(q, pos, Qt::CaseInsensitive)) != -1 && totalFound < maxTotalResults) {
                    if (m_cancelled) break;
                    foundInChapter++;
                    if (isChapterScope || foundInChapter <= 10) {
                        QString snippet = extractSnippet(plain, pos, q.length(), q);

                        QVariantMap res;
                        res["chapterIndex"] = i;
                        res["matchIndex"] = foundInChapter - 1;
                        res["matchNum"] = foundInChapter;
                        res["chapterTitle"] = isChapterScope ? QString("Kết quả %1 trong chương").arg(foundInChapter)
                                                            : ((i < mp.chapters().size()) ? mp.chapters()[i].title : QString("Chương %1").arg(i + 1));
                        res["snippet"] = snippet;
                        res["matchWord"] = plain.mid(pos, q.length());
                        results.append(res);
                        totalFound++;
                    }
                    pos += q.length();
                }
            }
            mp.close();
        }
    } else if (!m_params.currentPdfText.isEmpty()) {
        int pos = 0;
        int foundInChapter = 0;
        while ((pos = m_params.currentPdfText.indexOf(q, pos, Qt::CaseInsensitive)) != -1 && totalFound < maxTotalResults) {
            if (m_cancelled) break;
            foundInChapter++;
            if (isChapterScope || foundInChapter <= 15) {
                QString snippet = extractSnippet(m_params.currentPdfText, pos, q.length(), q);

                QVariantMap res;
                res["chapterIndex"] = m_params.currentChapterIndex;
                res["matchIndex"] = foundInChapter - 1;
                res["matchNum"] = foundInChapter;
                res["chapterTitle"] = isChapterScope ? QString("Kết quả %1 (Trang %2)").arg(foundInChapter).arg(m_params.currentChapterIndex)
                                                    : QString("Trang %1 (Trang hiện tại)").arg(m_params.currentChapterIndex);
                res["snippet"] = snippet;
                res["matchWord"] = m_params.currentPdfText.mid(pos, q.length());
                results.append(res);
                totalFound++;
            }
            pos += q.length();
        }
    }

    if (!m_cancelled) {
        emit searchCompleted(results);
    }
}

// ============================================================================
// ScanWorker Implementation
// ============================================================================

ScanWorker::ScanWorker(const QStringList &searchRoots, const QString &cacheDir, QObject *parent)
    : QThread(parent)
    , m_searchRoots(searchRoots)
    , m_cacheDir(cacheDir)
    , m_cancelled(0)
    , m_emitPartial(0)
{
}

ScanWorker::~ScanWorker()
{
    cancel();
    wait();
}

void ScanWorker::setPrefsSnapshot(const ScanPrefsSnapshot &prefs)
{
    m_prefs = prefs;
}

void ScanWorker::cancel(bool emitPartial)
{
    if (emitPartial) {
        m_emitPartial.fetchAndStoreOrdered(1);
    }
    m_cancelled.fetchAndStoreOrdered(1);
}

void ScanWorker::extractSeriesFromTitleOrFile(const QString &title, const QString &fileName, QString &outSeries, int &outIndex)
{
    if (!outSeries.isEmpty()) return;

    QRegExp reBrackets = minimalRx("^\\[([^\\]#]+)(?:[\\s_\\-–#]+|[\\s_\\-–#]+(?:Tập|Quyển|Book|Vol|Volume|Part|Phần|#)\\s*)(\\d+)\\]", Qt::CaseInsensitive);
    if (reBrackets.indexIn(title) != -1 || reBrackets.indexIn(fileName) != -1) {
        outSeries = reBrackets.cap(1).trimmed();
        outIndex = reBrackets.cap(2).toInt();
        return;
    }

    QRegExp reParens = minimalRx("\\(([^\\)#]+)(?:[\\s_\\-–#,]+|[\\s_\\-–#,]+(?:Tập|Quyển|Book|Vol|Volume|Part|Phần|#)\\s*)(\\d+)\\)", Qt::CaseInsensitive);
    if (reParens.indexIn(title) != -1 || reParens.indexIn(fileName) != -1) {
        outSeries = reParens.cap(1).trimmed();
        outIndex = reParens.cap(2).toInt();
        return;
    }

    QRegExp reDash("^(.*)\\s*[-–]\\s*(?:Tập|Quyển|Book|Vol|Volume|Part|Phần|#)\\s*(\\d+)\\s*[-–]", Qt::CaseInsensitive);
    if (reDash.indexIn(title) != -1 || reDash.indexIn(fileName) != -1) {
        outSeries = reDash.cap(1).trimmed();
        outIndex = reDash.cap(2).toInt();
        return;
    }
}

void ScanWorker::scanDir(const QString &dirPath, int depth, QSet<QString> &visitedDirs, QSet<QString> &addedPaths, QList<QVariantMap> &books)
{
    if (depth > 12 || m_cancelled) return;

    QDir dir(dirPath);
    if (!dir.exists()) return;

    QString canon = dir.canonicalPath();
    if (canon.isEmpty() || visitedDirs.contains(canon)) return;
    visitedDirs.insert(canon);

    QFileInfoList allFiles = dir.entryInfoList(QDir::Files, QDir::Time);
    foreach (const QFileInfo &fi, allFiles) {
        if (m_cancelled) return;

        QString ext = fi.suffix().toLower();
        if (!isSupportedBookExt(ext)) continue;

        QString absPath = fi.absoluteFilePath();
        if (addedPaths.contains(absPath)) continue;
        addedPaths.insert(absPath);

        if (m_prefs.hiddenPaths.contains(absPath)) {
            continue;
        }

        QString rawName = fi.completeBaseName().replace('_', ' ');
        QString bookTitle = rawName;
        QString bookAuthor = (ext == "epub") ? "EPUB eBook" :
                             (ext == "txt")  ? "Văn bản TXT" :
                             (ext == "md" || ext == "markdown") ? "Markdown" :
                             (ext == "html" || ext == "htm") ? "Web / HTML" :
                             (ext == "fb2")  ? "FB2 FictionBook" :
                             (ext == "pdf")  ? "Tài liệu PDF" :
                             (ext == "djvu" || ext == "djv") ? "Tài liệu DjVu" :
                             (ext == "cbz")  ? "Truyện tranh CBZ" :
                             (ext == "mobi" || ext == "prc") ? "Sách điện tử MOBI" :
                             (ext == "azw" || ext == "azw3" || ext == "kf8") ? "Sách Kindle AZW3" : "Tài liệu";
        QString bookSeries = "";
        int bookSeriesIndex = 0;
        QString cover = "";
        QString formatBadge = ext.toUpper();

        QString hash = QString(QCryptographicHash::hash(absPath.toUtf8(), QCryptographicHash::Md5).toHex());
        QString cachedNoCover = m_cacheDir + "/" + hash + ".nocover";

        QString cachedTitle = m_prefs.cachedTitles.value(absPath);
        QString cachedAuthor = m_prefs.cachedAuthors.value(absPath);
        if (cachedTitle.isEmpty() && m_prefs.lastBookPath == absPath) {
            cachedTitle = m_prefs.lastBookTitle;
            cachedAuthor = m_prefs.lastBookAuthor;
        }

        if (!cachedTitle.isEmpty()) {
            bookTitle = cachedTitle;
            if (!cachedAuthor.isEmpty()) {
                bookAuthor = cachedAuthor;
            }
        }

        cover = findCachedCoverFile(m_cacheDir, hash);

        bool dirtyMeta = false;
        bool dirtySeries = false;

        // Keep ScanWorker pure filesystem + cache lookup (~1-2s for 1,000+ books);
        // uncached covers are extracted asynchronously afterwards by CoverExtractWorker.
        if (cachedTitle.isEmpty()) {
            dirtyMeta = true;
        }

        // Only fallback to extracting Author from filename if no author or title was found from file metadata or cache
        bool authorIsGeneric = (bookAuthor.isEmpty() || 
                                bookAuthor == "Chưa rõ tác giả" || 
                                bookAuthor == "Unknown Author" ||
                                bookAuthor == "EPUB eBook" ||
                                bookAuthor == "Văn bản TXT" ||
                                bookAuthor == "FB2 FictionBook" ||
                                bookAuthor == "Tài liệu PDF" ||
                                bookAuthor == "Tài liệu DjVu" ||
                                bookAuthor == "Truyện tranh CBZ" ||
                                bookAuthor == "Sách điện tử MOBI");
        if (authorIsGeneric && (bookTitle.isEmpty() || bookTitle == rawName)) {
            if (rawName.contains(" - ")) {
                QStringList parts = rawName.split(" - ");
                if (parts.size() >= 2) {
                    bookTitle = parts[0].trimmed();
                    bookAuthor = parts[1].trimmed();
                }
            }
        }

        qint64 size = fi.size();
        QString fileSizeStr;
        if (size < 1024 * 1024) {
            fileSizeStr = QString("%1 KB").arg(qMax((qint64)1, size / 1024));
        } else {
            fileSizeStr = QString("%1 MB").arg(QString::number(size / (1024.0 * 1024.0), 'f', 1));
        }

        QDateTime modTime = fi.lastModified();

        if (bookSeries.isEmpty()) {
            if (m_prefs.cachedSeries.contains(absPath)) {
                bookSeries = m_prefs.cachedSeries.value(absPath);
                bookSeriesIndex = m_prefs.cachedSeriesIdx.value(absPath, 0);
            } else {
                extractSeriesFromTitleOrFile(bookTitle, rawName, bookSeries, bookSeriesIndex);
                dirtySeries = true;
            }
        }

        int savedProg = m_prefs.progressPct.value(absPath, 0);

        QVariantMap bookMap;
        bookMap["title"] = bookTitle;
        bookMap["author"] = bookAuthor;
        bookMap["series"] = bookSeries;
        bookMap["seriesIndex"] = bookSeriesIndex;
        bookMap["filePath"] = absPath;
        bookMap["path"] = absPath;
        bookMap["cover"] = cover;
        bookMap["format"] = (ext == "djv") ? QString("djvu") : ext;
        bookMap["formatBadge"] = formatBadge;
        bookMap["fileSize"] = fileSizeStr;
        bookMap["fileSizeBytes"] = size;
        bookMap["fileSizeSort"] = QString("%1").arg(size, 14, 10, QChar('0'));
        bookMap["dateModified"] = modTime.toString("dd/MM/yyyy");
        bookMap["modifiedIso"] = modTime.toString("yyyy-MM-dd hh:mm:ss");
        bookMap["modifiedTimestamp"] = (qint64)modTime.toTime_t();
        bookMap["progress"] = savedProg;
        bookMap["progressStr"] = savedProg > 0 ? QString("%1%").arg(savedProg) : QString("0%");
        bookMap["isCompleted"] = (savedProg >= 100);
        bookMap["isFavorite"] = m_prefs.favorites.contains(absPath);
        bookMap["isReadingList"] = m_prefs.readingList.contains(absPath);
        bookMap["isOpened"] = m_prefs.opened.contains(absPath);
        if (dirtyMeta) bookMap["_dirtyMeta"] = true;
        if (dirtySeries) bookMap["_dirtySeries"] = true;

        books.append(bookMap);
    }

    QFileInfoList dirEntries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &subDir, dirEntries) {
        if (m_cancelled) return;
        if (shouldSkipScanDir(subDir)) {
            continue;
        }
        scanDir(subDir.absoluteFilePath(), depth + 1, visitedDirs, addedPaths, books);
    }
}

void ScanWorker::run()
{
    QList<QVariantMap> books;
    QSet<QString> addedPaths;
    QSet<QString> visitedDirs;

    foreach (const QString &rootPath, m_searchRoots) {
        if (m_cancelled) break;
        scanDir(rootPath, 0, visitedDirs, addedPaths, books);
    }

    if (!m_cancelled || (m_emitPartial && !books.isEmpty())) {
        emit scanCompleted(books);
    }
}

// =============================================================================
// CoverExtractWorker Implementation (Asynchronous Background Cover Extraction)
// =============================================================================

CoverExtractWorker::CoverExtractWorker(const QStringList &bookPaths, const QString &cacheDir, QObject *parent)
    : QThread(parent)
    , m_bookPaths(bookPaths)
    , m_cacheDir(cacheDir)
    , m_cancelled(0)
{
}

CoverExtractWorker::~CoverExtractWorker()
{
    cancel();
    wait();
}

void CoverExtractWorker::cancel()
{
    m_cancelled.fetchAndStoreOrdered(1);
}

void CoverExtractWorker::run()
{
    foreach (const QString &path, m_bookPaths) {
        if (m_cancelled) break;

        QFileInfo fi(path);
        if (!fi.exists()) continue;
        if (fi.size() > 80 * 1024 * 1024) continue; // Skip files >80MB

        QString ext = fi.suffix().toLower();
        QString hash = QString(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Md5).toHex());
        QString existingCover = findCachedCoverFile(m_cacheDir, hash);
        QString cachedNoCover = m_cacheDir + "/" + hash + ".nocover";

        if (!existingCover.isEmpty()) {
            emit coverExtracted(path, existingCover);
            continue;
        }
        if (QFile::exists(cachedNoCover)) {
            continue;
        }

        QString cover;
        if (ext == "epub") {
            EpubParser ep;
            if (ep.open(path)) {
                cover = ep.extractCover(m_cacheDir);
                ep.close();
            }
        } else if (ext == "mobi" || ext == "azw" || ext == "azw3" || ext == "kf8" || ext == "prc") {
            MobiParser mp;
            if (mp.open(path)) {
                cover = mp.extractCover(m_cacheDir);
                mp.close();
            }
        } else if (ext == "fb2") {
            Fb2Parser fb;
            if (fb.open(path)) {
                cover = fb.extractCover(m_cacheDir);
                fb.close();
            }
        } else if (ext == "cbz") {
            CbzParser cp;
            if (cp.open(path)) {
                cover = cp.extractCover(m_cacheDir);
                cp.close();
            }
        }

        if (!cover.isEmpty() && QFile::exists(cover)) {
            emit coverExtracted(path, cover);
        } else {
            // Write marker file to avoid re-attempting corrupt or coverless books
            QFile noCov(cachedNoCover);
            if (noCov.open(QIODevice::WriteOnly)) {
                noCov.close();
            }
        }

        // Yield 15ms so GUI thread remains 100% responsive
        msleep(15);
    }
}
