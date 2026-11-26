#include "WiFiDropServer.hpp"
#include "BookManager.hpp"
#include <QDebug>
#include <QRegExp>

static QString escapeWifiJson(const QString &s)
{
    QString out;
    out.reserve(s.size() + 8);
    for (int i = 0; i < s.size(); ++i) {
        const QChar c = s.at(i);
        ushort u = c.unicode();
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (u < 0x20) out += QString("\\u%1").arg(u, 4, 16, QChar('0'));
        else out += c;
    }
    return out;
}

static void streamFileToSocket(QTcpSocket *socket, QFile &file, const QString &downloadFileName)
{
    if (!socket || !file.isOpen()) return;
    qint64 totalSize = file.size();
    QString safeFn = downloadFileName;
    safeFn.replace("\\", "_").replace("\"", "_").remove('\r').remove('\n');
    QByteArray hdr = QString("HTTP/1.1 200 OK\r\n"
                             "Content-Type: application/octet-stream\r\n"
                             "Content-Disposition: attachment; filename=\"%1\"\r\n"
                             "Content-Length: %2\r\n"
                             "Access-Control-Allow-Origin: *\r\n"
                             "Connection: close\r\n\r\n")
                         .arg(safeFn).arg(totalSize).toUtf8();
    socket->write(hdr);
    static const qint64 CHUNK_SIZE = 65536;
    char buf[65536];
    while (!file.atEnd() && socket->state() == QAbstractSocket::ConnectedState) {
        qint64 n = file.read(buf, CHUNK_SIZE);
        if (n <= 0) break;
        qint64 written = socket->write(buf, n);
        if (written < 0) break;
        if (socket->bytesToWrite() > 262144) {
            if (!socket->waitForBytesWritten(5000)) {
                break;
            }
        }
    }
    socket->flush();
    socket->waitForBytesWritten(2000);
}

WiFiDropServer::WiFiDropServer(QObject *parent)
    : QObject(parent)
    , m_server(NULL)
    , m_port(8080)
    , m_running(false)
{
    m_ipAddress = detectIpAddress();
}

WiFiDropServer::~WiFiDropServer()
{
    stop();
}

bool WiFiDropServer::isRunning() const
{
    return m_running;
}

QString WiFiDropServer::serverUrl() const
{
    if (!m_running || m_ipAddress.isEmpty()) {
        return QString();
    }
    return QString("http://%1:%2").arg(m_ipAddress).arg(m_port);
}

QString WiFiDropServer::ipAddress() const
{
    return m_ipAddress;
}

int WiFiDropServer::port() const
{
    return m_port;
}

void WiFiDropServer::setPort(int port)
{
    if (m_port != port && port > 1024 && port <= 65535) {
        m_port = port;
        emit portChanged();
        if (m_running) {
            stop();
            start();
        }
    }
}

int WiFiDropServer::sessionUploadCount() const
{
    return m_uploadedFiles.size();
}

QVariantList WiFiDropServer::uploadedFiles() const
{
    return m_uploadedFiles;
}

void WiFiDropServer::clearHistory()
{
    m_uploadedFiles.clear();
    emit uploadedFilesChanged();
    emit sessionUploadCountChanged();
}

void WiFiDropServer::refreshIp()
{
    QString newIp = detectIpAddress();
    if (newIp != m_ipAddress) {
        m_ipAddress = newIp;
        emit ipAddressChanged();
        emit serverUrlChanged();
    }
}

bool WiFiDropServer::start()
{
    if (m_running) {
        return true;
    }

    refreshIp();

    if (!m_server) {
        m_server = new QTcpServer(this);
        connect(m_server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
    }

    if (!m_server->listen(QHostAddress::Any, m_port)) {
        qWarning() << "[WiFiDropServer] Failed to listen on port" << m_port << ":" << m_server->errorString();
        m_running = false;
        emit runningChanged();
        emit serverUrlChanged();
        return false;
    }

    m_running = true;
    qDebug() << "[WiFiDropServer] Listening on" << serverUrl() << "(ip=" << m_ipAddress << "port=" << m_port << ")";
    emit runningChanged();
    emit serverUrlChanged();
    return true;
}

void WiFiDropServer::stop()
{
    if (m_server && m_server->isListening()) {
        m_server->close();
    }

    // Clean up active client contexts
    QList<QTcpSocket*> sockets = m_clients.keys();
    for (int i = 0; i < sockets.size(); ++i) {
        QTcpSocket *sock = sockets.at(i);
        ClientContext *ctx = m_clients.take(sock);
        if (ctx) {
            if (ctx->state != ClientContext::STATE_DONE && ctx->file) {
                QString partialPath = ctx->filePath;
                if (ctx->file->isOpen()) ctx->file->close();
                delete ctx->file;
                ctx->file = NULL;
                if (!partialPath.isEmpty() && QFile::exists(partialPath)) {
                    QFile::remove(partialPath);
                }
            }
            delete ctx;
        }
        sock->abort();
        sock->deleteLater();
    }
    m_clients.clear();

    if (m_running) {
        m_running = false;
        emit runningChanged();
        emit serverUrlChanged();
    }
}

void WiFiDropServer::onNewConnection()
{
    if (!m_server) return;

    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        if (!socket) continue;

        ClientContext *ctx = new ClientContext();
        m_clients.insert(socket, ctx);

        connect(socket, SIGNAL(readyRead()), this, SLOT(onClientReadyRead()));
        connect(socket, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onClientDisconnected()));
    }
}

void WiFiDropServer::onClientReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !m_clients.contains(socket)) return;

    ClientContext *ctx = m_clients.value(socket);
    QByteArray incoming = socket->readAll();
    ctx->buffer.append(incoming);

    processClient(socket, ctx);
}

