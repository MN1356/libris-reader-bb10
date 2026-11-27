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
#include <QScriptEngine>
#include <QScriptValue>
#include <QScriptValueIterator>
#include <QDateTime>
#include <QStringList>
#include <cstring>

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
    , m_openSearchReply(NULL)
    , m_downloadReply(NULL)
    , m_downloadFile(NULL)
    , m_feedRedirectCount(0)
    , m_downloadRedirectCount(0)
    , m_feedFailedFallbackAttempted(false)
    , m_downloadFailedFallbackAttempted(false)
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
        {
            QNetworkReply *_dying = m_feedReply;
            m_feedReply = NULL;
            _dying->disconnect(this);
            _dying->abort();
            _dying->deleteLater();
        }
    }
    if (m_openSearchReply) {
        {
            QNetworkReply *_dying = m_openSearchReply;
            m_openSearchReply = NULL;
            _dying->disconnect(this);
            _dying->abort();
            _dying->deleteLater();
        }
    }
    if (m_downloadReply) {
        {
            QNetworkReply *_dying = m_downloadReply;
            m_downloadReply = NULL;
            _dying->disconnect(this);
            _dying->abort();
            _dying->deleteLater();
        }
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

void OpdsManager::saveCatalogs()
{
    QSettings settings("Libris", "Libris");
    settings.remove("opdsCatalogs");
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
}

static QString normalizeCatalogKey(const QString &raw)
{
    if (raw == "Beautifully formatted public domain classic books")
        return QString::fromUtf8("Sách kinh điển công cộng định dạng EPUB chuẩn đẹp");
    if (raw == "Over 70,000 free books worldwide")
        return QString::fromUtf8("Hơn 70.000 tác phẩm sách miễn phí toàn cầu");
    if (raw == "Popular free books across many genres")
        return QString::fromUtf8("Kho sách miễn phí phổ biến nhiều thể loại");
    if (raw == "Calibre Server (Sample)")
        return QString::fromUtf8("Calibre Máy Chủ (Mẫu)");
    if (raw == "Local Wi-Fi Calibre sample (press & hold to edit IP)")
        return QString::fromUtf8("Ví dụ kết nối Calibre trong mạng Wi-Fi (nhấn giữ để sửa IP)");
    if (raw == "New Catalogs")
        return QString::fromUtf8("Kho sách mới");
    if (raw == "Custom user catalogs")
        return QString::fromUtf8("Kho sách người dùng thêm");
    return raw;
}

static QVariantList readCatalogArray(QSettings &settings)
{
    QVariantList list;
    int count = settings.beginReadArray("opdsCatalogs");
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        QVariantMap cat;
        cat["id"] = settings.value("id").toString();
        cat["name"] = normalizeCatalogKey(settings.value("name").toString());
        cat["url"] = settings.value("url").toString();
        // Automatically upgrade outdated HTTP/legacy OPDS endpoints and migrate defunct Feedbooks
        QString catUrlStr = cat["url"].toString();
        if (catUrlStr.contains("gutenberg.org", Qt::CaseInsensitive) &&
            (catUrlStr.contains("m.gutenberg", Qt::CaseInsensitive) ||
             catUrlStr.contains("ebooks.opds", Qt::CaseInsensitive) ||
             catUrlStr.startsWith("http://", Qt::CaseInsensitive))) {
            cat["url"] = "https://www.gutenberg.org/ebooks/search.opds/";
        } else if (cat["id"].toString() == "feedbooks" || catUrlStr.contains("feedbooks.com")) {
            cat["id"] = "manybooks";
            cat["name"] = "ManyBooks";
            cat["url"] = "https://manybooks.net/opds/index.php";
        }
        cat["username"] = settings.value("username").toString();
        cat["password"] = settings.value("password").toString();
        cat["description"] = normalizeCatalogKey(settings.value("description").toString());
        cat["isDefault"] = settings.value("isDefault", false).toBool();
        list.append(cat);
    }
    settings.endArray();
    return list;
}

void OpdsManager::loadCatalogs()
{
    m_catalogs.clear();
    QSettings settings("Libris", "Libris");
    m_catalogs = readCatalogArray(settings);

    if (m_catalogs.isEmpty()) {
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
    cat1["description"] = QString::fromUtf8("Sách kinh điển công cộng định dạng EPUB chuẩn đẹp");
    cat1["isDefault"] = true;
    m_catalogs.append(cat1);

    QVariantMap cat2;
    cat2["id"] = "gutenberg";
    cat2["name"] = "Project Gutenberg";
    cat2["url"] = "https://www.gutenberg.org/ebooks/search.opds/";
    cat2["description"] = QString::fromUtf8("Hơn 70.000 tác phẩm sách miễn phí toàn cầu");
    cat2["isDefault"] = true;
    m_catalogs.append(cat2);

    QVariantMap cat3;
    cat3["id"] = "manybooks";
    cat3["name"] = "ManyBooks";
    cat3["url"] = "https://manybooks.net/opds/index.php";
    cat3["description"] = QString::fromUtf8("Kho sách miễn phí phổ biến nhiều thể loại");
    cat3["isDefault"] = true;
    m_catalogs.append(cat3);

    QVariantMap cat4;
    cat4["id"] = "calibre_sample";
    cat4["name"] = QString::fromUtf8("Calibre Máy Chủ (Mẫu)");
    cat4["url"] = "http://192.168.1.100:8080/opds";
    cat4["description"] = QString::fromUtf8("Ví dụ kết nối Calibre trong mạng Wi-Fi (nhấn giữ để sửa IP)");
    cat4["isDefault"] = false;
    m_catalogs.append(cat4);

    saveCatalogs();
    rebuildCatalogModel();
    emit catalogsChanged();
}

void OpdsManager::addCatalog(const QString &name, const QString &url, const QString &username, const QString &password, const QString &description)
{
    if (url.trimmed().isEmpty()) return;

    QString cleanName = name.trimmed();
    if (cleanName.isEmpty()) {
        cleanName = QUrl(url).host();
        if (cleanName.isEmpty()) cleanName = QString::fromUtf8("Kho sách mới");
    }

    QString id = QString("custom_%1").arg(QDateTime::currentMSecsSinceEpoch());

    QVariantMap cat;
    cat["id"] = id;
    cat["name"] = cleanName;
    cat["url"] = url.trimmed();
    cat["username"] = username.trimmed();
    cat["password"] = password.trimmed();
    cat["description"] = description.trimmed().isEmpty() ? QString::fromUtf8("Kho sách người dùng thêm") : description.trimmed();
    cat["isDefault"] = false;

    m_catalogs.append(cat);

    saveCatalogs();
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

    saveCatalogs();
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
        saveCatalogs();
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

    m_feedRedirectCount = 0;
    m_feedFailedFallbackAttempted = false;

    sendFeedRequest(m_currentUrl);
}

void OpdsManager::prepareRequest(QNetworkRequest &req)
{
    req.setRawHeader("User-Agent", "Mozilla/5.0 (BlackBerry; U; BlackBerry 10; en-US) AppleWebKit/537.36 (KHTML, like Gecko) Libris/2.0");
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::AnyProtocol);
    sslConfig.setPeerVerifyMode(QSslSocket::defaultCaCertificates().isEmpty() ? QSslSocket::VerifyNone : QSslSocket::VerifyPeer);
    sslConfig.setSslOption(QSsl::SslOptionDisableServerNameIndication, false);
    req.setSslConfiguration(sslConfig);
}

