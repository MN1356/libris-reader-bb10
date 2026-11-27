#ifndef BOOKMANAGER_HPP
#define BOOKMANAGER_HPP

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QSet>
#include <QTimer>
#include <bb/cascades/DataModel>
#include <bb/cascades/GroupDataModel>
#include <bb/cascades/ArrayDataModel>
#include <bb/cascades/Image>
#include <bb/device/BatteryInfo>
#include <bb/device/HardwareInfo>
#include <bb/platform/PlatformInfo>
#include <bb/system/Clipboard>
#include <bb/multimedia/MediaKey>
#include "EpubParser.hpp"
#include "TxtParser.hpp"
#include "Fb2Parser.hpp"
#include "CbzParser.hpp"
#include "MobiParser.hpp"
#include "AppTranslator.hpp"
#include "TtsManager.hpp"
#include "DictManager.hpp"
#include "PdfParser.hpp"
#include "WiFiDropServer.hpp"

namespace bb {
    namespace cascades {
        class Control;
    }
    namespace multimedia {
        class MediaKeyWatcher;
    }
}

class SearchWorker;
class ScanWorker;
class CoverExtractWorker;
class TlsHttpWorker;
class OpdsManager;
class QSettings;
class WiFiDropServer;
class QSettings;

class BookManager : public QObject {
    Q_OBJECT

    // Main View Mode: "shelf" (Bookshelf) or "browser" (File Browser)
    Q_PROPERTY(QString mainViewMode READ mainViewMode WRITE setMainViewMode NOTIFY mainViewModeChanged)

    // Application UI Theme: "dark" or "light"
    Q_PROPERTY(QString appTheme READ appTheme WRITE setAppTheme NOTIFY appThemeChanged)
    Q_PROPERTY(bool onboardingCompleted READ onboardingCompleted WRITE setOnboardingCompleted NOTIFY onboardingCompletedChanged)
    Q_PROPERTY(QString textAlign READ textAlign WRITE setTextAlign NOTIFY textAlignChanged)
    Q_PROPERTY(bool overridePublisherCss READ overridePublisherCss WRITE setOverridePublisherCss NOTIFY overridePublisherCssChanged)
    Q_PROPERTY(QString paragraphIndent READ paragraphIndent WRITE setParagraphIndent NOTIFY typographyChanged)
    Q_PROPERTY(QString paragraphSpacing READ paragraphSpacing WRITE setParagraphSpacing NOTIFY typographyChanged)
    Q_PROPERTY(int readerFontWeight READ readerFontWeight WRITE setReaderFontWeight NOTIFY typographyChanged)
    Q_PROPERTY(QString darkTextTone READ darkTextTone WRITE setDarkTextTone NOTIFY typographyChanged)
    Q_PROPERTY(bool showFooterProgressBar READ showFooterProgressBar WRITE setShowFooterProgressBar NOTIFY footerSettingsChanged)
    Q_PROPERTY(bool showFooterTimeLeft READ showFooterTimeLeft WRITE setShowFooterTimeLeft NOTIFY footerSettingsChanged)
    Q_PROPERTY(int eyeRestMinutes READ eyeRestMinutes WRITE setEyeRestMinutes NOTIFY readingPrefsChanged)
    Q_PROPERTY(bool invertVolumeKeys READ invertVolumeKeys WRITE setInvertVolumeKeys NOTIFY readingPrefsChanged)
    Q_PROPERTY(bool bionicReading READ bionicReading WRITE setBionicReading NOTIFY typographyChanged)

    // Scanning & Loading status
    Q_PROPERTY(bool isScanning READ isScanning NOTIFY isScanningChanged)
    Q_PROPERTY(bool isLoadingBook READ isLoadingBook WRITE setIsLoadingBook NOTIFY isLoadingBookChanged)
    Q_PROPERTY(QString scanStatusText READ scanStatusText NOTIFY scanStatusTextChanged)

    // Hierarchical File Browser
    Q_PROPERTY(bb::cascades::DataModel* fileBrowserModel READ fileBrowserModel CONSTANT)
    Q_PROPERTY(QString currentBrowserPath READ currentBrowserPath NOTIFY currentBrowserPathChanged)
    Q_PROPERTY(QString currentBrowserTitle READ currentBrowserTitle NOTIFY currentBrowserPathChanged)
    Q_PROPERTY(bool canGoUp READ canGoUp NOTIFY currentBrowserPathChanged)
    Q_PROPERTY(int browserItemCount READ browserItemCount NOTIFY browserItemCountChanged)
    Q_PROPERTY(QString browserSearchQuery READ browserSearchQuery WRITE setBrowserSearchQuery NOTIFY browserSearchQueryChanged)
    Q_PROPERTY(QString browserFormatFilter READ browserFormatFilter WRITE setBrowserFormatFilter NOTIFY browserFormatFilterChanged)
    Q_PROPERTY(QString recentSearchQuery READ recentSearchQuery WRITE setRecentSearchQuery NOTIFY recentSearchQueryChanged)
    Q_PROPERTY(int recentHistoryCount READ recentHistoryCount NOTIFY recentHistoryCountChanged)

    // Reading Statistics & Overview
    Q_PROPERTY(bool isReaderActive READ isReaderActive WRITE setIsReaderActive NOTIFY isReaderActiveChanged)
    Q_PROPERTY(QString totalReadingTimeStr READ totalReadingTimeStr NOTIFY statsChanged)
    Q_PROPERTY(QString todayReadingTimeStr READ todayReadingTimeStr NOTIFY statsChanged)
    Q_PROPERTY(int totalReadingSeconds READ totalReadingSeconds NOTIFY statsChanged)
    Q_PROPERTY(int todayReadingSeconds READ todayReadingSeconds NOTIFY statsChanged)
    Q_PROPERTY(int totalLibraryBooks READ totalLibraryBooks NOTIFY totalScannedBooksChanged)
    Q_PROPERTY(int completedBooksCount READ completedBooksCount NOTIFY statsChanged)
    Q_PROPERTY(int readingBooksCount READ readingBooksCount NOTIFY statsChanged)
    Q_PROPERTY(int unreadBooksCount READ unreadBooksCount NOTIFY statsChanged)
    Q_PROPERTY(int averageProgress READ averageProgress NOTIFY statsChanged)
    Q_PROPERTY(int currentStreak READ currentStreak NOTIFY statsChanged)
    Q_PROPERTY(int dailyGoalMinutes READ dailyGoalMinutes WRITE setDailyGoalMinutes NOTIFY statsChanged)
    Q_PROPERTY(bb::cascades::DataModel* readingHistoryModel READ readingHistoryModel CONSTANT)
    Q_PROPERTY(bb::cascades::DataModel* homeFeedModel READ homeFeedModel CONSTANT)

