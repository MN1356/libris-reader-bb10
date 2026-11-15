#include "OpdsManager.hpp"
#include "BookManager.hpp"
#include "pugixml.hpp"

#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QCryptographicHash>
#include <QDebug>
#include <QRegExp>

OpdsManager::OpdsManager(BookManager *bookManager, QObject *parent)
    : QObject(parent)
    , m_bookManager(bookManager)
    , m_nam(new QNetworkAccessManager(this))
    , m_feedModel(new bb::cascades::GroupDataModel(QStringList() << "sortKey", this))
    , m_catalogModel(new bb::cascades::GroupDataModel(QStringList() << "category" << "name", this))
    , m_isLoading(false)
    , m_isDownloading(false)
    , m_downloadProgress(0)
    , m_downloadStatus("")
    , m_currentTitle(tr("Kho sách OPDS"))
    , m_currentUrl("")
    , m_hasSearch(false)
    , m_searchUrlTemplate("")
    , m_nextPageUrl("")
    , m_statusMessage("")
    , m_isAtRoot(true)
    , m_currentUsername("")
    , m_currentPassword("")
    , m_feedReply(NULL)
    , m_downloadReply(NULL)
    , m_downloadFile(NULL)
{
    m_feedModel->setGrouping(bb::cascades::ItemGrouping::None);
    m_catalogModel->setGrouping(bb::cascades::ItemGrouping::ByFullValue);

    // Global SSL / TLS bypass across all requests dispatched by m_nam
    connect(m_nam, SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)),
            this, SLOT(onNamSslErrors(QNetworkReply*, QList<QSslError>)));

    loadCatalogs();
}

OpdsManager::~OpdsManager()
{
    if (m_feedReply) {
        m_feedReply->abort();
        m_feedReply->deleteLater();
        m_feedReply = NULL;
    }
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
        m_downloadReply = NULL;
    }
    if (m_downloadFile) {
        if (m_downloadFile->isOpen()) m_downloadFile->close();
        delete m_downloadFile;
        m_downloadFile = NULL;
    }
}

void OpdsManager::rebuildCatalogModel()
{
    m_catalogModel->clear();
    for (int i = 0; i < m_catalogs.size(); ++i) {
        QVariantMap cat = m_catalogs.at(i).toMap();
        bool isDef = cat.value("isDefault", false).toBool();
        cat["category"] = isDef ? tr("DANH MỤC CÔNG CỘNG") : tr("KHO SÁCH CÁ NHÂN (CALIBRE, KAVITA...)");
        QString rawName = cat.value("name").toString();
        QString rawDesc = cat.value("description").toString();
        if (!rawName.isEmpty()) {
            cat["name"] = tr(rawName.toUtf8().constData());
        }
        if (!rawDesc.isEmpty()) {
            cat["description"] = tr(rawDesc.toUtf8().constData());
        }
        m_catalogModel->insert(cat);
    }
}

void OpdsManager::loadCatalogs()
{
    m_catalogs.clear();
    QSettings settings("BerryLeaf", "Settings");

    int count = settings.beginReadArray("opdsCatalogs");
    if (count > 0) {
        for (int i = 0; i < count; ++i) {
            settings.setArrayIndex(i);
            QVariantMap cat;
            cat["id"] = settings.value("id").toString();
            cat["name"] = settings.value("name").toString();
            cat["url"] = settings.value("url").toString();
            // Automatically upgrade outdated HTTP OPDS endpoints to HTTPS
            if (cat["url"].toString().contains("gutenberg.org") && (cat["url"].toString().contains("m.gutenberg") || cat["url"].toString().startsWith("http://"))) {
                cat["url"] = "https://www.gutenberg.org/ebooks.opds/";
            } else if (cat["url"].toString().contains("feedbooks.com") && cat["url"].toString().startsWith("http://")) {
                cat["url"] = "https://catalog.feedbooks.com/publicdomain/browse/top.atom";
            }
            cat["username"] = settings.value("username").toString();
            cat["password"] = settings.value("password").toString();
            cat["description"] = settings.value("description").toString();
            cat["isDefault"] = settings.value("isDefault", false).toBool();
            m_catalogs.append(cat);
        }
        settings.endArray();
    } else {
        settings.endArray();
        resetDefaultCatalogs();
        return;
    }

    if (m_isAtRoot) {
        m_currentTitle = tr("Kho sách OPDS");
        emit currentTitleChanged();
    }

    rebuildCatalogModel();
    emit catalogsChanged();
}

