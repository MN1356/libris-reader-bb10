#include <bb/cascades/Application>
#include <QtGlobal>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QTextCodec>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaType>
#include <QSslSocket>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "ApplicationUI.hpp"
#include "BookWorkers.hpp"

using namespace bb::cascades;

static QFile *s_sharedLogFile = NULL;
static QFile *s_sandboxLogFile = NULL;
static QMutex s_logMutex;

#if QT_VERSION < 0x050000
void librisLogHandler(QtMsgType type, const char *msg)
#else
void librisLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
#endif
{
    // Filter repetitive harmless driver warnings (e.g. BatteryInfo WlcSrcParameters on Passport)
    // to prevent waking up physical eMMC flash storage and wasting battery
    if (msg) {
#if QT_VERSION < 0x050000
        const char *rawStr = msg;
#else
        QByteArray utf8Check = msg.toUtf8();
        const char *rawStr = utf8Check.constData();
#endif
        if (strstr(rawStr, "WlcSrcParameters") != NULL ||
            (strstr(rawStr, "BatteryInfo") != NULL && strstr(rawStr, "Could not find list") != NULL)) {
            return;
        }
    }

    QMutexLocker locker(&s_logMutex);

    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString typeStr = "[DEBUG]";
    switch (type) {
    case QtDebugMsg:    typeStr = "[DEBUG]"; break;
    case QtWarningMsg:  typeStr = "[WARN ]"; break;
    case QtCriticalMsg: typeStr = "[CRIT ]"; break;
    case QtFatalMsg:    typeStr = "[FATAL]"; break;
    }

    QString formattedMsg = QString("%1 %2 %3\n").arg(timeStr).arg(typeStr).arg(msg);
    QByteArray utf8 = formattedMsg.toUtf8();

    // 1. Output to stderr / console (captured by slog2)
    fprintf(stderr, "%s", utf8.constData());

    // 2. Output to shared documents folder (accessible via BB10 File Manager)
    if (s_sharedLogFile && s_sharedLogFile->isOpen()) {
        s_sharedLogFile->write(utf8);
        // Only flush on critical/fatal errors to allow eMMC low-power sleep
        if (type == QtCriticalMsg || type == QtFatalMsg) {
            s_sharedLogFile->flush();
        }
    }

    // 3. Output to internal sandbox data folder
    if (s_sandboxLogFile && s_sandboxLogFile->isOpen()) {
        s_sandboxLogFile->write(utf8);
        if (type == QtCriticalMsg || type == QtFatalMsg) {
            s_sandboxLogFile->flush();
        }
    }
}

static void closeLogger()
{
    QMutexLocker locker(&s_logMutex);
    if (s_sharedLogFile) {
        if (s_sharedLogFile->isOpen()) {
            s_sharedLogFile->flush();
            s_sharedLogFile->close();
        }
        delete s_sharedLogFile;
        s_sharedLogFile = NULL;
    }
    if (s_sandboxLogFile) {
        if (s_sandboxLogFile->isOpen()) {
            s_sandboxLogFile->flush();
            s_sandboxLogFile->close();
        }
        delete s_sandboxLogFile;
        s_sandboxLogFile = NULL;
    }
}

static void initLogger()
{
    // Try opening shared documents log file
    QString sharedPath = "/accounts/1000/shared/documents/libris.log";
    s_sharedLogFile = new QFile(sharedPath);
    if (s_sharedLogFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        // Prevent log file from expanding indefinitely (truncate if > 2MB)
        if (s_sharedLogFile->size() > 2 * 1024 * 1024) {
            s_sharedLogFile->close();
            s_sharedLogFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
        }
    } else {
        fprintf(stderr, "Could not open shared log file: %s\n", qPrintable(sharedPath));
    }

    // Try opening sandbox internal log file
    QString sandboxDir = QDir::homePath() + "/data";
    QDir().mkpath(sandboxDir);
    QString sandboxPath = sandboxDir + "/libris.log";
    s_sandboxLogFile = new QFile(sandboxPath);
    if (s_sandboxLogFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        if (s_sandboxLogFile->size() > 2 * 1024 * 1024) {
            s_sandboxLogFile->close();
            s_sandboxLogFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
        }
    } else {
        fprintf(stderr, "Could not open sandbox log file: %s\n", qPrintable(sandboxPath));
    }

    qInstallMsgHandler(librisLogHandler);
    qAddPostRoutine(closeLogger);

    qDebug() << "==========================================";
    qDebug() << "=== Libris Starting ===";
    qDebug() << "Time:" << QDateTime::currentDateTime().toString(Qt::ISODate);
    qDebug() << "Log files:";
    qDebug() << "  Shared: " << sharedPath << (s_sharedLogFile->isOpen() ? "[OK]" : "[FAIL]");
    qDebug() << "  Sandbox:" << sandboxPath << (s_sandboxLogFile->isOpen() ? "[OK]" : "[FAIL]");
    qDebug() << "==========================================";
}