void OpdsManager::setupReply(QNetworkReply *reply)
{
    if (!reply) return;
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::AnyProtocol);
    sslConfig.setPeerVerifyMode(QSslSocket::defaultCaCertificates().isEmpty() ? QSslSocket::VerifyNone : QSslSocket::VerifyPeer);
    sslConfig.setSslOption(QSsl::SslOptionDisableServerNameIndication, false);
    reply->setSslConfiguration(sslConfig);
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));

    QTimer *timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, SIGNAL(timeout()), reply, SLOT(abort()));
    timeoutTimer->start(30000);
}

void OpdsManager::sendFeedRequest(const QString &urlStr)
{
    if (m_feedReply) {
        {
            QNetworkReply *_dying = m_feedReply;
            m_feedReply = NULL;
            _dying->disconnect(this);
            _dying->abort();
            _dying->deleteLater();
        }
    }

    m_isLoading = true;
    emit isLoadingChanged();

    m_statusMessage = tr("Đang tải dữ liệu...");
    emit statusMessageChanged();

    QUrl url = QUrl::fromEncoded(urlStr.toUtf8());
    QNetworkRequest req(url);
    prepareRequest(req);
    req.setRawHeader("Accept", "application/opds+json, application/opds-publication+json, application/atom+xml;profile=opds-catalog, application/atom+xml, application/xml, text/xml, application/json, */*");

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

        m_feedRedirectCount++;
        if (m_feedRedirectCount > 5) {
            m_statusMessage = tr("Lỗi chuyển hướng quá nhiều lần (Redirect loop)!");
            emit statusMessageChanged();
            return;
        }

        // Prevent infinite loop if fallback HTTP got redirected back to failing HTTPS
        if (m_feedFailedFallbackAttempted && redirectUrl.scheme().compare("https", Qt::CaseInsensitive) == 0) {
            qWarning() << "[OpdsManager] HTTP fallback was redirected back to HTTPS, aborting loop for:" << redirectUrl.toString();
            m_statusMessage = tr("Máy chủ bắt buộc dùng HTTPS với chuẩn TLS/mã hóa mới mà thiết bị BB10 chưa hỗ trợ.");
            emit statusMessageChanged();
            return;
        }

        qDebug() << "[OpdsManager] Following redirect (" << m_feedRedirectCount << ") to:" << redirectUrl.toString();
        m_currentUrl = redirectUrl.toString();
        emit currentUrlChanged();
        sendFeedRequest(m_currentUrl);
        return;
    }

    if (m_feedReply->error() != QNetworkReply::NoError) {
        QString err = m_feedReply->errorString();
        QNetworkReply::NetworkError netErr = m_feedReply->error();
        int statusCode = m_feedReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Smart fallback: only downgrade HTTPS->HTTP for anonymous public feeds (never leak Basic Auth credentials)
        if ((netErr == QNetworkReply::SslHandshakeFailedError || err.contains("SSL", Qt::CaseInsensitive) || err.contains("handshake", Qt::CaseInsensitive))
            && m_currentUsername.isEmpty()
            && m_currentUrl.startsWith("https://", Qt::CaseInsensitive)
            && !m_feedFailedFallbackAttempted) {

            m_feedFailedFallbackAttempted = true;
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

            sendFeedRequest(m_currentUrl);
            return;
        }

        m_statusMessage = formatNetworkError(statusCode, netErr, err, false);
        emit statusMessageChanged();
        m_feedReply->deleteLater();
        m_feedReply = NULL;
        return;
    }

    QString contentType = m_feedReply->header(QNetworkRequest::ContentTypeHeader).toString().toLower();
    QByteArray data = m_feedReply->readAll();
    m_feedReply->deleteLater();
    m_feedReply = NULL;

    parseFeedResponse(data, contentType);
}

QString OpdsManager::formatNetworkError(int statusCode, QNetworkReply::NetworkError netErr, const QString &errStr, bool isDownload) const
{
    if (statusCode == 503) {
        return isDownload
            ? tr("Máy chủ tải sách đang bảo trì hoặc quá tải (HTTP 503 Service Unavailable). Vui lòng thử lại sau.")
            : tr("Máy chủ đang bảo trì hoặc tạm ngừng phục vụ (HTTP 503 Service Unavailable). Vui lòng thử lại sau.");
    }
    if (statusCode == 502 || statusCode == 504) {
        return tr("Máy chủ phản hồi chậm hoặc cổng kết nối gặp sự cố (HTTP %1).").arg(statusCode);
    }
    if (statusCode == 401) {
        return tr("Máy chủ yêu cầu đăng nhập (HTTP 401). Hãy kiểm tra tên người dùng và mật khẩu.");
    }
    if (statusCode == 403) {
        return isDownload
            ? tr("Máy chủ chặn tải sách (HTTP 403 / Bảo vệ chống Bot).")
            : tr("Máy chủ từ chối kết nối (HTTP 403 / Bảo vệ chống Bot).");
    }
    if (statusCode == 404) {
        return tr("Không tìm thấy đường dẫn danh mục OPDS (HTTP 404).");
    }
    if (statusCode == 429) {
        return tr("Quá nhiều yêu cầu tới máy chủ (HTTP 429). Vui lòng đợi một lát rồi thử lại.");
    }
    if (statusCode >= 500) {
        return tr("Máy chủ gặp lỗi nội bộ (HTTP %1).").arg(statusCode);
    }
    if (netErr == QNetworkReply::SslHandshakeFailedError || errStr.contains("SSL", Qt::CaseInsensitive) || errStr.contains("handshake", Qt::CaseInsensitive)) {
        return isDownload
            ? tr("Lỗi bắt tay SSL khi tải sách. Máy chủ yêu cầu chuẩn TLS mới hơn.")
            : tr("Lỗi bắt tay SSL/TLS: Máy chủ yêu cầu phiên bản TLS hoặc thuật toán mã hóa (cipher) mới.");
    }
    return isDownload
        ? tr("Tải sách thất bại: %1").arg(errStr)
        : tr("Lỗi kết nối: %1").arg(errStr);
}

static QString xmlLocalName(const pugi::xml_node &node)
{
    if (!node) return QString();
    const char *raw = node.name();
    if (!raw) return QString();
    const char *colon = strchr(raw, ':');
    return QString::fromUtf8(colon ? (colon + 1) : raw).trimmed().toLower();
}

static pugi::xml_node findChildLocal(const pugi::xml_node &parent, const char *localName)
{
    QString target = QString::fromLatin1(localName).toLower();
    for (pugi::xml_node ch = parent.first_child(); ch; ch = ch.next_sibling()) {
        if (ch.type() == pugi::node_element && xmlLocalName(ch) == target) {
            return ch;
        }
    }
    return pugi::xml_node();
}

