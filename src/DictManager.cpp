#include "DictManager.hpp"
#include <bb/system/Clipboard>
#include <bb/system/InvokeManager>
#include <bb/system/InvokeRequest>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QRegExp>
#include <QNetworkConfigurationManager>
#include <QScriptEngine>
#include <QScriptValue>
#include <QScriptValueIterator>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslSocket>

DictManager::DictManager(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_dbReady(false)
    , m_isLookingUp(false)
    , m_isTranslating(false)
    , m_targetLanguage("vi")
{
    initDatabase();
}

DictManager::~DictManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

void DictManager::initDatabase()
{
    QString dbPath;
    QStringList candidates;
    candidates << "app/native/assets/dict/offline_dict.db"
               << "assets/dict/offline_dict.db"
               << QDir::currentPath() + "/assets/dict/offline_dict.db";

    for (int i = 0; i < candidates.size(); ++i) {
        if (QFile::exists(candidates[i])) {
            dbPath = candidates[i];
            break;
        }
    }

    if (dbPath.isEmpty()) {
        qWarning() << "[DictManager] Offline dictionary database not found!";
        return;
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", "berry_dict_db");
    m_db.setDatabaseName(dbPath);
    m_db.setConnectOptions("QSQLITE_OPEN_READONLY");

    if (!m_db.open()) {
        qWarning() << "[DictManager] Failed to open SQLite dictionary:" << m_db.lastError().text();
    } else {
        m_dbReady = true;
        qDebug() << "[DictManager] Offline dictionary successfully loaded from:" << dbPath;
    }
}

void DictManager::setTargetLanguage(const QString &lang)
{
    if (m_targetLanguage != lang && !lang.isEmpty()) {
        m_targetLanguage = lang;
        emit targetLanguageChanged();
    }
}

QString DictManager::cleanWord(const QString &raw) const
{
    QString w = raw.trimmed();
    // Decode percent-encoded strings if any remain
    if (w.contains('%')) {
        w = QUrl::fromPercentEncoding(w.toUtf8()).trimmed();
    }
    // Remove leading/trailing non-alphanumeric punctuation
    while (!w.isEmpty() && !w[0].isLetterOrNumber()) {
        w.remove(0, 1);
    }
    while (!w.isEmpty() && !w[w.length() - 1].isLetterOrNumber()) {
        w.chop(1);
    }
    return w.toLower();
}

QVariantMap DictManager::lookupOffline(const QString &word)
{
    QVariantMap res;
    res["found"] = false;
    res["word"] = word;
    res["phonetic"] = "";
    res["meaning"] = "";
    res["suggestions"] = QStringList();

    if (!m_dbReady || word.trimmed().isEmpty()) {
        emit offlineLookupDone(res);
        return res;
    }

    QString target = cleanWord(word);
    if (target.isEmpty()) {
        emit offlineLookupDone(res);
        return res;
    }

    QStringList candidates;
    candidates << target;

    // Stemming heuristics for English
    if (target.endsWith("ies") && target.length() > 4) {
        candidates << target.left(target.length() - 3) + "y";
    }
    if (target.endsWith("es") && target.length() > 3) {
        candidates << target.left(target.length() - 2);
    }
    if (target.endsWith("s") && target.length() > 2) {
        candidates << target.left(target.length() - 1);
    }
    if (target.endsWith("ing") && target.length() > 4) {
        candidates << target.left(target.length() - 3);
        candidates << target.left(target.length() - 3) + "e";
    }
    if (target.endsWith("ed") && target.length() > 3) {
        candidates << target.left(target.length() - 2);
        candidates << target.left(target.length() - 1); // e.g. loved -> love
    }
    if (target.endsWith("ly") && target.length() > 3) {
        candidates << target.left(target.length() - 2);
    }

    QSqlQuery q(m_db);
    for (int i = 0; i < candidates.size(); ++i) {
        q.prepare("SELECT word, phonetic, meaning FROM words WHERE word = ? LIMIT 1");
        q.addBindValue(candidates[i]);
        if (q.exec() && q.next()) {
            res["found"] = true;
            res["word"] = q.value(0).toString();
            res["phonetic"] = q.value(1).toString();
            res["meaning"] = q.value(2).toString();

            m_lastWord = res["word"].toString();
            m_lastPhonetic = res["phonetic"].toString();
            m_lastMeaning = res["meaning"].toString();
            emit lookupResultChanged();
            emit offlineLookupDone(res);
            return res;
        }
    }

    // If not found, search for prefix matches to suggest similar words
    q.prepare("SELECT word FROM words WHERE word LIKE ? LIMIT 5");
    q.addBindValue(target.left(3) + "%");
    QStringList sugg;
    if (q.exec()) {
        while (q.next()) {
            sugg << q.value(0).toString();
        }
    }
    res["suggestions"] = sugg;
    emit offlineLookupDone(res);
    return res;
}

void DictManager::lookupOnline(const QString &word, const QString &targetLang)
{
    QString clean = cleanWord(word);
    if (clean.isEmpty()) return;

    QNetworkConfigurationManager netMgr;
    if (!netMgr.isOnline()) {
        m_isLookingUp = false;
        emit lookupStateChanged();
        lookupOffline(clean);
        return;
    }

    m_isLookingUp = true;
    m_lastWord = clean;
    emit lookupStateChanged();

    QString tl = targetLang.isEmpty() ? (m_targetLanguage.isEmpty() ? "vi" : m_targetLanguage) : targetLang;
    QString encoded = QUrl::toPercentEncoding(clean);
    QString urlStr = QString("https://translate.googleapis.com/translate_a/single?client=dict-chrome-ex&sl=auto&tl=%1&dt=t&dt=bd&dt=rm&q=%2")
                     .arg(tl)
                     .arg(encoded);

    QUrl url = QUrl::fromEncoded(urlStr.toUtf8());
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "Mozilla/5.0 (BlackBerry; U; BlackBerry 10; en-US) AppleWebKit/537.36");

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::AnyProtocol);
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setSslOption(QSsl::SslOptionDisableServerNameIndication, false);
    req.setSslConfiguration(sslConfig);

    QNetworkReply *reply = m_nam->get(req);
    reply->ignoreSslErrors();
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply, SLOT(ignoreSslErrors()));
    connect(reply, SIGNAL(finished()), this, SLOT(onOnlineLookupFinished()));
}

