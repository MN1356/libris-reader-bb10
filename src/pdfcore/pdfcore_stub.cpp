#include "pdfcore.h"
#include "pdfsession.h"
#include "pdftocentry.h"
#include "pdfrenderedpage.h"
#include "pdfrenderrequest.h"
#include "pdfabstractdocument.h"
#include "pdfcroppads.h"
#include <bb/ImageData>

// ============================================================================
// NSRCropPads Implementation
// ============================================================================
NSRCropPads::NSRCropPads()
    : _left(0.0)
    , _right(0.0)
    , _top(0.0)
    , _bottom(0.0)
    , _rotation(0)
    , _isDetected(false)
{
}

NSRCropPads::~NSRCropPads()
{
}

void NSRCropPads::setRotation(unsigned int rotation)
{
    _rotation = rotation;
}

void NSRCropPads::rotateRight() {}
void NSRCropPads::rotateLeft() {}
void NSRCropPads::rotate180() {}
void NSRCropPads::mirrorVertical() {}
void NSRCropPads::mirrorHorizontal() {}

// ============================================================================
// NSRTocEntry Implementation
// ============================================================================
NSRTocEntry::NSRTocEntry(const QString& title, int page)
    : _title(title)
    , _page(page)
    , _isExternal(false)
{
}

NSRTocEntry::~NSRTocEntry()
{
    qDeleteAll(_children);
    _children.clear();
}

NSRTocEntry* NSRTocEntry::clone() const
{
    NSRTocEntry *e = new NSRTocEntry(_title, _page);
    e->_externalFile = _externalFile;
    e->_uri = _uri;
    e->_isExternal = _isExternal;
    for (int i = 0; i < _children.size(); ++i) {
        if (_children[i]) {
            e->appendChild(_children[i]->clone());
        }
    }
    return e;
}

QList<const NSRTocEntry*> NSRTocEntry::getChildren() const
{
    QList<const NSRTocEntry*> list;
    for (int i = 0; i < _children.size(); ++i) {
        list.append(_children[i]);
    }
    return list;
}

void NSRTocEntry::appendChild(NSRTocEntry *entry)
{
    if (entry) {
        _children.append(entry);
    }
}

// ============================================================================
// NSRSession Implementation
// ============================================================================
NSRSession::NSRSession()
    : _zoomGraphic(100.0)
    , _rotation(NSRAbstractDocument::NSR_DOCUMENT_ROTATION_0)
    , _page(1)
    , _zoomText(100)
    , _isFitToWidth(false)
{
}

NSRSession::NSRSession(const QString& file,
                       int page,
                       int zoomText,
                       double zoomGraphic,
                       bool isFitToWidth,
                       const QPointF& pos,
                       const QPointF& textPos,
                       NSRAbstractDocument::NSRDocumentRotation rotation)
    : _file(file)
    , _pos(pos)
    , _textPos(textPos)
    , _zoomGraphic(zoomGraphic)
    , _rotation(rotation)
    , _page(page)
    , _zoomText(zoomText)
    , _isFitToWidth(isFitToWidth)
{
}

// ============================================================================
// NSRRenderRequest Implementation
// ============================================================================
NSRRenderRequest::NSRRenderRequest()
    : _reason(NSR_RENDER_REASON_NONE)
    , _type(NSR_RENDER_TYPE_PAGE)
    , _rotation(NSRAbstractDocument::NSR_DOCUMENT_ROTATION_0)
    , _zoom(100.0)
    , _screenWidth(720.0)
    , _number(1)
    , _autoCrop(false)
    , _invertColors(false)
    , _textOnly(false)
    , _zoomToWidth(false)
{
}

NSRRenderRequest::NSRRenderRequest(int number)
    : _reason(NSR_RENDER_REASON_NONE)
    , _type(NSR_RENDER_TYPE_PAGE)
    , _rotation(NSRAbstractDocument::NSR_DOCUMENT_ROTATION_0)
    , _zoom(100.0)
    , _screenWidth(720.0)
    , _number(number)
    , _autoCrop(false)
    , _invertColors(false)
    , _textOnly(false)
    , _zoomToWidth(false)
{
}

NSRRenderRequest::NSRRenderRequest(int number, NSRRenderReason reason)
    : _reason(reason)
    , _type(NSR_RENDER_TYPE_PAGE)
    , _rotation(NSRAbstractDocument::NSR_DOCUMENT_ROTATION_0)
    , _zoom(100.0)
    , _screenWidth(720.0)
    , _number(number)
    , _autoCrop(false)
    , _invertColors(false)
    , _textOnly(false)
    , _zoomToWidth(false)
{
}

NSRRenderRequest::~NSRRenderRequest()
{
}