static QString xmlNodeInnerText(const pugi::xml_node &node)
{
    if (!node) return QString();
    QString raw = QString::fromUtf8(node.text().as_string()).trimmed();
    if (raw.isEmpty() && node.first_child()) {
        struct xml_buf_writer : public pugi::xml_writer {
            QByteArray buf;
            virtual void write(const void *data, size_t size) {
                buf.append(static_cast<const char*>(data), static_cast<int>(size));
            }
        } writer;
        for (pugi::xml_node ch = node.first_child(); ch; ch = ch.next_sibling()) {
            ch.print(writer, "", pugi::format_raw);
        }
        raw = QString::fromUtf8(writer.buf.constData(), writer.buf.size()).trimmed();
    }
    return raw;
}

static QString inferFormatFromTypeOrUrl(const QString &mimeLower, const QString &fullHref)
{
    QString pathLower = QUrl::fromEncoded(fullHref.toUtf8()).path().toLower();
    if (pathLower.isEmpty()) pathLower = fullHref.toLower();

    if (mimeLower.contains("epub") || pathLower.endsWith(".epub") || pathLower.contains(".epub.")) return "epub";
    if (mimeLower.contains("pdf") || pathLower.endsWith(".pdf")) return "pdf";
    if (mimeLower.contains("text/plain") || pathLower.endsWith(".txt")) return "txt";
    if (mimeLower.contains("fb2") || mimeLower.contains("fictionbook") || pathLower.endsWith(".fb2")) return "fb2";
    if (mimeLower.contains("cbz") || mimeLower.contains("comicbook") || pathLower.endsWith(".cbz")) return "cbz";
    if (mimeLower.contains("azw3") || mimeLower.contains("kf8") || pathLower.endsWith(".azw3") || pathLower.endsWith(".kf8")) return "azw3";
    if (mimeLower.contains("mobi") || mimeLower.contains("x-mobipocket") || pathLower.endsWith(".mobi") || pathLower.endsWith(".prc") || pathLower.endsWith(".azw")) return "mobi";
    if (mimeLower.contains("djvu") || pathLower.endsWith(".djvu") || pathLower.endsWith(".djv")) return "djvu";
    return QString();
}

static bool hasSpaceSeparatedToken(const QString &relLower, const QString &token)
{
    if (relLower == token) return true;
    QStringList parts = relLower.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    return parts.contains(token);
}

static bool isAcquisitionRel(const QString &relLower)
{
    return relLower.contains("opds-spec.org/acquisition") ||
           relLower.contains("x-stanza-acquisition") ||
           hasSpaceSeparatedToken(relLower, "acquisition") ||
           hasSpaceSeparatedToken(relLower, "enclosure");
}

static bool isNavigationLink(const QString &relLower, const QString &typeLower)
{
    if (typeLower.contains("opds+json") || typeLower.contains("opds-catalog") || typeLower.contains("atom+xml")) {
        return true;
    }
    return relLower.contains("opds-spec.org/subsection") ||
           relLower.contains("opds-spec.org/featured") ||
           relLower.contains("opds-spec.org/recommended") ||
           relLower.contains("opds-spec.org/sort/") ||
           relLower.contains("opds-spec.org/crawlable") ||
           relLower.contains("opds-spec.org/facet") ||
           hasSpaceSeparatedToken(relLower, "subsection") ||
           hasSpaceSeparatedToken(relLower, "collection") ||
           hasSpaceSeparatedToken(relLower, "alternate");
}

void OpdsManager::parseFeedResponse(const QByteArray &data, const QString &contentType)
{
    m_feedModel->clear();
    m_currentFeedItems.clear();
    m_hasSearch = false;
    m_searchUrlTemplate.clear();
    m_pendingSearchQuery.clear();
    m_nextPageUrl.clear();
    if (m_openSearchReply) {
        QNetworkReply *_dying = m_openSearchReply;
        m_openSearchReply = NULL;
        _dying->disconnect(this);
        _dying->abort();
        _dying->deleteLater();
    }

    QByteArray trimmedData = data.trimmed();
    if (trimmedData.isEmpty()) {
        emit hasSearchChanged();
        m_statusMessage = tr("Máy chủ trả về dữ liệu trống.");
        emit statusMessageChanged();
        return;
    }

    bool looksLikeJson = contentType.contains("json") || contentType.contains("opds+json") ||
                         trimmedData.startsWith('{') || trimmedData.startsWith('[');
    if (looksLikeJson) {
        parseOpds2Json(trimmedData);
        return;
    }

    // Detect HTML error / maintenance / bot-challenge page served with HTTP 200
    QByteArray headLower = trimmedData.left(512).toLower();
    if (contentType.contains("text/html") || headLower.startsWith("<!doctype html") || headLower.startsWith("<html")) {
        QString htmlText = QString::fromUtf8(trimmedData.constData(), qMin(trimmedData.size(), 8192));
        QRegExp titleRe("<title[^>]*>([^<]+)</title>", Qt::CaseInsensitive);
        QString pageTitle;
        if (titleRe.indexIn(htmlText) != -1) {
            pageTitle = stripHtml(titleRe.cap(1));
        }
        emit hasSearchChanged();
        m_statusMessage = pageTitle.isEmpty()
            ? tr("Máy chủ trả về trang web HTML thay vì danh mục OPDS.")
            : tr("Máy chủ trả về trang web HTML thay vì danh mục OPDS (%1).").arg(pageTitle);
        emit statusMessageChanged();
        return;
    }

    parseAtomFeed(trimmedData);
}

static QString extractOpds2Text(const QScriptValue &val)
{
    if (!val.isValid() || val.isNull() || val.isUndefined()) return QString();
    if (val.isString() || val.isNumber()) return val.toString().trimmed();
    if (val.isArray()) {
        int len = val.property("length").toInt32();
        for (int i = 0; i < len; ++i) {
            QString t = extractOpds2Text(val.property(i));
            if (!t.isEmpty()) return t;
        }
        return QString();
    }
    if (val.isObject()) {
        QScriptValue enVal = val.property("en");
        if (enVal.isString() && !enVal.toString().trimmed().isEmpty()) {
            return enVal.toString().trimmed();
        }
        QScriptValue nameVal = val.property("name");
        if (nameVal.isValid() && !nameVal.isUndefined()) {
            QString n = extractOpds2Text(nameVal);
            if (!n.isEmpty()) return n;
        }
        QScriptValueIterator it(val);
        while (it.hasNext()) {
            it.next();
            if (it.value().isString() && !it.value().toString().trimmed().isEmpty()) {
                return it.value().toString().trimmed();
            }
        }
    }
    return QString();
}

static QString extractOpds2Contributors(const QScriptValue &meta)
{
    if (!meta.isObject()) return QString();
    const char *keys[] = {"author", "editor", "translator", "contributor"};
    for (int k = 0; k < 4; ++k) {
        QScriptValue val = meta.property(keys[k]);
        if (!val.isValid() || val.isNull() || val.isUndefined()) continue;

        QStringList names;
        if (val.isArray()) {
            int len = val.property("length").toInt32();
            for (int i = 0; i < len; ++i) {
                QScriptValue el = val.property(i);
                QString n = el.isObject() ? extractOpds2Text(el.property("name")) : extractOpds2Text(el);
                if (!n.isEmpty() && !names.contains(n)) names.append(n);
            }
        } else if (val.isObject()) {
            QString n = extractOpds2Text(val.property("name"));
            if (!n.isEmpty()) names.append(n);
        } else {
            QString n = extractOpds2Text(val);
            if (!n.isEmpty()) names.append(n);
        }
        if (!names.isEmpty()) {
            return names.join(", ");
        }
    }
    return QString();
}

