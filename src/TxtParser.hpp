#ifndef TXTPARSER_HPP
#define TXTPARSER_HPP

#include <QString>
#include <QStringList>
#include <QList>
#include <QFileInfo>
#include "BookChapter.hpp"

class TxtParser {
public:
    TxtParser();
    ~TxtParser();

    bool open(const QString &filePath);
    void close();

    QString title() const { return m_title; }
    QString author() const { return m_author; }
    QString coverPath() const { return m_coverPath; }
    QList<EpubChapter> chapters() const { return m_chapters; }
    int chapterCount() const { return m_chapters.size(); }

    QString getChapterContent(int index);
    QString extractCover(const QString &cacheDir);

private:
    QString m_filePath;
    QString m_title;
    QString m_author;
    QString m_coverPath;
    QList<EpubChapter> m_chapters;
    QStringList m_chapterTexts;

    QString detectAndDecode(const QByteArray &data);
    QString escapeHtml(const QString &text);
};

#endif // TXTPARSER_HPP
