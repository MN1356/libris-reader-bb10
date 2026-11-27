#include "TlsHttpClient.hpp"

#include <QUrl>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/asn1.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

static QMutex s_sslInitMutex;
static bool s_sslInitialized = false;

static void ensureOpenSslInitialized()
{
    QMutexLocker locker(&s_sslInitMutex);
    if (!s_sslInitialized) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        s_sslInitialized = true;
    }
}

static QString findCaBundlePath()
{
    if (QFile::exists("app/native/assets/certs/cacert.pem")) {
        return "app/native/assets/certs/cacert.pem";
    }
    if (QFile::exists("assets/certs/cacert.pem")) {
        return "assets/certs/cacert.pem";
    }
    return QString();
}

static QString formatOpenSslError(const QString &prefix)
{
    unsigned long errCode = ERR_get_error();
    if (errCode == 0) {
        return prefix;
    }
    char errBuf[256];
    ERR_error_string_n(errCode, errBuf, sizeof(errBuf));
    while (ERR_get_error() != 0) {}
    return QString("%1 (%2)").arg(prefix).arg(QString::fromLatin1(errBuf));
}

static bool matchWildcardHost(const QString &pattern, const QString &host)
{
    QString p = pattern.trimmed().toLower();
    QString h = host.trimmed().toLower();
    if (p.isEmpty() || h.isEmpty()) return false;
    if (p == h) return true;

    // RFC 6125 single-label wildcard matching: "*.example.com" matches "a.example.com" only
    if (p.startsWith("*.")) {
        QString suffix = p.mid(1); // ".example.com"
        if (!suffix.contains('.')) return false;
        int firstDot = h.indexOf('.');
        if (firstDot <= 0) return false;
        QString remainder = h.mid(firstDot);
        return remainder == suffix;
    }
    return false;
}

static bool verifyPeerHostname(X509 *cert, const QString &host)
{
    if (!cert || host.isEmpty()) return false;

    bool hasDnsSan = false;
    GENERAL_NAMES *sans = (GENERAL_NAMES *)X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
    if (sans) {
        int count = sk_GENERAL_NAME_num(sans);
        for (int i = 0; i < count; ++i) {
            const GENERAL_NAME *entry = sk_GENERAL_NAME_value(sans, i);
            if (!entry || entry->type != GEN_DNS || !entry->d.dNSName) continue;
            hasDnsSan = true;
            const unsigned char *data = ASN1_STRING_data(entry->d.dNSName);
            int len = ASN1_STRING_length(entry->d.dNSName);
            if (data && len > 0) {
                QString dnsName = QString::fromUtf8((const char *)data, len);
                if (matchWildcardHost(dnsName, host)) {
                    GENERAL_NAMES_free(sans);
                    return true;
                }
            }
        }
        GENERAL_NAMES_free(sans);
    }

    if (hasDnsSan) {
        return false;
    }

    // Fallback to Subject CommonName only when no DNS SAN entries exist
    X509_NAME *subj = X509_get_subject_name(cert);
    if (!subj) return false;
    int idx = -1;
    while ((idx = X509_NAME_get_index_by_NID(subj, NID_commonName, idx)) >= 0) {
        X509_NAME_ENTRY *ne = X509_NAME_get_entry(subj, idx);
        if (!ne) continue;
        ASN1_STRING *asn = X509_NAME_ENTRY_get_data(ne);
        if (!asn) continue;
        unsigned char *utf8 = NULL;
        int len = ASN1_STRING_to_UTF8(&utf8, asn);
        if (len > 0 && utf8) {
            QString cn = QString::fromUtf8((const char *)utf8, len);
            OPENSSL_free(utf8);
            if (matchWildcardHost(cn, host)) {
                return true;
            }
        }
    }
    return false;
}

