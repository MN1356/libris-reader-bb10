#ifndef EPUBPARSER_HPP
#define EPUBPARSER_HPP

#include <QString>
#include <QStringList>
#include <QList>
#include <QPair>
#include <QHash>
#include <QFile>
#include "miniz.h"
#include "BookChapter.hpp"

struct EpubTocEntry {
    QString title;
    int chapterIndex;
    QString anchor;
    int level;
    QString href;
};

class EpubParser {
public:
    EpubParser();
    ~EpubParser();

    bool open(const QString &filePath);
    void close();
    bool isOpen() const { return m_zipOpen; }

    QString title() const { return m_title; }
    QString author() const { return m_author; }
    QString series() const { return m_series; }
    int seriesIndex() const { return m_seriesIndex; }
    QString coverPath() const { return m_coverPath; } // Extracted cover image path
    QString description() const { return m_description; }
    QString genre() const { return m_genre; }
    QList<EpubChapter> chapters() const { return m_chapters; }
    int chapterCount() const { return m_chapters.size(); }
    QList<EpubTocEntry> tocEntries() const { return m_tocEntries; }

    // Read full XHTML/HTML of a chapter by index (with optional image resolution)
    QString getChapterContent(int index, const QString &cacheDir = QString());

    // Read raw XHTML/HTML of any archive entry by path (including non-spine footnote files)
    QString readHtmlByPath(const QString &entryPath);
    QString opfDir() const { return m_opfDir; }

    // Extract real human-readable chapter title from chapter content (or anchor point)
    QString extractChapterTitle(int chapterIndex, const QString &anchor = QString());

    // Extract cover image to a cache folder for display in QML
    QString extractCover(const QString &cacheDir);

    // Inlined original book stylesheets with resolved local font URLs
    QString bookCss(const QString &cacheDir = QString());

private:
    QString m_filePath;
    QString m_title;
    QString m_author;
    QString m_series;
    int m_seriesIndex;
    QString m_description;
    QString m_genre;
    QString m_coverHref;
    QString m_coverPath;
    QString m_opfDir;
    QList<EpubChapter> m_chapters;
    QList<EpubTocEntry> m_tocEntries;
    QStringList m_cssFiles;
    QString m_resolvedBookCss;

    // Persistent zip archive in memory while book is open
    mz_zip_archive m_zip;
    bool m_zipOpen;
    QFile m_zipFile;
    QHash<QString, mz_uint> m_zipIndexMap;     // O(1) filename lookup
    QHash<int, QString> m_chapterContentCache; // In-memory chapter content cache
    QList<int> m_chapterCacheLru;              // LRU order tracking

    int findZipEntry(const QString &entryName);
    QByteArray readZipEntry(const QString &entryName);
};

#endif // EPUBPARSER_HPP
