#include "ApplicationUI.hpp"
#include "QmlTimer.hpp"
#include "OpdsManager.hpp"
#include <unistd.h>
#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/AbstractPane>
#include <bb/cascades/SceneCover>
#include <bb/cascades/Control>
#include <bb/cascades/Menu>
#include <bb/cascades/ActionItem>
#include <bb/cascades/HelpActionItem>
#include <bb/cascades/SettingsActionItem>
#include <bb/cascades/TouchKeyboardHandler>
#include <bb/cascades/TouchKeyboardEvent>
#include <bb/cascades/TouchType>
#include <bb/cascades/TrackpadHandler>
#include <bb/cascades/TrackpadEvent>
#include <bb/cascades/ConsumeTrackpadEvent>
#include <bb/cascades/TrackpadEventType>
#include <bb/cascades/NavigationHandler>
#include <bb/cascades/NavigationEvent>
#include <bb/cascades/ConsumeNavigationEvent>
#include <bb/cascades/NavigationEventType>
#include <bb/cascades/Page>
#include <bb/cascades/Container>
#include <bb/cascades/Label>
#include <bb/cascades/StackLayout>
#include <bb/cascades/SystemDefaults>
#include <bb/cascades/Color>
#include <bb/system/InvokeManager>
#include <bb/system/InvokeRequest>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QDeclarativeError>
#include <QCoreApplication>
#include <QUrl>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QList>
#include <QDebug>
#include <cstdlib>
#ifdef QT_DECLARATIVE_DEBUG
#include <bb/cascades/Window>
#include <bb/system/Screenshot>
#endif

using namespace bb::cascades;

