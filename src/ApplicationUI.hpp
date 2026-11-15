#ifndef APPLICATIONUI_HPP
#define APPLICATIONUI_HPP

#include <QObject>
#include <bb/system/InvokeManager>
#include <bb/system/InvokeRequest>
#include "BookManager.hpp"

namespace bb {
    namespace cascades {
        class Application;
        class AbstractPane;
    }
}

class ApplicationUI : public QObject {
    Q_OBJECT

public:
    explicit ApplicationUI(QObject *parent = 0);
    virtual ~ApplicationUI();

    Q_INVOKABLE void openBook(const QString &filePath);
    Q_INVOKABLE void openBookFromExternal(const QString &filePath);
    Q_INVOKABLE void showAbout();
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void openReadingStats();
    Q_INVOKABLE void openFilePicker();
    Q_INVOKABLE void sendLog();
    Q_INVOKABLE void switchToTab(const QString &tabName);
    Q_INVOKABLE void exitApp();
    Q_INVOKABLE void promptExit();

signals:
    void openBookRequested(const QString &filePath);
    void switchToTabRequested(const QString &tabName);
    void showAboutRequested();
    void openSettingsRequested();
    void openReadingStatsRequested();
    void openFilePickerRequested();
    void promptExitRequested();

public slots:
    void onInvoked(const bb::system::InvokeRequest &request);
    void onThumbnail();
    void onFullscreen();
    void onAsleep();
    void onAwake();
    void onInvisible();
    void onAboutToQuit();

private:
    BookManager m_bookManager;
    bb::system::InvokeManager m_invokeManager;
    bb::cascades::AbstractPane *m_rootPane;
    QString m_pendingInvokePath;

    void initCover();
    void processInvokeRequest(const bb::system::InvokeRequest &request);
};

#endif // APPLICATIONUI_HPP
