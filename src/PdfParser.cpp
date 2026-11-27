#include "PdfParser.hpp"
#include "pdfcore.h"
#include "pdfsession.h"
#include "pdftocentry.h"
#include "pdfrenderedpage.h"
#include "pdfrenderrequest.h"
#include "pdfabstractdocument.h"
#include "interfaces/ipdfsettings.h"

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QtGui/QImage>
#include <QTextCodec>
#include <QCryptographicHash>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QMap>
#include <QMetaObject>

class LibrisPdfSettings : public INSRSettings {
public:
    bool autoCrop;
    bool invertedColors;
    bool starting;

    LibrisPdfSettings()
        : autoCrop(false)
        , invertedColors(false)
        , starting(false)
    {}

    virtual bool isWordWrap() const { return true; }
    virtual bool isInvertedColors() const { return invertedColors; }
    virtual bool isAutoCrop() const { return autoCrop; }
    virtual bool isEncodingAutodetection() const { return true; }
    virtual QString getTextEncoding() const { return QString::fromLatin1("UTF-8"); }
    virtual bool isStarting() const { return starting; }
};

class PdfStreamWorker : public QThread {
public:
    explicit PdfStreamWorker(PdfParser *parser)
        : QThread(parser)
        , m_parser(parser)
        , m_doc(NULL)
        , m_pendingDoc(NULL)
        , m_hasPendingDoc(false)
        , m_stop(false)
        , m_generation(0)
        , m_pageWidth(720)
        , m_inverted(false)
        , m_autoCrop(false)
        , m_rotation(NSRAbstractDocument::NSR_DOCUMENT_ROTATION_0)
    {}

    virtual ~PdfStreamWorker() {
        stopWorker();
    }

    void setDocument(NSRAbstractDocument *doc, int gen, int pageWidth, bool inverted, bool autoCrop,
                     NSRAbstractDocument::NSRDocumentRotation rot) {
        QMutexLocker lock(&m_mutex);
        if (m_hasPendingDoc && m_pendingDoc && m_pendingDoc != doc) {
            delete m_pendingDoc;
        }
        m_pendingDoc = doc;
        m_hasPendingDoc = true;
        m_generation = gen;
        m_pageWidth = qMax(240, pageWidth);
        m_inverted = inverted;
        m_autoCrop = autoCrop;
        m_rotation = rot;
        m_queue.clear();
        m_cond.wakeOne();
    }

    void updateSettings(int gen, int pageWidth, bool inverted, bool autoCrop,
                        NSRAbstractDocument::NSRDocumentRotation rot) {
        QMutexLocker lock(&m_mutex);
        m_generation = gen;
        m_pageWidth = qMax(240, pageWidth);
        m_inverted = inverted;
        m_autoCrop = autoCrop;
        m_rotation = rot;
        m_queue.clear();
        m_cond.wakeOne();
    }

    void enqueuePage(int pageNumber, int gen, bool highPriority) {
        if (pageNumber < 1) return;
        QMutexLocker lock(&m_mutex);
        if (gen != m_generation || m_stop) return;
        if (!m_doc && !m_hasPendingDoc) return;
        int idx = m_queue.indexOf(pageNumber);
        if (idx >= 0) {
            if (!highPriority || idx == 0) return;
            m_queue.removeAt(idx);
        }
        if (highPriority) {
            m_queue.prepend(pageNumber);
        } else {
            m_queue.append(pageNumber);
        }
        m_cond.wakeOne();
    }

