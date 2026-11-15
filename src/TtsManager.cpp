#include "TtsManager.hpp"
#include <bb/multimedia/EqualizerPreset>
#include <QNetworkRequest>
#include <QNetworkConfigurationManager>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QDebug>
#include <QRegExp>
#include <QSettings>
#include <QSet>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>
#include <QScriptEngine>
#include <QScriptValue>

static void setupReplyBypass(QNetworkReply *reply)
{
    if (!reply) return;
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::AnyProtocol);
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setSslOption(QSsl::SslOptionDisableServerNameIndication, false);
    reply->setSslConfiguration(sslConfig);
    reply->ignoreSslErrors();
    QObject::connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply, SLOT(ignoreSslErrors()));
}

static void setupReplyBypassReq(QNetworkRequest &req)
{
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::AnyProtocol);
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setSslOption(QSsl::SslOptionDisableServerNameIndication, false);
    req.setSslConfiguration(sslConfig);
}

TtsManager::TtsManager(QObject *parent)
    : QObject(parent)
    , m_player(new bb::multimedia::MediaPlayer(this))
    , m_nam(new QNetworkAccessManager(this))
    , m_currentReply(NULL)
    , m_prefetchReply(NULL)
    , m_batchCacheReply(NULL)
    , m_downloadReply(NULL)
    , m_geminiTestReply(NULL)
    , m_currentIndex(0)
    , m_active(false)
    , m_playing(false)
    , m_loading(false)
    , m_speed(1.0)
    , m_pitch(1.0)
    , m_language("auto")
    , m_detectedChapterLang("vi")
    , m_ttsProvider("google")
    , m_ttsVoice("tw-ob")
    , m_ttsEngine("tw-ob")
    , m_ttsMode("online")
    , m_zaloApiKey("")
    , m_fptApiKey("")
    , m_voiceRssApiKey("")
    , m_customTtsUrl("http://localhost:5050/v1/audio/speech")
    , m_customTtsVoice("vi-VN-HoaiMyNeural")
    , m_customApiKey("")
    , m_geminiApiKey("")
    , m_geminiVoice("Puck")
    , m_geminiModel("gemini-2.5-flash")
    , m_geminiStyle("auto")
    , m_geminiTestStatus("")
    , m_geminiTestState("idle")
    , m_isTestingGemini(false)
    , m_bookTitle("")
    , m_bookAuthor("")
    , m_isChapterReading(false)
    , m_isOffline(false)
    , m_isCaching(false)
    , m_cachingIndex(0)
    , m_googleEndpointIndex(0)
{
    QSettings settings("BerryLife", "BerryReader");
    m_language = settings.value("reader/ttsLanguage", "auto").toString();
    m_ttsProvider = settings.value("reader/ttsProvider", "google").toString();
    if (m_ttsProvider != "google" && m_ttsProvider != "gemini") {
        m_ttsProvider = "google";
    }
    m_ttsVoice = settings.value("reader/ttsVoice", "tw-ob").toString();
    m_ttsEngine = settings.value("reader/ttsEngine", "tw-ob").toString();
    m_ttsMode = settings.value("reader/ttsMode", "online").toString();
    m_speed = settings.value("reader/ttsSpeed", 1.0).toDouble();
    m_pitch = settings.value("reader/ttsPitch", 1.0).toDouble();
    applyPitchToPlayer();

    m_zaloApiKey = settings.value("reader/zaloApiKey", "").toString();
    m_fptApiKey = settings.value("reader/fptApiKey", "").toString();
    m_voiceRssApiKey = settings.value("reader/voiceRssApiKey", "").toString();
    m_customTtsUrl = settings.value("reader/customTtsUrl", "http://localhost:5050/v1/audio/speech").toString();
    m_customTtsVoice = settings.value("reader/customTtsVoice", "vi-VN-HoaiMyNeural").toString();
    m_customApiKey = settings.value("reader/customApiKey", "").toString();
    m_geminiApiKey = settings.value("reader/geminiApiKey", "").toString();
    m_geminiVoice = settings.value("reader/geminiVoice", "Puck").toString();
    m_geminiModel = settings.value("reader/geminiModel", "gemini-2.5-flash").toString();
    if (m_geminiModel == "gemini-2.0-flash" || m_geminiModel.isEmpty()) {
        m_geminiModel = "gemini-2.5-flash";
        settings.setValue("reader/geminiModel", m_geminiModel);
    }
    m_geminiStyle = settings.value("reader/geminiStyle", "auto").toString();

    m_cacheDir = "/accounts/1000/shared/documents/.berryreader_tts";
    if (!QDir().mkpath(m_cacheDir)) {
        m_cacheDir = "data/tts_cache";
        QDir().mkpath(m_cacheDir);
    }

    if (!settings.value("reader/ttsUrlFixV3", false).toBool()) {
        clearCache();
        settings.setValue("reader/ttsUrlFixV3", true);
    }

    connect(m_player, SIGNAL(playbackCompleted()), this, SLOT(onPlaybackCompleted()));
    connect(m_player, SIGNAL(mediaStateChanged(bb::multimedia::MediaState::Type)),
            this, SLOT(onMediaStateChanged(bb::multimedia::MediaState::Type)));
}

TtsManager::~TtsManager()
{
    stop();
    cancelGeminiTest();
}

bool TtsManager::isActive() const
{
    return m_active;
}

bool TtsManager::isPlaying() const
{
    return m_playing;
}

bool TtsManager::isLoading() const
{
    return m_loading;
}

int TtsManager::currentSentenceIndex() const
{
    return m_currentIndex;
}

int TtsManager::totalSentences() const
{
    return m_sentences.size();
}

QString TtsManager::currentSentenceText() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_sentences.size()) {
        return m_sentences[m_currentIndex];
    }
    return QString();
}

double TtsManager::speed() const
{
    return m_speed;
}

void TtsManager::setSpeed(double spd)
{
    if (spd >= 0.5 && spd <= 2.0 && spd != m_speed) {
        m_speed = spd;
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/ttsSpeed", m_speed);

        m_player->setSpeed(m_speed);
        emit speedChanged(m_speed);
    }
}

double TtsManager::pitch() const
{
    return m_pitch;
}

void TtsManager::setPitch(double p)
{
    if (p >= 0.5 && p <= 2.0 && p != m_pitch) {
        m_pitch = p;
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/ttsPitch", m_pitch);
        applyPitchToPlayer();
        emit pitchChanged(m_pitch);
    }
}

void TtsManager::togglePitch()
{
    if (m_pitch == 1.0) {
        setPitch(1.2);
    } else if (m_pitch == 1.2) {
        setPitch(0.8);
    } else {
        setPitch(1.0);
    }
}

QString TtsManager::pitchDisplayName() const
{
    if (m_pitch <= 0.85) {
        return tr("Trầm (0.8x)");
    } else if (m_pitch >= 1.35) {
        return tr("Rất bổng (1.4x)");
    } else if (m_pitch >= 1.15) {
        return tr("Bổng (1.2x)");
    }
    return tr("Chuẩn (1.0x)");
}

void TtsManager::applyPitchToPlayer()
{
    if (!m_player) return;
    if (m_pitch <= 0.85) {
        m_player->setEqualizerPreset(bb::multimedia::EqualizerPreset::VoiceLower);
    } else if (m_pitch >= 1.35) {
        m_player->setEqualizerPreset(bb::multimedia::EqualizerPreset::TrebleBoost);
    } else if (m_pitch >= 1.15) {
        m_player->setEqualizerPreset(bb::multimedia::EqualizerPreset::VoiceBoost);
    } else {
        m_player->setEqualizerPreset(bb::multimedia::EqualizerPreset::Off);
    }
}

QString TtsManager::language() const
{
    return m_language;
}

void TtsManager::setLanguage(const QString &lang)
{
    if (!lang.isEmpty() && lang != m_language) {
        m_language = lang;
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/ttsLanguage", m_language);
        emit languageChanged(m_language);
        if (m_playing) {
            playCurrentSentence();
        }
    }
}

QString TtsManager::detectedLanguage() const
{
    if (!m_detectedChapterLang.isEmpty()) {
        return m_detectedChapterLang;
    }
    return "vi";
}

