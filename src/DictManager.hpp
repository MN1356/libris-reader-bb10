#ifndef DICTMANAGER_HPP
#define DICTMANAGER_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

class DictManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isLookingUp READ isLookingUp NOTIFY lookupStateChanged)
    Q_PROPERTY(bool isTranslating READ isTranslating NOTIFY translateStateChanged)
    Q_PROPERTY(QString lastWord READ lastWord NOTIFY lookupResultChanged)
    Q_PROPERTY(QString lastPhonetic READ lastPhonetic NOTIFY lookupResultChanged)
    Q_PROPERTY(QString lastMeaning READ lastMeaning NOTIFY lookupResultChanged)
    Q_PROPERTY(QString lastTranslatedText READ lastTranslatedText NOTIFY translationResultChanged)
    Q_PROPERTY(QString lastOriginalText READ lastOriginalText NOTIFY translationResultChanged)
    Q_PROPERTY(QString targetLanguage READ targetLanguage WRITE setTargetLanguage NOTIFY targetLanguageChanged)

public:
    explicit DictManager(QObject *parent = 0);
    virtual ~DictManager();

    bool isLookingUp() const { return m_isLookingUp; }
    bool isTranslating() const { return m_isTranslating; }
    QString lastWord() const { return m_lastWord; }
    QString lastPhonetic() const { return m_lastPhonetic; }
    QString lastMeaning() const { return m_lastMeaning; }
    QString lastTranslatedText() const { return m_lastTranslatedText; }
    QString lastOriginalText() const { return m_lastOriginalText; }
    QString targetLanguage() const { return m_targetLanguage; }
    void setTargetLanguage(const QString &lang);

    // Offline Dictionary
    Q_INVOKABLE QVariantMap lookupOffline(const QString &word);

    // Online Dictionary (Free Dictionary API & Wiktionary)
    Q_INVOKABLE void lookupOnline(const QString &word, const QString &targetLang = QString());

    // Translation (Google Translate direct REST endpoint)
    Q_INVOKABLE void translateText(const QString &text, const QString &sourceLang = "auto", const QString &targetLang = QString());

    // External Translation & Dictionary tools in Browser
    Q_INVOKABLE void openExternalTool(const QString &service, const QString &text, const QString &targetLang = QString());

    // Clipboard
    Q_INVOKABLE void copyToClipboard(const QString &text);

signals:
    void lookupStateChanged();
    void translateStateChanged();
    void lookupResultChanged();
    void translationResultChanged();
    void targetLanguageChanged();
    void offlineLookupDone(const QVariantMap &result);
    void onlineLookupDone(bool success, const QString &word, const QString &phonetic, const QString &meaning, const QString &errorMsg);
    void translationDone(bool success, const QString &original, const QString &translated, const QString &detectedLang);
    void toastMessage(const QString &message);

private slots:
    void onOnlineLookupFinished();
    void onTranslationFinished();

private:
    QNetworkAccessManager *m_nam;
    QSqlDatabase m_db;
    bool m_dbReady;

    bool m_isLookingUp;
    bool m_isTranslating;
    QString m_lastWord;
    QString m_lastPhonetic;
    QString m_lastMeaning;
    QString m_lastTranslatedText;
    QString m_lastOriginalText;
    QString m_targetLanguage;

    void initDatabase();
    QString cleanWord(const QString &raw) const;
    QString parseGoogleTranslateResponse(const QByteArray &data, QString &outDetectedLang) const;
};

#endif // DICTMANAGER_HPP