static bool opds2LinkHasRel(const QScriptValue &linkObj, const QString &targetSubstr, bool exactToken)
{
    QScriptValue relVal = linkObj.property("rel");
    if (!relVal.isValid() || relVal.isNull() || relVal.isUndefined()) return false;
    if (relVal.isArray()) {
        int len = relVal.property("length").toInt32();
        for (int i = 0; i < len; ++i) {
            QString r = relVal.property(i).toString().trimmed().toLower();
            if (exactToken ? hasSpaceSeparatedToken(r, targetSubstr) : r.contains(targetSubstr)) {
                return true;
            }
        }
        return false;
    }
    QString r = relVal.toString().trimmed().toLower();
    return exactToken ? hasSpaceSeparatedToken(r, targetSubstr) : r.contains(targetSubstr);
}

static QString opds2LinkRelJoined(const QScriptValue &linkObj)
{
    QScriptValue relVal = linkObj.property("rel");
    if (!relVal.isValid() || relVal.isNull() || relVal.isUndefined()) return QString();
    if (relVal.isArray()) {
        QStringList parts;
        int len = relVal.property("length").toInt32();
        for (int i = 0; i < len; ++i) {
            parts.append(relVal.property(i).toString().trimmed().toLower());
        }
        return parts.join(" ");
    }
    return relVal.toString().trimmed().toLower();
}

