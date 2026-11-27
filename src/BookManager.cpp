#include "BookManager.hpp"
#include "RegexUtil.hpp"
#include "ImageUriUtil.hpp"
#include "BuildInfo.hpp"
#include "BookWorkers.hpp"
#include "OpdsManager.hpp"
#include "WiFiDropServer.hpp"
#include "miniz.h"
#include <bb/cascades/Application>
#include <bb/cascades/ThemeSupport>
#include <bb/cascades/VisualStyle>
#include <bb/cascades/controls/control.h>
#include <bb/cascades/layouts/stacklayoutproperties.h>
#include <bb/device/DisplayInfo>
#include <bb/multimedia/MediaKeyWatcher>
#include <bb/multimedia/MediaKey>
#include <bb/system/InvokeManager>
#include <bb/system/InvokeRequest>
#include <bb/system/InvokeTargetReply>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QCryptographicHash>
#include <QDebug>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QLinearGradient>
#include <QtGui/QFont>
#include <QtGui/QFontMetrics>
#include <QtGui/QPen>
#include <QCoreApplication>
#include <QEvent>
#include <QLocale>
#include <QTime>
#include <QDate>
#include <QUrl>
#include <QtAlgorithms>
#include <QRegExp>
#include <QScriptEngine>
#include <QTextStream>

static QString escapeJsonString(const QString &str)
{
    QString out;
    out.reserve(str.length() + 16);
    for (int i = 0; i < str.length(); ++i) {
        QChar c = str.at(i);
        if (c == '\\') {
            out += "\\\\";
        } else if (c == '"') {
            out += "\\\"";
        } else if (c == '\b') {
            out += "\\b";
        } else if (c == '\f') {
            out += "\\f";
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if (c == '\t') {
            out += "\\t";
        } else if (c.unicode() == 0x2028) {
            out += "\\u2028";
        } else if (c.unicode() == 0x2029) {
            out += "\\u2029";
        } else if (c.unicode() < 32) {
            out += QString("\\u%1").arg(c.unicode(), 4, 16, QChar('0'));
        } else {
            out += c;
        }
    }
    return out;
}

static void extractSeriesFromTitleOrFile(const QString &title, const QString &fileName, QString &outSeries, int &outIndex) {
    if (!outSeries.isEmpty()) return;

    // Pattern 1: [Series Name #1] or [Series Name 01] or [Series Name - 01]
    QRegExp reBrackets = minimalRx("^\\[([^\\]#]+)(?:[\\s_\\-–#]+|[\\s_\\-–#]+(?:Tập|Quyển|Book|Vol|Volume|Part|Phần|#)\\s*)(\\d+)\\]", Qt::CaseInsensitive);
    if (reBrackets.indexIn(title) != -1 || reBrackets.indexIn(fileName) != -1) {
        outSeries = reBrackets.cap(1).trimmed();
        outIndex = reBrackets.cap(2).toInt();
        return;
    }

    // Pattern 2: (Series Name #1) or (Series Name, Book 1)
    QRegExp reParens = minimalRx("\\(([^\\)#]+)(?:[\\s_\\-–#,]+|[\\s_\\-–#,]+(?:Tập|Quyển|Book|Vol|Volume|Part|Phần|#)\\s*)(\\d+)\\)", Qt::CaseInsensitive);
    if (reParens.indexIn(title) != -1 || reParens.indexIn(fileName) != -1) {
        outSeries = reParens.cap(1).trimmed();
        outIndex = reParens.cap(2).toInt();
        return;
    }

    // Pattern 3: Series Name - Tập 1 / Book 1 / Vol 1 - Title
    QRegExp reDash("^(.*)\\s*[-–]\\s*(?:Tập|Quyển|Book|Vol|Volume|Part|Phần|#)\\s*(\\d+)\\s*[-–]", Qt::CaseInsensitive);
    if (reDash.indexIn(title) != -1 || reDash.indexIn(fileName) != -1) {
        outSeries = reDash.cap(1).trimmed();
        outIndex = reDash.cap(2).toInt();
        return;
    }
}

static bool isGenericChapterTitle(const QString &title);
static QString combineChapterTitle(const QString &baseTitle, const QString &subTitle, int chapterNumber = -1);

static bool isValidDecodedText(const QString &s)
{
    if (s.isEmpty()) return false;
    for (int i = 0; i < s.size(); ++i) {
        ushort u = s.at(i).unicode();
        if (u == 0xFFFD) return false;
        if (u < 32 || (u >= 0x7F && u <= 0x9F)) return false;
    }
    return true;
}

static bool hasKnownBookExtension(const QString &s)
{
    QString clean = s.trimmed();
    while (clean.endsWith('/')) clean.chop(1);
    static const char *exts[] = {
        ".epub", ".pdf", ".txt", ".md", ".markdown", ".html", ".htm",
        ".fb2", ".djvu", ".djv", ".cbz", ".mobi", ".azw3", ".azw", ".kf8", ".prc"
    };
    const int count = (int)(sizeof(exts) / sizeof(exts[0]));
    for (int i = 0; i < count; ++i) {
        if (clean.endsWith(QLatin1String(exts[i]), Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

static QString normalizeLeadingPath(const QString &s)
{
    QString p = s.trimmed();
    if (p.isEmpty()) return p;

    if (p.startsWith("file://", Qt::CaseInsensitive)) {
        p = p.mid(7);
        while (p.startsWith("///")) p = p.mid(1);
    }

    int accIdx = p.indexOf("/accounts/", 0, Qt::CaseInsensitive);
    if (accIdx > 0) {
        p = p.mid(accIdx);
    } else if (!p.startsWith('/')) {
        if (p.startsWith("accounts/", Qt::CaseInsensitive) ||
            p.startsWith("app/", Qt::CaseInsensitive) ||
            p.startsWith("removable/", Qt::CaseInsensitive) ||
            QFile::exists("/" + p)) {
            p.prepend('/');
        }
    }

    while (p.startsWith("//")) {
        p = p.mid(1);
    }
    return p;
}

static QString decodeQtAndPercentEscapes(const QString &s)
{
    QString cur = s;
    QRegExp uEscRx("%U([0-9A-Fa-f]{4})");
    QRegExp pctHexRx("%[0-9A-Fa-f]{2}");

    for (int pass = 0; pass < 4; ++pass) {
        QString next = cur;

        // Decode Qt QSettings::IniFormat %Uxxxx key escapes if any survived literally
        int pos = 0;
        while ((pos = uEscRx.indexIn(next, pos)) != -1) {
            bool ok = false;
            ushort code = (ushort)uEscRx.cap(1).toUInt(&ok, 16);
            if (ok && code >= 32 && code != 0xFFFD && !(code >= 0x7F && code <= 0x9F)) {
                next.replace(pos, 6, QChar(code));
                pos += 1;
            } else {
                pos += 6;
            }
        }

        if (pctHexRx.indexIn(next) >= 0) {
            QString pct = QUrl::fromPercentEncoding(next.toUtf8());
            if (!pct.isEmpty() && isValidDecodedText(pct)) {
                next = pct;
            }
        }

        if (next == cur || !isValidDecodedText(next)) {
            break;
        }
        cur = next;
    }
    return cur;
}

static bool isValidBase64Blob(const QString &s)
{
    if (s.length() < 8) return false;
    int padCount = 0;
    for (int i = 0; i < s.length(); ++i) {
        QChar c = s.at(i);
        ushort u = c.unicode();
        if (u == '=') {
            padCount++;
            if (padCount > 2) return false;
            continue;
        }
        if (padCount > 0) return false; // '=' only allowed at tail
        bool isAlphaNum = (u >= 'A' && u <= 'Z') || (u >= 'a' && u <= 'z') || (u >= '0' && u <= '9');
        bool isB64Sym = (u == '+' || u == '/' || u == '-' || u == '_');
        if (!isAlphaNum && !isB64Sym) return false;
    }
    return true;
}

static bool isPlausibleBookPath(const QString &s)
{
    if (!isValidDecodedText(s)) return false;
    if (QFile::exists(s)) return true;

    QRegExp pctHexRx("%[0-9A-Fa-f]{2}|%U[0-9A-Fa-f]{4}");
    if (pctHexRx.indexIn(s) >= 0) return false;

    bool hasExt = hasKnownBookExtension(s);
    bool hasAccounts = s.startsWith("/accounts/", Qt::CaseInsensitive) ||
                       s.startsWith("accounts/", Qt::CaseInsensitive) ||
                       s.contains("/accounts/", Qt::CaseInsensitive);
    if (s.startsWith('/') && (hasExt || hasAccounts)) return true;
    if (hasAccounts && (hasExt || s.count('/') >= 2)) return true;
    if (hasExt && s.contains('/')) return true;
    return false;
}

static bool looksLikeGarbledTitle(const QString &title)
{
    QString t = title.trimmed();
    if (t.length() <= 1) return true;
    if (!isValidDecodedText(t)) return true;
    if (t.contains("accounts/1000", Qt::CaseInsensitive) || t.contains("/accounts/", Qt::CaseInsensitive)) return true;
    if (QRegExp("%[0-9A-Fa-f]{2}|%U[0-9A-Fa-f]{4}").indexIn(t) >= 0) return true;

    if (!t.contains(' ')) {
        if (t.length() >= 10 &&
            (t.startsWith("L2") || t.startsWith("JTJ") || t.startsWith("YWNjb3Vud") || t.startsWith("ZmlsZ"))) {
            return true;
        }
        if (QRegExp("^[A-Za-z0-9+/_-]{12,}={1,2}$").exactMatch(t)) {
            return true;
        }
        if (QRegExp("^[0-9a-fA-F-]{16,}$").exactMatch(t) &&
            (t.length() >= 24 || (t.contains(QRegExp("[a-fA-F]")) && t.contains(QRegExp("[0-9]"))))) {
            return true;
        }
        if (t.length() >= 20 && QRegExp("^[A-Za-z0-9+/=_-]+$").exactMatch(t)) {
            bool hasDigit = false, hasUpper = false, hasLower = false, hasB64Sym = false;
            for (int i = 0; i < t.length(); ++i) {
                ushort u = t.at(i).unicode();
                if (u >= '0' && u <= '9') hasDigit = true;
                else if (u >= 'A' && u <= 'Z') hasUpper = true;
                else if (u >= 'a' && u <= 'z') hasLower = true;
                else if (u == '+' || u == '/') hasB64Sym = true;
            }
            if ((hasDigit && hasUpper && hasLower) || (hasB64Sym && (hasDigit || hasUpper))) {
                return true;
            }
        }
    }
    return false;
}

static QString sanitizeStatsFormat(const QString &fmt, const QString &path)
{
    static const char *known[] = {
        "epub", "pdf", "djvu", "djv", "txt", "md", "markdown", "html", "htm", "fb2", "cbz", "mobi", "azw3", "azw", "kf8", "prc"
    };
    const int count = (int)(sizeof(known) / sizeof(known[0]));

    QString f = fmt.trimmed().toLower();
    for (int i = 0; i < count; ++i) {
        if (f == QLatin1String(known[i])) return f;
    }

    QString suf = QFileInfo(path).suffix().trimmed().toLower();
    for (int i = 0; i < count; ++i) {
        if (suf == QLatin1String(known[i])) return suf;
    }
    return "epub";
}

static QString unwrapBookPath(const QString &raw)
{
    QString cur = normalizeLeadingPath(raw);
    if (cur.isEmpty()) return QString();

    if (isPlausibleBookPath(cur)) {
        return cur;
    }

    QString pctDecoded = normalizeLeadingPath(decodeQtAndPercentEscapes(cur));
    if (isPlausibleBookPath(pctDecoded)) {
        return pctDecoded;
    }
    if (isValidDecodedText(pctDecoded) && pctDecoded != cur) {
        cur = pctDecoded;
    }

    // Attempt bounded Base64 unwrapping ONLY for strictly valid Base64 tokens
    for (int step = 0; step < 8; ++step) {
        if (isPlausibleBookPath(cur)) break;
        if (!isValidBase64Blob(cur)) break;

        QString stdB64 = cur;
        stdB64.replace('-', '+');
        stdB64.replace('_', '/');
        QByteArray latin = stdB64.toLatin1();
        int rem = latin.size() % 4;
        if (rem != 0) {
            latin.append(QByteArray(4 - rem, '='));
        }

        QByteArray dec = QByteArray::fromBase64(latin);
        if (dec.isEmpty()) break;

        QString nextStr = QString::fromUtf8(dec).trimmed();
        if (!isValidDecodedText(nextStr) || nextStr == cur) break;

        QString pctNext = decodeQtAndPercentEscapes(nextStr);
        if (isValidDecodedText(pctNext)) {
            nextStr = pctNext;
        }
        nextStr = normalizeLeadingPath(nextStr);

        if (!isPlausibleBookPath(nextStr) &&
            !hasKnownBookExtension(nextStr) &&
            !isValidBase64Blob(nextStr) &&
            !nextStr.contains('/')) {
            break;
        }

        cur = nextStr;
    }

    cur = normalizeLeadingPath(decodeQtAndPercentEscapes(cur));
    return cur;
}

BookManager::BookManager(QObject *parent)
    : QObject(parent)
    , m_pdfParser(NULL)
    , m_translator(new AppTranslator(this))
    , m_language("en")
    , m_tts(NULL)
    , m_model(new bb::cascades::GroupDataModel(QStringList() << "modifiedIso", this))
    , m_folderModel(new bb::cascades::GroupDataModel(QStringList() << "folderName", this))
    , m_fileBrowserModel(new bb::cascades::GroupDataModel(QStringList() << "sortKey", this))
    , m_mainViewMode("shelf")
    , m_isScanning(true)
    , m_isLoadingBook(false)
    , m_scanStatusText("Đang nạp và quét thư viện sách...")
    , m_currentBrowserPath("root")
    , m_currentBrowserTitle("Duyệt Thư Mục")
    , m_canGoUp(false)
    , m_browserSearchQuery("")
    , m_browserFormatFilter("all")
    , m_recentSearchQuery("")
    , m_libraryViewMode("mosaic")
    , m_libraryBrowseMode("folder")
    , m_selectedFolderPath("")
    , m_selectedFolderName("")
    , m_screenWidth(720)
    , m_screenHeight(720)
    , m_sortBy("date_desc")
    , m_groupBy("none")
    , m_formatFilter("all")
    , m_categoryFilter("all")
    , m_searchQuery("")
    , m_currentFormat("epub")
    , m_currentBookSeries("")
    , m_currentBookSeriesIndex(0)
    , m_currentChapterIndex(0)
    , m_totalChapters(0)
    , m_fontSize(18)
    , m_fontFamily("sans")
    , m_theme("dark")
    , m_appTheme("dark")
    , m_readingMode("paged")
    , m_columnLayout("single")
    , m_mangaRtlMode(false)
    , m_currentPageInChapter(1)
    , m_totalPagesInChapter(1)
    , m_currentScrollPos(0)
    , m_shouldRestorePageOnLoad(false)
    , m_cachedContentChapter(-1)
    , m_cachedContentFontSize(0)
    , m_pdfInverted(false)
    , m_isPdfAutoCrop(false)
    , m_lastBookPath("")
    , m_lastBookTitle("")
    , m_lastBookAuthor("")
    , m_lastBookCover("")
    , m_lastBookFormat("")
    , m_lastBookProgress(0)
    , m_showFooterChapter(true)
    , m_showFooterProgress(true)
    , m_showFooterBattery(true)
    , m_showFooterClock(true)
    , m_lineSpacing("normal")
    , m_textAlign("justify")
    , m_overridePublisherCss(false)
    , m_marginTop(16)
    , m_marginBottom(16)
    , m_marginLeft(16)
    , m_marginRight(16)
    , m_totalReadingSeconds(0)
    , m_todayReadingSeconds(0)
    , m_dailyGoalMinutes(30)
    , m_todayDate(QDate::currentDate().toString("yyyy-MM-dd"))
    , m_readingHistoryModel(new bb::cascades::GroupDataModel(QStringList() << "sortKey", this))
    , m_homeFeedModel(new bb::cascades::ArrayDataModel(this))
    , m_isSearching(false)
    , m_isReaderActive(false)
    , m_isPdfReflow(false)
    , m_hasPdfText(false)
    , m_pdfHasAnyOcr(false)
    , m_ttsAutoAdvance(false)
    , m_searchWorker(NULL)
    , m_scanWorker(NULL)
    , m_coverWorker(NULL)
    , m_isSendingLog(false)
    , m_lastLogStatus("")
    , m_devServerIp("192.168.0.108")
    , m_isAppSleeping(false)
    , m_suppressPersistentSync(false)
    , m_shutdownSaved(false)
    , m_pendingTtsAfterPdfRender(false)
    , m_pendingJumpPageAfterPdfOpen(-1)
    , m_opdsManager(NULL)
    , m_wifiDropServer(NULL)
    , m_warmFilterEnabled(false)
    , m_warmFilterIntensity(25)
    , m_autoNightTheme(false)
    , m_focusModeEnabled(false)
    , m_volumeKeyTurnEnabled(false)
    , m_volUpWatcher(NULL)
    , m_volDownWatcher(NULL)
{
    m_model->setGrouping(bb::cascades::ItemGrouping::None);
    m_model->setSortedAscending(false);

    m_folderModel->setGrouping(bb::cascades::ItemGrouping::None);
    m_folderModel->setSortedAscending(true);

    m_fileBrowserModel->setGrouping(bb::cascades::ItemGrouping::None);
    m_fileBrowserModel->setSortedAscending(true);

    m_readingHistoryModel->setGrouping(bb::cascades::ItemGrouping::None);
    m_readingHistoryModel->setSortedAscending(true);

    connect(&m_readingHeartbeatTimer, SIGNAL(timeout()), this, SLOT(onReadingHeartbeat()));
    connect(this, SIGNAL(currentBookChanged()), this, SIGNAL(readerBrightnessChanged()));

    loadSettings();
    applyVisualStyle();
    m_translator->setLanguage(m_language);
    QCoreApplication::installTranslator(m_translator);

    openBrowserFolder("root");

    // Battery and Clock monitors for reader footer (started on demand when reading)
    connect(&m_batteryInfo, SIGNAL(levelChanged(int, bb::device::BatteryChargingState::Type)),
            this, SLOT(onBatteryLevelChanged(int, bb::device::BatteryChargingState::Type)));
    connect(&m_clockTimer, SIGNAL(timeout()), this, SLOT(onClockTick()));

    m_progressSaveTimer.setSingleShot(true);
    connect(&m_progressSaveTimer, SIGNAL(timeout()), this, SLOT(onProgressSaveTimeout()));

    m_cachedContentChapter = -1;
    m_cacheDir = "/accounts/1000/shared/documents/.libris_covers";
    QDir().mkpath(m_cacheDir);

    // Initialize Libris PDF & DjVu parser engine
    m_pdfParser = new PdfParser(this);
    m_pdfParser->setAutoCrop(m_isPdfAutoCrop);
    updateReadingHistoryModel();

    // Initialize WiFi Drop Server for wireless LAN book uploads
    m_wifiDropServer = new WiFiDropServer(this);
    connect(m_wifiDropServer, SIGNAL(bookUploaded(QString,QString,qint64)),
            this, SLOT(onBookUploadedFromWifi(QString,QString,qint64)));

    // Detect actual hardware screen width for 100% pixel-perfect fit
    bb::device::DisplayInfo display;
    m_screenWidth = display.pixelSize().width();
    m_screenHeight = display.pixelSize().height();
    if (m_screenWidth <= 0) {
        m_screenWidth = 720;
    }
    if (m_screenHeight <= 0) {
        m_screenHeight = 720;
    }
    m_pdfParser->setScreenWidth(m_screenWidth);

    connect(m_pdfParser, SIGNAL(pageRendered(int)), this, SLOT(onPdfPageRendered(int)));
    connect(m_pdfParser, SIGNAL(streamPageRendered(int)), this, SLOT(onPdfStreamPageRendered(int)));
    connect(m_pdfParser, SIGNAL(needIndicator(bool)), this, SLOT(onPdfIndicatorRequested(bool)));
    connect(m_pdfParser, SIGNAL(errorWhileOpening(int)),
            this, SLOT(onPdfError(int)));
    connect(m_pdfParser, SIGNAL(fileOpened(QString)), this, SLOT(onPdfFileOpened(QString)));

    // Initialize TTS Hybrid Engine
    m_tts = new TtsManager(this);
    connect(m_tts, SIGNAL(chapterFinished()), this, SLOT(onTtsChapterFinished()));
    connect(m_tts, SIGNAL(activeChanged(bool)), this, SLOT(onTtsActiveChangedForVolume(bool)));

    refreshCustomFonts();
    checkAutoNightTheme();

    // Watchdog timer for fail-safe book opening and page rendering
    m_watchdogTimer.setSingleShot(true);
    connect(&m_watchdogTimer, SIGNAL(timeout()), this, SLOT(onWatchdogTimeout()));

    // Watchdog timer for fail-safe library scanning (prevents frozen startup overlay)
    m_scanWatchdogTimer.setSingleShot(true);
    connect(&m_scanWatchdogTimer, SIGNAL(timeout()), this, SLOT(onScanWatchdogTimeout()));

    // Debounce timer for persistent backup sync
    m_persistSyncTimer.setSingleShot(true);
    connect(&m_persistSyncTimer, SIGNAL(timeout()), this, SLOT(onPersistentSyncTimeout()));

    m_paragraphIndent = "normal";
    m_paragraphSpacing = "normal";
    m_readerFontWeight = 400;
    m_darkTextTone = "bright";
    m_showFooterProgressBar = true;
    m_showFooterTimeLeft = true;
    m_eyeRestMinutes = 0;
    m_invertVolumeKeys = false;
    m_sessionReadingSeconds = 0;
    m_bionicReading = false;
    m_onboardingCompleted = false;
    m_pdfPagesModel = new bb::cascades::ArrayDataModel(this);
    m_pdfZoomMode = false;

    // Load persisted library cache immediately for instant startup without blocking scan
    loadLibraryCache();
    bool hadEmptyLibrary = m_allBooks.isEmpty();

    // Ship bundled lightweight sample books once for immediate multi-format testing
    installBundledSamplesIfNeeded();

    // Run full library scan on startup: blocking overlay if empty, silent background refresh if cache existed
    if (hadEmptyLibrary) {
        QTimer::singleShot(300, this, SLOT(scanLibrary()));
    } else {
        QTimer::singleShot(800, this, SLOT(scanLibraryQuiet()));
    }
}

QString BookManager::appVersion() const
{
    return QString::fromLatin1(LIBRIS_APP_VERSION);
}

QString BookManager::buildNumber() const
{
    return QString::fromLatin1(LIBRIS_BUILD_NUMBER);
}

QString BookManager::changelogText() const
{
    QString fileName = (m_language == "en") ? "changelog_en.txt" : "changelog_vi.txt";
    QStringList candidates;
    candidates << ("app/native/assets/" + fileName)
               << (QCoreApplication::applicationDirPath() + "/../app/native/assets/" + fileName)
               << (QDir::currentPath() + "/app/native/assets/" + fileName)
               << (QDir::currentPath() + "/assets/" + fileName);

    foreach (const QString &p, candidates) {
        QFile f(p);
        if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(f.readAll().constData()).trimmed();
            f.close();
            if (!content.isEmpty()) {
                return content;
            }
        }
    }

    return (m_language == "en")
           ? QString("Version %1 (Build %2)").arg(appVersion(), buildNumber())
           : QString("Phiên bản %1 (Build %2)").arg(appVersion(), buildNumber());
}

void BookManager::installBundledSamplesIfNeeded()
{
    QSettings settings("Libris", "Libris");
    if (settings.value("samples/installed_v1", false).toBool()) {
        return;
    }

    QStringList srcCandidates;
    srcCandidates << "app/native/assets/samples"
                  << (QCoreApplication::applicationDirPath() + "/../app/native/assets/samples")
                  << (QDir::currentPath() + "/app/native/assets/samples");

    QString srcDirPath;
    foreach (const QString &cand, srcCandidates) {
        QDir d(cand);
        if (d.exists() && !d.entryList(QDir::Files).isEmpty()) {
            srcDirPath = d.absolutePath();
            break;
        }
    }
    if (srcDirPath.isEmpty()) {
        return;
    }

    QString baseBooksDir = QDir("/accounts/1000/shared").exists()
                           ? "/accounts/1000/shared/books"
                           : QDir::homePath();
    QString destDirPath = baseBooksDir + "/Libris_Samples";
    if (!QDir().mkpath(destDirPath)) {
        return;
    }

    QDir srcDir(srcDirPath);
    QFileInfoList sampleFiles = srcDir.entryInfoList(QDir::Files | QDir::Readable);
    QStringList installedPaths;
    foreach (const QFileInfo &sfi, sampleFiles) {
        QString destFile = destDirPath + "/" + sfi.fileName();
        if (!QFile::exists(destFile)) {
            if (QFile::copy(sfi.absoluteFilePath(), destFile)) {
                installedPaths.append(destFile);
            }
        } else {
            installedPaths.append(destFile);
        }
    }

    settings.setValue("samples/installed_v1", true);
    settings.sync();

    foreach (const QString &p, installedPaths) {
        registerBookFile(p);
    }
}

template <typename WorkerT>
static void retireWorkerThread(WorkerT *&worker, QObject *receiver, bool waitBrieflyOnShutdown = false)
{
    if (!worker) return;
    worker->cancel();
    QObject::disconnect(worker, 0, receiver, 0);
    worker->setParent(NULL);
    QObject::connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    if (worker->isRunning()) {
        if (waitBrieflyOnShutdown && worker->wait(250)) {
            QObject::disconnect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
            delete worker;
        }
    } else {
        QObject::disconnect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
        delete worker;
    }
    worker = NULL;
}

void BookManager::cancelWorkers()
{
    retireWorkerThread(m_coverWorker, this, true);
    retireWorkerThread(m_searchWorker, this, true);
    retireWorkerThread(m_scanWorker, this, true);
    if (m_pdfParser) {
        m_pdfParser->prepareForDestruction();
    }
    if (m_wifiDropServer) {
        m_wifiDropServer->stop();
    }
}

BookManager::~BookManager()
{
    if (m_volUpWatcher) {
        delete m_volUpWatcher;
        m_volUpWatcher = NULL;
    }
    if (m_volDownWatcher) {
        delete m_volDownWatcher;
        m_volDownWatcher = NULL;
    }
    m_scanWatchdogTimer.stop();
    m_watchdogTimer.stop();
    m_persistSyncTimer.stop();
    if (!m_shutdownSaved) {
        saveCurrentProgress();
        syncToPersistentStorage();
    }

    cancelWorkers();
}

bb::cascades::DataModel* BookManager::model() const
{
    return m_model;
}

int BookManager::bookCount() const
{
    return m_model->size();
}

int BookManager::totalScannedBooks() const
{
    return m_allBooks.size();
}

QString BookManager::language() const
{
    return m_language;
}

void BookManager::setLanguage(const QString &lang)
{
    if ((lang == "vi" || lang == "en") && lang != m_language) {
        m_language = lang;
        if (m_translator) {
            QCoreApplication::removeTranslator(m_translator);
            m_translator->setLanguage(lang);
            QCoreApplication::installTranslator(m_translator);
            QCoreApplication::postEvent(QCoreApplication::instance(), new QEvent(QEvent::LanguageChange));
        }
        m_cachedContentChapter = -1;
        emit languageChanged();
        if (!m_currentFilePath.isEmpty()) {
            emit currentChapterChanged();
        }
        applyFilterAndSort();
        updateReadingHistoryModel();
        refreshBrowser();
        if (m_opdsManager) {
            m_opdsManager->loadCatalogs();
        }
        saveSettings();
        emit statsChanged();
    }
}

QString BookManager::tr(const QString &sourceText) const
{
    if (m_translator) {
        return m_translator->translate(0, sourceText.toUtf8().constData(), 0);
    }
    return sourceText;
}

QString BookManager::libraryViewMode() const
{
    return m_libraryViewMode;
}

void BookManager::setLibraryViewMode(const QString &mode)
{
    if (m_libraryViewMode != mode) {
        m_libraryViewMode = mode;
        emit libraryViewModeChanged();
        saveSettings();
    }
}

void BookManager::toggleLibraryViewMode()
{
    if (m_libraryViewMode == "mosaic") {
        setLibraryViewMode("detailed");
    } else {
        setLibraryViewMode("mosaic");
    }
}

bb::cascades::DataModel* BookManager::folderModel() const
{
    return m_folderModel;
}

QString BookManager::libraryBrowseMode() const
{
    return m_libraryBrowseMode;
}

void BookManager::setLibraryBrowseMode(const QString &mode)
{
    if (m_libraryBrowseMode != mode) {
        m_libraryBrowseMode = mode;
        emit libraryBrowseModeChanged();
        applyFilterAndSort();
        saveSettings();
    }
}

void BookManager::toggleLibraryBrowseMode()
{
    if (m_libraryBrowseMode == "folder") {
        setLibraryBrowseMode("all");
    } else {
        setLibraryBrowseMode("folder");
    }
}

QString BookManager::selectedFolderPath() const
{
    return m_selectedFolderPath;
}

QString BookManager::selectedFolderName() const
{
    return m_selectedFolderName;
}

void BookManager::selectFolder(const QString &folderPath)
{
    m_selectedFolderPath = folderPath;
    m_selectedFolderName = QFileInfo(folderPath).fileName();
    emit selectedFolderChanged();
    applyFilterAndSort();
}

void BookManager::clearSelectedFolder()
{
    m_selectedFolderPath.clear();
    m_selectedFolderName.clear();
    emit selectedFolderChanged();
    applyFilterAndSort();
}

int BookManager::screenWidth() const
{
    return m_screenWidth;
}

int BookManager::screenHeight() const
{
    return m_screenHeight;
}

bool BookManager::isSquareScreen() const
{
    return (m_screenWidth > 0 && m_screenHeight > 0 && qAbs(m_screenWidth - m_screenHeight) <= 120);
}

QString BookManager::sortBy() const
{
    return m_sortBy;
}

void BookManager::setSortBy(const QString &sortBy)
{
    if (m_sortBy != sortBy) {
        m_sortBy = sortBy;
        emit sortByChanged();
        applyFilterAndSort();
        saveSettings();
    }
}

QString BookManager::groupBy() const
{
    return m_groupBy;
}

void BookManager::setGroupBy(const QString &groupBy)
{
    if (m_groupBy != groupBy) {
        m_groupBy = groupBy;
        emit groupByChanged();
        applyFilterAndSort();
        saveSettings();
    }
}

QString BookManager::currentBookSeries() const
{
    return m_currentBookSeries;
}

int BookManager::currentBookSeriesIndex() const
{
    return m_currentBookSeriesIndex;
}

QString BookManager::formatFilter() const
{
    return m_formatFilter;
}

void BookManager::setFormatFilter(const QString &format)
{
    if (m_formatFilter != format) {
        m_formatFilter = format;
        emit formatFilterChanged();
        applyFilterAndSort();
        saveSettings();
    }
}

QString BookManager::searchQuery() const
{
    return m_searchQuery;
}

void BookManager::setSearchQuery(const QString &query)
{
    if (m_searchQuery != query) {
        m_searchQuery = query;
        emit searchQueryChanged();
        applyFilterAndSort();
    }
}

QString BookManager::categoryFilter() const
{
    return m_categoryFilter;
}

void BookManager::setCategoryFilter(const QString &category)
{
    if (m_categoryFilter != category) {
        m_categoryFilter = category;
        emit categoryFilterChanged();
        applyFilterAndSort();
    }
}

bool BookManager::isFavorite(const QString &filePath) const
{
    if (filePath.isEmpty()) return false;
    QSettings settings("Libris", "Libris");
    return settings.value(QString("favorites/%1").arg(filePath), false).toBool();
}

bool BookManager::toggleFavorite(const QString &filePath)
{
    if (filePath.isEmpty()) return false;
    QSettings settings("Libris", "Libris");
    bool current = settings.value(QString("favorites/%1").arg(filePath), false).toBool();
    bool next = !current;
    settings.setValue(QString("favorites/%1").arg(filePath), next);

    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            m_allBooks[i]["isFavorite"] = next;
            break;
        }
    }
    applyFilterAndSort();
    refreshBrowser();
    settings.sync();
    saveLibraryCache();
    syncToPersistentStorage();
    return next;
}

bool BookManager::isReadingList(const QString &filePath) const
{
    if (filePath.isEmpty()) return false;
    QSettings settings("Libris", "Libris");
    return settings.value(QString("readingList/%1").arg(filePath), false).toBool();
}

bool BookManager::toggleReadingList(const QString &filePath)
{
    if (filePath.isEmpty()) return false;
    QSettings settings("Libris", "Libris");
    bool current = settings.value(QString("readingList/%1").arg(filePath), false).toBool();
    bool next = !current;
    settings.setValue(QString("readingList/%1").arg(filePath), next);

    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            m_allBooks[i]["isReadingList"] = next;
            break;
        }
    }
    applyFilterAndSort();
    refreshBrowser();
    settings.sync();
    saveLibraryCache();
    syncToPersistentStorage();
    return next;
}

bool BookManager::toggleBookCompleted(const QString &filePath)
{
    if (filePath.isEmpty()) return false;
    QSettings settings("Libris", "Libris");
    int currentProg = settings.value(QString("progressPct/%1").arg(filePath), 0).toInt();
    int newProg = (currentProg >= 100) ? 0 : 100;

    settings.setValue(QString("progressPct/%1").arg(filePath), newProg);
    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            m_allBooks[i]["progress"] = newProg;
            m_allBooks[i]["progressStr"] = QString("%1%").arg(newProg);
            m_allBooks[i]["isCompleted"] = (newProg >= 100);
            break;
        }
    }
    applyFilterAndSort();
    refreshStats();
    refreshBrowser();
    settings.sync();
    saveLibraryCache();
    syncToPersistentStorage();
    return (newProg >= 100);
}

void BookManager::removeBookFromLibrary(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    QSettings settings("Libris", "Libris");
    settings.setValue(QString("hidden/%1").arg(filePath), true);

    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            m_allBooks.removeAt(i);
            break;
        }
    }
    applyFilterAndSort();
    updateFolderList();
    refreshStats();
    refreshBrowser();
    settings.sync();
    saveLibraryCache();
    syncToPersistentStorage();
}

static bool isAllowedUserStoragePath(const QString &p, const QString &homeDir)
{
    return !p.isEmpty() && (
        p.startsWith("/accounts/1000/shared/") ||
        p.startsWith("/accounts/1000/removable/") ||
        (!homeDir.isEmpty() && p.startsWith(homeDir + "/"))
    );
}

bool BookManager::deleteBookFile(const QString &filePath)
{
    if (filePath.isEmpty()) return false;
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) return false;

    QString absPath = fi.absoluteFilePath();
    QString canonPath = fi.canonicalFilePath();
    QString homeDir = QDir::homePath();
    if (!isAllowedUserStoragePath(absPath, homeDir) && !isAllowedUserStoragePath(canonPath, homeDir)) {
        qWarning() << "[BookManager] Refused deleting file outside user storage:" << filePath;
        return false;
    }

    // Release any open parser/file handles if this book is currently active
    QString curCanon = m_currentFilePath.isEmpty() ? QString() : QFileInfo(m_currentFilePath).canonicalFilePath();
    if (m_currentFilePath == filePath || (!canonPath.isEmpty() && curCanon == canonPath)) {
        stopTts();
        m_epubParser.close();
        m_txtParser.close();
        m_fb2Parser.close();
        m_cbzParser.close();
        m_mobiParser.close();
        if (m_pdfParser) {
            m_pdfParser->close();
        }
        if (m_pdfPagesModel) {
            m_pdfPagesModel->clear();
        }
        m_currentFilePath.clear();
        m_currentFormat.clear();
        m_cachedContentHtml.clear();
        m_cachedContentChapter = -1;
    }

    if (!QFile::remove(filePath)) {
        qWarning() << "[BookManager] QFile::remove failed for:" << filePath;
        return false;
    }

    // Remove cached cover thumbnail if present
    if (!m_cacheDir.isEmpty()) {
        QString coverPng = m_cacheDir + "/" + QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex() + ".png";
        if (QFile::exists(coverPng)) {
            QFile::remove(coverPng);
        }
    }

    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            m_allBooks.removeAt(i);
            break;
        }
    }

    QSettings settings("Libris", "Libris");
    settings.remove(QString("hidden/%1").arg(filePath));
    settings.remove(QString("opened/%1").arg(filePath));
    settings.remove(QString("progress/%1").arg(filePath));
    settings.remove(QString("progressPct/%1").arg(filePath));
    settings.remove(QString("pageInChapter/%1").arg(filePath));
    settings.remove(QString("totalPagesInChapter/%1").arg(filePath));
    settings.remove(QString("scrollPos/%1").arg(filePath));
    settings.remove(QString("completed/%1").arg(filePath));
    settings.remove(QString("favorite/%1").arg(filePath));
    settings.remove(QString("readingList/%1").arg(filePath));
    settings.remove(QString("rating/%1").arg(filePath));
    settings.remove(QString("review/%1").arg(filePath));
    settings.remove(QString("stats/bookSeconds/%1").arg(filePath));
    settings.sync();

    removeFromRecent(filePath);
    applyFilterAndSort();
    updateFolderList();
    refreshStats();
    refreshBrowser();
    saveLibraryCache();
    syncToPersistentStorage();
    emit bookCountChanged();
    return true;
}

void BookManager::removeFromRecent(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    QSettings settings("Libris", "Libris");
    settings.remove(QString("lastReadTime/%1").arg(filePath));

    if (m_lastBookPath == filePath) {
        m_lastBookPath.clear();
        m_lastBookTitle.clear();
        m_lastBookAuthor.clear();
        m_lastBookCover.clear();
        m_lastBookProgress = 0;
        m_lastBookFormat.clear();
        settings.remove("lastBook/filePath");
        settings.remove("lastBook/title");
        settings.remove("lastBook/author");
        settings.remove("lastBook/cover");
        settings.remove("lastBook/format");
        settings.remove("lastBook/progress");
        emit lastBookChanged();
    }

    updateReadingHistoryModel();
    refreshStats();
    settings.sync();
    syncToPersistentStorage();
}

void BookManager::clearReadingHistory()
{
    QSettings settings("Libris", "Libris");

    // 1. Remove all lastReadTime, progress, opened and legacy reading state keys
    QStringList allKeys = settings.allKeys();
    foreach (const QString &key, allKeys) {
        if (key.startsWith("lastReadTime/") ||
            key.startsWith("progress/") ||
            key.startsWith("pageInChapter/") ||
            key.startsWith("totalPagesInChapter/") ||
            key.startsWith("scrollPos/") ||
            key.startsWith("progressPct/") ||
            key.startsWith("opened/") ||
            key.startsWith("lastReadChapter/") ||
            key.startsWith("lastReadScroll/") ||
            key.startsWith("lastReadTargetScroll/") ||
            key.startsWith("lastReadTargetPage/") ||
            key.startsWith("completed/")) {
            settings.remove(key);
        }
    }

    // 2. Clear last active book hero banner
    m_lastBookPath.clear();
    m_lastBookTitle.clear();
    m_lastBookAuthor.clear();
    m_lastBookCover.clear();
    m_lastBookProgress = 0;
    m_lastBookFormat.clear();
    settings.remove("lastBook/filePath");
    settings.remove("lastBook/title");
    settings.remove("lastBook/author");
    settings.remove("lastBook/cover");
    settings.remove("lastBook/format");
    settings.remove("lastBook/progress");
    emit lastBookChanged();

    // 3. Reset progress & opened flags in memory
    for (int i = 0; i < m_allBooks.size(); ++i) {
        m_allBooks[i]["progress"] = 0;
        m_allBooks[i]["progressStr"] = QString("0%");
        m_allBooks[i]["isCompleted"] = false;
        m_allBooks[i]["isOpened"] = false;
    }

    // 4. Update data models & views
    if (m_readingHistoryModel) {
        m_readingHistoryModel->clear();
    }
    applyFilterAndSort();
    updateFolderList();
    updateReadingHistoryModel();
    refreshStats();
    refreshBrowser();

    settings.sync();
    saveLibraryCache();
    syncToPersistentStorage();
}

void BookManager::markAsUnread(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    QSettings settings("Libris", "Libris");
    settings.setValue(QString("progressPct/%1").arg(filePath), 0);
    settings.remove(QString("progress/%1").arg(filePath));
    settings.remove(QString("pageInChapter/%1").arg(filePath));
    settings.remove(QString("totalPagesInChapter/%1").arg(filePath));
    settings.remove(QString("scrollPos/%1").arg(filePath));
    settings.remove(QString("opened/%1").arg(filePath));
    settings.remove(QString("lastReadTime/%1").arg(filePath));
    settings.remove(QString("lastReadChapter/%1").arg(filePath));
    settings.remove(QString("lastReadScroll/%1").arg(filePath));
    settings.remove(QString("lastReadTargetScroll/%1").arg(filePath));
    settings.remove(QString("lastReadTargetPage/%1").arg(filePath));
    settings.setValue(QString("completed/%1").arg(filePath), false);

    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            m_allBooks[i]["progress"] = 0;
            m_allBooks[i]["progressStr"] = QString("0%");
            m_allBooks[i]["isCompleted"] = false;
            m_allBooks[i]["isOpened"] = false;
            break;
        }
    }

    if (m_lastBookPath == filePath) {
        m_lastBookProgress = 0;
        emit lastBookChanged();
    }

    applyFilterAndSort();
    updateReadingHistoryModel();
    refreshStats();
    settings.sync();
    saveLibraryCache();
    syncToPersistentStorage();
}

void BookManager::resetFilters()
{
    m_searchQuery = "";
    m_formatFilter = "all";
    m_categoryFilter = "all";
    m_sortBy = "date_desc";
    m_groupBy = "none";
    emit searchQueryChanged();
    emit formatFilterChanged();
    emit categoryFilterChanged();
    emit sortByChanged();
    emit groupByChanged();
    applyFilterAndSort();
    saveSettings();
}

QString BookManager::currentBookTitle() const
{
    return m_currentBookTitle;
}

QString BookManager::currentBookAuthor() const
{
    return m_currentBookAuthor;
}

QString BookManager::currentCoverPath() const
{
    if (!m_currentCoverPath.isEmpty() && QFile::exists(m_currentCoverPath)) {
        return m_currentCoverPath;
    }
    return QString();
}


QString BookManager::currentBookDescription() const
{
    return m_currentBookDescription;
}

QString BookManager::currentBookGenre() const
{
    QString g = m_currentBookGenre.trimmed();
    QString gLower = g.toLower();
    if (gLower.contains("http://") || gLower.contains("https://") ||
        gLower.contains("www.") || gLower.contains(".com") ||
        gLower.contains("converter") || gLower.contains("neevia") ||
        gLower.contains("batch") || gLower.contains("calibre")) {
        return QString();
    }
    return g;
}

QString BookManager::currentBookFileSize() const
{
    return m_currentBookFileSize;
}

QString BookManager::currentBookDateModified() const
{
    return m_currentBookDateModified;
}

QString BookManager::currentFilePath() const
{
    return m_currentFilePath;
}

QString BookManager::currentBookTimeSpentStr() const
{
    int secs = m_bookReadingSeconds.value(m_currentFilePath, 0);
    return formatDuration(secs);
}

int BookManager::currentChapterIndex() const
{
    return m_currentChapterIndex;
}

int BookManager::totalChapters() const
{
    return m_totalChapters;
}

QString BookManager::currentFormat() const
{
    return m_currentFormat;
}

bool BookManager::isPdf() const
{
    return m_currentFormat == "pdf" || m_currentFormat == "djvu";
}

bb::cascades::Image BookManager::currentPdfImage() const
{
    return m_currentPdfImage;
}

QString BookManager::currentPdfPagePath() const
{
    return m_currentPdfPagePath;
}

bool BookManager::isPdfInverted() const
{
    return m_pdfInverted;
}

QString BookManager::currentChapterTitle() const
{
    if (isPdf()) {
        return QString("Trang %1 / %2").arg(m_currentChapterIndex).arg(m_totalChapters);
    }

    // First check m_tocList for an enriched title for the current chapter
    for (int i = 0; i < m_tocList.size(); ++i) {
        QVariantMap item = m_tocList[i].toMap();
        int chIdx = item.value("chapterIndex", item.value("index", -1)).toInt();
        if (chIdx == m_currentChapterIndex) {
            QString tocTitle = item.value("title").toString().trimmed();
            if (!tocTitle.isEmpty() && !isGenericChapterTitle(tocTitle)) {
                return tocTitle;
            }
        }
    }

    QString title;
    if (m_currentFormat == "epub") {
        QList<EpubChapter> ch = m_epubParser.chapters();
        if (m_currentChapterIndex >= 0 && m_currentChapterIndex < ch.size()) {
            title = ch[m_currentChapterIndex].title.trimmed();
        }
    } else if (m_currentFormat == "txt") {
        QList<EpubChapter> ch = m_txtParser.chapters();
        if (m_currentChapterIndex >= 0 && m_currentChapterIndex < ch.size()) {
            title = ch[m_currentChapterIndex].title.trimmed();
        }
    } else if (m_currentFormat == "fb2") {
        QList<EpubChapter> ch = m_fb2Parser.chapters();
        if (m_currentChapterIndex >= 0 && m_currentChapterIndex < ch.size()) {
            title = ch[m_currentChapterIndex].title.trimmed();
        }
    } else if (m_currentFormat == "cbz") {
        QList<BookChapter> ch = m_cbzParser.chapters();
        if (m_currentChapterIndex >= 0 && m_currentChapterIndex < ch.size()) {
            title = ch[m_currentChapterIndex].title.trimmed();
        }
    } else if (m_currentFormat == "mobi") {
        QList<BookChapter> ch = m_mobiParser.chapters();
        if (m_currentChapterIndex >= 0 && m_currentChapterIndex < ch.size()) {
            title = ch[m_currentChapterIndex].title.trimmed();
        }
    }

    if (!title.isEmpty()) {
        return title;
    }

    if (m_currentChapterIndex == 0) {
        return tr("Bìa sách");
    }
    return QString("Chương %1").arg(m_currentChapterIndex);
}

QString BookManager::currentContent() const
{
    if (m_cachedContentChapter == m_currentChapterIndex &&
        m_cachedContentTheme == m_theme &&
        m_cachedContentFontSize == m_fontSize &&
        m_cachedContentFontFamily == m_fontFamily &&
        m_cachedContentSpacing == m_lineSpacing &&
        m_cachedContentMode == m_readingMode &&
        !m_cachedContentHtml.isEmpty()) {
        return m_cachedContentHtml;
    }

    QString rawBody;
    QString bookCss;
    if (isPdf()) {
        if (m_isPdfReflow) {
            if (m_currentPdfText.trimmed().isEmpty()) {
                rawBody = QString("<div style=\"text-align: center; padding: 40px 16px; font-family: -apple-system, sans-serif;\">"
                                  "<h3 style=\"color: #00A8DF; margin-bottom: 12px;\">%1</h3>"
                                  "<p style=\"color: #888888; font-size: 16px; line-height: 1.5;\">%2</p>"
                                  "<p style=\"color: #AAAAAA; font-size: 14px; margin-top: 20px;\">%3</p>"
                                  "</div>")
                          .arg(tr("Không tìm thấy lớp chữ OCR"))
                          .arg(tr("Trang PDF này được quét dạng hình ảnh và chưa có lớp văn bản (OCR text layer)."))
                          .arg(tr("Bấm nút Reflow trên thanh công cụ để trở về chế độ xem trang ảnh gốc."));
            } else {
                QString cleaned = m_currentPdfText;
                cleaned.remove(QChar(0x0000));
                cleaned.remove(QChar(0xFEFF));
                cleaned.remove(QChar(0xFFFD));
                cleaned.replace('\r', "");

                // Fix words hyphenated across line breaks: e.g. "thông-\ntin" -> "thôngtin"
                QRegExp hyphenWrap("([\\w\\dÀ-ỹ])-\\s*\\n\\s*([\\w\\dÀ-ỹ])");
                cleaned.replace(hyphenWrap, "\\1\\2");

                QStringList rawLines = cleaned.split('\n');
                QString curPara;
                for (int i = 0; i < rawLines.size(); ++i) {
                    QString line = rawLines[i].trimmed();
                    if (line.isEmpty()) {
                        if (!curPara.isEmpty()) {
                            QString p = curPara.trimmed();
                            p.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
                            p.replace(QRegExp("\\s+"), " ");
                            rawBody += "<p style=\"text-indent: 1.8em; margin: 0.6em 0;\">" + p + "</p>";
                            curPara.clear();
                        }
                        continue;
                    }

                    if (!curPara.isEmpty()) {
                        curPara += " " + line;
                    } else {
                        curPara = line;
                    }

                    QChar lastC = line.isEmpty() ? QChar() : line.at(line.length() - 1);
                    bool isTerminator = (lastC == '.' || lastC == '!' || lastC == '?' || lastC == ':' || lastC == '"' || lastC == 0x201D);
                    if (isTerminator && (line.length() < 55 || (i + 1 < rawLines.size() && rawLines[i + 1].trimmed().isEmpty()))) {
                        QString p = curPara.trimmed();
                        p.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
                        p.replace(QRegExp("\\s+"), " ");
                        rawBody += "<p style=\"text-indent: 1.8em; margin: 0.6em 0;\">" + p + "</p>";
                        curPara.clear();
                    }
                }

                if (!curPara.isEmpty()) {
                    QString p = curPara.trimmed();
                    p.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
                    p.replace(QRegExp("\\s+"), " ");
                    rawBody += "<p style=\"text-indent: 1.8em; margin: 0.6em 0;\">" + p + "</p>";
                }
            }
        } else {
            return QString();
        }
    } else if (m_currentFormat == "epub") {
        rawBody = m_epubParser.getChapterContent(m_currentChapterIndex, m_cacheDir);
        bookCss = m_epubParser.bookCss(m_cacheDir);
    } else if (m_currentFormat == "txt") {
        rawBody = m_txtParser.getChapterContent(m_currentChapterIndex);
    } else if (m_currentFormat == "fb2") {
        rawBody = m_fb2Parser.getChapterContent(m_currentChapterIndex);
    } else if (m_currentFormat == "cbz") {
        rawBody = m_cbzParser.getChapterContent(m_currentChapterIndex, m_cacheDir);
    } else if (m_currentFormat == "mobi") {
        rawBody = m_mobiParser.getChapterContent(m_currentChapterIndex, m_cacheDir);
        bookCss = m_mobiParser.bookCss();
    } else {
        return QString();
    }

    if (rawBody.isEmpty()) {
        return QString();
    }

    // Dynamic colors according to selected theme
    QString bgColor = "#000000";
    QString textColor = (m_darkTextTone == "dim") ? "#8E887D" : ((m_darkTextTone == "soft") ? "#B8B2A6" : "#DDDDDD");
    QString linkColor = "#4CA3FF";

    if (m_theme == "sepia") {
        bgColor = "#F4ECD8";
        textColor = "#3E2D1D";
        linkColor = "#8B4513";
    } else if (m_theme == "light") {
        bgColor = "#FFFFFF";
        textColor = "#111111";
        linkColor = "#0066CC";
    }

    // Extract inner body content if rawBody is a full HTML document
    QString content = rawBody;
    int bodyStart = content.indexOf("<body", Qt::CaseInsensitive);
    if (bodyStart != -1) {
        int bodyOpenClose = content.indexOf(">", bodyStart);
        if (bodyOpenClose != -1) {
            int bodyEnd = content.indexOf("</body>", Qt::CaseInsensitive);
            if (bodyEnd != -1) {
                content = content.mid(bodyOpenClose + 1, bodyEnd - (bodyOpenClose + 1));
            } else {
                content = content.mid(bodyOpenClose + 1);
            }
        }
    }
    if (content.trimmed().isEmpty()) {
        content = rawBody;
    }

    // Strip unsafe or unsupported scripts from book content
    content.remove(minimalRx("<script[^>]*>[\\s\\S]*</script>", Qt::CaseInsensitive));

    // Also collect any chapter-specific <style> tags from rawBody to preserve typography
    QRegExp styleRx = minimalRx("<style[^>]*>([\\s\\S]*)</style>", Qt::CaseInsensitive);
    int sPos = 0;
    QString chapterStyles;
    while ((sPos = styleRx.indexIn(rawBody, sPos)) != -1) {
        chapterStyles += "\n" + styleRx.cap(1);
        sPos += styleRx.matchedLength();
    }

    // Fallback if chapter has no text words and no images
    QString strippedText = content;
    strippedText.remove(QRegExp("<[^>]*>"));
    strippedText = strippedText.trimmed();
    bool hasImages = content.contains("<img", Qt::CaseInsensitive) || 
                     content.contains("<image", Qt::CaseInsensitive) || 
                     content.contains("<svg", Qt::CaseInsensitive);

    if (strippedText.isEmpty() && !hasImages) {
        if (m_currentChapterIndex == 0 && !m_currentCoverPath.isEmpty() && QFile::exists(m_currentCoverPath)) {
            QString coverSrc = fileToDataUri(m_currentCoverPath);
            if (coverSrc.isEmpty()) {
                coverSrc = QString("file://%1").arg(m_currentCoverPath);
            }
            content = QString("<div style=\"width:100%;min-height:85vh;display:-webkit-box;display:flex;-webkit-box-pack:center;justify-content:center;-webkit-box-align:center;align-items:center;text-align:center;box-sizing:border-box;padding:16px;\">"
                              "<img src=\"%1\" style=\"max-height:82vh;max-width:90vw;margin:auto;display:block;border-radius:6px;box-shadow:0 4px 18px rgba(0,0,0,0.6);\"/>"
                              "</div>").arg(coverSrc);
        } else {
            content = QString("<div style=\"text-align: center; padding-top: 25vh;\">"
                              "<h2 style=\"font-size: 1.4em; margin-bottom: 12px;\">%1</h2>"
                              "<p style=\"font-size: 1.0em; opacity: 0.85;\">%2</p>"
                              "</div>")
                      .arg(m_currentBookTitle)
                      .arg(currentChapterTitle());
        }
    }

    double lh = 1.65;
    if (m_lineSpacing == "tight") lh = 1.35;
    else if (m_lineSpacing == "loose") lh = 2.0;

    // Base viewport styles and layout rules
    int marginL = m_marginLeft;
    int marginR = m_marginRight;
    int marginT = m_marginTop;
    int marginB = m_marginBottom;
    int colGap = marginL + marginR;
    int screenW = (m_screenWidth > 0) ? m_screenWidth : 720;
    bool twoCol = isTwoColumnActive() && m_currentFormat != "cbz";
    int innerGap = colGap;
    int colW = twoCol ? qMax(100, (screenW - marginL - marginR - innerGap) / 2) : (screenW - colGap);
    if (colW < 100) colW = screenW;

    QString colRuleCss;
    if (twoCol) {
        QString ruleColor = (m_theme == "dark") ? "rgba(255,255,255,0.14)" : (m_theme == "sepia" ? "rgba(120,90,60,0.18)" : "rgba(0,0,0,0.12)");
        colRuleCss = QString("-webkit-column-rule: 1px solid %1; column-rule: 1px solid %1; ").arg(ruleColor);
    }

    QString html;
    html += "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no, minimum-scale=1.0, maximum-scale=1.0\"/>";
    html += "<style id=\"base-viewport-style\">";
    html += "*, *:before, *:after { box-sizing: border-box; }";
    html += "html { width: 100% !important; height: 100% !important; min-height: 100%; margin: 0 !important; padding: 0 !important; }";
    html += "body { margin: 0 !important; padding: 0 !important; word-wrap: break-word; -webkit-text-size-adjust: none; }";
    html += "img { max-width: 100% !important; max-height: 85vh !important; height: auto !important; display: block; margin: 10px auto; }";
    html += "svg { max-width: 100% !important; max-height: 85vh !important; display: block; margin: 0 auto; }";
    html += "image { max-width: 100% !important; max-height: 100% !important; }";
    html += "table, pre, code { max-width: 100% !important; }";
    html += "pre { white-space: pre-wrap !important; word-break: break-all !important; }";

    if (m_currentFormat == "cbz") {
        html += ".comic-box { width: 100% !important; height: 100% !important; display: -webkit-box; display: flex; -webkit-box-pack: center; justify-content: center; -webkit-box-align: center; align-items: center; margin: 0 !important; padding: 0 !important; } ";
        html += ".comic-img { max-width: 100% !important; max-height: 98vh !important; width: auto !important; height: auto !important; object-fit: contain; margin: auto !important; display: block !important; -webkit-column-break-inside: avoid; page-break-inside: avoid; } ";
    }

    if (m_readingMode == "paged") {
        html += "html, body { width: 100% !important; height: 100% !important; min-height: 100%; margin: 0 !important; padding: 0 !important; overflow: hidden !important; -webkit-user-select: text !important; user-select: text !important; } ";
        html += "#reader-viewport { "
             + QString("width: 100% !important; height: 100% !important; min-height: 100%; margin: 0; ")
             + QString("box-sizing: border-box; padding: %1px %2px %3px %4px; ")
                .arg(marginT).arg(marginR).arg(marginB).arg(marginL)
             + QString("overflow: hidden; ")
             + QString("-webkit-column-width: %1px; -webkit-column-gap: %2px; -webkit-column-fill: auto; ")
                .arg(colW).arg(innerGap)
             + QString("column-width: %1px; column-gap: %2px; column-fill: auto; ")
                .arg(colW).arg(innerGap)
             + colRuleCss
             + QString("-webkit-transform: translateZ(0); transform: translateZ(0); -webkit-backface-visibility: hidden; backface-visibility: hidden; ")
             + QString("} ");
        html += "#reader-viewport::-webkit-scrollbar { display: none; width: 0; height: 0; } ";
    } else {
        html += "html, body { width: 100% !important; height: 100% !important; margin: 0 !important; padding: 0 !important; overflow: hidden !important; -webkit-user-select: text !important; user-select: text !important; } ";
        html += "#reader-viewport { "
             + QString("width: 100% !important; height: 100% !important; margin: 0; ")
             + QString("box-sizing: border-box; padding: %1px %2px %3px %4px; ")
                .arg(marginT).arg(marginR).arg(marginB).arg(marginL)
             + QString("overflow-x: hidden; overflow-y: scroll; -webkit-overflow-scrolling: touch; ")
             + QString("-webkit-column-width: auto !important; column-width: auto !important; ")
             + QString("-webkit-column-count: auto !important; column-count: auto !important; ")
             + QString("-webkit-column-gap: 0 !important; column-gap: 0 !important; ")
             + QString("} ")
             + QString("#reader-viewport::-webkit-scrollbar { width: 4px; } ")
             + QString("#reader-viewport::-webkit-scrollbar-thumb { background: rgba(128,128,128,0.35); border-radius: 2px; } ");
    }

    html += "</style>";

    // Text selection & Search Highlight styles
    html += "<style id=\"selection-search-styles\">";
    html += "::selection { background-color: #00A8DF !important; color: #FFFFFF !important; } ";
    html += "::-webkit-selection { background-color: #00A8DF !important; color: #FFFFFF !important; } ";
    html += "mark.libris-search-match { background-color: #FFDE03 !important; color: #000000 !important; padding: 2px 1px; border-radius: 2px; } ";
    html += "mark.libris-search-current { background-color: #FF5722 !important; color: #FFFFFF !important; font-weight: bold; } ";
    html += "mark.libris-tts-active { background-color: rgba(0, 168, 223, 0.35) !important; color: inherit !important; border-radius: 4px; padding: 1px 3px; border-bottom: 2px solid #00A8DF !important; box-shadow: 0 0 10px rgba(0, 168, 223, 0.45); } ";
    html += "* { -webkit-touch-callout: none !important; } ";

    // User Highlights styles
    if (m_theme == "dark") {
        html += "mark.libris-highlight { cursor: pointer; border-radius: 2px; padding: 1px 2px; text-decoration: none !important; } ";
        html += "mark.libris-highlight.libris-hl-yellow { background-color: rgba(234, 179, 8, 0.45) !important; color: #FEF08A !important; border-bottom: 2px solid #EAB308; } ";
        html += "mark.libris-highlight.libris-hl-green { background-color: rgba(34, 197, 94, 0.45) !important; color: #BBF7D0 !important; border-bottom: 2px solid #22C55E; } ";
        html += "mark.libris-highlight.libris-hl-blue { background-color: rgba(56, 189, 248, 0.45) !important; color: #BAE6FD !important; border-bottom: 2px solid #38BDF8; } ";
        html += "mark.libris-highlight.libris-hl-pink { background-color: rgba(244, 114, 182, 0.45) !important; color: #FBCFE8 !important; border-bottom: 2px solid #F472B6; } ";
    } else {
        html += "mark.libris-highlight { cursor: pointer; border-radius: 2px; padding: 1px 2px; text-decoration: none !important; } ";
        html += "mark.libris-highlight.libris-hl-yellow { background-color: #FEF08A !important; color: #1E293B !important; } ";
        html += "mark.libris-highlight.libris-hl-green { background-color: #BBF7D0 !important; color: #14532D !important; } ";
        html += "mark.libris-highlight.libris-hl-blue { background-color: #BAE6FD !important; color: #0C4A6E !important; } ";
        html += "mark.libris-highlight.libris-hl-pink { background-color: #FBCFE8 !important; color: #831843 !important; } ";
    }

    // Focus Mode / Reading Ruler overlays (top and bottom dim bands leaving a clear reading window)
    html += ".libris-focus-mask { position: fixed; left: 0; right: 0; pointer-events: none; z-index: 9990; background: rgba(0,0,0,0.62); display: none; transition: height 0.12s ease; } ";
    html += "#libris-focus-top { top: 0; border-bottom: 1px solid rgba(212,163,89,0.55); } ";
    html += "#libris-focus-bottom { bottom: 0; border-top: 1px solid rgba(212,163,89,0.55); } ";
    html += "body.libris-focus-on .libris-focus-mask { display: block; } ";

    html += "</style>";

    // Inlined original book styles & embedded fonts
    if (!m_overridePublisherCss && (!bookCss.isEmpty() || !chapterStyles.isEmpty())) {
        html += "<style id=\"book-author-styles\">\n" + bookCss + "\n" + chapterStyles + "\n</style>";
    }

    // Adaptive Theme & Typography baseline
    QString fontFaceCss = currentFontFaceCss();
    QString fontCss = currentFontCss();
    QString alignCss = (m_textAlign == "left") ? "left" : "justify";

    html += QString("<style id=\"dynamic-theme-style\">");
    if (!fontFaceCss.isEmpty()) {
        html += fontFaceCss;
    }
    html += QString("body { ")
         + fontCss
         + QString("font-size: %1px; ").arg(m_fontSize)
         + QString("font-weight: %1; ").arg(m_readerFontWeight)
         + QString("line-height: %1; ").arg(lh)
         + QString("background-color: %1 !important; ").arg(bgColor)
         + QString("color: %2 !important; ").arg(textColor)
         + "}";
    html += QString("a { color: %1 !important; text-decoration: none; }").arg(linkColor);
    QString indentVal = (m_paragraphIndent == "none") ? "0" : ((m_paragraphIndent == "deep") ? "2.2em" : "1.5em");
    QString pSpaceVal = (m_paragraphSpacing == "compact") ? "0.25em 0" : ((m_paragraphSpacing == "wide") ? "1.0em 0" : "0.5em 0");
    html += QString("p:not([class]) { text-indent: %1; margin: %2; }").arg(indentVal, pSpaceVal);
    html += QString("p, li { text-align: %1 !important; }").arg(alignCss);

    if (m_theme == "dark") {
        html += ".banner, div[class*=\"box\"], div[class*=\"frame\"], div[class*=\"card\"], blockquote { "
                "background-color: #1A1A1A !important; "
                "border: 1px solid #444444 !important; "
                "border-radius: 10px; "
                "color: #DDDDDD !important; "
                "} "
                ".banner *, div[class*=\"box\"] *, div[class*=\"frame\"] *, div[class*=\"card\"] * { color: inherit; } "
                "font[color=\"#000000\"], font[color=\"black\"], font[color=\"#111111\"], font[color=\"#222222\"] { color: inherit !important; } "
                "hr, .divider { border-color: #444444 !important; background-color: #444444 !important; } ";
    } else if (m_theme == "sepia") {
        html += ".banner, div[class*=\"box\"], div[class*=\"frame\"], div[class*=\"card\"], blockquote { "
                "background-color: #EADFC8 !important; "
                "border: 1px solid #C0B098 !important; "
                "border-radius: 10px; "
                "color: #3E2D1D !important; "
                "} "
                ".banner *, div[class*=\"box\"] * { color: inherit; } "
                "font[color=\"#000000\"], font[color=\"black\"] { color: inherit !important; } ";
    }

    html += "</style>";

    // Isolated BlackBerry 10 native-style selection bar CSS (placed AFTER book-author-styles so book CSS cannot break it)
    html += "<style id=\"libris-bubble-style\">";
    html += "#libris-selection-bubble { "
            "position: fixed !important; "
            "display: none; "
            "z-index: 2147483647 !important; "
            "background: #141210 !important; "
            "color: #F5EFE6 !important; "
            "border: 1px solid #D4A359 !important; "
            "border-radius: 6px !important; "
            "box-shadow: 0 4px 14px rgba(0, 0, 0, 0.72) !important; "
            "padding: 0 !important; "
            "margin: 0 !important; "
            "width: auto !important; "
            "height: auto !important; "
            "overflow: hidden !important; "
            "white-space: nowrap !important; "
            "text-indent: 0 !important; "
            "font-family: 'Slate Pro', 'Helvetica Neue', Arial, sans-serif !important; "
            "font-size: 12px !important; "
            "line-height: 1 !important; "
            "-webkit-user-select: none !important; "
            "user-select: none !important; "
            "} ";
    html += "#libris-selection-bubble .libris-bubble-row { "
            "display: table !important; "
            "border-collapse: collapse !important; "
            "margin: 0 !important; "
            "padding: 0 !important; "
            "height: 36px !important; "
            "background: transparent !important; "
            "} ";
    html += "#libris-selection-bubble .libris-bubble-btn { "
            "display: table-cell !important; "
            "vertical-align: middle !important; "
            "height: 36px !important; "
            "line-height: 0 !important; "
            "font-size: 0 !important; "
            "padding: 0 11px !important; "
            "margin: 0 !important; "
            "color: #F5EFE6 !important; "
            "background: transparent !important; "
            "cursor: pointer !important; "
            "border: 0 !important; "
            "border-right: 1px solid #2A251E !important; "
            "border-radius: 0 !important; "
            "text-decoration: none !important; "
            "text-align: center !important; "
            "white-space: nowrap !important; "
            "text-indent: 0 !important; "
            "-webkit-tap-highlight-color: transparent !important; "
            "} ";
    html += "#libris-selection-bubble #b-btn-close { border-right: 0 !important; padding: 0 10px !important; } ";
    html += "#libris-selection-bubble .libris-bubble-btn:active { background-color: #A87932 !important; color: #0D0B09 !important; } ";
    html += "#libris-selection-bubble .libris-bubble-btn:active svg { fill: #0D0B09 !important; } ";
    html += "#libris-selection-bubble svg { "
            "display: inline-block !important; "
            "vertical-align: middle !important; "
            "margin: 0 !important; "
            "padding: 0 !important; "
            "max-width: none !important; "
            "max-height: none !important; "
            "float: none !important; "
            "} ";

    html += "</style>";

    // JavaScript for pagination, touch swipe gestures, and tap zones
    int targetPage = (m_currentPageInChapter >= 1) ? m_currentPageInChapter : 1;
    int targetScroll = (m_currentScrollPos >= 0) ? m_currentScrollPos : 0;
    m_shouldRestorePageOnLoad = false;

    QString curChapterHref;
    if (m_currentFormat == "epub") {
        QList<EpubChapter> chs = m_epubParser.chapters();
        if (m_currentChapterIndex >= 0 && m_currentChapterIndex < chs.size()) {
            curChapterHref = chs[m_currentChapterIndex].href;
        }
    } else if (m_currentFormat == "mobi") {
        QList<BookChapter> chs = m_mobiParser.chapters();
        if (m_currentChapterIndex >= 0 && m_currentChapterIndex < chs.size()) {
            curChapterHref = chs[m_currentChapterIndex].href;
        }
    } else if (m_currentFormat == "fb2") {
        QList<EpubChapter> chs = m_fb2Parser.chapters();
        if (m_currentChapterIndex >= 0 && m_currentChapterIndex < chs.size()) {
            curChapterHref = chs[m_currentChapterIndex].href;
        }
    }

    html += "<script>";
    html += QString("window.librisCurrentChapterHref = \"%1\";").arg(escapeJsonString(curChapterHref));
    html += QString("var readingMode = '%1';").arg(m_readingMode);
    html += QString("var isTwoColMode = %1;").arg(twoCol ? "true" : "false");
    bool mangaRtl = (m_currentFormat == "cbz" && m_mangaRtlMode);
    html += QString("var isMangaRtl = %1;").arg(mangaRtl ? "true" : "false");
    html += QString("var curPage = %1; var totalPages = 1;").arg(targetPage);
    html += QString("var savedTargetPage = %1; var savedTargetScroll = %2;").arg(targetPage).arg(targetScroll);
    html += QString("var screenW = %1;").arg(screenW);

    html += "function updatePageInfo() {";
    html += "  var vp = document.getElementById('reader-viewport');";
    html += "  if (!vp) return;";
    html += "  var scrTop = 0;";
    html += "  if (readingMode === 'scroll') {";
    html += "    var h = vp.clientHeight || window.innerHeight;";
    html += "    if (h <= 0) h = 720;";
    html += "    var maxScroll = Math.max(0, vp.scrollHeight - h);";
    html += "    totalPages = Math.max(1, Math.ceil(vp.scrollHeight / h));";
    html += "    if (maxScroll <= 5) {";
    html += "      curPage = 1;";
    html += "    } else if (vp.scrollTop >= maxScroll - 15) {";
    html += "      curPage = totalPages;";
    html += "    } else {";
    html += "      var scrollRatio = Math.max(0.0, Math.min(1.0, vp.scrollTop / maxScroll));";
    html += "      curPage = Math.min(totalPages, Math.max(1, Math.round(scrollRatio * (totalPages - 1)) + 1));";
    html += "    }";
    html += "    scrTop = Math.round(vp.scrollTop || 0);";
    html += "  } else {";
    html += "    var w = window.innerWidth || document.documentElement.clientWidth || (vp ? vp.clientWidth : 0) || screenW;";
    html += "    if (w <= 0) w = (screenW > 0 ? screenW : 720);";
    html += "    var h = window.innerHeight || document.documentElement.clientHeight;";
    html += "    if (h > 0 && (!vp.style.height || vp.style.height === '0px')) {";
    html += "      vp.style.height = h + 'px';";
    html += "    }";
    html += "    if (!vp.dataset.colConfigured || vp.dataset.lastWidth != w) {";
    html += "      vp.dataset.colConfigured = '1';";
    html += "      vp.dataset.lastWidth = w;";
    html += "      var computedStyle = window.getComputedStyle ? window.getComputedStyle(vp, null) : null;";
    html += "      var padL = computedStyle ? (parseFloat(computedStyle.paddingLeft) || 0) : (parseFloat(vp.style.paddingLeft) || 0);";
    html += "      var padR = computedStyle ? (parseFloat(computedStyle.paddingRight) || 0) : (parseFloat(vp.style.paddingRight) || 0);";
    html += "      var gap = Math.round(padL + padR);";
    html += "      var colW = isTwoColMode ? Math.max(100, Math.floor((w - padL - padR - gap) / 2)) : Math.max(100, Math.round(w - gap));";
    html += "      vp.style.webkitColumnWidth = colW + 'px';";
    html += "      vp.style.columnWidth = colW + 'px';";
    html += "      vp.style.webkitColumnGap = gap + 'px';";
    html += "      vp.style.columnGap = gap + 'px';";
    html += "    }";
    html += "    totalPages = Math.max(1, Math.ceil((vp.scrollWidth - 2) / w));";
    html += "    curPage = Math.min(totalPages, Math.max(1, Math.round(vp.scrollLeft / w) + 1));";
    html += "  }";
    html += "  if (window.navigator && window.navigator.cascades) {";
    html += "    window.navigator.cascades.postMessage('pageInfo:' + curPage + ':' + totalPages + ':' + scrTop);";
    html += "  }";
    html += "};";

    html += "function pageDown() {";
    html += "  var vp = document.getElementById('reader-viewport');";
    html += "  if (!vp) return;";
    html += "  var h = vp.clientHeight || window.innerHeight;";
    html += "  if (vp.scrollTop + h >= vp.scrollHeight - 15) {";
    html += "    if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('nextChapter');";
    html += "  } else {";
    html += "    vp.scrollTop = Math.min(vp.scrollHeight - h, vp.scrollTop + Math.round(h * 0.85));";
    html += "    updatePageInfo();";
    html += "  }";
    html += "};";

    html += "function pageUp() {";
    html += "  var vp = document.getElementById('reader-viewport');";
    html += "  if (!vp) return;";
    html += "  var h = vp.clientHeight || window.innerHeight;";
    html += "  if (vp.scrollTop <= 15) {";
    html += "    if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('prevChapter');";
    html += "  } else {";
    html += "    vp.scrollTop = Math.max(0, vp.scrollTop - Math.round(h * 0.85));";
    html += "    updatePageInfo();";
    html += "  }";
    html += "};";

    html += "var isPageTurnAnimating = false;\n";
    html += "var pageAnimId = null;\n";
    html += "function cancelPageAnimation() {\n";
    html += "  if (pageAnimId) {\n";
    html += "    var cRAF = window.cancelAnimationFrame || window.webkitCancelAnimationFrame || clearTimeout;\n";
    html += "    cRAF(pageAnimId);\n";
    html += "    pageAnimId = null;\n";
    html += "  }\n";
    html += "  isPageTurnAnimating = false;\n";
    html += "};\n";
    html += "function smoothScrollTo(targetX, duration, onComplete) {\n";
    html += "  var vp = document.getElementById('reader-viewport');\n";
    html += "  if (!vp) { if (onComplete) onComplete(); return; }\n";
    html += "  cancelPageAnimation();\n";
    html += "  isPageTurnAnimating = true;\n";
    html += "  var startX = vp.scrollLeft;\n";
    html += "  var dist = targetX - startX;\n";
    html += "  if (Math.abs(dist) < 2 || duration <= 16) {\n";
    html += "    vp.scrollLeft = targetX;\n";
    html += "    isPageTurnAnimating = false;\n";
    html += "    if (onComplete) onComplete();\n";
    html += "    return;\n";
    html += "  }\n";
    html += "  var startTime = null;\n";
    html += "  var rAF = window.requestAnimationFrame || window.webkitRequestAnimationFrame || function(fn) { return setTimeout(fn, 16); };\n";
    html += "  function animStep(timestamp) {\n";
    html += "    if (!startTime) startTime = timestamp || (new Date().getTime());\n";
    html += "    var now = timestamp || (new Date().getTime());\n";
    html += "    var progress = Math.min(1.0, (now - startTime) / duration);\n";
    html += "    var ease = 1.0 - Math.pow(1.0 - progress, 3);\n";
    html += "    vp.scrollLeft = Math.round(startX + dist * ease);\n";
    html += "    if (progress < 1.0) {\n";
    html += "      pageAnimId = rAF(animStep);\n";
    html += "    } else {\n";
    html += "      vp.scrollLeft = targetX;\n";
    html += "      pageAnimId = null;\n";
    html += "      isPageTurnAnimating = false;\n";
    html += "      if (onComplete) onComplete();\n";
    html += "    }\n";
    html += "  };\n";
    html += "  pageAnimId = rAF(animStep);\n";
    html += "};\n";
    html += "function animatePageTurn(dir) {\n";
    html += "  if (isPageTurnAnimating) return;\n";
    html += "  var vp = document.getElementById('reader-viewport');\n";
    html += "  if (!vp) return;\n";
    html += "  var w = window.innerWidth || document.documentElement.clientWidth || (vp ? vp.clientWidth : 0) || screenW;\n";
    html += "  if (w <= 0) w = (screenW > 0 ? screenW : 720);\n";
    html += "  if (dir === 'next') {\n";
    html += "    if (curPage < totalPages) {\n";
    html += "      var targetScroll = curPage * w;\n";
    html += "      smoothScrollTo(targetScroll, 180, function() {\n";
    html += "        curPage++;\n";
    html += "        updatePageInfo();\n";
    html += "      });\n";
    html += "    } else {\n";
    html += "      vp.style.webkitTransition = '-webkit-transform 120ms cubic-bezier(0.25, 1, 0.5, 1)';\n";
    html += "      vp.style.webkitTransform = 'translate3d(-35px, 0, 0)';\n";
    html += "      setTimeout(function() {\n";
    html += "        vp.style.webkitTransition = '-webkit-transform 150ms cubic-bezier(0.25, 1, 0.5, 1)';\n";
    html += "        vp.style.webkitTransform = 'translate3d(0, 0, 0)';\n";
    html += "        if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('nextChapter');\n";
    html += "      }, 120);\n";
    html += "    }\n";
    html += "  } else if (dir === 'prev') {\n";
    html += "    if (curPage > 1) {\n";
    html += "      var targetScroll = (curPage - 2) * w;\n";
    html += "      smoothScrollTo(targetScroll, 180, function() {\n";
    html += "        curPage--;\n";
    html += "        updatePageInfo();\n";
    html += "      });\n";
    html += "    } else {\n";
    html += "      vp.style.webkitTransition = '-webkit-transform 120ms cubic-bezier(0.25, 1, 0.5, 1)';\n";
    html += "      vp.style.webkitTransform = 'translate3d(35px, 0, 0)';\n";
    html += "      setTimeout(function() {\n";
    html += "        vp.style.webkitTransition = '-webkit-transform 150ms cubic-bezier(0.25, 1, 0.5, 1)';\n";
    html += "        vp.style.webkitTransform = 'translate3d(0, 0, 0)';\n";
    html += "        if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('prevChapter');\n";
    html += "      }, 120);\n";
    html += "    }\n";
    html += "  }\n";
    html += "};\n";
    html += "function nextPage() {\n";
    html += "  if (readingMode === 'scroll') { pageDown(); return; }\n";
    html += "  animatePageTurn('next');\n";
    html += "};\n";
    html += "function prevPage() {\n";
    html += "  if (readingMode === 'scroll') { pageUp(); return; }\n";
    html += "  animatePageTurn('prev');\n";
    html += "};\n";
    html += "function goToPage(p) {";
    html += "  var vp = document.getElementById('reader-viewport');";
    html += "  if (!vp) return;";
    html += "  cancelPageAnimation();";
    html += "  if (readingMode === 'scroll') {";
    html += "    var h = vp.clientHeight || window.innerHeight || 720;";
    html += "    totalPages = Math.max(1, Math.ceil(vp.scrollHeight / h));";
    html += "    curPage = Math.min(totalPages, Math.max(1, p));";
    html += "    vp.scrollTop = (curPage - 1) * h;";
    html += "    updatePageInfo();";
    html += "    return;";
    html += "  }";
    html += "  var w = window.innerWidth || document.documentElement.clientWidth || (vp ? vp.clientWidth : 0) || screenW;";
    html += "  if (w <= 0) w = (screenW > 0 ? screenW : 720);";
    html += "  totalPages = Math.max(1, Math.round(vp.scrollWidth / w));";
    html += "  curPage = Math.max(1, p);";
    html += "  if (totalPages > 1 && curPage > totalPages) curPage = totalPages;";
    html += "  vp.style.webkitTransition = 'none';";
    html += "  vp.style.webkitTransform = 'translate3d(0, 0, 0)';";
    html += "  vp.style.opacity = '1.0';";
    html += "  vp.scrollLeft = (curPage - 1) * w;";
    html += "  if (window.navigator && window.navigator.cascades) {";
    html += "    window.navigator.cascades.postMessage('pageInfo:' + curPage + ':' + totalPages + ':0');";
    html += "  }";
    html += "};";

    html += "function scrollByDelta(deltaY) {";
    html += "  var vp = document.getElementById('reader-viewport');";
    html += "  if (!vp) return;";
    html += "  var h = vp.clientHeight || window.innerHeight;";
    html += "  if (deltaY > 0 && vp.scrollTop + h >= vp.scrollHeight - 5) {";
    html += "    if (!window._chapterJumpDebounce) {";
    html += "      window._chapterJumpDebounce = true;";
    html += "      setTimeout(function() { window._chapterJumpDebounce = false; }, 800);";
    html += "      if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('nextChapter');";
    html += "    }";
    html += "    return;";
    html += "  }";
    html += "  if (deltaY < 0 && vp.scrollTop <= 5) {";
    html += "    if (!window._chapterJumpDebounce) {";
    html += "      window._chapterJumpDebounce = true;";
    html += "      setTimeout(function() { window._chapterJumpDebounce = false; }, 800);";
    html += "      if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('prevChapter');";
    html += "    }";
    html += "    return;";
    html += "  }";
    html += "  vp.scrollTop += deltaY;";
    html += "};";

    html += "function scrollDown(amount) { scrollByDelta(amount || 80); };";
    html += "function scrollUp(amount) { scrollByDelta(-(amount || 80)); };";

    // DOM wheel / mousewheel event listeners for WebKit trackpad support
    html += "window.addEventListener('wheel', function(e) {";
    html += "  if (readingMode === 'scroll') {";
    html += "    var d = e.deltaY || 0;";
    html += "    scrollByDelta(d);";
    html += "    if (e.preventDefault) e.preventDefault();";
    html += "  } else if (readingMode === 'paged') {";
    html += "    var d = e.deltaY || 0;";
    html += "    if (d > 25) { nextPage(); if (e.preventDefault) e.preventDefault(); }";
    html += "    else if (d < -25) { prevPage(); if (e.preventDefault) e.preventDefault(); }";
    html += "  }";
    html += "}, false);";
    html += "window.addEventListener('mousewheel', function(e) {";
    html += "  if (readingMode === 'scroll') {";
    html += "    var d = -e.wheelDelta || 0;";
    html += "    scrollByDelta(d);";
    html += "    if (e.preventDefault) e.preventDefault();";
    html += "  } else if (readingMode === 'paged') {";
    html += "    var d = -e.wheelDelta || 0;";
    html += "    if (d > 25) { nextPage(); if (e.preventDefault) e.preventDefault(); }";
    html += "    else if (d < -25) { prevPage(); if (e.preventDefault) e.preventDefault(); }";
    html += "  }";
    html += "}, false);";

    // Suppress BB10 context menu
    html += "window.addEventListener('contextmenu', function(e) {";
    html += "  if (e.preventDefault) e.preventDefault();";
    html += "  if (e.stopPropagation) e.stopPropagation();";
    html += "  return false;";
    html += "}, true);";

    // Selection reporting & Floating Bubble positioning
    html += "function updateSelectionBubble() {";
    html += "  var bubble = document.getElementById('libris-selection-bubble');";
    html += "  if (!bubble) return;";
    html += "  var sel = window.getSelection ? window.getSelection() : null;";
    html += "  if (!sel || sel.isCollapsed || sel.rangeCount === 0) {";
    html += "    bubble.style.display = 'none';";
    html += "    return;";
    html += "  }";
    html += "  var text = sel.toString().trim();";
    html += "  if (!text || text.length === 0) {";
    html += "    bubble.style.display = 'none';";
    html += "    return;";
    html += "  }";
    html += "  var range = sel.getRangeAt(0);";
    html += "  var rect = range.getBoundingClientRect();";
    html += "  if (!rect || (rect.width === 0 && rect.height === 0)) {";
    html += "    bubble.style.display = 'none';";
    html += "    return;";
    html += "  }";
    html += "  bubble.style.display = 'block';";
    html += "  var bW = bubble.offsetWidth || 260;";
    html += "  var bH = bubble.offsetHeight || 38;";
    html += "  var winW = window.innerWidth || document.documentElement.clientWidth || screenW;";
    html += "  var winH = window.innerHeight || document.documentElement.clientHeight || 720;";
    html += "  var midX = rect.left + (rect.width / 2);";
    html += "  var posX = midX - (bW / 2);";
    html += "  if (posX < 8) posX = 8;";
    html += "  if (posX + bW > winW - 8) posX = winW - bW - 8;";
    html += "  var posY = rect.top - bH - 8;";
    html += "  if (posY < 8) {";
    html += "    posY = rect.bottom + 8;";
    html += "    if (posY + bH > winH - 8) posY = winH - bH - 8;";
    html += "  }";
    html += "  bubble.style.left = Math.round(posX) + 'px';";
    html += "  bubble.style.top = Math.round(posY) + 'px';";
    html += "};";

    html += "function clearSelectionAndBubble() {";
    html += "  if (window.getSelection) window.getSelection().removeAllRanges();";
    html += "  var b = document.getElementById('libris-selection-bubble');";
    html += "  if (b) b.style.display = 'none';";
    html += "  if (window.navigator && window.navigator.cascades) {";
    html += "    window.navigator.cascades.postMessage('textSelected:');";
    html += "  }";
    html += "};";

    html += "function initSelectionBubble() {";
    html += "  var bubble = document.getElementById('libris-selection-bubble');";
    html += "  if (!bubble) return;";
    html += "  var cachedSelText = '';";
    html += "  bubble.addEventListener('touchstart', function(e) {";
    html += "    var s = window.getSelection ? window.getSelection().toString() : '';";
    html += "    if (s && s.trim()) cachedSelText = s.trim();";
    html += "    e.stopPropagation();";
    html += "  }, false);";
    html += "  bubble.addEventListener('touchend', function(e) {";
    html += "    e.stopPropagation();";
    html += "  }, false);";
    html += "  function getSelText() {";
    html += "    var s = window.getSelection ? window.getSelection().toString() : '';";
    html += "    s = s ? s.trim() : '';";
    html += "    return s ? s : cachedSelText;";
    html += "  };";
    html += "  var btnHl = document.getElementById('b-btn-highlight');";
    html += "  if (btnHl) {";
    html += "    btnHl.addEventListener('click', function(e) {";
    html += "      e.stopPropagation(); e.preventDefault();";
    html += "      var t = getSelText();";
    html += "      if (t && window.navigator && window.navigator.cascades) {";
    html += "        window.navigator.cascades.postMessage('requestHighlight:' + encodeURIComponent(t));";
    html += "      }";
    html += "      clearSelectionAndBubble();";
    html += "    }, false);";
    html += "  };";
    html += "  var btnSpeak = document.getElementById('b-btn-speak');";
    html += "  if (btnSpeak) {";
    html += "    btnSpeak.addEventListener('click', function(e) {";
    html += "      e.stopPropagation(); e.preventDefault();";
    html += "      var t = getSelText();";
    html += "      if (t && window.navigator && window.navigator.cascades) {";
    html += "        window.navigator.cascades.postMessage('speakText:' + encodeURIComponent(t));";
    html += "      }";
    html += "      clearSelectionAndBubble();";
    html += "    }, false);";
    html += "  };";
    html += "  var btnCopy = document.getElementById('b-btn-copy');";
    html += "  if (btnCopy) {";
    html += "    btnCopy.addEventListener('click', function(e) {";
    html += "      e.stopPropagation(); e.preventDefault();";
    html += "      var t = getSelText();";
    html += "      if (t && window.navigator && window.navigator.cascades) {";
    html += "        window.navigator.cascades.postMessage('copyText:' + encodeURIComponent(t));";
    html += "      }";
    html += "      clearSelectionAndBubble();";
    html += "    }, false);";
    html += "  }";
    html += "  var btnDict = document.getElementById('b-btn-dict');";
    html += "  if (btnDict) {";
    html += "    btnDict.addEventListener('click', function(e) {";
    html += "      e.stopPropagation(); e.preventDefault();";
    html += "      var t = getSelText();";
    html += "      if (t && window.navigator && window.navigator.cascades) {";
    html += "        window.navigator.cascades.postMessage('lookupWord:' + encodeURIComponent(t));";
    html += "      }";
    html += "      clearSelectionAndBubble();";
    html += "    }, false);";
    html += "  }";
    html += "  var btnTrans = document.getElementById('b-btn-trans');";
    html += "  if (btnTrans) {";
    html += "    btnTrans.addEventListener('click', function(e) {";
    html += "      e.stopPropagation(); e.preventDefault();";
    html += "      var t = getSelText();";
    html += "      if (t && window.navigator && window.navigator.cascades) {";
    html += "        window.navigator.cascades.postMessage('translateSentence:' + encodeURIComponent(t));";
    html += "      }";
    html += "      clearSelectionAndBubble();";
    html += "    }, false);";
    html += "  }";
    html += "  var btnSearch = document.getElementById('b-btn-search');";
    html += "  if (btnSearch) {";
    html += "    btnSearch.addEventListener('click', function(e) {";
    html += "      e.stopPropagation(); e.preventDefault();";
    html += "      var t = getSelText();";
    html += "      if (t && window.navigator && window.navigator.cascades) {";
    html += "        window.navigator.cascades.postMessage('searchKeyword:' + encodeURIComponent(t));";
    html += "      }";
    html += "      clearSelectionAndBubble();";
    html += "    }, false);";
    html += "  }";
    html += "  var btnClose = document.getElementById('b-btn-close');";
    html += "  if (btnClose) {";
    html += "    btnClose.addEventListener('click', function(e) {";
    html += "      e.stopPropagation(); e.preventDefault();";
    html += "      clearSelectionAndBubble();";
    html += "    }, false);";
    html += "  }";
    html += "};";

    html += "function reportSelection() {";
    html += "  var sel = window.getSelection ? window.getSelection().toString() : '';";
    html += "  if (sel && sel.trim().length > 0) {";
    html += "    if (window.navigator && window.navigator.cascades) {";
    html += "      window.navigator.cascades.postMessage('textSelected:' + encodeURIComponent(sel.trim()));";
    html += "    }";
    html += "  } else {";
    html += "    if (window.navigator && window.navigator.cascades) {";
    html += "      window.navigator.cascades.postMessage('textSelected:');";
    html += "    }";
    html += "  }";
    html += "  updateSelectionBubble();";
    html += "};";
    html += "document.addEventListener('selectionchange', function() { setTimeout(reportSelection, 80); }, false);";
    html += "document.addEventListener('mouseup', function() { reportSelection(); }, false);";
    html += "document.addEventListener('touchend', function() { reportSelection(); }, false);";

    // In-Page Search support
    html += "var currentMatchIndex = -1; var totalMatches = 0; var searchHighlightTimer = null;";
    html += "function clearInPageSearch() {";
    html += "  if (searchHighlightTimer) { clearTimeout(searchHighlightTimer); searchHighlightTimer = null; }";
    html += "  var marks = document.querySelectorAll('mark.libris-search-match');";
    html += "  for (var i = 0; i < marks.length; i++) {";
    html += "    var parent = marks[i].parentNode;";
    html += "    if (parent) { parent.replaceChild(document.createTextNode(marks[i].textContent), marks[i]); parent.normalize(); }";
    html += "  }";
    html += "  currentMatchIndex = -1; totalMatches = 0;";
    html += "  if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('searchStats:0:0');";
    html += "};";

    html += "function searchInPage(term, autoDismiss) {";
    html += "  clearInPageSearch();";
    html += "  if (!term || term.trim() === '') return;";
    html += "  var vp = document.getElementById('reader-viewport') || document.body;";
    html += "  var walker = document.createTreeWalker(vp, NodeFilter.SHOW_TEXT, null, false);";
    html += "  var nodesToReplace = [];";
    html += "  var escaped = term.replace(/[-[\\]{}()*+?.,\\\\^$|#\\s]/g, '\\\\$&');";
    html += "  var regex = new RegExp(escaped, 'gi');";
    html += "  while (walker.nextNode()) {";
    html += "    var node = walker.currentNode;";
    html += "    if (node.parentNode && node.parentNode.nodeName !== 'SCRIPT' && node.parentNode.nodeName !== 'STYLE' && node.parentNode.nodeName !== 'MARK') {";
    html += "      if (regex.test(node.nodeValue)) nodesToReplace.push(node);";
    html += "    }";
    html += "  }";
    html += "  for (var i = 0; i < nodesToReplace.length; i++) {";
    html += "    var textNode = nodesToReplace[i];";
    html += "    var val = textNode.nodeValue;";
    html += "    var match; var lastIndex = 0; var frag = document.createDocumentFragment();";
    html += "    regex.lastIndex = 0;";
    html += "    while ((match = regex.exec(val)) !== null) {";
    html += "      if (match.index > lastIndex) frag.appendChild(document.createTextNode(val.substring(lastIndex, match.index)));";
    html += "      var mark = document.createElement('mark');";
    html += "      mark.className = 'libris-search-match';";
    html += "      mark.appendChild(document.createTextNode(match[0]));";
    html += "      frag.appendChild(mark);";
    html += "      lastIndex = regex.lastIndex; totalMatches++;";
    html += "    }";
    html += "    if (lastIndex < val.length) frag.appendChild(document.createTextNode(val.substring(lastIndex)));";
    html += "    if (textNode.parentNode) textNode.parentNode.replaceChild(frag, textNode);";
    html += "  }";
    html += "  if (totalMatches > 0) { currentMatchIndex = 0; highlightCurrentMatch(autoDismiss); }";
    html += "  else { if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('searchStats:0:0'); }";
    html += "};";

    html += "function highlightCurrentMatch(autoDismiss) {";
    html += "  if (searchHighlightTimer) { clearTimeout(searchHighlightTimer); searchHighlightTimer = null; }";
    html += "  var marks = document.querySelectorAll('mark.libris-search-match');";
    html += "  for (var i = 0; i < marks.length; i++) marks[i].classList.remove('libris-search-current');";
    html += "  if (currentMatchIndex >= 0 && currentMatchIndex < marks.length) {";
    html += "    var el = marks[currentMatchIndex];";
    html += "    el.classList.add('libris-search-current');";
    html += "    if (readingMode === 'paged') {";
    html += "      var vp = document.getElementById('reader-viewport');";
    html += "      var w = window.innerWidth || document.documentElement.clientWidth || (vp ? vp.clientWidth : 0) || screenW || 720;";
    html += "      var rect = el.getBoundingClientRect();";
    html += "      var absX = (vp ? vp.scrollLeft : 0) + rect.left;";
    html += "      var pageIdx = Math.max(0, Math.floor(absX / w));";
    html += "      goToPage(pageIdx + 1);";
    html += "    } else {";
    html += "      el.scrollIntoView({ behavior: 'smooth', block: 'center' });";
    html += "    }";
    html += "    if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('searchStats:' + (currentMatchIndex + 1) + ':' + totalMatches);";
    html += "    if (autoDismiss) {";
    html += "      searchHighlightTimer = setTimeout(function() { clearInPageSearch(); }, 3500);";
    html += "    }";
    html += "  }";
    html += "};";

    html += "document.addEventListener('touchstart', function() {";
    html += "  if (searchHighlightTimer) { clearInPageSearch(); }";
    html += "}, false);";

    html += "function searchNext() { if (totalMatches > 0) { currentMatchIndex = (currentMatchIndex + 1) % totalMatches; highlightCurrentMatch(); } };";
    html += "function searchPrev() { if (totalMatches > 0) { currentMatchIndex = (currentMatchIndex - 1 + totalMatches) % totalMatches; highlightCurrentMatch(); } };";
    html += "function jumpToMatch(index) {";
    html += "  var marks = document.querySelectorAll('mark.libris-search-match');";
    html += "  if (index >= 0 && index < marks.length) {";
    html += "    currentMatchIndex = index;";
    html += "    highlightCurrentMatch(false);";
    html += "  }";
    html += "};";

    // =========================================================================
    // TTS Interactive Sentence Highlighter & Page Following
    // =========================================================================
    html += "function clearTtsHighlight() {\n";
    html += "  var oldMarks = document.querySelectorAll('mark.libris-tts-active');\n";
    html += "  for (var i = 0; i < oldMarks.length; i++) {\n";
    html += "    var m = oldMarks[i];\n";
    html += "    var p = m.parentNode;\n";
    html += "    if (p) {\n";
    html += "      while (m.firstChild) p.insertBefore(m.firstChild, m);\n";
    html += "      p.removeChild(m);\n";
    html += "      p.normalize();\n";
    html += "    }\n";
    html += "  }\n";
    html += "};\n";
    html += "function highlightTtsSentence(text) {\n";
    html += "  clearTtsHighlight();\n";
    html += "  if (!text) return;\n";
    html += "  var trimmed = text.trim();\n";
    html += "  if (trimmed.length < 2) return;\n";
    html += "  var vp = document.getElementById('reader-viewport') || document.body;\n";
    html += "  if (!vp) return;\n";
    html += "  var searchStr = trimmed;\n";
    html += "  if (searchStr.length > 35) {\n";
    html += "    searchStr = searchStr.substring(0, 35).trim();\n";
    html += "  }\n";
    html += "  var walker = document.createTreeWalker(vp, NodeFilter.SHOW_TEXT, null, false);\n";
    html += "  var textNode;\n";
    html += "  while ((textNode = walker.nextNode())) {\n";
    html += "    var parent = textNode.parentNode;\n";
    html += "    if (parent && (parent.nodeName === 'SCRIPT' || parent.nodeName === 'STYLE' || parent.id === 'libris-selection-bubble')) continue;\n";
    html += "    var val = textNode.nodeValue;\n";
    html += "    var idx = val.indexOf(searchStr);\n";
    html += "    if (idx === -1 && searchStr.length > 18) {\n";
    html += "      idx = val.indexOf(searchStr.substring(0, 18));\n";
    html += "    }\n";
    html += "    if (idx !== -1) {\n";
    html += "      try {\n";
    html += "        var matchLen = Math.min(val.length - idx, trimmed.length);\n";
    html += "        var range = document.createRange();\n";
    html += "        range.setStart(textNode, idx);\n";
    html += "        range.setEnd(textNode, idx + matchLen);\n";
    html += "        var mark = document.createElement('mark');\n";
    html += "        mark.className = 'libris-tts-active';\n";
    html += "        range.surroundContents(mark);\n";
    html += "        if (readingMode === 'paged') {\n";
    html += "          var w = window.innerWidth || document.documentElement.clientWidth || screenW || 720;\n";
    html += "          var offsetLeft = 0;\n";
    html += "          var cur = mark;\n";
    html += "          while (cur && cur !== vp) {\n";
    html += "            offsetLeft += cur.offsetLeft;\n";
    html += "            cur = cur.offsetParent;\n";
    html += "          }\n";
    html += "          var targetP = Math.floor(offsetLeft / w) + 1;\n";
    html += "          if (targetP > 0 && targetP !== curPage) {\n";
    html += "            goToPage(targetP);\n";
    html += "          }\n";
    html += "        } else {\n";
    html += "          mark.scrollIntoView({ behavior: 'smooth', block: 'center' });\n";
    html += "        }\n";
    html += "      } catch(err) {}\n";
    html += "      break;\n";
    html += "    }\n";
    html += "  }\n";
    html += "};\n";

    // Robust touch & tap handler for reading (Tapping center/screen toggles menu; horizontal swipe turns pages)
    html += "var startX = 0, startY = 0, touchStartTime = 0;\n";
    html += "var isDraggingPage = false;\n";
    html += "var lastToggleTime = 0;\n";
    html += "function requestToggleChrome() {\n";
    html += "  var now = Date.now();\n";
    html += "  if (now - lastToggleTime < 350) return;\n";
    html += "  lastToggleTime = now;\n";
    html += "  if (window.navigator && window.navigator.cascades) {\n";
    html += "    window.navigator.cascades.postMessage('toggleChrome');\n";
    html += "  }\n";
    html += "};\n";
    html += "window.addEventListener('touchstart', function(e) {\n";
    html += "  if (e.touches.length === 1) {\n";
    html += "    startX = e.touches[0].clientX;\n";
    html += "    startY = e.touches[0].clientY;\n";
    html += "    touchStartTime = Date.now();\n";
    html += "    isDraggingPage = false;\n";
    html += "    cancelPageAnimation();\n";
    html += "  }\n";
    html += "}, false);\n";
    html += "window.addEventListener('touchmove', function(e) {\n";
    html += "  if (e.touches.length === 1 && readingMode === 'paged') {\n";
    html += "    var dx = e.touches[0].clientX - startX;\n";
    html += "    var dy = e.touches[0].clientY - startY;\n";
    html += "    if (Math.abs(dx) > 6 && Math.abs(dx) > Math.abs(dy) * 0.5) {\n";
    html += "      if (e.preventDefault) e.preventDefault();\n";
    html += "      var vp = document.getElementById('reader-viewport');\n";
    html += "      if (vp) {\n";
    html += "        isDraggingPage = true;\n";
    html += "        var w = window.innerWidth || document.documentElement.clientWidth || (vp ? vp.clientWidth : 0) || screenW;\n";
    html += "        if (w <= 0) w = 720;\n";
    html += "        var baseScroll = (curPage - 1) * w;\n";
    html += "        var isAtStart = isMangaRtl ? (curPage <= 1 && dx < 0) : (curPage <= 1 && dx > 0);\n";
    html += "        var isAtEnd = isMangaRtl ? (curPage >= totalPages && dx > 0) : (curPage >= totalPages && dx < 0);\n";
    html += "        if (isAtStart) {\n";
    html += "          vp.scrollLeft = 0;\n";
    html += "          vp.style.webkitTransition = 'none';\n";
    html += "          vp.style.webkitTransform = 'translate3d(' + Math.round(dx * 0.35) + 'px, 0, 0)';\n";
    html += "        } else if (isAtEnd) {\n";
    html += "          vp.scrollLeft = (totalPages - 1) * w;\n";
    html += "          vp.style.webkitTransition = 'none';\n";
    html += "          vp.style.webkitTransform = 'translate3d(' + Math.round(dx * 0.35) + 'px, 0, 0)';\n";
    html += "        } else {\n";
    html += "          vp.style.webkitTransition = 'none';\n";
    html += "          vp.style.webkitTransform = 'translate3d(0, 0, 0)';\n";
    html += "          vp.scrollLeft = Math.round(baseScroll + (isMangaRtl ? dx : -dx));\n";
    html += "        }\n";
    html += "      }\n";
    html += "    }\n";
    html += "  }\n";
    html += "}, false);\n";
    html += "window.addEventListener('touchend', function(e) {\n";
    html += "  if (e.changedTouches.length === 1) {\n";
    html += "    var tgt = e.target;\n";
    html += "    while (tgt && tgt !== document.body) {\n";
    html += "      if (tgt.id === 'libris-selection-bubble' || (tgt.tagName && tgt.tagName.toUpperCase() === 'A') || (tgt.className && typeof tgt.className === 'string' && tgt.className.indexOf('libris-highlight') !== -1)) {\n";
    html += "        return;\n";
    html += "      }\n";
    html += "      tgt = tgt.parentNode;\n";
    html += "    }\n";
    html += "    var dx = e.changedTouches[0].clientX - startX;\n";
    html += "    var dy = e.changedTouches[0].clientY - startY;\n";
    html += "    var dt = Date.now() - touchStartTime;\n";
    html += "    var activeSel = window.getSelection ? window.getSelection().toString().trim() : '';\n";
    html += "    if (activeSel.length > 0) {\n";
    html += "      if (isDraggingPage) {\n";
    html += "        var vp = document.getElementById('reader-viewport');\n";
    html += "        if (vp) {\n";
    html += "          vp.style.webkitTransition = '-webkit-transform 150ms cubic-bezier(0.25, 1, 0.5, 1)';\n";
    html += "          vp.style.webkitTransform = 'translate3d(0, 0, 0)';\n";
    html += "        }\n";
    html += "        isDraggingPage = false;\n";
    html += "      }\n";
    html += "      setTimeout(reportSelection, 100);\n";
    html += "      return;\n";
    html += "    }\n";
    html += "    var vp = document.getElementById('reader-viewport');\n";
    html += "    var w = window.innerWidth || document.documentElement.clientWidth || (vp ? vp.clientWidth : 0) || screenW;\n";
    html += "    if (w <= 0) w = 720;\n";
    html += "    var h = window.innerHeight || document.documentElement.clientHeight || 720;\n";
    html += "    var curX = e.changedTouches[0].clientX;\n";
    html += "    var curY = e.changedTouches[0].clientY;\n";
    html += "    var isTap = (!isDraggingPage && Math.abs(dx) < 18 && Math.abs(dy) < 22 && dt < 450);\n";
    html += "    if (isTap && window.isChromeVisible) {\n";
    html += "      requestToggleChrome();\n";
    html += "      return;\n";
    html += "    }\n";
    html += "    if (readingMode === 'scroll') {\n";
    html += "      if (Math.abs(dy) > 35) {\n";
    html += "        if (vp) {\n";
    html += "          var vh = vp.clientHeight || window.innerHeight;\n";
    html += "          if (dy < -60 && vp.scrollTop + vh >= vp.scrollHeight - 5) {\n";
    html += "            if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('nextChapter');\n";
    html += "          } else if (dy > 60 && vp.scrollTop <= 5) {\n";
    html += "            if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('prevChapter');\n";
    html += "          }\n";
    html += "        }\n";
    html += "      } else if (isTap) {\n";
    html += "        requestToggleChrome();\n";
    html += "      }\n";
    html += "    } else {\n"; // Paged Mode
    html += "      if (isDraggingPage) {\n";
    html += "        isDraggingPage = false;\n";
    html += "        if (isMangaRtl) {\n";
    html += "          if (curPage <= 1 && dx < 0) {\n";
    html += "            vp.style.webkitTransition = '-webkit-transform 180ms cubic-bezier(0.25, 1, 0.5, 1)';\n";
    html += "            vp.style.webkitTransform = 'translate3d(0, 0, 0)';\n";
    html += "            if (dx < -30 && window.navigator && window.navigator.cascades) {\n";
    html += "              window.navigator.cascades.postMessage('prevChapter');\n";
    html += "            }\n";
    html += "            return;\n";
    html += "          }\n";
    html += "          if (curPage >= totalPages && dx > 0) {\n";
    html += "            vp.style.webkitTransition = '-webkit-transform 180ms cubic-bezier(0.25, 1, 0.5, 1)';\n";
    html += "            vp.style.webkitTransform = 'translate3d(0, 0, 0)';\n";
    html += "            if (dx > 30 && window.navigator && window.navigator.cascades) {\n";
    html += "              window.navigator.cascades.postMessage('nextChapter');\n";
    html += "            }\n";
    html += "            return;\n";
    html += "          }\n";
    html += "        } else {\n";
    html += "          if (curPage <= 1 && dx > 0) {\n";
    html += "            vp.style.webkitTransition = '-webkit-transform 180ms cubic-bezier(0.25, 1, 0.5, 1)';\n";
    html += "            vp.style.webkitTransform = 'translate3d(0, 0, 0)';\n";
    html += "            if (dx > 30 && window.navigator && window.navigator.cascades) {\n";
    html += "              window.navigator.cascades.postMessage('prevChapter');\n";
    html += "            }\n";
    html += "            return;\n";
    html += "          }\n";
    html += "          if (curPage >= totalPages && dx < 0) {\n";
    html += "            vp.style.webkitTransition = '-webkit-transform 180ms cubic-bezier(0.25, 1, 0.5, 1)';\n";
    html += "            vp.style.webkitTransform = 'translate3d(0, 0, 0)';\n";
    html += "            if (dx < -30 && window.navigator && window.navigator.cascades) {\n";
    html += "              window.navigator.cascades.postMessage('nextChapter');\n";
    html += "            }\n";
    html += "            return;\n";
    html += "          }\n";
    html += "        }\n";
    html += "        var velocity = Math.abs(dx) / Math.max(1, dt);\n";
    html += "        var isFlick = (dt < 350 && velocity > 0.20);\n";
    html += "        var targetPage = curPage;\n";
    html += "        if (isMangaRtl) {\n";
    html += "          if (dx > 20 || (isFlick && dx > 10)) {\n";
    html += "            if (curPage < totalPages) targetPage = curPage + 1;\n";
    html += "          } else if (dx < -20 || (isFlick && dx < -10)) {\n";
    html += "            if (curPage > 1) targetPage = curPage - 1;\n";
    html += "          }\n";
    html += "        } else {\n";
    html += "          if (dx < -20 || (isFlick && dx < -10)) {\n";
    html += "            if (curPage < totalPages) targetPage = curPage + 1;\n";
    html += "          } else if (dx > 20 || (isFlick && dx > 10)) {\n";
    html += "            if (curPage > 1) targetPage = curPage - 1;\n";
    html += "          }\n";
    html += "        }\n";
    html += "        var targetScroll = (targetPage - 1) * w;\n";
    html += "        var dist = Math.abs(targetScroll - vp.scrollLeft);\n";
    html += "        var animDuration = Math.max(100, Math.min(200, Math.round(180 * (dist / w))));\n";
    html += "        smoothScrollTo(targetScroll, animDuration, function() {\n";
    html += "          curPage = targetPage;\n";
    html += "          updatePageInfo();\n";
    html += "        });\n";
    html += "      } else if (Math.abs(dx) > 18 && Math.abs(dx) > Math.abs(dy) * 0.55) {\n";
    html += "        if (isMangaRtl) {\n";
    html += "          animatePageTurn(dx > 0 ? 'next' : 'prev');\n";
    html += "        } else {\n";
    html += "          animatePageTurn(dx < 0 ? 'next' : 'prev');\n";
    html += "        }\n";
    html += "      } else if (isTap) {\n";
    html += "        if (curX < w * 0.25 && curY > h * 0.15 && curY < h * 0.85) {\n";
    html += "          animatePageTurn(isMangaRtl ? 'next' : 'prev');\n";
    html += "        } else if (curX > w * 0.75 && curY > h * 0.15 && curY < h * 0.85) {\n";
    html += "          animatePageTurn(isMangaRtl ? 'prev' : 'next');\n";
    html += "        } else {\n";
    html += "          requestToggleChrome();\n";
    html += "        }\n";
    html += "      }\n";
    html += "    }\n";
    html += "  }\n";
    html += "}, false);\n";

    html += QString("var savedChapterHighlights = %1;\n").arg(getHighlightsJsonForCurrentChapter());

    html += "function highlightTextInContainer(root, term, id, color, note) {\n";
    html += "  if (!term || term.trim() === '') return;\n";
    html += "  var cleanTerm = term.trim();\n";
    html += "  var walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, null, false);\n";
    html += "  var nodesToReplace = [];\n";
    html += "  while (walker.nextNode()) {\n";
    html += "    var node = walker.currentNode;\n";
    html += "    if (node.parentNode && node.parentNode.nodeName !== 'SCRIPT' && node.parentNode.nodeName !== 'STYLE' && node.parentNode.nodeName !== 'MARK') {\n";
    html += "      var idx = node.nodeValue.indexOf(cleanTerm);\n";
    html += "      if (idx !== -1) {\n";
    html += "        nodesToReplace.push({ node: node, idx: idx, len: cleanTerm.length });\n";
    html += "      }\n";
    html += "    }\n";
    html += "  }\n";
    html += "  for (var i = nodesToReplace.length - 1; i >= 0; i--) {\n";
    html += "    var item = nodesToReplace[i];\n";
    html += "    var node = item.node; var idx = item.idx; var len = item.len;\n";
    html += "    var parent = node.parentNode;\n";
    html += "    if (!parent) continue;\n";
    html += "    var text = node.nodeValue;\n";
    html += "    var before = text.substring(0, idx);\n";
    html += "    var match = text.substring(idx, idx + len);\n";
    html += "    var after = text.substring(idx + len);\n";
    html += "    var mark = document.createElement('mark');\n";
    html += "    mark.className = 'libris-highlight libris-hl-' + (color || 'yellow');\n";
    html += "    mark.setAttribute('data-hl-id', id);\n";
    html += "    if (note) mark.setAttribute('data-hl-note', note);\n";
    html += "    mark.textContent = match;\n";
    html += "    mark.onclick = function(e) {\n";
    html += "      e.stopPropagation();\n";
    html += "      var hid = this.getAttribute('data-hl-id');\n";
    html += "      if (window.navigator && window.navigator.cascades) {\n";
    html += "        window.navigator.cascades.postMessage('clickedHighlight:' + hid);\n";
    html += "      }\n";
    html += "    };\n";
    html += "    var fragment = document.createDocumentFragment();\n";
    html += "    if (before) fragment.appendChild(document.createTextNode(before));\n";
    html += "    fragment.appendChild(mark);\n";
    html += "    if (after) fragment.appendChild(document.createTextNode(after));\n";
    html += "    parent.replaceChild(fragment, node);\n";
    html += "  }\n";
    html += "};\n";

    html += "function restoreChapterHighlights() {\n";
    html += "  if (!savedChapterHighlights || savedChapterHighlights.length === 0) return;\n";
    html += "  var vp = document.getElementById('reader-viewport') || document.body;\n";
    html += "  if (!vp) return;\n";
    html += "  for (var h = 0; h < savedChapterHighlights.length; h++) {\n";
    html += "    var hl = savedChapterHighlights[h];\n";
    html += "    if (!hl || !hl.text) continue;\n";
    html += "    highlightTextInContainer(vp, hl.text, hl.id, hl.color, hl.note);\n";
    html += "  }\n";
    html += "};\n";

    html += "function removeHighlightFromDom(id) {\n";
    html += "  var marks = document.querySelectorAll('mark[data-hl-id=\"' + id + '\"]');\n";
    html += "  for (var i = 0; i < marks.length; i++) {\n";
    html += "    var p = marks[i].parentNode;\n";
    html += "    if (p) { p.replaceChild(document.createTextNode(marks[i].textContent), marks[i]); p.normalize(); }\n";
    html += "  }\n";
    html += "};\n";

    html += "function scrollToHighlight(id) {\n";
    html += "  var el = document.querySelector('mark[data-hl-id=\"' + id + '\"]');\n";
    html += "  if (el) {\n";
    html += "    var vp = document.getElementById('reader-viewport');\n";
    html += "    if (readingMode === 'paged') {\n";
    html += "      var rect = el.getBoundingClientRect();\n";
    html += "      var w = window.innerWidth || document.documentElement.clientWidth || screenW;\n";
    html += "      if (w > 0 && rect) {\n";
    html += "        var pageIdx = Math.floor((el.offsetLeft || rect.left) / w);\n";
    html += "        if (pageIdx >= 0) goToPage(pageIdx + 1);\n";
    html += "      }\n";
    html += "    } else {\n";
    html += "      el.scrollIntoView({ behavior: 'smooth', block: 'center' });\n";
    html += "    }\n";
    html += "    el.style.outline = '2px solid #00A8DF';\n";
    html += "    setTimeout(function() { el.style.outline = 'none'; }, 2000);\n";
    html += "  }\n";
    html += "};\n";

    html += "function findTargetByAnchor(anchorId) {\n";
    html += "  if (!anchorId) return null;\n";
    html += "  var ids = [anchorId];\n";
    html += "  try { var dec = decodeURIComponent(anchorId); if (dec && dec !== anchorId) ids.push(dec); } catch(err) {}\n";
    html += "  for (var i = 0; i < ids.length; i++) {\n";
    html += "    var idStr = ids[i];\n";
    html += "    var el = document.getElementById(idStr);\n";
    html += "    if (el) return el;\n";
    html += "    var byName = document.getElementsByName(idStr);\n";
    html += "    if (byName && byName.length > 0) return byName[0];\n";
    html += "  }\n";
    html += "  return null;\n";
    html += "};\n";

    html += "function scrollToAnchor(name) {\n";
    html += "  if (!name || name.trim() === '') return;\n";
    html += "  var cleanName = name.trim();\n";
    html += "  if (cleanName.indexOf('#') === 0) cleanName = cleanName.substring(1);\n";
    html += "  if (cleanName === '') return;\n";
    html += "  var el = findTargetByAnchor(cleanName);\n";
    html += "  if (!el) return;\n";
    html += "  var vp = document.getElementById('reader-viewport');\n";
    html += "  if (!vp) return;\n";
    html += "  if (readingMode === 'paged') {\n";
    html += "    var w = window.innerWidth || document.documentElement.clientWidth || screenW;\n";
    html += "    if (w <= 0) w = 720;\n";
    html += "    var rect = el.getBoundingClientRect();\n";
    html += "    var measEl = el;\n";
    html += "    if (rect.width === 0 && rect.height === 0) {\n";
    html += "      if (el.nextElementSibling) measEl = el.nextElementSibling;\n";
    html += "      else if (el.parentNode && el.parentNode !== vp) measEl = el.parentNode;\n";
    html += "      rect = measEl.getBoundingClientRect();\n";
    html += "    }\n";
    html += "    var absX = (vp ? vp.scrollLeft : 0) + rect.left;\n";
    html += "    if (absX <= 0 && el.offsetLeft > 0) {\n";
    html += "      var cur = el; absX = 0;\n";
    html += "      while (cur && cur !== vp) { absX += cur.offsetLeft; cur = cur.offsetParent; }\n";
    html += "    }\n";
    html += "    var pageIdx = Math.max(0, Math.floor(absX / w));\n";
    html += "    goToPage(pageIdx + 1);\n";
    html += "  } else {\n";
    html += "    try { el.scrollIntoView({ behavior: 'smooth', block: 'start' }); } catch(e) { el.scrollIntoView(true); }\n";
    html += "    updatePageInfo();\n";
    html += "    setTimeout(updatePageInfo, 120);\n";
    html += "  }\n";
    html += "};\n";

    html += "function initLinksAndFootnotes() {\n";
    html += "  if (window.__librisLinksInited) return;\n";
    html += "  window.__librisLinksInited = true;\n";
    html += "  document.addEventListener('click', function(e) {\n";
    html += "    var a = e.target;\n";
    html += "    while (a && a !== document && (!a.tagName || a.tagName.toUpperCase() !== 'A')) {\n";
    html += "      a = a.parentNode;\n";
    html += "    }\n";
    html += "    if (!a || a === document) return;\n";
    html += "    var href = a.getAttribute('href') || a.getAttribute('xlink:href');\n";
    html += "    if (!href) return;\n";
    html += "    href = href.trim();\n";
    html += "    if (!href || href.indexOf('javascript:') === 0) {\n";
    html += "      e.preventDefault(); return;\n";
    html += "    }\n";
    html += "    if (href.indexOf('http://') === 0 || href.indexOf('https://') === 0 || href.indexOf('mailto:') === 0) {\n";
    html += "      e.preventDefault(); e.stopPropagation();\n";
    html += "      if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('openExternalUrl:' + encodeURIComponent(href));\n";
    html += "      return;\n";
    html += "    }\n";
    html += "    e.preventDefault();\n";
    html += "    e.stopPropagation();\n";
    html += "    var hashIdx = href.indexOf('#');\n";
    html += "    var filePart = (hashIdx !== -1) ? href.substring(0, hashIdx) : href;\n";
    html += "    var anchorId = (hashIdx !== -1) ? href.substring(hashIdx + 1) : '';\n";
    html += "    var qIdx = filePart.indexOf('?');\n";
    html += "    if (qIdx !== -1) filePart = filePart.substring(0, qIdx);\n";
    html += "    var refTitle = a.textContent ? a.textContent.trim() : '';\n";
    html += "    var curHref = (typeof window.librisCurrentChapterHref === 'string') ? window.librisCurrentChapterHref : '';\n";
    html += "    var cleanFile = filePart.replace(/^\\.\\//, '');\n";
    html += "    var curBase = curHref.split('/').pop().toLowerCase();\n";
    html += "    var fileBase = cleanFile.split('/').pop().toLowerCase();\n";
    html += "    var isSameFile = (cleanFile === '' || cleanFile === '.' || (curHref !== '' && (cleanFile === curHref || (cleanFile.indexOf('/') === -1 && fileBase === curBase))));\n";
    html += "    if (isSameFile && anchorId !== '') {\n";
    html += "      var targetEl = findTargetByAnchor(anchorId);\n";
    html += "      if (targetEl) {\n";
    html += "        var aCls = ((a.getAttribute('class') || '') + ' ' + (a.getAttribute('epub:type') || '') + ' ' + (a.getAttribute('role') || '')).toLowerCase();\n";
    html += "        var tCls = ((targetEl.getAttribute('class') || '') + ' ' + (targetEl.getAttribute('epub:type') || '') + ' ' + (targetEl.getAttribute('role') || '')).toLowerCase();\n";
    html += "        var tTag = targetEl.tagName ? targetEl.tagName.toLowerCase() : '';\n";
    html += "        var lowerAnc = anchorId.toLowerCase();\n";
    html += "        var lowerRef = refTitle.trim();\n";
    html += "        var isNote = (aCls.indexOf('noteref') !== -1 || aCls.indexOf('footnote') !== -1 || tCls.indexOf('footnote') !== -1 || tCls.indexOf('endnote') !== -1 || tCls.indexOf('rearnote') !== -1 || tTag === 'aside' || lowerAnc.indexOf('fn') === 0 || lowerAnc.indexOf('ftn') !== -1 || lowerAnc.indexOf('edn') !== -1 || lowerAnc.indexOf('note') !== -1 || lowerRef === '*' || /^[\\[\\(\\{]?\\s*(?:\\*+|[0-9]{1,3}|[ivx]+)\\s*[\\]\\)\\}]?$/i.test(lowerRef) || (a.parentNode && a.parentNode.tagName && a.parentNode.tagName.toUpperCase() === 'SUP'));\n";
    html += "        var pChk = a.parentNode; var insideNoteBlock = false;\n";
    html += "        while (pChk && pChk !== document.body) {\n";
    html += "          var pCls = (pChk.getAttribute ? ((pChk.getAttribute('class') || '') + ' ' + (pChk.getAttribute('epub:type') || '')) : '').toLowerCase();\n";
    html += "          var pTag = pChk.tagName ? pChk.tagName.toLowerCase() : '';\n";
    html += "          if (pTag === 'aside' || pCls.indexOf('footnote') !== -1 || pCls.indexOf('endnote') !== -1 || pCls.indexOf('doc-footnote') !== -1) { insideNoteBlock = true; break; }\n";
    html += "          pChk = pChk.parentNode;\n";
    html += "        }\n";
    html += "        var noteEl = targetEl;\n";
    html += "        if (tTag === 'a' && (!targetEl.textContent || targetEl.textContent.trim().length <= 4) && targetEl.parentNode) { noteEl = targetEl.parentNode; }\n";
    html += "        var noteText = noteEl.textContent ? noteEl.textContent.trim() : '';\n";
    html += "        if (!insideNoteBlock && isNote && noteText.length > 0 && noteText.length < 1500) {\n";
    html += "          var fnTitle = refTitle.length > 0 ? refTitle : 'Chú thích';\n";
    html += "          if (window.navigator && window.navigator.cascades) window.navigator.cascades.postMessage('showFootnote:' + encodeURIComponent(fnTitle) + ':' + encodeURIComponent(noteText));\n";
    html += "          return;\n";
    html += "        }\n";
    html += "        scrollToAnchor(anchorId);\n";
    html += "        return;\n";
    html += "      }\n";
    html += "    }\n";
    html += "    if (window.navigator && window.navigator.cascades) {\n";
    html += "      window.navigator.cascades.postMessage('openInternalLink:' + encodeURIComponent(href) + ':' + encodeURIComponent(refTitle));\n";
    html += "    }\n";
    html += "  }, true);\n";
    html += "};\n";

    html += "function initEvents() {";
    html += "  if (window.__librisEventsInited) { return; }";
    html += "  var vp = document.getElementById('reader-viewport');";
    html += "  if (!vp) { return; }";
    html += "  window.__librisEventsInited = true;";
    html += "  var _scrollThrottler = null;";
    html += "  if (vp) vp.addEventListener('scroll', function() {";
    html += "    if (!_scrollThrottler) {";
    html += "      _scrollThrottler = setTimeout(function() {";
    html += "        _scrollThrottler = null;";
    html += "        updatePageInfo();";
    html += "        updateSelectionBubble();";
    html += "      }, 150);";
    html += "    }";
    html += "  }, false);";
    html += "  initSelectionBubble();";
    html += "  restoreChapterHighlights();";
    html += "  initLinksAndFootnotes();";
    html += "  updatePageInfo();";
    html += "  if (readingMode === 'paged') {";
    html += "    if (typeof savedTargetPage !== 'undefined' && savedTargetPage > 1) {";
    html += "      goToPage(savedTargetPage);";
    html += "      setTimeout(function() { goToPage(savedTargetPage); }, 80);";
    html += "      setTimeout(function() { goToPage(savedTargetPage); }, 250);";
    html += "    } else if (typeof savedTargetScroll !== 'undefined' && savedTargetScroll > 0) {";
    html += "      var h = window.innerHeight || (vp ? vp.clientHeight : 0) || 720;";
    html += "      var maxScr = Math.max(1, (vp ? vp.scrollHeight : 0) - h);";
    html += "      var totP = Math.max(1, Math.ceil((vp ? vp.scrollHeight : 0) / h));";
    html += "      var calcP = Math.min(totP, Math.max(1, Math.round((savedTargetScroll / maxScr) * (totP - 1)) + 1));";
    html += "      goToPage(calcP);";
    html += "      setTimeout(function() { goToPage(calcP); }, 80);";
    html += "      setTimeout(function() { goToPage(calcP); }, 250);";
    html += "    }";
    html += "  } else if (readingMode === 'scroll') {";
    html += "    if (typeof savedTargetScroll !== 'undefined' && savedTargetScroll > 0) {";
    html += "      if (vp) vp.scrollTop = savedTargetScroll;";
    html += "      setTimeout(function() { if (vp) vp.scrollTop = savedTargetScroll; updatePageInfo(); }, 80);";
    html += "      setTimeout(function() { if (vp) vp.scrollTop = savedTargetScroll; updatePageInfo(); }, 250);";
    html += "    } else if (typeof savedTargetPage !== 'undefined' && savedTargetPage > 1) {";
    html += "      var h = (vp ? vp.clientHeight : 0) || window.innerHeight || 720;";
    html += "      var calcScroll = Math.round((savedTargetPage - 1) * h);";
    html += "      if (vp) vp.scrollTop = calcScroll;";
    html += "      setTimeout(function() { if (vp) vp.scrollTop = calcScroll; updatePageInfo(); }, 80);";
    html += "      setTimeout(function() { if (vp) vp.scrollTop = calcScroll; updatePageInfo(); }, 250);";
    html += "    }";
    html += "  }";
    if (!m_pendingScrollAnchor.isEmpty()) {
        QString escAnc = escapeJsonString(m_pendingScrollAnchor);
        html += QString("  setTimeout(function() { scrollToAnchor(\"%1\"); }, 80);\n").arg(escAnc);
        html += QString("  setTimeout(function() { scrollToAnchor(\"%1\"); }, 300);\n").arg(escAnc);
    }
    html += "};";
    html += "document.addEventListener('DOMContentLoaded', initEvents, false);";
    html += "window.addEventListener('load', function() { setTimeout(function() { if (!window.__librisEventsInited) { initEvents(); } else { updatePageInfo(); } }, 50); }, false);";
    html += "window.addEventListener('resize', function() {";
    html += "  setTimeout(function() {";
    html += "    updatePageInfo();";
    html += "    updateSelectionBubble();";
    html += "    var vp = document.getElementById('reader-viewport');";
    html += "    var w = window.innerWidth || document.documentElement.clientWidth || (vp ? vp.clientWidth : 0) || screenW;";
    html += "    if (vp && w > 0 && readingMode === 'paged') vp.scrollLeft = (curPage - 1) * w;";
    html += "  }, 100);";
    html += "}, false);";
    html += "setTimeout(initEvents, 50);";
    html += "setTimeout(initEvents, 250);";
    html += "</script>";

    html += "</head><body class=\"" + QString(m_focusModeEnabled ? "libris-focus-on" : "") + "\">";
    html += "<div id=\"libris-focus-top\" class=\"libris-focus-mask\" style=\"height:35vh;\"></div>";
    html += "<div id=\"libris-focus-bottom\" class=\"libris-focus-mask\" style=\"height:37vh;\"></div>";
    html += "<script>"
            "window.__librisFocusTopPct = 35;"
            "window.__librisFocusBandPct = 28;"
            "window.setLibrisFocusMode = function(on) {"
            "  if (on) document.body.classList.add('libris-focus-on');"
            "  else document.body.classList.remove('libris-focus-on');"
            "};"
            "window.moveLibrisFocusRuler = function(clientY) {"
            "  if (!document.body.classList.contains('libris-focus-on')) return;"
            "  var h = window.innerHeight || document.documentElement.clientHeight || 800;"
            "  var pct = Math.round((clientY / h) * 100);"
            "  var band = window.__librisFocusBandPct || 28;"
            "  var topH = Math.max(4, Math.min(100 - band - 4, pct - Math.round(band / 2)));"
            "  var botH = Math.max(4, 100 - topH - band);"
            "  window.__librisFocusTopPct = topH;"
            "  var tEl = document.getElementById('libris-focus-top');"
            "  var bEl = document.getElementById('libris-focus-bottom');"
            "  if (tEl) tEl.style.height = topH + 'vh';"
            "  if (bEl) bEl.style.height = botH + 'vh';"
            "};"
            "(function() {"
            "  var fStartX = 0, fStartY = 0, fDragging = false, fNearBorder = false;"
            "  document.addEventListener('touchstart', function(e) {"
            "    if (!document.body.classList.contains('libris-focus-on') || !e.touches || e.touches.length !== 1) return;"
            "    var t = e.touches[0];"
            "    fStartX = t.clientX; fStartY = t.clientY; fDragging = false;"
            "    var h = window.innerHeight || document.documentElement.clientHeight || 800;"
            "    var topPx = h * ((window.__librisFocusTopPct || 35) / 100);"
            "    var botPx = h * (((window.__librisFocusTopPct || 35) + (window.__librisFocusBandPct || 28)) / 100);"
            "    fNearBorder = (Math.abs(fStartY - topPx) < 44 || Math.abs(fStartY - botPx) < 44);"
            "  }, false);"
            "  document.addEventListener('touchmove', function(e) {"
            "    if (!document.body.classList.contains('libris-focus-on') || !e.touches || e.touches.length !== 1) return;"
            "    var t = e.touches[0];"
            "    var dx = t.clientX - fStartX, dy = t.clientY - fStartY;"
            "    if (fDragging || (Math.abs(dy) > 14 && Math.abs(dy) > Math.abs(dx) * 1.4 && (typeof readingMode === 'undefined' || readingMode === 'paged' || fNearBorder))) {"
            "      fDragging = true;"
            "      if (fNearBorder && e.preventDefault) e.preventDefault();"
            "      window.moveLibrisFocusRuler(t.clientY);"
            "    }"
            "  }, false);"
            "  document.addEventListener('touchend', function(e) {"
            "    if (document.body.classList.contains('libris-focus-on') && !fDragging && fNearBorder && e.changedTouches && e.changedTouches.length === 1) {"
            "      var t = e.changedTouches[0];"
            "      var dx = t.clientX - fStartX, dy = t.clientY - fStartY;"
            "      var w = window.innerWidth || document.documentElement.clientWidth || 720;"
            "      if (Math.abs(dx) < 16 && Math.abs(dy) < 16 && t.clientX >= w * 0.22 && t.clientX <= w * 0.78) {"
            "        window.moveLibrisFocusRuler(t.clientY);"
            "      }"
            "    }"
            "    fDragging = false; fNearBorder = false;"
            "  }, false);"
            "})();"
            "</script>";
    html += "<div id=\"reader-viewport\">";
    html += content;
    html += "</div>";

    // Floating selection bubble anchored directly to selected text line
    bool isVi = (m_language == "vi");
    QString hlTitle = isVi ? QString::fromUtf8("Đánh dấu") : "Highlight";
    QString speakTitle = isVi ? QString::fromUtf8("Đọc") : "Speak";
    QString copyTitle = isVi ? QString::fromUtf8("Sao chép") : "Copy";
    QString dictTitle = isVi ? QString::fromUtf8("Tra từ điển") : "Dictionary";
    QString transTitle = isVi ? QString::fromUtf8("Dịch văn bản") : "Translate";
    QString searchTitle = isVi ? QString::fromUtf8("Tìm kiếm") : "Search";
    QString closeTitle = isVi ? QString::fromUtf8("Đóng") : "Close";

    html += "<div id=\"libris-selection-bubble\"><div class=\"libris-bubble-row\">";

    // 1. Highlight
    html += QString("<div class=\"libris-bubble-btn\" id=\"b-btn-highlight\" title=\"%1\">"
                    "<svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"#E8ECF2\">"
                    "<path d=\"M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z\"/>"
                    "<path d=\"M3 21h18v2H3z\"/>"
                    "</svg></div>").arg(hlTitle);

    // 2. Speak
    html += QString("<div class=\"libris-bubble-btn\" id=\"b-btn-speak\" title=\"%1\">"
                    "<svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"#E8ECF2\">"
                    "<path d=\"M3 9v6h4l5 5V4L7 9H3zm13.5 3c0-1.77-1.02-3.29-2.5-4.03v8.05c1.48-.73 2.5-2.25 2.5-4.02zM14 3.23v2.06c2.89.86 5 3.54 5 6.71s-2.11 5.85-5 6.71v2.06c4.01-.91 7-4.49 7-8.77s-2.99-7.86-7-8.77z\"/>"
                    "</svg></div>").arg(speakTitle);

    // 3. Copy
    html += QString("<div class=\"libris-bubble-btn\" id=\"b-btn-copy\" title=\"%1\">"
                    "<svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"#E8ECF2\">"
                    "<path d=\"M16 1H4c-1.1 0-2 .9-2 2v14h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z\"/>"
                    "</svg></div>").arg(copyTitle);

    // 4. Dictionary
    html += QString("<div class=\"libris-bubble-btn\" id=\"b-btn-dict\" title=\"%1\">"
                    "<svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"#E8ECF2\">"
                    "<path d=\"M18 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zM6 4h5v8l-2.5-1.5L6 12V4z\"/>"
                    "</svg></div>").arg(dictTitle);

    // 5. Translate
    html += QString("<div class=\"libris-bubble-btn\" id=\"b-btn-trans\" title=\"%1\">"
                    "<svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"#E8ECF2\">"
                    "<path d=\"M12.87 15.07l-2.54-2.51.03-.08c1.74-1.94 2.98-4.17 3.71-6.49H17V4h-7V2H8v2H1v2h11.17C11.5 7.92 10.44 9.75 9 11.35 8.07 10.32 7.3 9.19 6.69 8h-2c.73 1.63 1.73 3.17 2.98 4.56l-5.09 5.02L4 19l5-5 3.11 3.11.76-2.04zM18.5 10h-2L12 22h2l1.12-3h4.75L21 22h2l-4.5-12zm-2.62 7l1.62-4.33L19.12 17h-3.24z\"/>"
                    "</svg></div>").arg(transTitle);

    // 6. Search
    html += QString("<div class=\"libris-bubble-btn\" id=\"b-btn-search\" title=\"%1\">"
                    "<svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"#E8ECF2\">"
                    "<path d=\"M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z\"/>"
                    "</svg></div>").arg(searchTitle);

    // 7. Close
    html += QString("<div class=\"libris-bubble-btn\" id=\"b-btn-close\" title=\"%1\">"
                    "<svg width=\"15\" height=\"15\" viewBox=\"0 0 24 24\" fill=\"#E8ECF2\">"
                    "<path d=\"M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z\"/>"
                    "</svg></div>").arg(closeTitle);
    html += "</div></div>";
    html += "</body></html>";

    m_cachedContentChapter = m_currentChapterIndex;
    m_cachedContentTheme = m_theme;
    m_cachedContentFontSize = m_fontSize;
    m_cachedContentFontFamily = m_fontFamily;
    m_cachedContentSpacing = m_lineSpacing;
    m_cachedContentMode = m_readingMode;
    m_cachedContentHtml = html;

    return html;
}

int BookManager::fontSize() const
{
    return m_fontSize;
}

void BookManager::setFontSize(int size)
{
    if (size >= 12 && size <= 36 && size != m_fontSize) {
        m_fontSize = size;
        emit fontSizeChanged();
        emit currentChapterChanged(); // Updates WebView content with new font size!
        saveSettings();
    }
}

QString BookManager::fontFamily() const
{
    return m_fontFamily;
}

void BookManager::setFontFamily(const QString &family)
{
    if (!family.isEmpty() && family != m_fontFamily) {
        m_fontFamily = family;
        emit fontFamilyChanged();
        m_cachedContentHtml.clear();
        emit currentChapterChanged(); // Updates WebView content with new font family!
        saveSettings();
    }
}

QString BookManager::theme() const
{
    return m_theme;
}

QString BookManager::appTheme() const
{
    return m_appTheme;
}

void BookManager::setAppTheme(const QString &theme)
{
    if (theme != m_appTheme && (theme == "dark" || theme == "light")) {
        m_appTheme = theme;
        if (m_theme != "sepia" && m_theme != m_appTheme) {
            m_theme = m_appTheme;
            m_cachedContentHtml.clear();
            emit themeChanged();
        }
        applyVisualStyle();
        emit appThemeChanged();
        saveSettings();
    }
}

void BookManager::setOnboardingCompleted(bool completed)
{
    if (m_onboardingCompleted != completed) {
        m_onboardingCompleted = completed;
        saveSettings();
        emit onboardingCompletedChanged();
    }
}

void BookManager::requestOnboardingGuide()
{
    emit openOnboardingGuideRequested();
}

void BookManager::applyVisualStyle()
{
    if (bb::cascades::Application::instance() && bb::cascades::Application::instance()->themeSupport()) {
        // Keep native BlackBerry 10 dark chrome for top TitleBars & bottom ActionBars even in Light Mode
        bb::cascades::VisualStyle::Type style = bb::cascades::VisualStyle::Dark;
        bb::cascades::Color primary = bb::cascades::Color::fromARGB(0xFFD4A359);
        bb::cascades::Color primaryBase = bb::cascades::Color::fromARGB(0xFFA87932);
        bb::cascades::Application::instance()->themeSupport()->setVisualStyleAndPrimaryColor(style, primary, primaryBase);
    }
}

void BookManager::setTheme(const QString &theme)
{
    if (theme != m_theme) {
        m_theme = theme;
        emit themeChanged();
        emit currentChapterChanged(); // Updates WebView content with new background/colors!
        saveSettings();

        // Invert PDF colors if switched to/from dark
        if (isPdf() && m_pdfParser) {
            bool wantInverted = (m_theme == "dark");
            if (m_pdfParser->isInvertedColors() != wantInverted) {
                m_pdfParser->switchInvertedColors();
                m_pdfInverted = m_pdfParser->isInvertedColors();
                emit pdfInvertedChanged();
                resetPdfPagesModelImages();
                requestPdfStreamPage(m_currentChapterIndex);
            }
        }
    }
}

QString BookManager::readingMode() const
{
    return m_readingMode;
}

void BookManager::setReadingMode(const QString &mode)
{
    if (mode != m_readingMode) {
        m_readingMode = mode;
        m_cachedContentHtml.clear();
        m_shouldRestorePageOnLoad = true;
        emit readingModeChanged();
        saveSettings();
    }
}

void BookManager::toggleReadingMode()
{
    if (m_readingMode == "paged") {
        setReadingMode("scroll");
    } else {
        setReadingMode("paged");
    }
}

QString BookManager::columnLayout() const
{
    return m_columnLayout;
}

void BookManager::setColumnLayout(const QString &layout)
{
    QString normalized = (layout == "double") ? "double" : "single";
    if (normalized != m_columnLayout) {
        m_columnLayout = normalized;
        m_cachedContentHtml.clear();
        m_shouldRestorePageOnLoad = true;
        emit columnLayoutChanged();
        saveSettings();
        emit currentChapterChanged();
    }
}

void BookManager::toggleColumnLayout()
{
    setColumnLayout(m_columnLayout == "double" ? "single" : "double");
}

bool BookManager::isTwoColumnActive() const
{
    return (m_columnLayout == "double");
}

bool BookManager::mangaRtlMode() const
{
    return m_mangaRtlMode;
}

void BookManager::setMangaRtlMode(bool enabled)
{
    if (m_mangaRtlMode != enabled) {
        m_mangaRtlMode = enabled;
        emit mangaRtlModeChanged();
        saveSettings();
        if (m_currentFormat == "cbz" && !m_currentFilePath.isEmpty()) {
            renderCurrentChapter();
        }
    }
}

void BookManager::toggleMangaRtlMode()
{
    setMangaRtlMode(!m_mangaRtlMode);
}

void BookManager::renderCurrentChapter()
{
    if (m_currentFilePath.isEmpty()) return;
    m_cachedContentHtml.clear();
    m_cachedContentChapter = -1;
    emit currentChapterChanged();
}

int BookManager::readingProgress() const
{
    if (m_totalChapters <= 0) return 0;
    if (isPdf()) {
        if (m_totalChapters <= 1) return 100;
        return qBound(0, (m_currentChapterIndex * 100) / m_totalChapters, 100);
    }

    if (m_totalChapters == 1) {
        if (m_totalPagesInChapter > 1) {
            return qBound(0, (m_currentPageInChapter * 100) / m_totalPagesInChapter, 100);
        }
        if (!isLoadingBook() && m_totalPagesInChapter == 1 && m_currentPageInChapter >= 1) {
            return 100;
        }
        return 0;
    }

    // Multi-chapter books
    if (m_currentChapterIndex >= m_totalChapters - 1) {
        // Last chapter: ensure readers reach 100% when on the final page
        if (m_totalPagesInChapter > 1) {
            if (m_currentPageInChapter >= m_totalPagesInChapter) {
                return 100;
            }
            double pageFraction = (double)(m_currentPageInChapter - 1) / (double)(m_totalPagesInChapter - 1);
            double overall = ((double)(m_totalChapters - 1) + pageFraction) / (double)m_totalChapters;
            return qBound(0, (int)(overall * 100.0), 100);
        } else {
            // Last chapter has only 1 page
            if (!isLoadingBook()) {
                return 100;
            }
            return qBound(0, ((m_totalChapters - 1) * 100) / m_totalChapters, 100);
        }
    }

    double pageFraction = 0.0;
    if (m_totalPagesInChapter > 1) {
        pageFraction = (double)(m_currentPageInChapter - 1) / (double)m_totalPagesInChapter;
    }
    double overall = ((double)m_currentChapterIndex + pageFraction) / (double)m_totalChapters;
    int prog = qBound(0, (int)(overall * 100.0), 100);
    return prog;
}

int BookManager::currentPageInChapter() const
{
    return m_currentPageInChapter;
}

int BookManager::totalPagesInChapter() const
{
    return m_totalPagesInChapter;
}

void BookManager::setPagePosition(int current, int total, int scrollPos)
{
    bool changed = (m_currentPageInChapter != current || m_totalPagesInChapter != total || m_currentScrollPos != scrollPos);
    m_currentPageInChapter = (current >= 1) ? current : 1;
    m_totalPagesInChapter = (total >= 1) ? total : 1;
    m_currentScrollPos = (scrollPos >= 0) ? scrollPos : 0;

    if (changed) {
        emit pagePositionChanged();
        emit readingProgressChanged();

        if (!m_currentFilePath.isEmpty()) {
            int prog = readingProgress();
            m_lastBookProgress = prog;
            emit lastBookChanged();

            for (int i = 0; i < m_allBooks.size(); ++i) {
                if (m_allBooks[i].value("filePath").toString() == m_currentFilePath) {
                    m_allBooks[i]["progress"] = prog;
                    m_allBooks[i]["progressStr"] = QString("%1%").arg(prog);
                    m_allBooks[i]["isCompleted"] = (prog >= 100);
                    break;
                }
            }

            // Debounce disk writes to avoid freezing the UI thread during scrolling:
            // Flush to flash memory 1.5 seconds after scrolling stops.
            m_progressSaveTimer.start(1500);
        }
    }
}

void BookManager::onProgressSaveTimeout()
{
    if (m_currentFilePath.isEmpty()) return;

    int prog = readingProgress();
    updateBookProgressInLibrary(m_currentFilePath, prog);

    QSettings settings("Libris", "Libris");
    settings.setValue(QString("progressPct/%1").arg(m_currentFilePath), prog);
    settings.setValue(QString("progress/%1").arg(m_currentFilePath), m_currentChapterIndex);
    settings.setValue(QString("pageInChapter/%1").arg(m_currentFilePath), m_currentPageInChapter);
    settings.setValue(QString("totalPagesInChapter/%1").arg(m_currentFilePath), m_totalPagesInChapter);
    settings.setValue(QString("scrollPos/%1").arg(m_currentFilePath), m_currentScrollPos);
    settings.setValue(QString("lastReadTime/%1").arg(m_currentFilePath), (qlonglong)QDateTime::currentDateTime().toMSecsSinceEpoch());
    settings.setValue("lastBook/filePath", m_currentFilePath);
    settings.setValue("lastBook/title", m_currentBookTitle);
    settings.setValue("lastBook/author", m_currentBookAuthor);
    settings.setValue("lastBook/cover", m_currentCoverPath);
    settings.setValue("lastBook/format", m_currentFormat);
    settings.setValue("lastBook/progress", prog);
    settings.sync();
    if (!m_suppressPersistentSync) {
        schedulePersistentSync();
    }
}

void BookManager::loadCurrentChapter()
{
    m_cachedContentHtml.clear();
    emit currentChapterChanged();
    emit readingProgressChanged();
    emit bookmarksChanged();
    saveSettings();

    if (m_tts) {
        m_tts->setBookContext(m_currentBookTitle, m_currentBookAuthor);
    }
}

TtsManager* BookManager::ttsManager()
{
    return m_tts;
}

OpdsManager* BookManager::opdsManager()
{
    if (!m_opdsManager) {
        m_opdsManager = new OpdsManager(this, this);
    }
    return m_opdsManager;
}

void BookManager::startTts()
{
    if (!m_tts) return;
    if (m_currentFormat == "cbz") {
        return;
    }
    m_pendingTtsAfterPdfRender = false;
    m_tts->setBookContext(m_currentBookTitle, m_currentBookAuthor);

    if (isPdf()) {
        refreshPdfPageText(m_currentChapterIndex);
        if (m_currentPdfText.trimmed().isEmpty() && m_pdfParser) {
            int nextOcrPg = m_pdfParser->findNextTextPage(m_currentChapterIndex, 25);
            if (nextOcrPg > 0 && nextOcrPg != m_currentChapterIndex) {
                goToChapter(nextOcrPg);
                refreshPdfPageText(nextOcrPg);
            }
        }

        if (!m_currentPdfText.trimmed().isEmpty()) {
            m_tts->startWithText(m_currentPdfText, QString(), false);
            if (m_tts->totalSentences() > 0) {
                int targetSentence = 0;
                if (m_isPdfReflow && m_currentPageInChapter > 1 && m_totalPagesInChapter > 1) {
                    targetSentence = qBound(0, (int)((double)(m_currentPageInChapter - 1) / m_totalPagesInChapter * m_tts->totalSentences()), m_tts->totalSentences() - 1);
                }
                if (targetSentence > 0) {
                    m_tts->seekSentence(targetSentence);
                } else {
                    m_tts->play();
                }
                return;
            }
        }
        QMetaObject::invokeMethod(m_tts, "errorOccurred", Qt::DirectConnection, Q_ARG(QString, tr("Trang PDF này chưa có lớp chữ OCR để đọc giọng nói.")));
        return;
    }

    int attempts = 0;
    while (attempts < 10) {
        attempts++;
        QString raw;
        if (m_currentFormat == "epub") {
            raw = m_epubParser.getChapterContent(m_currentChapterIndex, m_cacheDir);
        } else if (m_currentFormat == "txt") {
            raw = m_txtParser.getChapterContent(m_currentChapterIndex);
        } else if (m_currentFormat == "fb2") {
            raw = m_fb2Parser.getChapterContent(m_currentChapterIndex);
        } else if (m_currentFormat == "mobi") {
            raw = m_mobiParser.getChapterContent(m_currentChapterIndex, m_cacheDir);
        } else {
            raw = currentContent();
        }

        if (!raw.isEmpty()) {
            m_tts->startWithText(raw, QString(), false);
            if (m_tts->totalSentences() > 0) {
                int targetSentence = 0;
                if (m_currentPageInChapter > 1 && m_totalPagesInChapter > 1) {
                    targetSentence = qBound(0, (int)((double)(m_currentPageInChapter - 1) / m_totalPagesInChapter * m_tts->totalSentences()), m_tts->totalSentences() - 1);
                }
                if (targetSentence > 0) {
                    m_tts->seekSentence(targetSentence);
                } else {
                    m_tts->play();
                }
                return;
            }
        }

        if (m_currentChapterIndex >= m_totalChapters - 1) {
            return;
        }

        // Current chapter (like cover page / intro) has no readable text.
        // Automatically advance to the next chapter to find readable content and update the reader view.
        m_currentChapterIndex++;
        m_currentPageInChapter = 1;
        m_totalPagesInChapter = 1;
        loadCurrentChapter();
    }
}

void BookManager::onTtsChapterFinished()
{
    if (m_tts && m_tts->sleepAtChapterEnd()) {
        m_tts->setSleepAtChapterEnd(false);
        stopTts();
        return;
    }
    if (isPdf() && m_ttsAutoAdvance && m_currentChapterIndex < m_totalChapters) {
        int nextOcrPg = m_pdfParser ? m_pdfParser->findNextTextPage(m_currentChapterIndex + 1, 25) : -1;
        if (nextOcrPg > 0 && nextOcrPg <= m_totalChapters) {
            bool waitRender = m_pdfZoomMode;
            if (waitRender) {
                m_pendingTtsAfterPdfRender = true;
            }
            goToChapter(nextOcrPg);
            if (!waitRender) {
                startTts();
            }
        } else {
            stopTts();
        }
    } else if (!isPdf() && m_ttsAutoAdvance && m_currentChapterIndex < m_totalChapters - 1) {
        m_currentChapterIndex++;
        m_currentPageInChapter = 1;
        m_totalPagesInChapter = 1;
        loadCurrentChapter();
        startTts();
    } else {
        stopTts();
    }
}

void BookManager::stopTts()
{
    m_pendingTtsAfterPdfRender = false;
    if (m_tts) {
        m_tts->stop();
    }
}

void BookManager::setTtsAutoAdvance(bool advance)
{
    if (m_ttsAutoAdvance != advance) {
        m_ttsAutoAdvance = advance;
        saveSettings();
        emit ttsAutoAdvanceChanged(m_ttsAutoAdvance);
    }
}

void BookManager::toggleTts()
{
    if (m_tts && m_tts->isActive()) {
        stopTts();
    } else {
        startTts();
    }
}

void BookManager::nextChapter()
{
    if (isPdf()) {
        if (!m_pdfZoomMode) {
            if (m_currentChapterIndex < m_totalChapters) {
                bool wasTts = m_tts && m_tts->isActive();
                m_currentChapterIndex++;
                m_currentPageInChapter = m_currentChapterIndex;
                m_totalPagesInChapter = m_totalChapters;
                refreshPdfPageText(m_currentChapterIndex);
                if (m_isPdfReflow) {
                    updatePdfReflowContent();
                } else {
                    emit currentChapterChanged();
                    emit pdfJumpToPageRequested(m_currentChapterIndex);
                    requestPdfStreamPage(m_currentChapterIndex);
                }
                emit readingProgressChanged();
                emit bookmarksChanged();
                emit pagePositionChanged();
                m_progressSaveTimer.start(1500);
                if (wasTts) {
                    startTts();
                }
            }
            return;
        }
        if (m_pdfParser) {
            m_pdfParser->nextPage();
        }
        return;
    }

    if (m_currentChapterIndex < m_totalChapters - 1) {
        bool wasTts = m_tts && m_tts->isActive();
        m_currentChapterIndex++;
        m_currentPageInChapter = 1;
        m_totalPagesInChapter = 1;
        m_currentScrollPos = 0;
        m_shouldRestorePageOnLoad = false;
        loadCurrentChapter();
        if (wasTts) {
            startTts();
        }
    }
}

void BookManager::prevChapter()
{
    if (isPdf()) {
        if (!m_pdfZoomMode) {
            if (m_currentChapterIndex > 1) {
                bool wasTts = m_tts && m_tts->isActive();
                m_currentChapterIndex--;
                m_currentPageInChapter = m_currentChapterIndex;
                m_totalPagesInChapter = m_totalChapters;
                refreshPdfPageText(m_currentChapterIndex);
                if (m_isPdfReflow) {
                    updatePdfReflowContent();
                } else {
                    emit currentChapterChanged();
                    emit pdfJumpToPageRequested(m_currentChapterIndex);
                    requestPdfStreamPage(m_currentChapterIndex);
                }
                emit readingProgressChanged();
                emit bookmarksChanged();
                emit pagePositionChanged();
                m_progressSaveTimer.start(1500);
                if (wasTts) {
                    startTts();
                }
            }
            return;
        }
        if (m_pdfParser) {
            m_pdfParser->prevPage();
        }
        return;
    }

    if (m_currentChapterIndex > 0) {
        bool wasTts = m_tts && m_tts->isActive();
        m_currentChapterIndex--;
        m_currentPageInChapter = 1;
        m_totalPagesInChapter = 1;
        m_currentScrollPos = 0;
        m_shouldRestorePageOnLoad = false;
        loadCurrentChapter();
        if (wasTts) {
            startTts();
        }
    }
}

void BookManager::goToChapter(int index)
{
    if (isPdf()) {
        if (index >= 1 && index <= m_totalChapters) {
            if (!m_pdfZoomMode) {
                bool wasTts = m_tts && m_tts->isActive();
                if (wasTts) {
                    stopTts();
                }
                m_currentChapterIndex = index;
                m_currentPageInChapter = index;
                m_totalPagesInChapter = m_totalChapters;
                refreshPdfPageText(index);
                if (m_isPdfReflow) {
                    updatePdfReflowContent();
                } else {
                    emit currentChapterChanged();
                    emit pdfJumpToPageRequested(index);
                    requestPdfStreamPage(index);
                }
                emit readingProgressChanged();
                emit bookmarksChanged();
                emit pagePositionChanged();
                m_progressSaveTimer.start(1500);
                if (wasTts) {
                    startTts();
                }
                return;
            }
            if (m_pdfParser) {
                m_pdfParser->navigateToPage(index);
            }
        }
        return;
    }

    if (index >= 0 && index < m_totalChapters) {
        bool wasTts = m_tts && m_tts->isActive();
        if (wasTts) {
            stopTts();
        }
        m_currentChapterIndex = index;
        m_currentPageInChapter = 1;
        m_totalPagesInChapter = 1;
        m_currentScrollPos = 0;
        m_shouldRestorePageOnLoad = false;
        loadCurrentChapter();
        if (wasTts) {
            startTts();
        }
    }
}

static bool isGenericChapterTitle(const QString &title)
{
    QString t = title.trimmed();
    if (t.isEmpty()) return true;

    if (t.endsWith(".xhtml", Qt::CaseInsensitive) || t.endsWith(".html", Qt::CaseInsensitive) ||
        t.endsWith(".htm", Qt::CaseInsensitive) || t.contains(QRegExp("^(?:text\\/)?part[0-9]+", Qt::CaseInsensitive))) {
        return true;
    }

    QRegExp chCodeRe("^(?:ch|chap|chapter|sec|section|part)[_-]?[0-9]+$", Qt::CaseInsensitive);
    if (chCodeRe.exactMatch(t)) return true;

    QRegExp justNumRe("^#?\\s*([0-9]+|[IVXLCDM]+)\\s*[:.-]?\\s*$", Qt::CaseInsensitive);
    if (justNumRe.exactMatch(t)) return true;

    QRegExp genericRe("^\\s*(Chương|Chapter|Phần|Part|Hồi|Quyển|Tập|Mục|Tiết|Bài|Book|Section)\\s+([0-9]+|[IVXLCDM]+)\\s*[:.-]?\\s*$", Qt::CaseInsensitive);
    if (genericRe.exactMatch(t)) return true;

    QRegExp dupRe("^\\s*(?:Chương|Chapter)\\s+([0-9]+|[IVXLCDM]+)\\s*[:\\-–—]\\s*(?:(?:Chương|Chapter)\\s+)?\\1\\s*$", Qt::CaseInsensitive);
    if (dupRe.exactMatch(t)) return true;

    QRegExp singleWordRe("^(Chương|Chapter|Phần|Part|Hồi|Quyển|Tập|Mục|Tiết|Bài|Book|Section|Untitled|Page|Trang)\\s*$", Qt::CaseInsensitive);
    if (singleWordRe.exactMatch(t)) return true;

    return false;
}

static QString combineChapterTitle(const QString &baseTitle, const QString &subTitle, int chapterNumber)
{
    QString b = baseTitle.trimmed();
    QString s = subTitle.trimmed();

    if (s.isEmpty()) return b;

    while (!s.isEmpty() && (s.startsWith('-') || s.startsWith(QChar(0x2014)) || 
           s.startsWith(QChar(0x2013)) || s.startsWith(':') || s.startsWith(' ') ||
           s.startsWith(QChar(0x2022)) || s.startsWith('*'))) {
        s = s.mid(1).trimmed();
    }
    while (s.endsWith('.')) {
        s = s.left(s.length() - 1).trimmed();
    }
    if (s.isEmpty()) return b;

    QRegExp fullChapterRe("^\\s*((?:Chương|Chapter|Hồi|Phần|Quyển|Tập|Mục|Tiết|Bài|Book|Section)\\s+[0-9IVXLCDM]+)\\s*[:\\-–—.]\\s*(.+)$", Qt::CaseInsensitive);
    if (fullChapterRe.indexIn(s) != -1) {
        return QString("%1: %2").arg(fullChapterRe.cap(1).trimmed(), fullChapterRe.cap(2).trimmed());
    }

    if (isGenericChapterTitle(s)) {
        if (!isGenericChapterTitle(b)) return b;
        return s.isEmpty() ? b : s;
    }

    bool bIsFileName = (b.endsWith(".xhtml", Qt::CaseInsensitive) || b.endsWith(".html", Qt::CaseInsensitive) ||
                        b.endsWith(".htm", Qt::CaseInsensitive) || b.contains(QRegExp("^(?:text\\/)?part[0-9]+", Qt::CaseInsensitive)));

    if (b.isEmpty() || bIsFileName) {
        if (chapterNumber > 0) {
            return QString("Chương %1: %2").arg(chapterNumber).arg(s);
        }
        return s;
    }

    if (!isGenericChapterTitle(b)) {
        return b;
    }

    while (!b.isEmpty() && (b.endsWith(':') || b.endsWith('-') || b.endsWith(QChar(0x2014)) || 
           b.endsWith(QChar(0x2013)) || b.endsWith('.') || b.endsWith(' '))) {
        b = b.left(b.length() - 1).trimmed();
    }

    QRegExp justNumRe("^#?\\s*([0-9]+|[IVXLCDM]+)$", Qt::CaseInsensitive);
    if (justNumRe.exactMatch(b)) {
        return QString("Chương %1: %2").arg(b, s);
    }

    return QString("%1: %2").arg(b, s);
}

void BookManager::updateTocList()
{
    m_tocList.clear();

    if (isPdf() && m_pdfParser) {
        QList<EpubTocEntry> pToc = m_pdfParser->tocEntries();
        for (int i = 0; i < pToc.size(); ++i) {
            QVariantMap item;
            item["title"] = pToc[i].title;
            item["index"] = pToc[i].chapterIndex;
            item["chapterIndex"] = pToc[i].chapterIndex;
            item["page"] = pToc[i].chapterIndex;
            item["pageNumber"] = pToc[i].chapterIndex;
            item["level"] = pToc[i].level;
            item["anchor"] = pToc[i].anchor;
            m_tocList.append(item);
        }
    } else {
        if (m_currentFormat == "epub") {
            QList<EpubTocEntry> toc = m_epubParser.tocEntries();
            const QList<EpubChapter> &chapters = m_epubParser.chapters();

            // If TOC is empty or has only 1-2 entries while chapters has many, populate directly from chapters
            if (toc.isEmpty() || (toc.size() <= 1 && chapters.size() > 1)) {
                for (int i = 0; i < chapters.size(); ++i) {
                    QVariantMap item;
                    QString title = chapters[i].title.trimmed();
                    if (isGenericChapterTitle(title)) {
                        QString extracted = m_epubParser.extractChapterTitle(i);
                        if (!extracted.isEmpty()) {
                            title = combineChapterTitle(title, extracted, i + 1);
                        }
                    }
                    item["title"] = title;
                    item["index"] = i;
                    item["chapterIndex"] = i;
                    item["page"] = i + 1;
                    item["pageNumber"] = i + 1;
                    item["level"] = 0;
                    item["anchor"] = QString();
                    m_tocList.append(item);
                }
            } else {
                for (int i = 0; i < toc.size(); ++i) {
                    QVariantMap item;
                    QString title = toc[i].title.trimmed();
                    int cIdx = toc[i].chapterIndex;
                    if (isGenericChapterTitle(title) && cIdx >= 0 && cIdx < chapters.size()) {
                        QString chTitle = chapters[cIdx].title.trimmed();
                        if (!isGenericChapterTitle(chTitle)) {
                            title = combineChapterTitle(title, chTitle, cIdx + 1);
                        } else {
                            QString extracted = m_epubParser.extractChapterTitle(cIdx, toc[i].anchor);
                            if (!extracted.isEmpty()) {
                                title = combineChapterTitle(title, extracted, cIdx + 1);
                            }
                        }
                    }
                    item["title"] = title;
                    item["index"] = cIdx;
                    item["chapterIndex"] = cIdx;
                    item["page"] = cIdx + 1;
                    item["pageNumber"] = cIdx + 1;
                    item["level"] = toc[i].level;
                    item["anchor"] = toc[i].anchor;
                    m_tocList.append(item);
                }
            }
        } else {
            QList<EpubChapter> ch;
            if (m_currentFormat == "txt") ch = m_txtParser.chapters();
            else if (m_currentFormat == "fb2") ch = m_fb2Parser.chapters();
            else if (m_currentFormat == "cbz") ch = m_cbzParser.chapters();
            else if (m_currentFormat == "mobi") {
                QList<EpubTocEntry> mToc = m_mobiParser.tocEntries();
                if (!mToc.isEmpty()) {
                    for (int i = 0; i < mToc.size(); ++i) {
                        QVariantMap item;
                        item["title"] = mToc[i].title;
                        item["index"] = mToc[i].chapterIndex;
                        item["chapterIndex"] = mToc[i].chapterIndex;
                        item["page"] = mToc[i].chapterIndex + 1;
                        item["pageNumber"] = mToc[i].chapterIndex + 1;
                        item["level"] = mToc[i].level;
                        item["anchor"] = mToc[i].anchor;
                        m_tocList.append(item);
                    }
                } else {
                    ch = m_mobiParser.chapters();
                }
            }

            if (!ch.isEmpty()) {
                for (int i = 0; i < ch.size(); ++i) {
                    QVariantMap item;
                    item["title"] = ch[i].title;
                    item["index"] = i;
                    item["chapterIndex"] = i;
                    item["page"] = i + 1;
                    item["pageNumber"] = i + 1;
                    item["level"] = 0;
                    item["anchor"] = QString();
                    m_tocList.append(item);
                }
            }
        }
    }

    emit tocListChanged();
}

QVariantList BookManager::getChapterList() const
{
    return m_tocList;
}

QString BookManager::consumePendingScrollAnchor()
{
    QString a = m_pendingScrollAnchor;
    m_pendingScrollAnchor.clear();
    return a;
}

void BookManager::jumpToToc(int chapterIndex, int page, const QString &anchor)
{
    m_pendingScrollAnchor = anchor.trimmed();
    if (isPdf()) {
        int target = (page > 0) ? page : chapterIndex;
        goToChapter(target);
    } else {
        if (chapterIndex >= 0 && chapterIndex < m_totalChapters) {
            if (chapterIndex == m_currentChapterIndex) {
                if (!m_pendingScrollAnchor.isEmpty()) {
                    QString anc = m_pendingScrollAnchor;
                    m_pendingScrollAnchor.clear();
                    emit requestScrollToAnchor(anc);
                }
            } else {
                goToChapter(chapterIndex);
            }
        }
    }
}

namespace {

QString decodeHtmlEntitiesForNote(QString text)
{
    text.replace("&nbsp;", " ", Qt::CaseInsensitive);
    text.replace("&#160;", " ", Qt::CaseInsensitive);
    text.replace("&amp;", "&", Qt::CaseInsensitive);
    text.replace("&quot;", "\"", Qt::CaseInsensitive);
    text.replace("&apos;", "'", Qt::CaseInsensitive);
    text.replace("&#39;", "'", Qt::CaseInsensitive);
    text.replace("&lt;", "<", Qt::CaseInsensitive);
    text.replace("&gt;", ">", Qt::CaseInsensitive);
    text.replace("&ndash;", QString(QChar(0x2013)), Qt::CaseInsensitive);
    text.replace("&mdash;", QString(QChar(0x2014)), Qt::CaseInsensitive);
    text.replace("&lsquo;", QString(QChar(0x2018)), Qt::CaseInsensitive);
    text.replace("&rsquo;", QString(QChar(0x2019)), Qt::CaseInsensitive);
    text.replace("&ldquo;", QString(QChar(0x201C)), Qt::CaseInsensitive);
    text.replace("&rdquo;", QString(QChar(0x201D)), Qt::CaseInsensitive);
    text.replace("&hellip;", "...", Qt::CaseInsensitive);

    QRegExp numRx("&#(x?[0-9a-fA-F]+);");
    int pos = 0;
    while ((pos = numRx.indexIn(text, pos)) != -1) {
        QString rawNum = numRx.cap(1);
        bool ok = false;
        uint cp = rawNum.startsWith('x', Qt::CaseInsensitive)
                  ? rawNum.mid(1).toUInt(&ok, 16)
                  : rawNum.toUInt(&ok, 10);
        if (ok && cp > 0 && cp <= 0xFFFF) {
            QString rep = QString(QChar(cp));
            text.replace(pos, numRx.matchedLength(), rep);
            pos += rep.length();
        } else {
            pos += numRx.matchedLength();
        }
    }
    return text;
}

QString htmlFragmentToPlainNote(QString frag)
{
    frag.remove(minimalRx("<script[^>]*>[\\s\\S]*</script>", Qt::CaseInsensitive));
    frag.remove(minimalRx("<style[^>]*>[\\s\\S]*</style>", Qt::CaseInsensitive));
    frag.replace(QRegExp("<br\\s*/?>", Qt::CaseInsensitive), "\n");
    frag.replace(QRegExp("</(p|li|div|aside|dd|dt|tr|h[1-6])>", Qt::CaseInsensitive), "\n");
    frag.remove(QRegExp("<[^>]*>"));
    frag = decodeHtmlEntitiesForNote(frag);
    frag.replace(QChar(0x00A0), ' ');

    QStringList lines = frag.split('\n');
    QStringList cleaned;
    foreach (QString line, lines) {
        line.replace(QRegExp("[ \\t\\r]+"), " ");
        line = line.trimmed();
        if (!line.isEmpty()) {
            cleaned.append(line);
        }
    }
    QString result = cleaned.join("\n").trimmed();
    result.remove(QRegExp("[\\s]*[" + QString(QChar(0x2191)) + QString(QChar(0x21A9)) + QString(QChar(0x2190)) + "]+\\s*$"));
    return result.trimmed();
}

bool findTagEndAndClose(const QString &html, int tagStart, int *outOpenEnd, int *outInnerEnd, int *outBlockEnd, QString *outTagName)
{
    int len = html.length();
    if (tagStart < 0 || tagStart + 1 >= len || html.at(tagStart) != '<') return false;

    int nameStart = tagStart + 1;
    while (nameStart < len && html.at(nameStart).isSpace()) nameStart++;
    int nameEnd = nameStart;
    while (nameEnd < len && !html.at(nameEnd).isSpace() && html.at(nameEnd) != '>' && html.at(nameEnd) != '/') {
        nameEnd++;
    }
    if (nameEnd <= nameStart) return false;
    QString tagName = html.mid(nameStart, nameEnd - nameStart).toLower();

    // Find end of opening tag respecting quotes
    int openEnd = nameEnd;
    QChar quote;
    while (openEnd < len) {
        QChar c = html.at(openEnd);
        if (!quote.isNull()) {
            if (c == quote) quote = QChar();
        } else {
            if (c == '"' || c == '\'') quote = c;
            else if (c == '>') break;
        }
        openEnd++;
    }
    if (openEnd >= len) return false;

    *outOpenEnd = openEnd;
    *outTagName = tagName;

    // Self-closing tag
    if (openEnd > tagStart && html.at(openEnd - 1) == '/') {
        *outInnerEnd = openEnd + 1;
        *outBlockEnd = openEnd + 1;
        return true;
    }

    QRegExp tagScan("<(/?)" + QRegExp::escape(tagName) + "(?=[\\s/>])", Qt::CaseInsensitive);
    int depth = 1;
    int scanPos = openEnd + 1;
    while ((scanPos = tagScan.indexIn(html, scanPos)) != -1) {
        bool isClosing = !tagScan.cap(1).isEmpty();
        int gt = scanPos + tagScan.matchedLength();
        QChar q;
        while (gt < len) {
            QChar ch = html.at(gt);
            if (!q.isNull()) {
                if (ch == q) q = QChar();
            } else {
                if (ch == '"' || ch == '\'') q = ch;
                else if (ch == '>') break;
            }
            gt++;
        }
        if (gt >= len) break;
        bool isSelfClose = (!isClosing && gt > scanPos && html.at(gt - 1) == '/');
        if (isClosing) {
            depth--;
            if (depth == 0) {
                *outInnerEnd = scanPos;
                *outBlockEnd = gt + 1;
                return true;
            }
        } else if (!isSelfClose) {
            depth++;
        }
        scanPos = gt + 1;
    }
    int fallbackEnd = qMin(len, openEnd + 2000);
    *outInnerEnd = fallbackEnd;
    *outBlockEnd = fallbackEnd;
    return true;
}

int findAnchorTagStart(const QString &html, const QString &anchor)
{
    if (html.isEmpty() || anchor.isEmpty()) return -1;
    QStringList candidates;
    candidates << anchor;
    QString decoded = QString::fromUtf8(QUrl::fromPercentEncoding(anchor.toUtf8()).toUtf8());
    if (!decoded.isEmpty() && decoded != anchor) candidates << decoded;

    QRegExp attrRx("\\b(?:id|name)\\s*=\\s*([\"'])", Qt::CaseInsensitive);
    int pos = 0;
    while ((pos = attrRx.indexIn(html, pos)) != -1) {
        QChar quote = attrRx.cap(1).at(0);
        int valStart = pos + attrRx.matchedLength();
        int valEnd = html.indexOf(quote, valStart);
        if (valEnd == -1) break;
        QString val = html.mid(valStart, valEnd - valStart);
        bool match = false;
        foreach (const QString &cand, candidates) {
            if (val == cand) {
                match = true;
                break;
            }
        }
        if (match) {
            // Confirm we are inside an HTML opening tag (<...>)
            int lt = html.lastIndexOf('<', pos);
            int gt = html.lastIndexOf('>', pos);
            if (lt != -1 && lt > gt && lt + 1 < html.length() && html.at(lt + 1) != '/') {
                return lt;
            }
        }
        pos = valEnd + 1;
    }
    return -1;
}

QString extractFootnoteTextFromHtml(const QString &html, const QString &anchor, bool *outHasNoteMarkup, QString *outTagName)
{
    if (outHasNoteMarkup) *outHasNoteMarkup = false;
    if (outTagName) outTagName->clear();

    int tagStart = findAnchorTagStart(html, anchor);
    if (tagStart < 0) return QString();

    int openEnd = -1, innerEnd = -1, blockEnd = -1;
    QString tagName;
    if (!findTagEndAndClose(html, tagStart, &openEnd, &innerEnd, &blockEnd, &tagName)) return QString();

    QString openTag = html.mid(tagStart, openEnd - tagStart + 1).toLower();
    bool hasNoteMarkup = (tagName == "aside" || openTag.contains("footnote") || openTag.contains("endnote") || openTag.contains("rearnote") || openTag.contains("doc-footnote"));
    if (tagName.startsWith('h') && tagName.length() == 2 && tagName.at(1).isDigit()) {
        if (outTagName) *outTagName = tagName;
        return QString();
    }

    QString innerHtml = (innerEnd > openEnd + 1) ? html.mid(openEnd + 1, innerEnd - (openEnd + 1)) : QString();
    QString plain = htmlFragmentToPlainNote(innerHtml);

    // If anchor is an inline <a>, <span>, <sup>, or empty marker, expand to enclosing note paragraph/item
    if (tagName == "a" || tagName == "span" || tagName == "sup" || plain.length() <= 4) {
        QRegExp blkRx("<(aside|li|dd|p|blockquote|td)\\b[^>]*>", Qt::CaseInsensitive);
        int searchFrom = qMax(0, tagStart - 2500);
        int scan = searchFrom;
        int bestStart = -1, bestOpenEnd = -1, bestInnerEnd = -1, bestBlockEnd = -1;
        QString bestTag;
        while ((scan = blkRx.indexIn(html, scan)) != -1 && scan < tagStart) {
            int oEnd = -1, iEnd = -1, bEnd = -1;
            QString bTag;
            if (findTagEndAndClose(html, scan, &oEnd, &iEnd, &bEnd, &bTag) && bEnd > tagStart) {
                bestStart = scan;
                bestOpenEnd = oEnd;
                bestInnerEnd = iEnd;
                bestBlockEnd = bEnd;
                bestTag = bTag;
            }
            scan += blkRx.matchedLength();
        }
        if (bestStart != -1 && (bestBlockEnd - bestOpenEnd) < 4000) {
            QString blkOpen = html.mid(bestStart, bestOpenEnd - bestStart + 1).toLower();
            if (bestTag == "aside" || blkOpen.contains("footnote") || blkOpen.contains("endnote") || blkOpen.contains("rearnote")) {
                hasNoteMarkup = true;
            }
            QString blkInner = (bestInnerEnd > bestOpenEnd + 1) ? html.mid(bestOpenEnd + 1, bestInnerEnd - (bestOpenEnd + 1)) : QString();
            QString blkPlain = htmlFragmentToPlainNote(blkInner);
            if (blkPlain.length() > plain.length()) {
                plain = blkPlain;
                tagName = bestTag;
            }
        } else if (plain.length() <= 4) {
            // Bare sibling footnote line inside a multi-note div (stops before next footnote anchor or <hr>)
            QString slice = html.mid(blockEnd, 1800);
            QRegExp nextAnchorRx("<(?:[A-Za-z]+)\\b[^>]*\\b(?:id|name)\\s*=\\s*[\"'][^\"']*(?:fn|ftn|edn|note)[^\"']*[\"']|<hr\\b|</div|</section|</body", Qt::CaseInsensitive);
            int stopAt = nextAnchorRx.indexIn(slice, 1);
            if (stopAt > 0) slice = slice.left(stopAt);
            QString sibPlain = htmlFragmentToPlainNote(slice);
            if (!sibPlain.isEmpty()) {
                plain = sibPlain;
            }
        }
    }

    if (outHasNoteMarkup) *outHasNoteMarkup = hasNoteMarkup;
    if (outTagName) *outTagName = tagName;
    return plain;
}

bool looksLikeFootnoteRef(const QString &refTitle, const QString &anchor, const QString &href, bool targetHasNoteMarkup)
{
    if (targetHasNoteMarkup) return true;
    QString t = refTitle.trimmed();
    QRegExp shortSymRe("^[\\[\\(\\{]?\\s*(?:\\*+|[0-9]{1,3}|[a-z]|[ivxlcdm]+)\\s*[\\]\\)\\}]?$", Qt::CaseInsensitive);
    if (!t.isEmpty() && (t == "*" || shortSymRe.exactMatch(t))) return true;
    QString lAnc = anchor.toLower();
    if (lAnc.startsWith("fn") || lAnc.contains("ftn") || lAnc.contains("edn") || lAnc.contains("note") || lAnc.contains("chuthich") || lAnc.contains("comment")) {
        return true;
    }
    QString lHref = href.toLower();
    if (lHref.contains("footnote") || lHref.contains("endnote") || lHref.contains("notes.") || lHref.contains("chu_thich") || lHref.contains("chuthich")) {
        return true;
    }
    return false;
}

} // namespace

void BookManager::openInternalLink(const QString &rawHref, const QString &refTitle)
{
    QString href = rawHref.trimmed();
    if (href.isEmpty()) return;
    if (href.startsWith("http://", Qt::CaseInsensitive) ||
        href.startsWith("https://", Qt::CaseInsensitive) ||
        href.startsWith("mailto:", Qt::CaseInsensitive)) {
        return;
    }

    // Normalize URLs possibly forwarded from WebView.navigationRequested
    if (href.startsWith("about:blank", Qt::CaseInsensitive)) {
        href = href.mid(11);
    } else if (href.startsWith("local:///app/native/", Qt::CaseInsensitive)) {
        href = href.mid(20);
    } else if (href.startsWith("local:///", Qt::CaseInsensitive) || href.startsWith("file://", Qt::CaseInsensitive)) {
        QUrl u(href);
        QString p = u.path();
        while (p.startsWith('/')) p.remove(0, 1);
        href = p + (u.hasFragment() ? ("#" + u.fragment()) : QString());
    }

    int hashIdx = href.indexOf('#');
    QString filePart = (hashIdx != -1) ? href.left(hashIdx) : href;
    QString anchor = (hashIdx != -1) ? href.mid(hashIdx + 1) : QString();
    int qIdx = filePart.indexOf('?');
    if (qIdx != -1) filePart = filePart.left(qIdx);

    filePart = QString::fromUtf8(QUrl::fromPercentEncoding(filePart.toUtf8()).toUtf8());
    filePart.replace('\\', '/');
    anchor = anchor.trimmed();

    QString popupTitle = !refTitle.trimmed().isEmpty() ? refTitle.trimmed() : tr("Chú thích");

    if (m_currentFormat == "epub") {
        QList<EpubChapter> chs = m_epubParser.chapters();
        QString curZipHref = (m_currentChapterIndex >= 0 && m_currentChapterIndex < chs.size()) ? chs[m_currentChapterIndex].href : QString();
        QString targetZipPath;

        if (filePart.isEmpty() || filePart == ".") {
            targetZipPath = curZipHref;
        } else if (filePart.startsWith('/')) {
            targetZipPath = filePart.mid(1);
        } else {
            QString curDir = QFileInfo(curZipHref).path();
            if (curDir == ".") curDir.clear();
            targetZipPath = QDir::cleanPath(curDir.isEmpty() ? filePart : (curDir + "/" + filePart));
            while (targetZipPath.startsWith("./")) targetZipPath.remove(0, 2);
            while (targetZipPath.startsWith('/')) targetZipPath.remove(0, 1);
        }

        int targetChapIdx = -1;
        if (filePart.isEmpty() || filePart == ".") {
            targetChapIdx = m_currentChapterIndex;
        } else {
            QString opfRelPath = m_epubParser.opfDir() + filePart;
            while (opfRelPath.startsWith('/')) opfRelPath.remove(0, 1);
            QString baseName = QFileInfo(targetZipPath).fileName();

            for (int i = 0; i < chs.size(); ++i) {
                if (chs[i].href == targetZipPath || chs[i].href == opfRelPath) {
                    targetChapIdx = i;
                    break;
                }
            }
            if (targetChapIdx < 0) {
                for (int i = 0; i < chs.size(); ++i) {
                    if (chs[i].href.compare(targetZipPath, Qt::CaseInsensitive) == 0 ||
                        chs[i].href.compare(opfRelPath, Qt::CaseInsensitive) == 0) {
                        targetChapIdx = i;
                        break;
                    }
                }
            }
            if (targetChapIdx < 0 && !baseName.isEmpty()) {
                for (int i = 0; i < chs.size(); ++i) {
                    if (QFileInfo(chs[i].href).fileName().compare(baseName, Qt::CaseInsensitive) == 0) {
                        targetChapIdx = i;
                        break;
                    }
                }
            }
        }

        QString targetHtml;
        if (targetChapIdx >= 0 && targetChapIdx < chs.size()) {
            targetHtml = m_epubParser.readHtmlByPath(chs[targetChapIdx].href);
        } else if (!targetZipPath.isEmpty()) {
            targetHtml = m_epubParser.readHtmlByPath(targetZipPath);
        }

        if (!anchor.isEmpty() && !targetHtml.isEmpty()) {
            bool targetHasNote = false;
            QString targetTag;
            QString noteText = extractFootnoteTextFromHtml(targetHtml, anchor, &targetHasNote, &targetTag);
            if (!noteText.isEmpty() && noteText.length() < 1500 &&
                looksLikeFootnoteRef(refTitle, anchor, href, targetHasNote)) {
                emit showFootnotePopup(popupTitle, noteText);
                return;
            }
        }

        // Fallback: if anchor wasn't found at resolved filePart, scan any note-like EPUB chapters
        if (!anchor.isEmpty() && (targetHtml.isEmpty() || findAnchorTagStart(targetHtml, anchor) < 0)) {
            for (int i = 0; i < chs.size(); ++i) {
                QString chHref = chs[i].href.toLower();
                if (chHref.contains("foot") || chHref.contains("note") || chHref.contains("end") || chHref.contains("thich")) {
                    QString h = m_epubParser.readHtmlByPath(chs[i].href);
                    if (findAnchorTagStart(h, anchor) >= 0) {
                        targetChapIdx = i;
                        targetHtml = h;
                        bool hasNote = false;
                        QString tag;
                        QString noteText = extractFootnoteTextFromHtml(targetHtml, anchor, &hasNote, &tag);
                        if (!noteText.isEmpty() && noteText.length() < 1500 && looksLikeFootnoteRef(refTitle, anchor, href, hasNote)) {
                            emit showFootnotePopup(popupTitle, noteText);
                            return;
                        }
                        break;
                    }
                }
            }
        }

        if (targetChapIdx >= 0) {
            jumpToToc(targetChapIdx, 0, anchor);
            return;
        }
        qWarning() << "[BookManager] Could not resolve internal EPUB link:" << rawHref;
        return;
    }

    if (m_currentFormat == "mobi") {
        int count = m_mobiParser.chapterCount();
        int targetChapIdx = -1;
        QString targetHtml;
        if (!anchor.isEmpty()) {
            // Check current chapter first, then scan all chunks (footnotes are often at the end)
            if (m_currentChapterIndex >= 0 && m_currentChapterIndex < count) {
                QString h = m_mobiParser.rawChapterHtml(m_currentChapterIndex);
                if (findAnchorTagStart(h, anchor) >= 0) {
                    targetChapIdx = m_currentChapterIndex;
                    targetHtml = h;
                }
            }
            if (targetChapIdx < 0) {
                for (int i = count - 1; i >= 0; --i) {
                    QString h = m_mobiParser.rawChapterHtml(i);
                    if (findAnchorTagStart(h, anchor) >= 0) {
                        targetChapIdx = i;
                        targetHtml = h;
                        break;
                    }
                }
            }
            if (targetChapIdx >= 0) {
                bool hasNote = false;
                QString tag;
                QString noteText = extractFootnoteTextFromHtml(targetHtml, anchor, &hasNote, &tag);
                if (!noteText.isEmpty() && noteText.length() < 1500 &&
                    looksLikeFootnoteRef(refTitle, anchor, href, hasNote)) {
                    emit showFootnotePopup(popupTitle, noteText);
                    return;
                }
                jumpToToc(targetChapIdx, 0, anchor);
                return;
            }
        }
        qWarning() << "[BookManager] Could not resolve internal MOBI link:" << rawHref;
        return;
    }

    if (m_currentFormat == "fb2") {
        if (!anchor.isEmpty()) {
            QString notesHtml = m_fb2Parser.notesHtml();
            if (!notesHtml.isEmpty() && findAnchorTagStart(notesHtml, anchor) >= 0) {
                bool hasNote = false;
                QString tag;
                QString noteText = extractFootnoteTextFromHtml(notesHtml, anchor, &hasNote, &tag);
                if (!noteText.isEmpty()) {
                    emit showFootnotePopup(popupTitle, noteText);
                    return;
                }
            }
            int count = m_fb2Parser.chapterCount();
            for (int i = 0; i < count; ++i) {
                QString h = m_fb2Parser.getChapterContent(i);
                if (findAnchorTagStart(h, anchor) >= 0) {
                    bool hasNote = false;
                    QString tag;
                    QString noteText = extractFootnoteTextFromHtml(h, anchor, &hasNote, &tag);
                    if (!noteText.isEmpty() && noteText.length() < 1500 &&
                        looksLikeFootnoteRef(refTitle, anchor, href, hasNote)) {
                        emit showFootnotePopup(popupTitle, noteText);
                        return;
                    }
                    jumpToToc(i, 0, anchor);
                    return;
                }
            }
        }
        qWarning() << "[BookManager] Could not resolve internal FB2 link:" << rawHref;
        return;
    }
}

void BookManager::searchBook(const QString &query, const QString &scope)
{
    QString q = query.trimmed();
    retireWorkerThread(m_searchWorker, this, false);

    m_searchResults.clear();
    if (q.isEmpty()) {
        m_isSearching = false;
        emit isSearchingChanged();
        emit searchResultsChanged();
        return;
    }

    m_isSearching = true;
    emit isSearchingChanged();

    SearchParams params;
    params.query = q;
    params.scope = scope.isEmpty() ? "full" : scope;
    params.format = m_currentFormat;
    params.filePath = m_currentFilePath;
    params.currentPdfText = m_currentPdfText;
    params.currentChapterIndex = m_currentChapterIndex;
    params.maxTotalResults = 150;

    m_searchWorker = new SearchWorker(params, this);
    connect(m_searchWorker, SIGNAL(searchCompleted(QVariantList)), this, SLOT(onSearchCompleted(QVariantList)));
    m_searchWorker->start();
}

void BookManager::onSearchCompleted(const QVariantList &results)
{
    m_searchResults = results;
    m_isSearching = false;
    emit isSearchingChanged();
    emit searchResultsChanged();
}

void BookManager::cancelSearch()
{
    retireWorkerThread(m_searchWorker, this, false);
    m_isSearching = false;
    m_searchResults.clear();
    emit isSearchingChanged();
    emit searchResultsChanged();
}

void BookManager::refreshPdfPageText(int pageNumber)
{
    if (!isPdf() || !m_pdfParser) return;
    int pg = (pageNumber >= 1) ? pageNumber : m_currentChapterIndex;
    if (pg < 1) pg = 1;
    QString newText = m_pdfParser->pageText(pg);
    bool newHasText = !newText.trimmed().isEmpty();
    if (newHasText) {
        m_pdfHasAnyOcr = true;
    }
    bool textChanged = (m_currentPdfText != newText);
    bool flagChanged = (m_hasPdfText != newHasText);
    m_currentPdfText = newText;
    m_hasPdfText = newHasText;
    if (textChanged) emit currentPdfTextChanged();
    if (flagChanged) emit hasPdfTextChanged();
}

QString BookManager::getPdfPageText(int pageNumber)
{
    if (!isPdf() || !m_pdfParser) return QString();
    int pg = (pageNumber >= 1) ? pageNumber : m_currentChapterIndex;
    if (pg < 1) pg = 1;
    QString raw = m_pdfParser->pageText(pg);
    if (pg == m_currentChapterIndex) {
        bool newHasText = !raw.trimmed().isEmpty();
        if (newHasText) m_pdfHasAnyOcr = true;
        if (m_currentPdfText != raw) {
            m_currentPdfText = raw;
            emit currentPdfTextChanged();
        }
        if (m_hasPdfText != newHasText) {
            m_hasPdfText = newHasText;
            emit hasPdfTextChanged();
        }
    }
    raw.remove(QChar(0x0000));
    raw.remove(QChar(0xFEFF));
    raw.remove(QChar(0xFFFD));
    raw.replace('\r', "");
    QRegExp hyphenWrap("([\\w\\dÀ-ỹ])-\\s*\\n\\s*([\\w\\dÀ-ỹ])");
    raw.replace(hyphenWrap, "\\1\\2");
    return raw.trimmed();
}

void BookManager::setIsPdfReflow(bool reflow)
{
    if (!isPdf()) return;
    if (m_isPdfReflow != reflow) {
        m_isPdfReflow = reflow;
        m_cachedContentHtml.clear();
        if (m_isPdfReflow) {
            refreshPdfPageText(m_currentChapterIndex);
            emit isPdfReflowChanged();
            updatePdfReflowContent();
        } else {
            m_currentPageInChapter = m_currentChapterIndex;
            m_totalPagesInChapter = m_totalChapters;
            emit isPdfReflowChanged();
            emit readingProgressChanged();
            emit pagePositionChanged();
            if (!m_pdfZoomMode) {
                emit pdfJumpToPageRequested(m_currentChapterIndex);
                requestPdfStreamPage(m_currentChapterIndex);
            }
        }
    }
}

void BookManager::togglePdfReflow()
{
    if (!isPdf()) return;
    setIsPdfReflow(!m_isPdfReflow);
}

void BookManager::updatePdfReflowContent()
{
    m_cachedContentHtml.clear();
    m_cachedContentChapter = -1;
    emit currentChapterChanged();
}

void BookManager::copyText(const QString &text)
{
    m_dictManager.copyToClipboard(text);
}

bool BookManager::openBook(const QString &filePath)
{
    stopTts();
    m_pendingTtsAfterPdfRender = false;
    m_pendingJumpPageAfterPdfOpen = -1;
    setIsLoadingBook(true);
    // Safety watchdog: always ensure isLoadingBook drops to false after opening
    QTimer::singleShot(800, this, SLOT(resetLoadingState()));

    QFileInfo fi(filePath);
    if (!fi.exists() || fi.isDir()) {
        qWarning() << "[BookManager] File not found or is a directory:" << filePath;
        setIsLoadingBook(false);
        return false;
    }

    QSettings settings("Libris", "Libris");
    settings.setValue(QString("opened/%1").arg(filePath), true);

    m_currentFilePath = filePath;
    QString ext = fi.suffix().toLower();

    qint64 sz = fi.size();
    if (sz >= 1024 * 1024) {
        m_currentBookFileSize = QString("%1 MB").arg(QString::number(sz / (1024.0 * 1024.0), 'f', 1));
    } else {
        m_currentBookFileSize = QString("%1 KB").arg(sz / 1024);
    }
    m_currentBookDateModified = fi.lastModified().toString("dd/MM/yyyy");
    m_currentBookDescription.clear();
    m_currentBookGenre.clear();
    m_isPdfReflow = false;
    m_hasPdfText = false;
    m_pdfHasAnyOcr = false;
    m_currentPdfText.clear();
    m_searchResults.clear();
    emit isPdfReflowChanged();
    emit hasPdfTextChanged();
    emit currentPdfTextChanged();
    emit searchResultsChanged();

    m_cachedContentHtml.clear();
    m_cachedContentChapter = -1;
    m_tocList.clear();

    // Close any previous documents
    m_epubParser.close();
    m_txtParser.close();
    m_fb2Parser.close();
    m_cbzParser.close();
    m_mobiParser.close();
    if (m_pdfParser) {
        m_pdfParser->close();
    }
    if (m_pdfPagesModel) {
        m_pdfPagesModel->clear();
    }
    if (m_pdfZoomMode) {
        m_pdfZoomMode = false;
        emit pdfZoomModeChanged();
    }

    if (!m_bookReadingSeconds.contains(filePath)) {
        m_bookReadingSeconds[filePath] = settings.value(QString("stats/bookSeconds/%1").arg(filePath), 0).toInt();
    }
    int savedChapter = settings.value(QString("progress/%1").arg(filePath), 0).toInt();
    int savedPage = settings.value(QString("pageInChapter/%1").arg(filePath), 1).toInt();
    int savedScroll = settings.value(QString("scrollPos/%1").arg(filePath), 0).toInt();

    if (ext == "epub") {
        m_currentFormat = "epub";
        m_cachedContentHtml.clear();
        if (!m_epubParser.open(filePath)) {
            qWarning() << "[BookManager] Failed to parse EPUB:" << filePath;
            setIsLoadingBook(false);
            return false;
        }
        m_currentBookTitle = m_epubParser.title();
        m_currentBookAuthor = m_epubParser.author();
        m_currentBookSeries = m_epubParser.series();
        m_currentBookSeriesIndex = m_epubParser.seriesIndex();
        if (m_currentBookSeries.isEmpty()) {
            extractSeriesFromTitleOrFile(m_currentBookTitle, fi.fileName(), m_currentBookSeries, m_currentBookSeriesIndex);
        }
        m_currentBookDescription = m_epubParser.description();
        m_currentBookGenre = m_epubParser.genre();

        QString coverHash = QString(QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex());
        m_currentCoverPath = findCachedCoverFile(m_cacheDir, coverHash);
        if (m_currentCoverPath.isEmpty()) {
            m_currentCoverPath = m_epubParser.extractCover(m_cacheDir);
        }
        if (!m_currentCoverPath.isEmpty()) {
            updateBookCoverInLibrary(filePath, m_currentCoverPath);
        }
        m_totalChapters = m_epubParser.chapterCount();
        m_currentChapterIndex = (savedChapter >= 0 && savedChapter < m_totalChapters) ? savedChapter : 0;
        m_currentPageInChapter = (savedPage >= 1) ? savedPage : 1;
        m_currentScrollPos = (savedScroll >= 0) ? savedScroll : 0;
        m_shouldRestorePageOnLoad = true;
        emit currentBookChanged();
        updateTocList();
        loadCurrentChapter();
        ensureBookInLibrary(filePath);
        return true;
    } else if (ext == "txt" || ext == "md" || ext == "markdown" || ext == "html" || ext == "htm") {
        m_currentFormat = "txt";
        if (!m_txtParser.open(filePath)) {
            qWarning() << "[BookManager] Failed to parse TXT:" << filePath;
            setIsLoadingBook(false);
            return false;
        }
        m_currentBookTitle = m_txtParser.title();
        m_currentBookAuthor = m_txtParser.author();
        m_currentBookSeries.clear();
        m_currentBookSeriesIndex = 0;
        extractSeriesFromTitleOrFile(m_currentBookTitle, fi.fileName(), m_currentBookSeries, m_currentBookSeriesIndex);
        m_currentBookGenre = tr("Văn bản TXT");
        m_currentCoverPath = m_txtParser.coverPath();
        m_totalChapters = m_txtParser.chapterCount();
        m_currentChapterIndex = (savedChapter >= 0 && savedChapter < m_totalChapters) ? savedChapter : 0;
        m_currentPageInChapter = (savedPage >= 1) ? savedPage : 1;
        m_currentScrollPos = (savedScroll >= 0) ? savedScroll : 0;
        m_shouldRestorePageOnLoad = true;
        emit currentBookChanged();
        updateTocList();
        loadCurrentChapter();
        ensureBookInLibrary(filePath);
        return true;
    } else if (ext == "fb2") {
        m_currentFormat = "fb2";
        if (!m_fb2Parser.open(filePath)) {
            qWarning() << "[BookManager] Failed to parse FB2:" << filePath;
            setIsLoadingBook(false);
            return false;
        }
        m_fb2Parser.extractCover(m_cacheDir);
        m_currentBookTitle = m_fb2Parser.title();
        m_currentBookAuthor = m_fb2Parser.author();
        m_currentBookSeries = m_fb2Parser.series();
        m_currentBookSeriesIndex = m_fb2Parser.seriesIndex();
        if (m_currentBookSeries.isEmpty()) {
            extractSeriesFromTitleOrFile(m_currentBookTitle, fi.fileName(), m_currentBookSeries, m_currentBookSeriesIndex);
        }
        m_currentBookDescription = m_fb2Parser.description();
        m_currentBookGenre = m_fb2Parser.genre();
        m_currentCoverPath = m_fb2Parser.coverPath();
        if (!m_currentCoverPath.isEmpty()) {
            updateBookCoverInLibrary(filePath, m_currentCoverPath);
        }
        m_totalChapters = m_fb2Parser.chapterCount();
        m_currentChapterIndex = (savedChapter >= 0 && savedChapter < m_totalChapters) ? savedChapter : 0;
        m_currentPageInChapter = (savedPage >= 1) ? savedPage : 1;
        m_currentScrollPos = (savedScroll >= 0) ? savedScroll : 0;
        m_shouldRestorePageOnLoad = true;
        emit currentBookChanged();
        updateTocList();
        loadCurrentChapter();
        ensureBookInLibrary(filePath);
        return true;
    } else if (ext == "cbz") {
        m_currentFormat = "cbz";
        m_cachedContentHtml.clear();
        if (!m_cbzParser.open(filePath)) {
            qWarning() << "[BookManager] Failed to parse CBZ:" << filePath;
            setIsLoadingBook(false);
            return false;
        }
        m_currentBookTitle = m_cbzParser.title();
        m_currentBookAuthor = m_cbzParser.author();
        m_currentBookSeries = m_cbzParser.series();
        m_currentBookSeriesIndex = m_cbzParser.seriesIndex();
        if (m_currentBookSeries.isEmpty()) {
            extractSeriesFromTitleOrFile(m_currentBookTitle, fi.fileName(), m_currentBookSeries, m_currentBookSeriesIndex);
        }
        m_currentBookDescription = m_cbzParser.description();
        m_currentBookGenre = m_cbzParser.genre();

        QString coverHash = QString(QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex());
        m_currentCoverPath = findCachedCoverFile(m_cacheDir, coverHash);
        if (m_currentCoverPath.isEmpty()) {
            m_currentCoverPath = m_cbzParser.extractCover(m_cacheDir);
        }
        if (!m_currentCoverPath.isEmpty()) {
            updateBookCoverInLibrary(filePath, m_currentCoverPath);
        }
        m_totalChapters = m_cbzParser.chapterCount();
        m_currentChapterIndex = (savedChapter >= 0 && savedChapter < m_totalChapters) ? savedChapter : 0;
        m_currentPageInChapter = (savedPage >= 1) ? savedPage : 1;
        m_currentScrollPos = (savedScroll >= 0) ? savedScroll : 0;
        m_shouldRestorePageOnLoad = true;
        emit currentBookChanged();
        updateTocList();
        loadCurrentChapter();
        ensureBookInLibrary(filePath);
        return true;
    } else if (ext == "mobi" || ext == "azw" || ext == "azw3" || ext == "kf8" || ext == "prc") {
        m_currentFormat = "mobi";
        m_cachedContentHtml.clear();
        if (!m_mobiParser.open(filePath)) {
            qWarning() << "[BookManager] Failed to parse MOBI/AZW3:" << filePath;
            setIsLoadingBook(false);
            return false;
        }
        m_currentBookTitle = m_mobiParser.title();
        m_currentBookAuthor = m_mobiParser.author();
        m_currentBookSeries = m_mobiParser.series();
        m_currentBookSeriesIndex = m_mobiParser.seriesIndex();
        if (m_currentBookSeries.isEmpty()) {
            extractSeriesFromTitleOrFile(m_currentBookTitle, fi.fileName(), m_currentBookSeries, m_currentBookSeriesIndex);
        }
        m_currentBookDescription = m_mobiParser.description();
        m_currentBookGenre = m_mobiParser.genre();

        QString coverHash = QString(QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex());
        m_currentCoverPath = findCachedCoverFile(m_cacheDir, coverHash);
        if (m_currentCoverPath.isEmpty()) {
            m_currentCoverPath = m_mobiParser.extractCover(m_cacheDir);
        }
        if (!m_currentCoverPath.isEmpty()) {
            updateBookCoverInLibrary(filePath, m_currentCoverPath);
        }
        m_totalChapters = m_mobiParser.chapterCount();
        m_currentChapterIndex = (savedChapter >= 0 && savedChapter < m_totalChapters) ? savedChapter : 0;
        m_currentPageInChapter = (savedPage >= 1) ? savedPage : 1;
        m_currentScrollPos = (savedScroll >= 0) ? savedScroll : 0;
        m_shouldRestorePageOnLoad = true;
        emit currentBookChanged();
        updateTocList();
        loadCurrentChapter();
        ensureBookInLibrary(filePath);
        return true;
    } else if (ext == "pdf" || ext == "djvu" || ext == "djv") {
        m_currentFormat = (ext == "djv") ? "djvu" : ext;
        m_currentBookTitle = fi.completeBaseName().replace('_', ' ');
        m_currentBookAuthor = (ext == "pdf") ? "Tài liệu PDF" : "Tài liệu DjVu";
        m_currentBookSeries.clear();
        m_currentBookSeriesIndex = 0;
        extractSeriesFromTitleOrFile(m_currentBookTitle, fi.fileName(), m_currentBookSeries, m_currentBookSeriesIndex);
        m_currentBookGenre = (ext == "pdf") ? tr("Tài liệu PDF") : tr("Tài liệu DjVu");

        QString hash = QString(QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex());
        QString coverCandidatePng = m_cacheDir + "/" + hash + ".png";
        QString coverCandidateJpg = m_cacheDir + "/" + hash + ".jpg";
        if (QFile::exists(coverCandidatePng)) {
            m_currentCoverPath = coverCandidatePng;
        } else if (QFile::exists(coverCandidateJpg)) {
            m_currentCoverPath = coverCandidateJpg;
        } else {
            m_currentCoverPath.clear();
        }

        int targetPage = (savedChapter >= 1) ? savedChapter : 1;
        m_currentChapterIndex = targetPage;
        m_currentPageInChapter = targetPage;
        m_currentScrollPos = 0;
        m_shouldRestorePageOnLoad = false;
        m_totalChapters = 1;

        if (m_pdfParser) {
            if (!m_pdfParser->open(filePath, targetPage)) {
                setIsLoadingBook(false);
                emit pdfOpenError(tr("Không thể mở tệp PDF/DjVu."));
                return false;
            }
            if (!m_pdfParser->title().trimmed().isEmpty()) {
                m_currentBookTitle = m_pdfParser->title().trimmed();
            }
            if (!m_pdfParser->author().trimmed().isEmpty()) {
                m_currentBookAuthor = m_pdfParser->author().trimmed();
            }

            if (m_theme == "dark") {
                if (!m_pdfParser->isInvertedColors()) {
                    m_pdfParser->switchInvertedColors();
                }
            } else {
                if (m_pdfParser->isInvertedColors()) {
                    m_pdfParser->switchInvertedColors();
                }
            }
            m_pdfInverted = m_pdfParser->isInvertedColors();
        }

        emit currentBookChanged();
        emit pdfInvertedChanged();
        ensureBookInLibrary(filePath);
        return true;
    }

    return false;
}

void BookManager::onPdfFileOpened(const QString &file)
{
    Q_UNUSED(file);
    if (m_pdfParser) {
        m_totalChapters = m_pdfParser->chapterCount();
        m_totalPagesInChapter = m_totalChapters;
        QSettings settings("Libris", "Libris");
        settings.setValue(QString("totalChapters/%1").arg(m_currentFilePath), m_totalChapters);
        settings.setValue(QString("totalPages/%1").arg(m_currentFilePath), m_totalChapters);
        updateTocList();
        ensureBookInLibrary(m_currentFilePath);
        m_pdfHasAnyOcr = m_pdfParser->hasEmbeddedOcr(15);
        refreshPdfPageText(m_currentChapterIndex);
        if (m_totalChapters > 0 && m_pdfPagesModel && m_pdfPagesModel->size() != m_totalChapters) {
            rebuildPdfPagesModel(m_totalChapters);
            emit pdfJumpToPageRequested(m_currentChapterIndex);
            requestPdfStreamPage(m_currentChapterIndex);
        }
        emit currentBookChanged();
        emit readingProgressChanged();
        if (m_pendingJumpPageAfterPdfOpen > 0) {
            int pg = qBound(1, m_pendingJumpPageAfterPdfOpen, qMax(1, m_totalChapters));
            m_pendingJumpPageAfterPdfOpen = -1;
            if (pg != m_currentChapterIndex) {
                m_pdfParser->navigateToPage(pg);
            }
        }
    }
}

void BookManager::ensureBookInLibrary(const QString &filePath)
{
    if (filePath.isEmpty()) return;

    QSettings settings("Libris", "Libris");
    settings.setValue(QString("lastReadTime/%1").arg(filePath), (qlonglong)QDateTime::currentDateTime().toMSecsSinceEpoch());
    if (!m_currentBookTitle.isEmpty()) {
        settings.setValue(QString("meta/title/%1").arg(filePath), m_currentBookTitle);
    }
    if (!m_currentBookAuthor.isEmpty()) {
        settings.setValue(QString("meta/author/%1").arg(filePath), m_currentBookAuthor);
    }

    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            m_allBooks[i]["isOpened"] = true;
            if (!m_currentCoverPath.isEmpty() && m_allBooks[i].value("cover").toString().isEmpty()) {
                m_allBooks[i]["cover"] = m_currentCoverPath;
            }
            if (!m_currentBookTitle.isEmpty()) {
                m_allBooks[i]["title"] = m_currentBookTitle;
            }
            if (!m_currentBookAuthor.isEmpty()) {
                m_allBooks[i]["author"] = m_currentBookAuthor;
            }
            if (m_totalChapters > 0) {
                m_allBooks[i]["totalPages"] = m_totalChapters;
                m_allBooks[i]["totalChapters"] = m_totalChapters;
            }
            applyFilterAndSort();
            updateReadingHistoryModel();
            saveLibraryCache();
            return;
        }
    }

    QFileInfo fi(filePath);
    QVariantMap bookMap;
    bookMap["title"] = m_currentBookTitle.isEmpty() ? fi.completeBaseName().replace('_', ' ') : m_currentBookTitle;
    bookMap["author"] = m_currentBookAuthor.isEmpty() ? "" : m_currentBookAuthor;
    bookMap["series"] = m_currentBookSeries;
    bookMap["seriesIndex"] = m_currentBookSeriesIndex;
    bookMap["format"] = m_currentFormat;
    bookMap["filePath"] = filePath;
    bookMap["cover"] = m_currentCoverPath;
    bookMap["size"] = m_currentBookFileSize;
    bookMap["date"] = m_currentBookDateModified;
    {
        qint64 szBytes = fi.size();
        QDateTime modTime = fi.lastModified();
        QString fileSizeStr = (szBytes < 1024 * 1024)
            ? QString("%1 KB").arg(qMax((qint64)1, szBytes / 1024))
            : QString("%1 MB").arg(QString::number(szBytes / (1024.0 * 1024.0), 'f', 1));
        bookMap["path"] = filePath;
        bookMap["formatBadge"] = m_currentFormat.toUpper();
        bookMap["fileSize"] = fileSizeStr;
        bookMap["fileSizeBytes"] = szBytes;
        bookMap["fileSizeSort"] = QString("%1").arg(szBytes, 14, 10, QChar('0'));
        bookMap["dateModified"] = modTime.toString("dd/MM/yyyy");
        bookMap["modifiedIso"] = modTime.toString("yyyy-MM-dd hh:mm:ss");
        bookMap["modifiedTimestamp"] = (qint64)modTime.toTime_t();
    }
    int prog = readingProgress();
    bookMap["progress"] = prog;
    bookMap["progressStr"] = QString("%1%").arg(prog);
    bookMap["isCompleted"] = (prog >= 100);
    bookMap["totalPages"] = m_totalChapters;
    bookMap["totalChapters"] = m_totalChapters;

    bookMap["isFavorite"] = settings.value(QString("favorites/%1").arg(filePath), false).toBool();
    bookMap["isReadingList"] = settings.value(QString("readingList/%1").arg(filePath), false).toBool();
    bookMap["isOpened"] = true;

    m_allBooks.append(bookMap);
    applyFilterAndSort();
    updateFolderList();
    refreshBrowser();
    refreshStats();
    saveLibraryCache();
}

void BookManager::updateBookCoverInLibrary(const QString &filePath, const QString &coverPath)
{
    if (filePath.isEmpty() || coverPath.isEmpty()) return;

    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            m_allBooks[i]["cover"] = coverPath;
            break;
        }
    }

    // Update item in m_model in-place to avoid re-sorting library during book launch
    QVariantList indexPath = m_model->first();
    while (!indexPath.isEmpty()) {
        QVariantMap item = m_model->data(indexPath).toMap();
        if (item.value("filePath").toString() == filePath) {
            item["cover"] = coverPath;
            m_model->updateItem(indexPath, item);
            break;
        }
        indexPath = m_model->after(indexPath);
    }
}

void BookManager::updateBookProgressInLibrary(const QString &filePath, int progress)
{
    if (filePath.isEmpty()) return;

    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            m_allBooks[i]["progress"] = progress;
            m_allBooks[i]["progressStr"] = QString("%1%").arg(progress);
            m_allBooks[i]["isCompleted"] = (progress >= 100);
            break;
        }
    }

    if (m_model) {
        QVariantList indexPath = m_model->first();
        while (!indexPath.isEmpty()) {
            QVariantMap item = m_model->data(indexPath).toMap();
            if (item.value("filePath").toString() == filePath) {
                item["progress"] = progress;
                item["progressStr"] = QString("%1%").arg(progress);
                item["isCompleted"] = (progress >= 100);
                m_model->updateItem(indexPath, item);
                break;
            }
            indexPath = m_model->after(indexPath);
        }
    }
}

void BookManager::savePdfCoverThumbnail(const QString &filePath, const bb::ImageData &imgData)
{
    if (imgData.width() <= 0 || imgData.height() <= 0) return;

    QString hash = QString(QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex());
    QString outPath = m_cacheDir + "/" + hash + ".png";

    if (QFile::exists(outPath)) {
        m_currentCoverPath = outPath;
        updateBookCoverInLibrary(filePath, outPath);
        if (m_lastBookPath == filePath && m_lastBookCover != outPath) {
            m_lastBookCover = outPath;
            QSettings settings("Libris", "Libris");
            settings.setValue("lastBook/cover", outPath);
            emit lastBookChanged();
        }
        return;
    }

    QImage qimg = PdfParser::imageDataToQImage(imgData, m_pdfInverted);
    if (qimg.isNull()) return;

    QImage thumbnail = qimg.scaled(240, 320, Qt::KeepAspectRatio, Qt::FastTransformation);
    if (thumbnail.save(outPath, "PNG")) {
        m_currentCoverPath = outPath;
        updateBookCoverInLibrary(filePath, outPath);
        if (m_lastBookPath == filePath) {
            m_lastBookCover = outPath;
            QSettings settings("Libris", "Libris");
            settings.setValue("lastBook/cover", outPath);
            emit lastBookChanged();
        }
        emit currentBookChanged();
    }
}

void BookManager::onPdfPageRendered(int pageNumber)
{
    if (!m_pdfParser) return;

    bb::ImageData imgData = m_pdfParser->currentImageData();

    if (imgData.width() > 0 && imgData.height() > 0) {
        m_currentPdfImage = bb::cascades::Image(imgData);

        // Fast thumbnail save only once on page 1 if cover does not already exist
        if (pageNumber == 1 && !m_currentFilePath.isEmpty()) {
            QString outPath = m_cacheDir + "/" + QCryptographicHash::hash(m_currentFilePath.toUtf8(), QCryptographicHash::Md5).toHex() + ".png";
            if (!QFile::exists(outPath)) {
                savePdfCoverThumbnail(m_currentFilePath, imgData);
            }
        }
    }

    m_currentChapterIndex = pageNumber;
    m_totalChapters = m_pdfParser->chapterCount();
    m_currentPageInChapter = pageNumber;
    m_totalPagesInChapter = m_totalChapters;
    if (m_totalChapters > 0 && m_pdfPagesModel && m_pdfPagesModel->size() != m_totalChapters) {
        rebuildPdfPagesModel(m_totalChapters);
        emit pdfJumpToPageRequested(m_currentChapterIndex);
        requestPdfStreamPage(m_currentChapterIndex);
    }

    // Extract text layer from PDF page (for OCR search, dictionary, and text reflow)
    m_currentPdfText = m_pdfParser->currentText();
    m_hasPdfText = !m_currentPdfText.trimmed().isEmpty();
    emit currentPdfTextChanged();
    emit hasPdfTextChanged();

    if (m_isPdfReflow) {
        updatePdfReflowContent();
    }

    setIsLoadingBook(false);
    emit currentPdfImageChanged();
    emit currentChapterChanged();
    emit readingProgressChanged();
    emit bookmarksChanged();
    emit pagePositionChanged();
    saveSettings();

    if (m_pendingTtsAfterPdfRender) {
        m_pendingTtsAfterPdfRender = false;
        startTts();
    }
}

void BookManager::onPdfIndicatorRequested(bool enabled)
{
    Q_UNUSED(enabled);
}

void BookManager::onPdfError(int error)
{
    qWarning() << "[BookManager] Libris PDF/DjVu engine error:" << error;
    stopWatchdog();
    setIsLoadingBook(false);
    m_pendingTtsAfterPdfRender = false;
    m_pendingJumpPageAfterPdfOpen = -1;
    emit pdfOpenError(tr("Không thể mở hoặc giải mã tệp PDF/DjVu (mã lỗi %1).").arg(error));
}

void BookManager::rotatePdf()
{
    if (isPdf() && m_pdfParser) {
        m_pdfParser->rotate();
        resetPdfPagesModelImages();
        requestPdfStreamPage(m_currentChapterIndex);
    }
}

void BookManager::togglePdfInvert()
{
    if (isPdf() && m_pdfParser) {
        m_pdfParser->switchInvertedColors();
        m_pdfInverted = m_pdfParser->isInvertedColors();
        emit pdfInvertedChanged();
        resetPdfPagesModelImages();
        requestPdfStreamPage(m_currentChapterIndex);
    }
}

bool BookManager::isPdfAutoCrop() const
{
    return m_isPdfAutoCrop;
}

void BookManager::setIsPdfAutoCrop(bool enabled)
{
    if (m_isPdfAutoCrop == enabled) {
        return;
    }
    m_isPdfAutoCrop = enabled;
    if (m_pdfParser) {
        m_pdfParser->setAutoCrop(m_isPdfAutoCrop);
    }
    emit pdfAutoCropChanged();
    saveSettings();
    if (isPdf()) {
        resetPdfPagesModelImages();
        requestPdfStreamPage(m_currentChapterIndex);
    }
}

void BookManager::togglePdfAutoCrop()
{
    setIsPdfAutoCrop(!m_isPdfAutoCrop);
}

void BookManager::zoomPdfIn()
{
    if (isPdf() && m_pdfParser) {
        if (!m_pdfZoomMode) {
            enterPdfZoomMode(m_currentChapterIndex);
        }
        m_pdfParser->zoomIn();
    }
}

void BookManager::zoomPdfOut()
{
    if (isPdf() && m_pdfParser) {
        m_pdfParser->zoomOut();
    }
}

void BookManager::zoomPdfFit()
{
    if (isPdf() && m_pdfParser) {
        m_pdfParser->resetZoom();
        if (m_pdfZoomMode) {
            exitPdfZoomMode();
        }
    }
}

void BookManager::rebuildPdfPagesModel(int totalPages)
{
    if (!m_pdfPagesModel) return;
    m_pdfPagesModel->clear();
    if (totalPages <= 0) return;

    bb::cascades::Image emptyImg;
    QVariant emptyImgVar = QVariant::fromValue(emptyImg);
    for (int i = 1; i <= totalPages; ++i) {
        QVariantMap item;
        item["pageNumber"] = i;
        item["isLoaded"] = false;
        item["aspectRatio"] = 1.414;
        item["image"] = emptyImgVar;
        m_pdfPagesModel->append(item);
    }
}

void BookManager::resetPdfPagesModelImages()
{
    if (!m_pdfPagesModel) return;
    bb::cascades::Image emptyImg;
    QVariant emptyImgVar = QVariant::fromValue(emptyImg);
    int n = m_pdfPagesModel->size();
    for (int i = 0; i < n; ++i) {
        QVariantMap item = m_pdfPagesModel->value(i).toMap();
        if (item.value("isLoaded").toBool()) {
            item["isLoaded"] = false;
            item["image"] = emptyImgVar;
            m_pdfPagesModel->replace(i, item);
        }
    }
}

void BookManager::onPdfStreamPageRendered(int pageNumber)
{
    if (!m_pdfParser || !m_pdfPagesModel) return;
    if (pageNumber < 1 || pageNumber > m_pdfPagesModel->size()) return;

    bb::ImageData imgData = m_pdfParser->streamPageImageData(pageNumber);
    qDebug() << "[BookManager::onPdfStreamPageRendered] pg=" << pageNumber << "size=" << imgData.width() << "x" << imgData.height() << "modelSize=" << m_pdfPagesModel->size();
    if (imgData.width() <= 0 || imgData.height() <= 0) return;

    QVariantMap item = m_pdfPagesModel->value(pageNumber - 1).toMap();
    item["pageNumber"] = pageNumber;
    item["isLoaded"] = true;
    item["aspectRatio"] = (double)imgData.height() / (double)imgData.width();
    bb::cascades::Image cascadesImg(imgData);
    qDebug() << "[BookManager::onPdfStreamPageRendered] cascadesImg isNull=" << cascadesImg.isNull();
    item["image"] = QVariant::fromValue(cascadesImg);
    m_pdfPagesModel->replace(pageNumber - 1, item);
}

void BookManager::requestPdfStreamPage(int pageNumber)
{
    if (!isPdf() || !m_pdfParser || !m_pdfPagesModel || m_totalChapters <= 0) return;
    int pg = qBound(1, pageNumber, m_totalChapters);
    int centerPg = (m_currentChapterIndex >= 1 && m_currentChapterIndex <= m_totalChapters) ? m_currentChapterIndex : pg;

    m_pdfParser->setStreamPriorityPage(centerPg);
    if (qAbs(pg - centerPg) <= 4) {
        m_pdfParser->requestStreamPage(pg, centerPg);
    }
    if (pg + 1 <= m_totalChapters && qAbs((pg + 1) - centerPg) <= 4) {
        m_pdfParser->requestStreamPage(pg + 1, centerPg);
    }
    if (pg - 1 >= 1 && qAbs((pg - 1) - centerPg) <= 4) {
        m_pdfParser->requestStreamPage(pg - 1, centerPg);
    }

    // LRU memory eviction on model items far from current reading position (keep aspectRatio!)
    bb::cascades::Image emptyImg;
    QVariant emptyImgVar = QVariant::fromValue(emptyImg);
    int n = m_pdfPagesModel->size();
    for (int i = 0; i < n; ++i) {
        if (qAbs((i + 1) - centerPg) > 4) {
            QVariantMap item = m_pdfPagesModel->value(i).toMap();
            if (item.value("isLoaded").toBool()) {
                item["isLoaded"] = false;
                item["image"] = emptyImgVar;
                m_pdfPagesModel->replace(i, item);
            }
        }
    }
}

void BookManager::setPdfVisiblePage(int pageNumber)
{
    if (!isPdf() || m_totalChapters <= 0) return;
    int pg = qBound(1, pageNumber, m_totalChapters);
    if (pg != m_currentChapterIndex) {
        m_currentChapterIndex = pg;
        m_currentPageInChapter = pg;
        m_totalPagesInChapter = m_totalChapters;
        refreshPdfPageText(pg);
        emit readingProgressChanged();
        emit pagePositionChanged();
        m_progressSaveTimer.start(1500);
    }
    if (m_pdfParser && m_pdfPagesModel) {
        m_pdfParser->setStreamPriorityPage(pg);
        const int offsets[] = {0, 1, -1, 2};
        for (int k = 0; k < 4; ++k) {
            int t = pg + offsets[k];
            if (t >= 1 && t <= m_pdfPagesModel->size()) {
                if (!m_pdfPagesModel->value(t - 1).toMap().value("isLoaded").toBool()) {
                    m_pdfParser->requestStreamPage(t, pg);
                }
            }
        }
    }
}

void BookManager::setPdfZoomMode(bool enabled)
{
    if (enabled) {
        enterPdfZoomMode(m_currentChapterIndex);
    } else {
        exitPdfZoomMode();
    }
}

void BookManager::releaseReaderMemory()
{
    m_currentPdfImage = bb::cascades::Image();
    emit currentPdfImageChanged();
    resetPdfPagesModelImages();
    if (m_pdfPagesModel) {
        m_pdfPagesModel->clear();
    }
    if (m_pdfParser) {
        m_pdfParser->invalidateStreamCache();
    }
    m_cachedContentHtml.clear();
    m_cachedContentChapter = -1;
}

void BookManager::enterPdfZoomMode(int pageNumber)
{
    if (!isPdf() || !m_pdfParser) return;
    int pg = qBound(1, pageNumber, qMax(1, m_totalChapters));
    m_currentChapterIndex = pg;
    m_currentPageInChapter = pg;
    m_totalPagesInChapter = m_totalChapters;

    if (!m_pdfZoomMode) {
        m_pdfZoomMode = true;
        emit pdfZoomModeChanged();
    }

    bb::ImageData cached = m_pdfParser->streamPageImageData(pg);
    if (cached.width() > 0 && cached.height() > 0) {
        m_currentPdfImage = bb::cascades::Image(cached);
        emit currentPdfImageChanged();
    } else {
        m_pdfParser->navigateToPage(pg);
    }
    emit readingProgressChanged();
    emit pagePositionChanged();
}

void BookManager::exitPdfZoomMode()
{
    if (m_pdfZoomMode) {
        m_pdfZoomMode = false;
        emit pdfZoomModeChanged();
        emit pdfJumpToPageRequested(m_currentChapterIndex);
    }
}

void BookManager::registerBookFile(const QString &filePath)
{
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) return;

    QString absPath = fi.canonicalFilePath();
    if (absPath.isEmpty()) absPath = fi.absoluteFilePath();

    QSettings settings("Libris", "Libris");
    if (settings.value(QString("hidden/%1").arg(absPath), false).toBool()) {
        return;
    }

    QString ext = fi.suffix().toLower();
    QString rawName = fi.completeBaseName().replace('_', ' ');
    QString bookTitle = rawName;
    QString bookAuthor = (ext == "epub") ? "EPUB eBook" :
                         (ext == "txt")  ? "Văn bản TXT" :
                         (ext == "md" || ext == "markdown") ? "Markdown" :
                         (ext == "html" || ext == "htm") ? "Web / HTML" :
                         (ext == "fb2")  ? "FB2 FictionBook" :
                         (ext == "pdf")  ? "Tài liệu PDF" :
                         (ext == "djvu" || ext == "djv") ? "Tài liệu DjVu" :
                         (ext == "cbz")  ? "Truyện tranh CBZ" :
                         (ext == "mobi" || ext == "prc") ? "Sách điện tử MOBI" :
                         (ext == "azw" || ext == "azw3" || ext == "kf8") ? "Sách Kindle AZW3" : "Tài liệu";
    QString bookSeries;
    int bookSeriesIndex = 0;
    QString cover;
    QString formatBadge = ext.toUpper();

    QString hash = QString(QCryptographicHash::hash(absPath.toUtf8(), QCryptographicHash::Md5).toHex());
    cover = findCachedCoverFile(m_cacheDir, hash);

    if (ext == "epub") {
        EpubParser ep;
        if (ep.open(absPath)) {
            if (cover.isEmpty()) cover = ep.extractCover(m_cacheDir);
            if (!ep.title().isEmpty()) { bookTitle = ep.title(); settings.setValue(QString("meta/title/%1").arg(absPath), bookTitle); }
            if (!ep.author().isEmpty() && ep.author() != "Unknown Author") { bookAuthor = ep.author(); settings.setValue(QString("meta/author/%1").arg(absPath), bookAuthor); }
            if (!ep.series().isEmpty()) { bookSeries = ep.series(); bookSeriesIndex = ep.seriesIndex(); }
            ep.close();
        }
    } else if (ext == "fb2") {
        Fb2Parser fb;
        if (fb.open(absPath)) {
            if (cover.isEmpty()) cover = fb.extractCover(m_cacheDir);
            if (!fb.title().isEmpty()) { bookTitle = fb.title(); settings.setValue(QString("meta/title/%1").arg(absPath), bookTitle); }
            if (!fb.author().isEmpty()) { bookAuthor = fb.author(); settings.setValue(QString("meta/author/%1").arg(absPath), bookAuthor); }
            if (!fb.series().isEmpty()) { bookSeries = fb.series(); bookSeriesIndex = fb.seriesIndex(); }
            fb.close();
        }
    } else if (ext == "cbz") {
        CbzParser cp;
        if (cp.open(absPath)) {
            if (cover.isEmpty()) cover = cp.extractCover(m_cacheDir);
            if (!cp.title().isEmpty()) { bookTitle = cp.title(); settings.setValue(QString("meta/title/%1").arg(absPath), bookTitle); }
            if (!cp.author().isEmpty()) { bookAuthor = cp.author(); settings.setValue(QString("meta/author/%1").arg(absPath), bookAuthor); }
            if (!cp.series().isEmpty()) { bookSeries = cp.series(); bookSeriesIndex = cp.seriesIndex(); }
            cp.close();
        }
    } else if (ext == "mobi" || ext == "azw" || ext == "azw3" || ext == "kf8" || ext == "prc") {
        MobiParser mp;
        if (mp.open(absPath)) {
            if (cover.isEmpty()) cover = mp.extractCover(m_cacheDir);
            if (!mp.title().isEmpty()) { bookTitle = mp.title(); settings.setValue(QString("meta/title/%1").arg(absPath), bookTitle); }
            if (!mp.author().isEmpty()) { bookAuthor = mp.author(); settings.setValue(QString("meta/author/%1").arg(absPath), bookAuthor); }
            if (!mp.series().isEmpty()) { bookSeries = mp.series(); bookSeriesIndex = mp.seriesIndex(); }
            mp.close();
        }
    }

    if (bookSeries.isEmpty()) {
        extractSeriesFromTitleOrFile(bookTitle, rawName, bookSeries, bookSeriesIndex);
    }

    qint64 size = fi.size();
    QString fileSizeStr = (size < 1024 * 1024)
        ? QString("%1 KB").arg(qMax((qint64)1, size / 1024))
        : QString("%1 MB").arg(QString::number(size / (1024.0 * 1024.0), 'f', 1));
    QDateTime modTime = fi.lastModified();
    int savedProg = settings.value(QString("progressPct/%1").arg(absPath), 0).toInt();

    QVariantMap bookMap;
    bookMap["title"] = bookTitle;
    bookMap["author"] = bookAuthor;
    bookMap["series"] = bookSeries;
    bookMap["seriesIndex"] = bookSeriesIndex;
    bookMap["filePath"] = absPath;
    bookMap["path"] = absPath;
    bookMap["cover"] = cover;
    bookMap["format"] = (ext == "djv") ? QString("djvu") : ext;
    bookMap["formatBadge"] = formatBadge;
    bookMap["fileSize"] = fileSizeStr;
    bookMap["fileSizeBytes"] = size;
    bookMap["fileSizeSort"] = QString("%1").arg(size, 14, 10, QChar('0'));
    bookMap["dateModified"] = modTime.toString("dd/MM/yyyy");
    bookMap["modifiedIso"] = modTime.toString("yyyy-MM-dd hh:mm:ss");
    bookMap["modifiedTimestamp"] = (qint64)modTime.toTime_t();
    bookMap["progress"] = savedProg;
    bookMap["progressStr"] = savedProg > 0 ? QString("%1%").arg(savedProg) : QString("0%");
    bookMap["isCompleted"] = (savedProg >= 100);
    bookMap["isFavorite"] = settings.value(QString("favorites/%1").arg(absPath), false).toBool();
    bookMap["isReadingList"] = settings.value(QString("readingList/%1").arg(absPath), false).toBool();
    bookMap["isOpened"] = settings.value(QString("opened/%1").arg(absPath), false).toBool();

    int existingIdx = -1;
    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == absPath) {
            existingIdx = i;
            break;
        }
    }
    if (existingIdx >= 0) {
        m_allBooks[existingIdx] = bookMap;
    } else {
        m_allBooks.append(bookMap);
    }

    saveLibraryCache();
    updateFolderList();
    applyFilterAndSort();
    refreshBrowser();
    refreshStats();
    emit totalScannedBooksChanged();
}

void BookManager::applyFilterAndSort()
{
    m_model->clear();

    if (m_groupBy == "author") {
        m_model->setGrouping(bb::cascades::ItemGrouping::ByFullValue);
        m_model->setSortingKeys(QStringList() << "groupHeader" << "title");
        m_model->setSortedAscending(true);
    } else if (m_groupBy == "series") {
        m_model->setGrouping(bb::cascades::ItemGrouping::ByFullValue);
        m_model->setSortingKeys(QStringList() << "groupHeader" << "seriesIndexSort" << "title");
        m_model->setSortedAscending(true);
    } else {
        m_model->setGrouping(bb::cascades::ItemGrouping::None);
        if (m_sortBy == "date_asc") {
            m_model->setSortingKeys(QStringList() << "modifiedIso");
            m_model->setSortedAscending(true);
        } else if (m_sortBy == "title_asc") {
            m_model->setSortingKeys(QStringList() << "title");
            m_model->setSortedAscending(true);
        } else if (m_sortBy == "title_desc") {
            m_model->setSortingKeys(QStringList() << "title");
            m_model->setSortedAscending(false);
        } else if (m_sortBy == "author_asc") {
            m_model->setSortingKeys(QStringList() << "author" << "title");
            m_model->setSortedAscending(true);
        } else if (m_sortBy == "size_desc") {
            m_model->setSortingKeys(QStringList() << "fileSizeSort");
            m_model->setSortedAscending(false);
        } else { // default: date_desc (newest modified first)
            m_model->setSortingKeys(QStringList() << "modifiedIso");
            m_model->setSortedAscending(false);
        }
    }

    QString normFilter = m_formatFilter.trimmed().toLower();
    QString normQuery = m_searchQuery.trimmed().toLower();

    QList<QVariantMap> filteredList;
    QSettings filterSettings("Libris", "Libris");
    foreach (const QVariantMap &book, m_allBooks) {
        if (m_libraryBrowseMode == "folder" && !m_selectedFolderPath.isEmpty()) {
            QString bookDir = QFileInfo(book.value("filePath").toString()).path();
            if (bookDir != m_selectedFolderPath && !bookDir.startsWith(m_selectedFolderPath + "/")) {
                continue;
            }
        }
        if (!normFilter.isEmpty() && normFilter != "all") {
            QString bFmt = book.value("format").toString().toLower();
            if (normFilter == "mobi") {
                if (bFmt != "mobi" && bFmt != "azw" && bFmt != "azw3" && bFmt != "kf8" && bFmt != "prc") {
                    continue;
                }
            } else if (normFilter == "djvu") {
                if (bFmt != "djvu" && bFmt != "djv") {
                    continue;
                }
            } else if (bFmt != normFilter) {
                continue;
            }
        }
        int p = book.value("progress").toInt();
        bool isOpened = book.value("isOpened").toBool();
        if (!isOpened) {
            isOpened = filterSettings.value(QString("opened/%1").arg(book.value("filePath").toString()), false).toBool();
        }
        bool isCompleted = (p >= 100);
        bool isReading = (p > 0 && p < 100) || (isOpened && p < 100);

        if (m_categoryFilter == "all") {
            // Bookshelf default: show both currently reading and completed books
            if (!isReading && !isCompleted) {
                continue;
            }
        } else if (m_categoryFilter == "all_scanned") {
            // Show all scanned books
        } else if (m_categoryFilter == "favorites") {
            if (!book.value("isFavorite").toBool()) {
                continue;
            }
        } else if (m_categoryFilter == "readingList") {
            if (!book.value("isReadingList").toBool()) {
                continue;
            }
        } else if (m_categoryFilter == "reading") {
            if (p <= 0 || p >= 100) continue;
        } else if (m_categoryFilter == "completed") {
            if (p < 100) {
                continue;
            }
        } else if (m_categoryFilter == "unread") {
            if (p > 0 || isOpened || isCompleted) {
                continue;
            }
        } else if (m_categoryFilter.startsWith("shelf:")) {
            QString sName = m_categoryFilter.mid(6);
            if (!m_shelfBooks.value(sName).contains(book.value("filePath").toString())) {
                continue;
            }
        }
        if (!normQuery.isEmpty()) {
            QString title = book.value("title").toString().toLower();
            QString author = book.value("author").toString().toLower();
            if (!title.contains(normQuery) && !author.contains(normQuery)) {
                continue;
            }
        }
        QVariantMap item = book;
        if (item.value("filePath").toString() == m_lastBookPath) {
            if (!m_lastBookTitle.isEmpty()) item["title"] = m_lastBookTitle;
            if (!m_lastBookAuthor.isEmpty()) item["author"] = m_lastBookAuthor;
            if (!m_lastBookCover.isEmpty()) item["cover"] = m_lastBookCover;
        }
        if (m_groupBy == "author") {
            QString author = item.value("author").toString().trimmed();
            if (author.isEmpty() || author.toLower() == "unknown author" || author == "tác giả không rõ") {
                author = (m_language == "en" ? "Unknown Author" : "Tác giả không rõ");
            }
            item["groupHeader"] = author;
        } else if (m_groupBy == "series") {
            QString series = item.value("series").toString().trimmed();
            if (series.isEmpty()) {
                series = (m_language == "en" ? "Standalone Books" : "Sách đơn (Không thuộc series)");
            }
            item["groupHeader"] = series;
            int idx = item.value("seriesIndex").toInt();
            item["seriesIndexSort"] = QString("%1").arg(idx, 4, 10, QChar('0'));
        }
        filteredList.append(item);
    }

    m_model->insertList(filteredList);

    emit bookCountChanged();
    emit modelChanged();
}

void BookManager::updateFolderList()
{
    m_folderModel->clear();

    QMap<QString, QList<QVariantMap> > folderMap;
    foreach (const QVariantMap &book, m_allBooks) {
        QString fp = book.value("filePath").toString();
        QString dirPath = QFileInfo(fp).path();
        folderMap[dirPath].append(book);
    }

    QVariantList folderItems;
    for (QMap<QString, QList<QVariantMap> >::const_iterator it = folderMap.constBegin(); it != folderMap.constEnd(); ++it) {
        QString fullPath = it.key();
        const QList<QVariantMap> &booksInFolder = it.value();

        QString dispName = QFileInfo(fullPath).fileName();
        QString parentDir = QFileInfo(fullPath).path();
        QString parentName = QFileInfo(parentDir).fileName();
        if (!parentName.isEmpty() && parentName.toLower() != "sdcard2" && parentName.toLower() != "shared" && parentName.toLower() != "removable") {
            dispName = parentName + " / " + dispName;
        }

        QSet<QString> formats;
        foreach (const QVariantMap &b, booksInFolder) {
            formats.insert(b.value("formatBadge").toString());
        }
        QStringList formatList = formats.toList();
        qSort(formatList);

        QVariantMap item;
        item["folderPath"] = fullPath;
        item["folderName"] = dispName;
        item["shortName"] = QFileInfo(fullPath).fileName();
        item["parentName"] = parentName;
        item["bookCount"] = booksInFolder.size();
        item["bookCountStr"] = QString("%1 cuốn sách").arg(booksInFolder.size());
        item["formats"] = formatList.join(", ");
        item["icon"] = "asset:///icons/ic_folder.png";

        folderItems.append(item);
    }

    m_folderModel->insertList(folderItems);
    emit folderModelChanged();
}

void BookManager::scanLibraryQuiet()
{
    scanLibrary(true);
}

void BookManager::scanLibrary(bool quiet)
{
    retireWorkerThread(m_scanWorker, this, false);

    if (!quiet) {
        m_isScanning = true;
        m_scanStatusText = "Đang nạp và quét thư viện sách...";
        emit isScanningChanged();
        emit scanStatusTextChanged();
    }

    QStringList rawRoots;
    if (!m_scanFolders.isEmpty()) {
        foreach (const QString &cf, m_scanFolders) {
            if (QDir(cf).exists()) {
                rawRoots << cf;
            }
        }
    }
    if (rawRoots.isEmpty()) {
        // Dynamically discover all mounted removable volumes (SD cards / OTG)
        QStringList remParents;
        remParents << "/accounts/1000/removable" << "/removable";
        foreach (const QString &rp, remParents) {
            QDir rd(rp);
            if (rd.exists()) {
                foreach (const QFileInfo &vol, rd.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                    rawRoots << vol.absoluteFilePath();
                }
            }
        }
        rawRoots << "/accounts/1000/removable/sdcard2"
                 << "/accounts/1000/removable/sdcard"
                 << "/removable/sdcard2"
                 << "/removable/sdcard";

        // Internal shared storage & sandbox books fallback
        rawRoots << "/accounts/1000/shared";
        if (!QDir("/accounts/1000/shared").exists()) {
            rawRoots << QDir::homePath();
        } else if (QDir(QDir::homePath() + "/books").exists()) {
            rawRoots << (QDir::homePath() + "/books");
        }
    }

    // Deduplicate roots so parent and child directories do not scan redundantly
    QStringList searchRoots;
    foreach (const QString &r, rawRoots) {
        QDir d(r);
        if (d.exists()) {
            QString canon = d.canonicalPath();
            bool duplicate = false;
            foreach (const QString &existing, searchRoots) {
                if (canon == existing || canon.startsWith(existing + "/")) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                searchRoots.append(canon);
            }
        }
    }

    ScanPrefsSnapshot snapshot;
    {
        QSettings settings("Libris", "Libris");
        snapshot.lastBookPath = settings.value("lastBook/filePath").toString();
        snapshot.lastBookTitle = settings.value("lastBook/title").toString();
        snapshot.lastBookAuthor = settings.value("lastBook/author").toString();
        QStringList keys = settings.allKeys();
        foreach (const QString &k, keys) {
            if (k.startsWith("hidden/") && settings.value(k, false).toBool()) {
                snapshot.hiddenPaths.insert(k.mid(7));
            } else if (k.startsWith("meta/title/")) {
                snapshot.cachedTitles.insert(k.mid(11), settings.value(k).toString());
            } else if (k.startsWith("meta/author/")) {
                snapshot.cachedAuthors.insert(k.mid(12), settings.value(k).toString());
            } else if (k.startsWith("series/")) {
                snapshot.cachedSeries.insert(k.mid(7), settings.value(k).toString());
            } else if (k.startsWith("seriesIdx/")) {
                snapshot.cachedSeriesIdx.insert(k.mid(10), settings.value(k, 0).toInt());
            } else if (k.startsWith("favorites/") && settings.value(k, false).toBool()) {
                snapshot.favorites.insert(k.mid(10));
            } else if (k.startsWith("readingList/") && settings.value(k, false).toBool()) {
                snapshot.readingList.insert(k.mid(12));
            } else if (k.startsWith("opened/") && settings.value(k, false).toBool()) {
                snapshot.opened.insert(k.mid(7));
            } else if (k.startsWith("progressPct/")) {
                snapshot.progressPct.insert(k.mid(12), settings.value(k, 0).toInt());
            }
        }
    }

    m_scanWorker = new ScanWorker(searchRoots, m_cacheDir, this);
    m_scanWorker->setPrefsSnapshot(snapshot);
    connect(m_scanWorker, SIGNAL(scanCompleted(QList<QVariantMap>)), this, SLOT(onScanCompleted(QList<QVariantMap>)));
    m_scanWorker->start();

    // 25-second safety timer so the startup overlay is never stuck
    m_scanWatchdogTimer.start(25000);
}

void BookManager::reconcileLibraryWithSettings(QSettings &st)
{
    for (int i = m_allBooks.size() - 1; i >= 0; --i) {
        QString p = m_allBooks[i].value("filePath").toString();
        if (st.value(QString("hidden/%1").arg(p), false).toBool()) {
            m_allBooks.removeAt(i);
            continue;
        }
        m_allBooks[i]["isFavorite"] = st.value(QString("favorites/%1").arg(p), false).toBool();
        m_allBooks[i]["isReadingList"] = st.value(QString("readingList/%1").arg(p), false).toBool();
        m_allBooks[i]["isOpened"] = st.value(QString("opened/%1").arg(p), false).toBool();
        int pr = st.value(QString("progressPct/%1").arg(p), 0).toInt();
        m_allBooks[i]["progress"] = pr;
        m_allBooks[i]["progressStr"] = QString("%1%").arg(pr);
        m_allBooks[i]["isCompleted"] = (pr >= 100);
        if (!m_lastBookPath.isEmpty() && p == m_lastBookPath) {
            if (!m_lastBookTitle.isEmpty()) m_allBooks[i]["title"] = m_lastBookTitle;
            if (!m_lastBookAuthor.isEmpty()) m_allBooks[i]["author"] = m_lastBookAuthor;
            if (!m_lastBookCover.isEmpty()) m_allBooks[i]["cover"] = m_lastBookCover;
        }
    }
}

void BookManager::onScanCompleted(const QList<QVariantMap> &allBooks)
{
    m_scanWatchdogTimer.stop();
    m_allBooks = allBooks;

    {
        QSettings settings("Libris", "Libris");
        for (int i = 0; i < m_allBooks.size(); ++i) {
            QString absPath = m_allBooks[i].value("filePath").toString();
            if (m_allBooks[i].take("_dirtyMeta").toBool()) {
                QString t = m_allBooks[i].value("title").toString();
                QString a = m_allBooks[i].value("author").toString();
                if (!t.isEmpty()) settings.setValue(QString("meta/title/%1").arg(absPath), t);
                if (!a.isEmpty() && a != "EPUB eBook" && a != "FB2 FictionBook" && a != "Truyện tranh CBZ" && a != "Sách điện tử MOBI" && a != "Sách Kindle AZW3" && a != "Tài liệu PDF" && a != "Tài liệu DjVu") {
                    settings.setValue(QString("meta/author/%1").arg(absPath), a);
                }
            }
            if (m_allBooks[i].take("_dirtySeries").toBool()) {
                settings.setValue(QString("series/%1").arg(absPath), m_allBooks[i].value("series").toString());
                settings.setValue(QString("seriesIdx/%1").arg(absPath), m_allBooks[i].value("seriesIndex").toInt());
            }
        }
        reconcileLibraryWithSettings(settings);
    }

    qDebug() << "[BookManager] Total books found across all folders:" << m_allBooks.size();
    saveLibraryCache();
    updateFolderList();
    applyFilterAndSort();
    refreshBrowser();
    refreshStats();

    m_isScanning = false;
    m_scanStatusText.clear();
    emit isScanningChanged();
    emit scanStatusTextChanged();
    emit totalScannedBooksChanged();

    // Queue asynchronous background cover extraction for uncached books so scanning never stalls
    QStringList uncachedCovers;
    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (!m_allBooks[i].value("cover").toString().isEmpty()) continue;
        QString fmt = m_allBooks[i].value("format").toString().toLower();
        if (fmt == "epub" || fmt == "fb2" || fmt == "cbz" ||
            fmt == "mobi" || fmt == "azw" || fmt == "azw3" || fmt == "kf8" || fmt == "prc") {
            QString fp = m_allBooks[i].value("filePath").toString();
            QString hash = QString(QCryptographicHash::hash(fp.toUtf8(), QCryptographicHash::Md5).toHex());
            if (!QFile::exists(m_cacheDir + "/" + hash + ".nocover")) {
                uncachedCovers.append(fp);
            }
        }
    }
    if (!uncachedCovers.isEmpty()) {
        retireWorkerThread(m_coverWorker, this, false);
        m_coverWorker = new CoverExtractWorker(uncachedCovers, m_cacheDir, this);
        connect(m_coverWorker, SIGNAL(coverExtracted(QString,QString)), this, SLOT(onCoverExtracted(QString,QString)));
        connect(m_coverWorker, SIGNAL(finished()), this, SLOT(onBackgroundCoversFinished()));
        m_coverWorker->start();
    }
}

QString BookManager::libraryCacheFilePath() const
{
    return persistentDataDir() + "/library_books.dat";
}

void BookManager::saveLibraryCache()
{
    QString dir = persistentDataDir();
    QDir().mkpath(dir);
    QString path = libraryCacheFilePath();
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_4_8);
        quint32 magic = 0x42455252; // 'BERR'
        quint32 ver = 2;
        out << magic << ver << m_allBooks;
        file.close();
        qDebug() << "[BookManager] Saved library cache to:" << path << "Books count:" << m_allBooks.size();
    }
}

bool BookManager::loadLibraryCache()
{
    QString path = libraryCacheFilePath();
    QFile file(path);
    if (!file.exists()) return false;

    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);
        in.setVersion(QDataStream::Qt_4_8);
        quint32 magic = 0;
        quint32 ver = 0;
        in >> magic >> ver;
        if (magic == 0x42455252 && ver == 2) {
            in >> m_allBooks;
            file.close();
            qDebug() << "[BookManager] Loaded library cache from:" << path << "Books count:" << m_allBooks.size();

            // Reconcile cached entries with the authoritative QSettings state so that
            // favorites / reading list / hidden books / progress survive restarts.
            {
                QSettings st("Libris", "Libris");
                reconcileLibraryWithSettings(st);
            }
            if (!m_allBooks.isEmpty() && m_isScanning) {
                m_isScanning = false;
                m_scanStatusText.clear();
                emit isScanningChanged();
                emit scanStatusTextChanged();
            }

            updateFolderList();
            applyFilterAndSort();
            updateReadingHistoryModel();
            refreshStats();
            emit totalScannedBooksChanged();
            return true;
        }
        file.close();
    }
    return false;
}

void BookManager::cancelScan()
{
    qDebug() << "[BookManager] Scan cancelled by user tap or watchdog";
    m_scanWatchdogTimer.stop();
    if (m_scanWorker) {
        QObject::disconnect(m_scanWorker, 0, this, 0);
        if (m_scanWorker->isRunning()) {
            m_scanWorker->cancel();
        }
    }
    if (m_isScanning) {
        m_isScanning = false;
        m_scanStatusText.clear();
        emit isScanningChanged();
        emit scanStatusTextChanged();
        updateFolderList();
        applyFilterAndSort();
        refreshBrowser();
        refreshStats();
        emit totalScannedBooksChanged();
    }
}

void BookManager::onScanWatchdogTimeout()
{
    // Unfreeze UI overlay after 25s, but let ScanWorker continue finishing all remaining folders in background!
    qWarning() << "[BookManager] Scan watchdog timed out after 25s, unfreezing UI while ScanWorker continues in background";
    if (m_isScanning) {
        m_isScanning = false;
        m_scanStatusText.clear();
        emit isScanningChanged();
        emit scanStatusTextChanged();
    }
}

void BookManager::saveSettings()
{
    QSettings settings("Libris", "Libris");
    settings.setValue("app/language", m_language);
    settings.setValue("app/onboardingCompleted", m_onboardingCompleted);
    settings.setValue("reader/fontSize", m_fontSize);
    settings.setValue("reader/fontFamily", m_fontFamily);
    settings.setValue("reader/theme", m_theme);
    settings.setValue("app/theme", m_appTheme);
    settings.setValue("reader/readingMode", m_readingMode);
    settings.setValue("reader/columnLayout", m_columnLayout);
    settings.setValue("reader/mangaRtlMode", m_mangaRtlMode);
    settings.setValue("reader/lineSpacing", m_lineSpacing);
    settings.setValue("reader/textAlign", m_textAlign);
    settings.setValue("reader/overridePublisherCss", m_overridePublisherCss);
    settings.setValue("reader/paragraphIndent", m_paragraphIndent);
    settings.setValue("reader/paragraphSpacing", m_paragraphSpacing);
    settings.setValue("reader/fontWeight", m_readerFontWeight);
    settings.setValue("reader/darkTextTone", m_darkTextTone);
    settings.setValue("reader/showFooterProgressBar", m_showFooterProgressBar);
    settings.setValue("reader/showFooterTimeLeft", m_showFooterTimeLeft);
    settings.setValue("reader/eyeRestMinutes", m_eyeRestMinutes);
    settings.setValue("reader/invertVolumeKeys", m_invertVolumeKeys);
    settings.setValue("reader/bionicReading", m_bionicReading);
    settings.setValue("reader/showFooterChapter", m_showFooterChapter);
    settings.setValue("reader/showFooterProgress", m_showFooterProgress);
    settings.setValue("reader/showFooterBattery", m_showFooterBattery);
    settings.setValue("reader/showFooterClock", m_showFooterClock);
    settings.setValue("reader/marginTop", m_marginTop);
    settings.setValue("reader/marginBottom", m_marginBottom);
    settings.setValue("reader/marginLeft", m_marginLeft);
    settings.setValue("reader/marginRight", m_marginRight);
    settings.setValue("reader/ttsAutoAdvance", m_ttsAutoAdvance);
    settings.setValue("reader/pdfAutoCrop", m_isPdfAutoCrop);
    settings.setValue("reader/warmFilterEnabled", m_warmFilterEnabled);
    settings.setValue("reader/warmFilterIntensity", m_warmFilterIntensity);
    QMap<QString, int>::const_iterator itBr = m_formatBrightness.constBegin();
    while (itBr != m_formatBrightness.constEnd()) {
        settings.setValue("reader/brightness_" + itBr.key(), itBr.value());
        ++itBr;
    }
    settings.setValue("reader/autoNightTheme", m_autoNightTheme);
    settings.setValue("reader/focusModeEnabled", m_focusModeEnabled);
    settings.setValue("reader/volumeKeyTurn", m_volumeKeyTurnEnabled);
    settings.setValue("dev/serverIp", m_devServerIp);

    saveCustomShelvesToSettings(settings);

    settings.setValue("library/mainViewMode", m_mainViewMode);
    settings.setValue("library/viewMode", m_libraryViewMode);
    settings.setValue("library/browseMode", m_libraryBrowseMode);
    settings.setValue("library/sortBy", m_sortBy);
    settings.setValue("library/groupBy", m_groupBy);
    settings.setValue("library/formatFilter", m_formatFilter);

    settings.setValue("stats/totalReadingSeconds", m_totalReadingSeconds);
    settings.setValue("stats/todayReadingSeconds", m_todayReadingSeconds);
    settings.setValue("stats/dailyGoalMinutes", m_dailyGoalMinutes);
    settings.setValue("stats/todayDate", m_todayDate);

    // Save daily reading statistics
    settings.beginGroup("statsDaily");
    QMapIterator<QString, int> dIt(m_dailyReadingSeconds);
    while (dIt.hasNext()) {
        dIt.next();
        settings.setValue(dIt.key(), dIt.value());
    }
    settings.endGroup();

    // Save daily per-book reading statistics
    settings.beginGroup("statsDailyBooks");
    QMapIterator<QString, QMap<QString, int> > dbIt(m_dailyBookReadingSeconds);
    while (dbIt.hasNext()) {
        dbIt.next();
        QString dateKey = dbIt.key();
        QMap<QString, int> cleanDay;
        QMapIterator<QString, int> bIt(dbIt.value());
        while (bIt.hasNext()) {
            bIt.next();
            QString cleanPath = unwrapBookPath(bIt.key());
            if (cleanPath.isEmpty()) continue;
            if (bIt.value() > cleanDay.value(cleanPath, 0)) {
                cleanDay[cleanPath] = bIt.value();
            }
        }
        QStringList entries;
        QMapIterator<QString, int> cIt(cleanDay);
        while (cIt.hasNext()) {
            cIt.next();
            QString enc = QString::fromUtf8(QUrl::toPercentEncoding(cIt.key()));
            entries.append(enc + "|" + QString::number(cIt.value()));
        }
        settings.setValue(dateKey, entries.join(";"));
    }
    settings.endGroup();

    if (!m_currentFilePath.isEmpty()) {
        settings.setValue(QString("stats/bookSeconds/%1").arg(m_currentFilePath), m_bookReadingSeconds.value(m_currentFilePath, 0));
        int prog = readingProgress();
        settings.setValue(QString("progress/%1").arg(m_currentFilePath), m_currentChapterIndex);
        settings.setValue(QString("pageInChapter/%1").arg(m_currentFilePath), m_currentPageInChapter);
        settings.setValue(QString("totalPagesInChapter/%1").arg(m_currentFilePath), m_totalPagesInChapter);
        settings.setValue(QString("scrollPos/%1").arg(m_currentFilePath), m_currentScrollPos);
        settings.setValue(QString("progressPct/%1").arg(m_currentFilePath), prog);
        if (m_totalChapters > 0) {
            settings.setValue(QString("totalChapters/%1").arg(m_currentFilePath), m_totalChapters);
            settings.setValue(QString("totalPages/%1").arg(m_currentFilePath), m_totalChapters);
        }
        settings.setValue(QString("lastReadTime/%1").arg(m_currentFilePath), (qlonglong)QDateTime::currentDateTime().toMSecsSinceEpoch());
        settings.setValue("lastBook/filePath", m_currentFilePath);
        settings.setValue("lastBook/title", m_currentBookTitle);
        settings.setValue("lastBook/author", m_currentBookAuthor);
        settings.setValue("lastBook/cover", m_currentCoverPath);
        settings.setValue("lastBook/format", m_currentFormat);
        settings.setValue("lastBook/progress", prog);

        m_lastBookPath = m_currentFilePath;
        m_lastBookTitle = m_currentBookTitle;
        m_lastBookAuthor = m_currentBookAuthor;
        m_lastBookCover = m_currentCoverPath;
        m_lastBookFormat = m_currentFormat;
        m_lastBookProgress = prog;
        emit lastBookChanged();

        for (int i = 0; i < m_allBooks.size(); ++i) {
            if (m_allBooks[i].value("filePath").toString() == m_currentFilePath) {
                m_allBooks[i]["progress"] = prog;
                m_allBooks[i]["progressStr"] = QString("%1%").arg(prog);
                if (m_totalChapters > 0) {
                    m_allBooks[i]["totalPages"] = m_totalChapters;
                    m_allBooks[i]["totalChapters"] = m_totalChapters;
                }
                break;
            }
        }
        updateReadingHistoryModel();
    }
    settings.sync();
    if (!m_suppressPersistentSync) {
        schedulePersistentSync();
    }
}

void BookManager::saveCurrentProgress()
{
    if (m_currentFilePath.isEmpty()) return;
    if (m_progressSaveTimer.isActive()) {
        m_progressSaveTimer.stop();
    }
    onProgressSaveTimeout();
    saveSettings();
}

void BookManager::saveForShutdown()
{
    m_suppressPersistentSync = true;
    saveCurrentProgress();
    saveSettings();
    m_suppressPersistentSync = false;
    syncToPersistentStorage();
    m_shutdownSaved = true;
}

void BookManager::pauseTimersForSleep(bool saveProgress)
{
    qDebug() << "[BookManager] pauseTimersForSleep: pausing timers for zero background battery consumption";
    m_isAppSleeping = true;
    if (m_readingHeartbeatTimer.isActive()) {
        m_readingHeartbeatTimer.stop();
    }
    if (m_clockTimer.isActive()) {
        m_clockTimer.stop();
    }
    if (m_watchdogTimer.isActive()) {
        m_watchdogTimer.stop();
    }
    if (saveProgress) {
        saveCurrentProgress();
    }
    updateVolumeKeyWatchers();
}

void BookManager::resumeTimersFromSleep()
{
    qDebug() << "[BookManager] resumeTimersFromSleep: resuming timers (reader active:" << m_isReaderActive << ")";
    m_isAppSleeping = false;
    checkAutoNightTheme();
    updateVolumeKeyWatchers();
    if (m_isReaderActive) {
        if (!m_readingHeartbeatTimer.isActive()) {
            m_readingHeartbeatTimer.start(10000);
        }
        if (!m_clockTimer.isActive()) {
            m_clockTimer.start(30000);
        }
        emit currentTimeChanged();
        emit batteryLevelChanged();
    }
}

void BookManager::loadSettings()
{
    restoreFromPersistentStorageInternal(false, false);

    QSettings settings("Libris", "Libris");
    loadSettingsFromQSettings(settings);
}

void BookManager::loadSettingsFromQSettings(QSettings &settings)
{
    m_language = settings.value("app/language", "en").toString();
    m_onboardingCompleted = settings.value("app/onboardingCompleted", false).toBool();
    m_fontSize = settings.value("reader/fontSize", 18).toInt();
    m_fontFamily = settings.value("reader/fontFamily", "sans").toString();
    m_theme = settings.value("reader/theme", "dark").toString();
    m_appTheme = settings.value("app/theme", "dark").toString();
    if (m_theme != "sepia" && m_theme != m_appTheme) {
        m_theme = m_appTheme;
    }
    m_readingMode = settings.value("reader/readingMode", "paged").toString();
    QString loadedColumnLayout = settings.value("reader/columnLayout", "single").toString();
    m_columnLayout = (loadedColumnLayout == "double") ? "double" : "single";
    if (loadedColumnLayout != m_columnLayout) {
        settings.setValue("reader/columnLayout", m_columnLayout);
    }
    m_mangaRtlMode = settings.value("reader/mangaRtlMode", false).toBool();
    m_lineSpacing = settings.value("reader/lineSpacing", "normal").toString();
    QString loadedAlign = settings.value("reader/textAlign", "justify").toString();
    m_textAlign = (loadedAlign == "left") ? "left" : "justify";
    m_overridePublisherCss = settings.value("reader/overridePublisherCss", false).toBool();
    QString loadedIndent = settings.value("reader/paragraphIndent", "normal").toString();
    m_paragraphIndent = (loadedIndent == "none" || loadedIndent == "deep") ? loadedIndent : "normal";
    QString loadedPSpace = settings.value("reader/paragraphSpacing", "normal").toString();
    m_paragraphSpacing = (loadedPSpace == "compact" || loadedPSpace == "wide") ? loadedPSpace : "normal";
    int loadedFw = settings.value("reader/fontWeight", 400).toInt();
    m_readerFontWeight = (loadedFw == 500 || loadedFw == 600) ? loadedFw : 400;
    QString loadedTone = settings.value("reader/darkTextTone", "bright").toString();
    m_darkTextTone = (loadedTone == "soft" || loadedTone == "dim") ? loadedTone : "bright";
    m_showFooterProgressBar = settings.value("reader/showFooterProgressBar", true).toBool();
    m_showFooterTimeLeft = settings.value("reader/showFooterTimeLeft", true).toBool();
    m_eyeRestMinutes = qMax(0, settings.value("reader/eyeRestMinutes", 0).toInt());
    m_invertVolumeKeys = settings.value("reader/invertVolumeKeys", false).toBool();
    m_bionicReading = settings.value("reader/bionicReading", false).toBool();
    m_showFooterChapter = settings.value("reader/showFooterChapter", true).toBool();
    m_showFooterProgress = settings.value("reader/showFooterProgress", true).toBool();
    m_showFooterBattery = settings.value("reader/showFooterBattery", true).toBool();
    m_showFooterClock = settings.value("reader/showFooterClock", true).toBool();
    m_marginTop = settings.value("reader/marginTop", 16).toInt();
    m_marginBottom = settings.value("reader/marginBottom", 16).toInt();
    m_marginLeft = settings.value("reader/marginLeft", 16).toInt();
    m_marginRight = settings.value("reader/marginRight", 16).toInt();
    m_ttsAutoAdvance = settings.value("reader/ttsAutoAdvance", false).toBool();
    m_isPdfAutoCrop = settings.value("reader/pdfAutoCrop", false).toBool();
    if (m_pdfParser) {
        m_pdfParser->setAutoCrop(m_isPdfAutoCrop);
    }
    m_dailyGoalMinutes = qMax(5, settings.value("stats/dailyGoalMinutes", 30).toInt());
    m_warmFilterEnabled = settings.value("reader/warmFilterEnabled", false).toBool();
    m_warmFilterIntensity = qMax(10, qMin(60, settings.value("reader/warmFilterIntensity", 25).toInt()));
    const char *fmts[] = {"epub", "pdf", "djvu", "cbz", "txt", "fb2", "mobi"};
    for (int i = 0; i < 7; ++i) {
        QString fmtKey = QString::fromLatin1(fmts[i]);
        int val = qMax(20, qMin(100, settings.value("reader/brightness_" + fmtKey, 100).toInt()));
        m_formatBrightness.insert(fmtKey, val);
    }
    m_autoNightTheme = settings.value("reader/autoNightTheme", false).toBool();
    m_focusModeEnabled = settings.value("reader/focusModeEnabled", false).toBool();
    m_volumeKeyTurnEnabled = settings.value("reader/volumeKeyTurn", false).toBool();
    loadCustomShelvesFromSettings(settings);
    m_scanFolders = settings.value("scan/customFolders").toStringList();
    m_devServerIp = settings.value("dev/serverIp", "192.168.0.108").toString();

    loadReadingStatsFromSettings(settings);

    m_mainViewMode = settings.value("library/mainViewMode", "shelf").toString();
    if (m_mainViewMode == "browser" || m_mainViewMode.isEmpty()) {
        m_mainViewMode = "shelf"; // Ensure bookshelf tab always defaults to bookshelf mode
    }
    m_libraryViewMode = settings.value("library/viewMode", "mosaic").toString();
    m_libraryBrowseMode = settings.value("library/browseMode", "folder").toString();
    m_sortBy = settings.value("library/sortBy", "date_desc").toString();
    m_groupBy = settings.value("library/groupBy", "none").toString();
    m_formatFilter = settings.value("library/formatFilter", "all").toString();

    m_lastBookPath = settings.value("lastBook/filePath", "").toString();
    m_lastBookTitle = settings.value("lastBook/title", "").toString();
    m_lastBookAuthor = settings.value("lastBook/author", "").toString();
    m_lastBookCover = settings.value("lastBook/cover", "").toString();
    m_lastBookFormat = settings.value("lastBook/format", "").toString();
    m_lastBookProgress = settings.value("lastBook/progress", 0).toInt();

    loadHighlights();
    loadBookmarks();

    applyVisualStyle();
}

void BookManager::loadReadingStatsFromSettings(QSettings &settings)
{
    m_totalReadingSeconds = settings.value("stats/totalReadingSeconds", 0).toInt();
    m_todayDate = settings.value("stats/todayDate", QDate::currentDate().toString("yyyy-MM-dd")).toString();
    QString curDate = QDate::currentDate().toString("yyyy-MM-dd");
    if (m_todayDate == curDate) {
        m_todayReadingSeconds = settings.value("stats/todayReadingSeconds", 0).toInt();
    } else {
        m_todayDate = curDate;
        m_todayReadingSeconds = 0;
    }

    m_dailyReadingSeconds.clear();
    settings.beginGroup("statsDaily");
    QStringList dailyDates = settings.childKeys();
    foreach (const QString &d, dailyDates) {
        m_dailyReadingSeconds[d] = settings.value(d, 0).toInt();
    }
    settings.endGroup();

    m_dailyBookReadingSeconds.clear();
    settings.beginGroup("statsDailyBooks");
    QStringList dbDates = settings.childKeys();
    QRegExp entryDelimRx("[;,]");
    foreach (const QString &d, dbDates) {
        QVariant val = settings.value(d);
        bool wasListOrLegacy = (val.type() == QVariant::StringList);
        QString raw = wasListOrLegacy ? val.toStringList().join(";") : val.toString();
        if (raw.contains(',')) wasListOrLegacy = true;
        QStringList parts = raw.split(entryDelimRx, QString::SkipEmptyParts);
        bool hadCorruptArgToken = false;
        foreach (const QString &p, parts) {
            int pipeIdx = p.indexOf('|');
            if (pipeIdx > 0) {
                QString keyStr = p.left(pipeIdx);
                QString secsStr = p.mid(pipeIdx + 1).trimmed();
                int secs = secsStr.toInt();

                // Repair historical Qt QString("%1|%2").arg(enc).arg(secs) corruption where
                // "%2" inside "%2F" / "%2C" was replaced by <secs> (producing "<secs>Faccounts<secs>F...").
                if (!secsStr.isEmpty() && !keyStr.startsWith('/') && !keyStr.startsWith("%2F", Qt::CaseInsensitive)) {
                    const QString fTok = secsStr + "F";
                    const QString cTok = secsStr + "C";
                    if (keyStr.startsWith(fTok + "accounts", Qt::CaseInsensitive) ||
                        keyStr.startsWith(fTok + "app", Qt::CaseInsensitive) ||
                        keyStr.startsWith(fTok + "removable", Qt::CaseInsensitive) ||
                        keyStr.count(fTok) >= 2) {
                        hadCorruptArgToken = true;
                        QString repaired;
                        repaired.reserve(keyStr.length() + 16);
                        int pos = 0;
                        while (pos < keyStr.length()) {
                            bool notAfterPct = (pos == 0 || keyStr.at(pos - 1) != '%');
                            if (notAfterPct && keyStr.mid(pos, fTok.length()) == fTok) {
                                repaired += "%2F";
                                pos += fTok.length();
                            } else if (notAfterPct && keyStr.mid(pos, cTok.length()) == cTok) {
                                repaired += "%2C";
                                pos += cTok.length();
                            } else {
                                repaired += keyStr.at(pos);
                                ++pos;
                            }
                        }
                        keyStr = repaired;
                    }
                }

                QString path = unwrapBookPath(keyStr);
                if (!path.isEmpty() && secs > 0) {
                    if (secs > m_dailyBookReadingSeconds[d].value(path, 0)) {
                        m_dailyBookReadingSeconds[d][path] = secs;
                    }
                }
            }
        }
        if ((hadCorruptArgToken || wasListOrLegacy) && m_dailyBookReadingSeconds.contains(d)) {
            QStringList cleanEntries;
            QMapIterator<QString, int> cIt(m_dailyBookReadingSeconds[d]);
            while (cIt.hasNext()) {
                cIt.next();
                QString enc = QString::fromUtf8(QUrl::toPercentEncoding(cIt.key()));
                cleanEntries.append(enc + "|" + QString::number(cIt.value()));
            }
            settings.setValue(d, cleanEntries.join(";"));
        }
    }
    settings.endGroup();

    m_bookReadingSeconds.clear();
    QStringList allKeys = settings.allKeys();
    const QString bookSecPrefix = "stats/bookSeconds/";
    QList<QString> legacyEncodedKeysToRemove;
    foreach (const QString &k, allKeys) {
        if (k.startsWith(bookSecPrefix)) {
            QString rawSubKey = k.mid(bookSecPrefix.length());
            QString bPath = unwrapBookPath(rawSubKey);
            int secs = settings.value(k, 0).toInt();
            if (!bPath.isEmpty() && secs > 0) {
                if (secs > m_bookReadingSeconds.value(bPath, 0)) {
                    m_bookReadingSeconds[bPath] = secs;
                }
                if (isPlausibleBookPath(bPath) &&
                    (rawSubKey.contains('%') || !rawSubKey.contains('/'))) {
                    legacyEncodedKeysToRemove.append(k);
                }
            }
        }
    }
    foreach (const QString &oldKey, legacyEncodedKeysToRemove) {
        QString clean = unwrapBookPath(oldKey.mid(bookSecPrefix.length()));
        if (!clean.isEmpty() && m_bookReadingSeconds.value(clean, 0) > 0) {
            settings.setValue(QString("stats/bookSeconds/%1").arg(clean), m_bookReadingSeconds.value(clean, 0));
            settings.remove(oldKey);
        }
    }

    // Rebuild per-book totals from the per-day breakdown so that format
    // distribution / by-book stats are available right after startup.
    {
        QHash<QString, int> bookTotals;
        QMapIterator<QString, QMap<QString, int> > dit(m_dailyBookReadingSeconds);
        while (dit.hasNext()) {
            dit.next();
            QMapIterator<QString, int> bit(dit.value());
            while (bit.hasNext()) {
                bit.next();
                bookTotals[bit.key()] += bit.value();
            }
        }
        QHashIterator<QString, int> tit(bookTotals);
        while (tit.hasNext()) {
            tit.next();
            if (tit.value() > m_bookReadingSeconds.value(tit.key(), 0)) {
                m_bookReadingSeconds[tit.key()] = tit.value();
            }
        }
    }

    // Ensure today's reading seconds are synced into daily stats
    if (m_todayReadingSeconds > 0) {
        if (m_dailyReadingSeconds.value(m_todayDate, 0) < m_todayReadingSeconds) {
            m_dailyReadingSeconds[m_todayDate] = m_todayReadingSeconds;
        }
    }
}

void BookManager::setMarginTop(int margin)
{
    if (margin < 0) margin = 0;
    if (margin > 80) margin = 80;
    if (m_marginTop != margin) {
        m_marginTop = margin;
        m_cachedContentHtml.clear();
        emit marginsChanged();
        saveSettings();
    }
}

void BookManager::setMarginBottom(int margin)
{
    if (margin < 0) margin = 0;
    if (margin > 80) margin = 80;
    if (m_marginBottom != margin) {
        m_marginBottom = margin;
        m_cachedContentHtml.clear();
        emit marginsChanged();
        saveSettings();
    }
}

void BookManager::setMarginLeft(int margin)
{
    if (margin < 0) margin = 0;
    if (margin > 80) margin = 80;
    if (m_marginLeft != margin) {
        m_marginLeft = margin;
        m_cachedContentHtml.clear();
        emit marginsChanged();
        saveSettings();
    }
}

void BookManager::setMarginRight(int margin)
{
    if (margin < 0) margin = 0;
    if (margin > 80) margin = 80;
    if (m_marginRight != margin) {
        m_marginRight = margin;
        m_cachedContentHtml.clear();
        emit marginsChanged();
        saveSettings();
    }
}

void BookManager::resetMargins()
{
    m_marginTop = 16;
    m_marginBottom = 16;
    m_marginLeft = 16;
    m_marginRight = 16;
    m_cachedContentHtml.clear();
    emit marginsChanged();
    saveSettings();
}

void BookManager::setTextAlign(const QString &align)
{
    QString norm = (align == "left") ? "left" : "justify";
    if (m_textAlign != norm) {
        m_textAlign = norm;
        m_cachedContentHtml.clear();
        emit textAlignChanged();
        saveSettings();
    }
}

void BookManager::setOverridePublisherCss(bool overrideCss)
{
    if (m_overridePublisherCss != overrideCss) {
        m_overridePublisherCss = overrideCss;
        m_cachedContentHtml.clear();
        emit overridePublisherCssChanged();
        saveSettings();
    }
}

void BookManager::setParagraphIndent(const QString &indent)
{
    QString norm = (indent == "none" || indent == "deep") ? indent : "normal";
    if (m_paragraphIndent != norm) {
        m_paragraphIndent = norm;
        m_cachedContentHtml.clear();
        emit typographyChanged();
        saveSettings();
    }
}

void BookManager::setParagraphSpacing(const QString &spacing)
{
    QString norm = (spacing == "compact" || spacing == "wide") ? spacing : "normal";
    if (m_paragraphSpacing != norm) {
        m_paragraphSpacing = norm;
        m_cachedContentHtml.clear();
        emit typographyChanged();
        saveSettings();
    }
}

void BookManager::setReaderFontWeight(int weight)
{
    int norm = (weight == 500 || weight == 600) ? weight : 400;
    if (m_readerFontWeight != norm) {
        m_readerFontWeight = norm;
        m_cachedContentHtml.clear();
        emit typographyChanged();
        saveSettings();
    }
}

void BookManager::setDarkTextTone(const QString &tone)
{
    QString norm = (tone == "soft" || tone == "dim") ? tone : "bright";
    if (m_darkTextTone != norm) {
        m_darkTextTone = norm;
        m_cachedContentHtml.clear();
        emit typographyChanged();
        saveSettings();
    }
}

void BookManager::setShowFooterProgressBar(bool show)
{
    if (m_showFooterProgressBar != show) {
        m_showFooterProgressBar = show;
        emit footerSettingsChanged();
        saveSettings();
    }
}

void BookManager::setShowFooterTimeLeft(bool show)
{
    if (m_showFooterTimeLeft != show) {
        m_showFooterTimeLeft = show;
        emit footerSettingsChanged();
        saveSettings();
    }
}

void BookManager::setEyeRestMinutes(int minutes)
{
    int norm = qMax(0, minutes);
    if (m_eyeRestMinutes != norm) {
        m_eyeRestMinutes = norm;
        m_sessionReadingSeconds = 0;
        emit readingPrefsChanged();
        saveSettings();
    }
}

void BookManager::setInvertVolumeKeys(bool invert)
{
    if (m_invertVolumeKeys != invert) {
        m_invertVolumeKeys = invert;
        emit readingPrefsChanged();
        saveSettings();
    }
}

void BookManager::setBionicReading(bool enabled)
{
    if (m_bionicReading != enabled) {
        m_bionicReading = enabled;
        emit typographyChanged();
        saveSettings();
    }
}

int BookManager::getBookRating(const QString &filePath) const
{
    QString absPath = QFileInfo(filePath).absoluteFilePath();
    if (absPath.isEmpty()) absPath = filePath;
    QSettings settings("Libris", "Libris");
    return qBound(0, settings.value(QString("meta/rating/%1").arg(absPath), 0).toInt(), 5);
}

void BookManager::setBookRating(const QString &filePath, int rating)
{
    QString absPath = QFileInfo(filePath).absoluteFilePath();
    if (absPath.isEmpty()) absPath = filePath;
    int r = qBound(0, rating, 5);
    QSettings settings("Libris", "Libris");
    settings.setValue(QString("meta/rating/%1").arg(absPath), r);
    settings.sync();
    schedulePersistentSync();
}

QString BookManager::getBookReview(const QString &filePath) const
{
    QString absPath = QFileInfo(filePath).absoluteFilePath();
    if (absPath.isEmpty()) absPath = filePath;
    QSettings settings("Libris", "Libris");
    return settings.value(QString("meta/review/%1").arg(absPath)).toString();
}

void BookManager::setBookReview(const QString &filePath, const QString &review)
{
    QString absPath = QFileInfo(filePath).absoluteFilePath();
    if (absPath.isEmpty()) absPath = filePath;
    QSettings settings("Libris", "Libris");
    settings.setValue(QString("meta/review/%1").arg(absPath), review.trimmed());
    settings.sync();
    schedulePersistentSync();
}

bool BookManager::updateBookMetadata(const QString &filePath, const QString &title, const QString &author, const QString &series, int seriesIndex)
{
    QString absPath = QFileInfo(filePath).absoluteFilePath();
    if (absPath.isEmpty()) absPath = filePath;
    if (absPath.isEmpty()) return false;

    QString t = title.trimmed();
    QString a = author.trimmed();
    QString s = series.trimmed();
    int sIdx = qMax(0, seriesIndex);

    QSettings settings("Libris", "Libris");
    if (!t.isEmpty()) settings.setValue(QString("meta/title/%1").arg(absPath), t);
    if (!a.isEmpty()) settings.setValue(QString("meta/author/%1").arg(absPath), a);
    if (!s.isEmpty()) {
        settings.setValue(QString("series/%1").arg(absPath), s);
        settings.setValue(QString("seriesIdx/%1").arg(absPath), sIdx);
    } else {
        settings.remove(QString("series/%1").arg(absPath));
        settings.remove(QString("seriesIdx/%1").arg(absPath));
    }

    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == absPath || m_allBooks[i].value("filePath").toString() == filePath) {
            if (!t.isEmpty()) m_allBooks[i]["title"] = t;
            if (!a.isEmpty()) m_allBooks[i]["author"] = a;
            m_allBooks[i]["series"] = s;
            m_allBooks[i]["seriesIndex"] = sIdx;
            break;
        }
    }

    if (m_currentFilePath == absPath || m_currentFilePath == filePath) {
        if (!t.isEmpty()) m_currentBookTitle = t;
        if (!a.isEmpty()) m_currentBookAuthor = a;
        m_currentBookSeries = s;
        m_currentBookSeriesIndex = sIdx;
        emit currentBookChanged();
    }
    if (m_lastBookPath == absPath || m_lastBookPath == filePath) {
        if (!t.isEmpty()) m_lastBookTitle = t;
        if (!a.isEmpty()) m_lastBookAuthor = a;
        emit lastBookChanged();
    }

    saveLibraryCache();
    applyFilterAndSort();
    updateReadingHistoryModel();
    saveSettings();
    return true;
}

bool BookManager::hasLastBook() const
{
    return !m_lastBookPath.isEmpty() && QFile::exists(m_lastBookPath);
}

QString BookManager::lastBookTitle() const
{
    return m_lastBookTitle;
}

QString BookManager::lastBookAuthor() const
{
    return m_lastBookAuthor;
}

QString BookManager::lastBookCover() const
{
    return m_lastBookCover;
}

QString BookManager::lastBookFormat() const
{
    return m_lastBookFormat;
}

QString BookManager::lastBookPath() const
{
    return m_lastBookPath;
}

int BookManager::lastBookProgress() const
{
    return m_lastBookProgress;
}

bool BookManager::openLastBook()
{
    if (!hasLastBook()) {
        return false;
    }
    return openBook(m_lastBookPath);
}

bool BookManager::showFooterChapter() const
{
    return m_showFooterChapter;
}

void BookManager::setShowFooterChapter(bool show)
{
    if (m_showFooterChapter != show) {
        m_showFooterChapter = show;
        emit footerSettingsChanged();
        saveSettings();
    }
}

bool BookManager::showFooterProgress() const
{
    return m_showFooterProgress;
}

void BookManager::setShowFooterProgress(bool show)
{
    if (m_showFooterProgress != show) {
        m_showFooterProgress = show;
        emit footerSettingsChanged();
        saveSettings();
    }
}

bool BookManager::showFooterBattery() const
{
    return m_showFooterBattery;
}

void BookManager::setShowFooterBattery(bool show)
{
    if (m_showFooterBattery != show) {
        m_showFooterBattery = show;
        emit footerSettingsChanged();
        saveSettings();
    }
}

bool BookManager::showFooterClock() const
{
    return m_showFooterClock;
}

void BookManager::setShowFooterClock(bool show)
{
    if (m_showFooterClock != show) {
        m_showFooterClock = show;
        emit footerSettingsChanged();
        saveSettings();
    }
}

int BookManager::batteryLevel() const
{
    int lvl = m_batteryInfo.level();
    return (lvl >= 0 && lvl <= 100) ? lvl : 100;
}

QString BookManager::currentTime() const
{
    return QTime::currentTime().toString("hh:mm");
}

QString BookManager::lineSpacing() const
{
    return m_lineSpacing;
}

void BookManager::setLineSpacing(const QString &spacing)
{
    if (m_lineSpacing != spacing) {
        m_lineSpacing = spacing;
        emit lineSpacingChanged();
        emit currentChapterChanged();
        saveSettings();
    }
}

void BookManager::onClockTick()
{
    emit currentTimeChanged();
    checkAutoNightTheme();
}

void BookManager::onBatteryLevelChanged(int level, bb::device::BatteryChargingState::Type chargingState)
{
    Q_UNUSED(level);
    Q_UNUSED(chargingState);
    emit batteryLevelChanged();
}

QString BookManager::mainViewMode() const
{
    return m_mainViewMode;
}

void BookManager::setMainViewMode(const QString &mode)
{
    if ((mode == "shelf" || mode == "browser" || mode == "stats") && mode != m_mainViewMode) {
        m_mainViewMode = mode;
        if (m_mainViewMode == "stats") {
            refreshStats();
        }
        emit mainViewModeChanged();
        saveSettings();
    }
}

void BookManager::toggleMainViewMode()
{
    if (m_mainViewMode == "shelf") {
        setMainViewMode("stats");
    } else if (m_mainViewMode == "stats") {
        setMainViewMode("browser");
    } else {
        setMainViewMode("shelf");
    }
}

bool BookManager::isScanning() const
{
    return m_isScanning;
}

bool BookManager::isLoadingBook() const
{
    return m_isLoadingBook;
}

void BookManager::resetLoadingState()
{
    stopWatchdog();
    setIsLoadingBook(false);
}

void BookManager::startWatchdog(int ms)
{
    m_watchdogTimer.start(ms);
}

void BookManager::stopWatchdog()
{
    m_watchdogTimer.stop();
}

void BookManager::onWatchdogTimeout()
{
    setIsLoadingBook(false);
    emit pageRenderTimeout();
}

void BookManager::setIsLoadingBook(bool loading)
{
    if (m_isLoadingBook != loading) {
        m_isLoadingBook = loading;
        emit isLoadingBookChanged();
    }
}

QString BookManager::scanStatusText() const
{
    return tr(m_scanStatusText);
}

bb::cascades::DataModel* BookManager::fileBrowserModel() const
{
    return m_fileBrowserModel;
}

QString BookManager::currentBrowserPath() const
{
    return m_currentBrowserPath;
}

QString BookManager::currentBrowserTitle() const
{
    return m_currentBrowserTitle;
}

bool BookManager::canGoUp() const
{
    return m_canGoUp;
}

int BookManager::browserItemCount() const
{
    return m_fileBrowserModel ? m_fileBrowserModel->size() : 0;
}

int BookManager::readBookTotalPages(const QString &filePath, const QString &ext)
{
    if (filePath.isEmpty() || !QFile::exists(filePath)) return 0;

    QSettings settings("Libris", "Libris");
    int cached = settings.value(QString("totalPages/%1").arg(filePath), 0).toInt();
    if (cached > 0) return cached;
    cached = settings.value(QString("totalChapters/%1").arg(filePath), 0).toInt();
    if (cached > 0) return cached;

    // Check if book metadata was scanned into m_allBooks
    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            int p = m_allBooks[i].value("totalPages").toInt();
            if (p > 0) return p;
            p = m_allBooks[i].value("totalChapters").toInt();
            if (p > 0) return p;
            break;
        }
    }

    int result = 0;
    QString lowerExt = ext.toLower();

    if (lowerExt == "epub") {
        EpubParser ep;
        if (ep.open(filePath)) {
            result = ep.chapterCount();
        }
    } else if (lowerExt == "fb2") {
        Fb2Parser fb;
        if (fb.open(filePath)) {
            result = fb.chapterCount();
        }
    } else if (lowerExt == "pdf") {
        // Fast PDF page count reader without loading document into renderer
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            qint64 size = file.size();
            QByteArray data;
            if (size <= 4 * 1024 * 1024) {
                data = file.readAll();
            } else {
                data = file.read(2 * 1024 * 1024);
                file.seek(qMax((qint64)0, size - 2 * 1024 * 1024));
                data.append(file.readAll());
            }
            file.close();

            // Find all /Count N where /Type /Pages
            QRegExp rx("/Type\\s*/Pages[^>]*/Count\\s+(\\d+)");
            rx.setMinimal(true);
            int pos = 0;
            int maxPages = 0;
            while ((pos = rx.indexIn(data, pos)) != -1) {
                int count = rx.cap(1).toInt();
                if (count > maxPages) maxPages = count;
                pos += rx.matchedLength();
            }

            if (maxPages == 0) {
                QRegExp rx2("/Count\\s+(\\d+)[^>]*/Type\\s*/Pages");
                rx2.setMinimal(true);
                pos = 0;
                while ((pos = rx2.indexIn(data, pos)) != -1) {
                    int count = rx2.cap(1).toInt();
                    if (count > maxPages) maxPages = count;
                    pos += rx2.matchedLength();
                }
            }

            if (maxPages == 0) {
                QRegExp rx3("/Count\\s+(\\d+)");
                pos = 0;
                while ((pos = rx3.indexIn(data, pos)) != -1) {
                    int count = rx3.cap(1).toInt();
                    if (count > maxPages) maxPages = count;
                    pos += rx3.matchedLength();
                }
            }

            result = maxPages;
        }
    } else if (lowerExt == "txt") {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            qint64 size = file.size();
            file.close();
            // Estimate ~2500 bytes per page
            result = qMax(1, (int)(size / 2500));
        }
    } else if (lowerExt == "cbz") {
        CbzParser cp;
        if (cp.open(filePath)) {
            result = cp.chapterCount();
        }
    } else if (lowerExt == "mobi" || lowerExt == "azw" || lowerExt == "azw3" || lowerExt == "kf8" || lowerExt == "prc") {
        MobiParser mp;
        if (mp.open(filePath)) {
            result = mp.chapterCount();
        }
    }

    if (result > 0) {
        settings.setValue(QString("totalPages/%1").arg(filePath), result);
        settings.setValue(QString("totalChapters/%1").arg(filePath), result);
    }

    return result;
}

void BookManager::openBrowserFolder(const QString &folderPath)
{
    if (!m_fileBrowserModel) return;
    if (folderPath == "__UP__") {
        browserGoUp();
        return;
    }
    if (folderPath == "__INTERNAL__") {
        QString sharedPath = "/accounts/1000/shared";
        if (!QDir(sharedPath).exists()) sharedPath = QDir::homePath();
        openBrowserFolder(QDir(sharedPath).exists() ? sharedPath : "root");
        return;
    }
    if (folderPath == "__SDCARD__") {
        QStringList sdCandidates;
        sdCandidates << "/accounts/1000/removable/sdcard2"
                     << "/accounts/1000/removable/sdcard"
                     << "/removable/sdcard2"
                     << "/removable/sdcard";
        QString detectedSd;
        foreach (const QString &sd, sdCandidates) {
            if (QDir(sd).exists()) { detectedSd = sd; break; }
        }
        openBrowserFolder(detectedSd.isEmpty() ? "root" : detectedSd);
        return;
    }
    m_fileBrowserModel->clear();

    if (folderPath.isEmpty() || folderPath == "root") {
        m_currentBrowserPath = "root";
        m_currentBrowserTitle = tr("Duyệt Thư Mục");
        m_canGoUp = false;

        QVariantList rootItems;

        // 1. Check Removable MicroSD
        QStringList sdCandidates;
        sdCandidates << "/accounts/1000/removable/sdcard2"
                     << "/accounts/1000/removable/sdcard"
                     << "/removable/sdcard2"
                     << "/removable/sdcard";
        QString detectedSd;
        foreach (const QString &sd, sdCandidates) {
            if (QDir(sd).exists()) {
                detectedSd = sd;
                break;
            }
        }
        if (!detectedSd.isEmpty()) {
            QVariantMap item;
            item["itemType"] = "folder";
            item["isDir"] = true;
            item["folderType"] = "sdcard";
            item["name"] = tr("Thẻ nhớ ngoài");
            item["title"] = tr("Thẻ nhớ ngoài");
            item["path"] = detectedSd;
            item["subtext"] = tr("Thẻ nhớ ngoài (MicroSD)");
            item["sortKey"] = "1_0_sdcard";
            item["icon"] = "asset:///icons/ic_sdcard.png";
            rootItems.append(item);
        }

        // 2. Check Shared root (Internal storage)
        QString sharedPath = "/accounts/1000/shared";
        if (!QDir(sharedPath).exists()) {
            sharedPath = QDir::homePath();
        }
        if (QDir(sharedPath).exists()) {
            QVariantMap item;
            item["itemType"] = "folder";
            item["isDir"] = true;
            item["folderType"] = "internal";
            item["name"] = tr("Bộ nhớ trong");
            item["title"] = tr("Bộ nhớ trong");
            item["path"] = sharedPath;
            item["subtext"] = tr("Bộ nhớ thiết bị (Internal storage)");
            item["sortKey"] = "1_1_shared";
            item["icon"] = "asset:///icons/ic_storage.png";
            rootItems.append(item);
        }

        m_currentFolderAllItems = rootItems;
        applyBrowserFilter();
        emit currentBrowserPathChanged();
        return;
    }

    QDir dir(folderPath);
    if (!dir.exists()) {
        qDebug() << "[BookManager] openBrowserFolder: Directory does not exist, falling back to root:" << folderPath;
        openBrowserFolder("root");
        return;
    }

    qDebug() << "[BookManager] openBrowserFolder:" << folderPath;

    m_currentBrowserPath = dir.absolutePath();
    if (m_currentBrowserPath == "/accounts/1000/shared" || m_currentBrowserPath == QDir::homePath()) {
        m_currentBrowserTitle = tr("Bộ nhớ trong");
    } else if (m_currentBrowserPath.startsWith("/accounts/1000/removable/sdcard") || m_currentBrowserPath.startsWith("/removable/sdcard")) {
        if (m_currentBrowserPath == "/accounts/1000/removable/sdcard2" || m_currentBrowserPath == "/accounts/1000/removable/sdcard" || m_currentBrowserPath == "/removable/sdcard2" || m_currentBrowserPath == "/removable/sdcard") {
            m_currentBrowserTitle = tr("Thẻ nhớ ngoài");
        } else {
            m_currentBrowserTitle = dir.dirName();
        }
    } else {
        m_currentBrowserTitle = dir.dirName();
    }
    m_canGoUp = true;

    QVariantList items;

    // 1. Subfolders (Parent navigation uses native TitleBar dismiss/back action)
    QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    foreach (const QFileInfo &sub, subDirs) {
        if (shouldSkipScanDir(sub)) {
            continue;
        }

        QVariantMap item;
        item["itemType"] = "folder";
        item["isDir"] = true;
        item["folderType"] = "folder";
        item["name"] = sub.fileName();
        item["title"] = sub.fileName();
        item["path"] = sub.absoluteFilePath();
        item["subtext"] = tr("Thư mục");
        item["sortKey"] = "1_" + sub.fileName().toLower();
        item["icon"] = "asset:///icons/ic_folder.png";

        // Fast O(1) folder cover check (No synchronous ZIP parsing!)
        QString folderCover;
        QString subAbsPath = sub.absoluteFilePath();
        if (QFile::exists(subAbsPath + "/cover.jpg")) {
            folderCover = subAbsPath + "/cover.jpg";
        } else if (QFile::exists(subAbsPath + "/folder.jpg")) {
            folderCover = subAbsPath + "/folder.jpg";
        } else if (QFile::exists(subAbsPath + "/cover.png")) {
            folderCover = subAbsPath + "/cover.png";
        } else if (QFile::exists(subAbsPath + "/folder.png")) {
            folderCover = subAbsPath + "/folder.png";
        } else {
            // Check in-memory book cache prefix (pure memory lookup, zero disk I/O)
            QString subPrefix = subAbsPath;
            if (!subPrefix.endsWith('/')) subPrefix += '/';
            for (int i = 0; i < m_allBooks.size(); ++i) {
                QString bp = m_allBooks[i].value("filePath").toString();
                if (bp.startsWith(subPrefix)) {
                    QString c = m_allBooks[i].value("cover").toString();
                    if (!c.isEmpty() && QFile::exists(c)) {
                        folderCover = c;
                        break;
                    }
                }
            }
        }

        if (!folderCover.isEmpty() && QFile::exists(folderCover)) {
            item["cover"] = folderCover;
        }

        items.append(item);
    }

    // 2. Book files (case-insensitive extension matching)
    QFileInfoList allDirFiles = dir.entryInfoList(QDir::Files, QDir::Name | QDir::IgnoreCase);
    QFileInfoList bookFiles;
    foreach (const QFileInfo &cand, allDirFiles) {
        if (isSupportedBookExt(cand.suffix().toLower())) {
            bookFiles.append(cand);
        }
    }

    // Fast O(1) in-memory lookup table for library books
    QHash<QString, const QVariantMap*> bookLookup;
    for (int i = 0; i < m_allBooks.size(); ++i) {
        bookLookup.insert(m_allBooks[i].value("filePath").toString(), &m_allBooks[i]);
    }

    QSettings settings("Libris", "Libris");
    QStringList uncachedBooksForBackgroundExtraction;

    foreach (const QFileInfo &fi, bookFiles) {
        QString absPath = fi.absoluteFilePath();
        QString ext = fi.suffix().toLower();
        qint64 size = fi.size();
        QString fileSizeStr;
        if (size < 1024 * 1024) {
            fileSizeStr = QString("%1 KB").arg(qMax((qint64)1, size / 1024));
        } else {
            fileSizeStr = QString("%1 MB").arg(QString::number(size / (1024.0 * 1024.0), 'f', 1));
        }

        QString bookTitle = fi.completeBaseName().replace('_', ' ');
        QString bookAuthor = "";
        QString cover = "";
        int savedProg = 0;
        int totalPages = 0;
        int curChap = 0;
        bool isFav = false;
        bool isReading = false;

        const QVariantMap *cached = bookLookup.value(absPath, 0);
        if (cached) {
            bookTitle = cached->value("title", bookTitle).toString();
            bookAuthor = cached->value("author").toString();
            cover = cached->value("cover").toString();
            savedProg = cached->value("progress", 0).toInt();
            totalPages = cached->value("totalPages", 0).toInt();
            if (totalPages <= 0) totalPages = cached->value("totalChapters", 0).toInt();
            curChap = cached->value("currentChapter", 0).toInt();
            isFav = cached->value("isFavorite", false).toBool();
            isReading = cached->value("isReadingList", false).toBool();
        } else {
            QString metaTitle = settings.value(QString("meta/title/%1").arg(absPath)).toString();
            QString metaAuthor = settings.value(QString("meta/author/%1").arg(absPath)).toString();
            if (!metaTitle.isEmpty()) bookTitle = metaTitle;
            if (!metaAuthor.isEmpty()) bookAuthor = metaAuthor;
            savedProg = settings.value(QString("progressPct/%1").arg(absPath), 0).toInt();
            totalPages = settings.value(QString("totalPages/%1").arg(absPath), 0).toInt();
            if (totalPages <= 0) {
                totalPages = settings.value(QString("totalChapters/%1").arg(absPath), 0).toInt();
            }
            curChap = settings.value(QString("progress/%1").arg(absPath), 0).toInt();
            isFav = settings.value(QString("favorites/%1").arg(absPath), false).toBool();
            isReading = settings.value(QString("readingList/%1").arg(absPath), false).toBool();
        }

        if (absPath == m_lastBookPath) {
            if (!m_lastBookTitle.isEmpty()) bookTitle = m_lastBookTitle;
            if (!m_lastBookAuthor.isEmpty()) bookAuthor = m_lastBookAuthor;
            if (!m_lastBookCover.isEmpty()) cover = m_lastBookCover;
        }

        if (absPath == m_currentFilePath && m_totalChapters > 0) {
            totalPages = m_totalChapters;
            savedProg = qMax(savedProg, readingProgress());
            curChap = m_currentChapterIndex;
        }

        QString hash = QString(QCryptographicHash::hash(absPath.toUtf8(), QCryptographicHash::Md5).toHex());
        QString cachedJpg = m_cacheDir + "/" + hash + ".jpg";
        QString cachedPng = m_cacheDir + "/" + hash + ".png";

        if (cover.isEmpty()) {
            if (QFile::exists(cachedJpg)) {
                cover = cachedJpg;
            } else if (QFile::exists(cachedPng)) {
                cover = cachedPng;
            } else {
                QString cachedNoCover = m_cacheDir + "/" + hash + ".nocover";
                if (!QFile::exists(cachedNoCover) && size <= 80 * 1024 * 1024 &&
                    (ext == "epub" || ext == "mobi" || ext == "azw" || ext == "azw3" || ext == "kf8" || ext == "prc" || ext == "fb2" || ext == "cbz")) {
                    uncachedBooksForBackgroundExtraction.append(absPath);
                }
            }
        }

        QString pageInfoStr = "";
        if (totalPages > 0) {
            if (ext == "pdf" || ext == "djvu" || ext == "djv") {
                if (savedProg > 0 && curChap > 0) {
                    pageInfoStr = QString(tr("Trang %1 / %2")).arg(curChap).arg(totalPages);
                } else {
                    pageInfoStr = QString(tr("%1 trang")).arg(totalPages);
                }
            } else if (ext == "cbz") {
                if (savedProg > 0) {
                    pageInfoStr = QString(tr("Trang %1 / %2")).arg(curChap + 1).arg(totalPages);
                } else {
                    pageInfoStr = QString(tr("%1 trang")).arg(totalPages);
                }
            } else if (ext == "epub" || ext == "fb2" || ext == "mobi" || ext == "azw" || ext == "azw3" || ext == "kf8" || ext == "prc") {
                if (savedProg > 0) {
                    pageInfoStr = QString(tr("Chương %1 / %2")).arg(curChap + 1).arg(totalPages);
                } else {
                    pageInfoStr = QString(tr("%1 chương")).arg(totalPages);
                }
            } else {
                if (savedProg > 0 && curChap > 0) {
                    pageInfoStr = QString(tr("Trang %1 / %2")).arg(curChap + 1).arg(totalPages);
                } else {
                    pageInfoStr = QString(tr("%1 trang")).arg(totalPages);
                }
            }
        }

        QVariantMap item;
        item["itemType"] = "book";
        item["isDir"] = false;
        item["folderType"] = "book";
        item["name"] = bookTitle;
        item["title"] = bookTitle;
        item["author"] = bookAuthor;
        item["filePath"] = absPath;
        item["path"] = absPath;
        item["format"] = ext;
        item["formatBadge"] = ext.toUpper();
        item["fileSize"] = fileSizeStr;
        item["subtext"] = !bookAuthor.isEmpty() ? QString("%1 • %2").arg(bookAuthor, fileSizeStr) : fileSizeStr;
        item["cover"] = cover;
        item["progress"] = savedProg;
        item["progressStr"] = savedProg > 0 ? QString("%1%").arg(savedProg) : QString("0%");
        item["totalPages"] = totalPages;
        item["currentChapter"] = curChap;
        item["pageInfoStr"] = pageInfoStr;
        item["dateModified"] = fi.lastModified().toString("dd/MM/yyyy");
        item["isFavorite"] = isFav;
        item["isReadingList"] = isReading;
        item["sortKey"] = "2_" + bookTitle.toLower();
        item["icon"] = "asset:///icons/ic_book.png";

        items.append(item);
    }

    m_currentFolderAllItems = items;
    applyBrowserFilter();
    emit currentBrowserPathChanged();

    // Trigger asynchronous non-blocking cover extraction for uncached books in current folder
    retireWorkerThread(m_coverWorker, this, false);
    if (!uncachedBooksForBackgroundExtraction.isEmpty()) {
        m_coverWorker = new CoverExtractWorker(uncachedBooksForBackgroundExtraction, m_cacheDir, this);
        connect(m_coverWorker, SIGNAL(coverExtracted(QString,QString)), this, SLOT(onCoverExtracted(QString,QString)));
        m_coverWorker->start();
    }
}

void BookManager::onCoverExtracted(const QString &filePath, const QString &coverPath)
{
    if (filePath.isEmpty() || coverPath.isEmpty()) return;

    // 1. Update in m_currentFolderAllItems
    for (int i = 0; i < m_currentFolderAllItems.size(); ++i) {
        QVariantMap item = m_currentFolderAllItems[i].toMap();
        if (item.value("filePath").toString() == filePath || item.value("path").toString() == filePath) {
            item["cover"] = coverPath;
            m_currentFolderAllItems[i] = item;
            break;
        }
    }

    // 2. Update in m_fileBrowserModel in-place without triggering full jump/resort
    if (m_fileBrowserModel) {
        QVariantList indexPath = m_fileBrowserModel->first();
        while (!indexPath.isEmpty()) {
            QVariantMap item = m_fileBrowserModel->data(indexPath).toMap();
            if (item.value("filePath").toString() == filePath || item.value("path").toString() == filePath) {
                item["cover"] = coverPath;
                m_fileBrowserModel->updateItem(indexPath, item);
                break;
            }
            indexPath = m_fileBrowserModel->after(indexPath);
        }
    }

    // 3. Update in m_allBooks and m_model in-place if present
    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            m_allBooks[i]["cover"] = coverPath;
            break;
        }
    }
    if (m_model) {
        QVariantList indexPath = m_model->first();
        while (!indexPath.isEmpty()) {
            QVariantMap item = m_model->data(indexPath).toMap();
            if (item.value("filePath").toString() == filePath || item.value("path").toString() == filePath) {
                item["cover"] = coverPath;
                m_model->updateItem(indexPath, item);
                break;
            }
            indexPath = m_model->after(indexPath);
        }
    }
}

void BookManager::onBackgroundCoversFinished()
{
    saveLibraryCache();
}

QStringList BookManager::scanFolders() const
{
    return m_scanFolders;
}

QString BookManager::scanFoldersSummary() const
{
    if (m_scanFolders.isEmpty()) {
        return tr("Tự động (Bộ nhớ trong & Thẻ nhớ)");
    }
    QStringList names;
    for (int i = 0; i < m_scanFolders.size(); ++i) {
        QString n = QFileInfo(m_scanFolders.at(i)).fileName();
        names.append(n.isEmpty() ? m_scanFolders.at(i) : n);
    }
    return QString("%1 %2: %3").arg(m_scanFolders.size()).arg(tr("thư mục"), names.join(", "));
}

bool BookManager::isScanFolderSelected(const QString &folderPath) const
{
    if (folderPath.isEmpty()) return false;
    QString canon = QDir(folderPath).canonicalPath();
    for (int i = 0; i < m_scanFolders.size(); ++i) {
        const QString &f = m_scanFolders.at(i);
        if (f == folderPath || (!canon.isEmpty() && QDir(f).canonicalPath() == canon)) {
            return true;
        }
    }
    return false;
}

void BookManager::toggleScanFolder(const QString &folderPath)
{
    if (folderPath.isEmpty() || !QDir(folderPath).exists()) return;
    QString abs = QDir(folderPath).absolutePath();
    QString canon = QDir(folderPath).canonicalPath();
    int foundIdx = -1;
    for (int i = 0; i < m_scanFolders.size(); ++i) {
        const QString &f = m_scanFolders.at(i);
        if (f == abs || f == folderPath || (!canon.isEmpty() && QDir(f).canonicalPath() == canon)) {
            foundIdx = i;
            break;
        }
    }
    if (foundIdx >= 0) {
        m_scanFolders.removeAt(foundIdx);
    } else {
        m_scanFolders.append(abs);
    }
    QSettings settings("Libris", "Libris");
    settings.setValue("scan/customFolders", m_scanFolders);
    settings.sync();
    emit scanFoldersChanged();
}

void BookManager::clearScanFolders()
{
    if (m_scanFolders.isEmpty()) return;
    m_scanFolders.clear();
    QSettings settings("Libris", "Libris");
    settings.remove("scan/customFolders");
    settings.sync();
    emit scanFoldersChanged();
}

QString BookManager::parentFolderForPicker(const QString &dirPath) const
{
    if (dirPath.isEmpty() || dirPath == "root") return "root";
    QString clean = QDir(dirPath).absolutePath();
    if (clean == "/accounts/1000/shared" || clean == QDir::homePath() ||
        clean == "/accounts/1000/removable" || clean == "/removable") {
        return "root";
    }
    if (clean.startsWith("/accounts/1000/removable/") || clean.startsWith("/removable/")) {
        QDir upDir(clean);
        if (!upDir.cdUp() || upDir.absolutePath() == "/accounts/1000/removable" || upDir.absolutePath() == "/removable") {
            return "root";
        }
        return upDir.absolutePath();
    }
    QDir d(clean);
    if (!d.cdUp() || d.absolutePath() == "/accounts/1000" || d.absolutePath() == "/") {
        return "root";
    }
    return d.absolutePath();
}

QVariantList BookManager::listFoldersForPicker(const QString &dirPath) const
{
    QVariantList out;
    if (dirPath.isEmpty() || dirPath == "root") {
        QStringList remParents;
        remParents << "/accounts/1000/removable" << "/removable";
        QSet<QString> seenCanons;
        foreach (const QString &rp, remParents) {
            QDir rd(rp);
            if (!rd.exists()) continue;
            foreach (const QFileInfo &vol, rd.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase)) {
                QString canon = QDir(vol.absoluteFilePath()).canonicalPath();
                if (seenCanons.contains(canon)) continue;
                seenCanons.insert(canon);
                QVariantMap m;
                m["name"] = tr("Thẻ nhớ ngoài (%1)").arg(vol.fileName());
                m["path"] = vol.absoluteFilePath();
                m["subtext"] = vol.absoluteFilePath();
                m["isSelected"] = isScanFolderSelected(vol.absoluteFilePath());
                out.append(m);
            }
        }
        QString sharedPath = "/accounts/1000/shared";
        if (!QDir(sharedPath).exists()) sharedPath = QDir::homePath();
        if (QDir(sharedPath).exists()) {
            QVariantMap m;
            m["name"] = tr("Bộ nhớ trong");
            m["path"] = sharedPath;
            m["subtext"] = sharedPath;
            m["isSelected"] = isScanFolderSelected(sharedPath);
            out.append(m);
        }
        return out;
    }

    QDir d(dirPath);
    if (!d.exists()) return out;
    QFileInfoList subDirs = d.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    foreach (const QFileInfo &sub, subDirs) {
        if (shouldSkipScanDir(sub)) continue;
        QString p = sub.absoluteFilePath();
        QVariantMap m;
        m["name"] = sub.fileName();
        m["path"] = p;
        m["subtext"] = p;
        m["isSelected"] = isScanFolderSelected(p);
        out.append(m);
    }
    return out;
}

void BookManager::browserGoUp()
{
    if (m_currentBrowserPath.isEmpty() || m_currentBrowserPath == "root") {
        return;
    }

    // If current path is one of the top storage roots, go back to "root"
    QStringList topRoots;
    topRoots << "/accounts/1000/removable/sdcard2"
             << "/accounts/1000/removable/sdcard"
             << "/removable/sdcard2"
             << "/removable/sdcard"
             << "/accounts/1000/shared"
             << QDir::homePath();

    foreach (const QString &tr, topRoots) {
        if (!tr.isEmpty() && (m_currentBrowserPath == tr || m_currentBrowserPath == QDir(tr).canonicalPath())) {
            openBrowserFolder("root");
            return;
        }
    }

    QDir cur(m_currentBrowserPath);
    if (cur.cdUp()) {
        openBrowserFolder(cur.absolutePath());
    } else {
        openBrowserFolder("root");
    }
}

void BookManager::refreshBrowser()
{
    openBrowserFolder(m_currentBrowserPath);
}

void BookManager::applyBrowserFilter()
{
    if (!m_fileBrowserModel) return;
    m_fileBrowserModel->clear();

    QString q = m_browserSearchQuery.trimmed().toLower();
    QString fmt = m_browserFormatFilter.trimmed().toLower();

    QVariantList filtered;
    for (int i = 0; i < m_currentFolderAllItems.size(); ++i) {
        const QVariantMap item = m_currentFolderAllItems.at(i).toMap();
        QString itemType = item.value("itemType").toString();
        QString path = item.value("path").toString();

        if (path == "__UP__") {
            filtered.append(item);
            continue;
        }

        if (fmt != "all" && !fmt.isEmpty()) {
            if (itemType == "book") {
                QString itemFmt = item.value("format").toString().toLower();
                if (fmt == "mobi") {
                    if (itemFmt != "mobi" && itemFmt != "azw" && itemFmt != "azw3" && itemFmt != "kf8" && itemFmt != "prc") {
                        continue;
                    }
                } else if (itemFmt != fmt) {
                    continue;
                }
            }
        }

        if (!q.isEmpty()) {
            QString name = item.value("name").toString().toLower();
            QString title = item.value("title").toString().toLower();
            QString author = item.value("author").toString().toLower();
            if (!name.contains(q) && !title.contains(q) && !author.contains(q)) {
                continue;
            }
        }

        filtered.append(item);
    }

    m_fileBrowserModel->insertList(filtered);
    emit browserItemCountChanged();
}

void BookManager::setBrowserSearchQuery(const QString &query)
{
    if (m_browserSearchQuery != query) {
        m_browserSearchQuery = query;
        emit browserSearchQueryChanged();
        applyBrowserFilter();
    }
}

void BookManager::setBrowserFormatFilter(const QString &format)
{
    if (m_browserFormatFilter != format) {
        m_browserFormatFilter = format;
        emit browserFormatFilterChanged();
        applyBrowserFilter();
    }
}

void BookManager::clearBrowserFilter()
{
    bool changed = false;
    if (!m_browserSearchQuery.isEmpty()) {
        m_browserSearchQuery.clear();
        emit browserSearchQueryChanged();
        changed = true;
    }
    if (m_browserFormatFilter != "all") {
        m_browserFormatFilter = "all";
        emit browserFormatFilterChanged();
        changed = true;
    }
    if (changed) {
        applyBrowserFilter();
    }
}

void BookManager::setRecentSearchQuery(const QString &query)
{
    if (m_recentSearchQuery != query) {
        m_recentSearchQuery = query;
        emit recentSearchQueryChanged();
        updateReadingHistoryModel();
    }
}

QString BookManager::formatDuration(int seconds) const
{
    if (seconds <= 0) {
        return m_language == "vi" ? tr("0 phút") : "0 min";
    }
    if (seconds < 60) {
        return m_language == "vi" ? tr("Dưới 1 phút") : "< 1 min";
    }
    int mins = seconds / 60;
    int hours = mins / 60;
    int remMins = mins % 60;

    if (m_language == "vi") {
        if (hours > 0) {
            if (remMins > 0) {
                return QString("%1 giờ %2 phút").arg(hours).arg(remMins);
            }
            return QString("%1 giờ").arg(hours);
        }
        return QString("%1 phút").arg(remMins);
    } else {
        if (hours > 0) {
            QString hUnit = (hours == 1) ? "hour" : "hours";
            if (remMins > 0) {
                QString mUnit = (remMins == 1) ? "min" : "mins";
                return QString("%1 %2 %3 %4").arg(hours).arg(hUnit).arg(remMins).arg(mUnit);
            }
            return QString("%1 %2").arg(hours).arg(hUnit);
        }
        QString mUnit = (remMins == 1) ? "minute" : "minutes";
        return QString("%1 %2").arg(remMins).arg(mUnit);
    }
}

QString BookManager::formatShortDuration(int seconds) const
{
    if (seconds <= 0) {
        return "-";
    }
    const bool vi = (m_language == "vi");
    if (seconds < 60) {
        return vi ? "<1p" : "<1m";
    }
    int mins = seconds / 60;
    if (mins < 60) {
        return QString("%1%2").arg(mins).arg(vi ? "p" : "m");
    }
    int h = mins / 60;
    int m = mins % 60;
    if (m > 0) {
        return vi ? QString("%1g%2p").arg(h).arg(m) : QString("%1h%2m").arg(h).arg(m);
    }
    return vi ? QString("%1g").arg(h) : QString("%1h").arg(h);
}

QString BookManager::totalReadingTimeStr() const
{
    return formatDuration(m_totalReadingSeconds);
}

QString BookManager::todayReadingTimeStr() const
{
    return formatDuration(m_todayReadingSeconds);
}

int BookManager::totalReadingSeconds() const
{
    return m_totalReadingSeconds;
}

int BookManager::todayReadingSeconds() const
{
    return m_todayReadingSeconds;
}

int BookManager::totalLibraryBooks() const
{
    return m_allBooks.size();
}

int BookManager::completedBooksCount() const
{
    QSet<QString> countedPaths;
    int count = 0;
    foreach (const QVariantMap &b, m_allBooks) {
        QString path = b.value("filePath").toString();
        int p = b.value("progress").toInt();
        if (path == m_currentFilePath && m_totalChapters > 0) {
            p = qMax(p, readingProgress());
        }
        if (p >= 95) {
            count++;
            countedPaths.insert(path);
        }
    }
    if (!m_lastBookPath.isEmpty() && !countedPaths.contains(m_lastBookPath) && m_lastBookProgress >= 95) {
        count++;
        countedPaths.insert(m_lastBookPath);
    }
    return count;
}

int BookManager::readingBooksCount() const
{
    QSet<QString> countedPaths;
    int count = 0;
    foreach (const QVariantMap &b, m_allBooks) {
        QString path = b.value("filePath").toString();
        int p = b.value("progress").toInt();
        if (path == m_currentFilePath && m_totalChapters > 0) {
            p = qMax(p, readingProgress());
        }
        if (p > 0 && p < 95) {
            count++;
            countedPaths.insert(path);
        }
    }
    if (!m_lastBookPath.isEmpty() && !countedPaths.contains(m_lastBookPath) && m_lastBookProgress > 0 && m_lastBookProgress < 95) {
        count++;
        countedPaths.insert(m_lastBookPath);
    }
    if (!m_bookReadingSeconds.isEmpty()) {
        QSettings s("Libris", "Libris");
        QMapIterator<QString, int> it(m_bookReadingSeconds);
        while (it.hasNext()) {
            it.next();
            if (it.value() > 0 && !countedPaths.contains(it.key())) {
                int p = s.value(QString("progressPct/%1").arg(it.key()), 0).toInt();
                if (p > 0 && p < 95) {
                    count++;
                    countedPaths.insert(it.key());
                }
            }
        }
    }
    return count;
}

int BookManager::unreadBooksCount() const
{
    int count = 0;
    foreach (const QVariantMap &b, m_allBooks) {
        int p = b.value("progress").toInt();
        if (b.value("filePath").toString() == m_currentFilePath && m_totalChapters > 0) {
            p = qMax(p, readingProgress());
        }
        if (p <= 0) {
            count++;
        }
    }
    return count;
}

int BookManager::averageProgress() const
{
    if (m_allBooks.isEmpty()) {
        return 0;
    }
    int total = 0;
    foreach (const QVariantMap &b, m_allBooks) {
        int p = b.value("progress").toInt();
        if (b.value("filePath").toString() == m_currentFilePath && m_totalChapters > 0) {
            p = qMax(p, readingProgress());
        }
        total += p;
    }
    return total / m_allBooks.size();
}

bb::cascades::DataModel* BookManager::readingHistoryModel() const
{
    return m_readingHistoryModel;
}

bb::cascades::DataModel* BookManager::homeFeedModel() const
{
    return m_homeFeedModel;
}

int BookManager::currentStreak() const
{
    int streak = 0;
    QDate today = QDate::currentDate();
    QDate checkDate = today;
    if (m_dailyReadingSeconds.value(today.toString("yyyy-MM-dd"), 0) > 0) {
        while (m_dailyReadingSeconds.value(checkDate.toString("yyyy-MM-dd"), 0) > 0) {
            streak++;
            checkDate = checkDate.addDays(-1);
        }
    } else {
        checkDate = today.addDays(-1);
        while (m_dailyReadingSeconds.value(checkDate.toString("yyyy-MM-dd"), 0) > 0) {
            streak++;
            checkDate = checkDate.addDays(-1);
        }
    }
    return streak;
}

int BookManager::dailyGoalMinutes() const
{
    return m_dailyGoalMinutes;
}

void BookManager::setDailyGoalMinutes(int minutes)
{
    int clamped = qMax(5, qMin(240, minutes));
    if (m_dailyGoalMinutes != clamped) {
        m_dailyGoalMinutes = clamped;
        emit statsChanged();
        saveSettings();
    }
}

void BookManager::refreshStats()
{
    QSettings settings("Libris", "Libris");
    for (int i = 0; i < m_allBooks.size(); ++i) {
        QString path = m_allBooks[i].value("filePath").toString();
        int prog = settings.value(QString("progressPct/%1").arg(path), m_allBooks[i].value("progress").toInt()).toInt();
        if (path == m_currentFilePath && m_totalChapters > 0) {
            prog = qMax(prog, readingProgress());
        }
        m_allBooks[i]["progress"] = prog;
        m_allBooks[i]["progressStr"] = QString("%1%").arg(prog);
    }
    updateReadingHistoryModel();
    emit statsChanged();
}

// =============================================================================
// CALENDAR & READING STATISTICS (KOReader Style)
// =============================================================================

QString BookManager::formatDateYMD(int year, int month, int day) const
{
    return QString("%1-%2-%3")
            .arg(year, 4, 10, QChar('0'))
            .arg(month, 2, 10, QChar('0'))
            .arg(day, 2, 10, QChar('0'));
}

int BookManager::getDailyReadingSeconds(const QString &dateStr) const
{
    return m_dailyReadingSeconds.value(dateStr, 0);
}

QString BookManager::getDailyReadingTimeStr(const QString &dateStr) const
{
    int secs = getDailyReadingSeconds(dateStr);
    return formatDuration(secs);
}

QVariantList BookManager::getCalendarDays(int year, int month)
{
    QVariantList list;
    QDate firstDay(year, month, 1);
    if (!firstDay.isValid()) {
        firstDay = QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1);
        year = firstDay.year();
        month = firstDay.month();
    }
    int dow = firstDay.dayOfWeek(); // 1 = Monday, 7 = Sunday
    // Start calendar grid at Monday preceding firstDay (or firstDay itself if dow == 1)
    QDate cur = firstDay.addDays(-(dow - 1));
    QDate today = QDate::currentDate();

    // 6 rows * 7 columns = 42 days
    for (int i = 0; i < 42; ++i) {
        QString dateStr = cur.toString("yyyy-MM-dd");
        int secs = m_dailyReadingSeconds.value(dateStr, 0);
        int mins = secs / 60;

        int level = 0;
        if (secs > 0) {
            if (mins < 15) {
                level = 1;
            } else if (mins < 45) {
                level = 2;
            } else if (mins < 90) {
                level = 3;
            } else {
                level = 4;
            }
        }

        QString durStr = (secs > 0) ? formatShortDuration(secs) : "";

        QVariantMap dayMap;
        dayMap["year"] = cur.year();
        dayMap["month"] = cur.month();
        dayMap["day"] = cur.day();
        dayMap["dateStr"] = dateStr;
        dayMap["isCurrentMonth"] = (cur.month() == month);
        dayMap["isToday"] = (cur == today);
        dayMap["seconds"] = secs;
        dayMap["minutes"] = mins;
        dayMap["durationStr"] = durStr;
        dayMap["hasReading"] = (secs > 0);
        dayMap["intensityLevel"] = level;

        list.append(dayMap);
        cur = cur.addDays(1);
    }

    return list;
}

QString BookManager::resolveStatsCoverPath(const QString &existingCover, const QString &filePath, const QString &format) const
{
    if (!existingCover.isEmpty() && !existingCover.startsWith("asset:///")) {
        QString localPath = existingCover;
        if (localPath.startsWith("file://")) {
            localPath = localPath.mid(7);
        }
        if (QFile::exists(localPath) && QFileInfo(localPath).size() > 0) {
            return localPath;
        }
    }

    if (!filePath.isEmpty() && !m_cacheDir.isEmpty()) {
        QString hash = QString(QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex());
        QString cached = findCachedCoverFile(m_cacheDir, hash);
        if (!cached.isEmpty()) {
            return cached;
        }
    }

    QString defFmt = format.toLower();
    if (defFmt.isEmpty() && !filePath.isEmpty()) {
        defFmt = QFileInfo(filePath).suffix().toLower();
    }
    if (defFmt == "azw" || defFmt == "azw3" || defFmt == "kf8" || defFmt == "prc") {
        defFmt = "mobi";
    } else if (defFmt == "djv") {
        defFmt = "djvu";
    }
    if (defFmt != "pdf" && defFmt != "epub" && defFmt != "mobi" && defFmt != "fb2" && defFmt != "cbz" && defFmt != "djvu" && defFmt != "txt") {
        defFmt = "epub";
    }
    return QString("asset:///icons/default_cover_%1.png").arg(defFmt);
}

QVariantMap BookManager::getWeekData(const QString &anchorDateStr)
{
    QVariantMap result;
    QDate anchor = QDate::fromString(anchorDateStr, "yyyy-MM-dd");
    if (!anchor.isValid()) anchor = QDate::currentDate();

    int dow = anchor.dayOfWeek(); // 1 = Monday, 7 = Sunday
    QDate monday = anchor.addDays(-(dow - 1));
    QDate sunday = monday.addDays(6);
    QDate today = QDate::currentDate();

    int totalSecs = 0;
    int readingDays = 0;
    QVariantList daysList;

    static const char *viDows[] = { "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy", "Chủ Nhật" };
    static const char *viShortDows[] = { "T2", "T3", "T4", "T5", "T6", "T7", "CN" };
    static const char *enDows[] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
    static const char *enShortDows[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

    for (int i = 0; i < 7; ++i) {
        QDate cur = monday.addDays(i);
        QString dStr = cur.toString("yyyy-MM-dd");
        int secs = m_dailyReadingSeconds.value(dStr, 0);
        if (secs > 0) {
            totalSecs += secs;
            readingDays++;
        }

        QVariantMap dayMap;
        dayMap["dateStr"] = dStr;
        dayMap["dayNumber"] = cur.day();
        dayMap["month"] = cur.month();
        dayMap["year"] = cur.year();
        dayMap["isToday"] = (cur == today);
        dayMap["seconds"] = secs;
        dayMap["durationStr"] = formatDuration(secs);
        dayMap["shortDurationStr"] = (secs > 0) ? formatShortDuration(secs) : "";
        dayMap["hasReading"] = (secs > 0);
        dayMap["shortDow"] = (m_language == "vi") ? viShortDows[i] : enShortDows[i];
        dayMap["fullDow"] = (m_language == "vi") ? viDows[i] : enDows[i];

        // Books read on this day
        dayMap["books"] = getDayBooks(dStr);

        daysList.append(dayMap);
    }

    result["startDate"] = monday.toString("yyyy-MM-dd");
    result["endDate"] = sunday.toString("yyyy-MM-dd");
    result["prevWeekDate"] = monday.addDays(-7).toString("yyyy-MM-dd");
    result["nextWeekDate"] = monday.addDays(7).toString("yyyy-MM-dd");
    result["isCurrentWeek"] = (today >= monday && today <= sunday);
    result["totalSeconds"] = totalSecs;
    result["totalDurationStr"] = formatDuration(totalSecs);
    result["shortTotalDurationStr"] = (totalSecs > 0) ? formatShortDuration(totalSecs) : formatDuration(0);
    result["readingDays"] = readingDays;
    result["days"] = daysList;

    int avgSecs = (readingDays > 0) ? (totalSecs / readingDays) : 0;
    result["avgDailySeconds"] = avgSecs;
    result["avgDailyTimeStr"] = formatDuration(avgSecs);
    result["shortAvgDailyTimeStr"] = (avgSecs > 0) ? formatShortDuration(avgSecs) : formatDuration(0);

    QString weekTitle;
    if (m_language == "vi") {
        weekTitle = QString("Tuần %1/%2 - %3/%4/%5")
            .arg(monday.day()).arg(monday.month())
            .arg(sunday.day()).arg(sunday.month()).arg(sunday.year());
    } else {
        weekTitle = QString("Week %1/%2 - %3/%4/%5")
            .arg(monday.day()).arg(monday.month())
            .arg(sunday.day()).arg(sunday.month()).arg(sunday.year());
    }
    result["weekTitle"] = weekTitle;

    return result;
}

void BookManager::setControlSpaceQuota(bb::cascades::Control *control, float quota)
{
    if (!control) return;
    bb::cascades::StackLayoutProperties *props = qobject_cast<bb::cascades::StackLayoutProperties*>(control->layoutProperties());
    if (props) {
        props->setSpaceQuota(quota);
    } else {
        bb::cascades::StackLayoutProperties *newProps = new bb::cascades::StackLayoutProperties();
        newProps->setSpaceQuota(quota);
        control->setLayoutProperties(newProps);
    }
}

QVariantMap BookManager::getMonthStats(int year, int month)
{
    QVariantMap stats;
    QDate firstDay(year, month, 1);
    if (!firstDay.isValid()) {
        firstDay = QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1);
        year = firstDay.year();
        month = firstDay.month();
    }
    int daysInMonth = firstDay.daysInMonth();
    int totalSecs = 0;
    int readingDays = 0;
    int maxStreak = 0;
    int curStreak = 0;
    int mostActiveSecs = 0;
    QString mostActiveDay = "";

    for (int d = 1; d <= daysInMonth; ++d) {
        QDate dayDate(year, month, d);
        QString dateStr = dayDate.toString("yyyy-MM-dd");
        int secs = m_dailyReadingSeconds.value(dateStr, 0);
        if (secs > 0) {
            totalSecs += secs;
            readingDays++;
            curStreak++;
            if (curStreak > maxStreak) {
                maxStreak = curStreak;
            }
            if (secs > mostActiveSecs) {
                mostActiveSecs = secs;
                mostActiveDay = dateStr;
            }
        } else {
            curStreak = 0;
        }
    }

    int avgSecs = (readingDays > 0) ? (totalSecs / readingDays) : 0;
    int completionRate = (daysInMonth > 0) ? (readingDays * 100 / daysInMonth) : 0;

    stats["year"] = year;
    stats["month"] = month;
    stats["daysInMonth"] = daysInMonth;
    stats["totalSeconds"] = totalSecs;
    stats["totalTimeStr"] = formatDuration(totalSecs);
    stats["shortTotalTimeStr"] = (totalSecs > 0) ? formatShortDuration(totalSecs) : formatDuration(0);
    stats["readingDays"] = readingDays;
    stats["completionRate"] = completionRate;
    stats["avgDailySeconds"] = avgSecs;
    stats["avgDailyTimeStr"] = formatDuration(avgSecs);
    stats["shortAvgDailyTimeStr"] = (avgSecs > 0) ? formatShortDuration(avgSecs) : formatDuration(0);
    stats["maxStreak"] = maxStreak;
    stats["mostActiveDay"] = mostActiveDay;
    stats["mostActiveSeconds"] = mostActiveSecs;
    stats["mostActiveTimeStr"] = formatDuration(mostActiveSecs);

    QString monthName;
    if (m_language == "vi") {
        monthName = QString("Tháng %1, %2").arg(month).arg(year);
    } else {
        static const char *enMonths[] = {
            "January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"
        };
        monthName = QString("%1 %2").arg(enMonths[month - 1]).arg(year);
    }
    stats["monthName"] = monthName;

    return stats;
}

QVariantMap BookManager::resolveBookMetaForStats(const QString &rawPath, QSettings &settings) const
{
    QVariantMap info;
    QString path = unwrapBookPath(rawPath);
    if (path.isEmpty() && rawPath.trimmed().isEmpty()) {
        return info;
    }
    if (path.isEmpty()) {
        path = rawPath.trimmed();
    }

    QStringList candidates;
    candidates.append(path);
    if (!path.startsWith('/')) {
        candidates.append("/" + path);
    } else if (path.length() > 1) {
        candidates.append(path.mid(1));
    }
    QString rawTrim = rawPath.trimmed();
    if (!rawTrim.isEmpty() && !candidates.contains(rawTrim)) {
        candidates.append(rawTrim);
    }

    QFileInfo pathFi(path);
    QString fname = pathFi.fileName();
    if (fname.contains('%')) {
        QString decFname = decodeQtAndPercentEscapes(fname);
        if (isValidDecodedText(decFname)) fname = decFname;
    }
    bool canMatchByFileName = !fname.isEmpty() && fname.contains('.') &&
                              isValidDecodedText(fname) &&
                              !fname.contains("accounts", Qt::CaseInsensitive);

    QVariantMap libBook;
    bool matched = false;
    QString resolvedPath = path;

    // 1. Exact path match against m_allBooks
    foreach (const QVariantMap &b, m_allBooks) {
        QString bPath = b.value("filePath").toString();
        if (candidates.contains(bPath)) {
            libBook = b;
            resolvedPath = bPath;
            matched = true;
            break;
        }
    }

    // 1b. Case-insensitive and NFC-normalized path match (handles NFD vs NFC Vietnamese on SD cards)
    if (!matched) {
        foreach (const QVariantMap &b, m_allBooks) {
            QString bPath = b.value("filePath").toString();
            QString bPathNfc = bPath.normalized(QString::NormalizationForm_C);
            for (int ci = 0; ci < candidates.size(); ++ci) {
                QString candNfc = candidates.at(ci).normalized(QString::NormalizationForm_C);
                if (bPath.compare(candidates.at(ci), Qt::CaseInsensitive) == 0 ||
                    bPathNfc.compare(candNfc, Qt::CaseInsensitive) == 0) {
                    libBook = b;
                    resolvedPath = bPath;
                    matched = true;
                    break;
                }
            }
            if (matched) break;
        }
    }

    // 1c. Filename match against m_allBooks (covers symlink / SD mount / legacy encoded paths)
    if (!matched && canMatchByFileName) {
        QString fnameNfc = fname.normalized(QString::NormalizationForm_C);
        foreach (const QVariantMap &b, m_allBooks) {
            QString bPath = b.value("filePath").toString();
            QString bFnameNfc = QFileInfo(bPath).fileName().normalized(QString::NormalizationForm_C);
            if (bFnameNfc.compare(fnameNfc, Qt::CaseInsensitive) == 0) {
                libBook = b;
                resolvedPath = bPath;
                matched = true;
                break;
            }
        }
    }

    if (!resolvedPath.isEmpty() && !candidates.contains(resolvedPath)) {
        candidates.prepend(resolvedPath);
    }
    if (resolvedPath.startsWith('/') && resolvedPath.length() > 1 && !candidates.contains(resolvedPath.mid(1))) {
        candidates.append(resolvedPath.mid(1));
    }

    QString title;
    QString author;
    QString cover;
    QString format;
    int progress = 0;
    int totalPages = 0;
    int currentChapterIdx = -1;

    if (matched) {
        title = libBook.value("title").toString();
        author = libBook.value("author").toString();
        cover = libBook.value("cover").toString();
        format = libBook.value("format").toString();
        progress = libBook.value("progress", 0).toInt();
        totalPages = libBook.value("totalPages", 0).toInt();
        if (totalPages == 0) totalPages = libBook.value("totalChapters", 0).toInt();
        if (libBook.contains("currentPage")) currentChapterIdx = libBook.value("currentPage").toInt();
        else if (libBook.contains("currentChapter")) currentChapterIdx = libBook.value("currentChapter").toInt();
    }

    // 2. Override / match live active current book
    if (!m_currentFilePath.isEmpty()) {
        bool matchesCurrent = candidates.contains(m_currentFilePath) ||
                              (canMatchByFileName && QFileInfo(m_currentFilePath).fileName().compare(fname, Qt::CaseInsensitive) == 0);
        if (matchesCurrent) {
            resolvedPath = m_currentFilePath;
            if (!m_currentBookTitle.isEmpty()) title = m_currentBookTitle;
            if (!m_currentBookAuthor.isEmpty()) author = m_currentBookAuthor;
            if (!m_currentCoverPath.isEmpty()) cover = m_currentCoverPath;
            if (!m_currentFormat.isEmpty()) format = m_currentFormat;
            progress = readingProgress();
            if (m_totalChapters > 0) {
                totalPages = m_totalChapters;
                currentChapterIdx = m_currentChapterIndex;
            }
            matched = true;
        }
    }

    // 3. Fallback / match last opened book
    if (!m_lastBookPath.isEmpty()) {
        bool matchesLast = candidates.contains(m_lastBookPath) ||
                           (canMatchByFileName && QFileInfo(m_lastBookPath).fileName().compare(fname, Qt::CaseInsensitive) == 0);
        if (matchesLast) {
            if (!matched) resolvedPath = m_lastBookPath;
            if (title.isEmpty() || looksLikeGarbledTitle(title)) {
                if (!m_lastBookTitle.isEmpty()) title = m_lastBookTitle;
            }
            if (author.isEmpty() || looksLikeGarbledTitle(author)) {
                if (!m_lastBookAuthor.isEmpty()) author = m_lastBookAuthor;
            }
            if (cover.isEmpty()) cover = m_lastBookCover;
            if (format.isEmpty()) format = m_lastBookFormat;
            if (progress <= 0) progress = m_lastBookProgress;
            matched = true;
        }
    }

    // 4. Lookup cached metadata and progress in QSettings across candidate path keys
    for (int ci = 0; ci < candidates.size(); ++ci) {
        const QString &cand = candidates.at(ci);
        if (cand.isEmpty()) continue;

        if (title.isEmpty() || looksLikeGarbledTitle(title)) {
            QString sTitle = settings.value(QString("meta/title/%1").arg(cand),
                                            settings.value(QString("title/%1").arg(cand))).toString();
            if (sTitle.contains('%')) {
                QString decTitle = decodeQtAndPercentEscapes(sTitle);
                if (isValidDecodedText(decTitle)) sTitle = decTitle;
            }
            if (!sTitle.isEmpty() && !looksLikeGarbledTitle(sTitle)) {
                title = sTitle;
            }
        }
        if (author.isEmpty() || looksLikeGarbledTitle(author)) {
            QString sAuthor = settings.value(QString("meta/author/%1").arg(cand)).toString();
            if (!sAuthor.isEmpty() && !looksLikeGarbledTitle(sAuthor)) {
                author = sAuthor;
            }
        }
        if (progress <= 0) {
            progress = settings.value(QString("progressPct/%1").arg(cand), 0).toInt();
        }
        if (totalPages <= 0) {
            totalPages = settings.value(QString("totalPages/%1").arg(cand), 0).toInt();
            if (totalPages <= 0) {
                totalPages = settings.value(QString("totalChapters/%1").arg(cand), 0).toInt();
            }
        }
        if (currentChapterIdx < 0 && settings.contains(QString("progress/%1").arg(cand))) {
            currentChapterIdx = settings.value(QString("progress/%1").arg(cand), 0).toInt();
        }
    }

    // 5. Clean filename fallback (never expose raw Base64, hashes, paths, or broken UTF-8 bytes)
    if (title.isEmpty() || looksLikeGarbledTitle(title)) {
        QString cleanForName = resolvedPath;
        while (cleanForName.endsWith('/')) cleanForName.chop(1);
        QFileInfo fi(cleanForName);
        QString candidateName = fi.completeBaseName();
        if (candidateName.isEmpty() || candidateName.contains("accounts", Qt::CaseInsensitive) || !isValidDecodedText(candidateName)) {
            int lastSlash = cleanForName.lastIndexOf('/');
            if (lastSlash >= 0) {
                candidateName = cleanForName.mid(lastSlash + 1);
                int dot = candidateName.lastIndexOf('.');
                if (dot > 0) candidateName = candidateName.left(dot);
            }
        }
        if (candidateName.contains('%')) {
            QString decName = decodeQtAndPercentEscapes(candidateName);
            if (isValidDecodedText(decName)) candidateName = decName;
        }
        if (candidateName.endsWith(".kepub", Qt::CaseInsensitive)) {
            candidateName.chop(6);
        }
        candidateName = candidateName.replace('_', ' ').simplified();
        if (candidateName.isEmpty() || looksLikeGarbledTitle(candidateName)) {
            title = (m_language == "vi" ? tr("Tài liệu đã đọc") : QString("Document Read"));
        } else {
            title = candidateName;
        }
    }

    if (author.isEmpty() || looksLikeGarbledTitle(author)) {
        author = (m_language == "vi" ? "Chưa rõ tác giả" : "Unknown Author");
    }

    format = sanitizeStatsFormat(format, resolvedPath);
    cover = resolveStatsCoverPath(cover, resolvedPath, format);

    bool isPagedFmt = (format == "pdf" || format == "djvu" || format == "djv");
    int currentPage = 0;
    if (totalPages > 0) {
        if (currentChapterIdx >= 0) {
            currentPage = isPagedFmt ? qBound(1, currentChapterIdx, totalPages) : qBound(1, currentChapterIdx + 1, totalPages);
        } else if (progress > 0) {
            currentPage = qMax(1, qRound((double)totalPages * (double)progress / 100.0));
        }
    } else if (currentChapterIdx > 0) {
        currentPage = isPagedFmt ? currentChapterIdx : (currentChapterIdx + 1);
    }

    info["filePath"] = resolvedPath;
    info["title"] = title;
    info["author"] = author;
    info["cover"] = cover;
    info["format"] = format;
    info["formatBadge"] = format.toUpper();
    info["progress"] = progress;
    info["progressStr"] = QString("%1%").arg(progress);
    info["currentPage"] = currentPage;
    info["totalPages"] = totalPages;
    info["isPagedFmt"] = isPagedFmt;
    return info;
}

QVariantMap BookManager::getBookMetadata(const QString &filePath)
{
    QVariantMap res;
    if (filePath.trimmed().isEmpty()) return res;

    QFileInfo fi(filePath);
    QString absPath = fi.exists() ? fi.absoluteFilePath() : filePath;

    // 1. Fast path if this is the currently loaded book in reader
    if (!m_currentFilePath.isEmpty()) {
        QString curAbs = QFileInfo(m_currentFilePath).absoluteFilePath();
        if (absPath == curAbs || filePath == m_currentFilePath) {
            res["description"] = m_currentBookDescription;
            res["genre"] = m_currentBookGenre;
            res["series"] = m_currentBookSeries;
            res["seriesIndex"] = m_currentBookSeriesIndex;
            res["title"] = m_currentBookTitle;
            res["author"] = m_currentBookAuthor;
            return res;
        }
    }

    QSettings settings("Libris", "Libris");
    QString descKey = QString("meta/desc_v2/%1").arg(absPath);
    QString genreKey = QString("meta/genre/%1").arg(absPath);
    QString loadedKey = QString("meta/desc_loaded_v2/%1").arg(absPath);

    if (settings.value(loadedKey, false).toBool()) {
        res["description"] = settings.value(descKey).toString();
        res["genre"] = settings.value(genreKey).toString();
        res["series"] = settings.value(QString("series/%1").arg(absPath)).toString();
        res["seriesIndex"] = settings.value(QString("seriesIdx/%1").arg(absPath),
                                            settings.value(QString("seriesIndex/%1").arg(absPath), 0)).toInt();
        return res;
    }

    // 2. Parse metadata on demand from file
    if (!fi.exists() || !fi.isFile()) return res;

    QString ext = fi.suffix().toLower();
    QString desc, genre, series, title, author;
    int seriesIdx = 0;

    if (ext == "epub") {
        EpubParser ep;
        if (ep.open(absPath)) {
            desc = ep.description();
            genre = ep.genre();
            series = ep.series();
            seriesIdx = ep.seriesIndex();
            title = ep.title();
            author = ep.author();
            ep.close();
        }
    } else if (ext == "fb2") {
        Fb2Parser fb;
        if (fb.open(absPath)) {
            desc = fb.description();
            genre = fb.genre();
            series = fb.series();
            seriesIdx = fb.seriesIndex();
            title = fb.title();
            author = fb.author();
            fb.close();
        }
    } else if (ext == "cbz") {
        CbzParser cp;
        if (cp.open(absPath)) {
            desc = cp.description();
            genre = cp.genre();
            series = cp.series();
            seriesIdx = cp.seriesIndex();
            title = cp.title();
            author = cp.author();
            cp.close();
        }
    } else if (ext == "mobi" || ext == "azw" || ext == "azw3" || ext == "kf8" || ext == "prc") {
        MobiParser mp;
        if (mp.open(absPath)) {
            desc = mp.description();
            genre = mp.genre();
            series = mp.series();
            seriesIdx = mp.seriesIndex();
            title = mp.title();
            author = mp.author();
            mp.close();
        }
    }

    settings.setValue(loadedKey, true);
    if (!desc.isEmpty()) settings.setValue(descKey, desc);
    if (!genre.isEmpty()) settings.setValue(genreKey, genre);
    if (!series.isEmpty()) {
        settings.setValue(QString("series/%1").arg(absPath), series);
        settings.setValue(QString("seriesIdx/%1").arg(absPath), seriesIdx);
    }

    res["description"] = desc;
    res["genre"] = genre;
    res["series"] = series;
    res["seriesIndex"] = seriesIdx;
    res["title"] = title;
    res["author"] = author;
    return res;
}

QVariantList BookManager::getDayBooks(const QString &dateStr)
{
    QVariantList result;
    if (dateStr.isEmpty()) return result;

    QMap<QString, int> booksMap = m_dailyBookReadingSeconds.value(dateStr);

    // Only attribute unsplit legacy daily totals if NO per-book tracking exists at all and date is today
    if (booksMap.isEmpty() && m_dailyReadingSeconds.value(dateStr, 0) > 0 && dateStr == QDate::currentDate().toString("yyyy-MM-dd")) {
        if (!m_currentFilePath.isEmpty()) {
            booksMap[m_currentFilePath] = m_dailyReadingSeconds.value(dateStr, 0);
        } else if (!m_lastBookPath.isEmpty()) {
            booksMap[m_lastBookPath] = m_dailyReadingSeconds.value(dateStr, 0);
        }
    }

    QSettings settings("Libris", "Libris");
    QMap<QString, QVariantMap> mergedByCanonical;
    QMapIterator<QString, int> it(booksMap);
    while (it.hasNext()) {
        it.next();
        int secs = it.value();
        if (secs <= 0) continue;

        QVariantMap bookInfo = resolveBookMetaForStats(it.key(), settings);
        QString canonPath = bookInfo.value("filePath").toString();
        if (canonPath.isEmpty()) continue;

        if (mergedByCanonical.contains(canonPath)) {
            int prevSecs = mergedByCanonical[canonPath].value("daySeconds").toInt();
            if (secs > prevSecs) {
                mergedByCanonical[canonPath]["daySeconds"] = secs;
                mergedByCanonical[canonPath]["dayDurationStr"] = formatDuration(secs);
                mergedByCanonical[canonPath]["shortDayDurationStr"] = formatShortDuration(secs);
            }
            continue;
        }

        int totalPages = bookInfo.value("totalPages", 0).toInt();
        int curPage = bookInfo.value("currentPage", 0).toInt();
        bool isPagedFmt = bookInfo.value("isPagedFmt", false).toBool();

        QString pageCountStr;
        if (totalPages > 0) {
            QString unitStr = isPagedFmt ? (m_language == "vi" ? "trang" : "pg") : (m_language == "vi" ? "chương" : "ch");
            if (curPage > 0) {
                pageCountStr = QString("%1/%2 %3").arg(curPage).arg(totalPages).arg(unitStr);
            } else {
                pageCountStr = QString("%1 %2").arg(totalPages).arg(unitStr);
            }
        }

        bookInfo["daySeconds"] = secs;
        bookInfo["dayDurationStr"] = formatDuration(secs);
        bookInfo["shortDayDurationStr"] = formatShortDuration(secs);
        bookInfo["pageCountStr"] = pageCountStr;

        mergedByCanonical.insert(canonPath, bookInfo);
    }

    QMapIterator<QString, QVariantMap> mIt(mergedByCanonical);
    while (mIt.hasNext()) {
        mIt.next();
        result.append(mIt.value());
    }

    // Sort by daySeconds descending
    for (int i = 0; i < result.size() - 1; ++i) {
        for (int j = i + 1; j < result.size(); ++j) {
            if (result[i].toMap().value("daySeconds").toInt() < result[j].toMap().value("daySeconds").toInt()) {
                QVariant tmp = result[i];
                result[i] = result[j];
                result[j] = tmp;
            }
        }
    }

    return result;
}

QVariantMap BookManager::getOverallStats()
{
    QVariantMap overall;
    int totalSecs = m_totalReadingSeconds;
    int totalDays = 0;
    int bestDaySecs = 0;
    QString bestDayDate = "";

    QList<QDate> activeDates;
    QMapIterator<QString, int> it(m_dailyReadingSeconds);
    while (it.hasNext()) {
        it.next();
        if (it.value() > 0) {
            totalDays++;
            if (it.value() > bestDaySecs) {
                bestDaySecs = it.value();
                bestDayDate = it.key();
            }
            QDate d = QDate::fromString(it.key(), "yyyy-MM-dd");
            if (d.isValid()) {
                activeDates.append(d);
            }
        }
    }

    qSort(activeDates.begin(), activeDates.end());

    int maxStreak = 0;
    int curStreakRun = 0;
    for (int i = 0; i < activeDates.size(); ++i) {
        if (i == 0) {
            curStreakRun = 1;
        } else {
            if (activeDates[i] == activeDates[i - 1].addDays(1)) {
                curStreakRun++;
            } else if (activeDates[i] != activeDates[i - 1]) {
                curStreakRun = 1;
            }
        }
        if (curStreakRun > maxStreak) {
            maxStreak = curStreakRun;
        }
    }

    int currentStreak = 0;
    QDate today = QDate::currentDate();
    QDate checkDate = today;
    if (m_dailyReadingSeconds.value(today.toString("yyyy-MM-dd"), 0) > 0) {
        while (m_dailyReadingSeconds.value(checkDate.toString("yyyy-MM-dd"), 0) > 0) {
            currentStreak++;
            checkDate = checkDate.addDays(-1);
        }
    } else {
        checkDate = today.addDays(-1);
        while (m_dailyReadingSeconds.value(checkDate.toString("yyyy-MM-dd"), 0) > 0) {
            currentStreak++;
            checkDate = checkDate.addDays(-1);
        }
    }

    overall["totalReadingSeconds"] = totalSecs;
    overall["totalReadingTimeStr"] = formatDuration(totalSecs);
    overall["shortTotalReadingTimeStr"] = (totalSecs > 0) ? formatShortDuration(totalSecs) : formatDuration(0);
    overall["totalReadingDays"] = totalDays;
    overall["allTimeStreak"] = maxStreak;
    overall["currentStreak"] = currentStreak;
    overall["bestDayDate"] = bestDayDate;
    overall["bestDaySeconds"] = bestDaySecs;
    overall["bestDayTimeStr"] = formatDuration(bestDaySecs);
    overall["shortBestDayTimeStr"] = (bestDaySecs > 0) ? formatShortDuration(bestDaySecs) : "";
    overall["totalBooks"] = totalLibraryBooks();
    overall["completedBooks"] = completedBooksCount();
    overall["readingBooks"] = readingBooksCount();

    return overall;
}

QVariantMap BookManager::getBookDetailedStats(const QString &filePath)
{
    QVariantMap stats;
    if (filePath.isEmpty()) return stats;

    QSettings settings("Libris", "Libris");
    QVariantMap meta = resolveBookMetaForStats(filePath, settings);
    QString resolvedPath = meta.value("filePath").toString();
    if (resolvedPath.isEmpty()) resolvedPath = unwrapBookPath(filePath);
    if (resolvedPath.isEmpty()) resolvedPath = filePath;

    QString format = meta.value("format").toString();
    bool isPagedFmt = meta.value("isPagedFmt", false).toBool();
    int progress = meta.value("progress", 0).toInt();
    int currentPage = meta.value("currentPage", 0).toInt();
    int totalPages = meta.value("totalPages", 0).toInt();

    int totalSecs = qMax(m_bookReadingSeconds.value(filePath, 0),
                         m_bookReadingSeconds.value(resolvedPath, 0));
    if (totalSecs == 0) {
        QMapIterator<QString, QMap<QString, int> > it(m_dailyBookReadingSeconds);
        while (it.hasNext()) {
            it.next();
            int daySecs = it.value().value(filePath, 0);
            if (resolvedPath != filePath) {
                daySecs = qMax(daySecs, it.value().value(resolvedPath, 0));
            }
            totalSecs += daySecs;
        }
        if (totalSecs > 0) {
            m_bookReadingSeconds[resolvedPath] = totalSecs;
        }
    }

    stats["filePath"] = resolvedPath;
    stats["title"] = meta.value("title").toString();
    stats["author"] = meta.value("author").toString();
    stats["cover"] = meta.value("cover").toString();
    stats["format"] = format;
    stats["formatBadge"] = format.toUpper();
    stats["isPagedFmt"] = isPagedFmt;
    stats["progress"] = progress;
    stats["progressStr"] = QString("%1%").arg(progress);
    stats["currentPage"] = currentPage;
    stats["totalPages"] = totalPages;

    QString unitFull = isPagedFmt ? (m_language == "vi" ? "trang" : "pages") : (m_language == "vi" ? "chương" : "ch");
    stats["pageStr"] = (totalPages > 0) ? QString("%1 / %2 %3").arg(currentPage).arg(totalPages).arg(unitFull)
                                        : (progress > 0 ? QString("%1%").arg(progress) : (m_language == "vi" ? "Chưa mở" : "Not opened"));
    stats["totalSeconds"] = totalSecs;
    stats["totalTimeStr"] = formatDuration(totalSecs);
    stats["shortTotalTimeStr"] = (totalSecs > 0) ? formatShortDuration(totalSecs) : formatDuration(0);

    // Reading Speed (distinguish paged PDF/DjVu vs reflowable chapters with sanity thresholds)
    QString speedStr = m_language == "vi" ? "Đang hiệu chỉnh" : "Calibrating...";
    bool validSpeedSample = false;
    if (isPagedFmt) {
        if (currentPage > 0 && totalSecs >= 120) {
            double secsPerPage = (double)totalSecs / (double)currentPage;
            if (secsPerPage >= 5.0) {
                validSpeedSample = true;
                if (secsPerPage >= 60.0) {
                    speedStr = QString("~%1 %2").arg(QString::number(secsPerPage / 60.0, 'f', 1), m_language == "vi" ? "phút/trang" : "min/pg");
                } else {
                    speedStr = QString("~%1 %2").arg(QString::number((int)secsPerPage), m_language == "vi" ? "giây/trang" : "sec/pg");
                }
            }
        }
    } else {
        if (progress > 0 && totalSecs >= 300) {
            double pctPerHour = ((double)progress / (double)totalSecs) * 3600.0;
            if (pctPerHour <= 60.0) {
                validSpeedSample = true;
                speedStr = QString("~%1% / %2").arg(QString::number(qMax(1, qRound(pctPerHour)))).arg(m_language == "vi" ? "giờ" : "hr");
            }
        }
    }
    stats["readingSpeedStr"] = speedStr;

    // Remaining time estimation
    QString remainingStr;
    if (progress >= 100) {
        remainingStr = m_language == "vi" ? "Đã đọc xong" : "Completed";
    } else if (progress > 0 && validSpeedSample) {
        int remSecs = (int)((double)totalSecs * (100.0 - progress) / (double)progress);
        remainingStr = QString("~%1").arg(formatDuration(remSecs));
    } else {
        remainingStr = m_language == "vi" ? "Đang tính..." : "Estimating...";
    }
    stats["remainingTimeStr"] = remainingStr;

    return stats;
}

QVariantList BookManager::getBookStatsList()
{
    QVariantList list;
    QSet<QString> seenPaths;

    // Pin currently active or last book at top if tracked
    if (!m_currentFilePath.isEmpty()) {
        QVariantMap curStats = getBookDetailedStats(m_currentFilePath);
        list.append(curStats);
        seenPaths.insert(m_currentFilePath);
        seenPaths.insert(curStats.value("filePath").toString());
    } else if (!m_lastBookPath.isEmpty()) {
        QVariantMap lastStats = getBookDetailedStats(m_lastBookPath);
        list.append(lastStats);
        seenPaths.insert(m_lastBookPath);
        seenPaths.insert(lastStats.value("filePath").toString());
    }

    // Collect remaining books sorted by reading seconds descending
    QList<QVariantMap> rest;
    QMapIterator<QString, int> it(m_bookReadingSeconds);
    while (it.hasNext()) {
        it.next();
        if (it.value() > 0 && !seenPaths.contains(it.key())) {
            QVariantMap bs = getBookDetailedStats(it.key());
            QString canonPath = bs.value("filePath").toString();
            seenPaths.insert(it.key());
            if (!seenPaths.contains(canonPath)) {
                rest.append(bs);
                seenPaths.insert(canonPath);
            }
        }
    }
    for (int i = 0; i < m_allBooks.size(); ++i) {
        QString path = m_allBooks[i].value("filePath").toString();
        if (!seenPaths.contains(path) && (m_allBooks[i].value("progress", 0).toInt() > 0 || m_allBooks[i].value("isOpened", false).toBool())) {
            QVariantMap bs = getBookDetailedStats(path);
            QString canonPath = bs.value("filePath").toString();
            seenPaths.insert(path);
            if (!seenPaths.contains(canonPath)) {
                rest.append(bs);
                seenPaths.insert(canonPath);
            }
        }
    }

    struct BookStatsCmp {
        static bool lessThan(const QVariantMap &a, const QVariantMap &b) {
            int sA = a.value("totalSeconds").toInt();
            int sB = b.value("totalSeconds").toInt();
            if (sA != sB) return sA > sB;
            return a.value("progress").toInt() > b.value("progress").toInt();
        }
    };
    qSort(rest.begin(), rest.end(), BookStatsCmp::lessThan);
    for (int i = 0; i < rest.size(); ++i) {
        list.append(rest[i]);
    }

    return list;
}

QVariantList BookManager::getRecentDaysTrend(int numDays)
{
    QVariantList list;
    if (numDays <= 0) numDays = 7;

    int maxSecs = qMax(600, m_dailyGoalMinutes * 60);

    QList<QVariantMap> rawList;
    for (int i = numDays - 1; i >= 0; --i) {
        QDate d = QDate::currentDate().addDays(-i);
        QString dateStr = d.toString("yyyy-MM-dd");
        int secs = m_dailyReadingSeconds.value(dateStr, 0);
        if (secs > maxSecs) {
            maxSecs = secs;
        }

        QString dayName;
        int dow = d.dayOfWeek(); // 1 = Mon, 7 = Sun
        if (m_language == "vi") {
            static const char* viDays[] = {"", "T2", "T3", "T4", "T5", "T6", "T7", "CN"};
            dayName = viDays[dow];
        } else {
            static const char* enDays[] = {"", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
            dayName = enDays[dow];
        }

        QVariantMap dayMap;
        dayMap["dateStr"] = dateStr;
        dayMap["dayName"] = dayName;
        dayMap["dayNum"] = QString::number(d.day());
        dayMap["seconds"] = secs;
        dayMap["durationStr"] = formatDuration(secs);
        dayMap["shortDurationStr"] = formatShortDuration(secs);
        dayMap["isToday"] = (i == 0);
        rawList.append(dayMap);
    }

    for (int i = 0; i < rawList.size(); ++i) {
        QVariantMap m = rawList[i];
        int secs = m.value("seconds").toInt();
        double ratio = (double)secs / (double)maxSecs;
        if (secs == 0) {
            m["heightRatio"] = 0.05;
        } else {
            m["heightRatio"] = qBound(0.12, ratio, 1.0);
        }
        list.append(m);
    }

    return list;
}

QVariantMap BookManager::getFormatDistribution()
{
    QVariantMap dist;
    int epubSecs = 0, pdfSecs = 0, txtSecs = 0, fb2Secs = 0, cbzSecs = 0, mobiSecs = 0;
    int epubBooks = 0, pdfBooks = 0, txtBooks = 0, fb2Books = 0, cbzBooks = 0, mobiBooks = 0;
    QSet<QString> countedPaths;

    for (int i = 0; i < m_allBooks.size(); ++i) {
        QString path = m_allBooks[i].value("filePath").toString();
        QString fmt = m_allBooks[i].value("format").toString().toLower();
        if (fmt.isEmpty()) fmt = QFileInfo(path).suffix().toLower();
        int secs = m_bookReadingSeconds.value(path, 0);
        countedPaths.insert(path);

        if (fmt == "epub") { epubSecs += secs; epubBooks++; }
        else if (fmt == "pdf" || fmt == "djvu" || fmt == "djv") { pdfSecs += secs; pdfBooks++; }
        else if (fmt == "txt") { txtSecs += secs; txtBooks++; }
        else if (fmt == "fb2") { fb2Secs += secs; fb2Books++; }
        else if (fmt == "cbz") { cbzSecs += secs; cbzBooks++; }
        else if (fmt == "mobi" || fmt == "azw" || fmt == "azw3" || fmt == "kf8" || fmt == "prc") { mobiSecs += secs; mobiBooks++; }
    }

    QMapIterator<QString, int> it(m_bookReadingSeconds);
    while (it.hasNext()) {
        it.next();
        QString path = it.key();
        if (countedPaths.contains(path) || it.value() <= 0) continue;
        QString fmt = QFileInfo(path).suffix().toLower();
        int secs = it.value();
        if (fmt == "epub") { epubSecs += secs; epubBooks++; }
        else if (fmt == "pdf" || fmt == "djvu" || fmt == "djv") { pdfSecs += secs; pdfBooks++; }
        else if (fmt == "txt") { txtSecs += secs; txtBooks++; }
        else if (fmt == "fb2") { fb2Secs += secs; fb2Books++; }
        else if (fmt == "cbz") { cbzSecs += secs; cbzBooks++; }
        else if (fmt == "mobi" || fmt == "azw" || fmt == "azw3" || fmt == "kf8" || fmt == "prc") { mobiSecs += secs; mobiBooks++; }
    }

    int totalSecs = epubSecs + pdfSecs + txtSecs + fb2Secs + cbzSecs + mobiSecs;
    if (totalSecs == 0) totalSecs = 1;

    dist["epubSeconds"] = epubSecs;
    dist["epubTimeStr"] = formatDuration(epubSecs);
    dist["epubBooks"] = epubBooks;
    dist["epubPct"] = (epubSecs * 100) / totalSecs;

    dist["pdfSeconds"] = pdfSecs;
    dist["pdfTimeStr"] = formatDuration(pdfSecs);
    dist["pdfBooks"] = pdfBooks;
    dist["pdfPct"] = (pdfSecs * 100) / totalSecs;

    dist["txtSeconds"] = txtSecs;
    dist["txtTimeStr"] = formatDuration(txtSecs);
    dist["txtBooks"] = txtBooks;
    dist["txtPct"] = (txtSecs * 100) / totalSecs;

    dist["fb2Seconds"] = fb2Secs;
    dist["fb2TimeStr"] = formatDuration(fb2Secs);
    dist["fb2Books"] = fb2Books;
    dist["fb2Pct"] = (fb2Secs * 100) / totalSecs;

    dist["cbzSeconds"] = cbzSecs;
    dist["cbzTimeStr"] = formatDuration(cbzSecs);
    dist["cbzBooks"] = cbzBooks;
    dist["cbzPct"] = (cbzSecs * 100) / totalSecs;

    dist["mobiSeconds"] = mobiSecs;
    dist["mobiTimeStr"] = formatDuration(mobiSecs);
    dist["mobiBooks"] = mobiBooks;
    dist["mobiPct"] = (mobiSecs * 100) / totalSecs;

    return dist;
}

bool BookManager::isReaderActive() const
{
    return m_isReaderActive;
}

void BookManager::setIsReaderActive(bool active)
{
    if (m_isReaderActive != active) {
        m_isReaderActive = active;
        emit isReaderActiveChanged();

        if (m_isReaderActive) {
            qDebug() << "[BookManager] Reader is ACTIVE -> starting timers for:" << m_currentFilePath;
            if (!m_isAppSleeping) {
                if (!m_readingHeartbeatTimer.isActive()) {
                    m_readingHeartbeatTimer.start(10000);
                }
                if (!m_clockTimer.isActive()) {
                    m_clockTimer.start(30000);
                }
                emit currentTimeChanged();
                emit batteryLevelChanged();
            }
        } else {
            qDebug() << "[BookManager] Reader is INACTIVE -> stopping timers. Last book was:" << m_currentFilePath;
            m_readingHeartbeatTimer.stop();
            m_clockTimer.stop();
            saveCurrentProgress();
            saveSettings();
            updateReadingHistoryModel();
            emit statsChanged();
            emit dailyStatsChanged();
        }
        updateVolumeKeyWatchers();
    }
}

void BookManager::onReadingHeartbeat()
{
    if (!m_isReaderActive || m_currentFilePath.isEmpty() || m_isLoadingBook) {
        return;
    }
    m_totalReadingSeconds += 10;

    QString curDate = QDate::currentDate().toString("yyyy-MM-dd");
    if (m_todayDate != curDate) {
        m_todayDate = curDate;
        m_todayReadingSeconds = 0;
    }
    m_todayReadingSeconds += 10;

    int curBookSecs = m_bookReadingSeconds.value(m_currentFilePath, 0) + 10;
    m_bookReadingSeconds[m_currentFilePath] = curBookSecs;

    m_dailyReadingSeconds[curDate] = m_dailyReadingSeconds.value(curDate, 0) + 10;
    m_dailyBookReadingSeconds[curDate][m_currentFilePath] = m_dailyBookReadingSeconds[curDate].value(m_currentFilePath, 0) + 10;

    if (m_totalReadingSeconds % 60 == 0) {
        saveSettings();
        updateReadingHistoryModel();
    }
    m_sessionReadingSeconds += 10;
    if (m_eyeRestMinutes > 0 && m_sessionReadingSeconds > 0 && (m_sessionReadingSeconds % (m_eyeRestMinutes * 60) == 0)) {
        emit eyeRestReminder();
    }
    emit statsChanged();
    emit dailyStatsChanged();
}

void BookManager::updateReadingHistoryModel()
{
    if (!m_readingHistoryModel) return;
    m_readingHistoryModel->clear();

    QString q = m_recentSearchQuery.trimmed().toLower();

    QSettings settings("Libris", "Libris");
    bool lastBookIncluded = false;
    foreach (const QVariantMap &b, m_allBooks) {
        QString path = b.value("filePath").toString();
        int prog = b.value("progress").toInt();
        if (prog <= 0) {
            prog = settings.value(QString("progressPct/%1").arg(path), 0).toInt();
        }
        qlonglong lastRead = settings.value(QString("lastReadTime/%1").arg(path), 0).toLongLong();
        if (path == m_lastBookPath && lastRead == 0) {
            lastRead = 1;
        }

        if (lastRead > 0 || path == m_lastBookPath) {
            if (path == m_lastBookPath) {
                lastBookIncluded = true;
            }
            int bookSecs = m_bookReadingSeconds.value(path, 0);
            if (bookSecs <= 0) {
                bookSecs = settings.value(QString("stats/bookSeconds/%1").arg(path), 0).toInt();
            }

            QVariantMap item = b;
            if (path == m_lastBookPath) {
                if (!m_lastBookTitle.isEmpty()) {
                    item["title"] = m_lastBookTitle;
                }
                if (!m_lastBookAuthor.isEmpty()) {
                    item["author"] = m_lastBookAuthor;
                }
                if (!m_lastBookCover.isEmpty()) {
                    item["cover"] = m_lastBookCover;
                }
            } else {
                QString metaTitle = settings.value(QString("meta/title/%1").arg(path)).toString();
                QString metaAuthor = settings.value(QString("meta/author/%1").arg(path)).toString();
                if (!metaTitle.isEmpty()) item["title"] = metaTitle;
                if (!metaAuthor.isEmpty()) item["author"] = metaAuthor;
            }

            // Search filter by title, author, or filename
            if (!q.isEmpty()) {
                QString title = item.value("title").toString().toLower();
                QString author = item.value("author").toString().toLower();
                QString fileName = QFileInfo(path).fileName().toLower();
                if (!title.contains(q) && !author.contains(q) && !fileName.contains(q)) {
                    continue;
                }
            }

            item["progress"] = prog;
            item["progressStr"] = QString("%1%").arg(prog);
            item["timeSpentStr"] = formatDuration(bookSecs);
            item["status"] = prog >= 95 ? "completed" : "reading";
            item["statusText"] = prog >= 95 ? tr("Đã đọc xong") : (tr("Đang đọc ") + QString("%1%").arg(prog));

            // Sort key: strictly most recently read first (descending timestamp mapped to ascending key)
            if (path == m_lastBookPath) {
                item["sortKey"] = QString("0000000000000000_%1").arg(item.value("title").toString());
            } else {
                qlonglong invKey = 999999999999999LL - lastRead;
                item["sortKey"] = QString("%1_%2").arg(invKey, 16, 10, QChar('0')).arg(item.value("title").toString());
            }

            m_readingHistoryModel->insert(item);
        }
    }

    if (!m_lastBookPath.isEmpty() && !lastBookIncluded && QFile::exists(m_lastBookPath)) {
        QString title = !m_lastBookTitle.isEmpty() ? m_lastBookTitle : QFileInfo(m_lastBookPath).completeBaseName().replace('_', ' ');
        QString author = m_lastBookAuthor;
        QString fileName = QFileInfo(m_lastBookPath).fileName();

        bool matches = true;
        if (!q.isEmpty()) {
            if (!title.toLower().contains(q) && !author.toLower().contains(q) && !fileName.toLower().contains(q)) {
                matches = false;
            }
        }

        if (matches) {
            QVariantMap lastItem;
            lastItem["filePath"] = m_lastBookPath;
            lastItem["path"] = m_lastBookPath;
            lastItem["title"] = title;
            lastItem["author"] = author;
            lastItem["cover"] = m_lastBookCover;
            lastItem["format"] = m_lastBookFormat;
            lastItem["progress"] = m_lastBookProgress;
            lastItem["progressStr"] = QString("%1%").arg(m_lastBookProgress);
            int bookSecs = m_bookReadingSeconds.value(m_lastBookPath, 0);
            if (bookSecs <= 0) {
                bookSecs = settings.value(QString("stats/bookSeconds/%1").arg(m_lastBookPath), 0).toInt();
            }
            lastItem["timeSpentStr"] = formatDuration(bookSecs);
            lastItem["status"] = m_lastBookProgress >= 95 ? "completed" : "reading";
            lastItem["statusText"] = m_lastBookProgress >= 95 ? tr("Đã đọc xong") : (tr("Đang đọc ") + QString("%1%").arg(m_lastBookProgress));
            lastItem["isFavorite"] = settings.value(QString("favorites/%1").arg(m_lastBookPath), false).toBool();
            lastItem["isReadingList"] = settings.value(QString("readingList/%1").arg(m_lastBookPath), false).toBool();
            lastItem["isCompleted"] = settings.value(QString("completed/%1").arg(m_lastBookPath), false).toBool();
            lastItem["sortKey"] = QString("0000000000000000_%1").arg(lastItem.value("title").toString());
            m_readingHistoryModel->insert(lastItem);
        }
    }

    if (m_homeFeedModel) {
        m_homeFeedModel->clear();

        QVariantMap dashRow;
        dashRow["rowType"] = "dashboard";
        m_homeFeedModel->append(dashRow);

        QList<QVariantMap> otherRows;
        QString curLastAbs = m_lastBookPath.isEmpty() ? QString() : QFileInfo(m_lastBookPath).absoluteFilePath();
        for (QVariantList idx = m_readingHistoryModel->first(); !idx.isEmpty(); idx = m_readingHistoryModel->after(idx)) {
            QVariantMap row = m_readingHistoryModel->data(idx).toMap();
            QString rowPath = row.value("filePath").toString();
            if (hasLastBook() && !rowPath.isEmpty()) {
                QString rowAbs = QFileInfo(rowPath).absoluteFilePath();
                if (rowPath == m_lastBookPath || (!curLastAbs.isEmpty() && rowAbs == curLastAbs)) {
                    continue; // Already featured in Hero Card
                }
            }
            row["rowType"] = "book";
            otherRows.append(row);
        }

        if (!otherRows.isEmpty()) {
            QVariantMap secRow;
            secRow["rowType"] = "section";
            m_homeFeedModel->append(secRow);
            for (int i = 0; i < otherRows.size(); ++i) {
                m_homeFeedModel->append(otherRows.at(i));
            }
        } else if (!hasLastBook()) {
            QVariantMap secRow;
            secRow["rowType"] = "section";
            m_homeFeedModel->append(secRow);
            QVariantMap emptyRow;
            emptyRow["rowType"] = "empty";
            m_homeFeedModel->append(emptyRow);
        }
    }

    emit recentHistoryCountChanged();
    emit statsChanged();
}

// =============================================================================
// HIGHLIGHTS & NOTES IMPLEMENTATION
// =============================================================================

void BookManager::saveHighlights()
{
    m_cachedContentHtml.clear();
    QSettings settings("Libris", "Libris");
    settings.remove("highlights");
    settings.beginWriteArray("highlights");
    for (int i = 0; i < m_allHighlights.size(); ++i) {
        settings.setArrayIndex(i);
        const QVariantMap &hl = m_allHighlights.at(i);
        settings.setValue("id", hl.value("id"));
        settings.setValue("filePath", hl.value("filePath"));
        settings.setValue("bookTitle", hl.value("bookTitle"));
        settings.setValue("chapterIndex", hl.value("chapterIndex"));
        settings.setValue("chapterTitle", hl.value("chapterTitle"));
        settings.setValue("text", hl.value("text"));
        settings.setValue("note", hl.value("note"));
        settings.setValue("color", hl.value("color"));
        settings.setValue("createdAt", hl.value("createdAt"));
        settings.setValue("dateStr", hl.value("dateStr"));
    }
    settings.endArray();
    settings.sync();
    syncToPersistentStorage();
}

void BookManager::loadHighlights()
{
    m_allHighlights.clear();
    QSettings settings("Libris", "Libris");
    int size = settings.beginReadArray("highlights");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QVariantMap hl;
        hl["id"] = settings.value("id").toString();
        hl["filePath"] = settings.value("filePath").toString();
        hl["bookTitle"] = settings.value("bookTitle").toString();
        hl["chapterIndex"] = settings.value("chapterIndex").toInt();
        hl["chapterTitle"] = settings.value("chapterTitle").toString();
        hl["text"] = settings.value("text").toString();
        hl["note"] = settings.value("note").toString();
        hl["color"] = settings.value("color", "yellow").toString();
        hl["createdAt"] = settings.value("createdAt").toLongLong();
        hl["dateStr"] = settings.value("dateStr").toString();
        if (!hl["id"].toString().isEmpty() && !hl["text"].toString().isEmpty()) {
            m_allHighlights.append(hl);
        }
    }
    settings.endArray();

    // Fallback: restore from external persistent highlights JSON if sandbox highlights are empty
    if (m_allHighlights.isEmpty()) {
        QFile hlFile(persistentHighlightsPath());
        if (hlFile.exists() && hlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(hlFile.readAll());
            hlFile.close();
            QScriptEngine engine;
            QScriptValue root = engine.evaluate("(" + content + ")");
            if (root.isArray()) {
                int len = root.property("length").toInt32();
                for (int i = 0; i < len; ++i) {
                    QScriptValue item = root.property(i);
                    QVariantMap hl;
                    hl["id"] = item.property("id").toString();
                    hl["filePath"] = item.property("filePath").toString();
                    hl["bookTitle"] = item.property("bookTitle").toString();
                    hl["chapterIndex"] = item.property("chapterIndex").toInt32();
                    hl["chapterTitle"] = item.property("chapterTitle").toString();
                    hl["text"] = item.property("text").toString();
                    hl["note"] = item.property("note").toString();
                    hl["color"] = item.property("color").toString();
                    if (hl["color"].toString().isEmpty()) hl["color"] = "yellow";
                    hl["createdAt"] = (qlonglong)item.property("createdAt").toNumber();
                    hl["dateStr"] = item.property("dateStr").toString();
                    if (!hl["id"].toString().isEmpty() && !hl["text"].toString().isEmpty()) {
                        m_allHighlights.append(hl);
                    }
                }
            }
        }
    }

    emit highlightsChanged();
}

QString BookManager::addHighlight(const QString &text, const QString &note, const QString &color, int chapter, const QString &chapterTitle)
{
    if (text.trimmed().isEmpty()) return QString();

    int chap = (chapter >= 0) ? chapter : m_currentChapterIndex;
    QString chapTitle = !chapterTitle.isEmpty() ? chapterTitle : currentChapterTitle();
    if (chapTitle.trimmed().isEmpty()) {
        chapTitle = QString(tr("Chương %1")).arg(chap + 1);
    }

    QString hid = QString("hl_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(qrand() % 1000);
    QString clr = color.isEmpty() ? "yellow" : color;

    QVariantMap hl;
    hl["id"] = hid;
    hl["filePath"] = m_currentFilePath;
    hl["bookTitle"] = m_currentBookTitle.isEmpty() ? QFileInfo(m_currentFilePath).baseName() : m_currentBookTitle;
    hl["chapterIndex"] = chap;
    hl["chapterTitle"] = chapTitle;
    hl["text"] = text.trimmed();
    hl["note"] = note.trimmed();
    hl["color"] = clr;
    hl["createdAt"] = (qlonglong)QDateTime::currentDateTime().toMSecsSinceEpoch();
    hl["dateStr"] = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm");

    m_allHighlights.prepend(hl);
    m_lastAddedHighlightId = hid;
    saveHighlights();
    emit highlightsChanged();
    return hid;
}

bool BookManager::updateHighlight(const QString &id, const QString &note, const QString &color)
{
    for (int i = 0; i < m_allHighlights.size(); ++i) {
        if (m_allHighlights[i].value("id").toString() == id) {
            m_allHighlights[i]["note"] = note.trimmed();
            if (!color.isEmpty()) {
                m_allHighlights[i]["color"] = color;
            }
            saveHighlights();
            emit highlightsChanged();
            return true;
        }
    }
    return false;
}

bool BookManager::updateHighlightNote(const QString &id, const QString &note)
{
    return updateHighlight(id, note, QString());
}

bool BookManager::updateHighlightColor(const QString &id, const QString &color)
{
    return updateHighlight(id, getHighlight(id).value("note").toString(), color);
}

bool BookManager::removeHighlight(const QString &id)
{
    for (int i = 0; i < m_allHighlights.size(); ++i) {
        if (m_allHighlights[i].value("id").toString() == id) {
            m_allHighlights.removeAt(i);
            saveHighlights();
            emit highlightsChanged();
            return true;
        }
    }
    return false;
}

void BookManager::clearHighlights(const QString &filePath)
{
    if (filePath.isEmpty()) {
        m_allHighlights.clear();
    } else {
        for (int i = m_allHighlights.size() - 1; i >= 0; --i) {
            if (m_allHighlights[i].value("filePath").toString() == filePath) {
                m_allHighlights.removeAt(i);
            }
        }
    }
    saveHighlights();
    emit highlightsChanged();
}

QVariantMap BookManager::getHighlight(const QString &id) const
{
    for (int i = 0; i < m_allHighlights.size(); ++i) {
        if (m_allHighlights[i].value("id").toString() == id) {
            return m_allHighlights[i];
        }
    }
    return QVariantMap();
}

void BookManager::jumpToHighlight(const QString &id)
{
    QVariantMap hl = getHighlight(id);
    if (hl.isEmpty()) return;

    QString bookPath = hl.value("filePath").toString();
    int targetChapter = hl.value("chapterIndex").toInt();

    bool switchedBook = (bookPath != m_currentFilePath && !bookPath.isEmpty());
    if (switchedBook && !openBook(bookPath)) {
        return;
    }

    if (switchedBook && isPdf() && (m_totalChapters < targetChapter || m_isLoadingBook)) {
        m_pendingJumpPageAfterPdfOpen = targetChapter;
    } else {
        goToChapter(targetChapter);
    }
}

void BookManager::refreshHighlights()
{
    emit highlightsChanged();
}

QVariantList BookManager::currentBookHighlights() const
{
    QVariantList list;
    if (m_currentFilePath.isEmpty()) return list;
    for (int i = 0; i < m_allHighlights.size(); ++i) {
        if (m_allHighlights[i].value("filePath").toString() == m_currentFilePath) {
            list.append(m_allHighlights[i]);
        }
    }
    return list;
}


QVariantList BookManager::allHighlights() const
{
    QVariantList list;
    for (int i = 0; i < m_allHighlights.size(); ++i) {
        list.append(m_allHighlights[i]);
    }
    return list;
}


int BookManager::currentBookHighlightCount() const
{
    int count = 0;
    if (m_currentFilePath.isEmpty()) return 0;
    for (int i = 0; i < m_allHighlights.size(); ++i) {
        if (m_allHighlights[i].value("filePath").toString() == m_currentFilePath) {
            count++;
        }
    }
    return count;
}

int BookManager::totalHighlightCount() const
{
    return m_allHighlights.size();
}

QString BookManager::getHighlightsJsonForCurrentChapter() const
{
    QString json = "[";
    bool first = true;
    for (int i = 0; i < m_allHighlights.size(); ++i) {
        const QVariantMap &hl = m_allHighlights.at(i);
        if (hl.value("filePath").toString() == m_currentFilePath &&
            hl.value("chapterIndex").toInt() == m_currentChapterIndex) {
            if (!first) json += ",";
            first = false;
            QString idEsc = escapeJsonString(hl.value("id").toString());
            QString textEsc = escapeJsonString(hl.value("text").toString());
            QString colorEsc = escapeJsonString(hl.value("color").toString());
            QString noteEsc = escapeJsonString(hl.value("note").toString());
            idEsc.replace("</script", "<\\/script", Qt::CaseInsensitive);
            textEsc.replace("</script", "<\\/script", Qt::CaseInsensitive);
            colorEsc.replace("</script", "<\\/script", Qt::CaseInsensitive);
            noteEsc.replace("</script", "<\\/script", Qt::CaseInsensitive);
            json += QString("{\"id\":\"%1\",\"text\":\"%2\",\"color\":\"%3\",\"note\":\"%4\"}")
                    .arg(idEsc, textEsc, colorEsc, noteEsc);
        }
    }
    json += "]";
    return json;
}

// =============================================================================
// BOOKMARK FUNCTIONS
// =============================================================================

void BookManager::saveBookmarks()
{
    QSettings settings("Libris", "Libris");
    settings.remove("bookmarks");
    settings.beginWriteArray("bookmarks");
    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        settings.setArrayIndex(i);
        const QVariantMap &bm = m_allBookmarks.at(i);
        settings.setValue("id", bm.value("id"));
        settings.setValue("filePath", bm.value("filePath"));
        settings.setValue("bookTitle", bm.value("bookTitle"));
        settings.setValue("chapterIndex", bm.value("chapterIndex"));
        settings.setValue("chapterTitle", bm.value("chapterTitle"));
        settings.setValue("progress", bm.value("progress"));
        settings.setValue("snippet", bm.value("snippet"));
        settings.setValue("note", bm.value("note"));
        settings.setValue("createdAt", bm.value("createdAt"));
        settings.setValue("dateStr", bm.value("dateStr"));
    }
    settings.endArray();
    settings.sync();
    syncToPersistentStorage();
}

void BookManager::loadBookmarks()
{
    m_allBookmarks.clear();
    QSettings settings("Libris", "Libris");
    int size = settings.beginReadArray("bookmarks");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QVariantMap bm;
        bm["id"] = settings.value("id").toString();
        bm["filePath"] = settings.value("filePath").toString();
        bm["bookTitle"] = settings.value("bookTitle").toString();
        bm["chapterIndex"] = settings.value("chapterIndex").toInt();
        bm["chapterTitle"] = settings.value("chapterTitle").toString();
        bm["progress"] = settings.value("progress").toInt();
        bm["snippet"] = settings.value("snippet").toString();
        bm["note"] = settings.value("note").toString();
        bm["createdAt"] = settings.value("createdAt").toLongLong();
        bm["dateStr"] = settings.value("dateStr").toString();
        if (!bm["id"].toString().isEmpty() && !bm["filePath"].toString().isEmpty()) {
            m_allBookmarks.append(bm);
        }
    }
    settings.endArray();

    // Fallback: restore from external persistent bookmarks JSON if sandbox is empty
    if (m_allBookmarks.isEmpty()) {
        QFile bmFile(persistentBookmarksPath());
        if (bmFile.exists() && bmFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(bmFile.readAll());
            bmFile.close();
            QScriptEngine engine;
            QScriptValue root = engine.evaluate("(" + content + ")");
            if (root.isArray()) {
                int len = root.property("length").toInt32();
                for (int i = 0; i < len; ++i) {
                    QScriptValue item = root.property(i);
                    QVariantMap bm;
                    bm["id"] = item.property("id").toString();
                    bm["filePath"] = item.property("filePath").toString();
                    bm["bookTitle"] = item.property("bookTitle").toString();
                    bm["chapterIndex"] = item.property("chapterIndex").toInt32();
                    bm["chapterTitle"] = item.property("chapterTitle").toString();
                    bm["progress"] = item.property("progress").toInt32();
                    bm["snippet"] = item.property("snippet").toString();
                    bm["note"] = item.property("note").toString();
                    bm["createdAt"] = (qlonglong)item.property("createdAt").toNumber();
                    bm["dateStr"] = item.property("dateStr").toString();
                    if (!bm["id"].toString().isEmpty() && !bm["filePath"].toString().isEmpty()) {
                        m_allBookmarks.append(bm);
                    }
                }
            }
        }
    }

    emit bookmarksChanged();
}

QString BookManager::addBookmark(int chapter, const QString &chapterTitle, const QString &snippet, const QString &note)
{
    if (m_currentFilePath.isEmpty()) return QString();

    int chap = (chapter >= 0) ? chapter : m_currentChapterIndex;
    QString chapTitle = !chapterTitle.isEmpty() ? chapterTitle : currentChapterTitle();
    if (chapTitle.trimmed().isEmpty()) {
        chapTitle = isPdf() ? QString(tr("Trang %1")).arg(chap) : QString(tr("Chương %1")).arg(chap + 1);
    }

    // Check if bookmark already exists for this file and chapter
    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        if (m_allBookmarks[i].value("filePath").toString() == m_currentFilePath &&
            m_allBookmarks[i].value("chapterIndex").toInt() == chap) {
            return m_allBookmarks[i].value("id").toString();
        }
    }

    QString bmid = QString("bm_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(qrand() % 1000);

    QString snip = snippet;
    if (snip.isEmpty()) {
        if (isPdf()) {
            snip = m_currentPdfText.trimmed().left(140);
        } else {
            QString raw = currentContent();
            raw.remove(minimalRx("<style[^>]*>[\\s\\S]*</style>", Qt::CaseInsensitive));
            raw.remove(minimalRx("<script[^>]*>[\\s\\S]*</script>", Qt::CaseInsensitive));
            raw.remove(minimalRx("<!--[\\s\\S]*-->"));
            QRegExp tagRx("<[^>]*>");
            raw.remove(tagRx);
            raw.replace("&nbsp;", " ").replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">").replace("&quot;", "\"");
            raw = raw.simplified();
            snip = raw.left(140);
        }
    }

    QVariantMap bm;
    bm["id"] = bmid;
    bm["filePath"] = m_currentFilePath;
    bm["bookTitle"] = m_currentBookTitle.isEmpty() ? QFileInfo(m_currentFilePath).baseName() : m_currentBookTitle;
    bm["chapterIndex"] = chap;
    bm["chapterTitle"] = chapTitle;
    bm["progress"] = readingProgress();
    bm["snippet"] = snip;
    bm["note"] = note.trimmed();
    bm["createdAt"] = (qlonglong)QDateTime::currentDateTime().toMSecsSinceEpoch();
    bm["dateStr"] = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm");

    m_allBookmarks.prepend(bm);
    saveBookmarks();
    emit bookmarksChanged();
    return bmid;
}

bool BookManager::removeBookmark(const QString &id)
{
    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        if (m_allBookmarks[i].value("id").toString() == id) {
            m_allBookmarks.removeAt(i);
            saveBookmarks();
            emit bookmarksChanged();
            return true;
        }
    }
    return false;
}

bool BookManager::removeBookmarkAtChapter(const QString &filePath, int chapter)
{
    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        if (m_allBookmarks[i].value("filePath").toString() == filePath &&
            m_allBookmarks[i].value("chapterIndex").toInt() == chapter) {
            m_allBookmarks.removeAt(i);
            saveBookmarks();
            emit bookmarksChanged();
            return true;
        }
    }
    return false;
}

bool BookManager::toggleBookmarkCurrentPosition()
{
    if (m_currentFilePath.isEmpty()) return false;

    int chap = m_currentChapterIndex;
    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        if (m_allBookmarks[i].value("filePath").toString() == m_currentFilePath &&
            m_allBookmarks[i].value("chapterIndex").toInt() == chap) {
            m_allBookmarks.removeAt(i);
            saveBookmarks();
            emit bookmarksChanged();
            return false; // Removed
        }
    }

    addBookmark(chap);
    return true; // Added
}

bool BookManager::isBookmark(const QString &filePath, int chapter) const
{
    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        if (m_allBookmarks[i].value("filePath").toString() == filePath &&
            m_allBookmarks[i].value("chapterIndex").toInt() == chapter) {
            return true;
        }
    }
    return false;
}

bool BookManager::isCurrentChapterBookmarked() const
{
    if (m_currentFilePath.isEmpty()) return false;
    return isBookmark(m_currentFilePath, m_currentChapterIndex);
}

void BookManager::jumpToBookmark(const QString &id)
{
    QVariantMap bm = getBookmark(id);
    if (bm.isEmpty()) return;

    QString bookPath = bm.value("filePath").toString();
    int targetChapter = bm.value("chapterIndex").toInt();

    bool switchedBook = (bookPath != m_currentFilePath && !bookPath.isEmpty());
    if (switchedBook && !openBook(bookPath)) {
        return;
    }

    if (switchedBook && isPdf() && (m_totalChapters < targetChapter || m_isLoadingBook)) {
        m_pendingJumpPageAfterPdfOpen = targetChapter;
    } else {
        goToChapter(targetChapter);
    }
}

void BookManager::clearBookmarks(const QString &filePath)
{
    if (filePath.isEmpty()) {
        m_allBookmarks.clear();
    } else {
        for (int i = m_allBookmarks.size() - 1; i >= 0; --i) {
            if (m_allBookmarks[i].value("filePath").toString() == filePath) {
                m_allBookmarks.removeAt(i);
            }
        }
    }
    saveBookmarks();
    emit bookmarksChanged();
}

QVariantMap BookManager::getBookmark(const QString &id) const
{
    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        if (m_allBookmarks[i].value("id").toString() == id) {
            return m_allBookmarks[i];
        }
    }
    return QVariantMap();
}

bool BookManager::updateBookmarkNote(const QString &id, const QString &note)
{
    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        if (m_allBookmarks[i].value("id").toString() == id) {
            m_allBookmarks[i]["note"] = note.trimmed();
            saveBookmarks();
            emit bookmarksChanged();
            return true;
        }
    }
    return false;
}

void BookManager::refreshBookmarks()
{
    emit bookmarksChanged();
}

QVariantList BookManager::currentBookBookmarks() const
{
    QVariantList list;
    if (m_currentFilePath.isEmpty()) return list;

    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        if (m_allBookmarks[i].value("filePath").toString() == m_currentFilePath) {
            list.append(m_allBookmarks[i]);
        }
    }
    return list;
}

QVariantList BookManager::allBookmarks() const
{
    QVariantList list;
    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        list.append(m_allBookmarks[i]);
    }
    return list;
}

int BookManager::currentBookBookmarkCount() const
{
    int count = 0;
    if (m_currentFilePath.isEmpty()) return 0;
    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        if (m_allBookmarks[i].value("filePath").toString() == m_currentFilePath) {
            count++;
        }
    }
    return count;
}

int BookManager::totalBookmarkCount() const
{
    return m_allBookmarks.size();
}

QString BookManager::persistentBookmarksPath() const
{
    return persistentDataDir() + "/libris_bookmarks.json";
}

// =============================================================================
// REMOTE DIAGNOSTIC & SENDLOG SYSTEM
// =============================================================================

bool BookManager::isSendingLog() const
{
    return m_isSendingLog;
}

QString BookManager::lastLogStatus() const
{
    return m_lastLogStatus;
}

QString BookManager::devServerIp() const
{
    return m_devServerIp;
}

void BookManager::setDevServerIp(const QString &ip)
{
    QString clean = ip.trimmed();
    if (!clean.isEmpty() && m_devServerIp != clean) {
        m_devServerIp = clean;
        saveSettings();
        emit devServerIpChanged();
    }
}

void BookManager::sendLog(const QString &targetIp)
{
    Q_UNUSED(targetIp);
    qDebug() << "[BookManager::sendLog] Send log feature disabled.";
}

bool BookManager::sendBugReportEmail(const QString &category, const QString &subject, const QString &description, bool includeLog)
{
    QString catClean = category.trimmed();
    if (catClean.isEmpty()) catClean = "Bug";
    QString subjClean = subject.trimmed();
    if (subjClean.isEmpty()) subjClean = "Bug Report";

    QString fullSubject = QString("[Libris v%1 Build %2][%3] %4")
                              .arg(appVersion())
                              .arg(buildNumber())
                              .arg(catClean)
                              .arg(subjClean);

    QString fullBody;
    QString descClean = description.trimmed();
    if (!descClean.isEmpty()) {
        fullBody += descClean + "\n\n";
    }
    fullBody += "--- Device & App Info ---\n";
    fullBody += QString("App: Libris v%1 (Build %2)\n").arg(appVersion()).arg(buildNumber());
    fullBody += QString("Screen: %1x%2 | Theme: %3 | Lang: %4\n")
                    .arg(m_screenWidth).arg(m_screenHeight).arg(m_appTheme).arg(m_language);
    if (!m_currentFormat.isEmpty()) {
        fullBody += QString("Current Format: %1\n").arg(m_currentFormat);
    }

    if (includeLog) {
        QStringList logCandidates;
        logCandidates << "/accounts/1000/shared/documents/libris.log" << (QDir::homePath() + "/data/libris.log");
        QString logTail;
        for (int i = 0; i < logCandidates.size(); ++i) {
            QFile f(logCandidates.at(i));
            if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qint64 sz = f.size();
                if (sz > 4096) {
                    f.seek(sz - 4096);
                }
                QString raw = QString::fromUtf8(f.readAll());
                f.close();
                QStringList lines = raw.split('\n');
                if (lines.size() > 60) {
                    lines = lines.mid(lines.size() - 60);
                }
                logTail = lines.join("\n").trimmed();
                if (!logTail.isEmpty()) break;
            }
        }
        if (!logTail.isEmpty()) {
            fullBody += "\n--- Technical Log (Optional) ---\n" + logTail;
        }
    }

    if (fullBody.length() > 6000) {
        fullBody = fullBody.left(6000) + "\n...[truncated]";
    }

    QString mailto = QString("mailto:nhatlm0910@gmail.com?subject=%1&body=%2")
                         .arg(QString::fromUtf8(QUrl::toPercentEncoding(fullSubject)))
                         .arg(QString::fromUtf8(QUrl::toPercentEncoding(fullBody)));

    bb::system::InvokeManager invokeManager;
    bb::system::InvokeRequest request;
    request.setTarget("sys.pim.uib.email.hybridcomposer");
    request.setAction("bb.action.SENDEMAIL");
    request.setUri(QUrl::fromEncoded(mailto.toUtf8()));
    bb::system::InvokeTargetReply *reply = invokeManager.invoke(request);
    return (reply != NULL);
}

// =============================================================================
// PERSISTENT SHARED STORAGE (SURVIVES APP REINSTALLATION)
// =============================================================================

QString BookManager::persistentDataDir() const
{
    QString baseDir;
    if (QDir("/accounts/1000/shared/documents").exists() || QDir("/accounts/1000/shared").exists()) {
        baseDir = "/accounts/1000/shared/documents/Libris";
    } else {
        baseDir = QDir::homePath() + "/.libris";
    }
    QDir dir(baseDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return baseDir;
}

QString BookManager::persistentDataFilePath() const
{
    return persistentDataDir() + "/libris_userdata.ini";
}

QString BookManager::persistentJsonFilePath() const
{
    return persistentDataDir() + "/libris_userdata.json";
}

QString BookManager::persistentHighlightsPath() const
{
    return persistentDataDir() + "/libris_highlights.json";
}

QString BookManager::persistentStorageStatus() const
{
    if (m_persistentStorageStatus.isEmpty()) {
        return tr("Bộ nhớ ngoài sẵn sàng: ") + persistentDataDir();
    }
    return m_persistentStorageStatus;
}

void BookManager::schedulePersistentSync()
{
    m_persistSyncTimer.start(3000);
}

void BookManager::onPersistentSyncTimeout()
{
    if (!m_suppressPersistentSync) {
        syncToPersistentStorage();
    }
}

void BookManager::syncToPersistentStorage()
{
    static bool s_isSyncing = false;
    if (s_isSyncing) return;
    s_isSyncing = true;

    QString dirPath = persistentDataDir();
    QDir().mkpath(dirPath);

    QSettings settings("Libris", "Libris");
    QString iniPath = persistentDataFilePath();
    QSettings persistentSettings(iniPath, QSettings::IniFormat);
    persistentSettings.clear();

    QStringList allKeys = settings.allKeys();
    foreach (const QString &k, allKeys) {
        persistentSettings.setValue(k, settings.value(k));
    }
    persistentSettings.setValue("meta/lastSyncTime", (qlonglong)QDateTime::currentDateTime().toMSecsSinceEpoch());
    persistentSettings.setValue("meta/syncTimestamp", QDateTime::currentDateTime().toString(Qt::ISODate));
    persistentSettings.setValue("meta/appVersion", "2.0.0");
    persistentSettings.sync();

    // Export libris_userdata.json (atomic write via .tmp + rename)
    QString jsonPath = persistentJsonFilePath();
    QString jsonTmpPath = jsonPath + ".tmp";
    QFile jsonFile(jsonTmpPath);
    if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream out(&jsonFile);
        out.setCodec("UTF-8");

        out << "{\n";
        out << "  \"app\": \"Libris\",\n";
        out << "  \"version\": \"2.0.0\",\n";
        out << "  \"lastSync\": \"" << QDateTime::currentDateTime().toString(Qt::ISODate) << "\",\n";

        // lastBook
        out << "  \"lastBook\": {\n";
        out << "    \"filePath\": \"" << escapeJsonString(m_lastBookPath) << "\",\n";
        out << "    \"title\": \"" << escapeJsonString(m_lastBookTitle) << "\",\n";
        out << "    \"author\": \"" << escapeJsonString(m_lastBookAuthor) << "\",\n";
        out << "    \"cover\": \"" << escapeJsonString(m_lastBookCover) << "\",\n";
        out << "    \"format\": \"" << escapeJsonString(m_lastBookFormat) << "\",\n";
        out << "    \"progress\": " << m_lastBookProgress << "\n";
        out << "  },\n";

        // readingStatistics
        out << "  \"readingStatistics\": {\n";
        out << "    \"totalReadingSeconds\": " << m_totalReadingSeconds << ",\n";
        out << "    \"todayReadingSeconds\": " << m_todayReadingSeconds << ",\n";
        out << "    \"todayDate\": \"" << escapeJsonString(m_todayDate) << "\"\n";
        out << "  },\n";

        // reading progress & books
        out << "  \"books\": [\n";
        QSet<QString> bookPaths;
        foreach (const QString &k, allKeys) {
            if (k.startsWith("progressPct/")) {
                bookPaths.insert(k.mid(12));
            } else if (k.startsWith("progress/")) {
                bookPaths.insert(k.mid(9));
            } else if (k.startsWith("lastReadTime/")) {
                bookPaths.insert(k.mid(13));
            }
        }
        if (!m_lastBookPath.isEmpty()) bookPaths.insert(m_lastBookPath);
        if (!m_currentFilePath.isEmpty()) bookPaths.insert(m_currentFilePath);

        QList<QString> pathList = bookPaths.toList();
        bool firstBook = true;
        for (int i = 0; i < pathList.size(); ++i) {
            const QString &p = pathList.at(i);
            if (p.isEmpty()) continue;
            if (!firstBook) out << ",\n";
            firstBook = false;

            int pct = settings.value(QString("progressPct/%1").arg(p), 0).toInt();
            int chap = settings.value(QString("progress/%1").arg(p), 0).toInt();
            int page = settings.value(QString("pageInChapter/%1").arg(p), 1).toInt();
            int totPages = settings.value(QString("totalPagesInChapter/%1").arg(p), 1).toInt();
            int scroll = settings.value(QString("scrollPos/%1").arg(p), 0).toInt();
            qlonglong lastRead = settings.value(QString("lastReadTime/%1").arg(p), 0).toLongLong();
            bool isFav = settings.value(QString("favorites/%1").arg(p), false).toBool();
            bool isRList = settings.value(QString("readingList/%1").arg(p), false).toBool();
            bool isComp = (pct >= 100);

            out << "    {\n";
            out << "      \"filePath\": \"" << escapeJsonString(p) << "\",\n";
            out << "      \"progress\": " << pct << ",\n";
            out << "      \"chapterIndex\": " << chap << ",\n";
            out << "      \"pageInChapter\": " << page << ",\n";
            out << "      \"totalPagesInChapter\": " << totPages << ",\n";
            out << "      \"scrollPos\": " << scroll << ",\n";
            out << "      \"lastReadTime\": " << lastRead << ",\n";
            out << "      \"isFavorite\": " << (isFav ? "true" : "false") << ",\n";
            out << "      \"isReadingList\": " << (isRList ? "true" : "false") << ",\n";
            out << "      \"isCompleted\": " << (isComp ? "true" : "false") << "\n";
            out << "    }";
        }
        out << "\n  ],\n";

        // highlights
        out << "  \"highlights\": [\n";
        for (int i = 0; i < m_allHighlights.size(); ++i) {
            const QVariantMap &hl = m_allHighlights.at(i);
            if (i > 0) out << ",\n";
            out << "    {\n";
            out << "      \"id\": \"" << escapeJsonString(hl.value("id").toString()) << "\",\n";
            out << "      \"filePath\": \"" << escapeJsonString(hl.value("filePath").toString()) << "\",\n";
            out << "      \"bookTitle\": \"" << escapeJsonString(hl.value("bookTitle").toString()) << "\",\n";
            out << "      \"chapterIndex\": " << hl.value("chapterIndex").toInt() << ",\n";
            out << "      \"chapterTitle\": \"" << escapeJsonString(hl.value("chapterTitle").toString()) << "\",\n";
            out << "      \"text\": \"" << escapeJsonString(hl.value("text").toString()) << "\",\n";
            out << "      \"note\": \"" << escapeJsonString(hl.value("note").toString()) << "\",\n";
            out << "      \"color\": \"" << escapeJsonString(hl.value("color", "yellow").toString()) << "\",\n";
            out << "      \"createdAt\": " << hl.value("createdAt").toLongLong() << ",\n";
            out << "      \"dateStr\": \"" << escapeJsonString(hl.value("dateStr").toString()) << "\"\n";
            out << "    }";
        }
        out << "\n  ],\n";

        // bookmarks
        out << "  \"bookmarks\": [\n";
        for (int i = 0; i < m_allBookmarks.size(); ++i) {
            const QVariantMap &bm = m_allBookmarks.at(i);
            if (i > 0) out << ",\n";
            out << "    {\n";
            out << "      \"id\": \"" << escapeJsonString(bm.value("id").toString()) << "\",\n";
            out << "      \"filePath\": \"" << escapeJsonString(bm.value("filePath").toString()) << "\",\n";
            out << "      \"bookTitle\": \"" << escapeJsonString(bm.value("bookTitle").toString()) << "\",\n";
            out << "      \"chapterIndex\": " << bm.value("chapterIndex").toInt() << ",\n";
            out << "      \"chapterTitle\": \"" << escapeJsonString(bm.value("chapterTitle").toString()) << "\",\n";
            out << "      \"progress\": " << bm.value("progress").toInt() << ",\n";
            out << "      \"snippet\": \"" << escapeJsonString(bm.value("snippet").toString()) << "\",\n";
            out << "      \"note\": \"" << escapeJsonString(bm.value("note").toString()) << "\",\n";
            out << "      \"createdAt\": " << bm.value("createdAt").toLongLong() << ",\n";
            out << "      \"dateStr\": \"" << escapeJsonString(bm.value("dateStr").toString()) << "\"\n";
            out << "    }";
        }
        out << "\n  ]\n";
        out << "}\n";
        out.flush();
        jsonFile.close();
        QFile::remove(jsonPath);
        QFile::rename(jsonTmpPath, jsonPath);
    }

    // Export libris_highlights.json (atomic write via .tmp + rename)
    QString hlPath = persistentHighlightsPath();
    QString hlTmpPath = hlPath + ".tmp";
    QFile hlFile(hlTmpPath);
    if (hlFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream out(&hlFile);
        out.setCodec("UTF-8");
        out << "[\n";
        for (int i = 0; i < m_allHighlights.size(); ++i) {
            const QVariantMap &hl = m_allHighlights.at(i);
            if (i > 0) out << ",\n";
            out << "  {\n";
            out << "    \"id\": \"" << escapeJsonString(hl.value("id").toString()) << "\",\n";
            out << "    \"filePath\": \"" << escapeJsonString(hl.value("filePath").toString()) << "\",\n";
            out << "    \"bookTitle\": \"" << escapeJsonString(hl.value("bookTitle").toString()) << "\",\n";
            out << "    \"chapterIndex\": " << hl.value("chapterIndex").toInt() << ",\n";
            out << "    \"chapterTitle\": \"" << escapeJsonString(hl.value("chapterTitle").toString()) << "\",\n";
            out << "    \"text\": \"" << escapeJsonString(hl.value("text").toString()) << "\",\n";
            out << "    \"note\": \"" << escapeJsonString(hl.value("note").toString()) << "\",\n";
            out << "    \"color\": \"" << escapeJsonString(hl.value("color", "yellow").toString()) << "\",\n";
            out << "    \"createdAt\": " << hl.value("createdAt").toLongLong() << ",\n";
            out << "    \"dateStr\": \"" << escapeJsonString(hl.value("dateStr").toString()) << "\"\n";
            out << "  }";
        }
        out << "\n]\n";
        out.flush();
        hlFile.close();
        QFile::remove(hlPath);
        QFile::rename(hlTmpPath, hlPath);
    }

    // Export libris_bookmarks.json (atomic write via .tmp + rename)
    QString bmPath = persistentBookmarksPath();
    QString bmTmpPath = bmPath + ".tmp";
    QFile bmFile(bmTmpPath);
    if (bmFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream out(&bmFile);
        out.setCodec("UTF-8");
        out << "[\n";
        for (int i = 0; i < m_allBookmarks.size(); ++i) {
            const QVariantMap &bm = m_allBookmarks.at(i);
            if (i > 0) out << ",\n";
            out << "  {\n";
            out << "    \"id\": \"" << escapeJsonString(bm.value("id").toString()) << "\",\n";
            out << "    \"filePath\": \"" << escapeJsonString(bm.value("filePath").toString()) << "\",\n";
            out << "    \"bookTitle\": \"" << escapeJsonString(bm.value("bookTitle").toString()) << "\",\n";
            out << "    \"chapterIndex\": " << bm.value("chapterIndex").toInt() << ",\n";
            out << "    \"chapterTitle\": \"" << escapeJsonString(bm.value("chapterTitle").toString()) << "\",\n";
            out << "    \"progress\": " << bm.value("progress").toInt() << ",\n";
            out << "    \"snippet\": \"" << escapeJsonString(bm.value("snippet").toString()) << "\",\n";
            out << "    \"note\": \"" << escapeJsonString(bm.value("note").toString()) << "\",\n";
            out << "    \"createdAt\": " << bm.value("createdAt").toLongLong() << ",\n";
            out << "    \"dateStr\": \"" << escapeJsonString(bm.value("dateStr").toString()) << "\"\n";
            out << "  }";
        }
        out << "\n]\n";
        out.flush();
        bmFile.close();
        QFile::remove(bmPath);
        QFile::rename(bmTmpPath, bmPath);
    }

    m_persistentStorageStatus = tr("Đã đồng bộ dữ liệu vào ") + dirPath;
    emit persistentStorageStatusChanged();

    s_isSyncing = false;
}

void BookManager::restoreFromPersistentStorage()
{
    restoreFromPersistentStorageInternal(true, true);
}

void BookManager::restoreFromPersistentStorageInternal(bool forceOverwrite, bool reloadState)
{
    QString iniPath = persistentDataFilePath();
    QString jsonPath = persistentJsonFilePath();
    QFileInfo iniFi(iniPath);
    QFileInfo jsonFi(jsonPath);

    QSettings settings("Libris", "Libris");
    bool isSandboxEmpty = settings.allKeys().isEmpty() || !settings.contains("stats/totalReadingSeconds");

    if (iniFi.exists()) {
        QSettings persistentSettings(iniPath, QSettings::IniFormat);
        QStringList pKeys = persistentSettings.allKeys();
        if (!pKeys.isEmpty()) {
            if (forceOverwrite || isSandboxEmpty) {
                qDebug() << "[BookManager] Restoring settings from persistent INI:" << iniPath << "(Keys count:" << pKeys.size() << ", forceOverwrite:" << forceOverwrite << ")";
                foreach (const QString &k, pKeys) {
                    if (k == "meta/lastSyncTime" || k == "meta/syncTimestamp" || k == "meta/appVersion") continue;
                    settings.setValue(k, persistentSettings.value(k));
                }
                settings.sync();
                m_persistentStorageStatus = tr("Đã khôi phục toàn bộ dữ liệu từ bộ nhớ ngoài.");
            } else {
                int restoredCount = 0;
                foreach (const QString &k, pKeys) {
                    if (k == "meta/lastSyncTime" || k == "meta/syncTimestamp" || k == "meta/appVersion") continue;
                    if (!settings.contains(k)) {
                        settings.setValue(k, persistentSettings.value(k));
                        restoredCount++;
                    } else if (k.startsWith("lastReadTime/")) {
                        qlonglong sandTime = settings.value(k, 0).toLongLong();
                        qlonglong persTime = persistentSettings.value(k, 0).toLongLong();
                        if (persTime > sandTime) {
                            QString bookPath = k.mid(13);
                            settings.setValue(k, persTime);
                            if (persistentSettings.contains("progress/" + bookPath))
                                settings.setValue("progress/" + bookPath, persistentSettings.value("progress/" + bookPath));
                            if (persistentSettings.contains("progressPct/" + bookPath))
                                settings.setValue("progressPct/" + bookPath, persistentSettings.value("progressPct/" + bookPath));
                            if (persistentSettings.contains("pageInChapter/" + bookPath))
                                settings.setValue("pageInChapter/" + bookPath, persistentSettings.value("pageInChapter/" + bookPath));
                            if (persistentSettings.contains("scrollPos/" + bookPath))
                                settings.setValue("scrollPos/" + bookPath, persistentSettings.value("scrollPos/" + bookPath));
                            if (persistentSettings.contains("completed/" + bookPath))
                                settings.setValue("completed/" + bookPath, persistentSettings.value("completed/" + bookPath));
                            if (persistentSettings.contains("opened/" + bookPath))
                                settings.setValue("opened/" + bookPath, persistentSettings.value("opened/" + bookPath));
                            restoredCount++;
                        }
                    }
                }
                if (restoredCount > 0) {
                    settings.sync();
                    qDebug() << "[BookManager] Merged" << restoredCount << "keys from persistent storage.";
                }
                m_persistentStorageStatus = tr("Dữ liệu đã được đồng bộ với bộ nhớ ngoài.");
            }
        }
    } else if (jsonFi.exists() && (forceOverwrite || isSandboxEmpty)) {
        qDebug() << "[BookManager] Persistent INI missing, restoring from JSON fallback:" << jsonPath;
        QFile jFile(jsonPath);
        if (jFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(jFile.readAll());
            jFile.close();
            QScriptEngine engine;
            QScriptValue root = engine.evaluate("(" + content + ")");
            if (root.isObject()) {
                QScriptValue lb = root.property("lastBook");
                if (lb.isObject()) {
                    QString lbPath = lb.property("filePath").toString();
                    if (!lbPath.isEmpty()) {
                        settings.setValue("lastBook/filePath", lbPath);
                        settings.setValue("lastBook/title", lb.property("title").toString());
                        settings.setValue("lastBook/author", lb.property("author").toString());
                        settings.setValue("lastBook/cover", lb.property("cover").toString());
                        settings.setValue("lastBook/format", lb.property("format").toString());
                        settings.setValue("lastBook/progress", lb.property("progress").toInt32());
                    }
                }
                QScriptValue st = root.property("readingStatistics");
                if (st.isObject()) {
                    settings.setValue("stats/totalReadingSeconds", st.property("totalReadingSeconds").toInt32());
                    settings.setValue("stats/todayReadingSeconds", st.property("todayReadingSeconds").toInt32());
                    settings.setValue("stats/todayDate", st.property("todayDate").toString());
                }
                QScriptValue booksVal = root.property("books");
                if (booksVal.isArray()) {
                    int bLen = booksVal.property("length").toInt32();
                    for (int i = 0; i < bLen; ++i) {
                        QScriptValue b = booksVal.property(i);
                        QString bPath = b.property("filePath").toString();
                        if (!bPath.isEmpty()) {
                            settings.setValue(QString("progress/%1").arg(bPath), b.property("chapterIndex").toInt32());
                            settings.setValue(QString("pageInChapter/%1").arg(bPath), b.property("pageInChapter").toInt32());
                            settings.setValue(QString("totalPagesInChapter/%1").arg(bPath), b.property("totalPagesInChapter").toInt32());
                            settings.setValue(QString("scrollPos/%1").arg(bPath), b.property("scrollPos").toInt32());
                            settings.setValue(QString("progressPct/%1").arg(bPath), b.property("progress").toInt32());
                            settings.setValue(QString("lastReadTime/%1").arg(bPath), (qlonglong)b.property("lastReadTime").toNumber());
                            if (b.property("isFavorite").toBool()) settings.setValue(QString("favorites/%1").arg(bPath), true);
                            if (b.property("isReadingList").toBool()) settings.setValue(QString("readingList/%1").arg(bPath), true);
                            if (b.property("isCompleted").toBool()) settings.setValue(QString("completed/%1").arg(bPath), true);
                        }
                    }
                }
                settings.sync();
                m_persistentStorageStatus = tr("Đã khôi phục dữ liệu từ tệp JSON.");
            }
        }
    } else {
        m_persistentStorageStatus = tr("Bộ nhớ ngoài sẵn sàng: ") + persistentDataDir();
    }

    if (reloadState) {
        loadSettingsFromQSettings(settings);
        loadLibraryCache();
        reconcileLibraryWithSettings(settings);
        loadHighlights();
        loadBookmarks();
        applyFilterAndSort();
        updateReadingHistoryModel();
        refreshStats();

        emit highlightsChanged();
        emit bookmarksChanged();
        emit customShelvesChanged();
        emit languageChanged();
        emit fontSizeChanged();
        emit fontFamilyChanged();
        emit themeChanged();
        emit appThemeChanged();
        emit readingModeChanged();
        emit columnLayoutChanged();
        emit mangaRtlModeChanged();
        emit lineSpacingChanged();
        emit marginsChanged();
        emit footerSettingsChanged();
        emit ttsAutoAdvanceChanged(m_ttsAutoAdvance);
        emit mainViewModeChanged();
        emit libraryViewModeChanged();
        emit libraryBrowseModeChanged();
        emit sortByChanged();
        emit groupByChanged();
        emit formatFilterChanged();
        emit lastBookChanged();
        emit statsChanged();
        emit dailyStatsChanged();
        emit persistentStorageStatusChanged();
        return;
    }

    m_lastBookPath = settings.value("lastBook/filePath", m_lastBookPath).toString();
    m_lastBookTitle = settings.value("lastBook/title", m_lastBookTitle).toString();
    m_lastBookAuthor = settings.value("lastBook/author", m_lastBookAuthor).toString();
    m_lastBookCover = settings.value("lastBook/cover", m_lastBookCover).toString();
    m_lastBookFormat = settings.value("lastBook/format", m_lastBookFormat).toString();
    m_lastBookProgress = settings.value("lastBook/progress", m_lastBookProgress).toInt();

    m_totalReadingSeconds = settings.value("stats/totalReadingSeconds", m_totalReadingSeconds).toInt();
    m_todayReadingSeconds = settings.value("stats/todayReadingSeconds", m_todayReadingSeconds).toInt();

    loadHighlights();
    loadBookmarks();
    updateReadingHistoryModel();
    emit lastBookChanged();
    emit statsChanged();
    emit dailyStatsChanged();
    emit persistentStorageStatusChanged();
}

void BookManager::promptExit()
{
    qDebug() << "[BookManager] promptExit called";
    emit promptExitRequested();
}

void BookManager::exitApp()
{
    qDebug() << "[BookManager] exitApp: Saving current progress and shutting down...";
    saveForShutdown();
    pauseTimersForSleep(false);
    if (m_tts && m_tts->isActive()) {
        m_tts->stop();
    }
    cancelWorkers();
    if (bb::cascades::Application::instance()) {
        bb::cascades::Application::instance()->requestExit();
    }
}

QString BookManager::exportBookHighlightsToMarkdown(const QString &filePath)
{
    QString targetPath = filePath.isEmpty() ? m_currentFilePath : filePath;
    if (targetPath.isEmpty()) return QString();

    QString targetTitle = m_currentBookTitle;
    QString targetAuthor = m_currentBookAuthor;
    if (targetPath != m_currentFilePath) {
        targetTitle = QFileInfo(targetPath).baseName();
        targetAuthor.clear();
        for (int i = 0; i < m_allBooks.size(); ++i) {
            if (m_allBooks.at(i).value("filePath").toString() == targetPath) {
                QString t = m_allBooks.at(i).value("title").toString().trimmed();
                if (!t.isEmpty()) targetTitle = t;
                targetAuthor = m_allBooks.at(i).value("author").toString().trimmed();
                break;
            }
        }
    }

    // Filter highlights and bookmarks for this book
    QList<QVariantMap> bookHls;
    for (int i = 0; i < m_allHighlights.size(); ++i) {
        const QVariantMap &h = m_allHighlights.at(i);
        if (h.value("filePath").toString() == targetPath) {
            bookHls.append(h);
        }
    }

    QList<QVariantMap> bookBms;
    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        const QVariantMap &b = m_allBookmarks.at(i);
        if (b.value("filePath").toString() == targetPath) {
            bookBms.append(b);
        }
    }

    if (bookHls.isEmpty() && bookBms.isEmpty()) {
        return QString();
    }

    // Build Markdown string
    QString md;
    md += QString("# Ghi chú & Trích dẫn: %1\n\n").arg(targetTitle.isEmpty() ? QFileInfo(targetPath).baseName() : targetTitle);
    if (!targetAuthor.isEmpty()) {
        md += QString("- **Tác giả:** %1\n").arg(targetAuthor);
    }
    md += QString("- **Tài liệu:** `%1`\n").arg(QFileInfo(targetPath).fileName());
    md += QString("- **Ngày xuất:** %1\n").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"));
    md += QString("- **Tổng số trích dẫn:** %1 | **Dấu trang:** %2\n\n").arg(bookHls.size()).arg(bookBms.size());
    md += "---\n\n";

    if (!bookHls.isEmpty()) {
        md += "## 📌 Danh sách Trích dẫn & Ghi chú\n\n";
        for (int i = 0; i < bookHls.size(); ++i) {
            const QVariantMap &h = bookHls.at(i);
            QString chTitle = h.value("chapterTitle").toString();
            QString text = h.value("text").toString();
            QString note = h.value("note").toString();
            QString color = h.value("color", "yellow").toString();
            QString dateStr = h.value("dateStr").toString();

            if (!chTitle.isEmpty()) {
                md += QString("### %1\n\n").arg(chTitle);
            }
            md += QString("> %1\n\n").arg(text);
            if (!note.isEmpty()) {
                md += QString("*📝 Ghi chú:* %1\n\n").arg(note);
            }
            md += QString("*(Màu: %1 | %2)*\n\n---\n\n").arg(color, dateStr);
        }
    }

    if (!bookBms.isEmpty()) {
        md += "## 🔖 Danh sách Dấu trang\n\n";
        for (int i = 0; i < bookBms.size(); ++i) {
            const QVariantMap &b = bookBms.at(i);
            QString chTitle = b.value("chapterTitle").toString();
            QString snippet = b.value("snippet").toString();
            QString note = b.value("note").toString();
            int progress = b.value("progress", 0).toInt();
            QString dateStr = b.value("dateStr").toString();

            md += QString("### Dấu trang: %1 (Tiến độ: %2%)\n\n").arg(chTitle.isEmpty() ? QString("Chương %1").arg(b.value("chapterIndex").toInt() + 1) : chTitle).arg(progress);
            if (!snippet.isEmpty()) {
                md += QString("> %1\n\n").arg(snippet);
            }
            if (!note.isEmpty()) {
                md += QString("*📝 Ghi chú:* %1\n\n").arg(note);
            }
            md += QString("*(Ngày tạo: %1)*\n\n---\n\n").arg(dateStr);
        }
    }

    // Save to shared documents folder
    QString outDir = persistentDataDir();
    QDir().mkpath(outDir);
    QString cleanTitle = (targetTitle.isEmpty() ? QFileInfo(targetPath).baseName() : targetTitle);
    cleanTitle.replace(QRegExp("[\\\\/:*?\"<>|]"), "_");
    QString outPath = outDir + QString("/Libris_%1_Notes.md").arg(cleanTitle);

    QFile outFile(outPath);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream outStream(&outFile);
        outStream.setCodec("UTF-8");
        outStream << md;
        outFile.close();
        return outPath;
    }
    return QString();
}


QString BookManager::exportAllHighlightsToMarkdown()
{
    if (m_allHighlights.isEmpty() && m_allBookmarks.isEmpty()) {
        return QString();
    }

    QString md;
    md += "# Libris - Toàn bộ Trích dẫn & Dấu trang Thư viện\n\n";
    md += QString("- **Ngày xuất:** %1\n").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"));
    md += QString("- **Tổng trích dẫn:** %1 | **Tổng dấu trang:** %2\n\n").arg(m_allHighlights.size()).arg(m_allBookmarks.size());
    md += "---\n\n";

    // Group highlights and bookmarks by book (keyed by filePath when present to avoid title collisions)
    QMap<QString, QString> bookDisplayTitle;
    QMap<QString, QList<QVariantMap> > highlightsByBook;
    QMap<QString, QList<QVariantMap> > bookmarksByBook;
    QList<QString> bookOrder;

    for (int i = 0; i < m_allHighlights.size(); ++i) {
        const QVariantMap &h = m_allHighlights.at(i);
        QString bPath = h.value("filePath").toString();
        QString bTitle = h.value("bookTitle").toString();
        if (bTitle.isEmpty()) bTitle = QFileInfo(bPath).baseName();
        QString key = !bPath.isEmpty() ? bPath : bTitle;
        if (!bookDisplayTitle.contains(key)) {
            bookDisplayTitle[key] = bTitle;
            bookOrder.append(key);
        }
        highlightsByBook[key].append(h);
    }

    for (int i = 0; i < m_allBookmarks.size(); ++i) {
        const QVariantMap &b = m_allBookmarks.at(i);
        QString bPath = b.value("filePath").toString();
        QString bTitle = b.value("bookTitle").toString();
        if (bTitle.isEmpty()) bTitle = QFileInfo(bPath).baseName();
        QString key = !bPath.isEmpty() ? bPath : bTitle;
        if (!bookDisplayTitle.contains(key)) {
            bookDisplayTitle[key] = bTitle;
            bookOrder.append(key);
        }
        bookmarksByBook[key].append(b);
    }

    foreach (const QString &key, bookOrder) {
        QString bookTitle = bookDisplayTitle.value(key);
        md += QString("## 📚 %1\n\n").arg(bookTitle.isEmpty() ? key : bookTitle);

        QList<QVariantMap> hls = highlightsByBook.value(key);
        if (!hls.isEmpty()) {
            md += "### 📌 Trích dẫn & Ghi chú\n\n";
            for (int i = 0; i < hls.size(); ++i) {
                const QVariantMap &h = hls.at(i);
                QString chTitle = h.value("chapterTitle").toString();
                QString text = h.value("text").toString();
                QString note = h.value("note").toString();
                QString color = h.value("color", "yellow").toString();
                QString dateStr = h.value("dateStr").toString();

                if (!chTitle.isEmpty()) {
                    md += QString("#### %1\n\n").arg(chTitle);
                }
                md += QString("> %1\n\n").arg(text);
                if (!note.isEmpty()) {
                    md += QString("*📝 Ghi chú:* %1\n\n").arg(note);
                }
                md += QString("*(Màu: %1 | %2)*\n\n").arg(color, dateStr);
            }
        }

        QList<QVariantMap> bms = bookmarksByBook.value(key);
        if (!bms.isEmpty()) {
            md += "### 🔖 Dấu trang (Bookmarks)\n\n";
            for (int i = 0; i < bms.size(); ++i) {
                const QVariantMap &bm = bms.at(i);
                QString chTitle = bm.value("chapterTitle").toString();
                QString snippet = bm.value("snippet").toString();
                QString note = bm.value("note").toString();
                int progress = bm.value("progress", 0).toInt();
                QString dateStr = bm.value("dateStr").toString();

                md += QString("#### Dấu trang: %1 (Tiến độ: %2%)\n\n")
                      .arg(chTitle.isEmpty() ? QString("Chương %1").arg(bm.value("chapterIndex").toInt() + 1) : chTitle)
                      .arg(progress);
                if (!snippet.isEmpty()) {
                    md += QString("> %1\n\n").arg(snippet);
                }
                if (!note.isEmpty()) {
                    md += QString("*📝 Ghi chú:* %1\n\n").arg(note);
                }
                md += QString("*(Ngày tạo: %1)*\n\n").arg(dateStr);
            }
        }
        md += "---\n\n";
    }

    QString outDir = persistentDataDir();
    QDir().mkpath(outDir);
    QString outPath = outDir + "/Libris_All_Notes.md";

    QFile outFile(outPath);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream outStream(&outFile);
        outStream.setCodec("UTF-8");
        outStream << md;
        outFile.close();
        return outPath;
    }
    return QString();
}

QString BookManager::exportQuoteCardPng(const QString &quoteText, const QString &bookTitle, const QString &author, const QString &colorName)
{
    QString cleanQuote = quoteText.trimmed();
    if (cleanQuote.isEmpty()) return QString();
    if (cleanQuote.length() > 480) {
        cleanQuote = cleanQuote.left(477).trimmed() + QString::fromUtf8("…");
    }

    QColor accent("#D9AB60");
    if (colorName == "green") accent = QColor("#5FA876");
    else if (colorName == "blue") accent = QColor("#5B9BD5");
    else if (colorName == "pink") accent = QColor("#D6729B");

    QImage img(1080, 1080, QImage::Format_ARGB32);
    img.fill(QColor("#14110E"));

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    // Outer card
    QRect cardRect(72, 72, 936, 936);
    p.fillRect(cardRect, QColor("#1D1915"));
    p.setPen(QPen(QColor("#2E2720"), 2));
    p.drawRect(cardRect);

    // Accent left stripe
    p.fillRect(QRect(72, 72, 10, 936), accent);

    // Opening quote mark
    QFont markFont("Georgia", 72, QFont::Bold);
    p.setFont(markFont);
    p.setPen(accent);
    p.drawText(QRect(126, 106, 120, 110), Qt::AlignLeft | Qt::AlignTop, QString::fromUtf8("“"));

    // Quote text
    int fontSize = (cleanQuote.length() > 260) ? 26 : ((cleanQuote.length() > 140) ? 31 : 36);
    QFont quoteFont("Georgia", fontSize);
    p.setFont(quoteFont);
    p.setPen(QColor("#F5EFE6"));
    p.drawText(QRect(130, 210, 820, 560), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, cleanQuote);

    // Divider
    p.setPen(QPen(QColor("#3A3128"), 2));
    p.drawLine(130, 815, 950, 815);

    // Book title & author
    QString titleStr = bookTitle.trimmed().isEmpty() ? QString("Libris") : bookTitle.trimmed();
    QFont titleFont("Sans", 22, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(accent);
    p.drawText(QRect(130, 840, 820, 44), Qt::AlignLeft | Qt::AlignVCenter, titleStr);

    QString metaStr = author.trimmed().isEmpty() ? QString("Libris • BlackBerry 10") : (author.trimmed() + QString::fromUtf8("  •  Libris"));
    QFont metaFont("Sans", 16);
    p.setFont(metaFont);
    p.setPen(QColor("#9C9488"));
    p.drawText(QRect(130, 892, 820, 36), Qt::AlignLeft | Qt::AlignVCenter, metaStr);
    p.end();

    QString outDir = "/accounts/1000/shared/photos";
    if (!QDir().exists(outDir)) {
        outDir = persistentDataDir();
    }
    QDir().mkpath(outDir);
    QString outPath = outDir + QString("/Libris_Quote_%1.png").arg(QDateTime::currentMSecsSinceEpoch());
    if (img.save(outPath, "PNG")) {
        return outPath;
    }
    return QString();
}

void BookManager::onBookUploadedFromWifi(const QString &filePath, const QString &fileName, qint64 fileSize)
{
    qDebug() << "[BookManager] Book received via WiFi Drop:" << fileName << "(" << fileSize << "bytes) at" << filePath;
    registerBookFile(filePath);
}

// =============================================================================
// CUSTOM SHELVES / COLLECTIONS
// =============================================================================

void BookManager::loadCustomShelvesFromSettings(QSettings &settings)
{
    m_customShelves = settings.value("shelves/list").toStringList();
    m_shelfBooks.clear();
    settings.beginGroup("shelvesBooks");
    for (int i = 0; i < m_customShelves.size(); ++i) {
        const QString &s = m_customShelves.at(i);
        m_shelfBooks[s] = settings.value(s).toStringList();
    }
    settings.endGroup();
}

void BookManager::saveCustomShelvesToSettings(QSettings &settings)
{
    settings.setValue("shelves/list", m_customShelves);
    settings.remove("shelvesBooks");
    settings.beginGroup("shelvesBooks");
    for (int i = 0; i < m_customShelves.size(); ++i) {
        const QString &s = m_customShelves.at(i);
        settings.setValue(s, m_shelfBooks.value(s));
    }
    settings.endGroup();
}

QStringList BookManager::customShelves() const
{
    return m_customShelves;
}

bool BookManager::createShelf(const QString &name)
{
    QString trimmed = name.trimmed();
    if (trimmed.isEmpty() || m_customShelves.contains(trimmed)) {
        return false;
    }
    m_customShelves.append(trimmed);
    m_shelfBooks[trimmed] = QStringList();
    emit customShelvesChanged();
    saveSettings();
    return true;
}

bool BookManager::deleteShelf(const QString &name)
{
    if (!m_customShelves.contains(name)) {
        return false;
    }
    m_customShelves.removeAll(name);
    m_shelfBooks.remove(name);
    if (m_categoryFilter == ("shelf:" + name)) {
        m_categoryFilter = "all";
        emit categoryFilterChanged();
        applyFilterAndSort();
    }
    emit customShelvesChanged();
    saveSettings();
    return true;
}

bool BookManager::renameShelf(const QString &oldName, const QString &newName)
{
    QString trimmed = newName.trimmed();
    if (trimmed.isEmpty() || !m_customShelves.contains(oldName) || m_customShelves.contains(trimmed)) {
        return false;
    }
    int idx = m_customShelves.indexOf(oldName);
    m_customShelves[idx] = trimmed;
    QStringList books = m_shelfBooks.take(oldName);
    m_shelfBooks[trimmed] = books;
    if (m_categoryFilter == ("shelf:" + oldName)) {
        m_categoryFilter = "shelf:" + trimmed;
        emit categoryFilterChanged();
    }
    emit customShelvesChanged();
    saveSettings();
    return true;
}

bool BookManager::toggleBookInShelf(const QString &filePath, const QString &shelfName)
{
    if (filePath.isEmpty() || !m_customShelves.contains(shelfName)) {
        return false;
    }
    QStringList books = m_shelfBooks.value(shelfName);
    bool added = false;
    if (books.contains(filePath)) {
        books.removeAll(filePath);
        added = false;
    } else {
        books.append(filePath);
        added = true;
    }
    m_shelfBooks[shelfName] = books;
    emit customShelvesChanged();
    if (m_categoryFilter == ("shelf:" + shelfName)) {
        applyFilterAndSort();
    }
    saveSettings();
    return added;
}

bool BookManager::isBookInShelf(const QString &filePath, const QString &shelfName) const
{
    return m_shelfBooks.value(shelfName).contains(filePath);
}

QStringList BookManager::getBookShelves(const QString &filePath) const
{
    QStringList res;
    for (int i = 0; i < m_customShelves.size(); ++i) {
        const QString &s = m_customShelves.at(i);
        if (m_shelfBooks.value(s).contains(filePath)) {
            res.append(s);
        }
    }
    return res;
}

int BookManager::getShelfBookCount(const QString &shelfName) const
{
    return m_shelfBooks.value(shelfName).size();
}

// =============================================================================
// CUSTOM .TTF / .OTF FONTS
// =============================================================================

QVariantList BookManager::customFonts() const
{
    return m_customFonts;
}

void BookManager::refreshCustomFonts()
{
    m_customFonts.clear();
    QStringList searchDirs;
    searchDirs << "/accounts/1000/shared/documents/fonts"
               << "/accounts/1000/shared/books/fonts"
               << "/accounts/1000/removable/sdcard/fonts"
               << "/accounts/1000/removable/sdcard2/fonts";

    QSet<QString> seenNames;
    QStringList filters;
    filters << "*.ttf" << "*.TTF" << "*.otf" << "*.OTF";

    for (int i = 0; i < searchDirs.size(); ++i) {
        QDir dir(searchDirs.at(i));
        if (!dir.exists()) continue;
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);
        for (int j = 0; j < files.size(); ++j) {
            const QFileInfo &fi = files.at(j);
            QString baseName = fi.completeBaseName();
            QString lowerKey = baseName.toLower();
            if (seenNames.contains(lowerKey)) continue;
            seenNames.insert(lowerKey);

            QVariantMap entry;
            entry["name"] = baseName;
            entry["filePath"] = fi.absoluteFilePath();
            entry["value"] = QString("custom:%1").arg(fi.absoluteFilePath());
            entry["format"] = fi.suffix().toLower() == "otf" ? "opentype" : "truetype";
            m_customFonts.append(entry);
        }
    }
    emit customFontsChanged();
}

QString BookManager::currentFontFaceCss() const
{
    if (!m_fontFamily.startsWith("custom:")) {
        return QString();
    }
    QString fontPath = m_fontFamily.mid(7);
    QFileInfo fi(fontPath);
    if (!fi.exists() || !fi.isFile()) {
        return QString();
    }
    QString fmt = fi.suffix().toLower() == "otf" ? "opentype" : "truetype";
    QString fileUrl = QString::fromUtf8(QUrl::fromLocalFile(fi.absoluteFilePath()).toEncoded());
    return QString("@font-face { font-family: 'LibrisCustomFont'; src: url('%1') format('%2'); } ").arg(fileUrl, fmt);
}

QString BookManager::currentFontCss() const
{
    if (m_fontFamily.startsWith("custom:")) {
        QString fontPath = m_fontFamily.mid(7);
        if (QFile::exists(fontPath)) {
            return "font-family: 'LibrisCustomFont', serif !important; ";
        }
    }
    if (m_fontFamily == "serif") {
        return "font-family: Georgia, 'Palatino Linotype', 'Book Antiqua', Palatino, serif !important; ";
    } else if (m_fontFamily == "times") {
        return "font-family: 'Times New Roman', Times, serif !important; ";
    } else if (m_fontFamily == "garamond") {
        return "font-family: Garamond, Baskerville, 'Baskerville Old Face', 'Hoefler Text', serif !important; ";
    } else if (m_fontFamily == "monospace") {
        return "font-family: 'Courier New', Courier, Menlo, Consolas, monospace !important; ";
    } else if (m_fontFamily == "original") {
        return "";
    }
    return "font-family: 'Slate Pro', -apple-system, 'Helvetica Neue', 'Segoe UI', Arial, sans-serif !important; ";
}

QString BookManager::currentFontDisplayName() const
{
    if (m_fontFamily.startsWith("custom:")) {
        return QFileInfo(m_fontFamily.mid(7)).completeBaseName();
    }
    if (m_fontFamily == "serif") return "Serif";
    if (m_fontFamily == "garamond") return "Garamond";
    if (m_fontFamily == "times") return "Times";
    if (m_fontFamily == "monospace") return "Mono";
    if (m_fontFamily == "original") return "EPUB";
    return "Sans";
}

// =============================================================================
// WARM AMBER NIGHT FILTER & AUTO-NIGHT THEME
// =============================================================================

bool BookManager::warmFilterEnabled() const
{
    return m_warmFilterEnabled;
}

void BookManager::setWarmFilterEnabled(bool enabled)
{
    if (m_warmFilterEnabled != enabled) {
        m_warmFilterEnabled = enabled;
        emit warmFilterChanged();
        saveSettings();
    }
}

int BookManager::warmFilterIntensity() const
{
    return m_warmFilterIntensity;
}

void BookManager::setWarmFilterIntensity(int intensity)
{
    int clamped = qMax(10, qMin(60, intensity));
    if (m_warmFilterIntensity != clamped) {
        m_warmFilterIntensity = clamped;
        emit warmFilterChanged();
        saveSettings();
    }
}

void BookManager::toggleWarmFilter()
{
    setWarmFilterEnabled(!m_warmFilterEnabled);
}

QString BookManager::normalizedBrightnessFormat() const
{
    QString f = m_currentFormat.trimmed().toLower();
    if (f == "azw" || f == "azw3" || f == "kf8" || f == "prc") return "mobi";
    if (f == "djv") return "djvu";
    if (f.isEmpty()) return "epub";
    return f;
}

int BookManager::readerBrightness() const
{
    return m_formatBrightness.value(normalizedBrightnessFormat(), 100);
}

void BookManager::setReaderBrightness(int brightness)
{
    int clamped = qMax(20, qMin(100, brightness));
    QString fmt = normalizedBrightnessFormat();
    if (m_formatBrightness.value(fmt, 100) != clamped) {
        m_formatBrightness.insert(fmt, clamped);
        emit readerBrightnessChanged();
        saveSettings();
    }
}

bool BookManager::autoNightTheme() const
{
    return m_autoNightTheme;
}

void BookManager::setAutoNightTheme(bool enabled)
{
    if (m_autoNightTheme != enabled) {
        m_autoNightTheme = enabled;
        emit autoNightThemeChanged();
        if (m_autoNightTheme) {
            checkAutoNightTheme();
        }
        saveSettings();
    }
}

void BookManager::checkAutoNightTheme()
{
    if (!m_autoNightTheme) return;
    int h = QTime::currentTime().hour();
    if (h >= 20 || h < 6) {
        if (m_theme != "dark") {
            setTheme("dark");
        }
    } else {
        if (m_theme == "dark") {
            setTheme("sepia");
        }
    }
}

// =============================================================================
// DAILY HIGHLIGHT RECALL & SHAREABLE QUOTE CARD PNG EXPORT
// =============================================================================

QVariantMap BookManager::dailyRecallQuote() const
{
    if (m_allHighlights.isEmpty()) {
        return QVariantMap();
    }
    int idx = QDate::currentDate().dayOfYear() % m_allHighlights.size();
    return m_allHighlights.at(idx);
}

QString BookManager::exportQuoteCardImage(const QString &quoteText, const QString &bookTitle, const QString &author, const QString &note)
{
    QString trimmedQuote = quoteText.trimmed();
    if (trimmedQuote.isEmpty()) {
        return QString();
    }

    const int W = 1080;
    const int H = 1080;
    QImage img(W, H, QImage::Format_ARGB32);
    img.fill(QColor(10, 9, 8));

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // Warm obsidian background gradient
    QLinearGradient bgGrad(0, 0, W, H);
    bgGrad.setColorAt(0.0, QColor(28, 24, 19));
    bgGrad.setColorAt(1.0, QColor(8, 7, 6));
    painter.fillRect(0, 0, W, H, bgGrad);

    // Outer subtle hairline & inner brass frame
    painter.setPen(QPen(QColor(42, 37, 30), 2));
    painter.drawRect(44, 44, W - 88, H - 88);
    painter.setPen(QPen(QColor(212, 163, 89, 140), 2));
    painter.drawRect(60, 60, W - 120, H - 120);

    // Decorative Brass Opening Quote Mark
    QFont markFont("Georgia", 112, QFont::Bold);
    painter.setFont(markFont);
    painter.setPen(QColor(212, 163, 89));
    painter.drawText(QRect(110, 95, 160, 140), Qt::AlignLeft | Qt::AlignTop, QString::fromUtf8("“"));

    // Dynamic quote point size
    int len = trimmedQuote.length();
    int ptSize = (len <= 140) ? 42 : ((len <= 280) ? 34 : 27);
    QFont quoteFont("Georgia", ptSize);
    painter.setFont(quoteFont);
    painter.setPen(QColor(245, 239, 230));

    QRect quoteRect(116, 235, W - 232, note.trimmed().isEmpty() ? 575 : 460);
    QString displayQuote = trimmedQuote;
    if (displayQuote.length() > 520) {
        displayQuote = displayQuote.left(517) + "...";
    }
    painter.drawText(quoteRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, displayQuote);

    // Optional personal note
    QString trimmedNote = note.trimmed();
    if (!trimmedNote.isEmpty()) {
        QFont noteFont("Slate Pro", 22, QFont::Normal, true);
        painter.setFont(noteFont);
        painter.setPen(QColor(182, 173, 160));
        QRect noteRect(116, 715, W - 232, 110);
        painter.drawText(noteRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                         QString::fromUtf8("Ghi chú: ") + trimmedNote);
    }

    // Brass Gold Footer Divider
    painter.setPen(QPen(QColor(212, 163, 89, 180), 3));
    painter.drawLine(116, 865, W - 116, 865);

    // Book Title & Author (Bottom-Left)
    QString safeTitle = bookTitle.trimmed().isEmpty() ? QString("Libris") : bookTitle.trimmed();
    QFont titleFont("Slate Pro", 26, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(QColor(245, 239, 230));
    QFontMetrics fmTitle(titleFont);
    painter.drawText(QRect(116, 892, 620, 44), Qt::AlignLeft | Qt::AlignVCenter,
                     fmTitle.elidedText(safeTitle, Qt::ElideRight, 610));

    QString safeAuthor = author.trimmed();
    if (!safeAuthor.isEmpty()) {
        QFont authorFont("Slate Pro", 20);
        painter.setFont(authorFont);
        painter.setPen(QColor(156, 148, 136));
        QFontMetrics fmAuth(authorFont);
        painter.drawText(QRect(116, 940, 620, 36), Qt::AlignLeft | Qt::AlignVCenter,
                         fmAuth.elidedText(safeAuthor, Qt::ElideRight, 610));
    }

    // Mini Libris Emblem + LIBRIS Wordmark (Bottom-Right)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(245, 239, 230));
    painter.drawRect(808, 898, 10, 40);
    painter.drawRect(808, 928, 30, 10);
    painter.setBrush(QColor(212, 163, 89));
    QPolygon ribbonPoly;
    ribbonPoly << QPoint(825, 898) << QPoint(837, 898) << QPoint(837, 922) << QPoint(831, 916) << QPoint(825, 922);
    painter.drawPolygon(ribbonPoly);

    QFont brandFont("Slate Pro", 22, QFont::Bold);
    brandFont.setLetterSpacing(QFont::AbsoluteSpacing, 5);
    painter.setFont(brandFont);
    painter.setPen(QColor(230, 192, 123));
    painter.drawText(QRect(850, 898, 130, 44), Qt::AlignLeft | Qt::AlignVCenter, "LIBRIS");

    painter.end();

    QString outDir = "/accounts/1000/shared/photos";
    QDir().mkpath(outDir);
    if (!QFileInfo(outDir).isWritable()) {
        outDir = persistentDataDir();
        QDir().mkpath(outDir);
    }
    QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString outPath = outDir + QString("/Libris_Quote_%1.png").arg(stamp);
    if (img.save(outPath, "PNG")) {
        return outPath;
    }
    return QString();
}


// =============================================================================
// LIFECYCLE-SAFE VOLUME KEY PAGE TURN
// =============================================================================

bool BookManager::volumeKeyTurnEnabled() const
{
    return m_volumeKeyTurnEnabled;
}

void BookManager::setVolumeKeyTurnEnabled(bool enabled)
{
    if (m_volumeKeyTurnEnabled != enabled) {
        m_volumeKeyTurnEnabled = enabled;
        emit volumeKeyTurnEnabledChanged();
        updateVolumeKeyWatchers();
        saveSettings();
    }
}

void BookManager::updateVolumeKeyWatchers()
{
    bool shouldWatch = m_volumeKeyTurnEnabled && m_isReaderActive && !m_isAppSleeping && (!m_tts || !m_tts->isActive());
    if (shouldWatch) {
        if (!m_volUpWatcher) {
            m_volUpWatcher = new bb::multimedia::MediaKeyWatcher(bb::multimedia::MediaKey::VolumeUp, this);
            connect(m_volUpWatcher, SIGNAL(shortPress(bb::multimedia::MediaKey::Type)),
                    this, SLOT(onVolumeUpPressed(bb::multimedia::MediaKey::Type)));
        }
        if (!m_volDownWatcher) {
            m_volDownWatcher = new bb::multimedia::MediaKeyWatcher(bb::multimedia::MediaKey::VolumeDown, this);
            connect(m_volDownWatcher, SIGNAL(shortPress(bb::multimedia::MediaKey::Type)),
                    this, SLOT(onVolumeDownPressed(bb::multimedia::MediaKey::Type)));
        }
    } else {
        if (m_volUpWatcher) {
            disconnect(m_volUpWatcher, 0, this, 0);
            delete m_volUpWatcher;
            m_volUpWatcher = NULL;
        }
        if (m_volDownWatcher) {
            disconnect(m_volDownWatcher, 0, this, 0);
            delete m_volDownWatcher;
            m_volDownWatcher = NULL;
        }
    }
}

void BookManager::onVolumeUpPressed(bb::multimedia::MediaKey::Type key)
{
    Q_UNUSED(key);
    if (m_isReaderActive && !m_isAppSleeping && m_volumeKeyTurnEnabled) {
        if (m_invertVolumeKeys) emit volumePageNextRequested();
        else emit volumePagePrevRequested();
    }
}

void BookManager::onVolumeDownPressed(bb::multimedia::MediaKey::Type key)
{
    Q_UNUSED(key);
    if (m_isReaderActive && !m_isAppSleeping && m_volumeKeyTurnEnabled) {
        if (m_invertVolumeKeys) emit volumePagePrevRequested();
        else emit volumePageNextRequested();
    }
}

void BookManager::onTtsActiveChangedForVolume(bool active)
{
    Q_UNUSED(active);
    updateVolumeKeyWatchers();
}

bool BookManager::focusModeEnabled() const
{
    return m_focusModeEnabled;
}

void BookManager::setFocusModeEnabled(bool enabled)
{
    if (m_focusModeEnabled != enabled) {
        m_focusModeEnabled = enabled;
        emit focusModeChanged();
        saveSettings();
    }
}

void BookManager::toggleFocusMode()
{
    setFocusModeEnabled(!m_focusModeEnabled);
}

static bool seriesBookLessThan(const QVariantMap &a, const QVariantMap &b)
{
    int ia = a.value("seriesIndex").toInt();
    int ib = b.value("seriesIndex").toInt();
    if (ia > 0 && ib > 0 && ia != ib) return ia < ib;
    if (ia > 0 && ib <= 0) return true;
    if (ia <= 0 && ib > 0) return false;
    return a.value("title").toString().compare(b.value("title").toString(), Qt::CaseInsensitive) < 0;
}

QVariantMap BookManager::nextSeriesBook(const QString &filePath) const
{
    QString targetPath = filePath.trimmed().isEmpty() ? m_currentFilePath : filePath.trimmed();
    if (targetPath.isEmpty()) return QVariantMap();

    QString targetSeries;
    int targetSeriesIdx = 0;

    for (int i = 0; i < m_allBooks.size(); ++i) {
        const QVariantMap &b = m_allBooks.at(i);
        if (b.value("filePath").toString() == targetPath) {
            targetSeries = b.value("series").toString().trimmed();
            targetSeriesIdx = b.value("seriesIndex").toInt();
            break;
        }
    }
    if (targetSeries.isEmpty() && targetPath == m_currentFilePath) {
        targetSeries = m_currentBookSeries.trimmed();
        targetSeriesIdx = m_currentBookSeriesIndex;
    }
    if (targetSeries.isEmpty()) return QVariantMap();

    QList<QVariantMap> sameSeries;
    for (int i = 0; i < m_allBooks.size(); ++i) {
        const QVariantMap &b = m_allBooks.at(i);
        if (b.value("series").toString().trimmed().compare(targetSeries, Qt::CaseInsensitive) == 0) {
            sameSeries.append(b);
        }
    }
    if (sameSeries.size() < 2) return QVariantMap();

    qSort(sameSeries.begin(), sameSeries.end(), seriesBookLessThan);

    int curPos = -1;
    for (int i = 0; i < sameSeries.size(); ++i) {
        if (sameSeries.at(i).value("filePath").toString() == targetPath) {
            curPos = i;
            break;
        }
    }
    if (curPos >= 0 && curPos + 1 < sameSeries.size()) {
        return sameSeries.at(curPos + 1);
    }
    if (curPos < 0 && targetSeriesIdx > 0) {
        for (int i = 0; i < sameSeries.size(); ++i) {
            if (sameSeries.at(i).value("seriesIndex").toInt() > targetSeriesIdx) {
                return sameSeries.at(i);
            }
        }
    }
    return QVariantMap();
}

QVariantList BookManager::allBooksSnapshot() const
{
    QVariantList out;
    for (int i = 0; i < m_allBooks.size(); ++i) {
        out.append(m_allBooks.at(i));
    }
    return out;
}

QString BookManager::createBackupArchive()
{
    syncToPersistentStorage();

    QString dirPath = persistentDataDir();
    QDir().mkpath(dirPath);
    QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString outPath = dirPath + QString("/libris_backup_%1.librisbackup").arg(stamp);

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    QByteArray outUtf8 = outPath.toUtf8();
    if (!mz_zip_writer_init_file(&zip, outUtf8.constData(), 0)) {
        return QString();
    }

    QStringList files;
    files << "libris_userdata.ini"
          << "libris_userdata.json"
          << "libris_highlights.json"
          << "libris_bookmarks.json"
          << "libris_vocab.json"
          << "library_books.dat";

    int addedCount = 0;
    for (int i = 0; i < files.size(); ++i) {
        QString full = dirPath + "/" + files.at(i);
        if (QFile::exists(full)) {
            QByteArray nameUtf8 = files.at(i).toUtf8();
            QByteArray fullUtf8 = full.toUtf8();
            if (mz_zip_writer_add_file(&zip, nameUtf8.constData(), fullUtf8.constData(), NULL, 0, MZ_BEST_COMPRESSION)) {
                addedCount++;
            }
        }
    }

    if (!mz_zip_writer_finalize_archive(&zip)) {
        mz_zip_writer_end(&zip);
        QFile::remove(outPath);
        return QString();
    }
    mz_zip_writer_end(&zip);

    if (addedCount == 0) {
        QFile::remove(outPath);
        return QString();
    }
    return outPath;
}

bool BookManager::restoreBackupArchive(const QString &archivePath)
{
    QString cleanPath = archivePath.trimmed();
    if (cleanPath.startsWith("file://")) {
        cleanPath = QUrl(cleanPath).toLocalFile();
    }
    if (cleanPath.isEmpty() || !QFile::exists(cleanPath)) {
        return false;
    }

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    QByteArray inUtf8 = cleanPath.toUtf8();
    if (!mz_zip_reader_init_file(&zip, inUtf8.constData(), 0)) {
        return false;
    }

    QString dirPath = persistentDataDir();
    QDir().mkpath(dirPath);

    QStringList allowed;
    allowed << "libris_userdata.ini"
            << "libris_userdata.json"
            << "libris_highlights.json"
            << "libris_bookmarks.json"
            << "libris_vocab.json"
            << "library_books.dat";

    int extracted = 0;
    mz_uint numFiles = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&zip, i, &st)) continue;
        QString fname = QFileInfo(QString::fromUtf8(st.m_filename)).fileName();
        if (!allowed.contains(fname)) continue;

        QString dest = dirPath + "/" + fname;
        QByteArray destUtf8 = dest.toUtf8();
        if (mz_zip_reader_extract_to_file(&zip, i, destUtf8.constData(), 0)) {
            extracted++;
        }
    }
    mz_zip_reader_end(&zip);

    if (extracted == 0) {
        return false;
    }

    restoreFromPersistentStorage();
    m_dictManager.reloadVocab();
    return true;
}