    void stopWorker() {
        {
            QMutexLocker lock(&m_mutex);
            m_stop = true;
            m_queue.clear();
            m_cond.wakeAll();
        }
        if (isRunning()) {
            wait(3000);
        }
        if (m_pendingDoc) {
            delete m_pendingDoc;
            m_pendingDoc = NULL;
            m_hasPendingDoc = false;
        }
        if (m_doc) {
            delete m_doc;
            m_doc = NULL;
        }
    }

protected:
    virtual void run() {
        for (;;) {
            int pageNumber = 0;
            int gen = 0;
            int pageWidth = 720;
            bool inverted = false;
            bool autoCrop = false;
            NSRAbstractDocument::NSRDocumentRotation rot = NSRAbstractDocument::NSR_DOCUMENT_ROTATION_0;

            {
                QMutexLocker lock(&m_mutex);
                while (!m_stop && !m_hasPendingDoc && m_queue.isEmpty()) {
                    m_cond.wait(&m_mutex);
                }
                if (m_hasPendingDoc) {
                    NSRAbstractDocument *oldDoc = m_doc;
                    m_doc = m_pendingDoc;
                    m_pendingDoc = NULL;
                    m_hasPendingDoc = false;
                    lock.unlock();
                    if (oldDoc) delete oldDoc;
                    lock.relock();
                }
                if (m_stop) {
                    break;
                }
                if (m_queue.isEmpty() || !m_doc) {
                    continue;
                }
                pageNumber = m_queue.takeFirst();
                gen = m_generation;
                pageWidth = m_pageWidth;
                inverted = m_inverted;
                autoCrop = m_autoCrop;
                rot = m_rotation;
            }

            if (!m_doc || pageNumber < 1 || pageNumber > m_doc->getPageCount()) {
                continue;
            }

            m_doc->setPageWidth(pageWidth);
            m_doc->setZoomToWidth(true);
            m_doc->setInvertedColors(inverted);
            m_doc->setAutoCrop(autoCrop);
            m_doc->setRotation(rot);
            m_doc->setTextOnly(false);

            NSRRenderInfo info = m_doc->renderPage(pageNumber);
            bb::ImageData img;
            if (info.isSuccessRender()) {
                img = m_doc->getCurrentPage();
            }

            bool validGen = false;
            {
                QMutexLocker lock(&m_mutex);
                validGen = (!m_stop && !m_hasPendingDoc && gen == m_generation);
            }
            if (validGen && img.width() > 0 && img.height() > 0 && m_parser) {
                m_parser->storeStreamPage(gen, pageNumber, img);
                QMetaObject::invokeMethod(m_parser, "onStreamPageReady", Qt::QueuedConnection,
                                          Q_ARG(int, gen), Q_ARG(int, pageNumber));
            }
        }
        if (m_doc) {
            delete m_doc;
            m_doc = NULL;
        }
    }

private:
    PdfParser *m_parser;
    QMutex m_mutex;
    QWaitCondition m_cond;
    NSRAbstractDocument *m_doc;
    NSRAbstractDocument *m_pendingDoc;
    bool m_hasPendingDoc;
    bool m_stop;
    int m_generation;
    int m_pageWidth;
    bool m_inverted;
    bool m_autoCrop;
    NSRAbstractDocument::NSRDocumentRotation m_rotation;
    QList<int> m_queue;
};

struct PdfParserPrivate {
    QString filePath;
    QString title;
    QString author;
    QString series;
    int seriesIndex;
    QString coverPath;
    QString description;
    QString genre;
    int currentPage;
    int pageCount;
    QList<BookChapter> chapters;
    QList<EpubTocEntry> tocEntries;

    LibrisPdfSettings *pdfSettings;
    LibrisPdfCore *core;
    LibrisPdfSession *session;
    int screenWidth;

    PdfStreamWorker *streamWorker;
    mutable QMutex streamMutex;
    QMap<int, bb::ImageData> streamCache;
    QList<int> streamLru;
    int streamGeneration;

    PdfParserPrivate()
        : seriesIndex(0)
        , currentPage(1)
        , pageCount(0)
        , pdfSettings(NULL)
        , core(NULL)
        , session(NULL)
        , screenWidth(720)
        , streamWorker(NULL)
        , streamGeneration(0)
    {
    }
};