QString TtsManager::effectiveLanguageForSentence(int index) const
{
    if (m_language != "auto" && !m_language.isEmpty()) {
        return m_language;
    }
    if (index >= 0 && index < m_sentences.size()) {
        QString s = m_sentences[index];
        QString detected = detectLanguage(s);
        if (!detected.isEmpty() && detected != "en") {
            return detected;
        }
    }
    if (!m_detectedChapterLang.isEmpty()) {
        return m_detectedChapterLang;
    }
    return "vi";
}

void TtsManager::cycleLanguage()
{
    if (m_language == "auto") {
        setLanguage("vi");
    } else if (m_language == "vi") {
        setLanguage("en-US");
    } else if (m_language == "en-US" || m_language == "en") {
        setLanguage("en-GB");
    } else if (m_language == "en-GB") {
        setLanguage("en-AU");
    } else if (m_language == "en-AU") {
        setLanguage("fr-FR");
    } else if (m_language == "fr-FR" || m_language == "fr") {
        setLanguage("es-ES");
    } else if (m_language == "es-ES" || m_language == "es") {
        setLanguage("de");
    } else if (m_language == "de") {
        setLanguage("ru");
    } else if (m_language == "ru") {
        setLanguage("zh-CN");
    } else if (m_language == "zh-CN") {
        setLanguage("zh-TW");
    } else if (m_language == "zh-TW") {
        setLanguage("ja");
    } else if (m_language == "ja") {
        setLanguage("ko");
    } else {
        setLanguage("auto");
    }
}

bool TtsManager::isOffline() const
{
    return m_isOffline;
}

QString TtsManager::ttsMode() const
{
    return m_ttsMode;
}

void TtsManager::setTtsMode(const QString &mode)
{
    QString m = (mode == "offline") ? "offline" : "online";
    if (m != m_ttsMode) {
        m_ttsMode = m;
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/ttsMode", m_ttsMode);
        emit ttsModeChanged(m_ttsMode);
    }
}

void TtsManager::toggleMode()
{
    if (m_ttsMode == "offline") {
        setTtsMode("online");
    } else {
        setTtsMode("offline");
    }
}

int TtsManager::cacheProgress() const
{
    return m_cachingIndex;
}

bool TtsManager::isCaching() const
{
    return m_isCaching;
}

QString TtsManager::ttsProvider() const
{
    return m_ttsProvider;
}

void TtsManager::setTtsProvider(const QString &provider)
{
    QString prov = "google";
    if (provider == "gemini") {
        prov = "gemini";
    }
    if (prov != m_ttsProvider) {
        m_ttsProvider = prov;
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/ttsProvider", m_ttsProvider);

        emit ttsProviderChanged(m_ttsProvider);
        emit ttsVoiceChanged(m_ttsVoice);

        if (m_playing) {
            playCurrentSentence();
        }
    }
}

void TtsManager::cycleProvider()
{
    if (m_ttsProvider == "google") {
        setTtsProvider("gemini");
    } else {
        setTtsProvider("google");
    }
}

QString TtsManager::ttsVoice() const
{
    return m_ttsVoice;
}

void TtsManager::setTtsVoice(const QString &voice)
{
    if (!voice.isEmpty() && voice != m_ttsVoice) {
        m_ttsVoice = voice;
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/ttsVoice", m_ttsVoice);
        emit ttsVoiceChanged(m_ttsVoice);

        if (m_playing) {
            playCurrentSentence();
        }
    }
}

void TtsManager::cycleVoice()
{
    cycleProvider();
}

QString TtsManager::voiceDisplayName() const
{
    if (m_ttsProvider == "gemini") {
        return QString("Gemini AI (%1)").arg(m_geminiVoice.isEmpty() ? "Puck" : m_geminiVoice);
    }
    return tr("Google Translate (Tự động)");
}

QString TtsManager::ttsEngine() const
{
    return m_ttsEngine;
}

void TtsManager::setTtsEngine(const QString &engine)
{
    if (!engine.isEmpty()) {
        m_ttsEngine = engine;
        m_ttsVoice = engine;
        m_ttsProvider = "google";
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/ttsEngine", m_ttsEngine);
        settings.setValue("reader/ttsVoice", m_ttsVoice);
        settings.setValue("reader/ttsProvider", m_ttsProvider);
        emit ttsEngineChanged(m_ttsEngine);
        emit ttsVoiceChanged(m_ttsVoice);
        emit ttsProviderChanged(m_ttsProvider);
        if (m_playing) {
            playCurrentSentence();
        }
    }
}

void TtsManager::cycleEngine()
{
    cycleVoice();
}

QString TtsManager::zaloApiKey() const
{
    return m_zaloApiKey;
}

void TtsManager::setZaloApiKey(const QString &key)
{
    if (key != m_zaloApiKey) {
        m_zaloApiKey = key.trimmed();
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/zaloApiKey", m_zaloApiKey);
        emit zaloApiKeyChanged(m_zaloApiKey);
    }
}

QString TtsManager::fptApiKey() const
{
    return m_fptApiKey;
}

void TtsManager::setFptApiKey(const QString &key)
{
    if (key != m_fptApiKey) {
        m_fptApiKey = key.trimmed();
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/fptApiKey", m_fptApiKey);
        emit fptApiKeyChanged(m_fptApiKey);
    }
}

QString TtsManager::voiceRssApiKey() const
{
    return m_voiceRssApiKey;
}

void TtsManager::setVoiceRssApiKey(const QString &key)
{
    if (key != m_voiceRssApiKey) {
        m_voiceRssApiKey = key.trimmed();
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/voiceRssApiKey", m_voiceRssApiKey);
        emit voiceRssApiKeyChanged(m_voiceRssApiKey);
    }
}

QString TtsManager::customTtsUrl() const
{
    return m_customTtsUrl;
}

void TtsManager::setCustomTtsUrl(const QString &url)
{
    if (url != m_customTtsUrl) {
        m_customTtsUrl = url.trimmed();
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/customTtsUrl", m_customTtsUrl);
        emit customTtsUrlChanged(m_customTtsUrl);
    }
}

QString TtsManager::customTtsVoice() const
{
    return m_customTtsVoice;
}

void TtsManager::setCustomTtsVoice(const QString &voice)
{
    if (voice != m_customTtsVoice) {
        m_customTtsVoice = voice.trimmed();
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/customTtsVoice", m_customTtsVoice);
        emit customTtsVoiceChanged(m_customTtsVoice);
    }
}

QString TtsManager::customApiKey() const
{
    return m_customApiKey;
}

void TtsManager::setCustomApiKey(const QString &key)
{
    if (key != m_customApiKey) {
        m_customApiKey = key.trimmed();
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/customApiKey", m_customApiKey);
        emit customApiKeyChanged(m_customApiKey);
    }
}

QString TtsManager::geminiApiKey() const
{
    return m_geminiApiKey;
}

void TtsManager::setGeminiApiKey(const QString &key)
{
    QString trimmed = key.trimmed();
    if (trimmed != m_geminiApiKey) {
        m_geminiApiKey = trimmed;
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/geminiApiKey", m_geminiApiKey);
        emit geminiApiKeyChanged(m_geminiApiKey);

        if (!m_geminiTestStatus.isEmpty()) {
            m_geminiTestStatus = "";
            m_geminiTestState = "idle";
            emit geminiTestStatusChanged(m_geminiTestStatus);
            emit geminiTestStateChanged(m_geminiTestState);
        }
    }
}

QString TtsManager::geminiTestStatus() const
{
    return m_geminiTestStatus;
}

QString TtsManager::geminiTestState() const
{
    return m_geminiTestState;
}

bool TtsManager::isTestingGemini() const
{
    return m_isTestingGemini;
}

QString TtsManager::geminiVoice() const
{
    return m_geminiVoice;
}

void TtsManager::setGeminiVoice(const QString &voice)
{
    QString v = voice.trimmed();
    if (v.isEmpty()) v = "Puck";
    if (v != m_geminiVoice) {
        m_geminiVoice = v;
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/geminiVoice", m_geminiVoice);
        emit geminiVoiceChanged(m_geminiVoice);
        emit ttsVoiceChanged(m_geminiVoice);
        if (m_playing && m_ttsProvider == "gemini") {
            playCurrentSentence();
        }
    }
}