// ============================================================================
// NSRRenderedPage Implementation
// ============================================================================
NSRRenderedPage::NSRRenderedPage()
    : NSRRenderRequest()
    , _renderedZoom(100.0)
    , _minRenderZoom(100.0)
    , _maxRenderZoom(500.0)
    , _cached(false)
{
}

NSRRenderedPage::NSRRenderedPage(const NSRRenderRequest& req)
    : NSRRenderRequest(req)
    , _renderedZoom(100.0)
    , _minRenderZoom(100.0)
    , _maxRenderZoom(500.0)
    , _cached(false)
{
}

NSRRenderedPage::~NSRRenderedPage()
{
}

// ============================================================================
// Libris PDF/DjVu Core Implementation (Simulator Stub)
// ============================================================================
NSRReaderCore::NSRReaderCore(const INSRSettings *settings,
                             INSRThumbnailer *thumbnailer,
                             QObject *parent)
    : QObject(parent)
    , _settings(settings)
    , _thumbnailer(thumbnailer)
    , _doc(NULL)
    , _zoomDoc(NULL)
    , _preloadDoc(NULL)
    , _thread(NULL)
    , _zoomThread(NULL)
    , _preloadThread(NULL)
    , _cache(NULL)
    , _cropPadsCache(NULL)
    , _pagesLimit(0)
    , _screenWidth(720)
    , _isDestructing(false)
{
}

NSRReaderCore::~NSRReaderCore()
{
}

void NSRReaderCore::prepareForDestruction()
{
    _isDestructing = true;
}

QString NSRReaderCore::getVersion()
{
    return "1.5.3-sim";
}

void NSRReaderCore::loadSession(const NSRSession *session)
{
    if (session) {
        emit sessionFileOpened(session->getFile());
    }
}

void NSRReaderCore::resetSession()
{
}

bool NSRReaderCore::isSessionLoaded() const
{
    return false;
}

QString NSRReaderCore::getSessionFile() const
{
    return QString();
}

void NSRReaderCore::reloadSettings()
{
}

int NSRReaderCore::getPagesCount() const
{
    return 0;
}

NSRRenderedPage NSRReaderCore::getCurrentPage() const
{
    return NSRRenderedPage();
}

void NSRReaderCore::navigateToPage(PageLoad dir, int pageNumber)
{
    Q_UNUSED(dir);
    Q_UNUSED(pageNumber);
}

bool NSRReaderCore::isPageRendering() const
{
    return false;
}

void NSRReaderCore::setScreenWidth(int width)
{
    _screenWidth = width;
}

bool NSRReaderCore::isFitToWidth() const
{
    return true;
}

double NSRReaderCore::getZoom() const
{
    return 100.0;
}

void NSRReaderCore::setZoom(double zoom, NSRRenderRequest::NSRRenderReason reason)
{
    Q_UNUSED(zoom);
    Q_UNUSED(reason);
}

NSRAbstractDocument::NSRDocumentRotation NSRReaderCore::getRotation() const
{
    return NSRAbstractDocument::NSR_DOCUMENT_ROTATION_0;
}

void NSRReaderCore::rotate(RotateDirection dir)
{
    Q_UNUSED(dir);
}

void NSRReaderCore::saveCurrentPagePositions(const QPointF& pos, const QPointF& textPos)
{
    Q_UNUSED(pos);
    Q_UNUSED(textPos);
}

bool NSRReaderCore::isTextReflow() const
{
    return false;
}

bool NSRReaderCore::isTextReflowSwitchSupported() const
{
    return false;
}

void NSRReaderCore::switchTextReflow()
{
}

bool NSRReaderCore::isPasswordProtected(const QString& file) const
{
    Q_UNUSED(file);
    return false;
}

bool NSRReaderCore::isInvertedColors() const
{
    return false;
}

void NSRReaderCore::switchInvertedColors()
{
}

qint64 NSRReaderCore::getCacheSize() const
{
    return 0;
}

void NSRReaderCore::setCacheSize(qint64 size)
{
    Q_UNUSED(size);
}


NSRTocEntry* NSRReaderCore::getToc() const
{
    return NULL;
}

NSRAbstractDocument * NSRReaderCore::fileHandlerByPath(const QString& path, const QString& passwd) const
{
    Q_UNUSED(path);
    Q_UNUSED(passwd);
    return NULL;
}

void NSRReaderCore::onRenderDone() {}
void NSRReaderCore::onZoomRenderDone() {}
void NSRReaderCore::onPreloadRenderDone() {}
void NSRReaderCore::onZoomThreadFinished() {}
void NSRReaderCore::onPreloadThreadFinished() {}