PdfParser::PdfParser(QObject *parent)
    : QObject(parent)
    , d(new PdfParserPrivate())
{
    d->pdfSettings = new LibrisPdfSettings();
    d->session = new LibrisPdfSession();
    d->core = new LibrisPdfCore(d->pdfSettings, NULL, this);
    d->streamWorker = new PdfStreamWorker(this);
    d->streamWorker->start();

    connect(d->core, SIGNAL(pageRendered(int)), this, SLOT(onCorePageRendered(int)));
    connect(d->core, SIGNAL(sessionFileOpened(QString)), this, SLOT(onCoreSessionFileOpened(QString)));
    connect(d->core, SIGNAL(sessionFileClosed(QString)), this, SLOT(onCoreSessionFileClosed(QString)));
    connect(d->core, SIGNAL(needIndicator(bool)), this, SLOT(onCoreNeedIndicator(bool)));
    connect(d->core, SIGNAL(errorWhileOpening(NSRAbstractDocument::NSRDocumentError)),
            this, SLOT(onCoreError(NSRAbstractDocument::NSRDocumentError)));
}

PdfParser::~PdfParser()
{
    prepareForDestruction();
    if (d) {
        if (d->streamWorker) {
            d->streamWorker->stopWorker();
            delete d->streamWorker;
            d->streamWorker = NULL;
        }
        if (d->core) {
            delete d->core;
            d->core = NULL;
        }
        if (d->pdfSettings) {
            delete d->pdfSettings;
            d->pdfSettings = NULL;
        }
        if (d->session) {
            delete d->session;
            d->session = NULL;
        }
        delete d;
        d = NULL;
    }
}

void PdfParser::prepareForDestruction()
{
    if (d) {
        if (d->streamWorker) {
            d->streamWorker->stopWorker();
        }
        if (d->core) {
            d->core->prepareForDestruction();
        }
    }
}

bool PdfParser::open(const QString &filePath, int targetPage)
{
    close();

    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile() || !d->session || !d->core) {
        return false;
    }

    d->filePath = filePath;
    d->title = fi.completeBaseName();
    d->author = "Tài liệu " + fi.suffix().toUpper();
    d->currentPage = (targetPage >= 1) ? targetPage : 1;

    parseMetadata();

    d->session->setFile(filePath);
    d->session->setPage(d->currentPage);
    d->session->setFitToWidth(true);
    d->session->setZoomGraphic(0.0);

    d->core->setScreenWidth(d->screenWidth);
    d->core->loadSession(d->session);
    d->core->setZoom(0.0, LibrisPdfRenderRequest::NSR_RENDER_REASON_ZOOM_TO_WIDTH);

    {
        QMutexLocker lock(&d->streamMutex);
        d->streamGeneration++;
        d->streamCache.clear();
        d->streamLru.clear();
    }
    if (d->streamWorker) {
        NSRAbstractDocument *streamDoc = d->core->fileHandlerByPath(filePath);
        d->streamWorker->setDocument(streamDoc, d->streamGeneration, d->screenWidth,
                                     d->pdfSettings ? d->pdfSettings->invertedColors : false,
                                     d->pdfSettings ? d->pdfSettings->autoCrop : false,
                                     NSRAbstractDocument::NSR_DOCUMENT_ROTATION_0);
    }

    return true;
}

void PdfParser::close()
{
    if (!d) return;
    {
        QMutexLocker lock(&d->streamMutex);
        d->streamGeneration++;
        d->streamCache.clear();
        d->streamLru.clear();
    }
    if (d->streamWorker) {
        d->streamWorker->setDocument(NULL, d->streamGeneration, d->screenWidth, false, false,
                                     NSRAbstractDocument::NSR_DOCUMENT_ROTATION_0);
    }
    if (d->core && d->core->isSessionLoaded()) {
        d->core->resetSession();
    }
    d->filePath.clear();
    d->title.clear();
    d->author.clear();
    d->series.clear();
    d->seriesIndex = 0;
    d->coverPath.clear();
    d->description.clear();
    d->genre.clear();
    d->currentPage = 1;
    d->pageCount = 0;
    d->chapters.clear();
    d->tocEntries.clear();
}

