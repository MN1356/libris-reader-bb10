#include "EspeakEngine.hpp"
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QDebug>
#include <string.h>

extern "C" {
#include <espeak-ng/speak_lib.h>
}

static QByteArray *s_pcm = 0;

static int synthCallback(short *wav, int numsamples, espeak_EVENT *events)
{
    (void)events;
    if (wav && numsamples > 0 && s_pcm) {
        s_pcm->append(reinterpret_cast<const char *>(wav), numsamples * 2);
    }
    return 0;
}

EspeakEngine::EspeakEngine() : m_sampleRate(0), m_triedInit(false) {}

EspeakEngine &EspeakEngine::instance()
{
    static EspeakEngine inst;
    return inst;
}

bool EspeakEngine::ensureInitialized()
{
    QMutexLocker lock(&m_mutex);
    if (m_sampleRate > 0) return true;
    if (m_triedInit) return false;
    m_triedInit = true;

    QString parent;
    if (QFileInfo("app/native/assets/espeak-ng-data/phondata").exists()) parent = "app/native/assets";
    else if (QFileInfo("assets/espeak-ng-data/phondata").exists()) parent = "assets";
    else {
        qDebug() << "[EspeakEngine] data dir not found";
        return false;
    }
    int sr = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, parent.toUtf8().constData(), 0);
    if (sr <= 0) {
        qDebug() << "[EspeakEngine] espeak_Initialize failed" << sr;
        return false;
    }
    espeak_SetSynthCallback(synthCallback);
    m_sampleRate = sr;
    qDebug() << "[EspeakEngine] Initialized OK (sampleRate=" << sr << ", path=" << parent << ")";
    return true;
}

bool EspeakEngine::synthesizeToWav(const QString &text, const QString &voiceSpec,
                                   int rateWpm, int pitchVal,
                                   const QString &outputWavPath, QString *errorOut)
{
    if (!ensureInitialized()) {
        if (errorOut) *errorOut = "eSpeak not initialized";
        return false;
    }
    QMutexLocker lock(&m_mutex);

    if (espeak_SetVoiceByName(voiceSpec.toUtf8().constData()) != EE_OK) {
        QString base = voiceSpec.section('+', 0, 0);
        if (espeak_SetVoiceByName(base.toUtf8().constData()) != EE_OK) {
            if (espeak_SetVoiceByName(base.startsWith("vi") ? "vi" : "en") != EE_OK) {
                if (errorOut) *errorOut = "voice not found";
                return false;
            }
        }
    }
    espeak_SetParameter(espeakRATE, qBound(80, rateWpm, 360), 0);
    espeak_SetParameter(espeakPITCH, qBound(10, pitchVal, 95), 0);

    QByteArray pcm;
    s_pcm = &pcm;
    QByteArray utf8 = text.toUtf8();
    espeak_ERROR err = espeak_Synth(utf8.constData(), utf8.size() + 1, 0, POS_CHARACTER, 0,
                                    espeakCHARS_UTF8, NULL, NULL);
    espeak_Synchronize();
    s_pcm = 0;

    if (err != EE_OK || pcm.isEmpty()) {
        if (errorOut) *errorOut = "synthesis produced no audio";
        return false;
    }

    QByteArray h(44, 0);
    char *p = h.data();
    quint32 dataSize = pcm.size();
    quint32 chunk = 36 + dataSize, fmtSize = 16, sr = m_sampleRate, br = m_sampleRate * 2;
    quint16 fmt = 1, ch = 1, ba = 2, bps = 16;
    memcpy(p, "RIFF", 4); memcpy(p + 4, &chunk, 4); memcpy(p + 8, "WAVE", 4);
    memcpy(p + 12, "fmt ", 4); memcpy(p + 16, &fmtSize, 4);
    memcpy(p + 20, &fmt, 2); memcpy(p + 22, &ch, 2);
    memcpy(p + 24, &sr, 4); memcpy(p + 28, &br, 4);
    memcpy(p + 32, &ba, 2); memcpy(p + 34, &bps, 2);
    memcpy(p + 36, "data", 4); memcpy(p + 40, &dataSize, 4);

    QFile f(outputWavPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorOut) *errorOut = "cannot write wav";
        return false;
    }
    f.write(h);
    f.write(pcm);
    f.close();
    return true;
}