static bool waitSocketReady(int fd, bool waitForWrite, int timeoutMs, volatile bool *cancelFlag)
{
    int elapsed = 0;
    const int sliceMs = 200;
    while (elapsed < timeoutMs) {
        if (cancelFlag && *cancelFlag) return false;

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        int curSlice = qMin(sliceMs, timeoutMs - elapsed);
        struct timeval tv;
        tv.tv_sec = curSlice / 1000;
        tv.tv_usec = (curSlice % 1000) * 1000;

        int rc = select(fd + 1, waitForWrite ? NULL : &fds, waitForWrite ? &fds : NULL, NULL, &tv);
        if (rc > 0) {
            return true;
        }
        if (rc < 0 && errno != EINTR) {
            return false;
        }
        elapsed += curSlice;
    }
    return false;
}

static int connectTcpWithTimeout(const QString &host, int port, int timeoutMs, volatile bool *cancelFlag, QString &errOut)
{
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    QByteArray hostUtf8 = host.toUtf8();
    QByteArray portStr = QByteArray::number(port);
    struct addrinfo *res = NULL;
    int gai = getaddrinfo(hostUtf8.constData(), portStr.constData(), &hints, &res);
    if (gai != 0 || !res) {
        errOut = QString("DNS resolution failed for %1: %2").arg(host).arg(QString::fromLatin1(gai_strerror(gai)));
        return -1;
    }

    int connectedFd = -1;
    for (struct addrinfo *rp = res; rp != NULL; rp = rp->ai_next) {
        if (cancelFlag && *cancelFlag) break;

        int fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) continue;

        int flags = fcntl(fd, F_GETFL, 0);
        if (flags >= 0) {
            fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        }

        int rc = ::connect(fd, rp->ai_addr, rp->ai_addrlen);
        if (rc == 0) {
            connectedFd = fd;
            break;
        }
        if (errno == EINPROGRESS) {
            if (waitSocketReady(fd, true, timeoutMs, cancelFlag)) {
                int soErr = 0;
                socklen_t len = sizeof(soErr);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &soErr, &len) == 0 && soErr == 0) {
                    connectedFd = fd;
                    break;
                }
            }
        }
        ::close(fd);
    }

    freeaddrinfo(res);
    if (connectedFd < 0 && errOut.isEmpty()) {
        errOut = (cancelFlag && *cancelFlag) ? QString("Cancelled") : QString("Connection timed out or refused (%1)").arg(host);
    }
    return connectedFd;
}

static bool sslWriteAll(SSL *ssl, int fd, const char *data, int len, volatile bool *cancelFlag, QString &errOut)
{
    int sent = 0;
    while (sent < len) {
        if (cancelFlag && *cancelFlag) {
            errOut = "Cancelled";
            return false;
        }
        int n = SSL_write(ssl, data + sent, len - sent);
        if (n > 0) {
            sent += n;
            continue;
        }
        int sslErr = SSL_get_error(ssl, n);
        if (sslErr == SSL_ERROR_WANT_READ) {
            if (!waitSocketReady(fd, false, 20000, cancelFlag)) {
                errOut = "TLS write timeout";
                return false;
            }
        } else if (sslErr == SSL_ERROR_WANT_WRITE) {
            if (!waitSocketReady(fd, true, 20000, cancelFlag)) {
                errOut = "TLS write timeout";
                return false;
            }
        } else {
            errOut = formatOpenSslError("TLS write failed");
            return false;
        }
    }
    return true;
}

static int sslReadChunk(SSL *ssl, int fd, char *buf, int bufSize, volatile bool *cancelFlag, QString &errOut)
{
    while (true) {
        if (cancelFlag && *cancelFlag) {
            errOut = "Cancelled";
            return -1;
        }
        int n = SSL_read(ssl, buf, bufSize);
        if (n > 0) {
            return n;
        }
        int sslErr = SSL_get_error(ssl, n);
        if (sslErr == SSL_ERROR_ZERO_RETURN) {
            return 0;
        }
        if (sslErr == SSL_ERROR_WANT_READ) {
            if (!waitSocketReady(fd, false, 25000, cancelFlag)) {
                errOut = (cancelFlag && *cancelFlag) ? "Cancelled" : "TLS read timeout";
                return -1;
            }
            continue;
        }
        if (sslErr == SSL_ERROR_WANT_WRITE) {
            if (!waitSocketReady(fd, true, 25000, cancelFlag)) {
                errOut = (cancelFlag && *cancelFlag) ? "Cancelled" : "TLS read timeout";
                return -1;
            }
            continue;
        }
        if (sslErr == SSL_ERROR_SYSCALL && n == 0 && ERR_peek_error() == 0) {
            // Peer closed TCP stream at end of response
            return 0;
        }
        errOut = formatOpenSslError("TLS read failed");
        return -1;
    }
}

