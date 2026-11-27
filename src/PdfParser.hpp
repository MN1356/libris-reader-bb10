#ifndef PDFPARSER_HPP
#define PDFPARSER_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <bb/ImageData>
#include <bb/cascades/Image>
#include <QtGui/QImage>
#include "BookChapter.hpp"
#include "EpubParser.hpp"
#include "pdfabstractdocument.h"

class PdfParserPrivate;

class PdfParser : public QObject {
    Q_OBJECT
public:
    explicit PdfParser(QObject *parent = 0);
    virtual ~PdfParser();

    bool open(const QString &filePath, int targetPage = 1);
    void close();
    bool isOpen() const;

    QString title() const;
    QString author() const;
    QString series() const;
    int seriesIndex() const;
    QString coverPath() const;
    QString description() const;
    QString genre() const;
    int chapterCount() const;
    QList<BookChapter> chapters() const;
    QList<EpubTocEntry> tocEntries() const;

    QString extractCover(const QString &cacheDir);
    static QImage imageDataToQImage(const bb::ImageData &imgData, bool uninvert = false);

    // Reading & Navigation
    void navigateToPage(int pageNumber);
    void nextPage();
    void prevPage();
    int currentPage() const;

    bb::ImageData currentImageData() const;
    bb::cascades::Image currentImage() const;
    QString currentText() const;
    bool hasText() const;
    QString pageText(int pageNumber);
    bool hasEmbeddedOcr(int probeLimit = 5);
    int findNextTextPage(int startPage, int maxScanPages = 30);

    // Continuous stream rendering
    void requestStreamPage(int pageNumber, int focusPage = -1);
    void setStreamPriorityPage(int focusPage);
    bool hasStreamPage(int pageNumber, int generation = -1) const;
    bb::ImageData streamPageImageData(int pageNumber) const;
    void invalidateStreamCache();
    void storeStreamPage(int generation, int pageNumber, const bb::ImageData &img, const QString &pageText = QString());

    // Display & Layout Controls
    void setScreenWidth(int width);
    void setZoom(double zoom);
    double zoom() const;
    void resetZoom();
    void zoomIn();
    void zoomOut();

    void rotate();
    bool isInvertedColors() const;
    void switchInvertedColors();
    bool isAutoCrop() const;
    void setAutoCrop(bool enabled);
    void switchAutoCrop();
    bool isTextReflow() const;
    void switchTextReflow();
    bool isPasswordProtected(const QString &filePath) const;
    void prepareForDestruction();

Q_SIGNALS:
    void pageRendered(int pageNumber);
    void streamPageRendered(int pageNumber);
    void needIndicator(bool enabled);
    void errorWhileOpening(int errorCode);
    void fileOpened(const QString &filePath);
    void fileClosed(const QString &filePath);

private Q_SLOTS:
    void onCorePageRendered(int number);
    void onStreamPageReady(int generation, int pageNumber);
    void onCoreSessionFileOpened(const QString &file);
    void onCoreSessionFileClosed(const QString &file);
    void onCoreNeedIndicator(bool enabled);
    void onCoreError(NSRAbstractDocument::NSRDocumentError error);

private:
    PdfParserPrivate *d;
    void parseMetadata();
    void parseToc();
    void flattenToc(const void *entryPtr, int level);
};

#endif // PDFPARSER_HPP
