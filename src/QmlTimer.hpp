#ifndef QMLTIMER_HPP
#define QMLTIMER_HPP

#include <QObject>
#include <QTimer>

class QmlTimer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)
    Q_PROPERTY(bool repeat READ repeat WRITE setRepeat NOTIFY repeatChanged)
    Q_PROPERTY(bool singleShot READ singleShot WRITE setSingleShot NOTIFY singleShotChanged)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool active READ isRunning WRITE setRunning NOTIFY runningChanged)

public:
    explicit QmlTimer(QObject *parent = 0)
        : QObject(parent)
        , m_repeat(false)
    {
        m_timer.setSingleShot(true);
        connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    }

    int interval() const { return m_timer.interval(); }
    void setInterval(int msec) {
        if (m_timer.interval() != msec) {
            m_timer.setInterval(msec);
            emit intervalChanged(msec);
        }
    }

    bool repeat() const { return m_repeat; }
    void setRepeat(bool rep) {
        if (m_repeat != rep) {
            m_repeat = rep;
            m_timer.setSingleShot(!m_repeat);
            emit repeatChanged(m_repeat);
            emit singleShotChanged(!m_repeat);
        }
    }

    bool singleShot() const { return !m_repeat; }
    void setSingleShot(bool s) {
        setRepeat(!s);
    }

    bool isRunning() const { return m_timer.isActive(); }
    void setRunning(bool run) {
        if (run && !m_timer.isActive()) {
            start();
        } else if (!run && m_timer.isActive()) {
            stop();
        }
    }

public Q_SLOTS:
    void start() {
        m_timer.setSingleShot(!m_repeat);
        m_timer.start();
        emit runningChanged(true);
    }

    void stop() {
        m_timer.stop();
        emit runningChanged(false);
    }

    void restart() {
        stop();
        start();
    }

private Q_SLOTS:
    void onTimeout() {
        if (!m_repeat) {
            emit runningChanged(false);
        }
        emit timeout();
        emit triggered();
    }

Q_SIGNALS:
    void timeout();
    void triggered();
    void intervalChanged(int interval);
    void repeatChanged(bool repeat);
    void singleShotChanged(bool singleShot);
    void runningChanged(bool running);

private:
    QTimer m_timer;
    bool m_repeat;
};

#endif // QMLTIMER_HPP