ApplicationUI::ApplicationUI(QObject *parent)
    : QObject(parent)
    , m_rootPane(NULL)
    , m_debugCaptureTimer(NULL)
    , m_isForeground(true)
    , m_exitSaved(false)
{
    qDebug() << "ApplicationUI constructor: Initializing InvokeManager...";

    // Ensure clean process termination and zero battery drain in background/sleep
    if (Application::instance()) {
        Application::instance()->setAutoExit(true);
        connect(Application::instance(), SIGNAL(thumbnail()), this, SLOT(onThumbnail()));
        connect(Application::instance(), SIGNAL(fullscreen()), this, SLOT(onFullscreen()));
        connect(Application::instance(), SIGNAL(asleep()), this, SLOT(onAsleep()));
        connect(Application::instance(), SIGNAL(awake()), this, SLOT(onAwake()));
        connect(Application::instance(), SIGNAL(invisible()), this, SLOT(onInvisible()));
        connect(Application::instance(), SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));
    }

    // Connect invocation signals to receive launch/open requests from BB10 File Manager
    connect(&m_invokeManager, SIGNAL(invoked(const bb::system::InvokeRequest&)),
            this, SLOT(onInvoked(const bb::system::InvokeRequest&)));

    // Register TouchKeyboardHandler, TrackpadHandler, and NavigationHandler for Passport and Classic (Q20)
    qmlRegisterType<bb::cascades::TouchKeyboardHandler>("bb.cascades", 1, 3, "TouchKeyboardHandler");
    qmlRegisterUncreatableType<bb::cascades::TouchKeyboardEvent>("bb.cascades", 1, 3, "TouchKeyboardEvent", "Cannot create TouchKeyboardEvent");
    qmlRegisterUncreatableType<bb::cascades::TouchType>("bb.cascades", 1, 3, "TouchType", "Enum container");

    qmlRegisterType<bb::cascades::TrackpadHandler>("bb.cascades", 1, 3, "TrackpadHandler");
    qmlRegisterUncreatableType<bb::cascades::TrackpadEvent>("bb.cascades", 1, 3, "TrackpadEvent", "Cannot create TrackpadEvent");
    qmlRegisterUncreatableType<bb::cascades::ConsumeTrackpadEvent>("bb.cascades", 1, 3, "ConsumeTrackpadEvent", "Enum container");
    qmlRegisterUncreatableType<bb::cascades::TrackpadEventType>("bb.cascades", 1, 3, "TrackpadEventType", "Enum container");

    qmlRegisterType<bb::cascades::NavigationHandler>("bb.cascades", 1, 3, "NavigationHandler");
    qmlRegisterUncreatableType<bb::cascades::NavigationEvent>("bb.cascades", 1, 3, "NavigationEvent", "Cannot create NavigationEvent");
    qmlRegisterUncreatableType<bb::cascades::ConsumeNavigationEvent>("bb.cascades", 1, 3, "ConsumeNavigationEvent", "Enum container");
    qmlRegisterUncreatableType<bb::cascades::NavigationEventType>("bb.cascades", 1, 3, "NavigationEventType", "Enum container");

    // Also register for 1, 2 for backwards compatibility
    qmlRegisterType<bb::cascades::TouchKeyboardHandler>("bb.cascades", 1, 2, "TouchKeyboardHandler");
    qmlRegisterUncreatableType<bb::cascades::TouchKeyboardEvent>("bb.cascades", 1, 2, "TouchKeyboardEvent", "Cannot create TouchKeyboardEvent");
    qmlRegisterUncreatableType<bb::cascades::TouchType>("bb.cascades", 1, 2, "TouchType", "Enum container");
    qmlRegisterType<bb::cascades::TrackpadHandler>("bb.cascades", 1, 2, "TrackpadHandler");
    qmlRegisterUncreatableType<bb::cascades::TrackpadEvent>("bb.cascades", 1, 2, "TrackpadEvent", "Cannot create TrackpadEvent");
    qmlRegisterUncreatableType<bb::cascades::ConsumeTrackpadEvent>("bb.cascades", 1, 2, "ConsumeTrackpadEvent", "Enum container");
    qmlRegisterUncreatableType<bb::cascades::TrackpadEventType>("bb.cascades", 1, 2, "TrackpadEventType", "Enum container");
    qmlRegisterType<bb::cascades::NavigationHandler>("bb.cascades", 1, 2, "NavigationHandler");
    qmlRegisterUncreatableType<bb::cascades::NavigationEvent>("bb.cascades", 1, 2, "NavigationEvent", "Cannot create NavigationEvent");
    qmlRegisterUncreatableType<bb::cascades::ConsumeNavigationEvent>("bb.cascades", 1, 2, "ConsumeNavigationEvent", "Enum container");
    qmlRegisterUncreatableType<bb::cascades::NavigationEventType>("bb.cascades", 1, 2, "NavigationEventType", "Enum container");

    // Register Timer and QTimer for Cascades QML
    qmlRegisterType<QmlTimer>("bb.cascades", 1, 3, "Timer");
    qmlRegisterType<QmlTimer>("bb.cascades", 1, 2, "Timer");
    qmlRegisterType<QmlTimer>("bb.cascades", 1, 0, "Timer");
    qmlRegisterType<QmlTimer>("bb.cascades", 1, 3, "QTimer");
    qmlRegisterType<QmlTimer>("bb.cascades", 1, 2, "QTimer");
    qmlRegisterType<QmlTimer>("bb.cascades", 1, 0, "QTimer");

    // Register context properties globally so all ListItemComponents and Sheets can access them
    QDeclarativeEngine *engine = QmlDocument::defaultDeclarativeEngine();
    if (engine && engine->rootContext()) {
        engine->rootContext()->setContextProperty("app", this);
        engine->rootContext()->setContextProperty("bookManager", &m_bookManager);
        engine->rootContext()->setContextProperty("ttsManager", m_bookManager.ttsManager());
        engine->rootContext()->setContextProperty("dictManager", m_bookManager.dictManager());
        engine->rootContext()->setContextProperty("opdsManager", m_bookManager.opdsManager());
        engine->rootContext()->setContextProperty("wifiDropServer", m_bookManager.wifiDropServer());
    }

    QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(this);
    if (!qml) {
        qCritical() << "Fatal: QmlDocument::create('asset:///main.qml') returned null!";
        return;
    }

    qml->setContextProperty("app", this);
    qml->setContextProperty("bookManager", &m_bookManager);
    qml->setContextProperty("ttsManager", m_bookManager.ttsManager());
    qml->setContextProperty("dictManager", m_bookManager.dictManager());
    qml->setContextProperty("opdsManager", m_bookManager.opdsManager());
    qml->setContextProperty("wifiDropServer", m_bookManager.wifiDropServer());

    if (qml->hasErrors()) {
        QList<QDeclarativeError> errs = qml->errors();
        for (int i = 0; i < errs.size(); ++i) {
            qCritical() << "[QML ERROR before createRootObject]" << errs[i].toString();
        }
    }

    qDebug() << "ApplicationUI: Instantiating root object from main.qml...";
    m_rootPane = qml->createRootObject<AbstractPane>();
    if (!m_rootPane) {
        qCritical() << "Fatal: Failed to load asset:///main.qml (root is null)! Checking QML errors:";
        QString errorDetails;
        QList<QDeclarativeError> errs = qml->errors();
        for (int i = 0; i < errs.size(); ++i) {
            qCritical() << "  -> " << errs[i].toString();
            errorDetails += errs[i].toString() + "\n";
        }

        // Safety Fallback Scene: Never leave the screen as a blank black screen
        Page *fallbackPage = new Page();
        Container *fallbackContainer = new Container();
        fallbackContainer->setLayout(new StackLayout());
        fallbackContainer->setTopPadding(40.0f);
        fallbackContainer->setLeftPadding(30.0f);
        fallbackContainer->setRightPadding(30.0f);

        Label *titleLabel = new Label(fallbackContainer);
        titleLabel->setText(QString::fromUtf8("Libris - Khởi Động Thất Bại"));
        titleLabel->textStyle()->setBase(SystemDefaults::TextStyles::titleText());
        titleLabel->textStyle()->setColor(Color::fromARGB(0xFFEF4444));

        Label *descLabel = new Label(fallbackContainer);
        descLabel->setText(QString::fromUtf8("Không thể tải giao diện chính asset:///main.qml:\n\n") + errorDetails);
        descLabel->setMultiline(true);
        descLabel->textStyle()->setBase(SystemDefaults::TextStyles::smallText());
        descLabel->textStyle()->setColor(Color::White);

        fallbackContainer->add(titleLabel);
        fallbackContainer->add(descLabel);
        fallbackPage->setContent(fallbackContainer);

        Application::instance()->setScene(fallbackPage);
        return;
    }

    // Expose root pane globally so child pages and sheets can access it
    if (engine && engine->rootContext()) {
        engine->rootContext()->setContextProperty("rootPane", m_rootPane);
        engine->rootContext()->setContextProperty("rootTabbedPane", m_rootPane);
    }

    qDebug() << "ApplicationUI: Setting application scene...";
    Application::instance()->setScene(m_rootPane);
    qDebug() << "ApplicationUI: Scene set successfully!";

    // Verify and ensure native Application Menu (swipe-down from top bezel)
    Menu *menu = Application::instance()->menu();
    qDebug() << "ApplicationUI: Application menu after setScene is:" << menu;
    if (!menu) {
        menu = m_rootPane->findChild<Menu*>("appMenu");
        if (!menu) {
            menu = m_rootPane->findChild<Menu*>();
        }
        if (menu) {
            qDebug() << "ApplicationUI: Explicitly assigning found Menu object to Application:" << menu;
            Application::instance()->setMenu(menu);
        } else {
            qWarning() << "ApplicationUI: WARNING - No Menu object found in root scene!";
        }
    }

    // Explicitly enable BB10 top bezel swipe Application Menu gesture
    Application::instance()->setMenuEnabled(true);
    qDebug() << "ApplicationUI: Application isMenuEnabled:" << Application::instance()->isMenuEnabled();
    qDebug() << "ApplicationUI: Final Application Menu ptr:" << Application::instance()->menu();

    // If an invocation was received before the scene was ready, process it now
    if (!m_pendingInvokePath.isEmpty()) {
        qDebug() << "ApplicationUI: Processing pending invoke path from cold start:" << m_pendingInvokePath;
        QString path = m_pendingInvokePath;
        m_pendingInvokePath.clear();
        openBookFromExternal(path);
    }

    // Initialize Active Frame (Cover)
    initCover();

