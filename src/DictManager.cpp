#include "DictManager.hpp"
#include <bb/system/Clipboard>
#include <bb/system/InvokeManager>
#include <bb/system/InvokeRequest>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QTextStream>
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
    loadVocab();
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

    m_db = QSqlDatabase::addDatabase("QSQLITE", "libris_dict_db");
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

    // Always attempt the request and fall back to offline lookup on error
    // because QNetworkConfigurationManager::isOnline() can return false negatives on BB10.

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
    sslConfig.setPeerVerifyMode(QSslSocket::defaultCaCertificates().isEmpty() ? QSslSocket::VerifyNone : QSslSocket::VerifyPeer);
    sslConfig.setSslOption(QSsl::SslOptionDisableServerNameIndication, false);
    req.setSslConfiguration(sslConfig);

    static quint64 s_lookupSeq = 0;
    quint64 reqSeq = ++s_lookupSeq;
    QNetworkReply *reply = m_nam->get(req);
    reply->setProperty("reqSeq", qulonglong(reqSeq));
    connect(reply, SIGNAL(finished()), this, SLOT(onOnlineLookupFinished()));

    QTimer *timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, SIGNAL(timeout()), reply, SLOT(abort()));
    timeoutTimer->start(15000);
}

void DictManager::onOnlineLookupFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    QVariant seqVar = reply->property("reqSeq");
    if (seqVar.isValid()) {
        // Ignore stale replies if a newer lookupOnline() was already issued
        QList<QNetworkReply*> activeReplies = m_nam->findChildren<QNetworkReply*>();
        quint64 mySeq = seqVar.toULongLong();
        for (int i = 0; i < activeReplies.size(); ++i) {
            QNetworkReply *other = activeReplies.at(i);
            if (other && other != reply && other->property("reqSeq").toULongLong() > mySeq) {
                reply->deleteLater();
                return;
            }
        }
    }

    m_isLookingUp = false;
    emit lookupStateChanged();

    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        reply->deleteLater();
        QVariantMap offRes = lookupOffline(m_lastWord);
        if (offRes.value("found").toBool()) {
            emit onlineLookupDone(true,
                                  offRes.value("word").toString(),
                                  offRes.value("phonetic").toString(),
                                  offRes.value("meaning").toString(),
                                  "");
        } else {
            emit onlineLookupDone(false, m_lastWord, "", "", tr("Không thể kết nối đến từ điển trực tuyến: ") + err);
        }
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
                            termList << tStr;
                        }
                    }
                    definitionText += "  " + termList.join(", ") + "\n\n";
                }
            }
        }
    }

    definitionText = definitionText.trimmed();
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
    if (trimmed.isEmpty()) return;

    QString tl = targetLang.isEmpty() ? m_targetLanguage : targetLang;
    QString sl = sourceLang.isEmpty() ? "auto" : sourceLang;

    m_isTranslating = true;
    m_lastOriginalText = trimmed;
    m_lastTranslatedText.clear();
    emit translateStateChanged();
    emit translationResultChanged();

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
    sslConfig.setPeerVerifyMode(QSslSocket::defaultCaCertificates().isEmpty() ? QSslSocket::VerifyNone : QSslSocket::VerifyPeer);
    sslConfig.setSslOption(QSsl::SslOptionDisableServerNameIndication, false);
    req.setSslConfiguration(sslConfig);

    static quint64 s_translateSeq = 0;
    quint64 trSeq = ++s_translateSeq;
    QNetworkReply *reply = m_nam->get(req);
    reply->setProperty("trSeq", qulonglong(trSeq));
    connect(reply, SIGNAL(finished()), this, SLOT(onTranslationFinished()));

    QTimer *timeoutTimer = new QTimer(reply);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, SIGNAL(timeout()), reply, SLOT(abort()));
    timeoutTimer->start(15000);
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
            if (s.isArray() && s.property("length").toInt32() > 0 && !s.property(0).isNull() && !s.property(0).isUndefined()) {
                QString part = s.property(0).toString();
                if (!part.isEmpty() && part != "null") {
                    translated += part;
                }
            }
        }
    }

    // Detected source language is often at root[2]
    if (root.property("length").toInt32() > 2) {
        outDetectedLang = root.property(2).toString();
    }

    return translated.trimmed();
}

void DictManager::onTranslationFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    QVariant seqVar = reply->property("trSeq");
    if (seqVar.isValid()) {
        QList<QNetworkReply*> activeReplies = m_nam->findChildren<QNetworkReply*>();
        quint64 mySeq = seqVar.toULongLong();
        for (int i = 0; i < activeReplies.size(); ++i) {
            QNetworkReply *other = activeReplies.at(i);
            if (other && other != reply && other->property("trSeq").toULongLong() > mySeq) {
                reply->deleteLater();
                return;
            }
        }
    }

    m_isTranslating = false;
    emit translateStateChanged();

    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        reply->deleteLater();
        m_lastTranslatedText.clear();
        emit translationResultChanged();
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
        m_lastTranslatedText.clear();
        emit translationResultChanged();
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