void OpdsManager::resetDefaultCatalogs()
{
    m_catalogs.clear();

    QVariantMap cat1;
    cat1["id"] = "standardebooks";
    cat1["name"] = "Standard Ebooks";
    cat1["url"] = "https://standardebooks.org/feeds/atom/new-releases";
    cat1["description"] = tr("Sách kinh điển công cộng định dạng EPUB chuẩn đẹp");
    cat1["isDefault"] = true;
    m_catalogs.append(cat1);

    QVariantMap cat2;
    cat2["id"] = "gutenberg";
    cat2["name"] = "Project Gutenberg";
    cat2["url"] = "https://www.gutenberg.org/ebooks.opds/";
    cat2["description"] = tr("Hơn 70.000 tác phẩm sách miễn phí toàn cầu");
    cat2["isDefault"] = true;
    m_catalogs.append(cat2);

    QVariantMap cat3;
    cat3["id"] = "feedbooks";
    cat3["name"] = "Feedbooks Public Domain";
    cat3["url"] = "https://catalog.feedbooks.com/publicdomain/browse/top.atom";
    cat3["description"] = tr("Kho sách miễn phí phổ biến nhiều thể loại");
    cat3["isDefault"] = true;
    m_catalogs.append(cat3);

    QVariantMap cat4;
    cat4["id"] = "calibre_sample";
    cat4["name"] = tr("Calibre Máy Chủ (Mẫu)");
    cat4["url"] = "http://192.168.1.100:8080/opds";
    cat4["description"] = tr("Ví dụ kết nối Calibre trong mạng Wi-Fi (nhấn giữ để sửa IP)");
    cat4["isDefault"] = false;
    m_catalogs.append(cat4);

    QSettings settings("BerryLeaf", "Settings");
    settings.beginWriteArray("opdsCatalogs");
    for (int i = 0; i < m_catalogs.size(); ++i) {
        settings.setArrayIndex(i);
        const QVariantMap &cat = m_catalogs.at(i).toMap();
        settings.setValue("id", cat.value("id"));
        settings.setValue("name", cat.value("name"));
        settings.setValue("url", cat.value("url"));
        settings.setValue("username", cat.value("username", ""));
        settings.setValue("password", cat.value("password", ""));
        settings.setValue("description", cat.value("description"));
        settings.setValue("isDefault", cat.value("isDefault", false));
    }
    settings.endArray();

    rebuildCatalogModel();
    emit catalogsChanged();
}

void OpdsManager::addCatalog(const QString &name, const QString &url, const QString &username, const QString &password, const QString &description)
{
    if (url.trimmed().isEmpty()) return;

    QString cleanName = name.trimmed();
    if (cleanName.isEmpty()) {
        cleanName = QUrl(url).host();
        if (cleanName.isEmpty()) cleanName = tr("Kho sách mới");
    }

    QString id = QString("custom_%1").arg(QDateTime::currentMSecsSinceEpoch());

    QVariantMap cat;
    cat["id"] = id;
    cat["name"] = cleanName;
    cat["url"] = url.trimmed();
    cat["username"] = username.trimmed();
    cat["password"] = password.trimmed();
    cat["description"] = description.trimmed().isEmpty() ? tr("Kho sách người dùng thêm") : description.trimmed();
    cat["isDefault"] = false;

    m_catalogs.append(cat);

    QSettings settings("BerryLeaf", "Settings");
    settings.beginWriteArray("opdsCatalogs");
    for (int i = 0; i < m_catalogs.size(); ++i) {
        settings.setArrayIndex(i);
        const QVariantMap &c = m_catalogs.at(i).toMap();
        settings.setValue("id", c.value("id"));
        settings.setValue("name", c.value("name"));
        settings.setValue("url", c.value("url"));
        settings.setValue("username", c.value("username", ""));
        settings.setValue("password", c.value("password", ""));
        settings.setValue("description", c.value("description"));
        settings.setValue("isDefault", c.value("isDefault", false));
    }
    settings.endArray();

    rebuildCatalogModel();
    emit catalogsChanged();
}