bool PdfParser::isOpen() const
{
    return d && d->core && d->core->isSessionLoaded();
}

QString PdfParser::title() const { return d ? d->title : QString(); }
QString PdfParser::author() const { return d ? d->author : QString(); }
QString PdfParser::series() const { return d ? d->series : QString(); }
int PdfParser::seriesIndex() const { return d ? d->seriesIndex : 0; }
QString PdfParser::coverPath() const { return d ? d->coverPath : QString(); }
QString PdfParser::description() const { return d ? d->description : QString(); }
QString PdfParser::genre() const { return d ? d->genre : QString(); }
int PdfParser::currentPage() const { return d ? d->currentPage : 1; }
QList<BookChapter> PdfParser::chapters() const { return d ? d->chapters : QList<BookChapter>(); }
QList<EpubTocEntry> PdfParser::tocEntries() const { return d ? d->tocEntries : QList<EpubTocEntry>(); }

int PdfParser::chapterCount() const
{
    if (d && d->core && d->core->isSessionLoaded()) {
        int cnt = d->core->getPagesCount();
        if (cnt > 0) return cnt;
    }
    return d ? d->pageCount : 0;
}

void PdfParser::setScreenWidth(int width)
{
    if (d) {
        d->screenWidth = width;
        if (d->core) {
            d->core->setScreenWidth(width);
        }
        invalidateStreamCache();
    }
}

void PdfParser::navigateToPage(int pageNumber)
{
    if (d && d->core && d->core->isSessionLoaded()) {
        d->currentPage = pageNumber;
        d->core->navigateToPage(LibrisPdfCore::PAGE_LOAD_CUSTOM, pageNumber);
    }
}

void PdfParser::nextPage()
{
    if (d && d->core && d->core->isSessionLoaded()) {
        d->core->navigateToPage(LibrisPdfCore::PAGE_LOAD_NEXT);
    }
}

void PdfParser::prevPage()
{
    if (d && d->core && d->core->isSessionLoaded()) {
        d->core->navigateToPage(LibrisPdfCore::PAGE_LOAD_PREV);
    }
}

bb::ImageData PdfParser::currentImageData() const
{
    if (d && d->core && d->core->isSessionLoaded()) {
        return d->core->getCurrentPage().getImage();
    }
    return bb::ImageData();
}

bb::cascades::Image PdfParser::currentImage() const
{
    bb::ImageData data = currentImageData();
    if (data.width() > 0 && data.height() > 0) {
        return bb::cascades::Image(data);
    }
    return bb::cascades::Image();
}

QString PdfParser::currentText() const
{
    if (d && d->core && d->core->isSessionLoaded()) {
        return d->core->getCurrentPage().getText();
    }
    return QString();
}

bool PdfParser::hasText() const
{
    return !currentText().trimmed().isEmpty();
}

void PdfParser::rotate()
{
    if (d && d->core && d->core->isSessionLoaded()) {
        d->core->rotate(LibrisPdfCore::ROTATE_DIRECTION_RIGHT);
        invalidateStreamCache();
    }
}

void PdfParser::setZoom(double zoom)
{
    if (d && d->core && d->core->isSessionLoaded()) {
        d->core->setZoom(zoom, LibrisPdfRenderRequest::NSR_RENDER_REASON_ZOOM);
    }
}

double PdfParser::zoom() const
{
    if (d && d->core && d->core->isSessionLoaded()) {
        return d->core->getZoom();
    }
    return 100.0;
}

void PdfParser::resetZoom()
{
    if (d && d->core && d->core->isSessionLoaded()) {
        d->core->setZoom(0.0, LibrisPdfRenderRequest::NSR_RENDER_REASON_ZOOM_TO_WIDTH);
    }
}