void DictManager::onOnlineLookupFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    m_isLookingUp = false;
    emit lookupStateChanged();

    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        reply->deleteLater();
        emit onlineLookupDone(false, m_lastWord, "", "", tr("Không thể kết nối đến từ điển trực tuyến: ") + err);
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    // Parse JSON using QScriptEngine
    QScriptEngine engine;
    QScriptValue root = engine.evaluate("(" + QString::fromUtf8(data) + ")");

    if (!root.isArray()) {
        emit onlineLookupDone(false, m_lastWord, "", "", tr("Không tìm thấy kết quả từ điển trực tuyến."));
        return;
    }

    QString word = m_lastWord;
    QString phonetic;
    QString mainTranslation;

    // Sentence array is at root[0]
    QScriptValue sentences = root.property(0);
    if (sentences.isArray()) {
        int sLen = sentences.property("length").toInt32();
        for (int i = 0; i < sLen; ++i) {
            QScriptValue s = sentences.property(i);
            if (s.isArray() && s.property("length").toInt32() > 0) {
                QString trans = s.property(0).toString();
                if (!trans.isEmpty() && trans != "null") {
                    if (!mainTranslation.isEmpty()) mainTranslation += " ";
                    mainTranslation += trans;
                }
                if (s.property("length").toInt32() > 3) {
                    QString ph = s.property(3).toString();
                    if (!ph.isEmpty() && ph != "null") {
                        phonetic = ph;
                    }
                }
            }
        }
    }

    if (!phonetic.isEmpty() && !phonetic.startsWith("/")) {
        phonetic = "/" + phonetic + "/";
    }

    if (mainTranslation.contains('%')) {
        QString dec = QUrl::fromPercentEncoding(mainTranslation.toUtf8());
        if (!dec.isEmpty()) mainTranslation = dec;
    }

    QString definitionText;
    if (!mainTranslation.isEmpty()) {
        definitionText += tr("★ Nghĩa: ") + mainTranslation + "\n\n";
    }

    // Dictionary by part of speech is at root[1]
    QScriptValue dictEntries = root.property(1);
    if (dictEntries.isArray()) {
        int dLen = dictEntries.property("length").toInt32();
        for (int i = 0; i < dLen; ++i) {
            QScriptValue entry = dictEntries.property(i);
            if (entry.isArray() && entry.property("length").toInt32() >= 2) {
                QString pos = entry.property(0).toString();
                QString posTitle = pos;
                if (pos == "noun") posTitle = tr("Danh từ (noun)");
                else if (pos == "verb") posTitle = tr("Động từ (verb)");
                else if (pos == "adjective") posTitle = tr("Tính từ (adjective)");
                else if (pos == "adverb") posTitle = tr("Phó từ (adverb)");
                else if (pos == "preposition") posTitle = tr("Giới từ (preposition)");
                else if (pos == "conjunction") posTitle = tr("Liên từ (conjunction)");
                else if (pos == "pronoun") posTitle = tr("Đại từ (pronoun)");
                else if (pos == "interjection") posTitle = tr("Thán từ (interjection)");

                definitionText += "• [" + posTitle + "]:\n";

                QScriptValue terms = entry.property(1);
                if (terms.isArray()) {
                    QStringList termList;
                    int tLen = terms.property("length").toInt32();
                    for (int t = 0; t < tLen && t < 10; ++t) {
                        QString tStr = terms.property(t).toString();
                        if (!tStr.isEmpty() && tStr != "null") {
                            if (tStr.contains('%')) {
                                QString decTerm = QUrl::fromPercentEncoding(tStr.toUtf8());
                                if (!decTerm.isEmpty()) tStr = decTerm;
                            }
                            termList << tStr;
                        }
                    }
                    definitionText += "  " + termList.join(", ") + "\n\n";
                }
            }
        }
    }

    definitionText = definitionText.trimmed();
    if (definitionText.contains('%')) {
        QString decDef = QUrl::fromPercentEncoding(definitionText.toUtf8());
        if (!decDef.isEmpty()) definitionText = decDef;
    }
    if (definitionText.isEmpty()) {
        emit onlineLookupDone(false, m_lastWord, "", "", tr("Không tìm thấy định nghĩa cho từ này."));
        return;
    }

    m_lastWord = word;
    m_lastPhonetic = phonetic;
    m_lastMeaning = definitionText;
    emit lookupResultChanged();
    emit onlineLookupDone(true, word, phonetic, m_lastMeaning, "");
}