static QString escapeVocabJson(const QString &str)
{
    QString out;
    out.reserve(str.length() + 8);
    for (int i = 0; i < str.length(); ++i) {
        QChar c = str.at(i);
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

static QString escapeCsvField(const QString &str)
{
    QString s = str;
    s.replace("\r\n", " ").replace('\n', ' ').replace('\r', ' ');
    s.replace('"', "\"\"");
    return "\"" + s + "\"";
}

QString DictManager::vocabFilePath() const
{
    QString dirPath = (QDir("/accounts/1000/shared/documents").exists() || QDir("/accounts/1000/shared").exists())
        ? "/accounts/1000/shared/documents/Libris"
        : (QDir::homePath() + "/.libris");
    QDir().mkpath(dirPath);
    return dirPath + "/libris_vocab.json";
}

void DictManager::loadVocab()
{
    m_vocabList.clear();
    QFile f(vocabFilePath());
    if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QString content = QString::fromUtf8(f.readAll());
    f.close();

    QScriptEngine engine;
    QScriptValue arr = engine.evaluate("(" + content + ")");
    if (!arr.isArray()) return;

    int len = arr.property("length").toInt32();
    for (int i = 0; i < len; ++i) {
        QScriptValue it = arr.property(i);
        if (!it.isObject()) continue;
        QVariantMap m;
        m["word"] = it.property("word").toString();
        m["phonetic"] = it.property("phonetic").toString();
        m["meaning"] = it.property("meaning").toString();
        m["context"] = it.property("context").toString();
        m["bookTitle"] = it.property("bookTitle").toString();
        m["mastered"] = it.property("mastered").toBool();
        m["createdAt"] = it.property("createdAt").toString();
        if (!m["word"].toString().trimmed().isEmpty()) {
            m_vocabList.append(m);
        }
    }
}

void DictManager::saveVocab() const
{
    QFile f(vocabFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return;
    }
    QTextStream out(&f);
    out.setCodec("UTF-8");
    out << "[\n";
    for (int i = 0; i < m_vocabList.size(); ++i) {
        const QVariantMap m = m_vocabList.at(i).toMap();
        out << "  {"
            << "\"word\":\"" << escapeVocabJson(m.value("word").toString()) << "\","
            << "\"phonetic\":\"" << escapeVocabJson(m.value("phonetic").toString()) << "\","
            << "\"meaning\":\"" << escapeVocabJson(m.value("meaning").toString()) << "\","
            << "\"context\":\"" << escapeVocabJson(m.value("context").toString()) << "\","
            << "\"bookTitle\":\"" << escapeVocabJson(m.value("bookTitle").toString()) << "\","
            << "\"mastered\":" << (m.value("mastered").toBool() ? "true" : "false") << ","
            << "\"createdAt\":\"" << escapeVocabJson(m.value("createdAt").toString()) << "\""
            << "}";
        if (i + 1 < m_vocabList.size()) out << ",";
        out << "\n";
    }
    out << "]\n";
    f.close();
}

bool DictManager::saveVocabWord(const QString &word, const QString &phonetic, const QString &meaning, const QString &context, const QString &bookTitle)
{
    QString cleanW = word.trimmed();
    if (cleanW.isEmpty()) return false;

    for (int i = 0; i < m_vocabList.size(); ++i) {
        QVariantMap existing = m_vocabList.at(i).toMap();
        if (existing.value("word").toString().compare(cleanW, Qt::CaseInsensitive) == 0) {
            if (!phonetic.trimmed().isEmpty()) existing["phonetic"] = phonetic.trimmed();
            if (!meaning.trimmed().isEmpty()) existing["meaning"] = meaning.trimmed();
            if (!context.trimmed().isEmpty()) existing["context"] = context.trimmed();
            if (!bookTitle.trimmed().isEmpty()) existing["bookTitle"] = bookTitle.trimmed();
            m_vocabList[i] = existing;
            saveVocab();
            emit vocabChanged();
            return true;
        }
    }

    QVariantMap item;
    item["word"] = cleanW;
    item["phonetic"] = phonetic.trimmed();
    item["meaning"] = meaning.trimmed();
    item["context"] = context.trimmed();
    item["bookTitle"] = bookTitle.trimmed();
    item["mastered"] = false;
    item["createdAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_vocabList.prepend(item);
    saveVocab();
    emit vocabChanged();
    return true;
}

QVariantList DictManager::vocabList() const
{
    return m_vocabList;
}

void DictManager::reloadVocab()
{
    loadVocab();
    emit vocabChanged();
}

bool DictManager::removeVocabWord(int index)
{
    if (index < 0 || index >= m_vocabList.size()) return false;
    m_vocabList.removeAt(index);
    saveVocab();
    emit vocabChanged();
    return true;
}

bool DictManager::toggleVocabMastered(int index)
{
    if (index < 0 || index >= m_vocabList.size()) return false;
    QVariantMap item = m_vocabList.at(index).toMap();
    item["mastered"] = !item.value("mastered").toBool();
    m_vocabList[index] = item;
    saveVocab();
    emit vocabChanged();
    return true;
}

QString DictManager::exportVocabCsv()
{
    if (m_vocabList.isEmpty()) return QString();
    QFileInfo fi(vocabFilePath());
    QString csvPath = fi.absolutePath() + "/libris_vocab_anki.csv";
    QFile f(csvPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return QString();
    }
    QTextStream out(&f);
    out.setCodec("UTF-8");
    out << "Word,Phonetic,Meaning,Context,Book,Mastered\n";
    for (int i = 0; i < m_vocabList.size(); ++i) {
        const QVariantMap m = m_vocabList.at(i).toMap();
        out << escapeCsvField(m.value("word").toString()) << ","
            << escapeCsvField(m.value("phonetic").toString()) << ","
            << escapeCsvField(m.value("meaning").toString()) << ","
            << escapeCsvField(m.value("context").toString()) << ","
            << escapeCsvField(m.value("bookTitle").toString()) << ","
            << (m.value("mastered").toBool() ? "1" : "0") << "\n";
    }
    f.close();
    return csvPath;
}