QString TtsManager::geminiModel() const
{
    return m_geminiModel;
}

void TtsManager::setGeminiModel(const QString &model)
{
    QString m = model.trimmed();
    if (m.isEmpty()) m = "gemini-2.5-flash";
    if (m != m_geminiModel) {
        m_geminiModel = m;
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/geminiModel", m_geminiModel);
        emit geminiModelChanged(m_geminiModel);
    }
}

QString TtsManager::geminiStyle() const
{
    return m_geminiStyle;
}

void TtsManager::setGeminiStyle(const QString &style)
{
    QString s = style.trimmed().toLower();
    if (s.isEmpty()) s = "auto";
    if (s != m_geminiStyle) {
        m_geminiStyle = s;
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue("reader/geminiStyle", m_geminiStyle);
        emit geminiStyleChanged(m_geminiStyle);
        if (m_playing && m_ttsProvider == "gemini") {
            playCurrentSentence();
        }
    }
}

QString TtsManager::bookTitle() const
{
    return m_bookTitle;
}

QString TtsManager::bookAuthor() const
{
    return m_bookAuthor;
}

void TtsManager::setBookContext(const QString &title, const QString &author)
{
    QString t = title.trimmed();
    QString a = author.trimmed();
    if (t != m_bookTitle || a != m_bookAuthor) {
        m_bookTitle = t;
        m_bookAuthor = a;
        emit bookContextChanged(m_bookTitle, m_bookAuthor);
    }
}

void TtsManager::testVoice(const QString &sampleText)
{
    QString txt = sampleText;
    if (txt.isEmpty()) {
        QString l = m_language;
        if (l == "auto") l = m_detectedChapterLang;
        if (l == "vi") {
            txt = QString::fromUtf8("Xin chào, đây là giọng đọc sách trực tuyến của BerryReader.");
        } else if (l == "fr") {
            txt = QString::fromUtf8("Bonjour, ceci est la lecture vocale en ligne de BerryReader.");
        } else if (l == "de") {
            txt = QString::fromUtf8("Hallo, dies ist die Online-Sprachausgabe von BerryReader.");
        } else if (l == "es") {
            txt = QString::fromUtf8("Hola, esta es la lectura de voz en línea de BerryReader.");
        } else if (l == "ru") {
            txt = QString::fromUtf8("Здравствуйте, это онлайн голосовое чтение BerryReader.");
        } else if (l == "zh-CN") {
            txt = QString::fromUtf8("你好，这是 BerryReader 的在线语音朗读。");
        } else if (l == "ja") {
            txt = QString::fromUtf8("こんにちは、BerryReaderのオンライン音声読み上げです。");
        } else if (l == "ko") {
            txt = QString::fromUtf8("안녕하세요, BerryReader의 온라인 음성 읽기입니다.");
        } else {
            txt = QString::fromUtf8("Hello, this is the online text-to-speech voice of BerryReader.");
        }
    }
    startWithText(txt, m_language);
}

struct GoogleEndpointInfo {
    const char *host;
    const char *client;
    bool useHttp;
    const char *name;
};

static const GoogleEndpointInfo s_googleEndpoints[] = {
    { "translate.google.com", "tw-ob", false, "Web (tw-ob HTTPS)" },
    { "translate.google.com", "gtx", false, "Mobile (gtx HTTPS)" },
    { "translate.googleapis.com", "gtx", false, "Cloud APIs (HTTPS)" },
    { "translate.google.com", "webapp", false, "Webapp (HTTPS)" },
    { "translate.google.com", "dict-chrome-ex", false, "Dict Extension (HTTPS)" },
    { "translate.google.com", "tw-ob", true, "Web (tw-ob HTTP)" },
    { "translate.google.com", "gtx", true, "Mobile (gtx HTTP)" },
    { "translate.googleapis.com", "gtx", true, "Cloud APIs (HTTP)" }
};
static const int s_googleEndpointCount = sizeof(s_googleEndpoints) / sizeof(s_googleEndpoints[0]);

QUrl TtsManager::buildGoogleTtsUrl(const QString &text, const QString &lang, int endpointIdx) const
{
    int idx = qBound(0, endpointIdx, s_googleEndpointCount - 1);
    const GoogleEndpointInfo &ep = s_googleEndpoints[idx];
    QString scheme = ep.useHttp ? "http" : "https";
    QUrl url(QString("%1://%2/translate_tts").arg(scheme, QString::fromLatin1(ep.host)));
    url.addEncodedQueryItem("ie", "UTF-8");
    url.addEncodedQueryItem("client", QByteArray(ep.client));
    url.addEncodedQueryItem("tl", lang.toUtf8());
    url.addEncodedQueryItem("q", QUrl::toPercentEncoding(text));
    return url;
}

QUrl TtsManager::buildTtsUrl(const QString &text, const QString &lang, const QString &engine, bool useHttp) const
{
    Q_UNUSED(engine);
    return buildGoogleTtsUrl(text, lang, useHttp ? 5 : 0);
}

QNetworkRequest TtsManager::buildTtsRequest(const QUrl &url) const
{
    QNetworkRequest req(url);
    setupReplyBypassReq(req);
    req.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    req.setRawHeader("Referer", "http://translate.google.com/");
    return req;
}

QString TtsManager::detectLanguage(const QString &text) const
{
    if (text.isEmpty()) return "vi";

    int koreanCount = 0;
    int japaneseKanaCount = 0;
    int cjkCount = 0;
    int cyrillicCount = 0;
    int vietnameseCount = 0;

    const QString viChars = QString::fromUtf8("àáảãạăằắẳẵặâầấẩẫậèéẻẽẹêềếểễệìíỉĩịòóỏõọôồốổỗộơờớởỡợùúủũụưừứửữựỳýỷỹỵđ"
                                              "ÀÁẢÃẠĂẰẮẲẴẶÂẦẤẨẪẬÈÉẺẸÊỀẾỂỄỆÌÍỈĨỊÒÓỎÕỌÔỒỐỔỖỘƠỜỚỞỠỢÙÚỦŨỤƯỪỨỬỮỰỲÝỶỸỴĐ");

    for (int i = 0; i < text.length(); ++i) {
        ushort u = text[i].unicode();
        if ((u >= 0xAC00 && u <= 0xD7AF) || (u >= 0x1100 && u <= 0x11FF)) {
            koreanCount++;
        } else if ((u >= 0x3040 && u <= 0x309F) || (u >= 0x30A0 && u <= 0x30FF)) {
            japaneseKanaCount++;
        } else if ((u >= 0x4E00 && u <= 0x9FFF) || (u >= 0x3400 && u <= 0x4DBF)) {
            cjkCount++;
        } else if (u >= 0x0400 && u <= 0x04FF) {
            cyrillicCount++;
        } else if (viChars.contains(text[i])) {
            vietnameseCount++;
        }
    }

    if (koreanCount > 0) return "ko";
    if (japaneseKanaCount > 0) return "ja";
    if (cjkCount > 0 && cjkCount > text.length() * 0.1) return "zh-CN";
    if (cyrillicCount > 0 && cyrillicCount > text.length() * 0.1) return "ru";
    if (vietnameseCount > 0) return "vi";

    QString lower = text.toLower();
    QStringList words = lower.split(QRegExp("[\\s\\.,;:!\\?\\(\\)\"'\\[\\]«»—–-]+"), QString::SkipEmptyParts);
    int deMatches = 0;
    int frMatches = 0;
    int esMatches = 0;
    int enMatches = 0;

    QSet<QString> deSet; deSet << "der" << "die" << "das" << "und" << "in" << "zu" << "den" << "mit" << "von" << "nicht" << "sie" << "ist" << "sich";
    QSet<QString> frSet; frSet << "le" << "la" << "les" << "et" << "des" << "en" << "du" << "une" << "dans" << "pour" << "qui" << "sur" << "avec";
    QSet<QString> esSet; esSet << "el" << "la" << "los" << "las" << "de" << "en" << "y" << "un" << "una" << "que" << "por" << "con" << "para";
    QSet<QString> enSet; enSet << "the" << "and" << "to" << "of" << "a" << "in" << "that" << "it" << "is" << "was" << "for" << "with" << "as";

    for (int i = 0; i < words.size(); ++i) {
        const QString &w = words[i];
        if (deSet.contains(w)) deMatches++;
        if (frSet.contains(w)) frMatches++;
        if (esSet.contains(w)) esMatches++;
        if (enSet.contains(w)) enMatches++;
    }

    if (deMatches > 1 && deMatches >= frMatches && deMatches >= esMatches && deMatches >= enMatches) return "de";
    if (frMatches > 1 && frMatches >= deMatches && frMatches >= esMatches && frMatches >= enMatches) return "fr";
    if (esMatches > 1 && esMatches >= deMatches && esMatches >= frMatches && esMatches >= enMatches) return "es";
    if (enMatches > 1) return "en";

    return "vi";
}