void WiFiDropServer::onClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    if (m_clients.contains(socket)) {
        ClientContext *ctx = m_clients.take(socket);
        if (ctx) {
            if (ctx->state != ClientContext::STATE_DONE && ctx->file) {
                QString partialPath = ctx->filePath;
                if (ctx->file->isOpen()) ctx->file->close();
                delete ctx->file;
                ctx->file = NULL;
                if (!partialPath.isEmpty() && QFile::exists(partialPath)) {
                    QFile::remove(partialPath);
                }
            }
            delete ctx;
        }
    }
    socket->deleteLater();
}

void WiFiDropServer::processClient(QTcpSocket *socket, ClientContext *ctx)
{
    if (ctx->state == ClientContext::STATE_HEADERS) {
        // Reject HTTPS/TLS ClientHello immediately instead of letting browser hang until ERR_TIMED_OUT
        if (ctx->buffer.size() >= 3 &&
            static_cast<unsigned char>(ctx->buffer.at(0)) == 0x16 &&
            static_cast<unsigned char>(ctx->buffer.at(1)) == 0x03) {
            qDebug() << "[WiFiDropServer] Rejected TLS/HTTPS handshake on plain HTTP port" << m_port;
            socket->abort();
            return;
        }
        int headerEnd = ctx->buffer.indexOf("\r\n\r\n");
        if (headerEnd == -1) {
            if (ctx->buffer.size() > 16384) {
                sendResponse(socket, 400, "Bad Request", "text/plain", "Header too large");
                socket->disconnectFromHost();
            }
            return;
        }

        QByteArray headerBytes = ctx->buffer.left(headerEnd);
        ctx->buffer.remove(0, headerEnd + 4);

        QString headerStr = QString::fromUtf8(headerBytes.constData(), headerBytes.size());
        QStringList lines = headerStr.split("\r\n");
        if (lines.isEmpty()) {
            sendResponse(socket, 400, "Bad Request", "text/plain", "Empty request");
            socket->disconnectFromHost();
            return;
        }

        QString requestLine = lines.at(0);
        QStringList reqParts = requestLine.split(" ");
        if (reqParts.size() < 2) {
            sendResponse(socket, 400, "Bad Request", "text/plain", "Malformed request line");
            socket->disconnectFromHost();
            return;
        }

        ctx->method = reqParts.at(0).toUpper();
        ctx->path = reqParts.at(1);

        for (int i = 1; i < lines.size(); ++i) {
            const QString &line = lines.at(i);
            int colon = line.indexOf(':');
            if (colon != -1) {
                QString name = line.left(colon).trimmed().toLower();
                QString val = line.mid(colon + 1).trimmed();
                if (name == "content-length") {
                    ctx->contentLength = val.toLongLong();
                } else if (name == "content-type") {
                    int bIdx = val.indexOf("boundary=", 0, Qt::CaseInsensitive);
                    if (bIdx != -1) {
                        ctx->boundary = val.mid(bIdx + 9).trimmed();
                        int semi = ctx->boundary.indexOf(';');
                        if (semi != -1) {
                            ctx->boundary = ctx->boundary.left(semi).trimmed();
                        }
                        if (ctx->boundary.startsWith("\"") && ctx->boundary.endsWith("\"") && ctx->boundary.length() >= 2) {
                            ctx->boundary = ctx->boundary.mid(1, ctx->boundary.length() - 2);
                        }
                    }
                }
            }
        }

        if (ctx->method == "GET") {
            if (ctx->path == "/" || ctx->path == "/index.html") {
                sendResponse(socket, 200, "OK", "text/html; charset=utf-8", getHtmlPage().toUtf8());
            } else if (ctx->path == "/api/status") {
                QString json = QString("{\"status\":\"running\",\"device\":\"BlackBerry 10\",\"ip\":\"%1\",\"port\":%2,\"uploaded\":%3}")
                    .arg(escapeWifiJson(m_ipAddress)).arg(m_port).arg(m_uploadedFiles.size());
                sendResponse(socket, 200, "OK", "application/json; charset=utf-8", json.toUtf8());
            } else if (ctx->path == "/api/books") {
                BookManager *bm = qobject_cast<BookManager*>(parent());
                QVariantList books = bm ? bm->allBooksSnapshot() : QVariantList();
                QString out = "[";
                for (int i = 0; i < books.size(); ++i) {
                    QVariantMap b = books.at(i).toMap();
                    QString title = escapeWifiJson(b.value("title").toString());
                    QString author = escapeWifiJson(b.value("author").toString());
                    QString fmt = escapeWifiJson(b.value("format").toString());
                    QString sz = escapeWifiJson(b.value("fileSize").toString());
                    QString fp = escapeWifiJson(b.value("filePath").toString());
                    int prog = b.value("progress").toInt();
                    out += QString("{\"title\":\"%1\",\"author\":\"%2\",\"format\":\"%3\",\"size\":\"%4\",\"path\":\"%5\",\"progress\":%6}")
                           .arg(title, author, fmt, sz, fp).arg(prog);
                    if (i + 1 < books.size()) out += ",";
                }
                out += "]";
                sendResponse(socket, 200, "OK", "application/json; charset=utf-8", out.toUtf8());
            } else if (ctx->path == "/api/backup") {
                BookManager *bm = qobject_cast<BookManager*>(parent());
                QString bPath = bm ? bm->createBackupArchive() : QString();
                QFile bf(bPath);
                if (!bPath.isEmpty() && bf.open(QIODevice::ReadOnly)) {
                    streamFileToSocket(socket, bf, QFileInfo(bPath).fileName());
                    bf.close();
                } else {
                    sendResponse(socket, 500, "Internal Server Error", "text/plain", "Backup creation failed");
                }
            } else if (ctx->path.startsWith("/download?")) {
                QUrl reqUrl = QUrl::fromEncoded(("http://localhost" + ctx->path).toUtf8());
                QString reqPath = reqUrl.queryItemValue("path");
                QString canon = QFileInfo(reqPath).canonicalFilePath();
                bool allowed = !canon.isEmpty() && (
                    canon.startsWith("/accounts/1000/shared/books/") ||
                    canon.startsWith("/accounts/1000/shared/documents/") ||
                    canon.startsWith("/accounts/1000/removable/sdcard")
                );
                QFile df(canon);
                if (allowed && df.exists() && df.open(QIODevice::ReadOnly)) {
                    streamFileToSocket(socket, df, QFileInfo(canon).fileName());
                    df.close();
                } else {
                    sendResponse(socket, 404, "Not Found", "text/plain", "File not found or restricted");
                }
            } else {
                sendResponse(socket, 404, "Not Found", "text/plain", "Not Found");
            }
            socket->disconnectFromHost();
            ctx->state = ClientContext::STATE_DONE;
            return;
        } else if (ctx->method == "OPTIONS") {
            sendResponse(socket, 200, "OK", "text/plain", "");
            socket->disconnectFromHost();
            ctx->state = ClientContext::STATE_DONE;
            return;
        } else if (ctx->method == "POST" && ctx->path == "/upload") {
            if (ctx->boundary.isEmpty()) {
                sendResponse(socket, 400, "Bad Request", "text/plain", "Missing multipart boundary");
                socket->disconnectFromHost();
                ctx->state = ClientContext::STATE_DONE;
                return;
            }
            ctx->endDelimiter = QByteArray("\r\n--") + ctx->boundary.toAscii();
            ctx->state = ClientContext::STATE_PART_HEADER;
        } else {
            sendResponse(socket, 405, "Method Not Allowed", "text/plain", "Method Not Allowed");
            socket->disconnectFromHost();
            ctx->state = ClientContext::STATE_DONE;
            return;
        }
    }

    if (ctx->state == ClientContext::STATE_PART_HEADER) {
        int partHeaderEnd = ctx->buffer.indexOf("\r\n\r\n");
        if (partHeaderEnd == -1) {
            return;
        }

        QByteArray partHeaders = ctx->buffer.left(partHeaderEnd);
        ctx->buffer.remove(0, partHeaderEnd + 4);

        QString partHeadersStr = QString::fromUtf8(partHeaders.constData(), partHeaders.size());

        QString fileName;
        QRegExp fnStarRx("filename\\*=UTF-8''([^;\\r\\n]+)", Qt::CaseInsensitive);
        if (fnStarRx.indexIn(partHeadersStr) != -1) {
            fileName = QUrl::fromPercentEncoding(fnStarRx.cap(1).trimmed().toUtf8());
        } else {
            QRegExp fnRx("filename=\"([^\"]+)\"");
            if (fnRx.indexIn(partHeadersStr) != -1) {
                fileName = fnRx.cap(1);
            } else {
                QRegExp fnRx2("filename=([^;\\r\\n]+)");
                if (fnRx2.indexIn(partHeadersStr) != -1) {
                    fileName = fnRx2.cap(1).trimmed();
                }
            }
        }

        if (fileName.contains("%")) {
            fileName = QUrl::fromPercentEncoding(fileName.toUtf8());
        }
        fileName = fileName.trimmed();
        fileName.remove('\r').remove('\n');
        fileName = QFileInfo(fileName).fileName();
        fileName.replace(QRegExp("[\\\\/:*?\"<>|]"), "_");
        if (fileName.isEmpty() || fileName == "." || fileName == "..") {
            fileName = QString("book_%1.bin").arg(QDateTime::currentMSecsSinceEpoch());
        }

        QString upExt = QFileInfo(fileName).suffix().toLower();
        bool isBookExt = (upExt == "epub" || upExt == "pdf" || upExt == "mobi" ||
                          upExt == "azw" || upExt == "azw3" || upExt == "kf8" ||
                          upExt == "prc" || upExt == "fb2" || upExt == "cbz" ||
                          upExt == "djvu" || upExt == "djv" || upExt == "txt");
        if (!isBookExt) {
            sendResponse(socket, 400, "Bad Request", "application/json; charset=utf-8",
                         "{\"success\":false,\"error\":\"Unsupported eBook file format\"}");
            socket->disconnectFromHost();
            ctx->state = ClientContext::STATE_DONE;
            return;
        }

        ctx->fileName = fileName;

        QString targetDir = targetUploadDir();
        QString finalPath = targetDir + "/" + fileName;

        if (QFile::exists(finalPath)) {
            QFileInfo fi(finalPath);
            QString base = fi.completeBaseName();
            QString ext = fi.suffix();
            int counter = 1;
            while (QFile::exists(finalPath)) {
                if (ext.isEmpty()) {
                    finalPath = QString("%1/%2 (%3)").arg(targetDir).arg(base).arg(counter++);
                } else {
                    finalPath = QString("%1/%2 (%3).%4").arg(targetDir).arg(base).arg(counter++).arg(ext);
                }
            }
            ctx->fileName = QFileInfo(finalPath).fileName();
        }

        ctx->filePath = finalPath;
        ctx->file = new QFile(finalPath);
        if (!ctx->file->open(QIODevice::WriteOnly)) {
            delete ctx->file;
            ctx->file = NULL;
            sendResponse(socket, 500, "Internal Server Error", "application/json", "{\"success\":false,\"error\":\"Cannot open file for writing\"}");
            socket->disconnectFromHost();
            ctx->state = ClientContext::STATE_DONE;
            return;
        }

        ctx->state = ClientContext::STATE_STREAMING_BODY;
    }

    if (ctx->state == ClientContext::STATE_STREAMING_BODY) {
        int delimPos = ctx->buffer.indexOf(ctx->endDelimiter);
        if (delimPos != -1) {
            if (delimPos > 0 && ctx->file) {
                ctx->file->write(ctx->buffer.constData(), delimPos);
                ctx->bytesWritten += delimPos;
            }
            if (ctx->file) {
                ctx->file->flush();
                ctx->file->close();
            }

            ctx->state = ClientContext::STATE_DONE;

            qint64 totalSize = ctx->bytesWritten;
            QString uploadedPath = ctx->filePath;
            QString uploadedName = ctx->fileName;

            QVariantMap item;
            item["name"] = uploadedName;
            item["path"] = uploadedPath;
            item["size"] = formatSize(totalSize);
            item["bytes"] = totalSize;
            item["time"] = QDateTime::currentDateTime().toString("hh:mm:ss");
            item["format"] = QFileInfo(uploadedName).suffix().toUpper();

            m_uploadedFiles.prepend(item);
            while (m_uploadedFiles.size() > 100) {
                m_uploadedFiles.removeLast();
            }
            emit uploadedFilesChanged();
            emit sessionUploadCountChanged();

            QString upExt = QFileInfo(uploadedName).suffix().toLower();
            bool isBookExt = (upExt == "epub" || upExt == "pdf" || upExt == "mobi" ||
                              upExt == "azw" || upExt == "azw3" || upExt == "kf8" ||
                              upExt == "prc" || upExt == "fb2" || upExt == "cbz" ||
                              upExt == "djvu" || upExt == "djv" || upExt == "txt");
            if (isBookExt) {
                emit bookUploaded(uploadedPath, uploadedName, totalSize);
            }

            QString escName = escapeWifiJson(uploadedName);
            QString escPath = escapeWifiJson(uploadedPath);
            QString respJson = QString("{\"success\":true,\"fileName\":\"%1\",\"size\":%2,\"path\":\"%3\"}")
                .arg(escName, QString::number(totalSize), escPath);
            sendResponse(socket, 200, "OK", "application/json; charset=utf-8", respJson.toUtf8());
            socket->flush();
            socket->disconnectFromHost();
            return;
        } else {
            int keepLen = ctx->endDelimiter.size() + 8;
            if (ctx->buffer.size() > keepLen) {
                int writeLen = ctx->buffer.size() - keepLen;
                if (ctx->file) {
                    ctx->file->write(ctx->buffer.constData(), writeLen);
                    ctx->bytesWritten += writeLen;
                }
                ctx->buffer.remove(0, writeLen);
            }
        }
    }
}

