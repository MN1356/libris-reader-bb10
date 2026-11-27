#ifndef TLSHTTPCLIENT_HPP
#define TLSHTTPCLIENT_HPP

#include <QThread>
#include <QString>
#include <QByteArray>

/**
 * Native OpenSSL TLS 1.2 HTTPS worker thread for BlackBerry 10.
 *
 * Bypasses Qt 4.8.6's QSslCipher_from_SSL_CIPHER parser limitation (which omits
 * OpenSSL 1.0.1i's TLSv1.2 AEAD/SHA256 cipher suites), enabling compliant TLS 1.2
 * connections to modern GitHub endpoints while performing full X.509 chain and
 * SAN/CN hostname verification against the bundled Mozilla CA roots.
 */
class TlsHttpWorker : public QThread
{
    Q_OBJECT
public:
    TlsHttpWorker(const QString &url,
                  const QString &acceptHeader,
                  const QString &outputFilePath = QString(),
                  QObject *parent = 0);
    ~TlsHttpWorker();

    void requestCancel();
    bool isCancelled() const;

signals:
    void progressChanged(qint64 bytesReceived, qint64 bytesTotal);
    void requestFinished(bool ok, int httpStatus, const QByteArray &body, const QString &errorString);

protected:
    void run();

private:
    QString m_url;
    QString m_acceptHeader;
    QString m_outputFilePath;
    volatile bool m_cancelRequested;
};

#endif // TLSHTTPCLIENT_HPP
