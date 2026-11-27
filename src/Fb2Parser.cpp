#include "Fb2Parser.hpp"
#include "ImageUriUtil.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QByteArray>
#include <QTextCodec>
#include <QDebug>

Fb2Parser::Fb2Parser()
    : m_title("Tài liệu FB2")
    , m_author("Không rõ tác giả")
    , m_seriesIndex(0)
    , m_renderDepth(0)
{
}

Fb2Parser::~Fb2Parser()
{
    close();
}

void Fb2Parser::close()
{
    m_filePath.clear();
    m_title.clear();
    m_author.clear();
    m_series.clear();
    m_seriesIndex = 0;
    m_description.clear();
    m_genre.clear();
    m_coverPath.clear();
    m_coverBinaryId.clear();
    m_chapters.clear();
    m_chapterHtmls.clear();
    m_notesHtml.clear();
    m_binaries.clear();
}

QString Fb2Parser::escapeHtml(const QString &text)
{
    QString escaped = text;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\"", "&quot;");
    return escaped;
}

void Fb2Parser::parseTitleInfo(const pugi::xml_node &titleInfo)
{
    if (!titleInfo) return;

    pugi::xml_node titleNode = titleInfo.child("book-title");
    if (titleNode) {
        m_title = QString::fromUtf8(titleNode.child_value()).trimmed();
    }

    // Authors
    QStringList authorNames;
    for (pugi::xml_node a = titleInfo.child("author"); a; a = a.next_sibling("author")) {
        QString first = QString::fromUtf8(a.child("first-name").child_value()).trimmed();
        QString middle = QString::fromUtf8(a.child("middle-name").child_value()).trimmed();
        QString last = QString::fromUtf8(a.child("last-name").child_value()).trimmed();

        QString full;
        if (!first.isEmpty()) full += first;
        if (!middle.isEmpty()) full += (full.isEmpty() ? "" : " ") + middle;
        if (!last.isEmpty()) full += (full.isEmpty() ? "" : " ") + last;

        if (!full.isEmpty()) authorNames << full;
    }
    if (!authorNames.isEmpty()) {
        m_author = authorNames.join(", ");
    }

    // Cover image id
    pugi::xml_node coverImg = titleInfo.child("coverpage").child("image");
    if (coverImg) {
        pugi::xml_attribute hrefAttr = coverImg.attribute("l:href");
        if (!hrefAttr) hrefAttr = coverImg.attribute("xlink:href");
        if (!hrefAttr) hrefAttr = coverImg.attribute("href");

        if (hrefAttr) {
            QString hrefVal = QString::fromUtf8(hrefAttr.value()).trimmed();
            if (hrefVal.startsWith('#')) {
                hrefVal = hrefVal.mid(1);
            }
            m_coverBinaryId = hrefVal;
        }
    }

    // Genre
    QStringList genres;
    for (pugi::xml_node g = titleInfo.child("genre"); g; g = g.next_sibling("genre")) {
        QString gStr = QString::fromUtf8(g.child_value()).trimmed();
        if (!gStr.isEmpty()) genres << gStr;
    }
    m_genre = genres.join(", ");

    // Series (Sequence)
    m_series.clear();
    m_seriesIndex = 0;
    pugi::xml_node seqNode = titleInfo.child("sequence");
    if (seqNode) {
        m_series = QString::fromUtf8(seqNode.attribute("name").value()).trimmed();
        m_seriesIndex = QString::fromUtf8(seqNode.attribute("number").value()).toInt();
    }

    // Annotation (Description)
    pugi::xml_node annot = titleInfo.child("annotation");
    if (annot) {
        QString descText;
        for (pugi::xml_node p = annot.child("p"); p; p = p.next_sibling("p")) {
            QString pText = QString::fromUtf8(p.child_value()).trimmed();
            if (!pText.isEmpty()) {
                if (!descText.isEmpty()) descText += "\n\n";
                descText += pText;
            }
        }
        if (descText.isEmpty()) {
            descText = QString::fromUtf8(annot.child_value()).trimmed();
        }
        m_description = descText;
    }
}

void Fb2Parser::parseBinaries(const pugi::xml_node &root)
{
    for (pugi::xml_node bin = root.child("binary"); bin; bin = bin.next_sibling("binary")) {
        QString id = QString::fromUtf8(bin.attribute("id").value()).trimmed();
        QString data = QString::fromLatin1(bin.child_value()).remove(QChar('\n')).remove(QChar('\r')).remove(QChar(' ')).remove(QChar('\t'));
        if (!id.isEmpty() && !data.isEmpty()) {
            m_binaries.insert(id, data);
        }
    }
}