static bool hasReadableWords(const QString &s)
{
    if (s.isEmpty()) return false;
    for (int i = 0; i < s.length(); ++i) {
        if (s[i].isLetter()) return true;
    }
    return false;
}

static void removeTagBlock(QString &str, const QString &startTag, const QString &endTag)
{
    while (true) {
        int start = str.indexOf(startTag, 0, Qt::CaseInsensitive);
        if (start == -1) break;
        int end = str.indexOf(endTag, start, Qt::CaseInsensitive);
        if (end != -1) {
            str.remove(start, (end + endTag.length()) - start);
        } else {
            int closeStart = str.indexOf('>', start);
            if (closeStart != -1) {
                str.remove(start, closeStart - start + 1);
            } else {
                str.remove(start, str.length() - start);
                break;
            }
        }
    }
}

static void appendChunkedSentences(QStringList &sentences, const QString &text)
{
    QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || !hasReadableWords(trimmed)) return;

    const int MAX_CHUNK_LEN = 130;
    while (trimmed.length() > MAX_CHUNK_LEN) {
        int splitPos = -1;
        for (int k = MAX_CHUNK_LEN; k >= 35; --k) {
            QChar ch = trimmed[k];
            if (ch == ',' || ch == ';' || ch == ':' || ch == ' ' ||
                ch == '.' || ch == '!' || ch == '?' || ch == QChar(0x2026)) {
                splitPos = k + 1;
                break;
            }
        }
        if (splitPos <= 0) {
            splitPos = trimmed.indexOf(' ', MAX_CHUNK_LEN);
            if (splitPos == -1 || splitPos > 160) {
                splitPos = MAX_CHUNK_LEN;
            }
        }
        QString chunk = trimmed.left(splitPos).trimmed();
        if (hasReadableWords(chunk)) {
            sentences.append(chunk);
        }
        trimmed = trimmed.mid(splitPos).trimmed();
    }
    if (!trimmed.isEmpty() && hasReadableWords(trimmed)) {
        sentences.append(trimmed);
    }
}

void TtsManager::parseSentences(const QString &rawText)
{
    m_sentences.clear();

    QString clean = rawText;

    removeTagBlock(clean, "<script", "</script>");
    removeTagBlock(clean, "<style", "</style>");
    removeTagBlock(clean, "<head", "</head>");
    removeTagBlock(clean, "<svg", "</svg>");
    removeTagBlock(clean, "<img", ">");
    removeTagBlock(clean, "<image", ">");

    clean.replace(QRegExp("<(p|div|br|h1|h2|h3|h4|h5|h6|li|tr|section|article|blockquote)[^>]*>", Qt::CaseInsensitive), "\n");
    clean.replace(QRegExp("<[^>]+>"), " ");
    clean.replace("&nbsp;", " ");
    clean.replace("&amp;", "&");
    clean.replace("&lt;", "<");
    clean.replace("&gt;", ">");
    clean.replace("&quot;", "\"");
    clean.replace("&#39;", "'");
    clean.replace("&apos;", "'");
    clean.replace(QRegExp("&[a-zA-Z0-9#]+;"), " ");

    QStringList lines = clean.split('\n', QString::SkipEmptyParts);
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty() || !hasReadableWords(line)) continue;

        int start = 0;
        for (int j = 0; j < line.length(); ++j) {
            QChar c = line[j];
            bool isBoundary = (c == '.' || c == '!' || c == '?' || c == ';' || c == QChar(0x2026));
            int len = j - start + 1;

            if (isBoundary && len >= 20) {
                QString s = line.mid(start, len).trimmed();
                appendChunkedSentences(m_sentences, s);
                start = j + 1;
            } else if (len >= 120 && c.isSpace()) {
                QString s = line.mid(start, len).trimmed();
                appendChunkedSentences(m_sentences, s);
                start = j + 1;
            }
        }
        if (start < line.length()) {
            QString s = line.mid(start).trimmed();
            appendChunkedSentences(m_sentences, s);
        }
    }
}

QString TtsManager::getCacheFilePath(int index, const QString &lang) const
{
    if (index < 0 || index >= m_sentences.size()) return QString();
    QString speedStr = QString::number(m_speed, 'f', 2);
    QString pitchStr = QString::number(m_pitch, 'f', 2);
    QString voiceKey = m_ttsVoice;
    if (m_ttsProvider == "gemini") {
        voiceKey = m_geminiVoice + ":" + m_geminiModel + ":" + m_geminiStyle;
    }
    QString key = m_ttsProvider + ":" + voiceKey + ":" + speedStr + ":" + pitchStr + ":" + lang + ":" + m_sentences[index].trimmed();
    QByteArray hash = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex();
    QString ext = (m_ttsProvider == "gemini") ? ".wav" : ".mp3";
    return m_cacheDir + "/" + hash + ext;
}

bool TtsManager::isSentenceCached(int index, const QString &lang) const
{
    QString path = getCacheFilePath(index, lang);
    if (path.isEmpty()) return false;
    QFileInfo fi(path);
    if (fi.exists() && fi.size() > 500) return true;
    QString altPath = path.endsWith(".wav") ? (path.left(path.length() - 4) + ".mp3") : (path.left(path.length() - 4) + ".wav");
    QFileInfo altFi(altPath);
    return altFi.exists() && altFi.size() > 500;
}

void TtsManager::startWithText(const QString &htmlOrText, const QString &lang, bool isChapter)
{
    stop();

    m_isChapterReading = isChapter;

    parseSentences(htmlOrText);
    if (m_sentences.isEmpty()) {
        qWarning() << "[TTS] No readable text found in content";
        return;
    }

    QString sampleText;
    int sampleLimit = qMin(m_sentences.size(), 20);
    for (int i = 0; i < sampleLimit; ++i) {
        sampleText += m_sentences[i] + " ";
        if (sampleText.length() > 2000) break;
    }
    m_detectedChapterLang = detectLanguage(sampleText);
    emit detectedLanguageChanged(m_detectedChapterLang);

    if (!lang.isEmpty()) {
        m_language = lang;
        emit languageChanged(m_language);
    }

    m_currentIndex = 0;
    m_active = true;
    emit activeChanged(true);
    emit sentencesChanged(m_sentences.size());
    emit currentSentenceChanged(m_currentIndex, m_sentences[m_currentIndex]);

    playCurrentSentence();
}

void TtsManager::play()
{
    if (!m_active) return;

    if (m_player->mediaState() == bb::multimedia::MediaState::Paused) {
        m_player->play();
        m_playing = true;
        emit playingChanged(true);
    } else {
        playCurrentSentence();
    }
}

void TtsManager::pause()
{
    if (m_player->mediaState() == bb::multimedia::MediaState::Started) {
        m_player->pause();
        m_playing = false;
        emit playingChanged(false);
    }
}

void TtsManager::togglePlayPause()
{
    if (m_playing) {
        pause();
    } else {
        play();
    }
}

void TtsManager::stop()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = NULL;
    }
    if (m_prefetchReply) {
        m_prefetchReply->abort();
        m_prefetchReply->deleteLater();
        m_prefetchReply = NULL;
    }
    if (m_batchCacheReply) {
        m_batchCacheReply->abort();
        m_batchCacheReply->deleteLater();
        m_batchCacheReply = NULL;
    }
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
        m_downloadReply = NULL;
    }

    m_player->stop();
    m_playing = false;
    m_loading = false;
    m_active = false;
    m_isCaching = false;

    emit playingChanged(false);
    emit loadingChanged(false);
    emit activeChanged(false);
    emit isCachingChanged(false);
}