void OpdsManager::updateCatalog(const QString &id, const QString &name, const QString &url, const QString &username, const QString &password, const QString &description)
{
    int idx = -1;
    for (int i = 0; i < m_catalogs.size(); ++i) {
        if (m_catalogs.at(i).toMap().value("id").toString() == id) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return;

    QVariantMap cat = m_catalogs.at(idx).toMap();
    if (!name.trimmed().isEmpty()) cat["name"] = name.trimmed();
    if (!url.trimmed().isEmpty()) cat["url"] = url.trimmed();
    cat["username"] = username.trimmed();
    cat["password"] = password.trimmed();
    if (!description.trimmed().isEmpty()) cat["description"] = description.trimmed();

    m_catalogs[idx] = cat;

    QSettings settings("BerryLeaf", "Settings");
    settings.beginWriteArray("opdsCatalogs");
    for (int i = 0; i < m_catalogs.size(); ++i) {
        settings.setArrayIndex(i);
        const QVariantMap &c = m_catalogs.at(i).toMap();
        settings.setValue("id", c.value("id"));
        settings.setValue("name", c.value("name"));
        settings.setValue("url", c.value("url"));
        settings.setValue("username", c.value("username", ""));
        settings.setValue("password", c.value("password", ""));
        settings.setValue("description", c.value("description"));
        settings.setValue("isDefault", c.value("isDefault", false));
    }
    settings.endArray();

    rebuildCatalogModel();
    emit catalogsChanged();
}

void OpdsManager::removeCatalog(const QString &id)
{
    int idx = -1;
    for (int i = 0; i < m_catalogs.size(); ++i) {
        if (m_catalogs.at(i).toMap().value("id").toString() == id) {
            idx = i;
            break;
        }
    }

    if (idx >= 0) {
        m_catalogs.removeAt(idx);

        QSettings settings("BerryLeaf", "Settings");
        settings.beginWriteArray("opdsCatalogs");
        for (int i = 0; i < m_catalogs.size(); ++i) {
            settings.setArrayIndex(i);
            const QVariantMap &c = m_catalogs.at(i).toMap();
            settings.setValue("id", c.value("id"));
            settings.setValue("name", c.value("name"));
            settings.setValue("url", c.value("url"));
            settings.setValue("username", c.value("username", ""));
            settings.setValue("password", c.value("password", ""));
            settings.setValue("description", c.value("description"));
            settings.setValue("isDefault", c.value("isDefault", false));
        }
        settings.endArray();

        rebuildCatalogModel();
        emit catalogsChanged();
    }
}

void OpdsManager::openCatalog(const QString &catalogId)
{
    QVariantMap chosenCat;
    for (int i = 0; i < m_catalogs.size(); ++i) {
        const QVariantMap cat = m_catalogs.at(i).toMap();
        if (cat.value("id").toString() == catalogId) {
            chosenCat = cat;
            break;
        }
    }

    if (chosenCat.isEmpty()) return;

    m_currentUsername = chosenCat.value("username").toString();
    m_currentPassword = chosenCat.value("password").toString();
    m_historyStack.clear();
    emit canGoBackChanged();

    openFeed(chosenCat.value("url").toString(), chosenCat.value("name").toString(), false);
}

void OpdsManager::openFeed(const QString &url, const QString &title, bool addToHistory)
{
    if (url.trimmed().isEmpty()) return;

    if (addToHistory && !m_currentUrl.isEmpty()) {
        QVariantMap hist;
        hist["url"] = m_currentUrl;
        hist["title"] = m_currentTitle;
        hist["username"] = m_currentUsername;
        hist["password"] = m_currentPassword;
        m_historyStack.append(hist);
        emit canGoBackChanged();
    }

    m_currentUrl = url.trimmed();
    if (!title.isEmpty()) {
        m_currentTitle = title;
        emit currentTitleChanged();
    }
    emit currentUrlChanged();

    m_isAtRoot = false;
    emit isAtRootChanged();

    sendFeedRequest(m_currentUrl);
}

void OpdsManager::prepareRequest(QNetworkRequest &req)
{
    req.setRawHeader("User-Agent", "Mozilla/5.0 (BlackBerry; U; BlackBerry 10; en-US) AppleWebKit/537.36 (KHTML, like Gecko) BerryLeaf/2.0");
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::AnyProtocol);
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setSslOption(QSsl::SslOptionDisableServerNameIndication, false);
    req.setSslConfiguration(sslConfig);
}

void OpdsManager::setupReply(QNetworkReply *reply)
{
    if (!reply) return;
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::AnyProtocol);
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setSslOption(QSsl::SslOptionDisableServerNameIndication, false);
    reply->setSslConfiguration(sslConfig);
    reply->ignoreSslErrors();
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply, SLOT(ignoreSslErrors()));
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));
}

void OpdsManager::sendFeedRequest(const QString &urlStr)
{
    if (m_feedReply) {
        m_feedReply->abort();
        m_feedReply->deleteLater();
        m_feedReply = NULL;
    }

    m_isLoading = true;
    emit isLoadingChanged();

    m_statusMessage = tr("Đang tải dữ liệu...");
    emit statusMessageChanged();

    QUrl url = QUrl::fromEncoded(urlStr.toUtf8());
    QNetworkRequest req(url);
    prepareRequest(req);
    req.setRawHeader("Accept", "application/atom+xml,application/xml,text/xml,*/*");

    if (!m_currentUsername.isEmpty()) {
        QString auth = QString("%1:%2").arg(m_currentUsername, m_currentPassword);
        QByteArray authData = "Basic " + auth.toUtf8().toBase64();
        req.setRawHeader("Authorization", authData);
    }

    m_feedReply = m_nam->get(req);
    setupReply(m_feedReply);
    connect(m_feedReply, SIGNAL(finished()), this, SLOT(onFeedReplyFinished()));
}