QString Fb2Parser::renderNodeChildren(const pugi::xml_node &node)
{
    QString out;
    for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
        out += renderNode(child);
    }
    return out;
}

static QString fb2ImageMime(const QString &b64)
{
    if (b64.startsWith("iVBOR")) return "image/png";
    if (b64.startsWith("R0lG")) return "image/gif";
    if (b64.startsWith("UklG")) return "image/webp";
    return "image/jpeg";
}

QString Fb2Parser::renderNode(const pugi::xml_node &child)
{
    if (m_renderDepth >= 64) {
        return QString();
    }
    struct DepthGuard {
        int &d;
        DepthGuard(int &x) : d(x) { ++d; }
        ~DepthGuard() { --d; }
    } depthGuard(m_renderDepth);

    QString tag = QString::fromLatin1(child.name()).toLower();

    if (tag.isEmpty()) {
        // Text node
        return escapeHtml(QString::fromUtf8(child.value()));
    } else if (tag == "p") {
        QString pId = QString::fromUtf8(child.attribute("id").value()).trimmed();
        QString idAttr = pId.isEmpty() ? QString() : QString(" id=\"%1\"").arg(escapeHtml(pId));
        return QString("<p%1>%2</p>").arg(idAttr, renderNodeChildren(child));
    } else if (tag == "a") {
        pugi::xml_attribute hrefAttr = child.attribute("l:href");
        if (!hrefAttr) hrefAttr = child.attribute("xlink:href");
        if (!hrefAttr) hrefAttr = child.attribute("href");
        QString hrefVal = hrefAttr ? QString::fromUtf8(hrefAttr.value()).trimmed() : QString();
        QString typeVal = QString::fromUtf8(child.attribute("type").value()).trimmed().toLower();
        QString inner = renderNodeChildren(child);
        if (!hrefVal.isEmpty()) {
            QString clsAttr = (typeVal == "note" || hrefVal.startsWith('#')) ? " class=\"noteref\"" : QString();
            QString aId = QString::fromUtf8(child.attribute("id").value()).trimmed();
            QString idAttr = aId.isEmpty() ? QString() : QString(" id=\"%1\"").arg(escapeHtml(aId));
            return QString("<a href=\"%1\"%2%3>%4</a>").arg(escapeHtml(hrefVal), clsAttr, idAttr, inner);
        }
        return inner;
    } else if (tag == "strong") {
        return QString("<strong>%1</strong>").arg(renderNodeChildren(child));
    } else if (tag == "emphasis") {
        return QString("<em>%1</em>").arg(renderNodeChildren(child));
    } else if (tag == "strikethrough") {
        return QString("<s>%1</s>").arg(renderNodeChildren(child));
    } else if (tag == "sub") {
        return QString("<sub>%1</sub>").arg(renderNodeChildren(child));
    } else if (tag == "sup") {
        return QString("<sup>%1</sup>").arg(renderNodeChildren(child));
    } else if (tag == "code") {
        return QString("<code>%1</code>").arg(renderNodeChildren(child));
    } else if (tag == "empty-line") {
        return "<br/>";
    } else if (tag == "subtitle") {
        return QString("<h3 class=\"fb2-subtitle\">%1</h3>").arg(renderNodeChildren(child));
    } else if (tag == "epigraph" || tag == "cite" || tag == "poem" || tag == "stanza") {
        return QString("<blockquote class=\"fb2-epigraph\">%1</blockquote>").arg(renderNodeChildren(child));
    } else if (tag == "v") {
        return QString("<p>%1</p>").arg(renderNodeChildren(child));
    } else if (tag == "text-author") {
        return QString("<p style=\"text-align:right;font-style:italic;\">%1</p>").arg(renderNodeChildren(child));
    } else if (tag == "title") {
        return QString("<h3 class=\"fb2-subtitle\">%1</h3>").arg(renderNodeChildren(child));
    } else if (tag == "image") {
        pugi::xml_attribute href = child.attribute("l:href");
        if (!href) href = child.attribute("xlink:href");
        if (!href) href = child.attribute("href");
        if (href) {
            QString imgId = QString::fromUtf8(href.value()).trimmed();
            if (imgId.startsWith('#')) imgId = imgId.mid(1);
            if (m_binaries.contains(imgId)) {
                const QString b64 = m_binaries.value(imgId);
                return QString("<img src=\"data:%1;base64,%2\" style=\"max-width: 100%; height: auto; display: block; margin: 12px auto; border-radius: 4px;\"/>")
                       .arg(fb2ImageMime(b64)).arg(b64);
            }
        }
        return QString();
    } else if (tag == "section") {
        QString secId = QString::fromUtf8(child.attribute("id").value()).trimmed();
        QString nestedTitle;
        QString inner = renderSectionToHtml(child, nestedTitle);
        QString out;
        if (!nestedTitle.isEmpty()) {
            out += QString("<h3 class=\"fb2-subtitle\">%1</h3>").arg(escapeHtml(nestedTitle));
        }
        out += inner;
        if (!secId.isEmpty()) {
            return QString("<div id=\"%1\">%2</div>").arg(escapeHtml(secId), out);
        }
        return out;
    }
    // Generic recursion (links, styles, unknown wrappers keep their text)
    return renderNodeChildren(child);
}