void DictManager::translateText(const QString &text, const QString &sourceLang, const QString &targetLang)
{
    QString trimmed = text.trimmed();
    if (trimmed.contains('%')) {
        trimmed = QUrl::fromPercentEncoding(trimmed.toUtf8()).trimmed();
    }
    if (trimmed.isEmpty()) return;

    QString tl = targetLang.isEmpty() ? m_targetLanguage : targetLang;
    QString sl = sourceLang.isEmpty() ? "auto" : sourceLang;

    m_isTranslating = true;
    m_lastOriginalText = trimmed;
    emit translateStateChanged();

    // Google Translate public client API
    QString encoded = QUrl::toPercentEncoding(trimmed);
    QString urlStr = QString("https://translate.googleapis.com/translate_a/single?client=dict-chrome-ex&sl=%1&tl=%2&dt=t&q=%3")
                     .arg(sl)
                     .arg(tl)
                     .arg(encoded);

    QUrl url = QUrl::fromEncoded(urlStr.toUtf8());
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "Mozilla/5.0 (BlackBerry; U; BlackBerry 10; en-US) AppleWebKit/537.36");

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setProtocol(QSsl::AnyProtocol);
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setSslOption(QSsl::SslOptionDisableServerNameIndication, false);
    req.setSslConfiguration(sslConfig);

    QNetworkReply *reply = m_nam->get(req);
    reply->ignoreSslErrors();
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), reply, SLOT(ignoreSslErrors()));
    connect(reply, SIGNAL(finished()), this, SLOT(onTranslationFinished()));
}