void OpdsManager::onFeedReplyFinished()
{
    if (!m_feedReply) return;

    m_isLoading = false;
    emit isLoadingChanged();

    // Check for HTTP redirects (301, 302, 303, 307)
    QVariant redirectVar = m_feedReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectVar.isNull()) {
        QUrl redirectUrl = m_feedReply->url().resolved(redirectVar.toUrl());
        m_feedReply->deleteLater();
        m_feedReply = NULL;
        qDebug() << "[OpdsManager] Following redirect to:" << redirectUrl.toString();
        m_currentUrl = redirectUrl.toString();
        emit currentUrlChanged();
        sendFeedRequest(m_currentUrl);
        return;
    }

    if (m_feedReply->error() != QNetworkReply::NoError) {
        QString err = m_feedReply->errorString();
        // Smart fallback: if HTTPS fails with SSL handshake error, attempt HTTP fallback
        if ((m_feedReply->error() == QNetworkReply::SslHandshakeFailedError || err.contains("SSL", Qt::CaseInsensitive) || err.contains("handshake", Qt::CaseInsensitive))
            && m_currentUrl.startsWith("https://", Qt::CaseInsensitive)
            && !m_feedReply->property("isHttpFallback").toBool()) {

            QString httpUrl = "http://" + m_currentUrl.mid(8);
            qWarning() << "[OpdsManager] SSL handshake error on" << m_currentUrl << "- attempting automatic fallback to:" << httpUrl;
            m_feedReply->deleteLater();
            m_feedReply = NULL;
            m_currentUrl = httpUrl;
            emit currentUrlChanged();

            m_isLoading = true;
            emit isLoadingChanged();
            m_statusMessage = tr("Đang thử lại kết nối HTTP...");
            emit statusMessageChanged();

            QUrl url = QUrl::fromEncoded(httpUrl.toUtf8());
            QNetworkRequest req(url);
            prepareRequest(req);
            req.setRawHeader("Accept", "application/atom+xml,application/xml,text/xml,*/*");

            if (!m_currentUsername.isEmpty()) {
                QString auth = QString("%1:%2").arg(m_currentUsername, m_currentPassword);
                QByteArray authData = "Basic " + auth.toUtf8().toBase64();
                req.setRawHeader("Authorization", authData);
            }

            m_feedReply = m_nam->get(req);
            m_feedReply->setProperty("isHttpFallback", true);
            setupReply(m_feedReply);
            connect(m_feedReply, SIGNAL(finished()), this, SLOT(onFeedReplyFinished()));
            return;
        }

        m_statusMessage = tr("Lỗi kết nối: %1").arg(err);
        emit statusMessageChanged();
        m_feedReply->deleteLater();
        m_feedReply = NULL;
        return;
    }

    QByteArray data = m_feedReply->readAll();
    m_feedReply->deleteLater();
    m_feedReply = NULL;

    parseAtomFeed(data);
}