void TtsManager::nextSentence()
{
    if (m_currentIndex + 1 < m_sentences.size()) {
        seekSentence(m_currentIndex + 1);
    } else {
        m_player->stop();
        m_playing = false;
        emit playingChanged(false);
        emit sentenceFinished(m_currentIndex);
        if (m_isChapterReading) {
            emit chapterFinished();
        }
    }
}

void TtsManager::prevSentence()
{
    if (m_currentIndex > 0) {
        seekSentence(m_currentIndex - 1);
    }
}

void TtsManager::seekSentence(int index)
{
    if (index >= 0 && index < m_sentences.size()) {
        m_currentIndex = index;
        emit currentSentenceChanged(m_currentIndex, m_sentences[m_currentIndex]);
        playCurrentSentence();
    }
}

void TtsManager::toggleSpeed()
{
    if (m_speed == 1.0) {
        setSpeed(1.25);
    } else if (m_speed == 1.25) {
        setSpeed(1.5);
    } else if (m_speed == 1.5) {
        setSpeed(0.8);
    } else {
        setSpeed(1.0);
    }
}

static QByteArray createWavHeader(int pcmDataSize, int sampleRate = 24000, int channels = 1, int bitsPerSample = 16)
{
    QByteArray header;
    header.resize(44);
    char *h = header.data();

    // RIFF chunk descriptor
    memcpy(h, "RIFF", 4);
    quint32 chunkSize = 36 + pcmDataSize;
    memcpy(h + 4, &chunkSize, 4);
    memcpy(h + 8, "WAVE", 4);

    // fmt sub-chunk
    memcpy(h + 12, "fmt ", 4);
    quint32 subchunk1Size = 16;
    quint16 audioFormat = 1; // PCM
    quint16 numChannels = channels;
    quint32 sRate = sampleRate;
    quint32 byteRate = sampleRate * channels * (bitsPerSample / 8);
    quint16 blockAlign = channels * (bitsPerSample / 8);
    quint16 bPerSample = bitsPerSample;

    memcpy(h + 16, &subchunk1Size, 4);
    memcpy(h + 20, &audioFormat, 2);
    memcpy(h + 22, &numChannels, 2);
    memcpy(h + 24, &sRate, 4);
    memcpy(h + 28, &byteRate, 4);
    memcpy(h + 32, &blockAlign, 2);
    memcpy(h + 34, &bPerSample, 2);

    // data sub-chunk
    memcpy(h + 36, "data", 4);
    quint32 subchunk2Size = pcmDataSize;
    memcpy(h + 40, &subchunk2Size, 4);

    return header;
}

static QString escapeJsonString(const QString &s)
{
    QString res;
    res.reserve(s.length() + 32);
    for (int i = 0; i < s.length(); ++i) {
        QChar c = s.at(i);
        if (c == '"') res += "\\\"";
        else if (c == '\\') res += "\\\\";
        else if (c == '\b') res += "\\b";
        else if (c == '\f') res += "\\f";
        else if (c == '\n') res += "\\n";
        else if (c == '\r') res += "\\r";
        else if (c == '\t') res += "\\t";
        else if (c.unicode() < 32) {
            res += QString("\\u%1").arg(c.unicode(), 4, 16, QChar('0'));
        } else {
            res += c;
        }
    }
    return res;
}

static bool parseGeminiAudioResponse(const QByteArray &jsonBytes, QByteArray &outAudioBytes, QString &outErrorMsg)
{
    QString jsonStr = QString::fromUtf8(jsonBytes);

    // 1. Check for error in JSON
    int errIdx = jsonStr.indexOf("\"error\"");
    if (errIdx != -1) {
        int msgIdx = jsonStr.indexOf("\"message\"", errIdx);
        if (msgIdx != -1) {
            int colon = jsonStr.indexOf(":", msgIdx);
            int q1 = jsonStr.indexOf("\"", colon);
            int q2 = jsonStr.indexOf("\"", q1 + 1);
            if (q1 != -1 && q2 != -1) {
                outErrorMsg = jsonStr.mid(q1 + 1, q2 - q1 - 1);
            }
        }
        if (outErrorMsg.isEmpty()) {
            outErrorMsg = QString::fromUtf8("Lỗi phản hồi từ Gemini API.");
        }
        return false;
    }

    // 2. Extract mimeType
    QString mimeType = "audio/mp3";
    int mimeIdx = jsonStr.indexOf("\"mimeType\"");
    if (mimeIdx != -1) {
        int colon = jsonStr.indexOf(":", mimeIdx);
        int q1 = jsonStr.indexOf("\"", colon);
        int q2 = jsonStr.indexOf("\"", q1 + 1);
        if (q1 != -1 && q2 != -1) {
            mimeType = jsonStr.mid(q1 + 1, q2 - q1 - 1).trimmed();
        }
    }

    // 3. Extract base64 data
    int dataIdx = jsonStr.indexOf("\"data\"");
    if (dataIdx == -1) {
        outErrorMsg = QString::fromUtf8("Phản hồi từ Gemini không chứa dữ liệu âm thanh.");
        return false;
    }

    int colon = jsonStr.indexOf(":", dataIdx);
    int q1 = jsonStr.indexOf("\"", colon);
    if (q1 == -1) {
        outErrorMsg = QString::fromUtf8("Dữ liệu âm thanh Base64 không đúng định dạng.");
        return false;
    }

    int q2 = jsonStr.indexOf("\"", q1 + 1);
    if (q2 == -1) {
        outErrorMsg = QString::fromUtf8("Dữ liệu âm thanh Base64 bị cắt ngắn.");
        return false;
    }

    QString b64 = jsonStr.mid(q1 + 1, q2 - q1 - 1);
    QByteArray rawAudio = QByteArray::fromBase64(b64.toLatin1());
    if (rawAudio.size() < 100) {
        outErrorMsg = QString::fromUtf8("Dữ liệu âm thanh nhận từ Gemini quá ngắn.");
        return false;
    }

    // 4. Container check: RIFF WAV or ID3/MP3 header
    bool isWav = (rawAudio.size() >= 4 && memcmp(rawAudio.constData(), "RIFF", 4) == 0);
    bool isMp3 = false;
    if (rawAudio.size() >= 3) {
        const unsigned char *d = (const unsigned char*)rawAudio.constData();
        if (d[0] == 'I' && d[1] == 'D' && d[2] == '3') isMp3 = true;
        else if (d[0] == 0xFF && (d[1] & 0xE0) == 0xE0) isMp3 = true;
    }

    if (isWav || isMp3) {
        outAudioBytes = rawAudio;
    } else {
        // Raw PCM from Gemini: default sample rate is 24000Hz 16-bit Mono
        int sRate = 24000;
        if (mimeType.contains("rate=")) {
            int rIdx = mimeType.indexOf("rate=") + 5;
            int rEnd = mimeType.indexOf(";", rIdx);
            if (rEnd == -1) rEnd = mimeType.indexOf(" ", rIdx);
            if (rEnd == -1) rEnd = mimeType.length();
            int parsedRate = mimeType.mid(rIdx, rEnd - rIdx).toInt();
            if (parsedRate > 0) sRate = parsedRate;
        }
        outAudioBytes = createWavHeader(rawAudio.size(), sRate, 1, 16) + rawAudio;
    }

    return true;
}