#ifdef QT_DECLARATIVE_DEBUG
    m_debugCaptureTimer = new QTimer(this);
    m_debugCaptureTimer->setInterval(600);
    connect(m_debugCaptureTimer, SIGNAL(timeout()), this, SLOT(onDebugCaptureTimeout()));
    if (m_isForeground) {
        m_debugCaptureTimer->start();
    }
#endif
}

ApplicationUI::~ApplicationUI()
{
}

void ApplicationUI::initCover()
{
    qDebug() << "ApplicationUI: Initializing Cover Active Frame...";
    QmlDocument *qmlCover = QmlDocument::create("asset:///Cover.qml").parent(this);
    if (!qmlCover) {
        qWarning() << "Failed to create QmlDocument for Cover.qml";
        return;
    }

    qmlCover->setContextProperty("bookManager", &m_bookManager);

    if (qmlCover->hasErrors()) {
        QList<QDeclarativeError> errs = qmlCover->errors();
        for (int i = 0; i < errs.size(); ++i) {
            qWarning() << "[Cover QML Error]" << errs[i].toString();
        }
    }

    Control *coverRoot = qmlCover->createRootObject<Control>();
    if (coverRoot) {
        SceneCover *sceneCover = SceneCover::create().content(coverRoot);
        Application::instance()->setCover(sceneCover);
        qDebug() << "ApplicationUI: Active Frame Cover set successfully!";
    } else {
        qWarning() << "Warning: Failed to load asset:///Cover.qml for Active Frame. Checking errors:";
        QList<QDeclarativeError> errs = qmlCover->errors();
        for (int i = 0; i < errs.size(); ++i) {
            qWarning() << "  -> " << errs[i].toString();
        }
    }
}