void OpdsManager::parseAtomFeed(const QByteArray &data)
{
    m_feedModel->clear();
    m_hasSearch = false;
    m_searchUrlTemplate.clear();
    m_nextPageUrl.clear();

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(data.constData(), data.size(), pugi::parse_default | pugi::parse_ws_pcdata);
    if (!result) {
        m_statusMessage = tr("Không thể đọc cấu trúc danh mục OPDS XML!");
        emit statusMessageChanged();
        return;
    }

    pugi::xml_node feedNode = doc.child("feed");
    if (!feedNode) {
        // Some catalogs wrap in root element with different name or namespace
        for (pugi::xml_node n = doc.first_child(); n; n = n.next_sibling()) {
            if (QString(n.name()).toLower().contains("feed")) {
                feedNode = n;
                break;
            }
        }
    }

    if (!feedNode) {
        m_statusMessage = tr("Phản hồi không phải là nguồn OPDS Atom hợp lệ.");
        emit statusMessageChanged();
        return;
    }

    // Feed Title
    pugi::xml_node feedTitleNode = feedNode.child("title");
    if (feedTitleNode) {
        QString parsedTitle = QString::fromUtf8(feedTitleNode.text().as_string()).trimmed();
        if (!parsedTitle.isEmpty() && m_currentTitle.isEmpty()) {
            m_currentTitle = parsedTitle;
            emit currentTitleChanged();
        }
    }

    // Search link & Next page link
    for (pugi::xml_node link = feedNode.child("link"); link; link = link.next_sibling("link")) {
        QString rel = link.attribute("rel").as_string();
        QString href = link.attribute("href").as_string();
        QString type = QString(link.attribute("type").as_string()).toLower();

        if (rel == "search" || type.contains("search")) {
            m_searchUrlTemplate = resolveUrl(m_currentUrl, href);
            m_hasSearch = true;
        } else if (rel == "next") {
            m_nextPageUrl = resolveUrl(m_currentUrl, href);
        }
    }
    emit hasSearchChanged();

    QVariantList items;
    int index = 0;

    for (pugi::xml_node entry = feedNode.child("entry"); entry; entry = entry.next_sibling("entry")) {
        QString title = QString::fromUtf8(entry.child("title").text().as_string()).trimmed();
        if (title.isEmpty()) continue;

        // Author
        QString author;
        pugi::xml_node authorNode = entry.child("author");
        if (authorNode) {
            pugi::xml_node nameNode = authorNode.child("name");
            if (nameNode) {
                author = QString::fromUtf8(nameNode.text().as_string()).trimmed();
            } else {
                author = QString::fromUtf8(authorNode.text().as_string()).trimmed();
            }
        }

        // Summary / Content
        QString summary;
        pugi::xml_node sumNode = entry.child("summary");
        if (!sumNode) sumNode = entry.child("content");
        if (sumNode) {
            summary = stripHtml(QString::fromUtf8(sumNode.text().as_string()).trimmed());
        }

        // Links
        QString coverUrl;
        QString navUrl;
        QString epubUrl;
        QString pdfUrl;
        QString genericDownloadUrl;
        QString genericFormat;

        for (pugi::xml_node link = entry.child("link"); link; link = link.next_sibling("link")) {
            QString rel = link.attribute("rel").as_string();
            QString href = link.attribute("href").as_string();
            QString type = QString(link.attribute("type").as_string()).toLower();

            if (rel.contains("image") || rel.contains("thumbnail") || type.startsWith("image/")) {
                if (coverUrl.isEmpty() || rel.contains("thumbnail")) {
                    coverUrl = resolveUrl(m_currentUrl, href);
                }
            } else if (rel.contains("acquisition") || rel.contains("open-access") || type == "application/epub+zip" || type == "application/pdf") {
                QString fullHref = resolveUrl(m_currentUrl, href);
                if (type.contains("epub")) {
                    epubUrl = fullHref;
                } else if (type.contains("pdf")) {
                    pdfUrl = fullHref;
                } else if (genericDownloadUrl.isEmpty()) {
                    genericDownloadUrl = fullHref;
                    if (type.contains("text/plain")) genericFormat = "txt";
                    else if (type.contains("fb2")) genericFormat = "fb2";
                    else genericFormat = "epub";
                }
            } else if (rel == "subsection" || rel == "alternate" || type.contains("opds-catalog") || type.contains("atom+xml")) {
                if (navUrl.isEmpty()) {
                    navUrl = resolveUrl(m_currentUrl, href);
                }
            }
        }

        // Fallback for media:thumbnail or thumbnail tag (Standard Ebooks etc.)
        if (coverUrl.isEmpty()) {
            pugi::xml_node mediaThumb = entry.child("media:thumbnail");
            if (!mediaThumb) mediaThumb = entry.child("thumbnail");
            if (mediaThumb) {
                QString mUrl = mediaThumb.attribute("url").as_string();
                if (!mUrl.isEmpty()) {
                    coverUrl = resolveUrl(m_currentUrl, mUrl);
                }
            }
        }

        QVariantMap item;
        item["title"] = title;
        item["author"] = author;
        item["summary"] = summary;
        item["coverUrl"] = coverUrl;

        if (!epubUrl.isEmpty() || !pdfUrl.isEmpty() || !genericDownloadUrl.isEmpty()) {
            item["entryType"] = "book";
            item["isBook"] = true;
            QString mainUrl = !epubUrl.isEmpty() ? epubUrl : (!pdfUrl.isEmpty() ? pdfUrl : genericDownloadUrl);
            QString mainFmt = !epubUrl.isEmpty() ? "epub" : (!pdfUrl.isEmpty() ? "pdf" : (genericFormat.isEmpty() ? "epub" : genericFormat));
            item["downloadUrl"] = mainUrl;
            item["format"] = mainFmt;
            item["formatBadge"] = mainFmt.toUpper();
            item["epubUrl"] = epubUrl;
            item["pdfUrl"] = pdfUrl;
            item["subtext"] = !author.isEmpty() ? author : mainFmt.toUpper();
            item["sortKey"] = QString("2_%1").arg(index, 6, 10, QChar('0'));
        } else {
            item["entryType"] = "navigation";
            item["isBook"] = false;
            item["navUrl"] = !navUrl.isEmpty() ? navUrl : resolveUrl(m_currentUrl, entry.child("id").text().as_string());
            item["subtext"] = !summary.isEmpty() ? summary : tr("Danh mục con");
            item["sortKey"] = QString("1_%1").arg(index, 6, 10, QChar('0'));
        }

        items.append(item);
        index++;
    }

    // Add Next Page entry if available
    if (!m_nextPageUrl.isEmpty()) {
        QVariantMap nextItem;
        nextItem["entryType"] = "nextPage";
        nextItem["isBook"] = false;
        nextItem["title"] = tr("Trang tiếp theo ›");
        nextItem["subtext"] = tr("Nhấn để xem thêm các tác phẩm tiếp theo");
        nextItem["navUrl"] = m_nextPageUrl;
        nextItem["sortKey"] = QString("3_next");
        items.append(nextItem);
    }

    m_currentFeedItems = items;
    m_feedModel->insertList(items);

    if (items.isEmpty()) {
        m_statusMessage = tr("Không tìm thấy tác phẩm hoặc mục nào.");
    } else {
        m_statusMessage = "";
    }
    emit statusMessageChanged();
    emit feedLoaded();
}