QString Fb2Parser::renderSectionToHtml(const pugi::xml_node &section, QString &outTitle)
{
    outTitle.clear();
    QString bodyContent;

    // Check section title
    pugi::xml_node titleNode = section.child("title");
    if (titleNode) {
        for (pugi::xml_node tp = titleNode.child("p"); tp; tp = tp.next_sibling("p")) {
            if (!outTitle.isEmpty()) outTitle += " ";
            outTitle += QString::fromUtf8(tp.child_value()).trimmed();
        }
        if (outTitle.isEmpty()) {
            outTitle = QString::fromUtf8(titleNode.child_value()).trimmed();
        }
    }

    pugi::xml_node subNode = section.child("subtitle");
    if (subNode) {
        QString subText;
        for (pugi::xml_node sp = subNode.child("p"); sp; sp = sp.next_sibling("p")) {
            if (!subText.isEmpty()) subText += " ";
            subText += QString::fromUtf8(sp.child_value()).trimmed();
        }
        if (subText.isEmpty()) {
            subText = QString::fromUtf8(subNode.child_value()).trimmed();
        }
        if (!subText.isEmpty()) {
            if (outTitle.isEmpty()) {
                outTitle = subText;
            } else if (outTitle.length() < 35 || outTitle.contains(QRegExp("^\\s*(?:Chương|Chapter)\\s+[0-9]+", Qt::CaseInsensitive))) {
                if (outTitle.endsWith(':') || outTitle.endsWith('-')) outTitle += " " + subText;
                else outTitle += ": " + subText;
            }
        }
    }

    for (pugi::xml_node child = section.first_child(); child; child = child.next_sibling()) {
        QString tag = QString::fromLatin1(child.name()).toLower();
        if (tag == "title") continue; // Already handled above

        if (tag == "subtitle" && child == subNode) continue; // Already merged into the chapter title above
        bodyContent += renderNode(child);
    }

    return bodyContent;
}