void WiFiDropServer::sendResponse(QTcpSocket *socket, int statusCode, const QString &statusText, const QString &contentType, const QByteArray &body)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

    QByteArray resp;
    resp.append(QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toAscii());
    resp.append("Server: Libris-WiFiDrop/1.0\r\n");
    resp.append(QString("Content-Type: %1\r\n").arg(contentType).toAscii());
    resp.append(QString("Content-Length: %1\r\n").arg(body.size()).toAscii());
    resp.append("Access-Control-Allow-Origin: *\r\n");
    resp.append("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n");
    resp.append("Access-Control-Allow-Headers: Content-Type, Content-Length\r\n");
    resp.append("Connection: close\r\n");
    resp.append("\r\n");
    if (!body.isEmpty()) {
        resp.append(body);
    }
    socket->write(resp);
    socket->flush();
}

QString WiFiDropServer::targetUploadDir() const
{
    QString booksDir = "/accounts/1000/shared/books";
    if (QDir().mkpath(booksDir)) {
        return booksDir;
    }
    QString docsDir = "/accounts/1000/shared/documents";
    if (QDir().mkpath(docsDir)) {
        return docsDir;
    }
    QString homeBooks = QDir::homePath() + "/books";
    QDir().mkpath(homeBooks);
    return homeBooks;
}