QString OpdsManager::resolveUrl(const QString &baseUrl, const QString &relativeUrl) const
{
    if (relativeUrl.startsWith("http://") || relativeUrl.startsWith("https://")) {
        return relativeUrl;
    }
    QUrl base(baseUrl);
    return base.resolved(QUrl(relativeUrl)).toString();
}

QString OpdsManager::stripHtml(const QString &html) const
{
    QString result = html;
    result.replace(QRegExp("<br\\s*/?>", Qt::CaseInsensitive), " ");
    result.replace(QRegExp("<p>", Qt::CaseInsensitive), " ");
    result.replace(QRegExp("</p>", Qt::CaseInsensitive), "\n");
    result.remove(QRegExp("<[^>]*>"));
    result.replace("&quot;", "\"");
    result.replace("&amp;", "&");
    result.replace("&lt;", "<");
    result.replace("&gt;", ">");
    result.replace("&apos;", "'");
    result.replace("&#39;", "'");
    result.replace("&nbsp;", " ");
    return result.trimmed();
}

void OpdsManager::search(const QString &query)
{
    if (query.trimmed().isEmpty()) return;

    if (m_hasSearch && !m_searchUrlTemplate.isEmpty()) {
        QString sUrl = m_searchUrlTemplate;
        if (sUrl.contains("{searchTerms}")) {
            sUrl.replace("{searchTerms}", QUrl::toPercentEncoding(query.trimmed()));
        } else if (sUrl.contains("?")) {
            sUrl += "&query=" + QUrl::toPercentEncoding(query.trimmed());
        } else {
            sUrl += "?query=" + QUrl::toPercentEncoding(query.trimmed());
        }

        openFeed(sUrl, tr("Tìm: \"%1\"").arg(query.trimmed()), true);
    } else {
        // Filter local feed
        QString q = query.trimmed().toLower();
        QVariantList filtered;
        for (int i = 0; i < m_currentFeedItems.size(); ++i) {
            QVariantMap item = m_currentFeedItems.at(i).toMap();
            QString t = item.value("title").toString().toLower();
            QString a = item.value("author").toString().toLower();
            if (t.contains(q) || a.contains(q)) {
                filtered.append(item);
            }
        }
        m_feedModel->clear();
        m_feedModel->insertList(filtered);
    }
}

void OpdsManager::goBack()
{
    if (m_historyStack.isEmpty()) {
        goHome();
        return;
    }

    QVariantMap prev = m_historyStack.takeLast();
    emit canGoBackChanged();

    m_currentUrl = prev.value("url").toString();
    m_currentTitle = prev.value("title").toString();
    m_currentUsername = prev.value("username").toString();
    m_currentPassword = prev.value("password").toString();

    emit currentTitleChanged();
    emit currentUrlChanged();

    sendFeedRequest(m_currentUrl);
}

void OpdsManager::goHome()
{
    if (m_feedReply) {
        m_feedReply->abort();
        m_feedReply->deleteLater();
        m_feedReply = NULL;
    }

    m_historyStack.clear();
    emit canGoBackChanged();

    m_feedModel->clear();
    m_currentUrl = "";
    m_currentTitle = tr("Kho sách OPDS");
    emit currentTitleChanged();
    emit currentUrlChanged();

    m_isAtRoot = true;
    emit isAtRootChanged();
}

void OpdsManager::refreshFeed()
{
    if (!m_currentUrl.isEmpty()) {
        sendFeedRequest(m_currentUrl);
    }
}

