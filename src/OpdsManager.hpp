#ifndef OPDSMANAGER_HPP
#define OPDSMANAGER_HPP

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QSslError>
#include <QSslConfiguration>
#include <bb/cascades/GroupDataModel>

class BookManager;

class OpdsManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bb::cascades::GroupDataModel* feedModel READ feedModel CONSTANT)
    Q_PROPERTY(bb::cascades::GroupDataModel* catalogModel READ catalogModel CONSTANT)
    Q_PROPERTY(QVariantList catalogs READ catalogs NOTIFY catalogsChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY isDownloadingChanged)
    Q_PROPERTY(int downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString downloadStatus READ downloadStatus NOTIFY downloadStatusChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTitleChanged)
    Q_PROPERTY(QString currentUrl READ currentUrl NOTIFY currentUrlChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY canGoBackChanged)
    Q_PROPERTY(bool hasSearch READ hasSearch NOTIFY hasSearchChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool isAtRoot READ isAtRoot NOTIFY isAtRootChanged)

public:
    explicit OpdsManager(BookManager *bookManager, QObject *parent = 0);
    virtual ~OpdsManager();

    bb::cascades::GroupDataModel* feedModel() const { return m_feedModel; }
    bb::cascades::GroupDataModel* catalogModel() const { return m_catalogModel; }
    QVariantList catalogs() const { return m_catalogs; }
    bool isLoading() const { return m_isLoading; }
    bool isDownloading() const { return m_isDownloading; }
    int downloadProgress() const { return m_downloadProgress; }
    QString downloadStatus() const { return m_downloadStatus; }
    QString currentTitle() const { return m_currentTitle; }
    QString currentUrl() const { return m_currentUrl; }
    bool canGoBack() const { return !m_historyStack.isEmpty(); }
    bool hasSearch() const { return m_hasSearch; }
    QString statusMessage() const { return m_statusMessage; }
    bool isAtRoot() const { return m_isAtRoot; }

    Q_INVOKABLE void loadCatalogs();
    Q_INVOKABLE void addCatalog(const QString &name, const QString &url, const QString &username = "", const QString &password = "", const QString &description = "");
    Q_INVOKABLE void updateCatalog(const QString &id, const QString &name, const QString &url, const QString &username = "", const QString &password = "", const QString &description = "");
    Q_INVOKABLE void removeCatalog(const QString &id);
    Q_INVOKABLE void resetDefaultCatalogs();

    Q_INVOKABLE void openCatalog(const QString &catalogId);
    Q_INVOKABLE void openFeed(const QString &url, const QString &title = "", bool addToHistory = true);
    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void goHome();
    Q_INVOKABLE void refreshFeed();

    Q_INVOKABLE void downloadBook(const QString &title, const QString &author, const QString &downloadUrl, const QString &format, const QString &coverUrl = "");
    Q_INVOKABLE void cancelDownload();

Q_SIGNALS:
    void catalogsChanged();
    void feedLoaded();
    void isLoadingChanged();
    void isDownloadingChanged();
    void downloadProgressChanged();
    void downloadStatusChanged();
    void currentTitleChanged();
    void currentUrlChanged();
    void canGoBackChanged();
    void hasSearchChanged();
    void statusMessageChanged();
    void isAtRootChanged();
    void bookDownloaded(const QString &filePath, const QString &title);
    void downloadFailed(const QString &errorMsg);

private Q_SLOTS:
    void onFeedReplyFinished();
    void onDownloadDataReceived();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished();
    void onCoverReplyFinished();
    void onSslErrors(const QList<QSslError> &errors);
    void onNamSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    void prepareRequest(QNetworkRequest &req);
    void setupReply(QNetworkReply *reply);
    void parseAtomFeed(const QByteArray &data);
    QString resolveUrl(const QString &baseUrl, const QString &relativeUrl) const;
    QString stripHtml(const QString &html) const;
    void sendFeedRequest(const QString &url);
    void rebuildCatalogModel();

    BookManager *m_bookManager;
    QNetworkAccessManager *m_nam;

    bb::cascades::GroupDataModel *m_feedModel;
    bb::cascades::GroupDataModel *m_catalogModel;
    QVariantList m_currentFeedItems;
    QVariantList m_catalogs;

    bool m_isLoading;
    bool m_isDownloading;
    int m_downloadProgress;
    QString m_downloadStatus;
    QString m_currentTitle;
    QString m_currentUrl;
    bool m_hasSearch;
    QString m_searchUrlTemplate;
    QString m_nextPageUrl;
    QString m_statusMessage;
    bool m_isAtRoot;

    // Credentials for currently active catalog
    QString m_currentUsername;
    QString m_currentPassword;

    // Navigation history stack: QVariantMap with "url", "title", "username", "password"
    QList<QVariantMap> m_historyStack;

    // Active feed reply
    QNetworkReply *m_feedReply;

    // Active book download
    QNetworkReply *m_downloadReply;
    QFile *m_downloadFile;
    QString m_downloadDestPath;
    QString m_downloadBookTitle;
    QString m_downloadCoverUrl;
};

#endif // OPDSMANAGER_HPP