void OpdsManager::parseOpds2Json(const QByteArray &data)
{
    QScriptEngine engine;
    QScriptValue root = engine.evaluate("(" + QString::fromUtf8(data.constData(), data.size()) + ")");
    if (engine.hasUncaughtException() || !root.isObject() || root.isArray()) {
        emit hasSearchChanged();
        m_statusMessage = tr("Không thể đọc cấu trúc danh mục OPDS 2.0 JSON!");
        emit statusMessageChanged();
        return;
    }

    QScriptValue rootMeta = root.property("metadata");
    if (rootMeta.isObject()) {
        QString parsedTitle = extractOpds2Text(rootMeta.property("title"));
        if (!parsedTitle.isEmpty() && m_currentTitle.isEmpty()) {
            m_currentTitle = parsedTitle;
            emit currentTitleChanged();
        }
    }

    QScriptValue rootLinks = root.property("links");
    if (rootLinks.isArray()) {
        int len = rootLinks.property("length").toInt32();
        for (int i = 0; i < len; ++i) {
            QScriptValue l = rootLinks.property(i);
            if (!l.isObject()) continue;
            QString href = l.property("href").toString().trimmed();
            if (href.isEmpty()) continue;

            if (opds2LinkHasRel(l, "search", true)) {
                m_searchUrlTemplate = resolveUrl(m_currentUrl, href);
                m_searchUrlTemplate.replace("%7B", "{", Qt::CaseInsensitive).replace("%7D", "}", Qt::CaseInsensitive);
                m_hasSearch = true;
            } else if (opds2LinkHasRel(l, "next", true) && m_nextPageUrl.isEmpty()) {
                m_nextPageUrl = resolveUrl(m_currentUrl, href);
            }
        }
    }
    emit hasSearchChanged();

    QVariantList items;
    int index = 0;

    // Helper lambda-equivalent via local array processing for navigation entries
    QScriptValue navArr = root.property("navigation");
    if (navArr.isArray()) {
        int nLen = navArr.property("length").toInt32();
        for (int i = 0; i < nLen; ++i) {
            QScriptValue nav = navArr.property(i);
            if (!nav.isObject()) continue;
            QString href = nav.property("href").toString().trimmed();
            QString title = extractOpds2Text(nav.property("title"));
            if (href.isEmpty() || title.isEmpty()) continue;

            QString sub = nav.property("subtitle").isValid() ? extractOpds2Text(nav.property("subtitle")) : "";
            QScriptValue props = nav.property("properties");
            if (sub.isEmpty() && props.isObject() && props.property("numberOfItems").isNumber()) {
                sub = tr("%1 mục").arg(props.property("numberOfItems").toInt32());
            }

            QVariantMap item;
            item["entryType"] = "navigation";
            item["isBook"] = false;
            item["title"] = title;
            item["author"] = "";
            item["summary"] = sub;
            item["coverUrl"] = "";
            item["navUrl"] = resolveUrl(m_currentUrl, href);
            item["subtext"] = !sub.isEmpty() ? sub : tr("Danh mục con");
            item["sortKey"] = QString("1_%1").arg(index, 6, 10, QChar('0'));
            items.append(item);
            index++;
        }
    }

    // Collect publications from both top-level "publications" and "groups" (OPDS 2.0 feed groups)
    QList<QScriptValue> pubObjects;
    QScriptValue pubsArr = root.property("publications");
    if (pubsArr.isArray()) {
        int pLen = pubsArr.property("length").toInt32();
        for (int i = 0; i < pLen; ++i) {
            pubObjects.append(pubsArr.property(i));
        }
    }

    QScriptValue groupsArr = root.property("groups");
    if (groupsArr.isArray()) {
        int gLen = groupsArr.property("length").toInt32();
        for (int g = 0; g < gLen; ++g) {
            QScriptValue grp = groupsArr.property(g);
            if (!grp.isObject()) continue;

            QString grpTitle = extractOpds2Text(grp.property("metadata").property("title"));
            QString grpSelfUrl;
            QScriptValue gLinks = grp.property("links");
            if (gLinks.isArray()) {
                int glLen = gLinks.property("length").toInt32();
                for (int k = 0; k < glLen; ++k) {
                    QScriptValue gl = gLinks.property(k);
                    if (opds2LinkHasRel(gl, "self", true) || opds2LinkHasRel(gl, "alternate", true)) {
                        grpSelfUrl = resolveUrl(m_currentUrl, gl.property("href").toString().trimmed());
                        break;
                    }
                }
            }
            if (!grpTitle.isEmpty() && !grpSelfUrl.isEmpty()) {
                QVariantMap navItem;
                navItem["entryType"] = "navigation";
                navItem["isBook"] = false;
                navItem["title"] = grpTitle;
                navItem["author"] = "";
                navItem["summary"] = "";
                navItem["coverUrl"] = "";
                navItem["navUrl"] = grpSelfUrl;
                navItem["subtext"] = tr("Nhóm danh mục");
                navItem["sortKey"] = QString("1_%1").arg(index, 6, 10, QChar('0'));
                items.append(navItem);
                index++;
            }

            QScriptValue gNav = grp.property("navigation");
            if (gNav.isArray()) {
                int gnLen = gNav.property("length").toInt32();
                for (int i = 0; i < gnLen; ++i) {
                    QScriptValue nav = gNav.property(i);
                    if (!nav.isObject()) continue;
                    QString href = nav.property("href").toString().trimmed();
                    QString title = extractOpds2Text(nav.property("title"));
                    if (href.isEmpty() || title.isEmpty()) continue;
                    QVariantMap item;
                    item["entryType"] = "navigation";
                    item["isBook"] = false;
                    item["title"] = title;
                    item["navUrl"] = resolveUrl(m_currentUrl, href);
                    item["subtext"] = !grpTitle.isEmpty() ? grpTitle : tr("Danh mục con");
                    item["sortKey"] = QString("1_%1").arg(index, 6, 10, QChar('0'));
                    items.append(item);
                    index++;
                }
            }

            QScriptValue gPubs = grp.property("publications");
            if (gPubs.isArray()) {
                int gpLen = gPubs.property("length").toInt32();
                for (int i = 0; i < gpLen; ++i) {
                    pubObjects.append(gPubs.property(i));
                }
            }
        }
    }

    for (int p = 0; p < pubObjects.size(); ++p) {
        QScriptValue pub = pubObjects.at(p);
        if (!pub.isObject()) continue;
        QScriptValue meta = pub.property("metadata");
        if (!meta.isObject()) continue;

        QString title = extractOpds2Text(meta.property("title"));
        if (title.isEmpty()) continue;
        QString author = extractOpds2Contributors(meta);
        QString summary = stripHtml(extractOpds2Text(meta.property("description")));

        QString coverUrl;
        QScriptValue imgs = pub.property("images");
        if (imgs.isArray()) {
            int iLen = imgs.property("length").toInt32();
            int bestHeight = 999999;
            for (int i = 0; i < iLen; ++i) {
                QScriptValue im = imgs.property(i);
                QString ihref = im.property("href").toString().trimmed();
                if (ihref.isEmpty()) continue;
                int h = im.property("height").toInt32();
                if (coverUrl.isEmpty() || (h > 0 && h < bestHeight && h >= 100)) {
                    coverUrl = resolveUrl(m_currentUrl, ihref);
                    if (h > 0) bestHeight = h;
                }
            }
        }

        QString epubUrl;
        QString pdfUrl;
        QString genericDownloadUrl;
        QString genericFormat;
        QString navUrl;

        QScriptValue links = pub.property("links");
        if (links.isArray()) {
            int lLen = links.property("length").toInt32();
            for (int i = 0; i < lLen; ++i) {
                QScriptValue l = links.property(i);
                if (!l.isObject()) continue;
                QString href = l.property("href").toString().trimmed();
                if (href.isEmpty()) continue;
                QString typeLower = l.property("type").toString().trimmed().toLower();
                QString relLower = opds2LinkRelJoined(l);
                QString fullHref = resolveUrl(m_currentUrl, href);

                if (relLower.contains("image") || typeLower.startsWith("image/")) {
                    if (coverUrl.isEmpty() || relLower.contains("thumbnail")) {
                        coverUrl = fullHref;
                    }
                    continue;
                }

                QString inferredFmt = inferFormatFromTypeOrUrl(typeLower, fullHref);
                bool isDirectEbook = !inferredFmt.isEmpty() && !typeLower.contains("html");
                bool isAcq = isAcquisitionRel(relLower) && !typeLower.contains("html");

                if (isDirectEbook || isAcq) {
                    if (inferredFmt == "epub" && epubUrl.isEmpty()) {
                        epubUrl = fullHref;
                    } else if (inferredFmt == "pdf" && pdfUrl.isEmpty()) {
                        pdfUrl = fullHref;
                    } else if (genericDownloadUrl.isEmpty()) {
                        genericDownloadUrl = fullHref;
                        genericFormat = inferredFmt.isEmpty() ? "epub" : inferredFmt;
                    }
                } else if (isNavigationLink(relLower, typeLower) && navUrl.isEmpty()) {
                    navUrl = fullHref;
                }
            }
        }

        QVariantMap item;
        item["title"] = title;
        item["author"] = author;
        item["summary"] = summary;
        item["coverUrl"] = coverUrl;

        if (!epubUrl.isEmpty() || !pdfUrl.isEmpty() || !genericDownloadUrl.isEmpty()) {
            QString mainUrl = !epubUrl.isEmpty() ? epubUrl : (!pdfUrl.isEmpty() ? pdfUrl : genericDownloadUrl);
            QString mainFmt = !epubUrl.isEmpty() ? "epub" : (!pdfUrl.isEmpty() ? "pdf" : (genericFormat.isEmpty() ? "epub" : genericFormat));
            item["entryType"] = "book";
            item["isBook"] = true;
            item["downloadUrl"] = mainUrl;
            item["format"] = mainFmt;
            item["formatBadge"] = mainFmt.toUpper();
            item["epubUrl"] = epubUrl;
            item["pdfUrl"] = pdfUrl;
            item["subtext"] = !author.isEmpty() ? author : mainFmt.toUpper();
            item["sortKey"] = QString("2_%1").arg(index, 6, 10, QChar('0'));
        } else if (!navUrl.isEmpty()) {
            item["entryType"] = "navigation";
            item["isBook"] = false;
            item["navUrl"] = navUrl;
            item["subtext"] = !summary.isEmpty() ? summary : tr("Danh mục con");
            item["sortKey"] = QString("1_%1").arg(index, 6, 10, QChar('0'));
        } else {
            continue;
        }
        items.append(item);
        index++;
    }

    QScriptValue facetsArr = root.property("facets");
    if (facetsArr.isArray()) {
        int fLen = facetsArr.property("length").toInt32();
        for (int f = 0; f < fLen; ++f) {
            QScriptValue facetGrp = facetsArr.property(f);
            if (!facetGrp.isObject()) continue;
            QString facetGrpTitle = extractOpds2Text(facetGrp.property("metadata").property("title"));
            QScriptValue fLinks = facetGrp.property("links");
            if (!fLinks.isArray()) continue;
            int flLen = fLinks.property("length").toInt32();
            for (int k = 0; k < flLen; ++k) {
                QScriptValue fl = fLinks.property(k);
                if (!fl.isObject()) continue;
                QString fHref = fl.property("href").toString().trimmed();
                QString fTitle = extractOpds2Text(fl.property("title"));
                if (fHref.isEmpty() || fTitle.isEmpty()) continue;

                QVariantMap facetItem;
                facetItem["entryType"] = "navigation";
                facetItem["isBook"] = false;
                facetItem["title"] = fTitle;
                facetItem["navUrl"] = resolveUrl(m_currentUrl, fHref);
                facetItem["subtext"] = !facetGrpTitle.isEmpty() ? facetGrpTitle : tr("Bộ lọc danh mục");
                facetItem["sortKey"] = QString("1_%1").arg(index, 6, 10, QChar('0'));
                items.append(facetItem);
                index++;
            }
        }
    }

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

    m_statusMessage = items.isEmpty() ? tr("Không tìm thấy tác phẩm hoặc mục nào.") : "";
    emit statusMessageChanged();
    emit feedLoaded();
}