void OpdsManager::downloadBook(const QString &title, const QString &author, const QString &downloadUrl, const QString &format, const QString &coverUrl)
{
    if (m_isDownloading) {
        emit downloadFailed(tr("Đang có tác phẩm khác đang tải, vui lòng đợi hoàn tất!"));
        return;
    }

    if (downloadUrl.trimmed().isEmpty()) {
        emit downloadFailed(tr("Liên kết tải sách không hợp lệ!"));
        return;
    }

    m_isDownloading = true;
    m_downloadProgress = 0;
    m_downloadBookTitle = title.trimmed().isEmpty() ? tr("Sách chưa đặt tên") : title.trimmed();
    m_downloadCoverUrl = coverUrl.trimmed();
    m_downloadStatus = tr("Đang kết nối tới máy chủ...");

    emit isDownloadingChanged();
    emit downloadProgressChanged();
    emit downloadStatusChanged();

    // Directory: /accounts/1000/shared/documents/BerryLeaf
    QString destDir = "/accounts/1000/shared/documents/BerryLeaf";
    QDir dir(destDir);
    if (!dir.exists()) {
        dir.mkpath(destDir);
    }

    // Safe File Name
    QString safeTitle = m_downloadBookTitle;
    safeTitle.replace(QRegExp("[/\\\\:*?\"<>|]"), "_");
    QString safeAuthor = author.trimmed();
    safeAuthor.replace(QRegExp("[/\\\\:*?\"<>|]"), "_");

    QString ext = format.trimmed().toLower();
    if (ext.isEmpty()) ext = "epub";

    QString fileName;
    if (!safeAuthor.isEmpty()) {
        fileName = QString("%1 - %2.%3").arg(safeTitle, safeAuthor, ext);
    } else {
        fileName = QString("%1.%3").arg(safeTitle, ext);
    }

    m_downloadDestPath = QString("%1/%2").arg(destDir, fileName);

    if (m_downloadFile) {
        if (m_downloadFile->isOpen()) m_downloadFile->close();
        delete m_downloadFile;
        m_downloadFile = NULL;
    }

    m_downloadFile = new QFile(m_downloadDestPath);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        m_isDownloading = false;
        emit isDownloadingChanged();
        emit downloadFailed(tr("Không thể tạo file lưu sách trên thiết bị!"));
        delete m_downloadFile;
        m_downloadFile = NULL;
        return;
    }

    QUrl url = QUrl::fromEncoded(downloadUrl.toUtf8());
    QNetworkRequest req(url);
    prepareRequest(req);

    if (!m_currentUsername.isEmpty()) {
        QString auth = QString("%1:%2").arg(m_currentUsername, m_currentPassword);
        QByteArray authData = "Basic " + auth.toUtf8().toBase64();
        req.setRawHeader("Authorization", authData);
    }

    m_downloadReply = m_nam->get(req);
    setupReply(m_downloadReply);
    connect(m_downloadReply, SIGNAL(readyRead()), this, SLOT(onDownloadDataReceived()));
    connect(m_downloadReply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(onDownloadProgress(qint64, qint64)));
    connect(m_downloadReply, SIGNAL(finished()), this, SLOT(onDownloadFinished()));
}

void OpdsManager::onDownloadDataReceived()
{
    if (m_downloadReply && m_downloadFile && m_downloadFile->isOpen()) {
        m_downloadFile->write(m_downloadReply->readAll());
    }
}

void OpdsManager::onDownloadProgress(qint64 received, qint64 total)
{
    if (total > 0) {
        int pct = (int)((received * 100) / total);
        if (pct != m_downloadProgress) {
            m_downloadProgress = pct;
            emit downloadProgressChanged();
        }

        double recMb = received / (1024.0 * 1024.0);
        double totMb = total / (1024.0 * 1024.0);
        m_downloadStatus = QString("%1: %2% (%3 MB / %4 MB)").arg(tr("Đang tải"), QString::number(pct), QString::number(recMb, 'f', 1), QString::number(totMb, 'f', 1));
        emit downloadStatusChanged();
    } else {
        double recMb = received / (1024.0 * 1024.0);
        m_downloadStatus = QString("%1 (%2 MB)...").arg(tr("Đang tải"), QString::number(recMb, 'f', 1));
        emit downloadStatusChanged();
    }
}