QString DictManager::parseGoogleTranslateResponse(const QByteArray &data, QString &outDetectedLang) const
{
    QScriptEngine engine;
    QScriptValue root = engine.evaluate("(" + QString::fromUtf8(data) + ")");

    if (!root.isArray()) return QString();

    QString translated;
    // Sentence array is at root[0]
    QScriptValue sentences = root.property(0);
    if (sentences.isArray()) {
        int sLen = sentences.property("length").toInt32();
        for (int i = 0; i < sLen; ++i) {
            QScriptValue s = sentences.property(i);
            if (s.isArray() && s.property("length").toInt32() > 0) {
                QString part = s.property(0).toString();
                if (part.contains('%')) {
                    QString decPart = QUrl::fromPercentEncoding(part.toUtf8());
                    if (!decPart.isEmpty()) part = decPart;
                }
                translated += part;
            }
        }
    }

    // Detected source language is often at root[2]
    if (root.property("length").toInt32() > 2) {
        outDetectedLang = root.property(2).toString();
    }

    translated = translated.trimmed();
    if (translated.contains('%')) {
        QString dec = QUrl::fromPercentEncoding(translated.toUtf8());
        if (!dec.isEmpty()) translated = dec;
    }

    return translated;
}

void DictManager::onTranslationFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    m_isTranslating = false;
    emit translateStateChanged();

    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        reply->deleteLater();
        emit translationDone(false, m_lastOriginalText, tr("Lỗi dịch thuật: ") + err, "");
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QString detectedLang;
    QString translated = parseGoogleTranslateResponse(data, detectedLang);

    if (!translated.isEmpty()) {
        m_lastTranslatedText = translated;
        emit translationResultChanged();
        emit translationDone(true, m_lastOriginalText, translated, detectedLang);
    } else {
        emit translationDone(false, m_lastOriginalText, tr("Không thể phân tích kết quả dịch thuật."), "");
    }
}

void DictManager::openExternalTool(const QString &service, const QString &text, const QString &targetLang)
{
    QString tl = targetLang.isEmpty() ? m_targetLanguage : targetLang;
    QString encoded = QUrl::toPercentEncoding(text.trimmed());
    QString urlStr;

    if (service == "google") {
        urlStr = QString("https://translate.google.com/?sl=auto&tl=%1&text=%2").arg(tl).arg(encoded);
    } else if (service == "bing") {
        urlStr = QString("https://www.bing.com/translator?from=auto&to=%1&text=%2").arg(tl).arg(encoded);
    } else if (service == "deepl") {
        urlStr = QString("https://www.deepl.com/translator#auto/%1/%2").arg(tl).arg(encoded);
    } else if (service == "cambridge") {
        urlStr = QString("https://dictionary.cambridge.org/dictionary/english/%1").arg(encoded);
    } else if (service == "soha") {
        urlStr = QString("http://tratu.soha.vn/dict/en_vn/%1").arg(encoded);
    } else if (service == "yandex") {
        urlStr = QString("https://translate.yandex.com/?source_lang=auto&target_lang=%1&text=%2").arg(tl).arg(encoded);
    } else {
        urlStr = QString("https://www.google.com/search?q=define+%1").arg(encoded);
    }

    bb::system::InvokeManager invokeManager;
    bb::system::InvokeRequest request;
    request.setTarget("sys.browser");
    request.setAction("bb.action.OPEN");
    request.setUri(QUrl::fromEncoded(urlStr.toUtf8()));
    invokeManager.invoke(request);
}

void DictManager::copyToClipboard(const QString &text)
{
    if (text.isEmpty()) return;

    bb::system::Clipboard clipboard;
    clipboard.clear();
    clipboard.insert("text/plain", text.toUtf8());
    emit toastMessage(tr("Đã sao chép vào bộ nhớ tạm"));
}