    Q_PROPERTY(bb::cascades::DataModel* model READ model NOTIFY modelChanged)
    Q_PROPERTY(bb::cascades::DataModel* folderModel READ folderModel CONSTANT)
    Q_PROPERTY(int bookCount READ bookCount NOTIFY bookCountChanged)
    Q_PROPERTY(int totalScannedBooks READ totalScannedBooks NOTIFY totalScannedBooksChanged)
    Q_PROPERTY(QString libraryViewMode READ libraryViewMode WRITE setLibraryViewMode NOTIFY libraryViewModeChanged)
    Q_PROPERTY(QString libraryBrowseMode READ libraryBrowseMode WRITE setLibraryBrowseMode NOTIFY libraryBrowseModeChanged)
    Q_PROPERTY(QString selectedFolderPath READ selectedFolderPath NOTIFY selectedFolderChanged)
    Q_PROPERTY(QString selectedFolderName READ selectedFolderName NOTIFY selectedFolderChanged)
    Q_PROPERTY(int screenWidth READ screenWidth CONSTANT)
    Q_PROPERTY(int screenHeight READ screenHeight CONSTANT)
    Q_PROPERTY(bool isSquareScreen READ isSquareScreen CONSTANT)
    Q_PROPERTY(bool isHiRes READ isHiRes CONSTANT)
    Q_PROPERTY(double uiScale READ uiScale CONSTANT)
    Q_PROPERTY(QString uiBg READ uiBg NOTIFY appThemeChanged)
    Q_PROPERTY(QString uiCard READ uiCard NOTIFY appThemeChanged)
    Q_PROPERTY(QString uiSurface READ uiSurface NOTIFY appThemeChanged)
    Q_PROPERTY(QString uiActive READ uiActive NOTIFY appThemeChanged)
    Q_PROPERTY(QString uiHairline READ uiHairline NOTIFY appThemeChanged)
    Q_PROPERTY(QString uiInk READ uiInk NOTIFY appThemeChanged)
    Q_PROPERTY(QString uiMuted READ uiMuted NOTIFY appThemeChanged)
    Q_PROPERTY(QString uiAccent READ uiAccent NOTIFY appThemeChanged)
    Q_PROPERTY(QString uiDanger READ uiDanger NOTIFY appThemeChanged)
    Q_PROPERTY(QString sortBy READ sortBy WRITE setSortBy NOTIFY sortByChanged)
    Q_PROPERTY(QString groupBy READ groupBy WRITE setGroupBy NOTIFY groupByChanged)
    Q_PROPERTY(QString formatFilter READ formatFilter WRITE setFormatFilter NOTIFY formatFilterChanged)
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY categoryFilterChanged)
    Q_PROPERTY(QStringList customShelves READ customShelves NOTIFY customShelvesChanged)
    Q_PROPERTY(QStringList scanFolders READ scanFolders NOTIFY scanFoldersChanged)
    Q_PROPERTY(QString scanFoldersSummary READ scanFoldersSummary NOTIFY scanFoldersChanged)
    Q_PROPERTY(bool updateChecking READ updateChecking NOTIFY updateStateChanged)
    Q_PROPERTY(bool updateDownloading READ updateDownloading NOTIFY updateStateChanged)
    Q_PROPERTY(int updateProgress READ updateProgress NOTIFY updateStateChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateStateChanged)
    Q_PROPERTY(QString updateLatestVersion READ updateLatestVersion NOTIFY updateStateChanged)
    Q_PROPERTY(QString updateReleaseNotes READ updateReleaseNotes NOTIFY updateStateChanged)
    Q_PROPERTY(QString updateAssetName READ updateAssetName NOTIFY updateStateChanged)
    Q_PROPERTY(QString updateAssetSizeStr READ updateAssetSizeStr NOTIFY updateStateChanged)
    Q_PROPERTY(QString updateDownloadUrl READ updateDownloadUrl NOTIFY updateStateChanged)
    Q_PROPERTY(QString updateStatusText READ updateStatusText NOTIFY updateStateChanged)
    Q_PROPERTY(bool updateReadyToInstall READ updateReadyToInstall NOTIFY updateStateChanged)
    Q_PROPERTY(QString updateTargetBarPath READ updateTargetBarPath NOTIFY updateStateChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(TtsManager* tts READ ttsManager CONSTANT)
    Q_PROPERTY(DictManager* dict READ dictManager CONSTANT)
    Q_PROPERTY(OpdsManager* opds READ opdsManager CONSTANT)
    Q_PROPERTY(WiFiDropServer* wifiDropServer READ wifiDropServer CONSTANT)
    Q_PROPERTY(bool ttsAutoAdvance READ ttsAutoAdvance WRITE setTtsAutoAdvance NOTIFY ttsAutoAdvanceChanged)

    // Table of contents, search, and PDF reflow/OCR
    Q_PROPERTY(QVariantList tocList READ tocList NOTIFY tocListChanged)
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(bool isSearching READ isSearching NOTIFY isSearchingChanged)
    Q_PROPERTY(bool isPdfReflow READ isPdfReflow WRITE setIsPdfReflow NOTIFY isPdfReflowChanged)
    Q_PROPERTY(bool hasPdfText READ hasPdfText NOTIFY hasPdfTextChanged)
    Q_PROPERTY(bool pdfHasOcr READ pdfHasOcr NOTIFY hasPdfTextChanged)
    Q_PROPERTY(QString currentPdfText READ currentPdfText NOTIFY currentPdfTextChanged)

    // Highlights & Notes
    Q_PROPERTY(QVariantList currentBookHighlights READ currentBookHighlights NOTIFY highlightsChanged)
    Q_PROPERTY(QVariantList allHighlights READ allHighlights NOTIFY highlightsChanged)
    Q_PROPERTY(int currentBookHighlightCount READ currentBookHighlightCount NOTIFY highlightsChanged)
    Q_PROPERTY(int totalHighlightCount READ totalHighlightCount NOTIFY highlightsChanged)
    Q_PROPERTY(QString lastAddedHighlightId READ lastAddedHighlightId NOTIFY highlightsChanged)

    // Bookmarks
    Q_PROPERTY(QVariantList currentBookBookmarks READ currentBookBookmarks NOTIFY bookmarksChanged)
    Q_PROPERTY(QVariantList allBookmarks READ allBookmarks NOTIFY bookmarksChanged)
    Q_PROPERTY(int currentBookBookmarkCount READ currentBookBookmarkCount NOTIFY bookmarksChanged)
    Q_PROPERTY(int totalBookmarkCount READ totalBookmarkCount NOTIFY bookmarksChanged)
    Q_PROPERTY(bool isCurrentChapterBookmarked READ isCurrentChapterBookmarked NOTIFY bookmarksChanged)

    Q_PROPERTY(QString currentBookTitle READ currentBookTitle NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookAuthor READ currentBookAuthor NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookSeries READ currentBookSeries NOTIFY currentBookChanged)
    Q_PROPERTY(int currentBookSeriesIndex READ currentBookSeriesIndex NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentCoverPath READ currentCoverPath NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookDescription READ currentBookDescription NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookGenre READ currentBookGenre NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookFileSize READ currentBookFileSize NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookDateModified READ currentBookDateModified NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookTimeSpentStr READ currentBookTimeSpentStr NOTIFY statsChanged)
    Q_PROPERTY(int currentChapterIndex READ currentChapterIndex NOTIFY currentChapterChanged)
    Q_PROPERTY(int totalChapters READ totalChapters NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentChapterTitle READ currentChapterTitle NOTIFY currentChapterChanged)
    Q_PROPERTY(QString currentContent READ currentContent NOTIFY currentChapterChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(QString currentFontDisplayName READ currentFontDisplayName NOTIFY fontFamilyChanged)
    Q_PROPERTY(QVariantList customFonts READ customFonts NOTIFY customFontsChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool warmFilterEnabled READ warmFilterEnabled WRITE setWarmFilterEnabled NOTIFY warmFilterChanged)
    Q_PROPERTY(int warmFilterIntensity READ warmFilterIntensity WRITE setWarmFilterIntensity NOTIFY warmFilterChanged)
    Q_PROPERTY(int readerBrightness READ readerBrightness WRITE setReaderBrightness NOTIFY readerBrightnessChanged)
    Q_PROPERTY(bool autoNightTheme READ autoNightTheme WRITE setAutoNightTheme NOTIFY autoNightThemeChanged)
    Q_PROPERTY(bool focusModeEnabled READ focusModeEnabled WRITE setFocusModeEnabled NOTIFY focusModeChanged)
    Q_PROPERTY(bool volumeKeyTurnEnabled READ volumeKeyTurnEnabled WRITE setVolumeKeyTurnEnabled NOTIFY volumeKeyTurnEnabledChanged)
    Q_PROPERTY(QVariantMap dailyRecallQuote READ dailyRecallQuote NOTIFY highlightsChanged)
    Q_PROPERTY(QString readingMode READ readingMode WRITE setReadingMode NOTIFY readingModeChanged)
    Q_PROPERTY(QString columnLayout READ columnLayout WRITE setColumnLayout NOTIFY columnLayoutChanged)
    Q_PROPERTY(bool isTwoColumnActive READ isTwoColumnActive NOTIFY columnLayoutChanged)
    Q_PROPERTY(bool mangaRtlMode READ mangaRtlMode WRITE setMangaRtlMode NOTIFY mangaRtlModeChanged)
    Q_PROPERTY(int readingProgress READ readingProgress NOTIFY readingProgressChanged)
    Q_PROPERTY(int currentPageInChapter READ currentPageInChapter NOTIFY pagePositionChanged)
    Q_PROPERTY(int totalPagesInChapter READ totalPagesInChapter NOTIFY pagePositionChanged)

    // Format and PDF support
    Q_PROPERTY(QString currentFormat READ currentFormat NOTIFY currentBookChanged)
    Q_PROPERTY(bool isPdf READ isPdf NOTIFY currentBookChanged)
    Q_PROPERTY(bb::cascades::Image currentPdfImage READ currentPdfImage NOTIFY currentPdfImageChanged)
    Q_PROPERTY(QString currentPdfPagePath READ currentPdfPagePath NOTIFY currentPdfImageChanged)
    Q_PROPERTY(bool isPdfInverted READ isPdfInverted NOTIFY pdfInvertedChanged)
    Q_PROPERTY(bool isPdfAutoCrop READ isPdfAutoCrop WRITE setIsPdfAutoCrop NOTIFY pdfAutoCropChanged)
    Q_PROPERTY(bb::cascades::DataModel* pdfPagesModel READ pdfPagesModel CONSTANT)
    Q_PROPERTY(bool pdfZoomMode READ pdfZoomMode WRITE setPdfZoomMode NOTIFY pdfZoomModeChanged)

    // Last read book (Hero Card)
    Q_PROPERTY(bool hasLastBook READ hasLastBook NOTIFY lastBookChanged)
    Q_PROPERTY(QString lastBookTitle READ lastBookTitle NOTIFY lastBookChanged)
    Q_PROPERTY(QString lastBookAuthor READ lastBookAuthor NOTIFY lastBookChanged)
    Q_PROPERTY(QString lastBookCover READ lastBookCover NOTIFY lastBookChanged)
    Q_PROPERTY(QString lastBookFormat READ lastBookFormat NOTIFY lastBookChanged)
    Q_PROPERTY(QString lastBookPath READ lastBookPath NOTIFY lastBookChanged)
    Q_PROPERTY(int lastBookProgress READ lastBookProgress NOTIFY lastBookChanged)

    // Footer and line spacing settings
    Q_PROPERTY(bool showFooterChapter READ showFooterChapter WRITE setShowFooterChapter NOTIFY footerSettingsChanged)
    Q_PROPERTY(bool showFooterProgress READ showFooterProgress WRITE setShowFooterProgress NOTIFY footerSettingsChanged)
    Q_PROPERTY(bool showFooterBattery READ showFooterBattery WRITE setShowFooterBattery NOTIFY footerSettingsChanged)
    Q_PROPERTY(bool showFooterClock READ showFooterClock WRITE setShowFooterClock NOTIFY footerSettingsChanged)
    Q_PROPERTY(int batteryLevel READ batteryLevel NOTIFY batteryLevelChanged)
    Q_PROPERTY(QString currentTime READ currentTime NOTIFY currentTimeChanged)
    Q_PROPERTY(QString lineSpacing READ lineSpacing WRITE setLineSpacing NOTIFY lineSpacingChanged)

    // Page margins (Top, Bottom, Left, Right)
    Q_PROPERTY(int marginTop READ marginTop WRITE setMarginTop NOTIFY marginsChanged)
    Q_PROPERTY(int marginBottom READ marginBottom WRITE setMarginBottom NOTIFY marginsChanged)
    Q_PROPERTY(int marginLeft READ marginLeft WRITE setMarginLeft NOTIFY marginsChanged)
    Q_PROPERTY(int marginRight READ marginRight WRITE setMarginRight NOTIFY marginsChanged)

    // Remote Diagnostic & SendLog
    Q_PROPERTY(bool isSendingLog READ isSendingLog NOTIFY isSendingLogChanged)
    Q_PROPERTY(QString lastLogStatus READ lastLogStatus NOTIFY lastLogStatusChanged)
    Q_PROPERTY(QString devServerIp READ devServerIp WRITE setDevServerIp NOTIFY devServerIpChanged)

    // Persistent Shared Storage (Data survivability across app reinstalls)
    Q_PROPERTY(QString persistentStorageDir READ persistentDataDir CONSTANT)
    Q_PROPERTY(QString persistentStorageStatus READ persistentStorageStatus NOTIFY persistentStorageStatusChanged)

    // App Version, Build & Changelog (synchronized on every build bump)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString buildNumber READ buildNumber CONSTANT)
    Q_PROPERTY(QString changelogText READ changelogText NOTIFY languageChanged)

public:
    explicit BookManager(QObject *parent = 0);
    ~BookManager();

    bb::cascades::DataModel* model() const;
    int bookCount() const;
    int totalScannedBooks() const;

    QString language() const;
    Q_INVOKABLE void setLanguage(const QString &lang);
    Q_INVOKABLE QString tr(const QString &sourceText) const;
    QString appVersion() const;
    QString buildNumber() const;
    QString changelogText() const;

    bb::cascades::DataModel* folderModel() const;
    QString libraryBrowseMode() const;
    Q_INVOKABLE void setLibraryBrowseMode(const QString &mode);
    Q_INVOKABLE void toggleLibraryBrowseMode();
    QString selectedFolderPath() const;
    QString selectedFolderName() const;
    Q_INVOKABLE void selectFolder(const QString &folderPath);
    Q_INVOKABLE void clearSelectedFolder();
    int screenWidth() const;
    int screenHeight() const;
    bool isSquareScreen() const;

    QString libraryViewMode() const;
    Q_INVOKABLE void setLibraryViewMode(const QString &mode);
    Q_INVOKABLE void toggleLibraryViewMode();

    QString sortBy() const;
    Q_INVOKABLE void setSortBy(const QString &sortBy);

    QString groupBy() const;
    Q_INVOKABLE void setGroupBy(const QString &groupBy);

    QString formatFilter() const;
    Q_INVOKABLE void setFormatFilter(const QString &format);

    QString categoryFilter() const;
    Q_INVOKABLE void setCategoryFilter(const QString &category);

    // Custom Shelves
    QStringList customShelves() const;
    Q_INVOKABLE bool createShelf(const QString &name);
    Q_INVOKABLE bool deleteShelf(const QString &name);
    Q_INVOKABLE bool renameShelf(const QString &oldName, const QString &newName);
    Q_INVOKABLE bool toggleBookInShelf(const QString &filePath, const QString &shelfName);
    Q_INVOKABLE bool isBookInShelf(const QString &filePath, const QString &shelfName) const;
    Q_INVOKABLE QStringList getBookShelves(const QString &filePath) const;
    Q_INVOKABLE int getShelfBookCount(const QString &shelfName) const;

    Q_INVOKABLE bool isFavorite(const QString &filePath) const;
    Q_INVOKABLE bool toggleFavorite(const QString &filePath);
    Q_INVOKABLE bool isReadingList(const QString &filePath) const;
    Q_INVOKABLE bool toggleReadingList(const QString &filePath);
    Q_INVOKABLE bool toggleBookCompleted(const QString &filePath);
    Q_INVOKABLE void removeBookFromLibrary(const QString &filePath);
    Q_INVOKABLE bool deleteBookFile(const QString &filePath);
    Q_INVOKABLE void removeFromRecent(const QString &filePath);
    Q_INVOKABLE void clearReadingHistory();
    Q_INVOKABLE void markAsUnread(const QString &filePath);

    QString searchQuery() const;
    Q_INVOKABLE void setSearchQuery(const QString &query);
    Q_INVOKABLE void resetFilters();

    QString currentBookTitle() const;
    QString currentBookAuthor() const;
    QString currentBookSeries() const;
    int currentBookSeriesIndex() const;
    QString currentCoverPath() const;
    QString currentBookDescription() const;
    QString currentBookGenre() const;
    QString currentBookFileSize() const;
    QString currentBookDateModified() const;
    QString currentFilePath() const;
    QString currentBookTimeSpentStr() const;
    int currentChapterIndex() const;
    int totalChapters() const;
    QString currentChapterTitle() const;
    QString currentContent() const;
    int fontSize() const;
    Q_INVOKABLE void setFontSize(int size);
    QString fontFamily() const;
    Q_INVOKABLE void setFontFamily(const QString &family);
    QString currentFontDisplayName() const;
    QVariantList customFonts() const;
    Q_INVOKABLE void refreshCustomFonts();
    Q_INVOKABLE QString currentFontCss() const;
    Q_INVOKABLE QString currentFontFaceCss() const;

    bool warmFilterEnabled() const;
    Q_INVOKABLE void setWarmFilterEnabled(bool enabled);
    int warmFilterIntensity() const;
    Q_INVOKABLE void setWarmFilterIntensity(int intensity);
    Q_INVOKABLE void toggleWarmFilter();
    int readerBrightness() const;
    Q_INVOKABLE void setReaderBrightness(int brightness);
    bool autoNightTheme() const;
    Q_INVOKABLE void setAutoNightTheme(bool enabled);
    Q_INVOKABLE void checkAutoNightTheme();
    bool focusModeEnabled() const;
    Q_INVOKABLE void setFocusModeEnabled(bool enabled);
    Q_INVOKABLE void toggleFocusMode();
    Q_INVOKABLE QVariantMap nextSeriesBook(const QString &filePath = QString()) const;
    Q_INVOKABLE QVariantList allBooksSnapshot() const;

    bool volumeKeyTurnEnabled() const;
    Q_INVOKABLE void setVolumeKeyTurnEnabled(bool enabled);

    QVariantMap dailyRecallQuote() const;
    Q_INVOKABLE QString exportQuoteCardImage(const QString &quoteText, const QString &bookTitle, const QString &author = QString(), const QString &note = QString());
    QString theme() const;
    Q_INVOKABLE void setTheme(const QString &theme);
    QString appTheme() const;
    Q_INVOKABLE void setAppTheme(const QString &theme);
    bool onboardingCompleted() const { return m_onboardingCompleted; }
    Q_INVOKABLE void setOnboardingCompleted(bool completed);
    Q_INVOKABLE void requestOnboardingGuide();
    bool isHiRes() const { return m_screenWidth >= 1200; }
    double uiScale() const { return (m_screenWidth >= 1200) ? 1.5 : 1.0; }
    Q_INVOKABLE int dp(double px) const { return (px == 0.0) ? 0 : qRound(px * uiScale()); }
    QString uiBg() const { return (m_appTheme == "dark") ? QString("#0E0C0A") : QString("#ECEAE6"); }
    QString uiCard() const { return (m_appTheme == "dark") ? QString("#181512") : QString("#FFFFFF"); }
    QString uiSurface() const { return (m_appTheme == "dark") ? QString("#221E1A") : QString("#DFDCD5"); }
    QString uiActive() const { return (m_appTheme == "dark") ? QString("#261F15") : QString("#E3D5BD"); }
    QString uiHairline() const { return (m_appTheme == "dark") ? QString("#26211B") : QString("#CCC6BC"); }
    QString uiInk() const { return (m_appTheme == "dark") ? QString("#F5EFE6") : QString("#1C1916"); }
    QString uiMuted() const { return (m_appTheme == "dark") ? QString("#9C9488") : QString("#635B51"); }
    QString uiAccent() const { return (m_appTheme == "dark") ? QString("#D9AB60") : QString("#8A5D16"); }
    QString uiDanger() const { return (m_appTheme == "dark") ? QString("#E06C65") : QString("#B43B34"); }
    QString readingMode() const;
    Q_INVOKABLE void setReadingMode(const QString &mode);
    Q_INVOKABLE void toggleReadingMode();
    QString columnLayout() const;
    Q_INVOKABLE void setColumnLayout(const QString &layout);
    Q_INVOKABLE void toggleColumnLayout();
    bool isTwoColumnActive() const;
    bool mangaRtlMode() const;
    Q_INVOKABLE void setMangaRtlMode(bool enabled);
    Q_INVOKABLE void toggleMangaRtlMode();
    Q_INVOKABLE void renderCurrentChapter();
    int readingProgress() const;
    int currentPageInChapter() const;
    int totalPagesInChapter() const;

    // Format and PDF getters & invokables
    QString currentFormat() const;
    bool isPdf() const;
    bb::cascades::Image currentPdfImage() const;
    QString currentPdfPagePath() const;
    bool isPdfInverted() const;
    bool isPdfAutoCrop() const;
    Q_INVOKABLE void setIsPdfAutoCrop(bool enabled);
    Q_INVOKABLE void togglePdfAutoCrop();

    // Main View Mode
    QString mainViewMode() const;
    Q_INVOKABLE void setMainViewMode(const QString &mode);
    Q_INVOKABLE void toggleMainViewMode();

    // Scanning & Loading status
    bool isScanning() const;
    bool isLoadingBook() const;
    Q_INVOKABLE void setIsLoadingBook(bool loading);
    QString scanStatusText() const;

    // Hierarchical File Browser
    bb::cascades::DataModel* fileBrowserModel() const;
    QString currentBrowserPath() const;
    QString currentBrowserTitle() const;
    bool canGoUp() const;
    int browserItemCount() const;

    Q_INVOKABLE void openBrowserFolder(const QString &folderPath);
    Q_INVOKABLE void browserGoUp();
    Q_INVOKABLE void refreshBrowser();
    QString browserSearchQuery() const { return m_browserSearchQuery; }
    QString browserFormatFilter() const { return m_browserFormatFilter; }
    Q_INVOKABLE void setBrowserSearchQuery(const QString &query);
    Q_INVOKABLE void setBrowserFormatFilter(const QString &format);
    Q_INVOKABLE void clearBrowserFilter();

    // Reading Statistics & Overview
    bool isReaderActive() const;
    Q_SLOT void setIsReaderActive(bool active);
    QString totalReadingTimeStr() const;
    QString todayReadingTimeStr() const;
    int totalReadingSeconds() const;
    int todayReadingSeconds() const;
    int totalLibraryBooks() const;
    int completedBooksCount() const;
    int readingBooksCount() const;
    int unreadBooksCount() const;
    int averageProgress() const;
    int currentStreak() const;
    int dailyGoalMinutes() const;
    Q_INVOKABLE void setDailyGoalMinutes(int minutes);
    bb::cascades::DataModel* readingHistoryModel() const;
    bb::cascades::DataModel* homeFeedModel() const;
    QString recentSearchQuery() const { return m_recentSearchQuery; }
    Q_INVOKABLE void setRecentSearchQuery(const QString &query);
    int recentHistoryCount() const { return m_readingHistoryModel ? m_readingHistoryModel->size() : 0; }
    Q_INVOKABLE void refreshStats();

    // Calendar & Detailed Reading Statistics (KOReader Style)
    Q_INVOKABLE int getDailyReadingSeconds(const QString &dateStr) const;
    Q_INVOKABLE QString getDailyReadingTimeStr(const QString &dateStr) const;
    Q_INVOKABLE QVariantList getCalendarDays(int year, int month);
    Q_INVOKABLE QVariantMap getWeekData(const QString &anchorDateStr);
    Q_INVOKABLE void setControlSpaceQuota(bb::cascades::Control *control, float quota);
    Q_INVOKABLE QVariantMap getMonthStats(int year, int month);
    Q_INVOKABLE QVariantList getDayBooks(const QString &dateStr);
    Q_INVOKABLE QVariantMap getOverallStats();
    Q_INVOKABLE QString formatDateYMD(int year, int month, int day) const;
    Q_INVOKABLE QString formatDuration(int seconds) const;
    Q_INVOKABLE QString formatShortDuration(int seconds) const;
    Q_INVOKABLE void exitApp();
    Q_INVOKABLE void promptExit();
    void cancelWorkers();
    Q_INVOKABLE void saveSettings();

    // KOReader-style Extended Reading Statistics
    Q_INVOKABLE QVariantList getBookStatsList();
    Q_INVOKABLE QVariantMap getBookDetailedStats(const QString &filePath);
    Q_INVOKABLE QVariantMap getBookMetadata(const QString &filePath);
    Q_INVOKABLE QVariantList getRecentDaysTrend(int numDays = 7);
    Q_INVOKABLE QVariantMap getFormatDistribution();

    // Last read book (Hero Card)
    bool hasLastBook() const;
    QString lastBookTitle() const;
    QString lastBookAuthor() const;
    QString lastBookCover() const;
    QString lastBookFormat() const;
    QString lastBookPath() const;
    int lastBookProgress() const;
    Q_INVOKABLE bool openLastBook();

    // Footer settings & Device info
    bool showFooterChapter() const;
    Q_INVOKABLE void setShowFooterChapter(bool show);
    bool showFooterProgress() const;
    Q_INVOKABLE void setShowFooterProgress(bool show);
    bool showFooterBattery() const;
    Q_INVOKABLE void setShowFooterBattery(bool show);
    bool showFooterClock() const;
    Q_INVOKABLE void setShowFooterClock(bool show);
    int batteryLevel() const;
    QString currentTime() const;
    QString lineSpacing() const;
    Q_INVOKABLE void setLineSpacing(const QString &spacing);

    // Page margins
    int marginTop() const { return m_marginTop; }
    int marginBottom() const { return m_marginBottom; }
    int marginLeft() const { return m_marginLeft; }
    int marginRight() const { return m_marginRight; }
    Q_INVOKABLE void setMarginTop(int margin);
    Q_INVOKABLE void setMarginBottom(int margin);
    Q_INVOKABLE void setMarginLeft(int margin);
    Q_INVOKABLE void setMarginRight(int margin);
    Q_INVOKABLE void resetMargins();

    Q_INVOKABLE bool openBook(const QString &filePath);
    Q_INVOKABLE void nextChapter();
    Q_INVOKABLE void prevChapter();
    Q_INVOKABLE void goToChapter(int index);
    Q_INVOKABLE QVariantList getChapterList() const;
    Q_INVOKABLE void setPagePosition(int current, int total, int scrollPos = 0);

public Q_SLOTS:
    void saveCurrentProgress();
    void saveForShutdown();
    void pauseTimersForSleep(bool saveProgress = true);
    void resumeTimersFromSleep();

    // TTS Support
    TtsManager* ttsManager();
    bool ttsAutoAdvance() const { return m_ttsAutoAdvance; }
    Q_INVOKABLE void setTtsAutoAdvance(bool advance);
    Q_INVOKABLE void toggleTts();
    Q_INVOKABLE void startTts();
    Q_INVOKABLE void stopTts();

    // Cover and progress cache & update
    Q_INVOKABLE void updateBookCoverInLibrary(const QString &filePath, const QString &coverPath);
    Q_INVOKABLE void updateBookProgressInLibrary(const QString &filePath, int progress);
    Q_INVOKABLE void registerBookFile(const QString &filePath);

    // PDF specific actions
    Q_INVOKABLE void rotatePdf();
    Q_INVOKABLE void togglePdfInvert();
    Q_INVOKABLE void zoomPdfIn();
    Q_INVOKABLE void zoomPdfOut();
    Q_INVOKABLE void zoomPdfFit();
    bb::cascades::DataModel* pdfPagesModel() const { return m_pdfPagesModel; }
    bool pdfZoomMode() const { return m_pdfZoomMode; }
    Q_INVOKABLE void setPdfZoomMode(bool enabled);
    Q_INVOKABLE void enterPdfZoomMode(int pageNumber);
    Q_INVOKABLE void exitPdfZoomMode();
    Q_INVOKABLE void releaseReaderMemory();
    Q_INVOKABLE void requestPdfStreamPage(int pageNumber);
    Q_INVOKABLE void setPdfVisiblePage(int pageNumber);

    // TOC, Search, Reflow and Dictionary
    DictManager* dictManager() { return &m_dictManager; }
    OpdsManager* opdsManager();
    WiFiDropServer* wifiDropServer() const { return m_wifiDropServer; }
    QString cacheDir() const { return m_cacheDir; }
    QVariantList tocList() const { return m_tocList; }
    QVariantList searchResults() const { return m_searchResults; }
    bool isSearching() const { return m_isSearching; }
    bool isPdfReflow() const { return m_isPdfReflow; }
    void setIsPdfReflow(bool reflow);
    bool hasPdfText() const { return m_hasPdfText; }
    bool pdfHasOcr() const { return m_hasPdfText || m_pdfHasAnyOcr; }
    QString currentPdfText() const { return m_currentPdfText; }
    Q_INVOKABLE QString getPdfPageText(int pageNumber = -1);

    Q_INVOKABLE void jumpToToc(int chapterIndex, int page = 0, const QString &anchor = QString());
    Q_INVOKABLE QString consumePendingScrollAnchor();
    Q_INVOKABLE void searchBook(const QString &query, const QString &scope = "full");
    Q_INVOKABLE void cancelSearch();
    Q_INVOKABLE void togglePdfReflow();
    Q_INVOKABLE void copyText(const QString &text);

    // Highlight & Note functions
    Q_INVOKABLE QString addHighlight(const QString &text, const QString &note = QString(), const QString &color = "yellow", int chapter = -1, const QString &chapterTitle = QString());
    Q_INVOKABLE bool updateHighlight(const QString &id, const QString &note, const QString &color = QString());
    Q_INVOKABLE bool updateHighlightNote(const QString &id, const QString &note);
    Q_INVOKABLE bool updateHighlightColor(const QString &id, const QString &color);
    Q_INVOKABLE bool removeHighlight(const QString &id);
    Q_INVOKABLE void clearHighlights(const QString &filePath = QString());
    Q_INVOKABLE QVariantMap getHighlight(const QString &id) const;
    Q_INVOKABLE void jumpToHighlight(const QString &id);
    Q_INVOKABLE QString getHighlightsJsonForCurrentChapter() const;
    Q_INVOKABLE void refreshHighlights();
    Q_INVOKABLE QString exportBookHighlightsToMarkdown(const QString &filePath = QString());
    Q_INVOKABLE QString exportAllHighlightsToMarkdown();
    Q_INVOKABLE QString exportQuoteCardPng(const QString &quoteText, const QString &bookTitle, const QString &author = QString(), const QString &colorName = "yellow");

    QVariantList currentBookHighlights() const;
    QVariantList allHighlights() const;
    int currentBookHighlightCount() const;
    int totalHighlightCount() const;
    QString lastAddedHighlightId() const { return m_lastAddedHighlightId; }

    // Bookmark functions
    Q_INVOKABLE QString addBookmark(int chapter = -1, const QString &chapterTitle = QString(), const QString &snippet = QString(), const QString &note = QString());
    Q_INVOKABLE bool removeBookmark(const QString &id);
    Q_INVOKABLE bool removeBookmarkAtChapter(const QString &filePath, int chapter);
    Q_INVOKABLE bool toggleBookmarkCurrentPosition();
    Q_INVOKABLE bool isBookmark(const QString &filePath, int chapter) const;
    Q_INVOKABLE bool isCurrentChapterBookmarked() const;
    Q_INVOKABLE void jumpToBookmark(const QString &id);
    Q_INVOKABLE void clearBookmarks(const QString &filePath = QString());
    Q_INVOKABLE QVariantMap getBookmark(const QString &id) const;
    Q_INVOKABLE bool updateBookmarkNote(const QString &id, const QString &note);
    Q_INVOKABLE void refreshBookmarks();

    QVariantList currentBookBookmarks() const;
    QVariantList allBookmarks() const;
    int currentBookBookmarkCount() const;
    int totalBookmarkCount() const;
    Q_INVOKABLE QString persistentBookmarksPath() const;

    // Watchdog Timer for rendering & loading fail-safe
    Q_INVOKABLE void startWatchdog(int ms = 1200);
    Q_INVOKABLE void stopWatchdog();

    // Remote Diagnostic & SendLog
    bool isSendingLog() const;
    QString lastLogStatus() const;
    QString devServerIp() const;
    Q_INVOKABLE void setDevServerIp(const QString &ip);
    Q_INVOKABLE void sendLog(const QString &targetIp = QString());
    Q_INVOKABLE bool sendBugReportEmail(const QString &category, const QString &subject, const QString &description, bool includeLog);

    // Persistent Shared Storage
    Q_INVOKABLE QString persistentDataDir() const;
    Q_INVOKABLE QString persistentDataFilePath() const;
    Q_INVOKABLE QString persistentJsonFilePath() const;
    Q_INVOKABLE QString persistentHighlightsPath() const;
    Q_INVOKABLE QString persistentStorageStatus() const;
    QString textAlign() const { return m_textAlign; }
    bool overridePublisherCss() const { return m_overridePublisherCss; }
    Q_INVOKABLE void setTextAlign(const QString &align);
    Q_INVOKABLE void setOverridePublisherCss(bool overrideCss);
    QString paragraphIndent() const { return m_paragraphIndent; }
    QString paragraphSpacing() const { return m_paragraphSpacing; }
    int readerFontWeight() const { return m_readerFontWeight; }
    QString darkTextTone() const { return m_darkTextTone; }
    bool showFooterProgressBar() const { return m_showFooterProgressBar; }
    bool showFooterTimeLeft() const { return m_showFooterTimeLeft; }
    int eyeRestMinutes() const { return m_eyeRestMinutes; }
    bool invertVolumeKeys() const { return m_invertVolumeKeys; }
    Q_INVOKABLE void setParagraphIndent(const QString &indent);
    Q_INVOKABLE void setParagraphSpacing(const QString &spacing);
    Q_INVOKABLE void setReaderFontWeight(int weight);
    Q_INVOKABLE void setDarkTextTone(const QString &tone);
    Q_INVOKABLE void setShowFooterProgressBar(bool show);
    Q_INVOKABLE void setShowFooterTimeLeft(bool show);
    Q_INVOKABLE void setEyeRestMinutes(int minutes);
    Q_INVOKABLE void setInvertVolumeKeys(bool invert);
    bool bionicReading() const { return m_bionicReading; }
    Q_INVOKABLE void setBionicReading(bool enabled);
    Q_INVOKABLE int getBookRating(const QString &filePath) const;
    Q_INVOKABLE void setBookRating(const QString &filePath, int rating);
    Q_INVOKABLE QString getBookReview(const QString &filePath) const;
    Q_INVOKABLE void setBookReview(const QString &filePath, const QString &review);
    Q_INVOKABLE bool updateBookMetadata(const QString &filePath, const QString &title, const QString &author, const QString &series, int seriesIndex);
    Q_INVOKABLE void syncToPersistentStorage();
    Q_INVOKABLE void restoreFromPersistentStorage();
    Q_INVOKABLE QString createBackupArchive();
    Q_INVOKABLE bool restoreBackupArchive(const QString &archivePath);
    Q_INVOKABLE void cancelScan();
    Q_INVOKABLE QStringList scanFolders() const;
    Q_INVOKABLE QString scanFoldersSummary() const;
    Q_INVOKABLE QVariantList listFoldersForPicker(const QString &dirPath) const;
    Q_INVOKABLE QString parentFolderForPicker(const QString &dirPath) const;
    Q_INVOKABLE bool isScanFolderSelected(const QString &folderPath) const;
    Q_INVOKABLE void toggleScanFolder(const QString &folderPath);
    Q_INVOKABLE void clearScanFolders();

    bool updateChecking() const;
    bool updateDownloading() const;
    int updateProgress() const;
    bool updateAvailable() const;
    QString updateLatestVersion() const;
    QString updateReleaseNotes() const;
    QString updateAssetName() const;
    QString updateAssetSizeStr() const;
    QString updateDownloadUrl() const;
    QString updateStatusText() const;
    bool updateReadyToInstall() const;
    QString updateTargetBarPath() const;
    Q_INVOKABLE void checkForAppUpdate();
    Q_INVOKABLE void downloadAppUpdate();
    Q_INVOKABLE void cancelAppUpdateDownload();
    Q_INVOKABLE void finalizeBarInstallAndQuit();
    Q_INVOKABLE void openInternalLink(const QString &href, const QString &refTitle = QString());

public slots:
    void resetLoadingState();
    void onWatchdogTimeout();
    void scanLibrary(bool quiet = false);
    void scanLibraryQuiet();
    void onPdfPageRendered(int pageNumber);
    void onPdfStreamPageRendered(int pageNumber);
    void onPdfIndicatorRequested(bool enabled);
    void onPdfError(int error);
    void onPdfFileOpened(const QString &file);
    void onBookUploadedFromWifi(const QString &filePath, const QString &fileName, qint64 fileSize);

signals:
    void promptExitRequested();
    void modelChanged();
    void bookCountChanged();
    void totalScannedBooksChanged();
    void libraryViewModeChanged();
    void libraryBrowseModeChanged();
    void selectedFolderChanged();
    void folderModelChanged();
    void sortByChanged();
    void groupByChanged();
    void formatFilterChanged();
    void categoryFilterChanged();
    void customShelvesChanged();
    void scanFoldersChanged();
    void searchQueryChanged();
    void languageChanged();

    void currentBookChanged();
    void currentChapterChanged();
    void requestScrollToAnchor(const QString &anchor);
    void showFootnotePopup(const QString &title, const QString &body);
    void fontSizeChanged();
    void fontFamilyChanged();
    void customFontsChanged();
    void themeChanged();
    void warmFilterChanged();
    void readerBrightnessChanged();
    void autoNightThemeChanged();
    void focusModeChanged();
    void volumeKeyTurnEnabledChanged();
    void volumePageNextRequested();
    void volumePagePrevRequested();
    void appThemeChanged();
    void onboardingCompletedChanged();
    void openOnboardingGuideRequested();
    void readingModeChanged();
    void columnLayoutChanged();
    void mangaRtlModeChanged();
    void readingProgressChanged();
    void isReaderActiveChanged();
    void pagePositionChanged();
    void currentPdfImageChanged();
    void pdfInvertedChanged();
    void pdfAutoCropChanged();
    void pdfOpenError(const QString &errorMsg);
    void pdfZoomModeChanged();
    void pdfJumpToPageRequested(int pageNumber);
    void pageRenderTimeout();

    void lastBookChanged();
    void footerSettingsChanged();
    void batteryLevelChanged();
    void currentTimeChanged();
    void lineSpacingChanged();
    void marginsChanged();
    void textAlignChanged();
    void overridePublisherCssChanged();
    void typographyChanged();
    void readingPrefsChanged();
    void eyeRestReminder();

    void mainViewModeChanged();
    void isScanningChanged();
    void isLoadingBookChanged();
    void scanStatusTextChanged();
    void currentBrowserPathChanged();
    void browserItemCountChanged();
    void browserSearchQueryChanged();
    void browserFormatFilterChanged();
    void recentSearchQueryChanged();
    void recentHistoryCountChanged();
    void statsChanged();
    void dailyStatsChanged();
    void tocListChanged();
    void searchResultsChanged();
    void isSearchingChanged();
    void isPdfReflowChanged();
    void hasPdfTextChanged();
    void currentPdfTextChanged();
    void highlightsChanged();
    void bookmarksChanged();
    void ttsAutoAdvanceChanged(bool advance);

    void isSendingLogChanged();
    void lastLogStatusChanged();
    void devServerIpChanged();
    void logSendCompleted(bool success, const QString &message, const QString &url);
    void persistentStorageStatusChanged();
    void updateStateChanged();
    void updateReadyForRestartDialog(const QString &version, const QString &targetPath);
    void updateErrorOccurred(const QString &message);

private slots:
    void onClockTick();
    void onBatteryLevelChanged(int level, bb::device::BatteryChargingState::Type chargingState);
    void onVolumeUpPressed(bb::multimedia::MediaKey::Type key);
    void onVolumeDownPressed(bb::multimedia::MediaKey::Type key);
    void onTtsActiveChangedForVolume(bool active);
    void onReadingHeartbeat();
    void onTtsChapterFinished();
    void onSearchCompleted(const QVariantList &results);
    void onScanCompleted(const QList<QVariantMap> &allBooks);
    void onCoverExtracted(const QString &filePath, const QString &coverPath);
    void onBackgroundCoversFinished();
    void onScanWatchdogTimeout();
    void onProgressSaveTimeout();
    void onPersistentSyncTimeout();
    void onUpdateCheckFinished(bool ok, int httpStatus, const QByteArray &body, const QString &errorString);
    void onUpdateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onUpdateDownloadFinished(bool ok, int httpStatus, const QByteArray &body, const QString &errorString);

private:
    void schedulePersistentSync();
    QTimer m_persistSyncTimer;
    mutable EpubParser m_epubParser;
    mutable TxtParser m_txtParser;
    mutable Fb2Parser m_fb2Parser;
    mutable CbzParser m_cbzParser;
    mutable MobiParser m_mobiParser;
    PdfParser *m_pdfParser;

    AppTranslator *m_translator;
    QString m_language; // "vi" or "en"
    TtsManager *m_tts;
    DictManager m_dictManager;

    QVariantList m_tocList;
    QVariantList m_searchResults;
    QList<QVariantMap> m_allHighlights;
    QString m_lastAddedHighlightId;
    QList<QVariantMap> m_allBookmarks;

    void updateTocList();
    void updatePdfReflowContent();
    void saveHighlights();
    void loadHighlights();
    void saveBookmarks();
    void loadBookmarks();
    int readBookTotalPages(const QString &filePath, const QString &ext);
    QString resolveStatsCoverPath(const QString &existingCover, const QString &filePath, const QString &format) const;
    QVariantMap resolveBookMetaForStats(const QString &rawPath, QSettings &settings) const;

    bb::cascades::GroupDataModel *m_model;
    bb::cascades::GroupDataModel *m_folderModel;
    bb::cascades::GroupDataModel *m_fileBrowserModel;
    QString m_mainViewMode;      // "shelf" or "browser"
    bool m_isScanning;
    bool m_isLoadingBook;
    QString m_scanStatusText;
    QString m_currentBrowserPath;
    QString m_currentBrowserTitle;
    bool m_canGoUp;
    QVariantList m_currentFolderAllItems;
    QString m_browserSearchQuery;
    QString m_browserFormatFilter;
    QString m_recentSearchQuery;
    void applyBrowserFilter();
    QList<QVariantMap> m_allBooks;
    QString m_libraryViewMode;   // "mosaic" or "detailed"
    QString m_libraryBrowseMode; // "folder" or "all"
    QString m_selectedFolderPath;
    QString m_selectedFolderName;
    int m_screenWidth;
    int m_screenHeight;
    QString m_sortBy;          // "date_desc", "date_asc", "title_asc", "title_desc", "author_asc", "size_desc"
    QString m_groupBy;         // "none", "author", "series"
    QString m_formatFilter;    // "all", "epub", "pdf", "txt", "fb2", "djvu"
    QString m_categoryFilter;  // "all", "favorites", "readingList", "reading", "completed", "unread"
    QString m_searchQuery;

    QString m_currentFilePath;
    QString m_currentFormat; // "epub", "txt", "fb2", "pdf", "djvu"
    QString m_currentBookTitle;
    QString m_currentBookAuthor;
    QString m_currentBookSeries;
    int m_currentBookSeriesIndex;
    QString m_currentBookDescription;
    QString m_currentBookGenre;
    QString m_currentBookFileSize;
    QString m_currentBookDateModified;
    QString m_currentCoverPath;
    int m_currentChapterIndex;
    int m_totalChapters;
    int m_fontSize;
    QString m_fontFamily;
    QString m_theme;       // "dark", "sepia", "light"
    QString m_appTheme;    // "dark", "light"
    QString m_readingMode; // "paged", "scroll"
    QString m_columnLayout; // "single", "double"
    bool m_mangaRtlMode;
    int m_currentPageInChapter;
    int m_totalPagesInChapter;
    int m_currentScrollPos;
    mutable bool m_shouldRestorePageOnLoad;
    QString m_cacheDir;

    mutable QString m_cachedContentHtml;
    mutable int m_cachedContentChapter;
    mutable QString m_cachedContentTheme;
    mutable int m_cachedContentFontSize;
    mutable QString m_cachedContentFontFamily;
    mutable QString m_cachedContentSpacing;
    mutable QString m_cachedContentMode;

    bb::cascades::Image m_currentPdfImage;
    QString m_currentPdfPagePath;
    bool m_pdfInverted;
    bool m_isPdfAutoCrop;

    QString m_lastBookPath;
    QString m_lastBookTitle;
    QString m_lastBookAuthor;
    QString m_lastBookCover;
    QString m_lastBookFormat;
    int m_lastBookProgress;

    bool m_showFooterChapter;
    bool m_showFooterProgress;
    bool m_showFooterBattery;
    bool m_showFooterClock;
    QString m_lineSpacing;
    QString m_textAlign;    // "justify", "left"
    bool m_overridePublisherCss;
    int m_marginTop;
    int m_marginBottom;
    int m_marginLeft;
    int m_marginRight;
    bb::device::BatteryInfo m_batteryInfo;
    QTimer m_clockTimer;
    QTimer m_watchdogTimer;
    QTimer m_scanWatchdogTimer;
    QTimer m_readingHeartbeatTimer;
    QTimer m_progressSaveTimer;

    int m_totalReadingSeconds;
    int m_todayReadingSeconds;
    int m_dailyGoalMinutes;
    QString m_todayDate;
    QMap<QString, int> m_bookReadingSeconds;
    QMap<QString, int> m_dailyReadingSeconds;
    QMap<QString, QMap<QString, int> > m_dailyBookReadingSeconds;
    bb::cascades::GroupDataModel *m_readingHistoryModel;
    bb::cascades::ArrayDataModel *m_homeFeedModel;

    bool m_isSearching;
    bool m_isReaderActive;
    bool m_isPdfReflow;
    bool m_hasPdfText;
    bool m_pdfHasAnyOcr;
    QString m_currentPdfText;
    void refreshPdfPageText(int pageNumber);
    QString m_pendingScrollAnchor;
    bool m_ttsAutoAdvance;
    SearchWorker *m_searchWorker;
    ScanWorker *m_scanWorker;
    CoverExtractWorker *m_coverWorker;
    QStringList m_scanFolders;

    TlsHttpWorker *m_updateCheckWorker;
    TlsHttpWorker *m_updateDownloadWorker;
    bool m_updateChecking;
    bool m_updateDownloading;
    int m_updateProgress;
    bool m_updateAvailable;
    QString m_updateLatestVersion;
    QString m_updateReleaseNotes;
    QString m_updateAssetName;
    QString m_updateAssetSizeStr;
    QString m_updateDownloadUrl;
    QString m_updateStatusText;
    bool m_updateReadyToInstall;
    QString m_updateTargetBarPath;
    QString m_updatePartPath;
    QString resolveBarInstallerDir() const;

    void loadSettings();
    void loadSettingsFromQSettings(QSettings &settings);
    void restoreFromPersistentStorageInternal(bool forceOverwrite, bool reloadState);
    void loadReadingStatsFromSettings(QSettings &settings);
    void reconcileLibraryWithSettings(QSettings &settings);
    void applyVisualStyle();
    void loadCurrentChapter();
    void savePdfCoverThumbnail(const QString &filePath, const bb::ImageData &imgData);
    void updateFolderList();
    void applyFilterAndSort();
    void updateReadingHistoryModel();
    void ensureBookInLibrary(const QString &filePath);
    void saveLibraryCache();
    bool loadLibraryCache();
    QString libraryCacheFilePath() const;
    void installBundledSamplesIfNeeded();

    bool m_isSendingLog;
    QString m_lastLogStatus;
    QString m_devServerIp;
    bool m_isAppSleeping;
    bool m_suppressPersistentSync;
    bool m_shutdownSaved;
    bool m_pendingTtsAfterPdfRender;
    int m_pendingJumpPageAfterPdfOpen;
    OpdsManager *m_opdsManager;
    WiFiDropServer *m_wifiDropServer;
    QString m_persistentStorageStatus;

    QStringList m_customShelves;
    QMap<QString, QStringList> m_shelfBooks;
    QVariantList m_customFonts;
    bool m_warmFilterEnabled;
    int m_warmFilterIntensity;
    QMap<QString, int> m_formatBrightness;
    QString normalizedBrightnessFormat() const;
    bool m_autoNightTheme;
    bool m_focusModeEnabled;
    bool m_volumeKeyTurnEnabled;
    bb::multimedia::MediaKeyWatcher *m_volUpWatcher;
    bb::multimedia::MediaKeyWatcher *m_volDownWatcher;
    QString m_paragraphIndent;
    QString m_paragraphSpacing;
    int m_readerFontWeight;
    QString m_darkTextTone;
    bool m_showFooterProgressBar;
    bool m_showFooterTimeLeft;
    int m_eyeRestMinutes;
    bool m_invertVolumeKeys;
    int m_sessionReadingSeconds;
    bool m_bionicReading;
    bool m_onboardingCompleted;
    bb::cascades::ArrayDataModel *m_pdfPagesModel;
    bool m_pdfZoomMode;

    void rebuildPdfPagesModel(int totalPages);
    void resetPdfPagesModelImages();

    void updateVolumeKeyWatchers();
    void loadCustomShelvesFromSettings(QSettings &settings);
    void saveCustomShelvesToSettings(QSettings &settings);
};

#endif // BOOKMANAGER_HPP