void OpdsManager::onDownloadFinished()
{
    if (!m_downloadReply) return;

    // Check for redirects
    QVariant redirectVar = m_downloadReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectVar.isNull()) {
        QUrl redirectUrl = m_downloadReply->url().resolved(redirectVar.toUrl());
        m_downloadReply->deleteLater();
        m_downloadReply = NULL;
        qDebug() << "[OpdsManager] Download redirecting to:" << redirectUrl.toString();

        if (m_downloadFile) {
            m_downloadFile->close();
            m_downloadFile->remove();
            m_downloadFile->open(QIODevice::WriteOnly);
        }

        QUrl url = redirectUrl;
        QNetworkRequest req(url);
        prepareRequest(req);

        if (!m_currentUsername.isEmpty()) {
            QString auth = QString("%1:%2").arg(m_currentUsername, m_currentPassword);
            QByteArray authData = "Basic " + auth.toUtf8().toBase64();
            req.setRawHeader("Authorization", authData);
        }

        m_downloadReply = m_nam->get(req);
        setupReply(m_downloadReply);
        connect(m_downloadReply, SIGNAL(readyRead()), this, SLOT(onDownloadDataReceived()));
        connect(m_downloadReply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(onDownloadProgress(qint64, qint64)));
        connect(m_downloadReply, SIGNAL(finished()), this, SLOT(onDownloadFinished()));
        return;
    }

    if (m_downloadReply->error() != QNetworkReply::NoError) {
        QString err = m_downloadReply->errorString();

        // If download failed with SSL handshake error on HTTPS, attempt HTTP fallback
        if ((m_downloadReply->error() == QNetworkReply::SslHandshakeFailedError || err.contains("SSL", Qt::CaseInsensitive) || err.contains("handshake", Qt::CaseInsensitive))
            && m_downloadDestPath.isEmpty() == false
            && m_downloadReply->url().toString().startsWith("https://", Qt::CaseInsensitive)
            && !m_downloadReply->property("isHttpFallback").toBool()) {

            QUrl origUrl = m_downloadReply->url();
            QString httpUrl = "http://" + origUrl.toString().mid(8);
            qWarning() << "[OpdsManager] Download SSL handshake error - attempting fallback to:" << httpUrl;

            m_downloadReply->deleteLater();
            m_downloadReply = NULL;

            if (m_downloadFile) {
                m_downloadFile->close();
                m_downloadFile->remove();
                m_downloadFile->open(QIODevice::WriteOnly);
            }

            QUrl url = QUrl::fromEncoded(httpUrl.toUtf8());
            QNetworkRequest req(url);
            prepareRequest(req);

            if (!m_currentUsername.isEmpty()) {
                QString auth = QString("%1:%2").arg(m_currentUsername, m_currentPassword);
                QByteArray authData = "Basic " + auth.toUtf8().toBase64();
                req.setRawHeader("Authorization", authData);
            }

            m_downloadReply = m_nam->get(req);
            m_downloadReply->setProperty("isHttpFallback", true);
            setupReply(m_downloadReply);
            connect(m_downloadReply, SIGNAL(readyRead()), this, SLOT(onDownloadDataReceived()));
            connect(m_downloadReply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(onDownloadProgress(qint64, qint64)));
            connect(m_downloadReply, SIGNAL(finished()), this, SLOT(onDownloadFinished()));
            return;
        }

        m_isDownloading = false;
        emit isDownloadingChanged();

        if (m_downloadFile) {
            m_downloadFile->close();
            m_downloadFile->remove();
            delete m_downloadFile;
            m_downloadFile = NULL;
        }

        m_downloadReply->deleteLater();
        m_downloadReply = NULL;

        emit downloadFailed(tr("Tải sách thất bại: %1").arg(err));
        return;
    }

    if (m_downloadFile) {
        m_downloadFile->write(m_downloadReply->readAll());
        m_downloadFile->flush();
        m_downloadFile->close();
        delete m_downloadFile;
        m_downloadFile = NULL;
    }

    m_downloadReply->deleteLater();
    m_downloadReply = NULL;

    m_isDownloading = false;
    emit isDownloadingChanged();

    // Cache cover image if provided
    if (!m_downloadCoverUrl.isEmpty() && m_bookManager) {
        QString hash = QString(QCryptographicHash::hash(m_downloadDestPath.toUtf8(), QCryptographicHash::Md5).toHex());
        QString cacheCoverPath = QString("%1/%2.jpg").arg(m_bookManager->cacheDir(), hash);

        QUrl covUrl = QUrl::fromEncoded(m_downloadCoverUrl.toUtf8());
        QNetworkRequest req(covUrl);
        prepareRequest(req);

        QNetworkReply *covReply = m_nam->get(req);
        covReply->setProperty("cachePath", cacheCoverPath);
        setupReply(covReply);
        connect(covReply, SIGNAL(finished()), this, SLOT(onCoverReplyFinished()));
    }

    // Refresh library scan so the new book appears immediately
    if (m_bookManager) {
        m_bookManager->scanLibrary();
    }

    emit bookDownloaded(m_downloadDestPath, m_downloadBookTitle);
}

void OpdsManager::onCoverReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QString cachePath = reply->property("cachePath").toString();
        if (!cachePath.isEmpty()) {
            QFile f(cachePath);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(reply->readAll());
                f.close();
            }
        }
    }
    reply->deleteLater();
}

void OpdsManager::onSslErrors(const QList<QSslError> &errors)
{
    for (int i = 0; i < errors.size(); ++i) {
        qDebug() << "[OpdsManager] Bypassing SSL error:" << errors.at(i).errorString();
    }
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        reply->ignoreSslErrors();
    }
}

void OpdsManager::onNamSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    for (int i = 0; i < errors.size(); ++i) {
        qDebug() << "[OpdsManager] NAM bypassing SSL error:" << errors.at(i).errorString();
    }
    if (reply) {
        reply->ignoreSslErrors();
    }
}

void OpdsManager::cancelDownload()
{
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
        m_downloadReply = NULL;
    }

    if (m_downloadFile) {
        m_downloadFile->close();
        m_downloadFile->remove();
        delete m_downloadFile;
        m_downloadFile = NULL;
    }

    m_isDownloading = false;
    emit isDownloadingChanged();
    m_downloadStatus = tr("Đã hủy tải sách");
    emit downloadStatusChanged();
}