void PdfParser::zoomIn()
{
    if (d && d->core && d->core->isSessionLoaded()) {
        double current = d->core->getZoom();
        d->core->setZoom(current * 1.25, LibrisPdfRenderRequest::NSR_RENDER_REASON_ZOOM);
    }
}

void PdfParser::zoomOut()
{
    if (d && d->core && d->core->isSessionLoaded()) {
        double current = d->core->getZoom();
        d->core->setZoom(current / 1.25, LibrisPdfRenderRequest::NSR_RENDER_REASON_ZOOM);
    }
}

bool PdfParser::isInvertedColors() const
{
    if (d && d->core && d->core->isSessionLoaded()) {
        return d->core->isInvertedColors();
    }
    return false;
}

void PdfParser::switchInvertedColors()
{
    if (d && d->core && d->core->isSessionLoaded()) {
        d->core->switchInvertedColors();
        invalidateStreamCache();
    }
}

bool PdfParser::isAutoCrop() const
{
    return d && d->pdfSettings ? d->pdfSettings->autoCrop : false;
}

void PdfParser::setAutoCrop(bool enabled)
{
    if (d && d->pdfSettings) {
        if (d->pdfSettings->autoCrop == enabled) {
            return;
        }
        d->pdfSettings->autoCrop = enabled;
        if (d->core && d->core->isSessionLoaded()) {
            d->core->reloadSettings();
        }
        invalidateStreamCache();
    }
}

void PdfParser::switchAutoCrop()
{
    setAutoCrop(!isAutoCrop());
}

bool PdfParser::isTextReflow() const
{
    if (d && d->core && d->core->isSessionLoaded()) {
        return d->core->isTextReflow();
    }
    return false;
}

void PdfParser::switchTextReflow()
{
    if (d && d->core && d->core->isSessionLoaded()) {
        d->core->switchTextReflow();
    }
}

bool PdfParser::isPasswordProtected(const QString &filePath) const
{
    if (d && d->core) {
        return d->core->isPasswordProtected(filePath);
    }
    return false;
}

QImage PdfParser::imageDataToQImage(const bb::ImageData &imgData, bool uninvert)
{
    int w = imgData.width();
    int h = imgData.height();
    const unsigned char *pixels = (const unsigned char *)imgData.pixels();
    int stride = imgData.bytesPerLine();
    if (w <= 0 || h <= 0 || !pixels || stride <= 0) {
        return QImage();
    }

    bool isGray = (stride < w * 4);
    QImage qimg(w, h, QImage::Format_RGB32);
    for (int y = 0; y < h; ++y) {
        const unsigned char *srcRow = pixels + y * stride;
        QRgb *dstRow = reinterpret_cast<QRgb *>(qimg.scanLine(y));
        if (isGray) {
            for (int x = 0; x < w; ++x) {
                int v = srcRow[x];
                if (uninvert) v = 255 - v;
                dstRow[x] = qRgb(v, v, v);
            }
        } else {
            for (int x = 0; x < w; ++x) {
                int r = srcRow[x * 4];
                int g = srcRow[x * 4 + 1];
                int b = srcRow[x * 4 + 2];
                if (uninvert) {
                    r = 255 - r;
                    g = 255 - g;
                    b = 255 - b;
                }
                dstRow[x] = qRgb(r, g, b);
            }
        }
    }
    return qimg;
}

QString PdfParser::extractCover(const QString &cacheDir)
{
    if (!d || d->filePath.isEmpty()) return QString();

    QDir().mkpath(cacheDir);
    QString hash = QString(QCryptographicHash::hash(d->filePath.toUtf8(), QCryptographicHash::Md5).toHex());
    QString outPath = cacheDir + "/" + hash + ".png";

    if (QFile::exists(outPath)) {
        d->coverPath = outPath;
        return outPath;
    }

    bb::ImageData imgData = currentImageData();
    QImage qimg = imageDataToQImage(imgData, isInvertedColors());
    if (!qimg.isNull()) {
        QImage thumbnail = qimg.scaled(240, 320, Qt::KeepAspectRatio, Qt::FastTransformation);
        if (thumbnail.save(outPath, "PNG")) {
            d->coverPath = outPath;
            return outPath;
        }
    }

    return QString();
}