void OpdsManager::parseAtomFeed(const QByteArray &data)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_buffer(data.constData(), data.size(), pugi::parse_default | pugi::parse_ws_pcdata);
    if (!result) {
        emit hasSearchChanged();
        m_statusMessage = tr("Không thể đọc cấu trúc danh mục OPDS XML!");
        emit statusMessageChanged();
        return;
    }

    pugi::xml_node feedNode;
    for (pugi::xml_node n = doc.first_child(); n; n = n.next_sibling()) {
        if (n.type() == pugi::node_element) {
            QString ln = xmlLocalName(n);
            if (ln == "feed" || ln.contains("feed")) {
                feedNode = n;
                break;
            }
        }
    }

    if (!feedNode) {
        emit hasSearchChanged();
        m_statusMessage = tr("Phản hồi không phải là nguồn OPDS Atom hợp lệ.");
        emit statusMessageChanged();
        return;
    }

    // Feed Title (namespace-independent)
    pugi::xml_node feedTitleNode = findChildLocal(feedNode, "title");
    if (feedTitleNode) {
        QString parsedTitle = QString::fromUtf8(feedTitleNode.text().as_string()).trimmed();
        if (!parsedTitle.isEmpty() && m_currentTitle.isEmpty()) {
            m_currentTitle = parsedTitle;
            emit currentTitleChanged();
        }
    }

    // Feed-level links (Search, Next page, Facets if no entries)
    QString openSearchDescUrl;
    QVariantList items;
    int index = 0;

    for (pugi::xml_node link = feedNode.first_child(); link; link = link.next_sibling()) {
        if (link.type() != pugi::node_element || xmlLocalName(link) != "link") continue;

        QString rel = QString(link.attribute("rel").as_string()).trimmed().toLower();
        QString href = QString::fromUtf8(link.attribute("href").as_string()).trimmed();
        QString type = QString(link.attribute("type").as_string()).trimmed().toLower();
        if (href.isEmpty()) continue;

        if (hasSpaceSeparatedToken(rel, "search") || type.contains("opensearch")) {
            QString resolvedHref = resolveUrl(m_currentUrl, href);
            resolvedHref.replace("%7B", "{", Qt::CaseInsensitive).replace("%7D", "}", Qt::CaseInsensitive);
            if (resolvedHref.contains("{searchTerms}") || resolvedHref.contains("{query}")) {
                m_searchUrlTemplate = resolvedHref;
                m_hasSearch = true;
            } else if (type.contains("opensearchdescription") || type.contains("xml") || resolvedHref.endsWith(".xml", Qt::CaseInsensitive)) {
                openSearchDescUrl = resolvedHref;
                m_hasSearch = true;
            } else {
                m_searchUrlTemplate = resolvedHref;
                m_hasSearch = true;
            }
        } else if (hasSpaceSeparatedToken(rel, "next") && m_nextPageUrl.isEmpty()) {
            m_nextPageUrl = resolveUrl(m_currentUrl, href);
        } else if (rel.contains("opds-spec.org/facet") && !href.isEmpty()) {
            QString facetTitle = QString::fromUtf8(link.attribute("title").as_string()).trimmed();
            QString facetGroup = QString::fromUtf8(link.attribute("opds:facetGroup").as_string()).trimmed();
            if (!facetTitle.isEmpty()) {
                QVariantMap facetItem;
                facetItem["entryType"] = "navigation";
                facetItem["isBook"] = false;
                facetItem["title"] = facetTitle;
                facetItem["navUrl"] = resolveUrl(m_currentUrl, href);
                facetItem["subtext"] = !facetGroup.isEmpty() ? facetGroup : tr("Bộ lọc danh mục");
                facetItem["sortKey"] = QString("1_%1").arg(index, 6, 10, QChar('0'));
                items.append(facetItem);
                index++;
            }
        }
    }
    if (m_searchUrlTemplate.isEmpty() && !openSearchDescUrl.isEmpty()) {
        fetchOpenSearchDescription(openSearchDescUrl);
    }
    emit hasSearchChanged();

    for (pugi::xml_node entry = feedNode.first_child(); entry; entry = entry.next_sibling()) {
        if (entry.type() != pugi::node_element || xmlLocalName(entry) != "entry") continue;

        QString title;
        QStringList authors;
        QString summary;
        QString contentText;
        QString dcDesc;
        QString entryId;

        QString coverUrl;
        QString navUrl;
        QString epubUrl;
        bool epubIsOpen = false;
        QString pdfUrl;
        QString genericDownloadUrl;
        QString genericFormat;

        for (pugi::xml_node ch = entry.first_child(); ch; ch = ch.next_sibling()) {
            if (ch.type() != pugi::node_element) continue;
            QString tag = xmlLocalName(ch);

            if (tag == "title" && title.isEmpty()) {
                title = QString::fromUtf8(ch.text().as_string()).trimmed();
            } else if (tag == "id" && entryId.isEmpty()) {
                entryId = QString::fromUtf8(ch.text().as_string()).trimmed();
            } else if (tag == "author" || tag == "creator") {
                pugi::xml_node nameNode = findChildLocal(ch, "name");
                QString a = nameNode
                    ? QString::fromUtf8(nameNode.text().as_string()).trimmed()
                    : QString::fromUtf8(ch.text().as_string()).trimmed();
                if (!a.isEmpty() && !authors.contains(a)) {
                    authors.append(a);
                }
            } else if (tag == "summary" && summary.isEmpty()) {
                summary = stripHtml(xmlNodeInnerText(ch));
            } else if (tag == "content" && contentText.isEmpty()) {
                contentText = stripHtml(xmlNodeInnerText(ch));
            } else if (tag == "description" && dcDesc.isEmpty()) {
                dcDesc = stripHtml(xmlNodeInnerText(ch));
            } else if (tag == "thumbnail" && coverUrl.isEmpty()) {
                QString mUrl = QString::fromUtf8(ch.attribute("url").as_string()).trimmed();
                if (!mUrl.isEmpty()) coverUrl = resolveUrl(m_currentUrl, mUrl);
            } else if (tag == "link") {
                QString rel = QString(ch.attribute("rel").as_string()).trimmed().toLower();
                QString href = QString::fromUtf8(ch.attribute("href").as_string()).trimmed();
                QString type = QString(ch.attribute("type").as_string()).trimmed().toLower();
                if (href.isEmpty()) continue;

                QString fullHref = resolveUrl(m_currentUrl, href);
                if (rel.contains("image") || rel.contains("thumbnail") || rel.contains("cover") || type.startsWith("image/")) {
                    if (coverUrl.isEmpty() || rel.contains("thumbnail")) {
                        coverUrl = fullHref;
                    }
                    continue;
                }

                QString inferredFmt = inferFormatFromTypeOrUrl(type, fullHref);
                bool directEbook = !inferredFmt.isEmpty() && !type.contains("html");
                bool isAcq = isAcquisitionRel(rel) && !type.contains("html");

                if (directEbook || isAcq) {
                    bool isOpenAccess = rel.contains("open-access") || rel == "http://opds-spec.org/acquisition";
                    if (inferredFmt == "epub") {
                        if (epubUrl.isEmpty() || (isOpenAccess && !epubIsOpen)) {
                            epubUrl = fullHref;
                            epubIsOpen = isOpenAccess;
                        }
                    } else if (inferredFmt == "pdf") {
                        if (pdfUrl.isEmpty()) pdfUrl = fullHref;
                    } else if (genericDownloadUrl.isEmpty()) {
                        genericDownloadUrl = fullHref;
                        genericFormat = inferredFmt.isEmpty() ? "epub" : inferredFmt;
                    }
                } else if (isNavigationLink(rel, type) || (rel.isEmpty() && !href.isEmpty())) {
                    if (navUrl.isEmpty()) {
                        navUrl = fullHref;
                    }
                }
            }
        }

        if (title.isEmpty()) continue;
        QString author = authors.join(", ");
        if (summary.isEmpty()) summary = !contentText.isEmpty() ? contentText : dcDesc;

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
            item["navUrl"] = !navUrl.isEmpty() ? navUrl : resolveUrl(m_currentUrl, entryId);
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

void OpdsManager::fetchOpenSearchDescription(const QString &url)
{
    if (url.isEmpty()) return;
    if (m_openSearchReply) {
        QNetworkReply *_dying = m_openSearchReply;
        m_openSearchReply = NULL;
        _dying->disconnect(this);
        _dying->abort();
        _dying->deleteLater();
    }

    QUrl qUrl = url.contains('%') ? QUrl::fromEncoded(url.toUtf8()) : QUrl::fromUserInput(url);
    QNetworkRequest req(qUrl);
    prepareRequest(req);
    if (!m_currentUsername.isEmpty()) {
        QString auth = QString("%1:%2").arg(m_currentUsername, m_currentPassword);
        req.setRawHeader("Authorization", "Basic " + auth.toUtf8().toBase64());
    }

    m_openSearchReply = m_nam->get(req);
    setupReply(m_openSearchReply);
    connect(m_openSearchReply, SIGNAL(finished()), this, SLOT(onOpenSearchReplyFinished()));
}

void OpdsManager::onOpenSearchReplyFinished()
{
    QNetworkReply *reply = m_openSearchReply;
    if (!reply) return;
    m_openSearchReply = NULL;

    QVariant redirectVar = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectVar.isNull()) {
        int redirCount = reply->property("openSearchRedirCount").toInt();
        QUrl redir = reply->url().resolved(redirectVar.toUrl());
        reply->deleteLater();
        if (redirCount < 5) {
            fetchOpenSearchDescription(QString::fromUtf8(redir.toEncoded()));
            if (m_openSearchReply) {
                m_openSearchReply->setProperty("openSearchRedirCount", redirCount + 1);
            }
        }
        return;
    }

    QByteArray data = (reply->error() == QNetworkReply::NoError) ? reply->readAll() : QByteArray();
    QString descUrl = QString::fromUtf8(reply->url().toEncoded());
    reply->deleteLater();

    if (!data.isEmpty()) {
        pugi::xml_document doc;
        if (doc.load_buffer(data.constData(), data.size(), pugi::parse_default | pugi::parse_ws_pcdata)) {
            pugi::xml_node root = doc.first_child();
            for (pugi::xml_node n = doc.first_child(); n; n = n.next_sibling()) {
                if (n.type() == pugi::node_element && xmlLocalName(n).contains("opensearchdescription")) {
                    root = n;
                    break;
                }
            }
            QString bestTemplate;
            QString fallbackTemplate;
            for (pugi::xml_node urlNode = root.first_child(); urlNode; urlNode = urlNode.next_sibling()) {
                if (urlNode.type() != pugi::node_element || xmlLocalName(urlNode) != "url") continue;
                QString tpl = QString::fromUtf8(urlNode.attribute("template").as_string()).trimmed();
                QString typ = QString(urlNode.attribute("type").as_string()).toLower();
                if (tpl.isEmpty()) continue;
                QString resolved = resolveUrl(descUrl, tpl);
                resolved.replace("%7B", "{", Qt::CaseInsensitive).replace("%7D", "}", Qt::CaseInsensitive);
                if (typ.contains("opds+json") || typ.contains("atom+xml") || typ.contains("opds-catalog")) {
                    bestTemplate = resolved;
                    break;
                }
                if (fallbackTemplate.isEmpty()) {
                    fallbackTemplate = resolved;
                }
            }
            QString chosen = !bestTemplate.isEmpty() ? bestTemplate : fallbackTemplate;
            if (!chosen.isEmpty()) {
                m_searchUrlTemplate = chosen;
            }
        }
    }

    if (!m_pendingSearchQuery.isEmpty()) {
        QString q = m_pendingSearchQuery;
        m_pendingSearchQuery.clear();
        if (!m_searchUrlTemplate.isEmpty()) {
            search(q);
        }
    }
}

