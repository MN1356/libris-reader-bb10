#ifndef MOBIPARSER_HPP
#define MOBIPARSER_HPP

#include <QString>
#include <QStringList>
#include <QList>
#include <QVector>
#include <QFile>
#include <QHash>
#include "BookChapter.hpp"
#include "EpubParser.hpp"

class MobiParser {
public:
    MobiParser();
    ~MobiParser();

    bool open(const QString &filePath);
    void close();
    bool isOpen() const { return m_file.isOpen(); }

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
    QString rawChapterHtml(int index) const;
    QString extractCover(const QString &cacheDir);
    bool isKf8() const { return m_isKf8; }
    QString bookCss() const { return m_css; }

private:
    QString m_filePath;
    QString m_title;
    QString m_author;
    QString m_series;
    int m_seriesIndex;
    QString m_description;
    QString m_genre;
    QString m_coverPath;
    int m_coverRecordIndex;
    quint32 m_firstImageIndex;
    bool m_isKf8;
    int m_kf8BoundaryIndex;
    QString m_css;

    QList<BookChapter> m_chapters;
    QList<EpubTocEntry> m_tocEntries;
    QStringList m_chapterHtmls;
    QHash<int, QString> m_resolvedChapterCache;
    QList<int> m_cacheLru;

    QFile m_file;
    QVector<quint32> m_recordOffsets;
    quint16 m_compression;
    quint32 m_textEncoding;
    quint16 m_extraFlags;
    quint32 m_huffRecordIndex;
    quint32 m_huffRecordCount;

    // HUFF / CDIC decompression state
    bool m_huffLoaded;
    struct HuffEntry {
        quint8 codelen;
        bool term;
        quint64 maxcode;
    };
    QVector<HuffEntry> m_dict1;
    quint64 m_mincode[33];
    quint64 m_maxcode[33];
    QVector<QByteArray> m_dictionary;
    QVector<quint8> m_dictFlag; // 1 = phrase is already literal text

    QByteArray readRecord(int index);
    bool loadHuffTables();
    QByteArray decompressRecord(const QByteArray &rec);
    static QByteArray decompressPalmDoc(const unsigned char *src, int len);
    QByteArray decompressHuff(const unsigned char *src, int len);
    QByteArray huffUnpack(const unsigned char *src, int len, int depth);

    void parseHeader();
    void parseExth(const QByteArray &rec0, quint32 exthOffset);
    bool parseKf8Section(int kf8HeaderRecIdx);
    void parseFdstRecord(int fdstRecIdx, const QByteArray &fullData, QByteArray &xhtmlData, QString &cssData);
    void parseHtmlIntoChapters(const QString &fullHtml);
    QString getOrCacheImage(int recNum, const QString &cacheDir);
    QString resolveEmbeddedImages(const QString &html, const QString &cacheDir = QString());
    static QString extractChapterHeading(const QString &htmlChunk, int chapterIndex);
};

#endif // MOBIPARSER_HPP