QNetworkReply* TtsManager::sendProviderRequest(const QString &text, const QString &lang, int index, const QString &mode, int googleEndpointIdx)
{
    if (m_ttsProvider == "gemini") {
        if (m_geminiApiKey.trimmed().isEmpty()) {
            emit errorOccurred(QString::fromUtf8("Vui lòng nhập Gemini API Key trong Cài đặt Giọng đọc (lấy miễn phí tại aistudio.google.com)."));
            return NULL;
        }

        QString model = m_geminiModel.trimmed().isEmpty() ? "gemini-2.5-flash" : m_geminiModel.trimmed();
        QUrl url(QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2")
                 .arg(model)
                 .arg(m_geminiApiKey.trimmed()));

        QNetworkRequest req(url);
        setupReplyBypassReq(req);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QString voice = m_geminiVoice.trimmed().isEmpty() ? "Puck" : m_geminiVoice.trimmed();
        QString escapedPromptText = escapeJsonString(text);

        QString jsonBody = QString(
            "{\"contents\":[{\"role\":\"user\",\"parts\":[{\"text\":\"%1\"}]}],"
            "\"generationConfig\":{\"responseModalities\":[\"AUDIO\"],\"speechConfig\":{\"voiceConfig\":{\"prebuiltVoiceConfig\":{\"voiceName\":\"%2\"}}}}}"
        ).arg(escapedPromptText).arg(voice);

        QNetworkReply *reply = m_nam->post(req, jsonBody.toUtf8());
        setupReplyBypass(reply);
        reply->setProperty("index", index);
        reply->setProperty("lang", lang);
        reply->setProperty("provider", "gemini");
        reply->setProperty("mode", mode);
        return reply;
    } else {
        // google (auto-failover across 8 fallback endpoints)
        int epIdx = (googleEndpointIdx >= 0) ? googleEndpointIdx : m_googleEndpointIndex;
        epIdx = qBound(0, epIdx, s_googleEndpointCount - 1);
        QUrl url = buildGoogleTtsUrl(text, lang, epIdx);
        QNetworkRequest req = buildTtsRequest(url);
        QNetworkReply *reply = m_nam->get(req);
        setupReplyBypass(reply);
        reply->setProperty("index", index);
        reply->setProperty("lang", lang);
        reply->setProperty("provider", "google");
        reply->setProperty("googleEndpointIdx", epIdx);
        reply->setProperty("mode", mode);
        return reply;
    }
}

void TtsManager::handleAudioData(int index, const QString &lang, const QString &mode, const QByteArray &data)
{
    if (data.size() > 200) {
        QString path = getCacheFilePath(index, lang);
        if (data.size() >= 4 && memcmp(data.constData(), "RIFF", 4) == 0 && path.endsWith(".mp3")) {
            path = path.left(path.length() - 4) + ".wav";
        }
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
            file.close();
        }

        m_isOffline = false;
        emit isOfflineChanged(false);

        if (mode == "play" && index == m_currentIndex) {
            m_loading = false;
            emit loadingChanged(false);

            m_player->stop();
            m_player->setSourceUrl(QUrl::fromLocalFile(path));
            m_player->setSpeed(m_speed);
            applyPitchToPlayer();
            m_player->play();

            m_playing = true;
            emit playingChanged(true);

            prefetchSentence(m_currentIndex + 1);
        } else if (mode == "batch") {
            m_cachingIndex++;
            emit cacheProgressChanged(m_cachingIndex, m_sentences.size());
            processNextBatchCache();
        }
    } else {
        if (mode == "play" && index == m_currentIndex) {
            m_loading = false;
            emit loadingChanged(false);
            m_playing = false;
            emit playingChanged(false);
            emit errorOccurred(QString::fromUtf8("Dữ liệu âm thanh nhận về không hợp lệ. Hãy kiểm tra kết nối mạng hoặc API Key."));
        } else if (mode == "batch") {
            m_cachingIndex++;
            emit cacheProgressChanged(m_cachingIndex, m_sentences.size());
            processNextBatchCache();
        }
    }
}

void TtsManager::downloadAudioUrl(const QUrl &url, int index, const QString &lang, const QString &mode)
{
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
        m_downloadReply = NULL;
    }

    QNetworkRequest req(url);
    setupReplyBypassReq(req);
    req.setRawHeader("User-Agent", "Mozilla/5.0 BerryReader/1.0 (BlackBerry 10)");

    m_downloadReply = m_nam->get(req);
    setupReplyBypass(m_downloadReply);
    m_downloadReply->setProperty("index", index);
    m_downloadReply->setProperty("lang", lang);
    m_downloadReply->setProperty("mode", mode);
    connect(m_downloadReply, SIGNAL(finished()), this, SLOT(onDownloadReplyFinished()));
}

void TtsManager::onDownloadReplyFinished()
{
    if (!m_downloadReply) return;

    QNetworkReply *reply = m_downloadReply;
    m_downloadReply = NULL;
    reply->deleteLater();

    int idx = reply->property("index").toInt();
    QString lang = reply->property("lang").toString();
    QString mode = reply->property("mode").toString();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        handleAudioData(idx, lang, mode, data);
    } else {
        if (mode == "play" && idx == m_currentIndex) {
            m_loading = false;
            emit loadingChanged(false);
            m_playing = false;
            emit playingChanged(false);
            emit errorOccurred(QString::fromUtf8("Không thể tải file âm thanh từ máy chủ: ") + reply->errorString());
        } else if (mode == "batch") {
            m_cachingIndex++;
            emit cacheProgressChanged(m_cachingIndex, m_sentences.size());
            processNextBatchCache();
        }
    }
}

void TtsManager::playCurrentSentence()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_sentences.size()) return;

    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = NULL;
    }

    m_player->stop();

    QString effLang = effectiveLanguageForSentence(m_currentIndex);
    QString cachedPath = getCacheFilePath(m_currentIndex, effLang);

    // 1. Check local cache (Offline first!)
    if (isSentenceCached(m_currentIndex, effLang)) {
        m_loading = false;
        emit loadingChanged(false);

        // Ensure we point to the file that actually exists (.wav or .mp3)
        if (!QFile::exists(cachedPath)) {
            QString altPath = cachedPath.endsWith(".wav") ? (cachedPath.left(cachedPath.length() - 4) + ".mp3") : (cachedPath.left(cachedPath.length() - 4) + ".wav");
            if (QFile::exists(altPath)) {
                cachedPath = altPath;
            }
        }

        m_player->setSourceUrl(QUrl::fromLocalFile(cachedPath));
        m_player->setSpeed(m_speed);
        applyPitchToPlayer();
        m_player->play();

        m_playing = true;
        emit playingChanged(true);

        if (m_ttsMode != "offline") {
            prefetchSentence(m_currentIndex + 1);
        }
        return;
    }

    // 2. Offline Mode check
    if (m_ttsMode == "offline") {
        m_loading = false;
        m_playing = false;
        emit loadingChanged(false);
        emit playingChanged(false);
        emit errorOccurred(QString::fromUtf8("Chế độ Ngoại tuyến: Câu này chưa có trong bộ nhớ máy. Hãy bấm nút 'Tải về' để lưu trước toàn bộ chương."));
        return;
    }

    // 3. Online fetch directly (bypass BB10 Qt bearer management plugin quirks)
    m_loading = true;
    emit loadingChanged(true);

    QString text = m_sentences[m_currentIndex];
    m_currentReply = sendProviderRequest(text, effLang, m_currentIndex, "play");
    if (!m_currentReply) {
        m_loading = false;
        emit loadingChanged(false);
        m_playing = false;
        emit playingChanged(false);
        return;
    }
    connect(m_currentReply, SIGNAL(finished()), this, SLOT(onNetworkReplyFinished()));
}