QString OpdsManager::resolveUrl(const QString &baseUrl, const QString &relativeUrl) const
{
    QString rel = relativeUrl.trimmed();
    if (rel.startsWith("http://") || rel.startsWith("https://")) {
        return rel;
    }
    QUrl base = baseUrl.contains('%') ? QUrl::fromEncoded(baseUrl.toUtf8()) : QUrl::fromUserInput(baseUrl);
    QUrl relUrl = (rel.contains('%') && !rel.contains(' ')) ? QUrl::fromEncoded(rel.toUtf8()) : QUrl(rel);
    return QString::fromUtf8(base.resolved(relUrl).toEncoded());
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
    QString trimmed = query.trimmed();
    if (trimmed.isEmpty()) return;

    if (m_hasSearch && m_searchUrlTemplate.isEmpty() && m_openSearchReply) {
        m_pendingSearchQuery = trimmed;
        return;
    }

    if (m_hasSearch && !m_searchUrlTemplate.isEmpty()) {
        QString sUrl = m_searchUrlTemplate;
        QString enc = QString::fromUtf8(QUrl::toPercentEncoding(trimmed));
        sUrl.replace("%7B", "{", Qt::CaseInsensitive).replace("%7D", "}", Qt::CaseInsensitive);

        QRegExp rfcQueryRe("\\{\\?([^}]+)\\}");
        QRegExp rfcAmpRe("\\{\\&([^}]+)\\}");
        if (sUrl.contains("{searchTerms}")) {
            sUrl.replace("{searchTerms}", enc);
        } else if (sUrl.contains("{searchTerms?}")) {
            sUrl.replace("{searchTerms?}", enc);
        } else if (sUrl.contains("{query}")) {
            sUrl.replace("{query}", enc);
        } else if (rfcQueryRe.indexIn(sUrl) != -1) {
            QString paramName = rfcQueryRe.cap(1).split(',').first().trimmed();
            if (paramName.isEmpty()) paramName = "query";
            QString sep = sUrl.contains('?') ? "&" : "?";
            sUrl.replace(rfcQueryRe.cap(0), QString("%1%2=%3").arg(sep, paramName, enc));
        } else if (rfcAmpRe.indexIn(sUrl) != -1) {
            QString paramName = rfcAmpRe.cap(1).split(',').first().trimmed();
            if (paramName.isEmpty()) paramName = "query";
            QString sep = sUrl.contains('?') ? "&" : "?";
            sUrl.replace(rfcAmpRe.cap(0), QString("%1%2=%3").arg(sep, paramName, enc));
        } else if (sUrl.contains("?")) {
            sUrl += "&query=" + enc;
        } else {
            sUrl += "?query=" + enc;
        }
        // Remove any remaining optional OpenSearch/RFC-6570 template parameters
        sUrl.remove(QRegExp("\\{[^}]+\\}"));

        openFeed(sUrl, tr("Tìm: \"%1\"").arg(trimmed), true);
    } else {
        // Filter local feed
        QString q = trimmed.toLower();
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
        {
            QNetworkReply *_dying = m_feedReply;
            m_feedReply = NULL;
            _dying->disconnect(this);
            _dying->abort();
            _dying->deleteLater();
        }
    }
    if (m_openSearchReply) {
        {
            QNetworkReply *_dying = m_openSearchReply;
            m_openSearchReply = NULL;
            _dying->disconnect(this);
            _dying->abort();
            _dying->deleteLater();
        }
    }
    m_pendingSearchQuery.clear();

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
    if (downloadUrl.trimmed().isEmpty()) {
        emit downloadFailed(tr("Liên kết tải sách không hợp lệ!"));
        return;
    }

    if (m_isDownloading) {
        QVariantMap job;
        job["title"] = title;
        job["author"] = author;
        job["downloadUrl"] = downloadUrl;
        job["format"] = format;
        job["coverUrl"] = coverUrl;
        m_downloadQueue.append(job);
        emit queuedCountChanged();
        emit downloadQueued(title.trimmed().isEmpty() ? tr("Sách chưa đặt tên") : title.trimmed(), m_downloadQueue.size());
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

    // Directory: /accounts/1000/shared/documents/Libris
    QString destDir = "/accounts/1000/shared/documents/Libris";
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

    m_downloadRedirectCount = 0;
    m_downloadFailedFallbackAttempted = false;

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

        m_downloadRedirectCount++;
        if (m_downloadRedirectCount > 5) {
            m_isDownloading = false;
            emit isDownloadingChanged();
            if (m_downloadFile) {
                m_downloadFile->close();
                m_downloadFile->remove();
                delete m_downloadFile;
                m_downloadFile = NULL;
            }
            emit downloadFailed(tr("Lỗi chuyển hướng tải sách quá nhiều lần (Redirect loop)!"));
            startNextQueuedDownload();
            return;
        }

        if (m_downloadFailedFallbackAttempted && redirectUrl.scheme().compare("https", Qt::CaseInsensitive) == 0) {
            m_isDownloading = false;
            emit isDownloadingChanged();
            if (m_downloadFile) {
                m_downloadFile->close();
                m_downloadFile->remove();
                delete m_downloadFile;
                m_downloadFile = NULL;
            }
            emit downloadFailed(tr("Máy chủ bắt buộc dùng HTTPS với chuẩn TLS/mã hóa mới mà thiết bị BB10 chưa hỗ trợ."));
            startNextQueuedDownload();
            return;
        }

        qDebug() << "[OpdsManager] Download redirecting (" << m_downloadRedirectCount << ") to:" << redirectUrl.toString();

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
        QNetworkReply::NetworkError netErr = m_downloadReply->error();
        int statusCode = m_downloadReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        // Only downgrade HTTPS->HTTP when no credentials are attached
        if ((netErr == QNetworkReply::SslHandshakeFailedError || err.contains("SSL", Qt::CaseInsensitive) || err.contains("handshake", Qt::CaseInsensitive))
            && m_currentUsername.isEmpty()
            && m_downloadDestPath.isEmpty() == false
            && m_downloadReply->url().toString().startsWith("https://", Qt::CaseInsensitive)
            && !m_downloadFailedFallbackAttempted) {

            m_downloadFailedFallbackAttempted = true;
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

        emit downloadFailed(formatNetworkError(statusCode, netErr, err, true));
        startNextQueuedDownload();
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
        covReply->setProperty("bookPath", m_downloadDestPath);
        setupReply(covReply);
        connect(covReply, SIGNAL(finished()), this, SLOT(onCoverReplyFinished()));
    }

    // Register the newly downloaded book directly in library without full disk scan
    if (m_bookManager) {
        m_bookManager->registerBookFile(m_downloadDestPath);
    }

    emit bookDownloaded(m_downloadDestPath, m_downloadBookTitle);
    startNextQueuedDownload();
}

void OpdsManager::onCoverReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QVariant redirectVar = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectVar.isNull()) {
        int redirCount = reply->property("coverRedirCount").toInt();
        QUrl redirUrl = reply->url().resolved(redirectVar.toUrl());
        QString cachePath = reply->property("cachePath").toString();
        QString bookPath = reply->property("bookPath").toString();
        reply->deleteLater();
        if (redirCount < 3 && redirUrl.isValid()) {
            QNetworkRequest req(redirUrl);
            prepareRequest(req);
            QNetworkReply *newReply = m_nam->get(req);
            setupReply(newReply);
            newReply->setProperty("cachePath", cachePath);
            newReply->setProperty("bookPath", bookPath);
            newReply->setProperty("coverRedirCount", redirCount + 1);
            connect(newReply, SIGNAL(finished()), this, SLOT(onCoverReplyFinished()));
        }
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        QString cachePath = reply->property("cachePath").toString();
        QString bookPath = reply->property("bookPath").toString();
        if (!cachePath.isEmpty()) {
            QFile f(cachePath);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(reply->readAll());
                f.close();
                if (m_bookManager && !bookPath.isEmpty()) {
                    m_bookManager->updateBookCoverInLibrary(bookPath, cachePath);
                }
            }
        }
    }
    reply->deleteLater();
}

