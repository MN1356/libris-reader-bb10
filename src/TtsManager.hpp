#ifndef TTSMANAGER_HPP
#define TTSMANAGER_HPP

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QTime>
#include <bb/multimedia/MediaPlayer>

class TtsManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int currentSentenceIndex READ currentSentenceIndex NOTIFY currentSentenceChanged)
    Q_PROPERTY(int totalSentences READ totalSentences NOTIFY sentencesChanged)
    Q_PROPERTY(QString currentSentenceText READ currentSentenceText NOTIFY currentSentenceChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(double pitch READ pitch WRITE setPitch NOTIFY pitchChanged)
    Q_PROPERTY(QString pitchDisplayName READ pitchDisplayName NOTIFY pitchChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString detectedLanguage READ detectedLanguage NOTIFY detectedLanguageChanged)
    Q_PROPERTY(bool isOffline READ isOffline NOTIFY isOfflineChanged)
    Q_PROPERTY(QString ttsMode READ ttsMode WRITE setTtsMode NOTIFY ttsModeChanged)
    Q_PROPERTY(int cacheProgress READ cacheProgress NOTIFY cacheProgressChanged)
    Q_PROPERTY(bool isCaching READ isCaching NOTIFY isCachingChanged)
    Q_PROPERTY(QString ttsEngine READ ttsEngine WRITE setTtsEngine NOTIFY ttsEngineChanged)
    Q_PROPERTY(QString ttsProvider READ ttsProvider WRITE setTtsProvider NOTIFY ttsProviderChanged)
    Q_PROPERTY(QString ttsVoice READ ttsVoice WRITE setTtsVoice NOTIFY ttsVoiceChanged)
    Q_PROPERTY(QString voiceDisplayName READ voiceDisplayName NOTIFY ttsVoiceChanged)
    Q_PROPERTY(QString zaloApiKey READ zaloApiKey WRITE setZaloApiKey NOTIFY zaloApiKeyChanged)
    Q_PROPERTY(QString fptApiKey READ fptApiKey WRITE setFptApiKey NOTIFY fptApiKeyChanged)
    Q_PROPERTY(QString voiceRssApiKey READ voiceRssApiKey WRITE setVoiceRssApiKey NOTIFY voiceRssApiKeyChanged)
    Q_PROPERTY(QString customTtsUrl READ customTtsUrl WRITE setCustomTtsUrl NOTIFY customTtsUrlChanged)
    Q_PROPERTY(QString customTtsVoice READ customTtsVoice WRITE setCustomTtsVoice NOTIFY customTtsVoiceChanged)
    Q_PROPERTY(QString customApiKey READ customApiKey WRITE setCustomApiKey NOTIFY customApiKeyChanged)
    Q_PROPERTY(QString geminiApiKey READ geminiApiKey WRITE setGeminiApiKey NOTIFY geminiApiKeyChanged)
    Q_PROPERTY(QString geminiVoice READ geminiVoice WRITE setGeminiVoice NOTIFY geminiVoiceChanged)
    Q_PROPERTY(QString geminiModel READ geminiModel WRITE setGeminiModel NOTIFY geminiModelChanged)
    Q_PROPERTY(QString geminiStyle READ geminiStyle WRITE setGeminiStyle NOTIFY geminiStyleChanged)
    Q_PROPERTY(QString geminiTestStatus READ geminiTestStatus NOTIFY geminiTestStatusChanged)
    Q_PROPERTY(QString geminiTestState READ geminiTestState NOTIFY geminiTestStateChanged)
    Q_PROPERTY(bool isTestingGemini READ isTestingGemini NOTIFY isTestingGeminiChanged)
    Q_PROPERTY(QString bookTitle READ bookTitle NOTIFY bookContextChanged)
    Q_PROPERTY(QString bookAuthor READ bookAuthor NOTIFY bookContextChanged)

public:
    explicit TtsManager(QObject *parent = 0);
    virtual ~TtsManager();

    bool isActive() const;
    bool isPlaying() const;
    bool isLoading() const;
    int currentSentenceIndex() const;
    int totalSentences() const;
    QString currentSentenceText() const;
    double speed() const;
    Q_INVOKABLE void setSpeed(double spd);
    double pitch() const;
    Q_INVOKABLE void setPitch(double p);
    Q_INVOKABLE void togglePitch();
    QString pitchDisplayName() const;
    QString language() const;
    Q_INVOKABLE void setLanguage(const QString &lang);
    QString detectedLanguage() const;
    QString effectiveLanguageForSentence(int index) const;
    Q_INVOKABLE void cycleLanguage();
    bool isOffline() const;
    QString ttsMode() const;
    Q_INVOKABLE void setTtsMode(const QString &mode);
    Q_INVOKABLE void toggleMode();
    int cacheProgress() const;
    bool isCaching() const;

    QString ttsProvider() const;
    Q_INVOKABLE void setTtsProvider(const QString &provider);
    Q_INVOKABLE void cycleProvider();

    QString ttsVoice() const;
    Q_INVOKABLE void setTtsVoice(const QString &voice);
    Q_INVOKABLE void cycleVoice();
    QString voiceDisplayName() const;

    QString ttsEngine() const;
    Q_INVOKABLE void setTtsEngine(const QString &engine);
    Q_INVOKABLE void cycleEngine();

    QString zaloApiKey() const;
    Q_INVOKABLE void setZaloApiKey(const QString &key);

    QString fptApiKey() const;
    Q_INVOKABLE void setFptApiKey(const QString &key);

    QString voiceRssApiKey() const;
    Q_INVOKABLE void setVoiceRssApiKey(const QString &key);

    QString customTtsUrl() const;
    Q_INVOKABLE void setCustomTtsUrl(const QString &url);

    QString customTtsVoice() const;
    Q_INVOKABLE void setCustomTtsVoice(const QString &voice);

    QString customApiKey() const;
    Q_INVOKABLE void setCustomApiKey(const QString &key);

    QString geminiApiKey() const;
    Q_INVOKABLE void setGeminiApiKey(const QString &key);

    QString geminiVoice() const;
    Q_INVOKABLE void setGeminiVoice(const QString &voice);

    QString geminiModel() const;
    Q_INVOKABLE void setGeminiModel(const QString &model);

    QString geminiStyle() const;
    Q_INVOKABLE void setGeminiStyle(const QString &style);

    QString geminiTestStatus() const;
    QString geminiTestState() const;
    bool isTestingGemini() const;
    Q_INVOKABLE void testGeminiApiKey(const QString &key = QString());
    Q_INVOKABLE void cancelGeminiTest();

    QString bookTitle() const;
    QString bookAuthor() const;
    Q_INVOKABLE void setBookContext(const QString &title, const QString &author);

    Q_INVOKABLE void testVoice(const QString &sampleText = QString());

    Q_INVOKABLE void startWithText(const QString &htmlOrText, const QString &lang = QString(), bool isChapter = false);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void nextSentence();
    Q_INVOKABLE void prevSentence();
    Q_INVOKABLE void seekSentence(int index);
    Q_INVOKABLE void toggleSpeed();
    Q_INVOKABLE void preCacheChapter();
    Q_INVOKABLE void clearCache();

signals:
    void activeChanged(bool active);
    void playingChanged(bool playing);
    void loadingChanged(bool loading);
    void currentSentenceChanged(int index, const QString &text);
    void sentencesChanged(int total);
    void speedChanged(double speed);
    void pitchChanged(double pitch);
    void languageChanged(const QString &lang);
    void ttsEngineChanged(const QString &engine);
    void ttsProviderChanged(const QString &provider);
    void ttsVoiceChanged(const QString &voice);
    void zaloApiKeyChanged(const QString &key);
    void fptApiKeyChanged(const QString &key);
    void voiceRssApiKeyChanged(const QString &key);
    void customTtsUrlChanged(const QString &url);
    void customTtsVoiceChanged(const QString &voice);
    void customApiKeyChanged(const QString &key);
    void geminiApiKeyChanged(const QString &key);
    void geminiVoiceChanged(const QString &voice);
    void geminiModelChanged(const QString &model);
    void geminiStyleChanged(const QString &style);
    void geminiTestStatusChanged(const QString &status);
    void geminiTestStateChanged(const QString &state);
    void isTestingGeminiChanged(bool testing);
    void bookContextChanged(const QString &title, const QString &author);
    void detectedLanguageChanged(const QString &lang);
    void isOfflineChanged(bool offline);
    void ttsModeChanged(const QString &mode);
    void cacheProgressChanged(int current, int total);
    void isCachingChanged(bool caching);
    void errorOccurred(const QString &message);
    void sentenceFinished(int index);
    void chapterFinished();

private slots:
    void onPlaybackCompleted();
    void onMediaStateChanged(bb::multimedia::MediaState::Type state);
    void onNetworkReplyFinished();
    void onPrefetchReplyFinished();
    void onBatchCacheReplyFinished();
    void onDownloadReplyFinished();
    void onGeminiTestReplyFinished();

private:
    bb::multimedia::MediaPlayer *m_player;
    QNetworkAccessManager *m_nam;
    QNetworkReply *m_currentReply;
    QNetworkReply *m_prefetchReply;
    QNetworkReply *m_batchCacheReply;
    QNetworkReply *m_downloadReply;
    QNetworkReply *m_geminiTestReply;

    QString m_cacheDir;
    QStringList m_sentences;
    int m_currentIndex;
    bool m_active;
    bool m_playing;
    bool m_loading;
    double m_speed;
    double m_pitch;
    QString m_language; // "auto", "vi", "en", "fr", "de", "es", "ru", "zh-CN", "ja", "ko"
    QString m_detectedChapterLang;
    QString m_ttsProvider; // "google", "zalo", "fpt", "voicerss", "custom"
    QString m_ttsVoice;    // voice identifier
    QString m_ttsEngine;   // backwards compatibility for google engine
    QString m_ttsMode;     // "online", "offline"
    QString m_zaloApiKey;
    QString m_fptApiKey;
    QString m_voiceRssApiKey;
    QString m_customTtsUrl;
    QString m_customTtsVoice;
    QString m_customApiKey;
    QString m_geminiApiKey;
    QString m_geminiVoice;
    QString m_geminiModel;
    QString m_geminiStyle;
    QString m_geminiTestStatus;
    QString m_geminiTestState;
    bool m_isTestingGemini;
    QTime m_geminiTestTime;
    QString m_bookTitle;
    QString m_bookAuthor;

    bool m_isChapterReading;
    bool m_isOffline;
    bool m_isCaching;
    int m_cachingIndex;
    int m_googleEndpointIndex;

    void parseSentences(const QString &rawText);
    QString detectLanguage(const QString &text) const;
    QString getCacheFilePath(int index, const QString &lang) const;
    bool isSentenceCached(int index, const QString &lang) const;
    QUrl buildTtsUrl(const QString &text, const QString &lang, const QString &engine, bool useHttp = false) const;
    QUrl buildGoogleTtsUrl(const QString &text, const QString &lang, int endpointIdx) const;
    QNetworkRequest buildTtsRequest(const QUrl &url) const;
    QNetworkReply* sendProviderRequest(const QString &text, const QString &lang, int index, const QString &mode, int googleEndpointIdx = -1);
    void handleAudioData(int index, const QString &lang, const QString &mode, const QByteArray &data);
    void downloadAudioUrl(const QUrl &url, int index, const QString &lang, const QString &mode);
    void playCurrentSentence();
    void prefetchSentence(int index);
    void processNextBatchCache();
    void applyPitchToPlayer();
};

#endif // TTSMANAGER_HPP
