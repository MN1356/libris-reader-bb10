#include "BookManager.hpp"
#include "BookWorkers.hpp"
#include "OpdsManager.hpp"
#include <bb/cascades/Application>
#include <bb/cascades/ThemeSupport>
#include <bb/cascades/VisualStyle>
#include <bb/cascades/controls/control.h>
#include <bb/cascades/layouts/stacklayoutproperties.h>
#include <bb/device/DisplayInfo>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QCryptographicHash>
#include <QDebug>
#include <QtGui/QImage>
#include <QCoreApplication>
#include <QEvent>
#include <QLocale>
#include <QTime>
#include <QDate>
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
    QRegExp reBrackets("^\\[([^\\]#]+?)(?:[\\s_\\-–#]+|[\\s_\\-–#]+(?:Tập|Quyển|Book|Vol|Volume|Part|Phần|#)\\s*)(\\d+)\\]", Qt::CaseInsensitive);
    if (reBrackets.indexIn(title) != -1 || reBrackets.indexIn(fileName) != -1) {
        outSeries = reBrackets.cap(1).trimmed();
        outIndex = reBrackets.cap(2).toInt();
        return;
    }

    // Pattern 2: (Series Name #1) or (Series Name, Book 1)
    QRegExp reParens("\\(([^\\)#]+?)(?:[\\s_\\-–#,]+|[\\s_\\-–#,]+(?:Tập|Quyển|Book|Vol|Volume|Part|Phần|#)\\s*)(\\d+)\\)", Qt::CaseInsensitive);
    if (reParens.indexIn(title) != -1 || reParens.indexIn(fileName) != -1) {
        outSeries = reParens.cap(1).trimmed();
        outIndex = reParens.cap(2).toInt();
        return;
    }

    // Pattern 3: Series Name - Tập 1 / Book 1 / Vol 1 - Title
    QRegExp reDash("^(.*?)\\s*[-–]\\s*(?:Tập|Quyển|Book|Vol|Volume|Part|Phần|#)\\s*(\\d+)\\s*[-–]", Qt::CaseInsensitive);
    if (reDash.indexIn(title) != -1 || reDash.indexIn(fileName) != -1) {
        outSeries = reDash.cap(1).trimmed();
        outIndex = reDash.cap(2).toInt();
        return;
    }
}

static bool isGenericChapterTitle(const QString &title);
static QString combineChapterTitle(const QString &baseTitle, const QString &subTitle, int chapterNumber = -1);

static QString unwrapBookPath(const QString &raw)
{
    QString cur = raw.trimmed();
    if (cur.isEmpty()) return QString();

    // Check if cur itself is already a clean valid file path
    if (cur.startsWith('/') && (cur.contains(".epub", Qt::CaseInsensitive) ||
                                cur.contains(".pdf", Qt::CaseInsensitive) ||
                                cur.contains(".txt", Qt::CaseInsensitive) ||
                                cur.contains(".fb2", Qt::CaseInsensitive) ||
                                cur.contains(".djvu", Qt::CaseInsensitive) ||
                                QFile::exists(cur))) {
        return cur;
    }

    // Attempt recursive base64 unwrapping (handles multiple layers of base64 encoding from legacy saves)
    for (int step = 0; step < 12; ++step) {
        if (cur.startsWith('/') && cur.contains('/')) {
            if (cur.contains(".epub", Qt::CaseInsensitive) ||
                cur.contains(".pdf", Qt::CaseInsensitive) ||
                cur.contains(".txt", Qt::CaseInsensitive) ||
                cur.contains(".fb2", Qt::CaseInsensitive) ||
                cur.contains(".djvu", Qt::CaseInsensitive) ||
                QFile::exists(cur) ||
                cur.contains("/accounts/")) {
                break;
            }
        }

        QByteArray latin = cur.toLatin1();
        int rem = latin.size() % 4;
        if (rem != 0) {
            latin.append(QByteArray(4 - rem, '='));
        }

        QByteArray dec = QByteArray::fromBase64(latin);
        if (dec.isEmpty()) break;

        QString nextStr = QString::fromUtf8(dec);
        if (nextStr.isEmpty() || nextStr == cur) break;

        bool printable = true;
        for (int i = 0; i < qMin(nextStr.size(), 40); ++i) {
            ushort u = nextStr.at(i).unicode();
            if (u < 32 && u != '\t' && u != '\n' && u != '\r') {
                printable = false;
                break;
            }
        }
        if (!printable) break;

        cur = nextStr.trimmed();
    }

    if (cur.contains('%')) {
        cur = QUrl::fromPercentEncoding(cur.toUtf8());
    }

    if (cur.contains("/accounts/")) {
        cur = cur.mid(cur.indexOf("/accounts/"));
    } else if (cur.contains("accounts/")) {
        cur = "/" + cur.mid(cur.indexOf("accounts/"));
    }

    return cur;
}

BookManager::BookManager(QObject *parent)
    : QObject(parent)
    , m_translator(new AppTranslator(this))
    , m_language("vi")
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
    , m_currentPageInChapter(1)
    , m_totalPagesInChapter(1)
    , m_currentScrollPos(0)
    , m_shouldRestorePageOnLoad(false)
    , m_pdfInverted(false)
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
    , m_marginTop(16)
    , m_marginBottom(16)
    , m_marginLeft(16)
    , m_marginRight(16)
    , m_totalReadingSeconds(0)
    , m_todayReadingSeconds(0)
    , m_todayDate(QDate::currentDate().toString("yyyy-MM-dd"))
    , m_readingHistoryModel(new bb::cascades::GroupDataModel(QStringList() << "sortKey", this))
    , m_isSearching(false)
    , m_isReaderActive(false)
    , m_isPdfReflow(false)
    , m_hasPdfText(false)
    , m_ttsAutoAdvance(false)
    , m_searchWorker(NULL)
    , m_scanWorker(NULL)
    , m_isSendingLog(false)
    , m_lastLogStatus("")
    , m_devServerIp("192.168.0.108")
    , m_logNetworkManager(new QNetworkAccessManager(this))
    , m_devLogReply(NULL)
    , m_cloudLogReply(NULL)
    , m_isAppSleeping(false)
    , m_opdsManager(NULL)
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
    m_cacheDir = "/accounts/1000/shared/documents/.berryreader_covers";
    QDir().mkpath(m_cacheDir);

    // Initialize NSRReaderCore engine for PDF & DjVu
    m_pdfCore = new NSRReaderCore(NULL, NULL, this);

    // Detect actual hardware screen width for 100% pixel-perfect fit
    bb::device::DisplayInfo display;
    m_screenWidth = display.pixelSize().width();
    if (m_screenWidth <= 0) {
        m_screenWidth = 720;
    }
    m_pdfCore->setScreenWidth(m_screenWidth);

    connect(m_pdfCore, SIGNAL(pageRendered(int)), this, SLOT(onPdfPageRendered(int)));
    connect(m_pdfCore, SIGNAL(needIndicator(bool)), this, SLOT(onPdfIndicatorRequested(bool)));
    connect(m_pdfCore, SIGNAL(errorWhileOpening(NSRAbstractDocument::NSRDocumentError)),
            this, SLOT(onPdfError(NSRAbstractDocument::NSRDocumentError)));
    connect(m_pdfCore, SIGNAL(sessionFileOpened(QString)), this, SLOT(onPdfFileOpened(QString)));

    // Initialize TTS Hybrid Engine
    m_tts = new TtsManager(this);
    connect(m_tts, SIGNAL(chapterFinished()), this, SLOT(onTtsChapterFinished()));

    // Watchdog timer for fail-safe book opening and page rendering
    m_watchdogTimer.setSingleShot(true);
    connect(&m_watchdogTimer, SIGNAL(timeout()), this, SLOT(onWatchdogTimeout()));

    // Watchdog timer for fail-safe library scanning (prevents frozen startup overlay)
    m_scanWatchdogTimer.setSingleShot(true);
    connect(&m_scanWatchdogTimer, SIGNAL(timeout()), this, SLOT(onScanWatchdogTimeout()));

    // Initial library scan scheduled shortly after UI loads to display startup screen
    QTimer::singleShot(150, this, SLOT(scanLibrary()));
}

BookManager::~BookManager()
{
    m_scanWatchdogTimer.stop();
    m_watchdogTimer.stop();
    saveCurrentProgress();
    syncToPersistentStorage();

    if (m_searchWorker) {
        m_searchWorker->cancel();
        m_searchWorker->wait();
        delete m_searchWorker;
        m_searchWorker = NULL;
    }
    if (m_scanWorker) {
        m_scanWorker->cancel();
        m_scanWorker->wait();
        delete m_scanWorker;
        m_scanWorker = NULL;
    }
    if (m_pdfCore) {
        m_pdfCore->prepareForDestruction();
    }
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
    QSettings settings("BerryLife", "BerryReader");
    return settings.value(QString("favorites/%1").arg(filePath), false).toBool();
}

bool BookManager::toggleFavorite(const QString &filePath)
{
    if (filePath.isEmpty()) return false;
    QSettings settings("BerryLife", "BerryReader");
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
    syncToPersistentStorage();
    return next;
}

bool BookManager::isReadingList(const QString &filePath) const
{
    if (filePath.isEmpty()) return false;
    QSettings settings("BerryLife", "BerryReader");
    return settings.value(QString("readingList/%1").arg(filePath), false).toBool();
}

bool BookManager::toggleReadingList(const QString &filePath)
{
    if (filePath.isEmpty()) return false;
    QSettings settings("BerryLife", "BerryReader");
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
    syncToPersistentStorage();
    return next;
}

bool BookManager::toggleBookCompleted(const QString &filePath)
{
    if (filePath.isEmpty()) return false;
    QSettings settings("BerryLife", "BerryReader");
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
    syncToPersistentStorage();
    return (newProg >= 100);
}

void BookManager::removeBookFromLibrary(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    QSettings settings("BerryLife", "BerryReader");
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
    syncToPersistentStorage();
}

