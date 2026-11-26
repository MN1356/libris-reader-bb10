#ifndef CBZPARSER_HPP
#define CBZPARSER_HPP

#include <QString>
#include <QStringList>
#include <QList>
#include <QFile>
#include <QHash>
#include "miniz.h"
#include "BookChapter.hpp"
#include "EpubParser.hpp"

class CbzParser {
public:
    CbzParser();
    ~CbzParser();

    bool open(const QString &filePath);
    void close();
    bool isOpen() const { return m_zipOpen; }

    QString title() const { return m_title; }
    QString author() const { return m_author; }
    QString series() const { return m_series; }
    int seriesIndex() const { return m_seriesIndex; }
    QString coverPath() const { return m_coverPath; }
    QString description() const { return m_description; }
    QString genre() const { return m_genre; }
    QList<BookChapter> chapters() const { return m_chapters; }
    int chapterCount() const { return m_chapters.size(); }
    QList<EpubTocEntry> tocEntries() const { return m_tocEntries; }

    QString getChapterContent(int index, const QString &cacheDir = QString());
    QString extractCover(const QString &cacheDir);

private:
    QString m_filePath;
    QString m_title;
    QString m_author;
    QString m_series;
    int m_seriesIndex;
    QString m_description;
    QString m_genre;
    QString m_coverPath;
    QString m_coverEntryName;
    QList<BookChapter> m_chapters;
    QList<EpubTocEntry> m_tocEntries;

    mz_zip_archive m_zip;
    bool m_zipOpen;
    QFile m_zipFile;
    QHash<int, QString> m_contentCache;
    QList<int> m_cacheLru;

    QByteArray readZipEntry(const QString &entryName);
    void parseComicInfo(const QByteArray &xmlData);
    static bool naturalLessThan(const QString &s1, const QString &s2);
};

#endif // CBZPARSER_HPP