TlsHttpWorker::TlsHttpWorker(const QString &url,
                             const QString &acceptHeader,
                             const QString &outputFilePath,
                             QObject *parent)
    : QThread(parent)
    , m_url(url)
    , m_acceptHeader(acceptHeader)
    , m_outputFilePath(outputFilePath)
    , m_cancelRequested(false)
{
}

TlsHttpWorker::~TlsHttpWorker()
{
    requestCancel();
    wait(3000);
}

void TlsHttpWorker::requestCancel()
{
    m_cancelRequested = true;
}

bool TlsHttpWorker::isCancelled() const
{
    return m_cancelRequested;
}

void TlsHttpWorker::run()
{
    ensureOpenSslInitialized();

    QString caPath = findCaBundlePath();
    if (caPath.isEmpty()) {
        emit requestFinished(false, 0, QByteArray(), "Trusted CA bundle (cacert.pem) not found");
        return;
    }

    QUrl currentUrl(m_url);
    const int maxRedirects = 8;

    for (int hop = 0; hop <= maxRedirects; ++hop) {
        if (m_cancelRequested) {
            emit requestFinished(false, 0, QByteArray(), "Cancelled");
            return;
        }

        if (!currentUrl.isValid() || currentUrl.scheme().toLower() != "https" || currentUrl.host().isEmpty()) {
            emit requestFinished(false, 0, QByteArray(), "Invalid HTTPS URL: " + currentUrl.toString());
            return;
        }

        QString host = currentUrl.host();
        int port = currentUrl.port(443);

        QString errMsg;
        int fd = connectTcpWithTimeout(host, port, 15000, &m_cancelRequested, errMsg);
        if (fd < 0) {
            emit requestFinished(false, 0, QByteArray(), errMsg);
            return;
        }

        SSL_CTX *ctx = SSL_CTX_new(TLSv1_2_client_method());
        if (!ctx) {
            ::close(fd);
            emit requestFinished(false, 0, QByteArray(), formatOpenSslError("SSL_CTX_new failed"));
            return;
        }

        SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION);
        SSL_CTX_set_cipher_list(ctx,
            "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:"
            "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:"
            "ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256:"
            "ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA384:"
            "AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:"
            "HIGH:!aNULL:!eNULL:!MD5:!RC4:!EXPORT");

        if (SSL_CTX_load_verify_locations(ctx, caPath.toUtf8().constData(), NULL) != 1) {
            QString loadErr = formatOpenSslError("Failed to load CA bundle");
            SSL_CTX_free(ctx);
            ::close(fd);
            emit requestFinished(false, 0, QByteArray(), loadErr);
            return;
        }
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

        SSL *ssl = SSL_new(ctx);
        if (!ssl) {
            SSL_CTX_free(ctx);
            ::close(fd);
            emit requestFinished(false, 0, QByteArray(), formatOpenSslError("SSL_new failed"));
            return;
        }

        SSL_set_fd(ssl, fd);
        SSL_set_tlsext_host_name(ssl, host.toUtf8().constData());

        bool handshakeOk = false;
        while (!m_cancelRequested) {
            int rc = SSL_connect(ssl);
            if (rc == 1) {
                handshakeOk = true;
                break;
            }
            int sslErr = SSL_get_error(ssl, rc);
            if (sslErr == SSL_ERROR_WANT_READ) {
                if (!waitSocketReady(fd, false, 15000, &m_cancelRequested)) {
                    errMsg = m_cancelRequested ? "Cancelled" : "SSL handshake timeout";
                    break;
                }
            } else if (sslErr == SSL_ERROR_WANT_WRITE) {
                if (!waitSocketReady(fd, true, 15000, &m_cancelRequested)) {
                    errMsg = m_cancelRequested ? "Cancelled" : "SSL handshake timeout";
                    break;
                }
            } else {
                errMsg = formatOpenSslError("SSL handshake failed");
                qWarning() << "[TlsHttp]" << host << errMsg;
                break;
            }
        }

        if (!handshakeOk) {
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            ::close(fd);
            emit requestFinished(false, 0, QByteArray(), errMsg.isEmpty() ? "Cancelled" : errMsg);
            return;
        }

        long verifyRes = SSL_get_verify_result(ssl);
        if (verifyRes != X509_V_OK) {
            QString vErr = QString("Certificate verification failed (%1)").arg(QString::fromLatin1(X509_verify_cert_error_string(verifyRes)));
            qWarning() << "[TlsHttp]" << host << vErr;
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            ::close(fd);
            emit requestFinished(false, 0, QByteArray(), vErr);
            return;
        }

        X509 *peerCert = SSL_get_peer_certificate(ssl);
        bool hostOk = verifyPeerHostname(peerCert, host);
        if (peerCert) X509_free(peerCert);
        if (!hostOk) {
            qWarning() << "[TlsHttp] Hostname mismatch for" << host;
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            ::close(fd);
            emit requestFinished(false, 0, QByteArray(), "SSL certificate hostname mismatch");
            return;
        }

        qDebug() << "[TlsHttp] Connected:" << host << "protocol=" << SSL_get_version(ssl) << "cipher=" << SSL_get_cipher(ssl);

        QByteArray pathBytes = currentUrl.encodedPath();
        if (pathBytes.isEmpty()) pathBytes = "/";
        QByteArray queryBytes = currentUrl.encodedQuery();
        if (!queryBytes.isEmpty()) {
            pathBytes.append('?');
            pathBytes.append(queryBytes);
        }

        QByteArray hostHeader = host.toUtf8();
        if (port != 443) {
            hostHeader.append(':');
            hostHeader.append(QByteArray::number(port));
        }

        QByteArray req;
        req.append("GET ");
        req.append(pathBytes);
        req.append(" HTTP/1.1\r\n");
        req.append("Host: ");
        req.append(hostHeader);
        req.append("\r\nUser-Agent: Libris-BB10/2.3\r\n");
        if (!m_acceptHeader.isEmpty()) {
            req.append("Accept: ");
            req.append(m_acceptHeader.toUtf8());
            req.append("\r\n");
        }
        req.append("Accept-Encoding: identity\r\n");
        req.append("Connection: close\r\n\r\n");

        if (!sslWriteAll(ssl, fd, req.constData(), req.size(), &m_cancelRequested, errMsg)) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            ::close(fd);
            emit requestFinished(false, 0, QByteArray(), errMsg);
            return;
        }

        // Read HTTP headers
        QByteArray headerBuf;
        int headerEndIdx = -1;
        char ioBuf[16384];
        while (headerEndIdx < 0) {
            int n = sslReadChunk(ssl, fd, ioBuf, sizeof(ioBuf), &m_cancelRequested, errMsg);
            if (n < 0) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                ::close(fd);
                emit requestFinished(false, 0, QByteArray(), errMsg);
                return;
            }
            if (n == 0) break;
            headerBuf.append(ioBuf, n);
            headerEndIdx = headerBuf.indexOf("\r\n\r\n");
            if (headerBuf.size() > 65536 && headerEndIdx < 0) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                ::close(fd);
                emit requestFinished(false, 0, QByteArray(), "HTTP response header too large");
                return;
            }
        }

        if (headerEndIdx < 0) {
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            ::close(fd);
            emit requestFinished(false, 0, QByteArray(), "Incomplete HTTP response");
            return;
        }

        QByteArray rawHeaders = headerBuf.left(headerEndIdx);
        QByteArray bodyLeftover = headerBuf.mid(headerEndIdx + 4);

        QList<QByteArray> headerLines = rawHeaders.split('\n');
        int httpStatus = 0;
        if (!headerLines.isEmpty()) {
            QByteArray statusLine = headerLines.at(0).trimmed();
            int sp1 = statusLine.indexOf(' ');
            if (sp1 > 0) {
                QByteArray rest = statusLine.mid(sp1 + 1).trimmed();
                int sp2 = rest.indexOf(' ');
                httpStatus = (sp2 > 0 ? rest.left(sp2) : rest).toInt();
            }
        }

        QByteArray locationHeader;
        qint64 contentLength = -1;
        bool isChunked = false;
        for (int i = 1; i < headerLines.size(); ++i) {
            QByteArray line = headerLines.at(i).trimmed();
            int colon = line.indexOf(':');
            if (colon <= 0) continue;
            QByteArray key = line.left(colon).trimmed().toLower();
            QByteArray val = line.mid(colon + 1).trimmed();
            if (key == "location") {
                locationHeader = val;
            } else if (key == "content-length") {
                bool okLen = false;
                qint64 parsed = val.toLongLong(&okLen);
                if (okLen && parsed >= 0) contentLength = parsed;
            } else if (key == "transfer-encoding") {
                if (val.toLower().contains("chunked")) isChunked = true;
            }
        }

        // Handle HTTP redirects
        if ((httpStatus == 301 || httpStatus == 302 || httpStatus == 303 || httpStatus == 307 || httpStatus == 308)
            && !locationHeader.isEmpty()) {
            QUrl nextUrl = currentUrl.resolved(QUrl::fromEncoded(locationHeader));
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            ::close(fd);
            if (!nextUrl.isValid() || nextUrl.scheme().toLower() != "https") {
                emit requestFinished(false, httpStatus, QByteArray(), "Unsupported redirect target scheme");
                return;
            }
            currentUrl = nextUrl;
            continue;
        }

        // Stream response body (either to file or into memory)
        QFile outFile;
        bool writeToFile = (!m_outputFilePath.isEmpty() && httpStatus >= 200 && httpStatus < 300);
        if (writeToFile) {
            outFile.setFileName(m_outputFilePath);
            if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                ::close(fd);
                emit requestFinished(false, httpStatus, QByteArray(), "Cannot open output file for writing");
                return;
            }
        }

        QByteArray memoryBody;
        qint64 bytesReceived = 0;
        int lastEmittedPct = -1;
        qint64 lastEmittedBytes = 0;

        bool transferOk = true;
        const qint64 maxMemBytes = 4 * 1024 * 1024;

        if (isChunked) {
            QByteArray chunkBuf = bodyLeftover;
            bool doneChunks = false;
            while (!doneChunks && !m_cancelRequested) {
                int lineEnd = chunkBuf.indexOf("\r\n");
                while (lineEnd < 0) {
                    int n = sslReadChunk(ssl, fd, ioBuf, sizeof(ioBuf), &m_cancelRequested, errMsg);
                    if (n < 0) { transferOk = false; break; }
                    if (n == 0) { errMsg = "Unexpected EOF in chunked stream"; transferOk = false; break; }
                    chunkBuf.append(ioBuf, n);
                    lineEnd = chunkBuf.indexOf("\r\n");
                }
                if (!transferOk || m_cancelRequested) break;

                QByteArray sizeLine = chunkBuf.left(lineEnd);
                int semi = sizeLine.indexOf(';');
                if (semi >= 0) sizeLine = sizeLine.left(semi);
                bool okHex = false;
                qint64 chunkSize = sizeLine.trimmed().toLongLong(&okHex, 16);
                chunkBuf.remove(0, lineEnd + 2);
                if (!okHex || chunkSize < 0) {
                    errMsg = "Malformed HTTP chunk header";
                    transferOk = false;
                    break;
                }
                if (chunkSize == 0) {
                    doneChunks = true;
                    break;
                }

                while (chunkBuf.size() < chunkSize + 2) {
                    int n = sslReadChunk(ssl, fd, ioBuf, sizeof(ioBuf), &m_cancelRequested, errMsg);
                    if (n < 0) { transferOk = false; break; }
                    if (n == 0) { errMsg = "Unexpected EOF reading HTTP chunk"; transferOk = false; break; }
                    chunkBuf.append(ioBuf, n);
                }
                if (!transferOk || m_cancelRequested) break;

                QByteArray piece = chunkBuf.left((int)chunkSize);
                chunkBuf.remove(0, (int)chunkSize + 2);

                if (writeToFile) {
                    if (outFile.write(piece) != piece.size()) {
                        errMsg = "Disk write failed";
                        transferOk = false;
                        break;
                    }
                } else {
                    if (memoryBody.size() + piece.size() > maxMemBytes) {
                        errMsg = "Response too large";
                        transferOk = false;
                        break;
                    }
                    memoryBody.append(piece);
                }
                bytesReceived += piece.size();
                if (contentLength > 0) {
                    int pct = qBound(0, (int)((bytesReceived * 100) / contentLength), 100);
                    if (pct != lastEmittedPct) {
                        lastEmittedPct = pct;
                        emit progressChanged(bytesReceived, contentLength);
                    }
                } else if (bytesReceived - lastEmittedBytes >= 65536) {
                    lastEmittedBytes = bytesReceived;
                    emit progressChanged(bytesReceived, -1);
                }
            }
        } else {
            if (!bodyLeftover.isEmpty()) {
                QByteArray initial = bodyLeftover;
                if (contentLength >= 0 && initial.size() > contentLength) {
                    initial = initial.left((int)contentLength);
                }
                if (writeToFile) {
                    if (outFile.write(initial) != initial.size()) {
                        errMsg = "Disk write failed";
                        transferOk = false;
                    }
                } else {
                    memoryBody.append(initial);
                }
                bytesReceived += initial.size();
                if (contentLength > 0) {
                    int pct = qBound(0, (int)((bytesReceived * 100) / contentLength), 100);
                    lastEmittedPct = pct;
                    emit progressChanged(bytesReceived, contentLength);
                }
            }

            while (transferOk && !m_cancelRequested) {
                if (contentLength >= 0 && bytesReceived >= contentLength) {
                    break;
                }
                int n = sslReadChunk(ssl, fd, ioBuf, sizeof(ioBuf), &m_cancelRequested, errMsg);
                if (n < 0) {
                    transferOk = false;
                    break;
                }
                if (n == 0) {
                    if (contentLength >= 0 && bytesReceived < contentLength) {
                        errMsg = "Connection closed before download completed";
                        transferOk = false;
                    }
                    break;
                }
                int take = n;
                if (contentLength >= 0 && bytesReceived + take > contentLength) {
                    take = (int)(contentLength - bytesReceived);
                }
                if (writeToFile) {
                    if (outFile.write(ioBuf, take) != take) {
                        errMsg = "Disk write failed";
                        transferOk = false;
                        break;
                    }
                } else {
                    if (memoryBody.size() + take > maxMemBytes) {
                        errMsg = "Response too large";
                        transferOk = false;
                        break;
                    }
                    memoryBody.append(ioBuf, take);
                }
                bytesReceived += take;
                if (contentLength > 0) {
                    int pct = qBound(0, (int)((bytesReceived * 100) / contentLength), 100);
                    if (pct != lastEmittedPct) {
                        lastEmittedPct = pct;
                        emit progressChanged(bytesReceived, contentLength);
                    }
                } else if (bytesReceived - lastEmittedBytes >= 65536) {
                    lastEmittedBytes = bytesReceived;
                    emit progressChanged(bytesReceived, -1);
                }
            }
        }

        if (writeToFile) {
            outFile.flush();
            outFile.close();
            if (!transferOk || m_cancelRequested) {
                QFile::remove(m_outputFilePath);
            }
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        ::close(fd);

        if (m_cancelRequested) {
            emit requestFinished(false, httpStatus, QByteArray(), "Cancelled");
            return;
        }
        if (!transferOk) {
            emit requestFinished(false, httpStatus, QByteArray(), errMsg);
            return;
        }

        emit requestFinished(httpStatus >= 200 && httpStatus < 300,
                             httpStatus,
                             memoryBody,
                             (httpStatus >= 200 && httpStatus < 300) ? QString() : QString("HTTP %1").arg(httpStatus));
        return;
    }

    emit requestFinished(false, 0, QByteArray(), "Too many HTTP redirects");
}