void PdfParser::onCorePageRendered(int number)
{
    int gen = 0;
    if (d) {
        d->currentPage = number;
        if (d->core) {
            d->pageCount = d->core->getPagesCount();
        }
        {
            QMutexLocker lock(&d->streamMutex);
            gen = d->streamGeneration;
        }
        bb::ImageData imgData = currentImageData();
        if (imgData.width() > 0 && imgData.height() > 0) {
            storeStreamPage(gen, number, imgData);
            emit streamPageRendered(number);
        }
    }
    emit pageRendered(number);
}

void PdfParser::requestStreamPage(int pageNumber, bool highPriority)
{
    if (!d || !d->streamWorker || pageNumber < 1) return;
    int gen = 0;
    {
        QMutexLocker lock(&d->streamMutex);
        if (d->streamCache.contains(pageNumber)) {
            emit streamPageRendered(pageNumber);
            return;
        }
        gen = d->streamGeneration;
    }
    d->streamWorker->enqueuePage(pageNumber, gen, highPriority);
}

bb::ImageData PdfParser::streamPageImageData(int pageNumber) const
{
    if (!d) return bb::ImageData();
    QMutexLocker lock(&d->streamMutex);
    return d->streamCache.value(pageNumber);
}

void PdfParser::invalidateStreamCache()
{
    if (!d) return;
    int gen = 0;
    {
        QMutexLocker lock(&d->streamMutex);
        d->streamGeneration++;
        d->streamCache.clear();
        d->streamLru.clear();
        gen = d->streamGeneration;
    }
    if (d->streamWorker && d->core) {
        bool inv = d->core->isInvertedColors();
        bool ac = d->pdfSettings ? d->pdfSettings->autoCrop : false;
        NSRAbstractDocument::NSRDocumentRotation rot = d->core->getRotation();
        d->streamWorker->updateSettings(gen, d->screenWidth, inv, ac, rot);
    }
}

void PdfParser::storeStreamPage(int generation, int pageNumber, const bb::ImageData &img)
{
    if (!d || img.width() <= 0 || img.height() <= 0) return;
    QMutexLocker lock(&d->streamMutex);
    if (generation != d->streamGeneration) return;
    d->streamCache.insert(pageNumber, img);
    d->streamLru.removeAll(pageNumber);
    d->streamLru.append(pageNumber);
    while (d->streamLru.size() > 5) {
        int evictPage = d->streamLru.takeFirst();
        d->streamCache.remove(evictPage);
    }
}

void PdfParser::onStreamPageReady(int generation, int pageNumber)
{
    if (!d) return;
    bool valid = false;
    {
        QMutexLocker lock(&d->streamMutex);
        valid = (generation == d->streamGeneration && d->streamCache.contains(pageNumber));
    }
    if (valid) {
        emit streamPageRendered(pageNumber);
    }
}

void PdfParser::onCoreSessionFileOpened(const QString &file)
{
    if (d && d->core) {
        d->pageCount = d->core->getPagesCount();
        parseToc();
    }
    emit fileOpened(file);
}

void PdfParser::onCoreSessionFileClosed(const QString &file)
{
    emit fileClosed(file);
}

void PdfParser::onCoreNeedIndicator(bool enabled)
{
    emit needIndicator(enabled);
}

void PdfParser::onCoreError(NSRAbstractDocument::NSRDocumentError error)
{
    emit errorWhileOpening((int)error);
}