void Fb2Parser::parseBody(const pugi::xml_node &body)
{
    if (!body) return;

    int sectionIdx = 0;
    for (pugi::xml_node sec = body.child("section"); sec; sec = sec.next_sibling("section")) {
        sectionIdx++;
        QString chTitle;
        QString htmlContent = renderSectionToHtml(sec, chTitle);

        if (chTitle.isEmpty()) {
            chTitle = QString("Chương %1").arg(sectionIdx);
        }

        EpubChapter c;
        c.id = QString("sec_%1").arg(sectionIdx);
        c.title = chTitle;
        c.href = QString("fb2://%1").arg(sectionIdx);
        m_chapters.append(c);

        QString secId = QString::fromUtf8(sec.attribute("id").value()).trimmed();
        QString fullHtml;
        fullHtml += "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/>";
        fullHtml += "<style>";
        fullHtml += "body { font-family: -apple-system, sans-serif; line-height: 1.6; padding: 12px; margin: 0; word-wrap: break-word; }";
        fullHtml += "h2.fb2-title { text-align: center; margin: 1.2em 0 1em 0; font-size: 1.3em; border-bottom: 1px solid rgba(128,128,128,0.2); padding-bottom: 0.5em; }";
        fullHtml += "h3.fb2-subtitle { text-align: center; margin: 1em 0 0.8em 0; font-size: 1.1em; font-style: italic; opacity: 0.85; }";
        fullHtml += "p { text-indent: 1.8em; margin: 0.6em 0; }";
        fullHtml += "blockquote.fb2-epigraph { font-style: italic; margin: 1.5em 2em; padding-left: 1em; border-left: 3px solid rgba(128,128,128,0.4); opacity: 0.9; }";
        fullHtml += "</style></head><body>";
        if (!secId.isEmpty()) fullHtml += QString("<div id=\"%1\">").arg(escapeHtml(secId));
        fullHtml += QString("<h2 class=\"fb2-title\">%1</h2>").arg(escapeHtml(chTitle));
        fullHtml += htmlContent;
        if (!secId.isEmpty()) fullHtml += "</div>";
        fullHtml += "</body></html>";

        m_chapterHtmls.append(fullHtml);
    }

    // If body had no sections, render all paragraphs as a single chapter
    if (m_chapters.isEmpty()) {
        QString htmlContent = renderNodeChildren(body);
        EpubChapter c;
        c.id = "main";
        c.title = m_title;
        c.href = "fb2://main";
        m_chapters.append(c);

        QString fullHtml;
        fullHtml += "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/>";
        fullHtml += "<style>";
        fullHtml += "body { font-family: -apple-system, sans-serif; line-height: 1.6; padding: 12px; margin: 0; word-wrap: break-word; }";
        fullHtml += "h2.fb2-title { text-align: center; margin: 1.2em 0 1em 0; font-size: 1.3em; }";
        fullHtml += "p { text-indent: 1.8em; margin: 0.6em 0; }";
        fullHtml += "</style></head><body>";
        fullHtml += QString("<h2 class=\"fb2-title\">%1</h2>").arg(escapeHtml(m_title));
        fullHtml += htmlContent;
        fullHtml += "</body></html>";

        m_chapterHtmls.append(fullHtml);
    }
}

void Fb2Parser::parseNotesBody(const pugi::xml_node &body)
{
    if (!body) return;

    for (pugi::xml_node child = body.first_child(); child; child = child.next_sibling()) {
        QString tag = QString::fromLatin1(child.name()).toLower();
        if (tag == "section") {
            QString noteId = QString::fromUtf8(child.attribute("id").value()).trimmed();
            QString noteTitle;
            QString inner = renderSectionToHtml(child, noteTitle);
            if (!noteId.isEmpty()) {
                m_notesHtml += QString("<div class=\"footnote\" id=\"%1\">").arg(escapeHtml(noteId));
                if (!noteTitle.isEmpty()) {
                    m_notesHtml += QString("<p><strong>%1</strong></p>").arg(escapeHtml(noteTitle));
                }
                m_notesHtml += inner + "</div>\n";
            } else {
                m_notesHtml += inner + "\n";
            }
        } else if (tag == "p" || tag == "cite" || tag == "poem") {
            m_notesHtml += renderNode(child) + "\n";
        }
    }
}

