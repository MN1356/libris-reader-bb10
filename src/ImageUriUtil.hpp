#ifndef IMAGEURIUTIL_HPP
#define IMAGEURIUTIL_HPP

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QString>

inline QString detectImageMime(const QByteArray &data, const QString &fileNameHint = QString())
{
    if (data.startsWith("\x89PNG")) return "image/png";
    if (data.startsWith("GIF8")) return "image/gif";
    if (data.size() >= 12 && data.mid(0, 4) == "RIFF" && data.mid(8, 4) == "WEBP") return "image/webp";
    if (data.startsWith("\xFF\xD8\xFF")) return "image/jpeg";

    QString head = QString::fromLatin1(data.constData(), qMin(data.size(), 256)).trimmed().toLower();
    if (head.startsWith("<svg") || head.contains("<svg")) return "image/svg+xml";

    QString ext = QFileInfo(fileNameHint).suffix().toLower();
    if (ext == "png") return "image/png";
    if (ext == "gif") return "image/gif";
    if (ext == "webp") return "image/webp";
    if (ext == "svg" || ext == "svgz") return "image/svg+xml";
    if (ext == "bmp") return "image/bmp";
    return "image/jpeg";
}

inline QString bytesToDataUri(const QByteArray &data, const QString &fileNameHint = QString())
{
    if (data.isEmpty()) return QString();
    return QString("data:%1;base64,%2")
           .arg(detectImageMime(data, fileNameHint))
           .arg(QString::fromLatin1(data.toBase64().constData()));
}

inline QString fileToDataUri(const QString &filePath)
{
    if (filePath.isEmpty()) return QString();
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    if (f.size() <= 0 || f.size() > 8 * 1024 * 1024) {
        f.close();
        return QString();
    }
    QByteArray bytes = f.readAll();
    f.close();
    return bytesToDataUri(bytes, filePath);
}

inline QString findCachedCoverFile(const QString &cacheDir, const QString &hash)
{
    if (cacheDir.isEmpty() || hash.isEmpty()) return QString();
    const char *exts[] = { "jpg", "png", "gif", "jpeg", "webp" };
    for (int i = 0; i < 5; ++i) {
        QString p = cacheDir + "/" + hash + "." + QLatin1String(exts[i]);
        if (QFile::exists(p) && QFileInfo(p).size() > 0) {
            return p;
        }
    }
    return QString();
}

#endif // IMAGEURIUTIL_HPP