void PdfParser::parseMetadata()
{
    if (!d || !d->filePath.endsWith(".pdf", Qt::CaseInsensitive)) {
        return;
    }

    QFile file(d->filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    qint64 size = file.size();
    if (size <= 0) return;

    QByteArray data = file.read(qMin(size, (qint64)8192));
    if (size > 8192) {
        file.seek(qMax((qint64)0, size - 8192));
        data.append(file.read(8192));
    }
    file.close();

    struct PdfStrHelper {
        static QString decode(const QByteArray &raw) {
            if (raw.size() >= 2 && (unsigned char)raw[0] == 0xFE && (unsigned char)raw[1] == 0xFF) {
                QTextCodec *u16 = QTextCodec::codecForName("UTF-16BE");
                if (u16) return u16->toUnicode(raw.constData() + 2, raw.size() - 2).trimmed();
            }
            return QString::fromUtf8(raw).trimmed();
        }
    };

    // Fast search for /Title (...)
    int titleIdx = data.indexOf("/Title");
    if (titleIdx >= 0) {
        int openParen = data.indexOf('(', titleIdx);
        int closeParen = (openParen >= 0) ? data.indexOf(')', openParen) : -1;
        if (openParen >= 0 && (openParen - titleIdx) <= 12 && closeParen > openParen && (closeParen - openParen) < 256) {
            QString extracted = PdfStrHelper::decode(data.mid(openParen + 1, closeParen - openParen - 1));
            if (!extracted.isEmpty()) {
                d->title = extracted;
            }
        }
    }

    // Fast search for /Author (...)
    int authorIdx = data.indexOf("/Author");
    if (authorIdx >= 0) {
        int openParen = data.indexOf('(', authorIdx);
        int closeParen = (openParen >= 0) ? data.indexOf(')', openParen) : -1;
        if (openParen >= 0 && (openParen - authorIdx) <= 12 && closeParen > openParen && (closeParen - openParen) < 256) {
            QString extracted = PdfStrHelper::decode(data.mid(openParen + 1, closeParen - openParen - 1));
            if (!extracted.isEmpty()) {
                d->author = extracted;
            }
        }
    }
}

void PdfParser::parseToc()
{
    if (!d) return;
    d->tocEntries.clear();
    d->chapters.clear();

    if (d->core && d->core->isSessionLoaded()) {
        LibrisPdfTocEntry *root = d->core->getToc();
        if (root) {
            QList<const LibrisPdfTocEntry *> children = root->getChildren();
            for (int i = 0; i < children.size(); ++i) {
                flattenToc(children.at(i), 0);
            }
            delete root;
        }
    }

    if (d->tocEntries.isEmpty()) {
        int total = chapterCount();
        for (int i = 1; i <= total; ++i) {
            EpubTocEntry item;
            item.title = QString("Trang %1").arg(i);
            item.chapterIndex = i;
            item.anchor = QString();
            item.level = 0;
            item.href = QString::number(i);
            d->tocEntries.append(item);

            BookChapter ch;
            ch.id = QString::number(i);
            ch.href = QString::number(i);
            ch.title = item.title;
            d->chapters.append(ch);
        }
    }
}

void PdfParser::flattenToc(const void *entryPtr, int level)
{
    if (!d || !entryPtr) return;
    const LibrisPdfTocEntry *entry = static_cast<const LibrisPdfTocEntry *>(entryPtr);

    QString title = entry->getTitle().trimmed();
    int page = entry->getPage();
    if (!title.isEmpty() && page > 0) {
        EpubTocEntry item;
        item.title = title;
        item.chapterIndex = page;
        item.anchor = QString();
        item.level = level;
        item.href = QString::number(page);
        d->tocEntries.append(item);

        BookChapter ch;
        ch.id = QString::number(page);
        ch.href = QString::number(page);
        ch.title = title;
        d->chapters.append(ch);
    }

    QList<const LibrisPdfTocEntry *> children = entry->getChildren();
    for (int i = 0; i < children.size(); ++i) {
        flattenToc(children.at(i), level + 1);
    }
}