bool Fb2Parser::open(const QString &filePath)
{
    close();
    m_filePath = filePath;

    QFileInfo fi(filePath);
    if (!fi.exists() || fi.size() <= 0 || fi.size() > 40 * 1024 * 1024) {
        qWarning() << "[Fb2Parser] Missing or oversized FB2 file:" << filePath << fi.size();
        return false;
    }

    pugi::xml_document doc;
    // pugixml only understands UTF-8/UTF-16/Latin-1: transcode legacy encodings
    // (windows-1251, koi8-r, ...) which are very common in FB2 files.
    QByteArray rawXml;
    {
        QFile xmlFile(filePath);
        if (!xmlFile.open(QIODevice::ReadOnly)) {
            qWarning() << "[Fb2Parser] Cannot open FB2 file:" << filePath;
            return false;
        }
        rawXml = xmlFile.readAll();
        xmlFile.close();
    }
    {
        QByteArray head = rawXml.left(200).toLower();
        int encPos = head.indexOf("encoding=");
        if (head.startsWith("<?xml") && encPos != -1) {
            int q1 = encPos + 9;
            if (q1 < head.size()) {
                char quote = head.at(q1);
                int q2 = head.indexOf(quote, q1 + 1);
                if ((quote == '"' || quote == '\'') && q2 > q1) {
                    QByteArray encName = head.mid(q1 + 1, q2 - q1 - 1).trimmed();
                    if (encName != "utf-8" && encName != "utf8" && !encName.isEmpty()) {
                        QTextCodec *codec = QTextCodec::codecForName(encName);
                        if (codec) {
                            QByteArray utf8 = codec->toUnicode(rawXml).toUtf8();
                            int declEnd = utf8.indexOf("?>");
                            if (utf8.startsWith("<?xml") && declEnd != -1) {
                                utf8 = QByteArray("<?xml version=\"1.0\" encoding=\"utf-8\"?>") + utf8.mid(declEnd + 2);
                            }
                            rawXml = utf8;
                        }
                    }
                }
            }
        }
    }
    pugi::xml_parse_result result = doc.load_buffer(rawXml.constData(), rawXml.size());
    if (!result) {
        qWarning() << "[Fb2Parser] XML parse error:" << result.description() << "at offset:" << result.offset;
        return false;
    }

    pugi::xml_node root = doc.child("FictionBook");
    if (!root) {
        qWarning() << "[Fb2Parser] Not a valid FictionBook XML file:" << filePath;
        return false;
    }

    // Parse metadata
    pugi::xml_node titleInfo = root.child("description").child("title-info");
    parseTitleInfo(titleInfo);

    // Parse embedded binaries (images)
    parseBinaries(root);

    // Parse body chapters and any secondary notes/footnotes bodies
    pugi::xml_node mainBody;
    for (pugi::xml_node b = root.child("body"); b; b = b.next_sibling("body")) {
        QString bName = QString::fromUtf8(b.attribute("name").value()).trimmed().toLower();
        if (bName == "notes" || bName == "footnotes" || mainBody) {
            parseNotesBody(b);
        } else {
            mainBody = b;
            parseBody(b);
        }
    }

    // Prepend dedicated book cover chapter
    EpubChapter coverCh;
    coverCh.id = "cover";
    coverCh.title = "Bìa sách";
    coverCh.href = "fb2://cover";
    m_chapters.prepend(coverCh);
    m_chapterHtmls.prepend(QString());

    qDebug() << "[Fb2Parser] Opened:" << m_title << "by" << m_author << "Chapters:" << m_chapters.size();
    return !m_chapters.isEmpty();
}

QString Fb2Parser::getChapterContent(int index)
{
    if (index < 0 || index >= m_chapters.size()) {
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
        QString coverSrc = fileToDataUri(m_coverPath);
        if (coverSrc.isEmpty() && !m_coverBinaryId.isEmpty() && m_binaries.contains(m_coverBinaryId)) {
            const QString b64 = m_binaries.value(m_coverBinaryId);
            coverSrc = QString("data:%1;base64,%2").arg(fb2ImageMime(b64), b64);
        }
        if (coverSrc.isEmpty() && !m_coverPath.isEmpty() && QFile::exists(m_coverPath)) {
            coverSrc = QString("file://%1").arg(m_coverPath);
        }
        if (!coverSrc.isEmpty()) {
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
    if (index < m_chapterHtmls.size()) {
        return m_chapterHtmls[index];
    }
    return QString();
}

QString Fb2Parser::extractCover(const QString &cacheDir)
{
    if (m_coverBinaryId.isEmpty()) {
        return QString();
    }

    QDir().mkpath(cacheDir);
    QString hash = QString(QCryptographicHash::hash(m_filePath.toUtf8(), QCryptographicHash::Md5).toHex());
    QString existingCover = findCachedCoverFile(cacheDir, hash);
    if (!existingCover.isEmpty()) {
        m_coverPath = existingCover;
        return m_coverPath;
    }

    QString binId = m_coverBinaryId;
    if (binId.startsWith('#')) {
        binId = binId.mid(1);
    }

    if (!m_binaries.contains(binId)) {
        foreach (const QString &k, m_binaries.keys()) {
            if (k.compare(binId, Qt::CaseInsensitive) == 0 || k.contains("cover", Qt::CaseInsensitive)) {
                binId = k;
                break;
            }
        }
    }

    if (!m_binaries.contains(binId)) {
        return QString();
    }

    QByteArray imgData = QByteArray::fromBase64(m_binaries.value(binId).toLatin1());
    if (imgData.isEmpty()) {
        return QString();
    }

    QString ext = "jpg";
    if (imgData.startsWith("\x89PNG")) ext = "png";
    else if (imgData.startsWith("GIF8")) ext = "gif";
    else if (imgData.size() >= 12 && imgData.mid(0, 4) == "RIFF" && imgData.mid(8, 4) == "WEBP") ext = "webp";
    QString outPath = cacheDir + "/" + hash + "." + ext;

    QFile outFile(outPath);
    if (outFile.open(QIODevice::WriteOnly)) {
        outFile.write(imgData);
        outFile.close();
        m_coverPath = outPath;
        return m_coverPath;
    }

    return QString();
}