QString WiFiDropServer::detectIpAddress() const
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    QString fallbackIp;

    for (int i = 0; i < interfaces.size(); ++i) {
        const QNetworkInterface &iface = interfaces.at(i);
        QNetworkInterface::InterfaceFlags flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) ||
            !(flags & QNetworkInterface::IsRunning) ||
            (flags & QNetworkInterface::IsLoopBack) ||
            (flags & QNetworkInterface::IsPointToPoint)) {
            continue;
        }

        bool isWifi = iface.name().contains("bcm", Qt::CaseInsensitive) ||
                      iface.name().contains("tiw", Qt::CaseInsensitive) ||
                      iface.name().contains("wlan", Qt::CaseInsensitive) ||
                      iface.name().contains("wifi", Qt::CaseInsensitive) ||
                      iface.name().startsWith("en", Qt::CaseInsensitive);

        QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (int j = 0; j < entries.size(); ++j) {
            QHostAddress addr = entries.at(j).ip();
            QString ipStr = addr.toString();
            if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
                addr != QHostAddress::LocalHost &&
                !ipStr.startsWith("127.") &&
                !ipStr.startsWith("169.254.")) {
                if (isWifi) {
                    return addr.toString();
                } else if (fallbackIp.isEmpty()) {
                    fallbackIp = addr.toString();
                }
            }
        }
    }

    if (!fallbackIp.isEmpty()) {
        return fallbackIp;
    }

    QList<QHostAddress> addrs = QNetworkInterface::allAddresses();
    for (int i = 0; i < addrs.size(); ++i) {
        const QHostAddress &addr = addrs.at(i);
        if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
            addr != QHostAddress::LocalHost &&
            !addr.toString().startsWith("127.")) {
            return addr.toString();
        }
    }

    return QString();
}

QString WiFiDropServer::formatSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    }
}