void BookManager::removeFromRecent(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    QSettings settings("BerryLife", "BerryReader");
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
    QSettings settings("BerryLife", "BerryReader");

    // 1. Remove all lastReadTime, progressPct and reading state keys
    QStringList allKeys = settings.allKeys();
    foreach (const QString &key, allKeys) {
        if (key.startsWith("lastReadTime/") ||
            key.startsWith("progressPct/") ||
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

    // 3. Reset progress in memory
    for (int i = 0; i < m_allBooks.size(); ++i) {
        m_allBooks[i]["progress"] = 0;
        m_allBooks[i]["isCompleted"] = false;
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
    syncToPersistentStorage();
}

void BookManager::markAsUnread(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    QSettings settings("BerryLife", "BerryReader");
    settings.setValue(QString("progressPct/%1").arg(filePath), 0);
    settings.remove(QString("lastReadChapter/%1").arg(filePath));
    settings.remove(QString("lastReadScroll/%1").arg(filePath));
    settings.remove(QString("lastReadTargetScroll/%1").arg(filePath));
    settings.remove(QString("lastReadTargetPage/%1").arg(filePath));
    settings.setValue(QString("completed/%1").arg(filePath), false);

    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            m_allBooks[i]["progress"] = 0;
            m_allBooks[i]["isCompleted"] = false;
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
    } else {
        return QString();
    }

    if (rawBody.isEmpty()) {
        return QString();
    }

    // Dynamic colors according to selected theme
    QString bgColor = "#000000";
    QString textColor = "#DDDDDD";
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
    content.remove(QRegExp("<script[^>]*>[\\s\\S]*?</script>", Qt::CaseInsensitive));

    // Also collect any chapter-specific <style> tags from rawBody to preserve typography
    QRegExp styleRx("<style[^>]*>([\\s\\S]*?)</style>", Qt::CaseInsensitive);
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
            QString coverSrc = QString("file://%1").arg(m_currentCoverPath);
            QFile f(m_currentCoverPath);
            if (f.open(QIODevice::ReadOnly)) {
                QByteArray b = f.readAll();
                f.close();
                QString ext = QFileInfo(m_currentCoverPath).suffix().toLower();
                QString mime = (ext == "png") ? "image/png" : "image/jpeg";
                coverSrc = QString("data:%1;base64,%2").arg(mime).arg(QString::fromLatin1(b.toBase64().constData()));
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
    int colW = screenW - colGap;
    if (colW < 100) colW = screenW;

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

    if (m_readingMode == "paged") {
        html += "html, body { width: 100% !important; height: 100% !important; min-height: 100%; margin: 0 !important; padding: 0 !important; overflow: hidden !important; -webkit-user-select: text !important; user-select: text !important; } ";
        html += "#reader-viewport { "
             + QString("width: 100% !important; height: 100% !important; min-height: 100%; margin: 0; ")
             + QString("box-sizing: border-box; padding: %1px %2px %3px %4px; ")
                .arg(marginT).arg(marginR).arg(marginB).arg(marginL)
             + QString("overflow: hidden; ")
             + QString("-webkit-column-width: %1px; -webkit-column-gap: %2px; -webkit-column-fill: auto; ")
                .arg(colW).arg(colGap)
             + QString("column-width: %1px; column-gap: %2px; column-fill: auto; ")
                .arg(colW).arg(colGap)
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
    html += "mark.berry-search-match { background-color: #FFDE03 !important; color: #000000 !important; padding: 2px 1px; border-radius: 2px; } ";
    html += "mark.berry-search-current { background-color: #FF5722 !important; color: #FFFFFF !important; font-weight: bold; } ";
    html += "mark.berry-tts-active { background-color: rgba(0, 168, 223, 0.35) !important; color: inherit !important; border-radius: 4px; padding: 1px 3px; border-bottom: 2px solid #00A8DF !important; box-shadow: 0 0 10px rgba(0, 168, 223, 0.45); } ";
    html += "* { -webkit-touch-callout: none !important; } ";

    // User Highlights styles
    if (m_theme == "dark") {
        html += "mark.berry-highlight { cursor: pointer; border-radius: 2px; padding: 1px 2px; text-decoration: none !important; } ";
        html += "mark.berry-highlight.berry-hl-yellow { background-color: rgba(234, 179, 8, 0.45) !important; color: #FEF08A !important; border-bottom: 2px solid #EAB308; } ";
        html += "mark.berry-highlight.berry-hl-green { background-color: rgba(34, 197, 94, 0.45) !important; color: #BBF7D0 !important; border-bottom: 2px solid #22C55E; } ";
        html += "mark.berry-highlight.berry-hl-blue { background-color: rgba(56, 189, 248, 0.45) !important; color: #BAE6FD !important; border-bottom: 2px solid #38BDF8; } ";
        html += "mark.berry-highlight.berry-hl-pink { background-color: rgba(244, 114, 182, 0.45) !important; color: #FBCFE8 !important; border-bottom: 2px solid #F472B6; } ";
    } else {
        html += "mark.berry-highlight { cursor: pointer; border-radius: 2px; padding: 1px 2px; text-decoration: none !important; } ";
        html += "mark.berry-highlight.berry-hl-yellow { background-color: #FEF08A !important; color: #1E293B !important; } ";
        html += "mark.berry-highlight.berry-hl-green { background-color: #BBF7D0 !important; color: #14532D !important; } ";
        html += "mark.berry-highlight.berry-hl-blue { background-color: #BAE6FD !important; color: #0C4A6E !important; } ";
        html += "mark.berry-highlight.berry-hl-pink { background-color: #FBCFE8 !important; color: #831843 !important; } ";
    }

    html += "#berry-selection-bubble { "
            "position: fixed; "
            "display: none; "
            "z-index: 2147483647; "
            "background: rgba(24, 28, 36, 0.96); "
            "color: #F8FAFC; "
            "border: 1px solid rgba(255, 255, 255, 0.20); "
            "border-radius: 20px; "
            "box-shadow: 0 6px 20px rgba(0, 0, 0, 0.65), 0 1px 3px rgba(0, 0, 0, 0.4); "
            "padding: 2px 5px; "
            "white-space: nowrap; "
            "font-family: -apple-system, 'Helvetica Neue', 'Segoe UI', Arial, sans-serif; "
            "line-height: 1.0; "
            "-webkit-user-select: none; "
            "user-select: none; "
            "} ";
    html += ".berry-bubble-btn { "
            "display: inline-block; "
            "vertical-align: middle; "
            "height: 34px; "
            "line-height: 34px; "
            "padding: 0 8px; "
            "color: #F1F5F9; "
            "background: transparent; "
            "cursor: pointer; "
            "border-radius: 17px; "
            "text-decoration: none; "
            "text-align: center; "
            "box-sizing: border-box; "
            "-webkit-tap-highlight-color: transparent; "
            "} ";
    html += ".berry-bubble-btn:active { background-color: #00A8DF !important; color: #FFFFFF !important; } ";
    html += ".berry-bubble-btn svg { display: inline-block; vertical-align: middle; } ";
    html += ".berry-bubble-btn span { display: inline-block; vertical-align: middle; margin-left: 4px; font-size: 12px; font-weight: 600; } ";
    html += ".berry-bubble-sep { "
            "display: inline-block; "
            "width: 1px; "
            "height: 16px; "
            "background: rgba(255, 255, 255, 0.16); "
            "vertical-align: middle; "
            "margin: 0 1px; "
            "} ";
    html += "</style>";

    // Inlined original book styles & embedded fonts
    if (!bookCss.isEmpty() || !chapterStyles.isEmpty()) {
        html += "<style id=\"book-author-styles\">\n" + bookCss + "\n" + chapterStyles + "\n</style>";
    }

    // Adaptive Theme & Typography baseline
    QString fontCss;
    if (m_fontFamily == "serif") {
        fontCss = "font-family: Georgia, 'Palatino Linotype', 'Book Antiqua', Palatino, serif; ";
    } else if (m_fontFamily == "times") {
        fontCss = "font-family: 'Times New Roman', Times, serif; ";
    } else if (m_fontFamily == "garamond") {
        fontCss = "font-family: Garamond, Baskerville, 'Baskerville Old Face', 'Hoefler Text', serif; ";
    } else if (m_fontFamily == "monospace") {
        fontCss = "font-family: 'Courier New', Courier, Menlo, Consolas, monospace; ";
    } else if (m_fontFamily == "original") {
        fontCss = "";
    } else {
        fontCss = "font-family: 'Slate Pro', -apple-system, 'Helvetica Neue', 'Segoe UI', Arial, sans-serif; ";
    }

    html += QString("<style id=\"dynamic-theme-style\">");
    html += QString("body { ")
         + fontCss
         + QString("font-size: %1px; ").arg(m_fontSize)
         + QString("line-height: %1; ").arg(lh)
         + QString("background-color: %1 !important; ").arg(bgColor)
         + QString("color: %2 !important; ").arg(textColor)
         + "}";
    html += QString("a { color: %1 !important; text-decoration: none; }").arg(linkColor);
    html += "p:not([class]) { text-indent: 1.5em; margin: 0.5em 0; }";

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

    // JavaScript for pagination, touch swipe gestures, and tap zones
    int targetPage = (m_currentPageInChapter >= 1) ? m_currentPageInChapter : 1;
    int targetScroll = (m_currentScrollPos >= 0) ? m_currentScrollPos : 0;
    m_shouldRestorePageOnLoad = false;

    html += "<script>";
    html += QString("var readingMode = '%1';").arg(m_readingMode);
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
    html += "      var colW = Math.max(100, Math.round(w - gap));";
    html += "      vp.style.webkitColumnWidth = colW + 'px';";
    html += "      vp.style.columnWidth = colW + 'px';";
    html += "      vp.style.webkitColumnGap = gap + 'px';";
    html += "      vp.style.columnGap = gap + 'px';";
    html += "    }";
    html += "    totalPages = Math.max(1, Math.round(vp.scrollWidth / w));";
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
    html += "  var bubble = document.getElementById('berry-selection-bubble');";
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
    html += "  var bH = bubble.offsetHeight || 34;";
    html += "  var winW = window.innerWidth || document.documentElement.clientWidth || screenW;";
    html += "  var winH = window.innerHeight || document.documentElement.clientHeight || 720;";
    html += "  var midX = rect.left + (rect.width / 2);";
    html += "  var posX = midX - (bW / 2);";
    html += "  if (posX < 8) posX = 8;";
    html += "  if (posX + bW > winW - 8) posX = winW - bW - 8;";
    html += "  var posY = rect.top - bH - 12;";
    html += "  if (posY < 10) {";
    html += "    posY = rect.bottom + 12;";
    html += "    if (posY + bH > winH - 10) posY = winH - bH - 10;";
    html += "  }";
    html += "  bubble.style.left = Math.round(posX) + 'px';";
    html += "  bubble.style.top = Math.round(posY) + 'px';";
    html += "};";

    html += "function clearSelectionAndBubble() {";
    html += "  if (window.getSelection) window.getSelection().removeAllRanges();";
    html += "  var b = document.getElementById('berry-selection-bubble');";
    html += "  if (b) b.style.display = 'none';";
    html += "  if (window.navigator && window.navigator.cascades) {";
    html += "    window.navigator.cascades.postMessage('textSelected:');";
    html += "  }";
    html += "};";

    html += "function initSelectionBubble() {";
    html += "  var bubble = document.getElementById('berry-selection-bubble');";
    html += "  if (!bubble) return;";
    html += "  function getSelText() {";
    html += "    var s = window.getSelection ? window.getSelection().toString() : '';";
    html += "    return s ? s.trim() : '';";
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
    html += "  var marks = document.querySelectorAll('mark.berry-search-match');";
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
    html += "      mark.className = 'berry-search-match';";
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
    html += "  var marks = document.querySelectorAll('mark.berry-search-match');";
    html += "  for (var i = 0; i < marks.length; i++) marks[i].classList.remove('berry-search-current');";
    html += "  if (currentMatchIndex >= 0 && currentMatchIndex < marks.length) {";
    html += "    marks[currentMatchIndex].classList.add('berry-search-current');";
    html += "    marks[currentMatchIndex].scrollIntoView({ behavior: 'smooth', block: 'center' });";
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
    html += "  var marks = document.querySelectorAll('mark.berry-search-match');";
    html += "  if (index >= 0 && index < marks.length) {";
    html += "    currentMatchIndex = index;";
    html += "    highlightCurrentMatch(false);";
    html += "  }";
    html += "};";

    // =========================================================================
    // TTS Interactive Sentence Highlighter & Page Following
    // =========================================================================
    html += "function clearTtsHighlight() {\n";
    html += "  var oldMarks = document.querySelectorAll('mark.berry-tts-active');\n";
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
    html += "    if (parent && (parent.nodeName === 'SCRIPT' || parent.nodeName === 'STYLE' || parent.id === 'berry-selection-bubble')) continue;\n";
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
    html += "        mark.className = 'berry-tts-active';\n";
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
    html += "        if (curPage <= 1 && dx > 0) {\n";
    html += "          vp.scrollLeft = 0;\n";
    html += "          vp.style.webkitTransition = 'none';\n";
    html += "          vp.style.webkitTransform = 'translate3d(' + Math.round(dx * 0.35) + 'px, 0, 0)';\n";
    html += "        } else if (curPage >= totalPages && dx < 0) {\n";
    html += "          vp.scrollLeft = (totalPages - 1) * w;\n";
    html += "          vp.style.webkitTransition = 'none';\n";
    html += "          vp.style.webkitTransform = 'translate3d(' + Math.round(dx * 0.35) + 'px, 0, 0)';\n";
    html += "        } else {\n";
    html += "          vp.style.webkitTransition = 'none';\n";
    html += "          vp.style.webkitTransform = 'translate3d(0, 0, 0)';\n";
    html += "          vp.scrollLeft = Math.round(baseScroll - dx);\n";
    html += "        }\n";
    html += "      }\n";
    html += "    }\n";
    html += "  }\n";
    html += "}, false);\n";
    html += "window.addEventListener('touchend', function(e) {\n";
    html += "  if (e.changedTouches.length === 1) {\n";
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
    html += "        if (curPage <= 1 && dx > 0) {\n";
    html += "          vp.style.webkitTransition = '-webkit-transform 180ms cubic-bezier(0.25, 1, 0.5, 1)';\n";
    html += "          vp.style.webkitTransform = 'translate3d(0, 0, 0)';\n";
    html += "          if (dx > 30 && window.navigator && window.navigator.cascades) {\n";
    html += "            window.navigator.cascades.postMessage('prevChapter');\n";
    html += "          }\n";
    html += "          return;\n";
    html += "        }\n";
    html += "        if (curPage >= totalPages && dx < 0) {\n";
    html += "          vp.style.webkitTransition = '-webkit-transform 180ms cubic-bezier(0.25, 1, 0.5, 1)';\n";
    html += "          vp.style.webkitTransform = 'translate3d(0, 0, 0)';\n";
    html += "          if (dx < -30 && window.navigator && window.navigator.cascades) {\n";
    html += "            window.navigator.cascades.postMessage('nextChapter');\n";
    html += "          }\n";
    html += "          return;\n";
    html += "        }\n";
    html += "        var velocity = Math.abs(dx) / Math.max(1, dt);\n";
    html += "        var isFlick = (dt < 350 && velocity > 0.20);\n";
    html += "        var targetPage = curPage;\n";
    html += "        if (dx < -20 || (isFlick && dx < -10)) {\n";
    html += "          if (curPage < totalPages) targetPage = curPage + 1;\n";
    html += "        } else if (dx > 20 || (isFlick && dx > 10)) {\n";
    html += "          if (curPage > 1) targetPage = curPage - 1;\n";
    html += "        }\n";
    html += "        var targetScroll = (targetPage - 1) * w;\n";
    html += "        var dist = Math.abs(targetScroll - vp.scrollLeft);\n";
    html += "        var animDuration = Math.max(100, Math.min(200, Math.round(180 * (dist / w))));\n";
    html += "        smoothScrollTo(targetScroll, animDuration, function() {\n";
    html += "          curPage = targetPage;\n";
    html += "          updatePageInfo();\n";
    html += "        });\n";
    html += "      } else if (Math.abs(dx) > 18 && Math.abs(dx) > Math.abs(dy) * 0.55) {\n";
    html += "        animatePageTurn(dx < 0 ? 'next' : 'prev');\n";
    html += "      } else if (isTap) {\n";
    html += "        if (curX < w * 0.25 && curY > h * 0.15 && curY < h * 0.85) {\n";
    html += "          animatePageTurn('prev');\n";
    html += "        } else if (curX > w * 0.75 && curY > h * 0.15 && curY < h * 0.85) {\n";
    html += "          animatePageTurn('next');\n";
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
    html += "    mark.className = 'berry-highlight berry-hl-' + (color || 'yellow');\n";
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

    html += "function scrollToAnchor(name) {\n";
    html += "  if (!name || name.trim() === '') return;\n";
    html += "  var cleanName = name.trim();\n";
    html += "  if (cleanName.indexOf('#') === 0) cleanName = cleanName.substring(1);\n";
    html += "  if (cleanName === '') return;\n";
    html += "  var el = document.getElementById(cleanName);\n";
    html += "  if (!el) {\n";
    html += "    var list = document.getElementsByName(cleanName);\n";
    html += "    if (list && list.length > 0) el = list[0];\n";
    html += "  }\n";
    html += "  if (!el) {\n";
    html += "    try { el = document.querySelector('[id=\"' + cleanName + '\"], [name=\"' + cleanName + '\"]'); } catch(e){}\n";
    html += "  }\n";
    html += "  if (!el) return;\n";
    html += "  var vp = document.getElementById('reader-viewport');\n";
    html += "  if (!vp) return;\n";
    html += "  if (readingMode === 'paged') {\n";
    html += "    var w = window.innerWidth || document.documentElement.clientWidth || screenW;\n";
    html += "    if (w <= 0) w = 720;\n";
    html += "    var offsetLeft = 0;\n";
    html += "    var cur = el;\n";
    html += "    while (cur && cur !== vp) {\n";
    html += "      offsetLeft += cur.offsetLeft;\n";
    html += "      cur = cur.offsetParent;\n";
    html += "    }\n";
    html += "    var pageIdx = Math.floor(offsetLeft / w);\n";
    html += "    if (pageIdx >= 0) goToPage(pageIdx + 1);\n";
    html += "  } else {\n";
    html += "    el.scrollIntoView({ behavior: 'smooth', block: 'start' });\n";
    html += "    updatePageInfo();\n";
    html += "  }\n";
    html += "};\n";

    html += "function initEvents() {";
    html += "  var vp = document.getElementById('reader-viewport');";
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
        html += QString("  setTimeout(function() { scrollToAnchor('%1'); }, 80);\n").arg(m_pendingScrollAnchor);
        html += QString("  setTimeout(function() { scrollToAnchor('%1'); }, 300);\n").arg(m_pendingScrollAnchor);
    }
    html += "};";
    html += "document.addEventListener('DOMContentLoaded', initEvents, false);";
    html += "window.addEventListener('load', function() { setTimeout(initEvents, 50); }, false);";
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

    html += "</head><body>";
    html += "<div id=\"reader-viewport\">";
    html += content;
    html += "</div>";

    // Floating selection bubble anchored directly to selected text line
    bool isVi = (m_language == "vi");
    QString hlTitle = isVi ? QString::fromUtf8("Đánh dấu") : "Highlight";
    QString speakTitle = isVi ? QString::fromUtf8("Đọc") : "Speak";
    QString copyTitle = isVi ? QString::fromUtf8("Sao chép") : "Copy";
    QString dictTitle = isVi ? QString::fromUtf8("Tra từ điển") : "Dictionary";
    QString dictLabel = isVi ? QString::fromUtf8("Tra từ") : "Dict";
    QString transTitle = isVi ? QString::fromUtf8("Dịch văn bản") : "Translate";
    QString transLabel = isVi ? QString::fromUtf8("Dịch") : "Translate";
    QString searchTitle = isVi ? QString::fromUtf8("Tìm kiếm") : "Search";
    QString closeTitle = isVi ? QString::fromUtf8("Đóng") : "Close";

    html += "<div id=\"berry-selection-bubble\">";

    // 1. Highlight (Highlighter in yellow)
    html += QString("<span class=\"berry-bubble-btn\" id=\"b-btn-highlight\" title=\"%1\">"
                    "<svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"#FFE600\">"
                    "<path d=\"M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z\"/>"
                    "<path d=\"M3 21h18v2H3z\"/>"
                    "</svg></span>").arg(hlTitle);
    html += "<span class=\"berry-bubble-sep\"></span>";

    // 2. Speak (Speaker in Cascades cyan)
    html += QString("<span class=\"berry-bubble-btn\" id=\"b-btn-speak\" title=\"%1\">"
                    "<svg width=\"17\" height=\"17\" viewBox=\"0 0 24 24\" fill=\"#00A8DF\">"
                    "<path d=\"M3 9v6h4l5 5V4L7 9H3zm13.5 3c0-1.77-1.02-3.29-2.5-4.03v8.05c1.48-.73 2.5-2.25 2.5-4.02zM14 3.23v2.06c2.89.86 5 3.54 5 6.71s-2.11 5.85-5 6.71v2.06c4.01-.91 7-4.49 7-8.77s-2.99-7.86-7-8.77z\"/>"
                    "</svg></span>").arg(speakTitle);
    html += "<span class=\"berry-bubble-sep\"></span>";

    // 3. Copy (Overlapping documents)
    html += QString("<span class=\"berry-bubble-btn\" id=\"b-btn-copy\" title=\"%1\">"
                    "<svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"#E2E8F0\">"
                    "<path d=\"M16 1H4c-1.1 0-2 .9-2 2v14h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z\"/>"
                    "</svg></span>").arg(copyTitle);
    html += "<span class=\"berry-bubble-sep\"></span>";

    // 4. Dict (Book icon + text)
    html += QString("<span class=\"berry-bubble-btn\" id=\"b-btn-dict\" title=\"%1\">"
                    "<svg width=\"15\" height=\"15\" viewBox=\"0 0 24 24\" fill=\"#E2E8F0\">"
                    "<path d=\"M18 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zM6 4h5v8l-2.5-1.5L6 12V4z\"/>"
                    "</svg><span>%2</span></span>").arg(dictTitle).arg(dictLabel);
    html += "<span class=\"berry-bubble-sep\"></span>";

    // 5. Translate (Translate icon + text)
    html += QString("<span class=\"berry-bubble-btn\" id=\"b-btn-trans\" title=\"%1\">"
                    "<svg width=\"15\" height=\"15\" viewBox=\"0 0 24 24\" fill=\"#E2E8F0\">"
                    "<path d=\"M12.87 15.07l-2.54-2.51.03-.08c1.74-1.94 2.98-4.17 3.71-6.49H17V4h-7V2H8v2H1v2h11.17C11.5 7.92 10.44 9.75 9 11.35 8.07 10.32 7.3 9.19 6.69 8h-2c.73 1.63 1.73 3.17 2.98 4.56l-5.09 5.02L4 19l5-5 3.11 3.11.76-2.04zM18.5 10h-2L12 22h2l1.12-3h4.75L21 22h2l-4.5-12zm-2.62 7l1.62-4.33L19.12 17h-3.24z\"/>"
                    "</svg><span>%2</span></span>").arg(transTitle).arg(transLabel);
    html += "<span class=\"berry-bubble-sep\"></span>";

    // 6. Search (Magnifier)
    html += QString("<span class=\"berry-bubble-btn\" id=\"b-btn-search\" title=\"%1\">"
                    "<svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"#E2E8F0\">"
                    "<path d=\"M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z\"/>"
                    "</svg></span>").arg(searchTitle);
    html += "<span class=\"berry-bubble-sep\"></span>";

    // 7. Close (✕)
    html += QString("<span class=\"berry-bubble-btn\" id=\"b-btn-close\" title=\"%1\" style=\"padding: 0 6px;\">"
                    "<svg width=\"15\" height=\"15\" viewBox=\"0 0 24 24\" fill=\"#94A3B8\">"
                    "<path d=\"M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z\"/>"
                    "</svg></span>").arg(closeTitle);
    html += "</div>";
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
        applyVisualStyle();
        emit appThemeChanged();
        saveSettings();
    }
}

void BookManager::applyVisualStyle()
{
    if (bb::cascades::Application::instance() && bb::cascades::Application::instance()->themeSupport()) {
        if (m_appTheme == "dark") {
            bb::cascades::Application::instance()->themeSupport()->setVisualStyle(bb::cascades::VisualStyle::Dark);
        } else {
            bb::cascades::Application::instance()->themeSupport()->setVisualStyle(bb::cascades::VisualStyle::Bright);
        }
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
        if (isPdf() && m_pdfCore) {
            bool wantInverted = (m_theme == "dark");
            if (m_pdfCore->isInvertedColors() != wantInverted) {
                m_pdfCore->switchInvertedColors();
                m_pdfInverted = m_pdfCore->isInvertedColors();
                emit pdfInvertedChanged();
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

    QSettings settings("BerryLife", "BerryReader");
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
    syncToPersistentStorage();
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
    if (isPdf() && !m_isPdfReflow) {
        return;
    }
    m_tts->setBookContext(m_currentBookTitle, m_currentBookAuthor);

    int attempts = 0;
    while (attempts < 10) {
        attempts++;
        QString raw;
        if (isPdf() && m_isPdfReflow) {
            raw = m_currentPdfText;
        } else if (m_currentFormat == "epub") {
            raw = m_epubParser.getChapterContent(m_currentChapterIndex, m_cacheDir);
        } else if (m_currentFormat == "txt") {
            raw = m_txtParser.getChapterContent(m_currentChapterIndex);
        } else if (m_currentFormat == "fb2") {
            raw = m_fb2Parser.getChapterContent(m_currentChapterIndex);
        } else {
            raw = currentContent();
        }

        if (!raw.isEmpty()) {
            m_tts->startWithText(raw, QString(), true);
            if (m_tts->totalSentences() > 0) {
                // If user is reading beyond page 1 of this chapter, jump to the sentence on the current page
                if (m_currentPageInChapter > 1 && m_totalPagesInChapter > 1) {
                    int targetSentence = qBound(0, (int)((double)(m_currentPageInChapter - 1) / m_totalPagesInChapter * m_tts->totalSentences()), m_tts->totalSentences() - 1);
                    if (targetSentence > 0) {
                        m_tts->seekSentence(targetSentence);
                    }
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
    if (m_ttsAutoAdvance && m_currentChapterIndex < m_totalChapters - 1) {
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
        if (m_pdfCore) {
            m_pdfCore->navigateToPage(NSRReaderCore::PAGE_LOAD_NEXT);
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
        if (m_pdfCore) {
            m_pdfCore->navigateToPage(NSRReaderCore::PAGE_LOAD_PREV);
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
        if (m_pdfCore && index >= 1 && index <= m_totalChapters) {
            m_pdfCore->navigateToPage(NSRReaderCore::PAGE_LOAD_CUSTOM, index);
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

static void flattenPdfToc(const NSRTocEntry *entry, int level, QVariantList &outList)
{
    if (!entry) return;
    QString title = entry->getTitle().trimmed();
    int page = entry->getPage();
    if (!title.isEmpty() && page > 0) {
        QVariantMap item;
        item["title"] = title;
        item["index"] = page;
        item["chapterIndex"] = page;
        item["page"] = page;
        item["pageNumber"] = page;
        item["level"] = level;
        item["anchor"] = QString();
        outList.append(item);
    }
    QList<const NSRTocEntry *> children = entry->getChildren();
    for (int i = 0; i < children.size(); ++i) {
        flattenPdfToc(children[i], level + 1, outList);
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

    if (isPdf() && m_pdfCore) {
        NSRTocEntry *root = m_pdfCore->getToc();
        if (root) {
            flattenPdfToc(root, 0, m_tocList);
            delete root;
        }

        if (m_tocList.isEmpty()) {
            int pages = m_pdfCore->getPagesCount();
            if (pages <= 0) pages = m_totalChapters;
            for (int i = 1; i <= pages; ++i) {
                QVariantMap item;
                item["title"] = QString("Trang %1").arg(i);
                item["index"] = i;
                item["chapterIndex"] = i;
                item["page"] = i;
                item["pageNumber"] = i;
                item["level"] = 0;
                item["anchor"] = QString();
                m_tocList.append(item);
            }
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

void BookManager::searchBook(const QString &query, const QString &scope)
{
    QString q = query.trimmed();
    if (m_searchWorker) {
        m_searchWorker->cancel();
        m_searchWorker->wait();
        delete m_searchWorker;
        m_searchWorker = NULL;
    }

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
    if (m_searchWorker) {
        m_searchWorker->cancel();
        m_searchWorker->wait();
        delete m_searchWorker;
        m_searchWorker = NULL;
    }
    m_isSearching = false;
    m_searchResults.clear();
    emit isSearchingChanged();
    emit searchResultsChanged();
}

void BookManager::setIsPdfReflow(bool reflow)
{
    if (!isPdf()) return;
    if (m_isPdfReflow != reflow) {
        m_isPdfReflow = reflow;
        emit isPdfReflowChanged();
        if (m_isPdfReflow) {
            updatePdfReflowContent();
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
    setIsLoadingBook(true);
    // Safety watchdog: always ensure isLoadingBook drops to false after opening
    QTimer::singleShot(800, this, SLOT(resetLoadingState()));

    QFileInfo fi(filePath);
    if (!fi.exists() || fi.isDir()) {
        qWarning() << "[BookManager] File not found or is a directory:" << filePath;
        setIsLoadingBook(false);
        return false;
    }

    QSettings settings("BerryLife", "BerryReader");
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
    m_currentPdfText.clear();
    m_searchResults.clear();
    emit isPdfReflowChanged();
    emit hasPdfTextChanged();
    emit currentPdfTextChanged();
    emit searchResultsChanged();

    // Close any previous documents
    m_epubParser.close();
    m_txtParser.close();
    m_fb2Parser.close();
    if (m_pdfCore && m_pdfCore->isSessionLoaded()) {
        m_pdfCore->resetSession();
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
        QString cachedJpg = m_cacheDir + "/" + coverHash + ".jpg";
        QString cachedPng = m_cacheDir + "/" + coverHash + ".png";
        if (QFile::exists(cachedJpg)) {
            m_currentCoverPath = cachedJpg;
        } else if (QFile::exists(cachedPng)) {
            m_currentCoverPath = cachedPng;
        } else {
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
        loadCurrentChapter();
        updateTocList();
        ensureBookInLibrary(filePath);
        return true;
    } else if (ext == "txt") {
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
        loadCurrentChapter();
        updateTocList();
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
        loadCurrentChapter();
        updateTocList();
        ensureBookInLibrary(filePath);
        return true;
    } else if (ext == "pdf" || ext == "djvu") {
        m_currentFormat = ext;
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

        m_pdfSession = NSRSession();
        m_pdfSession.setFile(filePath);
        m_pdfSession.setPage(targetPage);
        m_pdfSession.setFitToWidth(true);
        m_pdfSession.setZoomGraphic(0.0);

        m_pdfCore->loadSession(&m_pdfSession);
        m_pdfCore->setZoom(0.0, NSRRenderRequest::NSR_RENDER_REASON_ZOOM_TO_WIDTH);

        if (m_theme == "dark") {
            if (!m_pdfCore->isInvertedColors()) {
                m_pdfCore->switchInvertedColors();
            }
        } else {
            if (m_pdfCore->isInvertedColors()) {
                m_pdfCore->switchInvertedColors();
            }
        }
        m_pdfInverted = m_pdfCore->isInvertedColors();

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
    if (m_pdfCore) {
        m_totalChapters = m_pdfCore->getPagesCount();
        m_totalPagesInChapter = m_totalChapters;
        QSettings settings("BerryLife", "BerryReader");
        settings.setValue(QString("totalChapters/%1").arg(m_currentFilePath), m_totalChapters);
        settings.setValue(QString("totalPages/%1").arg(m_currentFilePath), m_totalChapters);
        updateTocList();
        ensureBookInLibrary(m_currentFilePath);
        emit currentBookChanged();
        emit readingProgressChanged();
    }
}

void BookManager::ensureBookInLibrary(const QString &filePath)
{
    if (filePath.isEmpty()) return;

    QSettings settings("BerryLife", "BerryReader");
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
        return;
    }

    int w = imgData.width();
    int h = imgData.height();
    const unsigned char *pixels = (const unsigned char *)imgData.pixels();
    int stride = imgData.bytesPerLine();

    QImage qimg(w, h, QImage::Format_RGB32);
    for (int y = 0; y < h; ++y) {
        const unsigned int *srcRow = (const unsigned int *)(pixels + y * stride);
        QRgb *dstRow = (QRgb *)qimg.scanLine(y);
        for (int x = 0; x < w; ++x) {
            dstRow[x] = srcRow[x];
        }
    }

    QImage thumbnail = qimg.scaled(240, 320, Qt::KeepAspectRatio, Qt::FastTransformation);
    if (thumbnail.save(outPath, "PNG")) {
        m_currentCoverPath = outPath;
        updateBookCoverInLibrary(filePath, outPath);
        emit currentBookChanged();
    }
}

void BookManager::onPdfPageRendered(int pageNumber)
{
    if (!m_pdfCore) return;

    NSRRenderedPage page = m_pdfCore->getCurrentPage();
    bb::ImageData imgData = page.getImage();

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
    m_totalChapters = m_pdfCore->getPagesCount();
    m_currentPageInChapter = pageNumber;
    m_totalPagesInChapter = m_totalChapters;

    // Extract text layer from PDF page (for OCR search, dictionary, and text reflow)
    m_currentPdfText = page.getText();
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
}

void BookManager::onPdfIndicatorRequested(bool enabled)
{
    Q_UNUSED(enabled);
}

void BookManager::onPdfError(NSRAbstractDocument::NSRDocumentError error)
{
    qWarning() << "[BookManager] NSRReaderCore document error:" << error;
}

void BookManager::rotatePdf()
{
    if (isPdf() && m_pdfCore) {
        m_pdfCore->rotate(NSRReaderCore::ROTATE_DIRECTION_RIGHT);
    }
}

void BookManager::togglePdfInvert()
{
    if (isPdf() && m_pdfCore) {
        m_pdfCore->switchInvertedColors();
        m_pdfInverted = m_pdfCore->isInvertedColors();
        emit pdfInvertedChanged();
    }
}

void BookManager::zoomPdfIn()
{
    if (isPdf() && m_pdfCore) {
        double currentZoom = m_pdfCore->getZoom();
        m_pdfCore->setZoom(currentZoom * 1.25, NSRRenderRequest::NSR_RENDER_REASON_ZOOM);
    }
}

void BookManager::zoomPdfOut()
{
    if (isPdf() && m_pdfCore) {
        double currentZoom = m_pdfCore->getZoom();
        m_pdfCore->setZoom(currentZoom / 1.25, NSRRenderRequest::NSR_RENDER_REASON_ZOOM);
    }
}

void BookManager::zoomPdfFit()
{
    if (isPdf() && m_pdfCore) {
        m_pdfCore->setZoom(0.0, NSRRenderRequest::NSR_RENDER_REASON_ZOOM_TO_WIDTH);
    }
}

void BookManager::scanDirectoryRecursively(const QString &dirPath, int depth, QSet<QString> &addedPaths)
{
    if (depth > 8) return;

    QDir dir(dirPath);
    if (!dir.exists()) return;

    QStringList nameFilters;
    nameFilters << "*.epub" << "*.EPUB"
                << "*.txt"  << "*.TXT"
                << "*.fb2"  << "*.FB2"
                << "*.pdf"  << "*.PDF"
                << "*.djvu" << "*.DJVU";

    // 1. Files in current directory
    QFileInfoList fileEntries = dir.entryInfoList(nameFilters, QDir::Files, QDir::Time);
    foreach (const QFileInfo &fi, fileEntries) {
        QString absPath = fi.absoluteFilePath();
        if (addedPaths.contains(absPath)) continue;
        addedPaths.insert(absPath);

        QString ext = fi.suffix().toLower();
        QString rawName = fi.completeBaseName().replace('_', ' ');
        QString bookTitle = rawName;
        QString bookAuthor = (ext == "epub") ? "EPUB eBook" :
                             (ext == "txt")  ? "Văn bản TXT" :
                             (ext == "fb2")  ? "FB2 FictionBook" :
                             (ext == "pdf")  ? "Tài liệu PDF" : "Tài liệu DjVu";
        QString bookSeries = "";
        int bookSeriesIndex = 0;
        QString cover = "";
        QString formatBadge = ext.toUpper();

        QSettings settings("BerryLife", "BerryReader");

        QString hash = QString(QCryptographicHash::hash(absPath.toUtf8(), QCryptographicHash::Md5).toHex());
        QString cachedJpg = m_cacheDir + "/" + hash + ".jpg";
        QString cachedPng = m_cacheDir + "/" + hash + ".png";
        QString cachedNoCover = m_cacheDir + "/" + hash + ".nocover";

        QString cachedTitle = settings.value(QString("meta/title/%1").arg(absPath)).toString();
        QString cachedAuthor = settings.value(QString("meta/author/%1").arg(absPath)).toString();
        if (cachedTitle.isEmpty() && settings.value("lastBook/filePath").toString() == absPath) {
            cachedTitle = settings.value("lastBook/title").toString();
            cachedAuthor = settings.value("lastBook/author").toString();
        }

        if (!cachedTitle.isEmpty()) {
            bookTitle = cachedTitle;
            if (!cachedAuthor.isEmpty()) {
                bookAuthor = cachedAuthor;
            }
        }

        if (QFile::exists(cachedJpg)) {
            cover = cachedJpg;
        } else if (QFile::exists(cachedPng)) {
            cover = cachedPng;
        }

        if (ext == "epub" && ((cover.isEmpty() && !QFile::exists(cachedNoCover)) || cachedTitle.isEmpty())) {
            EpubParser ep;
            if (ep.open(absPath)) {
                if (cover.isEmpty() && !QFile::exists(cachedNoCover)) {
                    QString c = ep.extractCover(m_cacheDir);
                    if (!c.isEmpty()) {
                        cover = c;
                    } else {
                        QFile noCov(cachedNoCover);
                        if (noCov.open(QIODevice::WriteOnly)) noCov.close();
                    }
                }
                if (!ep.title().isEmpty()) {
                    bookTitle = ep.title();
                    settings.setValue(QString("meta/title/%1").arg(absPath), bookTitle);
                }
                if (!ep.author().isEmpty() && ep.author() != "Unknown Author") {
                    bookAuthor = ep.author();
                    settings.setValue(QString("meta/author/%1").arg(absPath), bookAuthor);
                }
                if (!ep.series().isEmpty()) {
                    bookSeries = ep.series();
                    bookSeriesIndex = ep.seriesIndex();
                    settings.setValue(QString("series/%1").arg(absPath), bookSeries);
                    settings.setValue(QString("seriesIdx/%1").arg(absPath), bookSeriesIndex);
                }
                ep.close();
            }
        } else if (ext == "fb2" && ((cover.isEmpty() && !QFile::exists(cachedNoCover)) || cachedTitle.isEmpty())) {
            Fb2Parser fb;
            if (fb.open(absPath)) {
                if (cover.isEmpty() && !QFile::exists(cachedNoCover)) {
                    QString c = fb.extractCover(m_cacheDir);
                    if (!c.isEmpty()) {
                        cover = c;
                    } else {
                        QFile noCov(cachedNoCover);
                        if (noCov.open(QIODevice::WriteOnly)) noCov.close();
                    }
                }
                if (!fb.title().isEmpty()) {
                    bookTitle = fb.title();
                    settings.setValue(QString("meta/title/%1").arg(absPath), bookTitle);
                }
                if (!fb.author().isEmpty()) {
                    bookAuthor = fb.author();
                    settings.setValue(QString("meta/author/%1").arg(absPath), bookAuthor);
                }
                if (!fb.series().isEmpty()) {
                    bookSeries = fb.series();
                    bookSeriesIndex = fb.seriesIndex();
                    settings.setValue(QString("series/%1").arg(absPath), bookSeries);
                    settings.setValue(QString("seriesIdx/%1").arg(absPath), bookSeriesIndex);
                }
                fb.close();
            }
        }

        // Only fallback to extracting Author from filename if no author or title was found from file metadata or cache
        bool authorIsGeneric = (bookAuthor.isEmpty() || 
                                bookAuthor == "Chưa rõ tác giả" || 
                                bookAuthor == "Unknown Author" ||
                                bookAuthor == "EPUB eBook" ||
                                bookAuthor == "Văn bản TXT" ||
                                bookAuthor == "FB2 FictionBook" ||
                                bookAuthor == "Tài liệu PDF" ||
                                bookAuthor == "Tài liệu DjVu");
        if (authorIsGeneric && (bookTitle.isEmpty() || bookTitle == rawName)) {
            if (rawName.contains(" - ")) {
                QStringList parts = rawName.split(" - ");
                if (parts.size() >= 2) {
                    bookTitle = parts[0].trimmed();
                    bookAuthor = parts[1].trimmed();
                }
            }
        }

        qint64 size = fi.size();
        QString fileSizeStr;
        if (size < 1024 * 1024) {
            fileSizeStr = QString("%1 KB").arg(qMax((qint64)1, size / 1024));
        } else {
            fileSizeStr = QString("%1 MB").arg(QString::number(size / (1024.0 * 1024.0), 'f', 1));
        }

        QDateTime modTime = fi.lastModified();

        if (settings.value(QString("hidden/%1").arg(absPath), false).toBool()) {
            continue;
        }

        if (bookSeries.isEmpty()) {
            if (settings.contains(QString("series/%1").arg(absPath))) {
                bookSeries = settings.value(QString("series/%1").arg(absPath)).toString();
                bookSeriesIndex = settings.value(QString("seriesIdx/%1").arg(absPath), 0).toInt();
            } else {
                extractSeriesFromTitleOrFile(bookTitle, rawName, bookSeries, bookSeriesIndex);
                settings.setValue(QString("series/%1").arg(absPath), bookSeries);
                settings.setValue(QString("seriesIdx/%1").arg(absPath), bookSeriesIndex);
            }
        }

        int savedProg = settings.value(QString("progressPct/%1").arg(absPath), 0).toInt();

        QVariantMap bookMap;
        bookMap["title"] = bookTitle;
        bookMap["author"] = bookAuthor;
        bookMap["series"] = bookSeries;
        bookMap["seriesIndex"] = bookSeriesIndex;
        bookMap["filePath"] = absPath;
        bookMap["path"] = absPath; // For QML compatibility
        bookMap["cover"] = cover;
        bookMap["format"] = ext;
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

        m_allBooks.append(bookMap);
    }

    // 2. Recurse into subdirectories (skipping hidden and heavy media folders)
    QFileInfoList dirEntries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &subDir, dirEntries) {
        QString sName = subDir.fileName().toLower();
        if (sName.startsWith(".") || sName == "android" || sName == "lost.dir" ||
            sName == "dcim" || sName == "camera" || sName == "photos" ||
            sName == "music" || sName == "videos" || sName == "voice" ||
            sName == "appdata" || sName == "cache" || sName == "system") {
            continue;
        }
        scanDirectoryRecursively(subDir.absoluteFilePath(), depth + 1, addedPaths);
    }
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
    foreach (const QVariantMap &book, m_allBooks) {
        if (m_libraryBrowseMode == "folder" && !m_selectedFolderPath.isEmpty()) {
            QString bookDir = QFileInfo(book.value("filePath").toString()).path();
            if (bookDir != m_selectedFolderPath && !bookDir.startsWith(m_selectedFolderPath + "/")) {
                continue;
            }
        }
        if (!normFilter.isEmpty() && normFilter != "all") {
            if (book.value("format").toString().toLower() != normFilter) {
                continue;
            }
        }
        int p = book.value("progress").toInt();
        bool isOpened = book.value("isOpened").toBool();
        if (!isOpened) {
            QSettings settings("BerryLife", "BerryReader");
            isOpened = settings.value(QString("opened/%1").arg(book.value("filePath").toString()), false).toBool();
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
        item["icon"] = "asset:///icons/ic_open.png";

        folderItems.append(item);
    }

    m_folderModel->insertList(folderItems);
    emit folderModelChanged();
}

void BookManager::scanLibrary()
{
    if (m_scanWorker) {
        m_scanWorker->cancel();
        m_scanWorker->wait();
        delete m_scanWorker;
        m_scanWorker = NULL;
    }

    m_isScanning = true;
    emit isScanningChanged();

    QStringList rawRoots;
    // Removable MicroSD
    rawRoots << "/accounts/1000/removable/sdcard2"
             << "/accounts/1000/removable/sdcard"
             << "/removable/sdcard2"
             << "/removable/sdcard";

    // Internal storage
    rawRoots << "/accounts/1000/shared";
    if (!QDir("/accounts/1000/shared").exists()) {
        rawRoots << QDir::homePath();
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

    m_scanWorker = new ScanWorker(searchRoots, m_cacheDir, this);
    connect(m_scanWorker, SIGNAL(scanCompleted(QList<QVariantMap>)), this, SLOT(onScanCompleted(QList<QVariantMap>)));
    m_scanWorker->start();

    // 15-second safety timer so the startup overlay is never stuck
    m_scanWatchdogTimer.start(15000);
}

void BookManager::onScanCompleted(const QList<QVariantMap> &allBooks)
{
    m_scanWatchdogTimer.stop();
    m_allBooks = allBooks;

    // Synchronize current lastBook metadata into m_allBooks if present
    if (!m_lastBookPath.isEmpty()) {
        for (int i = 0; i < m_allBooks.size(); ++i) {
            if (m_allBooks[i].value("filePath").toString() == m_lastBookPath) {
                if (!m_lastBookTitle.isEmpty()) m_allBooks[i]["title"] = m_lastBookTitle;
                if (!m_lastBookAuthor.isEmpty()) m_allBooks[i]["author"] = m_lastBookAuthor;
                if (!m_lastBookCover.isEmpty()) m_allBooks[i]["cover"] = m_lastBookCover;
                break;
            }
        }
    }

    qDebug() << "[BookManager] Total books found across all folders:" << m_allBooks.size();
    updateFolderList();
    applyFilterAndSort();
    refreshBrowser();
    refreshStats();

    m_isScanning = false;
    emit isScanningChanged();
    emit totalScannedBooksChanged();
}

void BookManager::cancelScan()
{
    qDebug() << "[BookManager] Scan cancelled by user tap or watchdog";
    m_scanWatchdogTimer.stop();
    if (m_scanWorker && m_scanWorker->isRunning()) {
        m_scanWorker->cancel();
    }
    if (m_isScanning) {
        m_isScanning = false;
        emit isScanningChanged();
        updateFolderList();
        applyFilterAndSort();
        refreshBrowser();
        refreshStats();
        emit totalScannedBooksChanged();
    }
}

void BookManager::onScanWatchdogTimeout()
{
    qWarning() << "[BookManager] Scan watchdog timed out after 15s, forcing UI unfreeze";
    cancelScan();
}

void BookManager::saveSettings()
{
    QSettings settings("BerryLife", "BerryReader");
    settings.setValue("app/language", m_language);
    settings.setValue("reader/fontSize", m_fontSize);
    settings.setValue("reader/fontFamily", m_fontFamily);
    settings.setValue("reader/theme", m_theme);
    settings.setValue("app/theme", m_appTheme);
    settings.setValue("reader/readingMode", m_readingMode);
    settings.setValue("reader/lineSpacing", m_lineSpacing);
    settings.setValue("reader/showFooterChapter", m_showFooterChapter);
    settings.setValue("reader/showFooterProgress", m_showFooterProgress);
    settings.setValue("reader/showFooterBattery", m_showFooterBattery);
    settings.setValue("reader/showFooterClock", m_showFooterClock);
    settings.setValue("reader/marginTop", m_marginTop);
    settings.setValue("reader/marginBottom", m_marginBottom);
    settings.setValue("reader/marginLeft", m_marginLeft);
    settings.setValue("reader/marginRight", m_marginRight);
    settings.setValue("reader/ttsAutoAdvance", m_ttsAutoAdvance);
    settings.setValue("dev/serverIp", m_devServerIp);

    settings.setValue("library/mainViewMode", m_mainViewMode);
    settings.setValue("library/viewMode", m_libraryViewMode);
    settings.setValue("library/browseMode", m_libraryBrowseMode);
    settings.setValue("library/sortBy", m_sortBy);
    settings.setValue("library/groupBy", m_groupBy);
    settings.setValue("library/formatFilter", m_formatFilter);

    settings.setValue("stats/totalReadingSeconds", m_totalReadingSeconds);
    settings.setValue("stats/todayReadingSeconds", m_todayReadingSeconds);
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
        QStringList entries;
        QMapIterator<QString, int> bIt(dbIt.value());
        while (bIt.hasNext()) {
            bIt.next();
            QString cleanPath = unwrapBookPath(bIt.key());
            if (cleanPath.isEmpty()) continue;
            QString enc = QString::fromUtf8(QUrl::toPercentEncoding(cleanPath));
            entries.append(QString("%1|%2").arg(enc).arg(bIt.value()));
        }
        settings.setValue(dateKey, entries.join(","));
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
    syncToPersistentStorage();
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

void BookManager::pauseTimersForSleep()
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
    saveCurrentProgress();
}

void BookManager::resumeTimersFromSleep()
{
    qDebug() << "[BookManager] resumeTimersFromSleep: resuming timers (reader active:" << m_isReaderActive << ")";
    m_isAppSleeping = false;
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
    restoreFromPersistentStorage();

    QSettings settings("BerryLife", "BerryReader");
    QString defaultLang = QLocale::system().name().startsWith("vi") ? "vi" : "en";
    m_language = settings.value("app/language", defaultLang).toString();
    m_fontSize = settings.value("reader/fontSize", 18).toInt();
    m_fontFamily = settings.value("reader/fontFamily", "sans").toString();
    m_theme = settings.value("reader/theme", "dark").toString();
    m_appTheme = settings.value("app/theme", "dark").toString();
    m_readingMode = settings.value("reader/readingMode", "paged").toString();
    m_lineSpacing = settings.value("reader/lineSpacing", "normal").toString();
    m_showFooterChapter = settings.value("reader/showFooterChapter", true).toBool();
    m_showFooterProgress = settings.value("reader/showFooterProgress", true).toBool();
    m_showFooterBattery = settings.value("reader/showFooterBattery", true).toBool();
    m_showFooterClock = settings.value("reader/showFooterClock", true).toBool();
    m_marginTop = settings.value("reader/marginTop", 16).toInt();
    m_marginBottom = settings.value("reader/marginBottom", 16).toInt();
    m_marginLeft = settings.value("reader/marginLeft", 16).toInt();
    m_marginRight = settings.value("reader/marginRight", 16).toInt();
    m_ttsAutoAdvance = settings.value("reader/ttsAutoAdvance", false).toBool();
    m_devServerIp = settings.value("dev/serverIp", "192.168.0.108").toString();

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
    foreach (const QString &d, dbDates) {
        QString raw = settings.value(d).toString();
        // Support both new comma delimiter and legacy semicolon delimiter
        QStringList parts = raw.contains(',') ? raw.split(',', QString::SkipEmptyParts) : raw.split(';', QString::SkipEmptyParts);
        foreach (const QString &p, parts) {
            int pipeIdx = p.indexOf('|');
            if (pipeIdx > 0) {
                QString keyStr = p.left(pipeIdx);
                QString path = unwrapBookPath(keyStr);
                int secs = p.mid(pipeIdx + 1).toInt();
                if (!path.isEmpty() && secs > 0) {
                    m_dailyBookReadingSeconds[d][path] = secs;
                }
            }
        }
    }
    settings.endGroup();

    // Ensure today's reading seconds are synced into daily stats
    if (m_todayReadingSeconds > 0) {
        if (m_dailyReadingSeconds.value(m_todayDate, 0) < m_todayReadingSeconds) {
            m_dailyReadingSeconds[m_todayDate] = m_todayReadingSeconds;
        }
    }

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

void BookManager::setMarginTop(int margin)
{
    if (margin < 0) margin = 0;
    if (margin > 80) margin = 80;
    if (m_marginTop != margin) {
        m_marginTop = margin;
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
    emit marginsChanged();
    saveSettings();
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

    QSettings settings("BerryLife", "BerryReader");
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
            QRegExp rx("/Type\\s*/Pages[^>]*?/Count\\s+(\\d+)");
            rx.setMinimal(true);
            int pos = 0;
            int maxPages = 0;
            while ((pos = rx.indexIn(data, pos)) != -1) {
                int count = rx.cap(1).toInt();
                if (count > maxPages) maxPages = count;
                pos += rx.matchedLength();
            }

            if (maxPages == 0) {
                QRegExp rx2("/Count\\s+(\\d+)[^>]*?/Type\\s*/Pages");
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
        openBrowserFolder("root");
        return;
    }

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

    // 0. Up to parent folder entry - sortKey with \x01 ensures it is ALWAYS on top above all symbols!
    QVariantMap upItem;
    upItem["itemType"] = "folder";
    upItem["isDir"] = true;
    upItem["folderType"] = "up";
    upItem["name"] = ".. (" + tr("Lùi lại") + ")";
    upItem["title"] = upItem["name"];
    upItem["path"] = "__UP__";
    upItem["subtext"] = tr("Trở về thư mục trước");
    upItem["sortKey"] = QString("\x01_00_back");
    upItem["icon"] = "asset:///icons/ic_previous.png";
    items.append(upItem);

    // 1. Subfolders
    QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    foreach (const QFileInfo &sub, subDirs) {
        QString sName = sub.fileName().toLower();
        if (sName.startsWith(".") || sName == "android" || sName == "lost.dir" ||
            sName == "dcim" || sName == "camera" || sName == "photos" ||
            sName == "music" || sName == "videos" || sName == "voice" ||
            sName == "appdata" || sName == "cache" || sName == "system") {
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

        // Try to get cover image from books inside this subfolder
        QString folderCover;
        QString subAbsPath = sub.absoluteFilePath();
        QString subPrefix = subAbsPath;
        if (!subPrefix.endsWith('/')) subPrefix += '/';

        // 1. Check in already cached library books (m_allBooks)
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

        // 2. If not found in m_allBooks, scan directly inside subfolder
        if (folderCover.isEmpty()) {
            QDir subDir(subAbsPath);
            // Check direct image files (cover.jpg, folder.jpg, etc.)
            QStringList imgFilters;
            imgFilters << "cover.jpg" << "cover.png" << "cover.jpeg"
                       << "folder.jpg" << "folder.png"
                       << "*.jpg" << "*.png";
            QFileInfoList imgFiles = subDir.entryInfoList(imgFilters, QDir::Files, QDir::Name | QDir::IgnoreCase);
            if (!imgFiles.isEmpty()) {
                folderCover = imgFiles.first().absoluteFilePath();
            } else {
                // Check for first book file inside subfolder
                QStringList bFilters;
                bFilters << "*.epub" << "*.fb2";
                QFileInfoList bFiles = subDir.entryInfoList(bFilters, QDir::Files, QDir::Name | QDir::IgnoreCase);

                // If direct subfolder has no books, check 1 level deeper
                if (bFiles.isEmpty()) {
                    QFileInfoList nestedDirs = subDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
                    foreach (const QFileInfo &nd, nestedDirs) {
                        QDir nDir(nd.absoluteFilePath());
                        bFiles = nDir.entryInfoList(bFilters, QDir::Files, QDir::Name | QDir::IgnoreCase);
                        if (!bFiles.isEmpty()) break;
                    }
                }

                if (!bFiles.isEmpty()) {
                    QString bookPath = bFiles.first().absoluteFilePath();
                    QString ext = bFiles.first().suffix().toLower();
                    QString hash = QCryptographicHash::hash(bookPath.toUtf8(), QCryptographicHash::Md5).toHex();
                    QString cachedJpg = m_cacheDir + "/" + hash + ".jpg";
                    QString cachedPng = m_cacheDir + "/" + hash + ".png";
                    if (QFile::exists(cachedJpg)) {
                        folderCover = cachedJpg;
                    } else if (QFile::exists(cachedPng)) {
                        folderCover = cachedPng;
                    } else {
                        if (ext == "epub") {
                            EpubParser ep;
                            if (ep.open(bookPath)) {
                                folderCover = ep.extractCover(m_cacheDir);
                            }
                        } else if (ext == "fb2") {
                            Fb2Parser fb;
                            if (fb.open(bookPath)) {
                                folderCover = fb.extractCover(m_cacheDir);
                            }
                        }
                    }
                }
            }
        }

        if (!folderCover.isEmpty() && QFile::exists(folderCover)) {
            item["cover"] = folderCover;
        }

        items.append(item);
    }

    // 2. Book files
    QStringList filters;
    filters << "*.epub" << "*.txt" << "*.fb2" << "*.pdf" << "*.djvu";
    QFileInfoList bookFiles = dir.entryInfoList(filters, QDir::Files, QDir::Name | QDir::IgnoreCase);

    // Fast O(1) in-memory lookup table for library books
    QHash<QString, const QVariantMap*> bookLookup;
    for (int i = 0; i < m_allBooks.size(); ++i) {
        bookLookup.insert(m_allBooks[i].value("filePath").toString(), &m_allBooks[i]);
    }

    QSettings settings("BerryLife", "BerryReader");

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
                if (ext == "epub") {
                    EpubParser ep;
                    if (ep.open(absPath)) {
                        QString c = ep.extractCover(m_cacheDir);
                        if (!c.isEmpty()) {
                            cover = c;
                        }
                        if (bookAuthor.isEmpty() && !ep.author().isEmpty() && ep.author() != "Unknown Author") {
                            bookAuthor = ep.author();
                        }
                        if (!ep.title().isEmpty()) {
                            bookTitle = ep.title();
                        }
                    }
                } else if (ext == "fb2") {
                    Fb2Parser fb;
                    if (fb.open(absPath)) {
                        QString c = fb.extractCover(m_cacheDir);
                        if (!c.isEmpty()) {
                            cover = c;
                        }
                        if (bookAuthor.isEmpty() && !fb.author().isEmpty()) {
                            bookAuthor = fb.author();
                        }
                        if (!fb.title().isEmpty()) {
                            bookTitle = fb.title();
                        }
                    }
                }
            }
        }

        QString pageInfoStr = "";
        if (totalPages > 0) {
            if (ext == "pdf" || ext == "djvu") {
                if (savedProg > 0 && curChap > 0) {
                    pageInfoStr = QString(tr("Trang %1 / %2")).arg(curChap).arg(totalPages);
                } else {
                    pageInfoStr = QString(tr("%1 trang")).arg(totalPages);
                }
            } else if (ext == "epub" || ext == "fb2") {
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
                if (itemFmt != fmt) {
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
    if (seconds < 60) {
        return "<1m";
    }
    int mins = seconds / 60;
    if (mins < 60) {
        return QString("%1m").arg(mins);
    }
    int h = mins / 60;
    int m = mins % 60;
    if (m > 0) {
        return QString("%1h%2").arg(h).arg(m);
    }
    return QString("%1h").arg(h);
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
    QMapIterator<QString, int> it(m_bookReadingSeconds);
    while (it.hasNext()) {
        it.next();
        if (it.value() > 0 && !countedPaths.contains(it.key())) {
            QSettings s("BerryLife", "BerryReader");
            int p = s.value(QString("progressPct/%1").arg(it.key()), 0).toInt();
            if (p > 0 && p < 95) {
                count++;
                countedPaths.insert(it.key());
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

void BookManager::refreshStats()
{
    QSettings settings("BerryLife", "BerryReader");
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
        QString heatColor = "transparent";
        if (secs > 0) {
            if (mins < 15) {
                level = 1;
                heatColor = "#1B4D3E";
            } else if (mins < 45) {
                level = 2;
                heatColor = "#236B50";
            } else if (mins < 90) {
                level = 3;
                heatColor = "#2E8B57";
            } else {
                level = 4;
                heatColor = "#34C759";
            }
        }

        QString durStr;
        if (secs <= 0) {
            durStr = "";
        } else if (secs < 60) {
            durStr = "<1m";
        } else if (mins < 60) {
            durStr = QString("%1m").arg(mins);
        } else {
            int h = mins / 60;
            int m = mins % 60;
            if (m > 0) {
                durStr = QString("%1h%2").arg(h).arg(m);
            } else {
                durStr = QString("%1h").arg(h);
            }
        }

        int booksCount = m_dailyBookReadingSeconds.value(dateStr).size();

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
        dayMap["heatColor"] = heatColor;
        dayMap["booksCount"] = booksCount;

        list.append(dayMap);
        cur = cur.addDays(1);
    }

    return list;
}

struct BookCalendarSpan {
    QString path;
    QString title;
    QString color;
    int startCol;
    int endCol;
    int track;
};

QVariantList BookManager::getCalendarMonthBookTracks(int year, int month)
{
    QVariantList weeksList;
    QDate firstDay(year, month, 1);
    if (!firstDay.isValid()) {
        firstDay = QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1);
        year = firstDay.year();
        month = firstDay.month();
    }
    int dow = firstDay.dayOfWeek(); // 1 = Mon, 7 = Sun
    QDate cur = firstDay.addDays(-(dow - 1));

    // KOReader-style muted book banner colors
    QStringList bannerColors;
    bannerColors << "#475569"   // Slate
                 << "#334155"   // Dark Slate
                 << "#0F766E"   // Teal
                 << "#B45309"   // Amber / Brown
                 << "#4338CA"   // Indigo
                 << "#4D7C0F"   // Olive
                 << "#BE123C";  // Rose

    QMap<QString, QString> bookColorMap;
    int nextColorIdx = 0;

    // 6 weeks
    for (int w = 0; w < 6; ++w) {
        QString dates[7];
        for (int c = 0; c < 7; ++c) {
            dates[c] = cur.toString("yyyy-MM-dd");
            cur = cur.addDays(1);
        }

        QMap<QString, QList<int> > bookDays;
        for (int c = 0; c < 7; ++c) {
            const QString &dStr = dates[c];
            QMap<QString, int> dayBooks = m_dailyBookReadingSeconds.value(dStr);
            if (dayBooks.isEmpty() && m_dailyReadingSeconds.value(dStr, 0) > 0) {
                if (!m_currentFilePath.isEmpty()) dayBooks[m_currentFilePath] = m_dailyReadingSeconds.value(dStr, 0);
                else if (!m_lastBookPath.isEmpty()) dayBooks[m_lastBookPath] = m_dailyReadingSeconds.value(dStr, 0);
            }
            QMapIterator<QString, int> bit(dayBooks);
            while (bit.hasNext()) {
                bit.next();
                if (bit.value() > 0) {
                    bookDays[bit.key()].append(c);
                }
            }
        }

        QList<BookCalendarSpan> spans;
        QMapIterator<QString, QList<int> > it(bookDays);
        while (it.hasNext()) {
            it.next();
            QString path = it.key();
            QList<int> cols = it.value();
            if (cols.isEmpty()) continue;
            qSort(cols);

            if (!bookColorMap.contains(path)) {
                bookColorMap[path] = bannerColors[nextColorIdx % bannerColors.size()];
                nextColorIdx++;
            }
            QString color = bookColorMap[path];

            QString title;
            foreach (const QVariantMap &b, m_allBooks) {
                if (b.value("filePath").toString() == path) {
                    title = b.value("title").toString();
                    break;
                }
            }
            if (title.isEmpty()) {
                QFileInfo fi(path);
                title = fi.completeBaseName();
            }

            int runStart = cols[0];
            int runEnd = cols[0];
            for (int k = 1; k < cols.size(); ++k) {
                if (cols[k] == runEnd + 1) {
                    runEnd = cols[k];
                } else {
                    BookCalendarSpan sp;
                    sp.path = path;
                    sp.title = title;
                    sp.color = color;
                    sp.startCol = runStart;
                    sp.endCol = runEnd;
                    sp.track = -1;
                    spans.append(sp);
                    runStart = cols[k];
                    runEnd = cols[k];
                }
            }
            BookCalendarSpan sp;
            sp.path = path;
            sp.title = title;
            sp.color = color;
            sp.startCol = runStart;
            sp.endCol = runEnd;
            sp.track = -1;
            spans.append(sp);
        }

        // Sort spans: longer spans first, then by startCol
        for (int i = 0; i < spans.size() - 1; ++i) {
            for (int j = i + 1; j < spans.size(); ++j) {
                int lenI = spans[i].endCol - spans[i].startCol;
                int lenJ = spans[j].endCol - spans[j].startCol;
                if (lenJ > lenI || (lenJ == lenI && spans[j].startCol < spans[i].startCol)) {
                    spans.swap(i, j);
                }
            }
        }

        bool trackOcc[3][7];
        memset(trackOcc, 0, sizeof(trackOcc));

        for (int i = 0; i < spans.size(); ++i) {
            for (int t = 0; t < 3; ++t) {
                bool conflict = false;
                for (int c = spans[i].startCol; c <= spans[i].endCol; ++c) {
                    if (trackOcc[t][c]) {
                        conflict = true;
                        break;
                    }
                }
                if (!conflict) {
                    spans[i].track = t;
                    for (int c = spans[i].startCol; c <= spans[i].endCol; ++c) {
                        trackOcc[t][c] = true;
                    }
                    break;
                }
            }
        }

        QVariantList track0Elements;
        QVariantList track1Elements;
        QVariantList track2Elements;

        for (int t = 0; t < 3; ++t) {
            QList<BookCalendarSpan> tSpans;
            for (int i = 0; i < spans.size(); ++i) {
                if (spans[i].track == t) {
                    tSpans.append(spans[i]);
                }
            }
            for (int i = 0; i < tSpans.size() - 1; ++i) {
                for (int j = i + 1; j < tSpans.size(); ++j) {
                    if (tSpans[j].startCol < tSpans[i].startCol) {
                        tSpans.swap(i, j);
                    }
                }
            }

            QVariantList elements;
            if (!tSpans.isEmpty()) {
                int curCol = 0;
                for (int i = 0; i < tSpans.size(); ++i) {
                    const BookCalendarSpan &sp = tSpans[i];
                    if (sp.startCol > curCol) {
                        QVariantMap spacer;
                        spacer["isSpacer"] = true;
                        spacer["spaceQuota"] = (double)(sp.startCol - curCol);
                        spacer["title"] = "";
                        spacer["color"] = "transparent";
                        spacer["filePath"] = "";
                        elements.append(spacer);
                    }
                    QVariantMap item;
                    item["isSpacer"] = false;
                    item["spaceQuota"] = (double)(sp.endCol - sp.startCol + 1);
                    item["title"] = sp.title;
                    item["color"] = sp.color;
                    item["filePath"] = sp.path;
                    elements.append(item);
                    curCol = sp.endCol + 1;
                }
                if (curCol < 7) {
                    QVariantMap spacer;
                    spacer["isSpacer"] = true;
                    spacer["spaceQuota"] = (double)(7 - curCol);
                    spacer["title"] = "";
                    spacer["color"] = "transparent";
                    spacer["filePath"] = "";
                    elements.append(spacer);
                }
            }
            if (t == 0) track0Elements = elements;
            else if (t == 1) track1Elements = elements;
            else track2Elements = elements;
        }

        QVariantMap weekData;
        weekData["hasTrack0"] = !track0Elements.isEmpty();
        weekData["track0"] = track0Elements;
        weekData["hasTrack1"] = !track1Elements.isEmpty();
        weekData["track1"] = track1Elements;
        weekData["hasTrack2"] = !track2Elements.isEmpty();
        weekData["track2"] = track2Elements;

        weeksList.append(weekData);
    }

    return weeksList;
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
    result["readingDays"] = readingDays;
    result["days"] = daysList;

    int avgSecs = (readingDays > 0) ? (totalSecs / readingDays) : 0;
    result["avgDailySeconds"] = avgSecs;
    result["avgDailyTimeStr"] = formatDuration(avgSecs);

    QString weekTitle;
    if (m_language == "vi") {
        weekTitle = QString("Tuần %1/%2 - %3/%4/%5")
            .arg(monday.day()).arg(monday.month())
            .arg(sunday.day()).arg(sunday.month()).arg(sunday.year());
    } else {
        weekTitle = QString("Week %1/%2 - %3/%4/%5")
            .arg(monday.month()).arg(monday.day())
            .arg(sunday.month()).arg(sunday.day()).arg(sunday.year());
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
    stats["readingDays"] = readingDays;
    stats["completionRate"] = completionRate;
    stats["avgDailySeconds"] = avgSecs;
    stats["avgDailyTimeStr"] = formatDuration(avgSecs);
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

QVariantList BookManager::getDayBooks(const QString &dateStr)
{
    QVariantList result;
    if (dateStr.isEmpty()) return result;

    QMap<QString, int> booksMap = m_dailyBookReadingSeconds.value(dateStr);

    // If day has reading seconds but booksMap is empty (e.g. legacy/migrated), populate with last/current book
    if (booksMap.isEmpty() && m_dailyReadingSeconds.value(dateStr, 0) > 0) {
        if (!m_currentFilePath.isEmpty()) {
            booksMap[m_currentFilePath] = m_dailyReadingSeconds.value(dateStr, 0);
        } else if (!m_lastBookPath.isEmpty()) {
            booksMap[m_lastBookPath] = m_dailyReadingSeconds.value(dateStr, 0);
        }
    }

    QMapIterator<QString, int> it(booksMap);
    while (it.hasNext()) {
        it.next();
        QString path = unwrapBookPath(it.key());
        int secs = it.value();
        if (path.isEmpty() || secs <= 0) continue;

        QFileInfo fi(path);
        QVariantMap bookInfo;
        bool found = false;
        foreach (const QVariantMap &b, m_allBooks) {
            QString bPath = b.value("filePath").toString();
            if (bPath == path) {
                bookInfo = b;
                found = true;
                break;
            }
        }

        // If not found by exact path, try matching by filename
        if (!found) {
            QString fname = fi.fileName();
            if (!fname.isEmpty()) {
                foreach (const QVariantMap &b, m_allBooks) {
                    QString bPath = b.value("filePath").toString();
                    if (bPath.endsWith(fname) || QFileInfo(bPath).fileName().compare(fname, Qt::CaseInsensitive) == 0) {
                        bookInfo = b;
                        found = true;
                        break;
                    }
                }
            }
        }

        // If still not found, check if it matches current or last book
        if (!found) {
            if (!m_lastBookPath.isEmpty() && (m_lastBookPath == path || QFileInfo(m_lastBookPath).fileName() == fi.fileName())) {
                bookInfo["filePath"] = m_lastBookPath;
                bookInfo["title"] = m_lastBookTitle;
                bookInfo["author"] = m_lastBookAuthor;
                bookInfo["cover"] = m_lastBookCover;
                bookInfo["format"] = m_lastBookFormat;
                bookInfo["progress"] = m_lastBookProgress;
                found = true;
            } else if (!m_currentFilePath.isEmpty() && (m_currentFilePath == path || QFileInfo(m_currentFilePath).fileName() == fi.fileName())) {
                bookInfo["filePath"] = m_currentFilePath;
                bookInfo["title"] = m_currentBookTitle;
                bookInfo["author"] = m_currentBookAuthor;
                bookInfo["cover"] = m_currentCoverPath;
                bookInfo["format"] = m_currentFormat;
                bookInfo["progress"] = readingProgress();
                found = true;
            }
        }

        if (!found) {
            QString t = fi.completeBaseName().replace('_', ' ');
            // If the base name still looks like a raw hash / base64 without spaces and > 20 chars
            if (t.length() > 20 && !t.contains(' ')) {
                QSettings s("BerryLife", "BerryReader");
                QString sTitle = s.value(QString("title/%1").arg(path)).toString();
                if (!sTitle.isEmpty()) {
                    t = sTitle;
                } else {
                    t = (m_language == "vi" ? tr("Tài liệu đã đọc") : "Reading document");
                }
            }
            if (t.contains("accounts", Qt::CaseInsensitive) || t.length() <= 1) {
                QString clean = path;
                while (clean.endsWith('/')) clean.chop(1);
                int lastSlash = clean.lastIndexOf('/');
                if (lastSlash >= 0) {
                    t = clean.mid(lastSlash + 1);
                    int dot = t.lastIndexOf('.');
                    if (dot > 0) t = t.left(dot);
                }
            }
            bookInfo["filePath"] = path;
            bookInfo["title"] = t.isEmpty() ? (m_language == "vi" ? tr("Tài liệu") : "Document") : t;
            bookInfo["author"] = (m_language == "vi" ? tr("Chưa rõ tác giả") : "Unknown Author");
            bookInfo["format"] = fi.suffix().isEmpty() ? "EPUB" : fi.suffix().toUpper();
            bookInfo["cover"] = "asset:///icons/default_cover.png";
            QSettings s("BerryLife", "BerryReader");
            bookInfo["progress"] = s.value(QString("progressPct/%1").arg(path), 0).toInt();
        }

        bookInfo["daySeconds"] = secs;
        bookInfo["dayDurationStr"] = formatDuration(secs);

        QSettings settings("BerryLife", "BerryReader");
        int prog = bookInfo.value("progress", 0).toInt();
        if (prog <= 0) {
            prog = settings.value(QString("progressPct/%1").arg(path), 0).toInt();
            bookInfo["progress"] = prog;
        }
        bookInfo["progressStr"] = QString("%1%").arg(prog);

        int totalPages = bookInfo.value("totalPages", 0).toInt();
        if (totalPages <= 0) totalPages = bookInfo.value("totalChapters", 0).toInt();
        if (totalPages <= 0) totalPages = settings.value(QString("totalPages/%1").arg(path), 0).toInt();
        if (totalPages <= 0) totalPages = settings.value(QString("totalChapters/%1").arg(path), 0).toInt();
        if (path == m_currentFilePath && m_totalChapters > 0) totalPages = m_totalChapters;

        int curChap = bookInfo.value("currentChapter", 0).toInt();
        if (curChap <= 0) curChap = settings.value(QString("progress/%1").arg(path), 0).toInt();
        if (path == m_currentFilePath && m_currentChapterIndex > 0) curChap = m_currentChapterIndex;

        int curPage = 0;
        QString ext = fi.suffix().toLower();
        if (totalPages > 0) {
            if (curChap > 0) {
                curPage = (ext == "pdf" || ext == "djvu") ? curChap : (curChap + 1);
            } else if (prog > 0) {
                curPage = qMax(1, qRound((double)prog * totalPages / 100.0));
            }
        }

        QString pageCountStr = "";
        if (totalPages > 0) {
            if (curPage > 0) {
                pageCountStr = QString(tr("Trang %1 / %2")).arg(curPage).arg(totalPages);
            } else {
                pageCountStr = QString(tr("%1 trang")).arg(totalPages);
            }
        } else if (prog > 0) {
            pageCountStr = QString("%1%").arg(prog);
        }

        bookInfo["currentPage"] = curPage;
        bookInfo["totalPages"] = totalPages;
        bookInfo["pageCountStr"] = pageCountStr;
        bookInfo["formatBadge"] = bookInfo.value("formatBadge", ext.toUpper()).toString().toUpper();
        if (bookInfo.value("formatBadge").toString().isEmpty()) {
            bookInfo["formatBadge"] = ext.toUpper();
        }

        result.append(bookInfo);
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
    overall["totalReadingDays"] = totalDays;
    overall["allTimeStreak"] = maxStreak;
    overall["currentStreak"] = currentStreak;
    overall["bestDayDate"] = bestDayDate;
    overall["bestDaySeconds"] = bestDaySecs;
    overall["bestDayTimeStr"] = formatDuration(bestDaySecs);
    overall["totalBooks"] = totalLibraryBooks();
    overall["completedBooks"] = completedBooksCount();
    overall["readingBooks"] = readingBooksCount();

    return overall;
}

QVariantMap BookManager::getBookDetailedStats(const QString &filePath)
{
    QVariantMap stats;
    if (filePath.isEmpty()) return stats;

    QString title = "";
    QString author = "";
    QString cover = "";
    QString format = "";
    int progress = 0;
    int totalPages = 0;
    int currentPage = 0;

    // 1. Try finding in m_allBooks
    for (int i = 0; i < m_allBooks.size(); ++i) {
        if (m_allBooks[i].value("filePath").toString() == filePath) {
            const QVariantMap &b = m_allBooks[i];
            title = b.value("title").toString();
            author = b.value("author").toString();
            cover = b.value("cover").toString();
            format = b.value("format").toString();
            progress = b.value("progress", 0).toInt();
            totalPages = b.value("totalPages", 0).toInt();
            if (totalPages == 0) totalPages = b.value("totalChapters", 0).toInt();
            currentPage = b.value("currentPage", 0).toInt();
            if (currentPage == 0) currentPage = b.value("currentChapter", 0).toInt();
            break;
        }
    }

    // 2. If it is current active book, override with live in-memory data
    if (filePath == m_currentFilePath && !m_currentFilePath.isEmpty()) {
        if (!m_currentBookTitle.isEmpty()) title = m_currentBookTitle;
        if (!m_currentBookAuthor.isEmpty()) author = m_currentBookAuthor;
        if (!m_currentCoverPath.isEmpty()) cover = m_currentCoverPath;
        if (!m_currentFormat.isEmpty()) format = m_currentFormat;
        progress = readingProgress();
        if (m_totalChapters > 0) totalPages = m_totalChapters;
        if (m_currentChapterIndex > 0) currentPage = m_currentChapterIndex;
    } else if (filePath == m_lastBookPath && !m_lastBookPath.isEmpty()) {
        if (title.isEmpty()) title = m_lastBookTitle;
        if (author.isEmpty()) author = m_lastBookAuthor;
        if (cover.isEmpty()) cover = m_lastBookCover;
        if (format.isEmpty()) format = m_lastBookFormat;
        if (progress == 0) progress = m_lastBookProgress;
    }

    if (title.isEmpty()) {
        QFileInfo fi(filePath);
        title = fi.completeBaseName();
    }
    if (author.isEmpty()) {
        author = (m_language == "vi" ? "Chưa rõ tác giả" : "Unknown Author");
    }
    if (format.isEmpty()) {
        QFileInfo fi(filePath);
        format = fi.suffix().toLower();
    }

    // Calculate pages read if currentPage == 0 but progress > 0
    if (currentPage == 0 && totalPages > 0 && progress > 0) {
        currentPage = qRound((double)totalPages * (double)progress / 100.0);
    }

    int totalSecs = m_bookReadingSeconds.value(filePath, 0);
    if (totalSecs == 0) {
        QMapIterator<QString, QMap<QString, int> > it(m_dailyBookReadingSeconds);
        while (it.hasNext()) {
            it.next();
            totalSecs += it.value().value(filePath, 0);
        }
        if (totalSecs > 0) {
            m_bookReadingSeconds[filePath] = totalSecs;
        }
    }

    stats["filePath"] = filePath;
    stats["title"] = title;
    stats["author"] = author;
    stats["cover"] = cover.isEmpty() ? "asset:///icons/default_cover.png" : cover;
    stats["format"] = format;
    stats["formatBadge"] = format.toUpper();
    stats["progress"] = progress;
    stats["progressStr"] = QString("%1%").arg(progress);
    stats["currentPage"] = currentPage;
    stats["totalPages"] = totalPages;
    stats["pageStr"] = (totalPages > 0) ? QString("%1 / %2 %3").arg(currentPage).arg(totalPages).arg(m_language == "vi" ? "trang" : "pages")
                                        : (progress > 0 ? QString("%1%").arg(progress) : (m_language == "vi" ? "Chưa mở" : "Not opened"));
    stats["totalSeconds"] = totalSecs;
    stats["totalTimeStr"] = formatDuration(totalSecs);

    // Reading Speed (KOReader style: min/page or pages/hr or %/hr)
    QString speedStr = "--";
    QString pagesPerHourStr = "--";
    if (currentPage > 0 && totalSecs > 30) {
        double secsPerPage = (double)totalSecs / (double)currentPage;
        if (secsPerPage >= 60.0) {
            speedStr = QString("~%1 %2").arg(QString::number(secsPerPage / 60.0, 'f', 1), m_language == "vi" ? "phút/trang" : "min/pg");
        } else {
            speedStr = QString("~%1 %2").arg(QString::number((int)secsPerPage), m_language == "vi" ? "giây/trang" : "sec/pg");
        }
        double pph = (double)currentPage * 3600.0 / (double)totalSecs;
        pagesPerHourStr = QString("~%1 %2").arg(QString::number(pph, 'f', 0), m_language == "vi" ? "trang/giờ" : "pgs/hr");
    } else if (progress > 0 && totalSecs > 30) {
        double pctPerHour = ((double)progress / (double)totalSecs) * 3600.0;
        speedStr = QString("~%1% / %2").arg(qRound(pctPerHour)).arg(m_language == "vi" ? "giờ" : "hr");
        pagesPerHourStr = "--";
    } else {
        speedStr = m_language == "vi" ? "Đang ghi nhận..." : "Tracking...";
        pagesPerHourStr = "--";
    }
    stats["readingSpeedStr"] = speedStr;
    stats["pagesPerHourStr"] = pagesPerHourStr;

    // Remaining time estimation (KOReader style)
    QString remainingStr = "--";
    if (progress >= 100) {
        remainingStr = m_language == "vi" ? "Đã đọc xong" : "Completed";
    } else if (progress > 0 && totalSecs > 30) {
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

    // First: current or last book
    if (!m_currentFilePath.isEmpty()) {
        list.append(getBookDetailedStats(m_currentFilePath));
        seenPaths.insert(m_currentFilePath);
    } else if (!m_lastBookPath.isEmpty()) {
        list.append(getBookDetailedStats(m_lastBookPath));
        seenPaths.insert(m_lastBookPath);
    }

    // Second: all books in m_bookReadingSeconds
    QMapIterator<QString, int> it(m_bookReadingSeconds);
    while (it.hasNext()) {
        it.next();
        if (it.value() > 0 && !seenPaths.contains(it.key())) {
            list.append(getBookDetailedStats(it.key()));
            seenPaths.insert(it.key());
        }
    }

    // Third: books in m_allBooks that were opened
    for (int i = 0; i < m_allBooks.size(); ++i) {
        QString path = m_allBooks[i].value("filePath").toString();
        if (!seenPaths.contains(path) && (m_allBooks[i].value("progress", 0).toInt() > 0 || m_allBooks[i].value("isOpened", false).toBool())) {
            list.append(getBookDetailedStats(path));
            seenPaths.insert(path);
        }
    }

    return list;
}

QVariantList BookManager::getRecentDaysTrend(int numDays)
{
    QVariantList list;
    if (numDays <= 0) numDays = 7;

    int maxSecs = 1800; // minimum scale = 30 mins so tiny reading days don't fill 100%

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
    int epubSecs = 0, pdfSecs = 0, txtSecs = 0, fb2Secs = 0, otherSecs = 0;
    int epubBooks = 0, pdfBooks = 0, txtBooks = 0, fb2Books = 0, otherBooks = 0;

    for (int i = 0; i < m_allBooks.size(); ++i) {
        QString path = m_allBooks[i].value("filePath").toString();
        QString fmt = m_allBooks[i].value("format").toString().toLower();
        int secs = m_bookReadingSeconds.value(path, 0);

        if (fmt == "epub") { epubSecs += secs; epubBooks++; }
        else if (fmt == "pdf") { pdfSecs += secs; pdfBooks++; }
        else if (fmt == "txt") { txtSecs += secs; txtBooks++; }
        else if (fmt == "fb2") { fb2Secs += secs; fb2Books++; }
        else { otherSecs += secs; otherBooks++; }
    }

    int totalSecs = epubSecs + pdfSecs + txtSecs + fb2Secs + otherSecs;
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
    emit statsChanged();
    emit dailyStatsChanged();
}

void BookManager::updateReadingHistoryModel()
{
    if (!m_readingHistoryModel) return;
    m_readingHistoryModel->clear();

    QString q = m_recentSearchQuery.trimmed().toLower();

    QSettings settings("BerryLife", "BerryReader");
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

    emit recentHistoryCountChanged();
    emit statsChanged();
}

// =============================================================================
// HIGHLIGHTS & NOTES IMPLEMENTATION
// =============================================================================

void BookManager::saveHighlights()
{
    QSettings settings("BerryLife", "BerryReader");
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
    QSettings settings("BerryLife", "BerryReader");
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

    if (bookPath != m_currentFilePath && !bookPath.isEmpty()) {
        openBook(bookPath);
    }

    goToChapter(targetChapter);
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
            QString textEsc = hl.value("text").toString();
            textEsc.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", " ").replace("\r", "");
            QString noteEsc = hl.value("note").toString();
            noteEsc.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", " ").replace("\r", "");
            json += QString("{\"id\":\"%1\",\"text\":\"%2\",\"color\":\"%3\",\"note\":\"%4\"}")
                    .arg(hl.value("id").toString())
                    .arg(textEsc)
                    .arg(hl.value("color").toString())
                    .arg(noteEsc);
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
    QSettings settings("BerryLife", "BerryReader");
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
    QSettings settings("BerryLife", "BerryReader");
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
            QRegExp tagRx("<[^>]*>");
            raw.remove(tagRx);
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

    if (bookPath != m_currentFilePath && !bookPath.isEmpty()) {
        openBook(bookPath);
    }

    goToChapter(targetChapter);
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
    return persistentDataDir() + "/berryreader_bookmarks.json";
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

QString BookManager::collectDiagnosticReport() const
{
    QString report;
    QTextStream out(&report);

    out << "================================================================================\n";
    out << "BERRYREADER DIAGNOSTIC SYSTEM LOG REPORT\n";
    out << "Generated Time : " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
    out << "================================================================================\n\n";

    // 1. Hardware & System
    bb::device::HardwareInfo hwInfo;
    bb::device::DisplayInfo dispInfo;
    bb::platform::PlatformInfo platInfo;

    out << "[HARDWARE & PLATFORM]\n";
    out << "Device Model   : " << hwInfo.modelName() << "\n";
    out << "Device Name    : " << hwInfo.deviceName() << "\n";
    out << "Hardware ID    : " << hwInfo.hardwareId() << "\n";
    out << "Display Res    : " << dispInfo.pixelSize().width() << "x" << dispInfo.pixelSize().height() << "\n";
    out << "OS Version     : " << platInfo.osVersion() << "\n";
    out << "Battery Level  : " << m_batteryInfo.level() << "%\n\n";

    // 2. Active Application State
    out << "[ACTIVE APP STATE]\n";
    out << "Current File   : " << m_currentFilePath << "\n";
    out << "Current Title  : " << m_currentBookTitle << "\n";
    out << "Current Format : " << m_currentFormat << "\n";
    out << "Current Author : " << m_currentBookAuthor << "\n";
    out << "Chapter Index  : " << (m_currentChapterIndex + 1) << " / " << m_totalChapters << "\n";
    out << "Page In Chapter: " << m_currentPageInChapter << " / " << m_totalPagesInChapter << "\n";
    out << "Scroll Position: " << m_currentScrollPos << "\n";
    out << "Theme          : Reader=" << m_theme << ", App=" << m_appTheme << "\n";
    out << "Font Settings  : Family=" << m_fontFamily << ", Size=" << m_fontSize << ", Spacing=" << m_lineSpacing << "\n";
    out << "Margins (L/R/T/B): " << m_marginLeft << "/" << m_marginRight << "/" << m_marginTop << "/" << m_marginBottom << "\n";
    out << "Total Books    : " << m_allBooks.size() << "\n";
    out << "Main View Mode : " << m_mainViewMode << "\n";
    out << "Browse Mode    : " << m_libraryBrowseMode << "\n\n";

    // 3. Runtime Log Extracts
    out << "================================================================================\n";
    out << "[RECENT RUNTIME LOGS]\n";
    out << "================================================================================\n";

    QString logFilePath = "/accounts/1000/shared/documents/berryreader.log";
    QFile logFile(logFilePath);
    if (!logFile.exists() || logFile.size() == 0) {
        logFilePath = QDir::homePath() + "/data/berryreader.log";
        logFile.setFileName(logFilePath);
    }

    if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QStringList lines;
        QTextStream in(&logFile);
        in.setCodec("UTF-8");
        while (!in.atEnd()) {
            lines.append(in.readLine());
            if (lines.size() > 400) {
                lines.removeFirst();
            }
        }
        logFile.close();
        out << "Source: " << logFilePath << " (" << lines.size() << " recent lines)\n";
        out << "--------------------------------------------------------------------------------\n";
        foreach (const QString &line, lines) {
            out << line << "\n";
        }
    } else {
        out << "Could not open log file from shared or sandbox path: " << logFilePath << "\n";
    }

    out << "\n================================================================================\n";
    out << "END OF DIAGNOSTIC LOG REPORT\n";
    out << "================================================================================\n";

    return report;
}

void BookManager::sendLog(const QString &targetIp)
{
    Q_UNUSED(targetIp);
    qDebug() << "[BookManager::sendLog] Send log feature disabled.";
}

void BookManager::onDevLogPostFinished()
{
    m_logTimeoutTimer.stop();
    if (!m_devLogReply) return;

    if (m_devLogReply->error() == QNetworkReply::NoError) {
        QByteArray resp = m_devLogReply->readAll();
        qDebug() << "[BookManager::sendLog] Direct POST success:" << resp;
        m_devLogReply->deleteLater();
        m_devLogReply = NULL;

        finishLogSend(true, tr("Đã gửi trực tiếp tới máy chủ phát triển (%1:8080) thành công!").arg(m_devServerIp));
    } else {
        qDebug() << "[BookManager::sendLog] Direct POST failed (" << m_devLogReply->errorString()
                 << "), triggering cloud fallback...";
        m_devLogReply->deleteLater();
        m_devLogReply = NULL;

        triggerCloudFallback(m_cachedDiagnosticReport);
    }
}

void BookManager::onLogPostTimeout()
{
    if (m_devLogReply && m_devLogReply->isRunning()) {
        qDebug() << "[BookManager::sendLog] Dev server connection timed out after 4s. Switching to cloud fallback...";
        m_devLogReply->abort();
        m_devLogReply->deleteLater();
        m_devLogReply = NULL;
        triggerCloudFallback(m_cachedDiagnosticReport);
    }
}

void BookManager::triggerCloudFallback(const QString &reportData)
{
    m_lastLogStatus = tr("Không thể kết nối máy chủ nội bộ. Đang tải lên đám mây (dpaste)...");
    emit lastLogStatusChanged();

    QUrl dpasteUrl("https://dpaste.org/api/");
    QNetworkRequest request(dpasteUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("User-Agent", "BerryReader/2.0.0 (BlackBerry 10; Native)");

    QByteArray body = "format=url&expiry_days=7&content=" + QUrl::toPercentEncoding(reportData);

    if (m_cloudLogReply) {
        m_cloudLogReply->abort();
        m_cloudLogReply->deleteLater();
        m_cloudLogReply = NULL;
    }

    m_cloudLogReply = m_logNetworkManager->post(request, body);
    connect(m_cloudLogReply, SIGNAL(finished()), this, SLOT(onCloudLogPostFinished()));

    // 8-second timeout for cloud fallback
    m_logTimeoutTimer.setInterval(8000);
    disconnect(&m_logTimeoutTimer, 0, 0, 0);
    connect(&m_logTimeoutTimer, SIGNAL(timeout()), this, SLOT(onCloudLogTimeout()));
    m_logTimeoutTimer.start();
}

void BookManager::onCloudLogPostFinished()
{
    m_logTimeoutTimer.stop();
    if (!m_cloudLogReply) return;

    if (m_cloudLogReply->error() == QNetworkReply::NoError) {
        QString pasteUrl = QString::fromUtf8(m_cloudLogReply->readAll()).trimmed();
        m_cloudLogReply->deleteLater();
        m_cloudLogReply = NULL;

        bb::system::Clipboard clipboard;
        clipboard.clear();
        clipboard.insert("text/plain", pasteUrl.toUtf8());

        qDebug() << "[BookManager::sendLog] Cloud paste success:" << pasteUrl;
        finishLogSend(true, tr("Đã tải log lên đám mây và sao chép link vào bộ nhớ tạm: %1").arg(pasteUrl), pasteUrl);
    } else {
        qDebug() << "[BookManager::sendLog] Cloud fallback failed:" << m_cloudLogReply->errorString();
        m_cloudLogReply->deleteLater();
        m_cloudLogReply = NULL;

        finishLogSend(false, tr("Không thể gửi qua mạng. Đã lưu báo cáo tại: Documents/BerryReader_Debug.log"));
    }
}

void BookManager::onCloudLogTimeout()
{
    if (m_cloudLogReply && m_cloudLogReply->isRunning()) {
        qDebug() << "[BookManager::sendLog] Cloud fallback timed out after 8s.";
        m_cloudLogReply->abort();
        m_cloudLogReply->deleteLater();
        m_cloudLogReply = NULL;
        finishLogSend(false, tr("Hết thời gian chờ kết nối. Đã lưu báo cáo tại: Documents/BerryReader_Debug.log"));
    }
}

void BookManager::finishLogSend(bool success, const QString &status, const QString &url)
{
    m_isSendingLog = false;
    m_lastLogStatus = status;
    emit isSendingLogChanged();
    emit lastLogStatusChanged();
    emit logSendCompleted(success, status, url);
}

// =============================================================================
// PERSISTENT SHARED STORAGE (SURVIVES APP REINSTALLATION)
// =============================================================================

QString BookManager::persistentDataDir() const
{
    QString baseDir;
    if (QDir("/accounts/1000/shared/documents").exists() || QDir("/accounts/1000/shared").exists()) {
        baseDir = "/accounts/1000/shared/documents/BerryReader";
    } else {
        baseDir = QDir::homePath() + "/.berryreader";
    }
    QDir dir(baseDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return baseDir;
}

QString BookManager::persistentDataFilePath() const
{
    return persistentDataDir() + "/berryreader_userdata.ini";
}

QString BookManager::persistentJsonFilePath() const
{
    return persistentDataDir() + "/berryreader_userdata.json";
}

QString BookManager::persistentHighlightsPath() const
{
    return persistentDataDir() + "/berryreader_highlights.json";
}

QString BookManager::persistentStorageStatus() const
{
    if (m_persistentStorageStatus.isEmpty()) {
        return tr("Bộ nhớ ngoài sẵn sàng: ") + persistentDataDir();
    }
    return m_persistentStorageStatus;
}

void BookManager::syncToPersistentStorage()
{
    static bool s_isSyncing = false;
    if (s_isSyncing) return;
    s_isSyncing = true;

    QString dirPath = persistentDataDir();
    QDir().mkpath(dirPath);

    QSettings settings("BerryLife", "BerryReader");
    QString iniPath = persistentDataFilePath();
    QSettings persistentSettings(iniPath, QSettings::IniFormat);

    QStringList allKeys = settings.allKeys();
    foreach (const QString &k, allKeys) {
        persistentSettings.setValue(k, settings.value(k));
    }
    persistentSettings.setValue("meta/lastSyncTime", (qlonglong)QDateTime::currentDateTime().toMSecsSinceEpoch());
    persistentSettings.setValue("meta/syncTimestamp", QDateTime::currentDateTime().toString(Qt::ISODate));
    persistentSettings.setValue("meta/appVersion", "2.0.0");
    persistentSettings.sync();

    // Export berryreader_userdata.json
    QString jsonPath = persistentJsonFilePath();
    QFile jsonFile(jsonPath);
    if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream out(&jsonFile);
        out.setCodec("UTF-8");

        out << "{\n";
        out << "  \"app\": \"BerryReader\",\n";
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
        jsonFile.close();
    }

    // Export berryreader_highlights.json
    QString hlPath = persistentHighlightsPath();
    QFile hlFile(hlPath);
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
        hlFile.close();
    }

    // Export berryreader_bookmarks.json
    QString bmPath = persistentBookmarksPath();
    QFile bmFile(bmPath);
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
        bmFile.close();
    }

    m_persistentStorageStatus = tr("Đã đồng bộ dữ liệu vào ") + dirPath;
    emit persistentStorageStatusChanged();

    s_isSyncing = false;
}

void BookManager::restoreFromPersistentStorage()
{
    QString iniPath = persistentDataFilePath();
    QString jsonPath = persistentJsonFilePath();
    QFileInfo iniFi(iniPath);
    QFileInfo jsonFi(jsonPath);

    QSettings settings("BerryLife", "BerryReader");
    bool isSandboxEmpty = settings.allKeys().isEmpty() || !settings.contains("stats/totalReadingSeconds");

    if (iniFi.exists()) {
        QSettings persistentSettings(iniPath, QSettings::IniFormat);
        QStringList pKeys = persistentSettings.allKeys();
        if (!pKeys.isEmpty()) {
            if (isSandboxEmpty) {
                qDebug() << "[BookManager] Detected reinstalled app or empty sandbox. Restoring from:" << iniPath << "(Keys count:" << pKeys.size() << ")";
                foreach (const QString &k, pKeys) {
                    if (k.startsWith("meta/")) continue;
                    settings.setValue(k, persistentSettings.value(k));
                }
                settings.sync();
                m_persistentStorageStatus = tr("Đã khôi phục toàn bộ dữ liệu từ bộ nhớ ngoài.");
            } else {
                int restoredCount = 0;
                foreach (const QString &k, pKeys) {
                    if (k.startsWith("meta/")) continue;
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
    } else if (jsonFi.exists() && isSandboxEmpty) {
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
    saveCurrentProgress();
    saveSettings();
    pauseTimersForSleep();
    if (m_tts && m_tts->isActive()) {
        m_tts->stop();
    }
    if (bb::cascades::Application::instance()) {
        bb::cascades::Application::instance()->requestExit();
    }
    QCoreApplication::quit();
}