Q_DECL_EXPORT int main(int argc, char **argv)
{
    // Force UTF-8 encoding across Qt 4 runtime for Vietnamese & international text
    QTextCodec *utf8Codec = QTextCodec::codecForName("UTF-8");
    if (utf8Codec) {
        QTextCodec::setCodecForCStrings(utf8Codec);
        QTextCodec::setCodecForTr(utf8Codec);
        QTextCodec::setCodecForLocale(utf8Codec);
    }

    initLogger();

    // Register cross-thread queued types for ScanWorker / SearchWorker
    qRegisterMetaType<QList<QVariantMap> >("QList<QVariantMap>");
    qRegisterMetaType<QList<QVariantMap> >();
    qRegisterMetaType<QVariantList>("QVariantList");

    // Initialize global SSL / TLS configuration for BlackBerry 10 (Qt 4.8.6)
    if (QSslSocket::supportsSsl()) {
        qDebug() << "[NetworkSecurity] QSslSocket::supportsSsl() is true | Supported ciphers count:"
                 << QSslSocket::supportedCiphers().size();

        QStringList certCandidates;
        certCandidates << "app/native/assets/certs/cacert.pem"
                       << "assets/certs/cacert.pem"
                       << QDir::currentPath() + "/app/native/assets/certs/cacert.pem"
                       << QDir::currentPath() + "/assets/certs/cacert.pem";

        QList<QSslCertificate> caCerts;
        for (int i = 0; i < certCandidates.size(); ++i) {
            QFile certFile(certCandidates[i]);
            if (certFile.open(QIODevice::ReadOnly)) {
                caCerts = QSslCertificate::fromData(certFile.readAll(), QSsl::Pem);
                certFile.close();
                if (!caCerts.isEmpty()) {
                    qDebug() << "[NetworkSecurity] Successfully loaded" << caCerts.size()
                             << "bundled CA certificates from" << certCandidates[i];
                    break;
                }
            }
        }

        if (caCerts.isEmpty()) {
            qWarning() << "[NetworkSecurity] Could not load bundled CA certs from any candidate path!";
        } else {
            QSslSocket::setDefaultCaCertificates(caCerts);
        }

        // Set AnyProtocol in Qt 4.8 so OpenSSL 1.0.1 can negotiate TLS 1.2
        // (QSsl::SecureProtocols in Qt 4.8 forces TLSv1_client_method / TLS 1.0 which modern servers reject)
        // Set PeerVerifyMode to VerifyNone as fallback so self-signed/untrusted certs never block the user
        QSslConfiguration defConfig = QSslConfiguration::defaultConfiguration();
        defConfig.setProtocol(QSsl::AnyProtocol);
        defConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        defConfig.setSslOption(QSsl::SslOptionDisableServerNameIndication, false);
        if (!caCerts.isEmpty()) {
            defConfig.setCaCertificates(caCerts);
        }
        QSslConfiguration::setDefaultConfiguration(defConfig);
        qDebug() << "[NetworkSecurity] Global SSL initialized with AnyProtocol (TLS 1.2 enabled), VerifyNone, and SNI enabled.";
    } else {
        qWarning() << "[NetworkSecurity] QSslSocket::supportsSsl() returned false!";
    }

    int ret = 0;
    {
        qDebug() << "Creating bb::cascades::Application...";
        Application app(argc, argv);

        qDebug() << "Initializing ApplicationUI...";
        ApplicationUI ui;

        qDebug() << "Entering Application::exec() main event loop...";
        ret = Application::exec();
        qDebug() << "Application exited with code:" << ret;
    }

    closeLogger();
    return ret;
}