QString WiFiDropServer::getHtmlPage() const
{
    static const char page[] =
"<!DOCTYPE html>\n"
"<html lang=\"__L_LANG__\">\n"
"<head>\n"
"<meta charset=\"UTF-8\">\n"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"<title>Libris Wireless Drop</title>\n"
"<style>\n"
":root {\n"
"  --bg: #0A0908;\n"
"  --card-bg: #141210;\n"
"  --card-border: rgba(212,163,89,0.18);\n"
"  --primary: #D4A359;\n"
"  --primary-hover: #E6C07B;\n"
"  --primary-glow: rgba(212,163,89,0.22);\n"
"  --text: #F5EFE6;\n"
"  --text-muted: #9C9488;\n"
"  --success: #2ea043;\n"
"  --danger: #f85149;\n"
"}\n"
"@media (prefers-color-scheme: light) {\n"
"  :root {\n"
"    --bg: #FAF7F2;\n"
"    --card-bg: #FFFFFF;\n"
"    --card-border: rgba(122,82,20,0.16);\n"
"    --primary: #7A5214;\n"
"    --primary-hover: #5E3F0F;\n"
"    --text: #1A1714;\n"
"    --text-muted: #5C554C;\n"
"    --primary-glow: rgba(122,82,20,0.14);\n"
"  }\n"
"}\n"
"* { box-sizing: border-box; margin: 0; padding: 0; }\n"
"body {\n"
"  font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, Helvetica, Arial, sans-serif;\n"
"  background-color: var(--bg);\n"
"  color: var(--text);\n"
"  min-height: 100vh;\n"
"  display: flex;\n"
"  flex-direction: column;\n"
"  align-items: center;\n"
"  padding: 32px 16px;\n"
"}\n"
".container {\n"
"  width: 100%;\n"
"  max-width: 620px;\n"
"}\n"
".header {\n"
"  text-align: center;\n"
"  margin-bottom: 28px;\n"
"}\n"
".logo-icon {\n"
"  display: inline-flex;\n"
"  align-items: center;\n"
"  justify-content: center;\n"
"  width: 56px;\n"
"  height: 56px;\n"
"  background: linear-gradient(135deg, #E6C07B, #A87932);\n"
"  border-radius: 16px;\n"
"  box-shadow: 0 8px 24px var(--primary-glow);\n"
"  margin-bottom: 12px;\n"
"}\n"
".logo-icon svg {\n"
"  width: 32px;\n"
"  height: 32px;\n"
"  fill: #0D0B09;\n"
"}\n"
"h1 {\n"
"  font-size: 26px;\n"
"  font-weight: 700;\n"
"  letter-spacing: -0.5px;\n"
"  margin-bottom: 6px;\n"
"}\n"
".device-badge {\n"
"  display: inline-flex;\n"
"  align-items: center;\n"
"  gap: 6px;\n"
"  font-size: 13px;\n"
"  color: var(--primary);\n"
"  background: var(--primary-glow);\n"
"  padding: 4px 12px;\n"
"  border-radius: 20px;\n"
"  font-weight: 600;\n"
"}\n"
".device-badge::before {\n"
"  content: \"\";\n"
"  width: 8px;\n"
"  height: 8px;\n"
"  background-color: var(--primary);\n"
"  border-radius: 50%;\n"
"  box-shadow: 0 0 8px var(--primary);\n"
"}\n"
".dropzone {\n"
"  background-color: var(--card-bg);\n"
"  border: 2px dashed var(--primary);\n"
"  border-radius: 20px;\n"
"  padding: 44px 24px;\n"
"  text-align: center;\n"
"  cursor: pointer;\n"
"  transition: all 0.2s ease;\n"
"  box-shadow: 0 8px 30px rgba(0,0,0,0.12);\n"
"  position: relative;\n"
"}\n"
".dropzone.dragover {\n"
"  background-color: var(--primary-glow);\n"
"  transform: scale(1.015);\n"
"  border-color: var(--primary-hover);\n"
"}\n"
".drop-icon {\n"
"  width: 48px;\n"
"  height: 48px;\n"
"  margin-bottom: 14px;\n"
"  fill: var(--primary);\n"
"}\n"
".drop-title {\n"
"  font-size: 18px;\n"
"  font-weight: 600;\n"
"  margin-bottom: 6px;\n"
"}\n"
".drop-sub {\n"
"  font-size: 14px;\n"
"  color: var(--text-muted);\n"
"  margin-bottom: 16px;\n"
"}\n"
".btn-select {\n"
"  background-color: #D4A359;\n"
"  color: #0D0B09;\n"
"  border: none;\n"
"  padding: 10px 22px;\n"
"  border-radius: 12px;\n"
"  font-size: 14px;\n"
"  font-weight: 600;\n"
"  cursor: pointer;\n"
"  transition: background 0.15s ease;\n"
"}\n"
".btn-select:hover {\n"
"  background-color: #E6C07B;\n"
"}\n"
".format-pills {\n"
"  margin-top: 20px;\n"
"  display: flex;\n"
"  flex-wrap: wrap;\n"
"  justify-content: center;\n"
"  gap: 6px;\n"
"}\n"
".pill {\n"
"  font-size: 11px;\n"
"  font-weight: 600;\n"
"  padding: 3px 8px;\n"
"  border-radius: 6px;\n"
"  background: rgba(255,255,255,0.06);\n"
"  color: var(--text-muted);\n"
"  border: 1px solid var(--card-border);\n"
"}\n"
".uploads-card {\n"
"  margin-top: 28px;\n"
"  background-color: var(--card-bg);\n"
"  border: 1px solid var(--card-border);\n"
"  border-radius: 16px;\n"
"  padding: 20px;\n"
"  box-shadow: 0 4px 20px rgba(0,0,0,0.1);\n"
"}\n"
".uploads-header {\n"
"  display: flex;\n"
"  justify-content: space-between;\n"
"  align-items: center;\n"
"  margin-bottom: 16px;\n"
"  font-size: 15px;\n"
"  font-weight: 600;\n"
"}\n"
".file-list {\n"
"  list-style: none;\n"
"  display: flex;\n"
"  flex-direction: column;\n"
"  gap: 12px;\n"
"}\n"
".file-item {\n"
"  display: flex;\n"
"  flex-direction: column;\n"
"  background: rgba(255,255,255,0.03);\n"
"  border: 1px solid var(--card-border);\n"
"  border-radius: 12px;\n"
"  padding: 12px 14px;\n"
"  gap: 8px;\n"
"}\n"
".file-info {\n"
"  display: flex;\n"
"  align-items: center;\n"
"  justify-content: space-between;\n"
"}\n"
".file-name {\n"
"  font-size: 14px;\n"
"  font-weight: 500;\n"
"  max-width: 380px;\n"
"  white-space: nowrap;\n"
"  overflow: hidden;\n"
"  text-overflow: ellipsis;\n"
"}\n"
".file-size {\n"
"  font-size: 12px;\n"
"  color: var(--text-muted);\n"
"}\n"
".progress-track {\n"
"  height: 6px;\n"
"  background: rgba(255,255,255,0.08);\n"
"  border-radius: 3px;\n"
"  overflow: hidden;\n"
"}\n"
".progress-bar {\n"
"  height: 100%;\n"
"  width: 0%;\n"
"  background-color: var(--primary);\n"
"  border-radius: 3px;\n"
"  transition: width 0.15s ease;\n"
"}\n"
".status-tag {\n"
"  font-size: 12px;\n"
"  font-weight: 600;\n"
"  display: flex;\n"
"  align-items: center;\n"
"  gap: 4px;\n"
"}\n"
".status-done {\n"
"  color: var(--success);\n"
"}\n"
".status-uploading {\n"
"  color: var(--primary);\n"
"}\n"
".status-error {\n"
"  color: var(--danger);\n"
"}\n"
".footer {\n"
"  margin-top: 36px;\n"
"  font-size: 12px;\n"
"  color: var(--text-muted);\n"
"  text-align: center;\n"
"  line-height: 1.6;\n"
"}\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class=\"container\">\n"
"  <div class=\"header\">\n"
"    <div class=\"logo-icon\">\n"
"      <svg viewBox=\"0 0 24 24\"><path d=\"M17 8C8 10 5.9 16.17 3.82 21.34l1.89.66.95-2.3c.48.17.98.3 1.34.3C19 20 22 3 22 3c-1 2-8 2.25-13 3.25S2 11.5 2 13.5s1.75 3.75 1.75 3.75C7 8 17 8 17 8z\"/></svg>\n"
"    </div>\n"
"    <h1>Libris Wireless Drop</h1>\n"
"    <div class=\"device-badge\">BlackBerry 10 &bull; __L_READY__</div>\n"
"  </div>\n"
"\n"
"  <div class=\"dropzone\" id=\"dropzone\">\n"
"    <svg class=\"drop-icon\" viewBox=\"0 0 24 24\"><path d=\"M19.35 10.04C18.67 6.59 15.64 4 12 4 9.11 4 6.6 5.64 5.35 8.04 2.34 8.36 0 10.91 0 14c0 3.31 2.69 6 6 6h13c2.76 0 5-2.24 5-5 0-2.64-2.05-4.78-4.65-4.96zM14 13v4h-4v-4H7l5-5 5 5h-3z\"/></svg>\n"
"    <div class=\"drop-title\">__L_DROP_TITLE__</div>\n"
"    <div class=\"drop-sub\">__L_DROP_SUB__</div>\n"
"    <button type=\"button\" class=\"btn-select\" id=\"btnSelect\">__L_BTN_SELECT__</button>\n"
"    <input type=\"file\" id=\"fileInput\" multiple accept=\".epub,.pdf,.mobi,.azw,.azw3,.kf8,.prc,.fb2,.txt,.cbz,.djvu,.djv\" style=\"display:none;\">\n"
"    <div class=\"format-pills\">\n"
"      <span class=\"pill\">EPUB</span>\n"
"      <span class=\"pill\">PDF</span>\n"
"      <span class=\"pill\">MOBI</span>\n"
"      <span class=\"pill\">AZW3</span>\n"
"      <span class=\"pill\">KF8</span>\n"
"      <span class=\"pill\">PRC</span>\n"
"      <span class=\"pill\">FB2</span>\n"
"      <span class=\"pill\">TXT</span>\n"
"      <span class=\"pill\">CBZ</span>\n"
"      <span class=\"pill\">DJVU</span>\n"
"    </div>\n"
"  </div>\n"
"\n"
"  <div class=\"uploads-card\" id=\"uploadsCard\" style=\"display:none;\">\n"
"    <div class=\"uploads-header\">\n"
"      <span>__L_QUEUE_TITLE__</span>\n"
"      <span id=\"uploadCountBadge\" style=\"color:var(--primary); font-size:13px;\">0 __L_FILE_UNIT__</span>\n"
"    </div>\n"
"    <ul class=\"file-list\" id=\"fileList\"></ul>\n"
"  </div>\n"
"\n"
"  <div class=\"uploads-card\" style=\"margin-top:20px;\">\n"
"    <div class=\"uploads-header\">\n"
"      <span>__L_LIB_HEADER__</span>\n"
"      <div style=\"display:flex; gap:8px;\">\n"
"        <a href=\"/api/backup\" style=\"background:var(--primary);color:#0D0B09;padding:6px 12px;border-radius:8px;text-decoration:none;font-size:12px;font-weight:700;\">__L_BTN_BACKUP__</a>\n"
"        <button type=\"button\" onclick=\"loadBooks()\" style=\"background:rgba(212,163,89,0.16);color:var(--primary);border:1px solid rgba(212,163,89,0.4);padding:6px 12px;border-radius:8px;cursor:pointer;font-size:12px;font-weight:600;\">__L_BTN_LOAD__</button>\n"
"      </div>\n"
"    </div>\n"
"    <ul class=\"file-list\" id=\"deviceBookList\"><li class=\"file-item\" style=\"color:var(--text-muted);font-size:13px;\">__L_LIB_HINT__</li></ul>\n"
"  </div>\n"
"\n"
"  <div class=\"footer\">\n"
"    __L_FOOTER_1__ <code>/accounts/1000/shared/books</code><br>\n"
"    __L_FOOTER_2__\n"
"  </div>\n"
"</div>\n"
"\n"
"<script>\n"
"const I18N = {\n"
"  unit: '__L_FILE_UNIT__',\n"
"  queued: '__L_QUEUED__',\n"
"  uploading: '__L_UPLOADING__',\n"
"  saved: '__L_SAVED__',\n"
"  errUpload: '__L_ERR_UPLOAD__',\n"
"  errConn: '__L_ERR_CONN__',\n"
"  loadingBooks: '__L_LOADING_BOOKS__',\n"
"  emptyBooks: '__L_EMPTY_BOOKS__',\n"
"  download: '__L_DOWNLOAD__',\n"
"  errLoadBooks: '__L_ERR_LOAD_BOOKS__'\n"
"};\n"
"const dropzone = document.getElementById('dropzone');\n"
"const fileInput = document.getElementById('fileInput');\n"
"const btnSelect = document.getElementById('btnSelect');\n"
"const uploadsCard = document.getElementById('uploadsCard');\n"
"const fileList = document.getElementById('fileList');\n"
"const uploadCountBadge = document.getElementById('uploadCountBadge');\n"
"\n"
"let queue = [];\n"
"let isUploading = false;\n"
"let totalCompleted = 0;\n"
"\n"
"btnSelect.addEventListener('click', (e) => {\n"
"  e.stopPropagation();\n"
"  fileInput.click();\n"
"});\n"
"dropzone.addEventListener('click', () => fileInput.click());\n"
"\n"
"['dragenter', 'dragover'].forEach(name => {\n"
"  dropzone.addEventListener(name, (e) => {\n"
"    e.preventDefault();\n"
"    dropzone.classList.add('dragover');\n"
"  });\n"
"});\n"
"\n"
"['dragleave', 'drop'].forEach(name => {\n"
"  dropzone.addEventListener(name, (e) => {\n"
"    e.preventDefault();\n"
"    dropzone.classList.remove('dragover');\n"
"  });\n"
"});\n"
"\n"
"dropzone.addEventListener('drop', (e) => {\n"
"  if (e.dataTransfer && e.dataTransfer.files) {\n"
"    handleFiles(e.dataTransfer.files);\n"
"  }\n"
"});\n"
"\n"
"fileInput.addEventListener('change', (e) => {\n"
"  if (e.target.files) {\n"
"    handleFiles(e.target.files);\n"
"    fileInput.value = '';\n"
"  }\n"
"});\n"
"\n"
"function formatBytes(bytes) {\n"
"  if (bytes < 1024) return bytes + ' B';\n"
"  if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';\n"
"  return (bytes / (1024 * 1024)).toFixed(1) + ' MB';\n"
"}\n"
"\n"
"function handleFiles(files) {\n"
"  if (!files || files.length === 0) return;\n"
"  uploadsCard.style.display = 'block';\n"
"\n"
"  for (let i = 0; i < files.length; i++) {\n"
"    const file = files[i];\n"
"    const item = {\n"
"      id: 'f_' + Date.now() + '_' + Math.random().toString(36).substr(2, 6),\n"
"      file: file,\n"
"      name: file.name,\n"
"      size: file.size,\n"
"      status: 'pending',\n"
"      progress: 0\n"
"    };\n"
"    queue.push(item);\n"
"    renderItem(item);\n"
"  }\n"
"  updateCountBadge();\n"
"  processQueue();\n"
"}\n"
"\n"
"function renderItem(item) {\n"
"  const li = document.createElement('li');\n"
"  li.id = item.id;\n"
"  li.className = 'file-item';\n"
"  li.innerHTML = `\n"
"    <div class=\"file-info\">\n"
"      <span class=\"file-name\" title=\"${escapeHtml(item.name)}\">${escapeHtml(item.name)}</span>\n"
"      <span class=\"file-size\">${formatBytes(item.size)}</span>\n"
"    </div>\n"
"    <div class=\"progress-track\">\n"
"      <div class=\"progress-bar\" style=\"width: 0%\"></div>\n"
"    </div>\n"
"    <div class=\"status-tag status-uploading\">${I18N.queued}</div>\n"
"  `;\n"
"  fileList.prepend(li);\n"
"}\n"
"\n"
"function escapeHtml(str) {\n"
"  return str.replace(/[&<>\"']/g, m => ({'&':'&amp;','<':'&lt;','>':'&gt;','\"':'&quot;',\"'\":'&#39;'})[m]);\n"
"}\n"
"\n"
"function updateCountBadge() {\n"
"  uploadCountBadge.textContent = totalCompleted + ' / ' + (totalCompleted + queue.length) + ' ' + I18N.unit;\n"
"}\n"
"\n"
"function processQueue() {\n"
"  if (isUploading || queue.length === 0) return;\n"
"  isUploading = true;\n"
"  const item = queue.shift();\n"
"  uploadFile(item);\n"
"}\n"
"\n"
"function uploadFile(item) {\n"
"  const elem = document.getElementById(item.id);\n"
"  const bar = elem ? elem.querySelector('.progress-bar') : null;\n"
"  const statusTag = elem ? elem.querySelector('.status-tag') : null;\n"
"\n"
"  if (statusTag) {\n"
"    statusTag.className = 'status-tag status-uploading';\n"
"    statusTag.textContent = I18N.uploading + '0%';\n"
"  }\n"
"\n"
"  const formData = new FormData();\n"
"  formData.append('file', item.file, item.name);\n"
"\n"
"  const xhr = new XMLHttpRequest();\n"
"  xhr.open('POST', '/upload', true);\n"
"\n"
"  xhr.upload.onprogress = (e) => {\n"
"    if (e.lengthComputable) {\n"
"      const pct = Math.round((e.loaded / e.total) * 100);\n"
"      if (bar) bar.style.width = pct + '%';\n"
"      if (statusTag) statusTag.textContent = I18N.uploading + pct + '%';\n"
"    }\n"
"  };\n"
"\n"
"  xhr.onload = () => {\n"
"    if (xhr.status === 200) {\n"
"      if (bar) bar.style.width = '100%';\n"
"      if (statusTag) {\n"
"        statusTag.className = 'status-tag status-done';\n"
"        statusTag.innerHTML = '&#10003; ' + I18N.saved;\n"
"      }\n"
"      totalCompleted++;\n"
"    } else {\n"
"      if (statusTag) {\n"
"        statusTag.className = 'status-tag status-error';\n"
"        statusTag.innerHTML = '&#10007; ' + I18N.errUpload;\n"
"      }\n"
"    }\n"
"    updateCountBadge();\n"
"    isUploading = false;\n"
"    processQueue();\n"
"  };\n"
"\n"
"  xhr.onerror = () => {\n"
"    if (statusTag) {\n"
"      statusTag.className = 'status-tag status-error';\n"
"      statusTag.innerHTML = '&#10007; ' + I18N.errConn;\n"
"    }\n"
"    updateCountBadge();\n"
"    isUploading = false;\n"
"    processQueue();\n"
"  };\n"
"\n"
"  xhr.send(formData);\n"
"}\n"
"\n"
"function loadBooks() {\n"
"  const ul = document.getElementById('deviceBookList');\n"
"  if (!ul) return;\n"
"  ul.innerHTML = '<li class=\"file-item\" style=\"color:var(--text-muted);font-size:13px;\">' + I18N.loadingBooks + '</li>';\n"
"  fetch('/api/books').then(r => r.json()).then(list => {\n"
"    if (!list || list.length === 0) {\n"
"      ul.innerHTML = '<li class=\"file-item\" style=\"color:var(--text-muted);font-size:13px;\">' + I18N.emptyBooks + '</li>';\n"
"      return;\n"
"    }\n"
"    ul.innerHTML = '';\n"
"    list.forEach(b => {\n"
"      const li = document.createElement('li');\n"
"      li.className = 'file-item';\n"
"      li.innerHTML = '<div class=\"file-info\"><div class=\"file-name\">' + escapeHtml(b.title || 'Untitled') + '</div>' +\n"
"        '<div class=\"file-size\">' + escapeHtml((b.format || '').toUpperCase() + (b.size ? ' • ' + b.size : '') + (b.author ? ' • ' + b.author : '')) + '</div></div>' +\n"
"        '<a href=\"/download?path=' + encodeURIComponent(b.path || '') + '\" style=\"color:var(--primary);font-size:12px;font-weight:700;text-decoration:none;\">' + I18N.download + '</a>';\n"
"      ul.appendChild(li);\n"
"    });\n"
"  }).catch(() => {\n"
"    ul.innerHTML = '<li class=\"file-item\" style=\"color:var(--danger);font-size:13px;\">' + I18N.errLoadBooks + '</li>';\n"
"  });\n"
"}\n"
"</script>\n"
"</body>\n"
"</html>";

    BookManager *bm = qobject_cast<BookManager*>(parent());
    bool isEn = (bm && bm->language() == "en");

    struct HtmlI18nToken { const char *key; const char *vi; const char *en; };
    static const HtmlI18nToken tokens[] = {
        { "__L_LANG__", "vi", "en" },
        { "__L_READY__", "Sẵn sàng nhận sách", "Ready to receive books" },
        { "__L_DROP_TITLE__", "Kéo & thả tệp sách vào đây", "Drag &amp; drop eBook files here" },
        { "__L_DROP_SUB__", "hoặc bấm để duyệt tệp từ máy tính / điện thoại", "or click to browse files from your computer / phone" },
        { "__L_BTN_SELECT__", "Chọn Tệp Sách", "Select Book Files" },
        { "__L_QUEUE_TITLE__", "Danh sách truyền tệp", "Transfer Queue" },
        { "__L_FILE_UNIT__", "tệp", "files" },
        { "__L_LIB_HEADER__", "Thư viện trên máy &amp; Sao lưu", "Device Library &amp; Backup" },
        { "__L_BTN_BACKUP__", "Tải .librisbackup", "Download .librisbackup" },
        { "__L_BTN_LOAD__", "Tải danh sách", "Load Books" },
        { "__L_LIB_HINT__", "Bấm 'Tải danh sách' để xem và tải sách từ BlackBerry về máy tính.", "Click 'Load Books' to view and download books from BlackBerry to your PC." },
        { "__L_FOOTER_1__", "Tệp tải lên sẽ được lưu tự động vào thư mục", "Uploaded files are automatically saved to" },
        { "__L_FOOTER_2__", "và tức thì hiển thị trong thư viện Libris Reader trên BlackBerry 10.", "and immediately indexed in your Libris Reader library on BlackBerry 10." },
        { "__L_QUEUED__", "Chờ truyền...", "Queued..." },
        { "__L_UPLOADING__", "Đang truyền: ", "Uploading: " },
        { "__L_SAVED__", "Đã lưu vào Libris", "Saved to Libris" },
        { "__L_ERR_UPLOAD__", "Lỗi truyền tệp", "Upload failed" },
        { "__L_ERR_CONN__", "Lỗi kết nối", "Connection error" },
        { "__L_LOADING_BOOKS__", "Đang tải danh sách...", "Loading books..." },
        { "__L_EMPTY_BOOKS__", "Chưa có sách nào trong thư viện.", "No books found in library." },
        { "__L_DOWNLOAD__", "Tải về", "Download" },
        { "__L_ERR_LOAD_BOOKS__", "Không thể tải danh sách sách.", "Failed to load book list." }
    };

    QString html = QString::fromUtf8(page);
    const int count = sizeof(tokens) / sizeof(tokens[0]);
    for (int i = 0; i < count; ++i) {
        html.replace(QLatin1String(tokens[i].key),
                     QString::fromUtf8(isEn ? tokens[i].en : tokens[i].vi));
    }
    return html;
}
