#ifndef WIFIDROPSERVER_HPP
#define WIFIDROPSERVER_HPP

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QMap>

class WiFiDropServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(QString serverUrl READ serverUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(QString ipAddress READ ipAddress NOTIFY ipAddressChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(int sessionUploadCount READ sessionUploadCount NOTIFY sessionUploadCountChanged)
    Q_PROPERTY(QVariantList uploadedFiles READ uploadedFiles NOTIFY uploadedFilesChanged)
    Q_PROPERTY(QString accessPin READ accessPin NOTIFY accessPinChanged)

public:
    explicit WiFiDropServer(QObject *parent = 0);
    virtual ~WiFiDropServer();

    bool isRunning() const;
    QString serverUrl() const;
    QString ipAddress() const;
    int port() const;
    void setPort(int port);
    int sessionUploadCount() const;
    QVariantList uploadedFiles() const;
    QString accessPin() const;

    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void refreshIp();
    Q_INVOKABLE void clearHistory();

signals:
    void runningChanged();
    void serverUrlChanged();
    void ipAddressChanged();
    void portChanged();
    void sessionUploadCountChanged();
    void uploadedFilesChanged();
    void accessPinChanged();
    void bookUploaded(const QString &filePath, const QString &fileName, qint64 fileSize);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    struct ClientContext {
        enum State {
            STATE_HEADERS,
            STATE_PART_HEADER,
            STATE_STREAMING_BODY,
            STATE_CLIP_BODY,
            STATE_DONE
        };

        State state;
        QByteArray buffer;
        QString method;
        QString path;
        QString providedPin;
        QString boundary;
        QByteArray endDelimiter;
        qint64 contentLength;
        QFile *file;
        QString fileName;
        QString filePath;
        qint64 bytesWritten;

        ClientContext()
            : state(STATE_HEADERS)
            , contentLength(0)
            , file(NULL)
            , bytesWritten(0)
        {
        }

        ~ClientContext() {
            if (file) {
                bool removePartial = (state != STATE_DONE);
                if (file->isOpen()) {
                    file->close();
                }
                delete file;
                file = NULL;
                if (removePartial && !filePath.isEmpty() && QFile::exists(filePath)) {
                    QFile::remove(filePath);
                }
            }
        }
    };

    void processClient(QTcpSocket *socket, ClientContext *ctx);
    void sendResponse(QTcpSocket *socket, int statusCode, const QString &statusText, const QString &contentType, const QByteArray &body);
    QString detectIpAddress() const;
    QString targetUploadDir() const;
    static QString formatSize(qint64 bytes);
    QString getHtmlPage() const;

    QTcpServer *m_server;
    int m_port;
    bool m_running;
    QString m_ipAddress;
    QMap<QTcpSocket*, ClientContext*> m_clients;
    QVariantList m_uploadedFiles;
    QString m_accessPin;

    bool isAuthorizedRequest(const ClientContext *ctx) const;
};

#endif // WIFIDROPSERVER_HPP
