#ifndef FB2PARSER_HPP
#define FB2PARSER_HPP

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include "BookChapter.hpp"
#include "pugixml.hpp"

class Fb2Parser {
public:
    Fb2Parser();
    ~Fb2Parser();

    bool open(const QString &filePath);
    void close();

    QString title() const { return m_title; }
    QString author() const { return m_author; }
    QString series() const { return m_series; }
    int seriesIndex() const { return m_seriesIndex; }
    QString coverPath() const { return m_coverPath; }
    QString description() const { return m_description; }
    QString genre() const { return m_genre; }
    QList<EpubChapter> chapters() const { return m_chapters; }
    int chapterCount() const { return m_chapters.size(); }

    QString getChapterContent(int index);
    QString notesHtml() const { return m_notesHtml; }
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
    QString m_coverBinaryId;
    QList<EpubChapter> m_chapters;
    QStringList m_chapterHtmls;
    QString m_notesHtml;
    QMap<QString, QString> m_binaries; // id -> base64 data
    int m_renderDepth;

    void parseTitleInfo(const pugi::xml_node &titleInfoNode);
    void parseBinaries(const pugi::xml_node &root);
    void parseBody(const pugi::xml_node &body);
    void parseNotesBody(const pugi::xml_node &body);
    QString renderSectionToHtml(const pugi::xml_node &section, QString &outTitle);
    QString renderNodeChildren(const pugi::xml_node &node);
    QString renderNode(const pugi::xml_node &node);
    QString escapeHtml(const QString &text);
};

#endif // FB2PARSER_HPP
