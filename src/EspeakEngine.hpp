#ifndef ESPEAKENGINE_HPP
#define ESPEAKENGINE_HPP

#include <QString>
#include <QMutex>

class EspeakEngine
{
public:
    static EspeakEngine &instance();

    bool ensureInitialized();
    bool isAvailable() const { return m_sampleRate > 0; }
    bool synthesizeToWav(const QString &text, const QString &voiceSpec,
                         int rateWpm, int pitchVal,
                         const QString &outputWavPath, QString *errorOut = 0);

private:
    EspeakEngine();
    int m_sampleRate;
    bool m_triedInit;
    QMutex m_mutex;
};

#endif