void ApplicationUI::onInvoked(const bb::system::InvokeRequest &request)
{
    qDebug() << "ApplicationUI::onInvoked - Action:" << request.action()
             << "Target:" << request.target()
             << "MimeType:" << request.mimeType()
             << "URI:" << request.uri().toString();

    processInvokeRequest(request);
}

void ApplicationUI::processInvokeRequest(const bb::system::InvokeRequest &request)
{
    QUrl uri = request.uri();
    QString filePath;

    if (!uri.isEmpty()) {
        filePath = uri.toLocalFile();
        if (filePath.isEmpty()) {
            filePath = uri.path();
            if (filePath.startsWith("file://", Qt::CaseInsensitive)) {
                filePath = filePath.mid(7);
            }
            filePath = QUrl::fromPercentEncoding(filePath.toUtf8());
        } else if (filePath.startsWith("file://", Qt::CaseInsensitive)) {
            filePath = filePath.mid(7);
        }
    } else if (!request.data().isEmpty()) {
        filePath = QString::fromUtf8(request.data()).trimmed();
        if (filePath.startsWith("file://", Qt::CaseInsensitive)) {
            filePath = filePath.mid(7);
        }
        filePath = QUrl::fromPercentEncoding(filePath.toUtf8());
    }

    qDebug() << "ApplicationUI::processInvokeRequest - Resolved filePath:" << filePath;

    if (filePath.isEmpty()) {
        qWarning() << "ApplicationUI::processInvokeRequest: Empty file path!";
        return;
    }

    if (m_rootPane) {
        openBookFromExternal(filePath);
    } else {
        qDebug() << "ApplicationUI::processInvokeRequest: Root pane not ready yet, queueing path:" << filePath;
        m_pendingInvokePath = filePath;
    }
}

void ApplicationUI::openBookFromExternal(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    qDebug() << "ApplicationUI::openBookFromExternal: Triggering open for:" << filePath;

    if (m_rootPane) {
        emit openBookRequested(filePath);
    } else {
        m_pendingInvokePath = filePath;
    }
}

void ApplicationUI::openBook(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    qDebug() << "ApplicationUI::openBook: Opening:" << filePath;
    emit openBookRequested(filePath);
}

void ApplicationUI::showAbout()
{
    qDebug() << "ApplicationUI::showAbout called";
    emit showAboutRequested();
}

void ApplicationUI::openSettings()
{
    qDebug() << "ApplicationUI::openSettings called";
    emit openSettingsRequested();
}

void ApplicationUI::openReadingStats()
{
    qDebug() << "ApplicationUI::openReadingStats called";
    emit openReadingStatsRequested();
}

void ApplicationUI::openFilePicker()
{
    qDebug() << "ApplicationUI::openFilePicker called";
    emit openFilePickerRequested();
}

void ApplicationUI::openWifiDrop()
{
    qDebug() << "ApplicationUI::openWifiDrop called";
    emit openWifiDropRequested();
}

void ApplicationUI::openOpds()
{
    qDebug() << "ApplicationUI::openOpds called";
    emit openOpdsRequested();
}

void ApplicationUI::sendLog()
{
}

void ApplicationUI::switchToTab(const QString &tabName)
{
    qDebug() << "ApplicationUI::switchToTab called for:" << tabName;
    emit switchToTabRequested(tabName);
}

void ApplicationUI::promptExit()
{
    qDebug() << "ApplicationUI::promptExit called";
    emit promptExitRequested();
}

void ApplicationUI::exitApp()
{
    qDebug() << "[ApplicationUI] exitApp: Performing clean exit and state preservation...";
    if (!m_exitSaved) {
        m_exitSaved = true;
        m_bookManager.saveForShutdown();
    }
    m_bookManager.pauseTimersForSleep(false);
    if (m_bookManager.ttsManager() && m_bookManager.ttsManager()->isActive()) {
        m_bookManager.ttsManager()->stop();
    }
    m_bookManager.cancelWorkers();

    // Start a 600ms watchdog timer: if the Cascades/Navigator handshake takes too long
    // or any native thread fails to join, force clean process termination so the icon
    // on the BlackBerry 10 home screen never gets dimmed/frozen.
    QTimer::singleShot(600, this, SLOT(onHardExitTimeout()));

    qDebug() << "[ApplicationUI] exitApp: Requesting application exit via Navigator...";
    if (Application::instance()) {
        Application::instance()->requestExit();
    }
}

