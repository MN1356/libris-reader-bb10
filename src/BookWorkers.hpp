#ifndef BOOKWORKERS_HPP
#define BOOKWORKERS_HPP

#include <QThread>
#include <QAtomicInt>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QSet>
#include <QHash>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QCryptographicHash>
#include <QMetaType>
#include "EpubParser.hpp"
#include "TxtParser.hpp"
#include "Fb2Parser.hpp"
#include "CbzParser.hpp"
#include "MobiParser.hpp"
#include "PdfParser.hpp"

inline bool isSupportedBookExt(const QString &extLower)
{
    return (extLower == "epub" || extLower == "txt"  || extLower == "md"   ||
            extLower == "markdown" || extLower == "html" || extLower == "htm" ||
            extLower == "fb2"  || extLower == "pdf"  || extLower == "djvu" ||
            extLower == "djv"  || extLower == "cbz"  || extLower == "mobi" ||
            extLower == "azw"  || extLower == "azw3" || extLower == "kf8"  ||
            extLower == "prc");
}

inline bool shouldSkipScanDir(const QFileInfo &subDir)
{
    QString sName = subDir.fileName().toLower();
    if (sName.startsWith(".") || sName == "lost.dir" ||
        sName == "dcim" || sName == "camera" || sName == "photos" ||
        sName == "music" || sName == "videos" || sName == "voice" ||
        sName == "appdata" || sName == "cache" || sName == "system" ||
        sName == "system volume information" || sName == "$recycle.bin" ||
        sName == "recycle.bin" || sName == "nvram" || sName == "barinstaller" ||
        sName == "logs" || sName == "tmp" || sName == "temp" ||
        sName == "obb") {
        return true;
    }
    QString absLower = subDir.absoluteFilePath().toLower();
    if (absLower.endsWith("/android/data") || absLower.endsWith("/android/obb")) {
        return true;
    }
    return false;
}

struct SearchParams {
    QString query;
    QString format;
    QString filePath;
    QString currentPdfText;
    int currentChapterIndex;
    int maxTotalResults;
    QString scope;

    SearchParams() : currentChapterIndex(0), maxTotalResults(150), scope("full") {}
};

class SearchWorker : public QThread
{
    Q_OBJECT
public:
    explicit SearchWorker(const SearchParams &params, QObject *parent = 0);
    ~SearchWorker();
    void cancel();

signals:
    void searchCompleted(const QVariantList &results);

protected:
    void run();

private:
    SearchParams m_params;
    QAtomicInt m_cancelled;

    static QString extractSnippet(const QString &text, int matchPos, int matchLen, const QString &query = QString());
};

struct ScanPrefsSnapshot {
    QSet<QString> hiddenPaths;
    QHash<QString, QString> cachedTitles;
    QHash<QString, QString> cachedAuthors;
    QHash<QString, QString> cachedSeries;
    QHash<QString, int> cachedSeriesIdx;
    QSet<QString> favorites;
    QSet<QString> readingList;
    QSet<QString> opened;
    QHash<QString, int> progressPct;
    QString lastBookPath;
    QString lastBookTitle;
    QString lastBookAuthor;
};

class ScanWorker : public QThread
{
    Q_OBJECT
public:
    explicit ScanWorker(const QStringList &searchRoots, const QString &cacheDir, QObject *parent = 0);
    ~ScanWorker();
    void setPrefsSnapshot(const ScanPrefsSnapshot &prefs);
    void cancel(bool emitPartial = false);

signals:
    void scanCompleted(const QList<QVariantMap> &allBooks);

protected:
    void run();

private:
    void scanDir(const QString &dirPath, int depth, QSet<QString> &visitedDirs, QSet<QString> &addedPaths, QList<QVariantMap> &books);
    void extractSeriesFromTitleOrFile(const QString &title, const QString &rawFileName, QString &outSeries, int &outSeriesIndex);

    QStringList m_searchRoots;
    QString m_cacheDir;
    ScanPrefsSnapshot m_prefs;
    QAtomicInt m_cancelled;
    QAtomicInt m_emitPartial;
};

class CoverExtractWorker : public QThread
{
    Q_OBJECT
public:
    explicit CoverExtractWorker(const QStringList &bookPaths, const QString &cacheDir, QObject *parent = 0);
    ~CoverExtractWorker();
    void cancel();

signals:
    void coverExtracted(const QString &filePath, const QString &coverPath);

protected:
    void run();

private:
    QStringList m_bookPaths;
    QString m_cacheDir;
    QAtomicInt m_cancelled;
};

Q_DECLARE_METATYPE(QList<QVariantMap>)

#endif // BOOKWORKERS_HPP