void TtsManager::onNetworkReplyFinished()
{
    if (!m_currentReply) return;

    QNetworkReply *reply = m_currentReply;
    m_currentReply = NULL;
    reply->deleteLater();

    int idx = reply->property("index").toInt();
    QString lang = reply->property("lang").toString();
    QString provider = reply->property("provider").toString();
    QString mode = reply->property("mode").toString();
    if (lang.isEmpty()) lang = effectiveLanguageForSentence(idx);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        if (provider == "google") {
            // Verify valid audio MP3 data (size > 300 and not HTML captcha / error page)
            if (data.size() > 300 && !data.startsWith("<html") && !data.startsWith("<!DOCTYPE") && !data.startsWith("<HTML")) {
                int successfulEp = reply->property("googleEndpointIdx").toInt();
                m_googleEndpointIndex = successfulEp;
                handleAudioData(idx, lang, mode, data);
                return;
            } else {
                qWarning() << "[TtsManager] Google returned non-audio response (size:" << data.size()
                           << ") from endpoint" << reply->property("googleEndpointIdx").toInt()
                           << "-> Triggering auto-failover";
            }
        } else if (provider == "gemini") {
            QByteArray audioBytes;
            QString errorMsg;
            if (parseGeminiAudioResponse(data, audioBytes, errorMsg)) {
                handleAudioData(idx, lang, mode, audioBytes);
                return;
            } else {
                qWarning() << "[TtsManager] Gemini parse error:" << errorMsg;
                m_loading = false;
                emit loadingChanged(false);
                m_playing = false;
                emit playingChanged(false);
                emit errorOccurred(QString::fromUtf8("Lỗi Gemini: ") + errorMsg);
                return;
            }
        } else {
            handleAudioData(idx, lang, mode, data);
            return;
        }
    } else {
        qWarning() << "[TtsManager] Network error:" << reply->error() << reply->errorString()
                   << "from provider" << provider << "endpoint" << reply->property("googleEndpointIdx").toInt();
        if (provider == "gemini") {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray errBody = reply->readAll();
            QString errorMsg;
            QByteArray dummy;
            parseGeminiAudioResponse(errBody, dummy, errorMsg);
            if (statusCode == 429) {
                errorMsg = QString::fromUtf8("Đã vượt quá giới hạn lượt gọi (Rate Limit) của Gemini API. Vui lòng chờ vài giây rồi thử lại.");
            } else if (errorMsg.isEmpty()) {
                errorMsg = reply->errorString();
            }
            m_loading = false;
            emit loadingChanged(false);
            m_playing = false;
            emit playingChanged(false);
            emit errorOccurred(QString::fromUtf8("Lỗi Gemini API (%1): %2").arg(statusCode).arg(errorMsg));
            return;
        }
    }

    // Auto-failover for Google TTS to next endpoint
    if (provider == "google") {
        int currentEp = reply->property("googleEndpointIdx").toInt();
        int nextEp = currentEp + 1;
        if (nextEp < s_googleEndpointCount && idx == m_currentIndex) {
            qDebug() << "[TtsManager] Google auto-failover: switching from" << s_googleEndpoints[currentEp].name
                     << "to" << s_googleEndpoints[nextEp].name << "for sentence" << idx;
            QString text = m_sentences[idx];
            m_currentReply = sendProviderRequest(text, lang, idx, mode, nextEp);
            if (m_currentReply) {
                connect(m_currentReply, SIGNAL(finished()), this, SLOT(onNetworkReplyFinished()));
                m_loading = true;
                emit loadingChanged(true);
                return;
            }
        }
    }

    m_loading = false;
    emit loadingChanged(false);
    m_isOffline = true;
    emit isOfflineChanged(true);
    emit errorOccurred(QString::fromUtf8("Lỗi kết nối khi tải giọng đọc. Hãy kiểm tra kết nối mạng hoặc máy chủ TTS."));
    m_playing = false;
    emit playingChanged(false);
}

void TtsManager::prefetchSentence(int index)
{
    if (index < 0 || index >= m_sentences.size()) return;
    QString effLang = effectiveLanguageForSentence(index);
    if (isSentenceCached(index, effLang)) return;
    if (m_prefetchReply) return;

    QString text = m_sentences[index];
    m_prefetchReply = sendProviderRequest(text, effLang, index, "prefetch");
    if (m_prefetchReply) {
        connect(m_prefetchReply, SIGNAL(finished()), this, SLOT(onPrefetchReplyFinished()));
    }
}

void TtsManager::onPrefetchReplyFinished()
{
    if (!m_prefetchReply) return;

    QNetworkReply *reply = m_prefetchReply;
    m_prefetchReply = NULL;
    reply->deleteLater();

    int idx = reply->property("index").toInt();
    QString lang = reply->property("lang").toString();
    QString provider = reply->property("provider").toString();
    if (lang.isEmpty()) lang = effectiveLanguageForSentence(idx);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        if (provider == "google") {
            if (data.size() > 300 && !data.startsWith("<html") && !data.startsWith("<!DOCTYPE") && !data.startsWith("<HTML")) {
                int successfulEp = reply->property("googleEndpointIdx").toInt();
                m_googleEndpointIndex = successfulEp;
                handleAudioData(idx, lang, "prefetch", data);
                return;
            }
        } else if (provider == "gemini") {
            QByteArray audioBytes;
            QString errorMsg;
            if (parseGeminiAudioResponse(data, audioBytes, errorMsg)) {
                handleAudioData(idx, lang, "prefetch", audioBytes);
                return;
            }
        } else {
            handleAudioData(idx, lang, "prefetch", data);
            return;
        }
    }

    if (provider == "google") {
        int currentEp = reply->property("googleEndpointIdx").toInt();
        int nextEp = currentEp + 1;
        if (nextEp < s_googleEndpointCount && idx < m_sentences.size()) {
            QString text = m_sentences[idx];
            m_prefetchReply = sendProviderRequest(text, lang, idx, "prefetch", nextEp);
            if (m_prefetchReply) {
                connect(m_prefetchReply, SIGNAL(finished()), this, SLOT(onPrefetchReplyFinished()));
            }
        }
    }
}

void TtsManager::preCacheChapter()
{
    if (m_sentences.isEmpty() || m_isCaching) return;

    m_isCaching = true;
    m_cachingIndex = 0;
    emit isCachingChanged(true);
    emit cacheProgressChanged(m_cachingIndex, m_sentences.size());

    processNextBatchCache();
}

void TtsManager::processNextBatchCache()
{
    if (!m_isCaching) return;

    while (m_cachingIndex < m_sentences.size()) {
        QString effLang = effectiveLanguageForSentence(m_cachingIndex);
        if (!isSentenceCached(m_cachingIndex, effLang)) {
            break;
        }
        m_cachingIndex++;
        emit cacheProgressChanged(m_cachingIndex, m_sentences.size());
    }

    if (m_cachingIndex >= m_sentences.size()) {
        m_isCaching = false;
        emit isCachingChanged(false);
        emit cacheProgressChanged(m_sentences.size(), m_sentences.size());
        return;
    }

    QString effLang = effectiveLanguageForSentence(m_cachingIndex);
    QString text = m_sentences[m_cachingIndex];
    m_batchCacheReply = sendProviderRequest(text, effLang, m_cachingIndex, "batch");
    if (m_batchCacheReply) {
        connect(m_batchCacheReply, SIGNAL(finished()), this, SLOT(onBatchCacheReplyFinished()));
    } else {
        m_isCaching = false;
        emit isCachingChanged(false);
    }
}

void TtsManager::onBatchCacheReplyFinished()
{
    if (!m_batchCacheReply) return;

    QNetworkReply *reply = m_batchCacheReply;
    m_batchCacheReply = NULL;
    reply->deleteLater();

    int idx = reply->property("index").toInt();
    QString lang = reply->property("lang").toString();
    QString provider = reply->property("provider").toString();
    if (lang.isEmpty()) lang = effectiveLanguageForSentence(idx);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        if (provider == "google") {
            if (data.size() > 300 && !data.startsWith("<html") && !data.startsWith("<!DOCTYPE") && !data.startsWith("<HTML")) {
                int successfulEp = reply->property("googleEndpointIdx").toInt();
                m_googleEndpointIndex = successfulEp;
                handleAudioData(idx, lang, "batch", data);
                return;
            }
        } else if (provider == "gemini") {
            QByteArray audioBytes;
            QString errorMsg;
            if (parseGeminiAudioResponse(data, audioBytes, errorMsg)) {
                handleAudioData(idx, lang, "batch", audioBytes);
                return;
            }
        } else {
            handleAudioData(idx, lang, "batch", data);
            return;
        }
    }

    // Auto-failover for batch cache
    if (provider == "google") {
        int currentEp = reply->property("googleEndpointIdx").toInt();
        int nextEp = currentEp + 1;
        if (nextEp < s_googleEndpointCount && m_isCaching && idx == m_cachingIndex) {
            QString text = m_sentences[idx];
            m_batchCacheReply = sendProviderRequest(text, lang, idx, "batch", nextEp);
            if (m_batchCacheReply) {
                connect(m_batchCacheReply, SIGNAL(finished()), this, SLOT(onBatchCacheReplyFinished()));
                return;
            }
        }
    }

    m_cachingIndex++;
    emit cacheProgressChanged(m_cachingIndex, m_sentences.size());

    if (m_isCaching && m_cachingIndex < m_sentences.size()) {
        processNextBatchCache();
    } else {
        m_isCaching = false;
        emit isCachingChanged(false);
    }
}

void TtsManager::clearCache()
{
    QDir dir(m_cacheDir);
    QStringList files = dir.entryList(QStringList() << "*.mp3" << "*.wav", QDir::Files);
    for (int i = 0; i < files.size(); ++i) {
        dir.remove(files[i]);
    }
}