void ApplicationUI::onHardExitTimeout()
{
    qDebug() << "[ApplicationUI] onHardExitTimeout: Watchdog triggered -> terminating process now";
    _exit(0);
}

void ApplicationUI::onThumbnail()
{
    qDebug() << "[ApplicationUI] onThumbnail: app minimized to Active Frame";
    m_isForeground = false;
    if (m_debugCaptureTimer) m_debugCaptureTimer->stop();
    m_bookManager.pauseTimersForSleep();
}

void ApplicationUI::onFullscreen()
{
    qDebug() << "[ApplicationUI] onFullscreen: app returned to foreground";
    m_isForeground = true;
    if (m_debugCaptureTimer) m_debugCaptureTimer->start();
    m_bookManager.resumeTimersFromSleep();
    if (m_bookManager.totalScannedBooks() == 0 && !m_bookManager.isScanning()) {
        qDebug() << "[ApplicationUI] onFullscreen: 0 books detected on foreground resume (e.g. permission newly granted), triggering library scan...";
        m_bookManager.scanLibrary();
    }
}

void ApplicationUI::onAsleep()
{
    qDebug() << "[ApplicationUI] onAsleep: device screen turned off / standby";
    if (m_debugCaptureTimer) m_debugCaptureTimer->stop();
    m_bookManager.pauseTimersForSleep();
}

void ApplicationUI::onAwake()
{
    qDebug() << "[ApplicationUI] onAwake: device screen turned on";
    if (m_isForeground) {
        if (m_debugCaptureTimer) m_debugCaptureTimer->start();
        m_bookManager.resumeTimersFromSleep();
    }
}

void ApplicationUI::onInvisible()
{
    qDebug() << "[ApplicationUI] onInvisible: app window is completely hidden";
    m_isForeground = false;
    if (m_debugCaptureTimer) m_debugCaptureTimer->stop();
    m_bookManager.pauseTimersForSleep();
}

void ApplicationUI::onAboutToQuit()
{
    if (m_exitSaved) {
        return;
    }
    m_exitSaved = true;
    qDebug() << "[ApplicationUI] onAboutToQuit: Performing complete shutdown cleanup";
    if (m_debugCaptureTimer) m_debugCaptureTimer->stop();
    m_bookManager.saveForShutdown();
    m_bookManager.pauseTimersForSleep(false);
    if (m_bookManager.ttsManager() && m_bookManager.ttsManager()->isActive()) {
        m_bookManager.ttsManager()->stop();
    }
    m_bookManager.cancelWorkers();
}

void ApplicationUI::onDebugCaptureTimeout()
{
#ifdef QT_DECLARATIVE_DEBUG
    QString reqPath = QDir::currentPath() + "/data/ui_capture.req";
    if (!QFile::exists(reqPath)) {
        return;
    }
    QFile::remove(reqPath);
    bb::cascades::Window *win = Application::instance() ? Application::instance()->mainWindow() : 0;
    if (!win) {
        qWarning() << "[DebugCapture] mainWindow() is null";
        return;
    }
    QString outPath = QDir::currentPath() + "/data/ui_capture.png";
    QFile::remove(outPath);
    bb::system::Screenshot ss;
    QString saved = ss.captureWindow(QUrl::fromLocalFile(outPath), win->handle());
    qDebug() << "[DebugCapture] captureWindow saved:" << saved << "err:" << ss.error() << "outPath:" << outPath;
#endif
}

void ApplicationUI::shareFile(const QString &filePath, const QString &mimeType)
{
    QString cleanPath = filePath.trimmed();
    if (cleanPath.isEmpty()) return;
    if (cleanPath.startsWith("file://")) {
        cleanPath = QUrl(cleanPath).toLocalFile();
    }
    bb::system::InvokeRequest req;
    req.setAction("bb.action.SHARE");
    req.setMimeType(mimeType.isEmpty() ? "image/png" : mimeType);
    req.setUri(QUrl::fromLocalFile(cleanPath));
    m_invokeManager.invoke(req);
}

void ApplicationUI::shareText(const QString &text)
{
    QString clean = text.trimmed();
    if (clean.isEmpty()) return;
    bb::system::InvokeRequest req;
    req.setAction("bb.action.SHARE");
    req.setMimeType("text/plain");
    req.setData(clean.toUtf8());
    m_invokeManager.invoke(req);
}