void OpdsManager::onSslErrors(const QList<QSslError> &errors)
{
    for (int i = 0; i < errors.size(); ++i) {
        qWarning() << "[OpdsManager] SSL verification error:" << errors.at(i).errorString();
    }
}

void OpdsManager::onNamSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_UNUSED(reply);
    for (int i = 0; i < errors.size(); ++i) {
        qWarning() << "[OpdsManager] NAM SSL verification error:" << errors.at(i).errorString();
    }
}

void OpdsManager::cancelDownload()
{
    if (m_downloadReply) {
        {
            QNetworkReply *_dying = m_downloadReply;
            m_downloadReply = NULL;
            _dying->disconnect(this);
            _dying->abort();
            _dying->deleteLater();
        }
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
    startNextQueuedDownload();
}

void OpdsManager::clearDownloadQueue()
{
    if (!m_downloadQueue.isEmpty()) {
        m_downloadQueue.clear();
        emit queuedCountChanged();
    }
}

void OpdsManager::startNextQueuedDownload()
{
    if (m_isDownloading || m_downloadQueue.isEmpty()) {
        return;
    }
    QVariantMap nextJob = m_downloadQueue.takeFirst();
    emit queuedCountChanged();
    downloadBook(nextJob.value("title").toString(),
                 nextJob.value("author").toString(),
                 nextJob.value("downloadUrl").toString(),
                 nextJob.value("format").toString(),
                 nextJob.value("coverUrl").toString());
}