void TtsManager::testGeminiApiKey(const QString &keyToTest)
{
    if (m_geminiTestReply) {
        m_geminiTestReply->abort();
        m_geminiTestReply->deleteLater();
        m_geminiTestReply = NULL;
    }

    QString key = keyToTest.trimmed();
    if (key.isEmpty()) {
        key = m_geminiApiKey.trimmed();
    }

    if (key.isEmpty()) {
        m_isTestingGemini = false;
        m_geminiTestState = "error";
        m_geminiTestStatus = tr("Vui lòng nhập Google AI Studio API Key trước khi kiểm tra.");
        emit isTestingGeminiChanged(false);
        emit geminiTestStateChanged(m_geminiTestState);
        emit geminiTestStatusChanged(m_geminiTestStatus);
        return;
    }

    if (key != m_geminiApiKey) {
        setGeminiApiKey(key);
    }

    if (m_playing) {
        stop();
    }

    m_isTestingGemini = true;
    m_geminiTestState = "testing";
    m_geminiTestStatus = tr("Đang kết nối và kiểm tra API Key với Google AI Studio...");
    emit isTestingGeminiChanged(true);
    emit geminiTestStateChanged(m_geminiTestState);
    emit geminiTestStatusChanged(m_geminiTestStatus);

    QString model = m_geminiModel.trimmed().isEmpty() ? "gemini-2.5-flash" : m_geminiModel.trimmed();
    QString voice = m_geminiVoice.trimmed().isEmpty() ? "Puck" : m_geminiVoice.trimmed();

    QUrl url(QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2")
             .arg(model)
             .arg(key));

    QNetworkRequest req(url);
    setupReplyBypassReq(req);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QString testPhrase = QString::fromUtf8("BerryReader xin chào! Kết nối API thành công.");
    QString escapedPhrase = escapeJsonString(testPhrase);

    QString jsonBody = QString(
        "{\"contents\":[{\"role\":\"user\",\"parts\":[{\"text\":\"%1\"}]}],"
        "\"generationConfig\":{\"responseModalities\":[\"AUDIO\"],\"speechConfig\":{\"voiceConfig\":{\"prebuiltVoiceConfig\":{\"voiceName\":\"%2\"}}}}}"
    ).arg(escapedPhrase).arg(voice);

    m_geminiTestTime.start();
    m_geminiTestReply = m_nam->post(req, jsonBody.toUtf8());
    setupReplyBypass(m_geminiTestReply);
    m_geminiTestReply->setProperty("model", model);
    m_geminiTestReply->setProperty("key", key);

    connect(m_geminiTestReply, SIGNAL(finished()), this, SLOT(onGeminiTestReplyFinished()));
}

void TtsManager::cancelGeminiTest()
{
    if (m_geminiTestReply) {
        m_geminiTestReply->abort();
        m_geminiTestReply->deleteLater();
        m_geminiTestReply = NULL;
    }
    if (m_isTestingGemini) {
        m_isTestingGemini = false;
        m_geminiTestState = "idle";
        m_geminiTestStatus = tr("Đã hủy kiểm tra.");
        emit isTestingGeminiChanged(false);
        emit geminiTestStateChanged(m_geminiTestState);
        emit geminiTestStatusChanged(m_geminiTestStatus);
    }
}

void TtsManager::onGeminiTestReplyFinished()
{
    if (!m_geminiTestReply) return;

    QNetworkReply *reply = m_geminiTestReply;
    m_geminiTestReply = NULL;
    reply->deleteLater();

    if (reply->error() == QNetworkReply::OperationCanceledError) {
        return;
    }

    int elapsedMs = m_geminiTestTime.elapsed();
    m_isTestingGemini = false;
    emit isTestingGeminiChanged(false);

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    QString modelName = reply->property("model").toString();

    if (reply->error() == QNetworkReply::NoError && (statusCode == 200 || statusCode == 0)) {
        QByteArray audioBytes;
        QString parseErr;
        if (parseGeminiAudioResponse(data, audioBytes, parseErr)) {
            QString ext = (audioBytes.size() >= 4 && memcmp(audioBytes.constData(), "RIFF", 4) == 0) ? ".wav" : ".mp3";
            QString testSamplePath = m_cacheDir + "/gemini_test_sample" + ext;
            QFile sampleFile(testSamplePath);
            if (sampleFile.open(QIODevice::WriteOnly)) {
                sampleFile.write(audioBytes);
                sampleFile.close();
            }

            m_player->stop();
            m_player->setSourceUrl(QUrl::fromLocalFile(testSamplePath));
            m_player->setSpeed(1.0);
            applyPitchToPlayer();
            m_player->play();

            m_geminiTestState = "success";
            m_geminiTestStatus = tr("✓ Kết nối thành công (200 OK - %1 ms)! API Key hợp lệ và đang phát thử giọng đọc.").arg(elapsedMs);
            emit geminiTestStateChanged(m_geminiTestState);
            emit geminiTestStatusChanged(m_geminiTestStatus);
            return;
        } else {
            if (data.contains("candidates") || data.contains("content")) {
                m_geminiTestState = "success";
                m_geminiTestStatus = tr("✓ Kết nối thành công (200 OK - %1 ms)! API Key hợp lệ (Mô hình '%2' phản hồi dạng văn bản).").arg(elapsedMs).arg(modelName);
                emit geminiTestStateChanged(m_geminiTestState);
                emit geminiTestStatusChanged(m_geminiTestStatus);
                return;
            } else {
                m_geminiTestState = "error";
                m_geminiTestStatus = tr("✗ Phản hồi không có âm thanh: %1").arg(parseErr);
                emit geminiTestStateChanged(m_geminiTestState);
                emit geminiTestStatusChanged(m_geminiTestStatus);
                return;
            }
        }
    } else {
        QByteArray dummy;
        QString apiErrorMsg;
        parseGeminiAudioResponse(data, dummy, apiErrorMsg);
        if (apiErrorMsg.isEmpty()) {
            apiErrorMsg = reply->errorString();
        }

        m_geminiTestState = "error";
        if (statusCode == 400) {
            m_geminiTestStatus = tr("✗ Lỗi 400 (Bad Request): API Key không hợp lệ hoặc model không hỗ trợ.\nChi tiết: %1").arg(apiErrorMsg);
        } else if (statusCode == 403) {
            m_geminiTestStatus = tr("✗ Lỗi 403 (Forbidden): API Key bị từ chối truy cập hoặc chưa được cấp quyền.\nChi tiết: %1").arg(apiErrorMsg);
        } else if (statusCode == 404) {
            m_geminiTestStatus = tr("✗ Lỗi 404 (Not Found): Mô hình '%1' không tồn tại trên API v1beta.\nChi tiết: %2").arg(modelName).arg(apiErrorMsg);
        } else if (statusCode == 429) {
            m_geminiTestStatus = tr("✗ Lỗi 429 (Rate Limit): Đã vượt quá hạn mức gọi (Quota / Rate limit). Vui lòng chờ 1 lát rồi thử lại.");
        } else if (statusCode > 0) {
            m_geminiTestStatus = tr("✗ Lỗi HTTP %1: %2").arg(statusCode).arg(apiErrorMsg);
        } else {
            m_geminiTestStatus = tr("✗ Lỗi kết nối: %1 (Vui lòng kiểm tra kết nối mạng Wi-Fi / 3G)").arg(apiErrorMsg);
        }

        emit geminiTestStateChanged(m_geminiTestState);
        emit geminiTestStatusChanged(m_geminiTestStatus);
    }
}

void TtsManager::onPlaybackCompleted()
{
    if (m_active) {
        nextSentence();
    }
}

void TtsManager::onMediaStateChanged(bb::multimedia::MediaState::Type state)
{
    if (state == bb::multimedia::MediaState::Started) {
        if (!m_playing) {
            m_playing = true;
            emit playingChanged(true);
        }
    } else if (state == bb::multimedia::MediaState::Paused) {
        if (m_playing) {
            m_playing = false;
            emit playingChanged(false);
        }
    } else if (state == bb::multimedia::MediaState::Stopped) {
        if (!m_active && m_playing) {
            m_playing = false;
            emit playingChanged(false);
        }
    }
}
