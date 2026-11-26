#include "MobiParser.hpp"
#include "RegexUtil.hpp"
#include "ImageUriUtil.hpp"
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QTextCodec>
#include <QRegExp>
#include <QDebug>

static inline quint16 readBE16(const unsigned char *p)
{
    return ((quint16)p[0] << 8) | (quint16)p[1];
}

static inline quint32 readBE32(const unsigned char *p)
{
    return ((quint32)p[0] << 24) | ((quint32)p[1] << 16) | ((quint32)p[2] << 8) | (quint32)p[3];
}

MobiParser::MobiParser()
    : m_seriesIndex(0)
    , m_coverRecordIndex(-1)
    , m_firstImageIndex(0)
    , m_isKf8(false)
    , m_kf8BoundaryIndex(-1)
    , m_compression(0)
    , m_textEncoding(65001)
    , m_extraFlags(0)
    , m_huffRecordIndex(0)
    , m_huffRecordCount(0)
    , m_huffLoaded(false)
{
}

MobiParser::~MobiParser()
{
    close();
}

void MobiParser::close()
{
    if (m_file.isOpen()) {
        m_file.close();
    }
    m_filePath.clear();
    m_title.clear();
    m_author.clear();
    m_series.clear();
    m_seriesIndex = 0;
    m_description.clear();
    m_genre.clear();
    m_coverPath.clear();
    m_coverRecordIndex = -1;
    m_firstImageIndex = 0;
    m_isKf8 = false;
    m_kf8BoundaryIndex = -1;
    m_css.clear();

    m_chapters.clear();
    m_tocEntries.clear();
    m_chapterHtmls.clear();
    m_resolvedChapterCache.clear();
    m_cacheLru.clear();

    m_recordOffsets.clear();
    m_compression = 0;
    m_textEncoding = 65001;
    m_extraFlags = 0;
    m_huffRecordIndex = 0;
    m_huffRecordCount = 0;
    m_huffLoaded = false;
    m_dict1.clear();
    m_dictionary.clear();
    m_dictFlag.clear();
}

QByteArray MobiParser::readRecord(int index)
{
    if (!m_file.isOpen() || index < 0 || index >= m_recordOffsets.size()) {
        return QByteArray();
    }

    quint32 offset = m_recordOffsets[index];
    quint32 nextOffset = (index + 1 < m_recordOffsets.size()) ? m_recordOffsets[index + 1] : (quint32)m_file.size();

    if (nextOffset < offset || offset >= (quint32)m_file.size()) {
        return QByteArray();
    }

    qint64 len = nextOffset - offset;
    if (len <= 0 || len > 100 * 1024 * 1024) { // 100MB sanity limit
        return QByteArray();
    }

    if (!m_file.seek(offset)) {
        return QByteArray();
    }

    return m_file.read(len);
}

bool MobiParser::open(const QString &filePath)
{
    close();

    m_filePath = filePath;
    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::ReadOnly)) {
        qWarning() << "[MobiParser] Failed to open file:" << filePath;
        return false;
    }

    if (m_file.size() < 78) {
        qWarning() << "[MobiParser] File too small for PDB header:" << filePath;
        close();
        return false;
    }

    QByteArray pdbHdr = m_file.read(78);
    if (pdbHdr.size() < 78) {
        close();
        return false;
    }

    const unsigned char *pHdr = (const unsigned char*)pdbHdr.constData();
    QByteArray dbType = pdbHdr.mid(60, 4);
    QByteArray dbCreator = pdbHdr.mid(64, 4);

    if (dbType != "BOOK" && dbType != "TEXt" && dbCreator != "MOBI") {
        // Not a standard PalmDOC / MOBI file
        qWarning() << "[MobiParser] Unrecognized PDB type/creator:" << dbType << dbCreator;
        close();
        return false;
    }

    quint16 numRecords = readBE16(pHdr + 76);
    if (numRecords == 0) {
        qWarning() << "[MobiParser] Zero records in PDB:" << filePath;
        close();
        return false;
    }

    // Read record list: numRecords * 8 bytes
    QByteArray recList = m_file.read(numRecords * 8);
    if (recList.size() < numRecords * 8) {
        qWarning() << "[MobiParser] Truncated record list:" << filePath;
        close();
        return false;
    }

    m_recordOffsets.resize(numRecords);
    const unsigned char *pRecList = (const unsigned char*)recList.constData();
    for (int i = 0; i < numRecords; ++i) {
        m_recordOffsets[i] = readBE32(pRecList + i * 8);
    }

    // Parse Record 0
    QByteArray rec0 = readRecord(0);
    if (rec0.size() < 16) {
        qWarning() << "[MobiParser] Invalid record 0 size:" << rec0.size();
        close();
        return false;
    }

    const unsigned char *pRec0 = (const unsigned char*)rec0.constData();
    m_compression = readBE16(pRec0 + 0);
    quint32 textLength = readBE32(pRec0 + 4);
    quint16 textRecordCount = readBE16(pRec0 + 8);

    bool isMobi = (rec0.size() >= 32 && rec0.mid(16, 4) == "MOBI");
    if (isMobi) {
        quint32 mobiHeaderLen = readBE32(pRec0 + 20);
        quint32 mobiVersion = (rec0.size() >= 40) ? readBE32(pRec0 + 36) : 0;
        if (mobiVersion >= 8) {
            m_isKf8 = true;
        }
        m_textEncoding = readBE32(pRec0 + 28);
        quint32 fullNameOffset = (rec0.size() >= 88) ? readBE32(pRec0 + 84) : 0;
        quint32 fullNameLength = (rec0.size() >= 92) ? readBE32(pRec0 + 88) : 0;

        if (rec0.size() >= 112) {
            m_firstImageIndex = readBE32(pRec0 + 108);
        }
        if (rec0.size() >= 116) {
            m_huffRecordIndex = readBE32(pRec0 + 112);
        }
        if (rec0.size() >= 120) {
            m_huffRecordCount = readBE32(pRec0 + 116);
        }

        quint32 exthFlags = (rec0.size() >= 132) ? readBE32(pRec0 + 128) : 0;

        if (mobiHeaderLen >= 228 && rec0.size() >= 244) {
            m_extraFlags = readBE16(pRec0 + 242);
        }

        if (fullNameOffset > 0 && fullNameLength > 0 && (fullNameOffset + fullNameLength) <= (quint32)rec0.size()) {
            QByteArray nameBytes = rec0.mid(fullNameOffset, fullNameLength);
            m_title = (m_textEncoding == 65001) ? QString::fromUtf8(nameBytes) : QString::fromLatin1(nameBytes);
            m_title = m_title.trimmed();
        }

        if (exthFlags & 0x40) {
            quint32 exthOffset = 16 + mobiHeaderLen;
            if (exthOffset + 12 <= (quint32)rec0.size() && rec0.mid(exthOffset, 4) == "EXTH") {
                parseExth(rec0, exthOffset);
            }
        }

        // Check for KF8 / AZW3 (pure KF8 or dual-format combo)
        int kf8HeaderRecIdx = -1;
        if (m_isKf8) {
            kf8HeaderRecIdx = 0;
        } else if (m_kf8BoundaryIndex > 0 && m_kf8BoundaryIndex < m_recordOffsets.size()) {
            QByteArray bRec = readRecord(m_kf8BoundaryIndex);
            if (bRec.startsWith("BOUNDARY")) {
                kf8HeaderRecIdx = m_kf8BoundaryIndex + 1;
            } else if (bRec.size() >= 32 && bRec.mid(16, 4) == "MOBI") {
                kf8HeaderRecIdx = m_kf8BoundaryIndex;
            }
        }

        if (kf8HeaderRecIdx >= 0 && kf8HeaderRecIdx < m_recordOffsets.size()) {
            if (parseKf8Section(kf8HeaderRecIdx)) {
                return true;
            }
        }
    }

    if (m_title.isEmpty()) {
        m_title = QFileInfo(filePath).completeBaseName().replace('_', ' ');
    }
    if (m_author.isEmpty()) {
        m_author = "Sách điện tử MOBI";
    }
    if (m_genre.isEmpty()) {
        m_genre = "eBook";
    }

    // Load HUFF/CDIC tables if needed
    if (m_compression == 17480) { // 0x4448 "DH"
        if (!loadHuffTables()) {
            qWarning() << "[MobiParser] Failed to load HUFF/CDIC tables for:" << filePath;
            close();
            return false;
        }
    }

    // Decompress text records (records 1 to textRecordCount)
    int maxTextRecords = qMin((int)textRecordCount, numRecords - 1);
    QByteArray fullData;
    fullData.reserve(textLength > 0 ? textLength : (maxTextRecords * 4096));

    for (int r = 1; r <= maxTextRecords; ++r) {
        QByteArray rec = readRecord(r);
        if (rec.isEmpty()) continue;

        QByteArray decomp = decompressRecord(rec);
        fullData.append(decomp);
    }

    if (fullData.isEmpty()) {
        qWarning() << "[MobiParser] No text data extracted from MOBI:" << filePath;
        close();
        return false;
    }

    QString fullHtml;
    if (m_textEncoding == 65001) {
        fullHtml = QString::fromUtf8(fullData.constData(), fullData.size());
        if (fullHtml.isEmpty() && !fullData.isEmpty()) {
            fullHtml = QString::fromLatin1(fullData.constData(), fullData.size());
        }
    } else {
        QTextCodec *codec = QTextCodec::codecForName("Windows-1252");
        if (codec) {
            fullHtml = codec->toUnicode(fullData);
        } else {
            fullHtml = QString::fromLatin1(fullData.constData(), fullData.size());
        }
    }

    parseHtmlIntoChapters(fullHtml);

    if (m_chapters.isEmpty()) {
        qWarning() << "[MobiParser] No chapters generated for:" << filePath;
        close();
        return false;
    }

    return true;
}

void MobiParser::parseExth(const QByteArray &rec0, quint32 exthOffset)
{
    const unsigned char *pRec0 = (const unsigned char*)rec0.constData();
    quint32 exthLen = readBE32(pRec0 + exthOffset + 4);
    quint32 exthCount = readBE32(pRec0 + exthOffset + 8);

    quint32 pos = exthOffset + 12;
    quint32 end = qMin(exthOffset + exthLen, (quint32)rec0.size());

    for (quint32 i = 0; i < exthCount && pos + 8 <= end; ++i) {
        quint32 recType = readBE32(pRec0 + pos);
        quint32 recLen = readBE32(pRec0 + pos + 4);

        if (recLen < 8 || pos + recLen > end) break;

        QByteArray data = rec0.mid(pos + 8, recLen - 8);
        pos += recLen;

        if (recType == 100 && m_author.isEmpty()) { // Author
            m_author = (m_textEncoding == 65001) ? QString::fromUtf8(data) : QString::fromLatin1(data);
            m_author = m_author.trimmed();
        } else if (recType == 129 && m_author.isEmpty()) { // Author
            m_author = (m_textEncoding == 65001) ? QString::fromUtf8(data) : QString::fromLatin1(data);
            m_author = m_author.trimmed();
        } else if ((recType == 103 || recType == 125) && m_description.isEmpty()) { // Description
            m_description = (m_textEncoding == 65001) ? QString::fromUtf8(data) : QString::fromLatin1(data);
            m_description = m_description.trimmed();
        } else if (recType == 105 && m_genre.isEmpty()) { // Subject / Genre
            m_genre = (m_textEncoding == 65001) ? QString::fromUtf8(data) : QString::fromLatin1(data);
            m_genre = m_genre.trimmed();
        } else if (recType == 128 || recType == 503) { // Updated Title
            QString t = (m_textEncoding == 65001) ? QString::fromUtf8(data) : QString::fromLatin1(data);
            t = t.trimmed();
            if (!t.isEmpty()) m_title = t;
        } else if (recType == 121 && data.size() >= 4) { // KF8 Boundary record index in dual MOBI
            m_kf8BoundaryIndex = (int)readBE32((const unsigned char*)data.constData());
        } else if ((recType == 201 || recType == 202) && data.size() >= 4 && m_coverRecordIndex < 0) { // Cover offset relative to firstImageIndex
            quint32 coverOffset = readBE32((const unsigned char*)data.constData());
            m_coverRecordIndex = (int)(m_firstImageIndex + coverOffset);
        }
    }
}

void MobiParser::parseFdstRecord(int fdstRecIdx, const QByteArray &fullData, QByteArray &xhtmlData, QString &cssData)
{
    QByteArray fdstRec = readRecord(fdstRecIdx);
    if (fdstRec.size() < 12 || !fdstRec.startsWith("FDST")) {
        xhtmlData = fullData;
        return;
    }

    const unsigned char *pFdst = (const unsigned char*)fdstRec.constData();
    quint32 numSections = readBE32(pFdst + 8);
    if (numSections == 0 || (12 + numSections * 8) > (quint32)fdstRec.size()) {
        xhtmlData = fullData;
        return;
    }

    quint32 f0Start = readBE32(pFdst + 12);
    quint32 f0End = readBE32(pFdst + 16);
    if (f0End <= (quint32)fullData.size() && f0End > f0Start) {
        xhtmlData = fullData.mid(f0Start, f0End - f0Start);
    } else {
        xhtmlData = fullData;
    }

    // Extract CSS stylesheets from Flow 1 and subsequent flows
    QString combinedCss;
    for (quint32 s = 1; s < numSections; ++s) {
        quint32 sStart = readBE32(pFdst + 12 + s * 8);
        quint32 sEnd = readBE32(pFdst + 12 + s * 8 + 4);
        if (sEnd <= (quint32)fullData.size() && sEnd > sStart) {
            QByteArray sData = fullData.mid(sStart, sEnd - sStart);
            QString cssStr = QString::fromUtf8(sData.constData(), sData.size());
            if (!cssStr.isEmpty()) {
                combinedCss += cssStr + "\n";
            }
        }
    }
    cssData = combinedCss.trimmed();
}

bool MobiParser::parseKf8Section(int kf8HeaderRecIdx)
{
    QByteArray recHdr = readRecord(kf8HeaderRecIdx);
    if (recHdr.size() < 32 || recHdr.mid(16, 4) != "MOBI") {
        return false;
    }

    const unsigned char *pRec = (const unsigned char*)recHdr.constData();
    quint16 compression = readBE16(pRec + 0);
    quint32 textLength = readBE32(pRec + 4);
    quint16 textRecordCount = readBE16(pRec + 8);
    quint32 mobiHeaderLen = readBE32(pRec + 20);
    quint32 mobiVersion = (recHdr.size() >= 40) ? readBE32(pRec + 36) : 0;
    quint32 textEncoding = (recHdr.size() >= 32) ? readBE32(pRec + 28) : 65001;

    if (mobiVersion < 8 && kf8HeaderRecIdx > 0) {
        return false;
    }

    m_isKf8 = true;
    m_compression = compression;
    m_textEncoding = textEncoding;

    quint32 firstImage = (recHdr.size() >= 112) ? readBE32(pRec + 108) : 0;
    if (firstImage > 0 && firstImage != 0xFFFFFFFFu) {
        if (firstImage < (quint32)kf8HeaderRecIdx && (kf8HeaderRecIdx + firstImage) < (quint32)m_recordOffsets.size()) {
            firstImage += kf8HeaderRecIdx;
        }
        m_firstImageIndex = firstImage;
    }

    quint32 huffIdx = (recHdr.size() >= 116) ? readBE32(pRec + 112) : 0;
    if (huffIdx > 0 && huffIdx != 0xFFFFFFFFu) {
        if (huffIdx < (quint32)kf8HeaderRecIdx && (kf8HeaderRecIdx + huffIdx) < (quint32)m_recordOffsets.size()) {
            huffIdx += kf8HeaderRecIdx;
        }
        m_huffRecordIndex = huffIdx;
    }
    m_huffRecordCount = (recHdr.size() >= 120) ? readBE32(pRec + 116) : 0;

    m_extraFlags = 0;
    if (mobiHeaderLen >= 228 && recHdr.size() >= 244) {
        m_extraFlags = readBE16(pRec + 242);
    }

    // Parse EXTH in KF8 header
    quint32 exthFlags = (recHdr.size() >= 132) ? readBE32(pRec + 128) : 0;
    if (exthFlags & 0x40) {
        quint32 exthOffset = 16 + mobiHeaderLen;
        if (exthOffset + 12 <= (quint32)recHdr.size() && recHdr.mid(exthOffset, 4) == "EXTH") {
            parseExth(recHdr, exthOffset);
        }
    }

    if (m_title.isEmpty()) {
        m_title = QFileInfo(m_filePath).completeBaseName().replace('_', ' ');
    }
    if (m_author.isEmpty()) {
        m_author = "Sách Kindle KF8 / AZW3";
    }
    if (m_genre.isEmpty()) {
        m_genre = "Kindle eBook";
    }

    // Load HUFF/CDIC tables if needed
    if (m_compression == 17480) {
        if (!loadHuffTables()) {
            qWarning() << "[MobiParser] Failed to load KF8 HUFF/CDIC tables for:" << m_filePath;
            return false;
        }
    }

    // Decompress KF8 text records
    int startRec = kf8HeaderRecIdx + 1;
    int endRec = qMin((int)(kf8HeaderRecIdx + textRecordCount), m_recordOffsets.size() - 1);
    QByteArray fullData;
    fullData.reserve(textLength > 0 ? textLength : (textRecordCount * 4096));

    for (int r = startRec; r <= endRec; ++r) {
        QByteArray rec = readRecord(r);
        if (rec.isEmpty()) continue;
        QByteArray decomp = decompressRecord(rec);
        fullData.append(decomp);
    }

    if (fullData.isEmpty()) {
        qWarning() << "[MobiParser] Empty KF8 rawML data for:" << m_filePath;
        return false;
    }

    // Locate FDST record
    int fdstIdx = -1;
    int searchLimit = (m_firstImageIndex > 0 && m_firstImageIndex < (quint32)m_recordOffsets.size())
                      ? (int)m_firstImageIndex : m_recordOffsets.size();
    for (int r = endRec + 1; r < searchLimit; ++r) {
        QByteArray rData = readRecord(r);
        if (rData.startsWith("FDST")) {
            fdstIdx = r;
            break;
        }
    }

    QByteArray xhtmlData;
    m_css.clear();

    if (fdstIdx != -1) {
        parseFdstRecord(fdstIdx, fullData, xhtmlData, m_css);
    } else {
        xhtmlData = fullData;
    }

    QString fullHtml;
    if (m_textEncoding == 65001) {
        fullHtml = QString::fromUtf8(xhtmlData.constData(), xhtmlData.size());
        if (fullHtml.isEmpty() && !xhtmlData.isEmpty()) {
            fullHtml = QString::fromLatin1(xhtmlData.constData(), xhtmlData.size());
        }
    } else {
        QTextCodec *codec = QTextCodec::codecForName("Windows-1252");
        fullHtml = codec ? codec->toUnicode(xhtmlData) : QString::fromLatin1(xhtmlData.constData(), xhtmlData.size());
    }

    // Clean up empty skeleton placeholders so text fragments flow directly inside body
    fullHtml.remove(QRegExp("<div[^>]*aid=[\"'][^\"']*[\"'][^>]*>\\s*</div>", Qt::CaseInsensitive));
    fullHtml.remove(QRegExp("<div[^>]*aid=[\"'][^\"']*[\"'][^>]*/>", Qt::CaseInsensitive));
    fullHtml.remove(QRegExp("<p[^>]*aid=[\"'][^\"']*[\"'][^>]*>\\s*</p>", Qt::CaseInsensitive));

    parseHtmlIntoChapters(fullHtml);
    return !m_chapters.isEmpty();
}

QByteArray MobiParser::decompressPalmDoc(const unsigned char *src, int len)
{
    QByteArray out;
    out.reserve(4096);
    int i = 0;
    while (i < len) {
        unsigned char c = src[i++];
        if (c >= 1 && c <= 8) {
            while (c-- > 0 && i < len) {
                out.append((char)src[i++]);
            }
        } else if (c <= 0x7F) {
            out.append((char)c);
        } else if (c >= 0xC0) {
            out.append(' ');
            out.append((char)(c ^ 0x80));
        } else {
            if (i >= len) break;
            unsigned char c2 = src[i++];
            int distance = (((c << 8) | c2) >> 3) & 0x07FF;
            int length = (c2 & 0x07) + 3;
            int start = out.size() - distance;
            if (distance > 0 && start >= 0) {
                for (int k = 0; k < length; ++k) {
                    out.append(out.at(start + k));
                }
            }
        }
    }
    return out;
}

static inline quint64 readBE64(const unsigned char *p)
{
    return ((quint64)readBE32(p) << 32) | (quint64)readBE32(p + 4);
}

bool MobiParser::loadHuffTables()
{
    m_huffLoaded = false;
    m_dict1.clear();
    m_dictionary.clear();
    m_dictFlag.clear();

    if (m_huffRecordCount < 2 || m_huffRecordIndex >= (quint32)m_recordOffsets.size()) {
        return false;
    }

    QByteArray huffRec = readRecord(m_huffRecordIndex);
    if (huffRec.size() < 24 || huffRec.mid(0, 4) != "HUFF") {
        return false;
    }

    const unsigned char *pHuff = (const unsigned char*)huffRec.constData();
    quint32 off1 = readBE32(pHuff + 8);
    quint32 off2 = readBE32(pHuff + 12);

    if ((quint64)off1 + 256 * 4 > (quint64)huffRec.size() || (quint64)off2 + 64 * 4 > (quint64)huffRec.size()) {
        return false;
    }

    m_dict1.resize(256);
    for (int i = 0; i < 256; ++i) {
        quint32 v = readBE32(pHuff + off1 + i * 4);
        HuffEntry e;
        e.codelen = (quint8)(v & 0x1F);
        e.term = (v & 0x80) != 0;
        quint64 maxcode = v >> 8;
        if (e.codelen == 0) {
            return false;
        }
        e.maxcode = ((maxcode + 1) << (32 - e.codelen)) - 1;
        m_dict1[i] = e;
    }

    m_mincode[0] = 0;
    m_maxcode[0] = (((quint64)1) << 32) - 1;
    for (int cl = 1; cl <= 32; ++cl) {
        quint32 mn = readBE32(pHuff + off2 + (cl - 1) * 8);
        quint32 mx = readBE32(pHuff + off2 + (cl - 1) * 8 + 4);
        m_mincode[cl] = ((quint64)mn) << (32 - cl);
        m_maxcode[cl] = (((quint64)mx + 1) << (32 - cl)) - 1;
    }

    // Read CDIC records
    for (quint32 c = 1; c < m_huffRecordCount; ++c) {
        QByteArray cdicRec = readRecord(m_huffRecordIndex + c);
        if (cdicRec.size() < 16 || cdicRec.mid(0, 4) != "CDIC") {
            continue;
        }

        const unsigned char *pCdic = (const unsigned char*)cdicRec.constData();
        quint32 phrases = readBE32(pCdic + 8);
        quint32 bits = readBE32(pCdic + 12);
        if (bits > 16) continue;
        quint32 remaining = phrases > (quint32)m_dictionary.size() ? phrases - (quint32)m_dictionary.size() : 0;
        quint32 n = qMin((quint32)(1u << bits), remaining);

        for (quint32 i = 0; i < n; ++i) {
            if (16 + (i + 1) * 2 > (quint32)cdicRec.size()) break;
            quint32 off = readBE16(pCdic + 16 + i * 2);
            if (16 + off + 2 > (quint32)cdicRec.size()) {
                m_dictionary.append(QByteArray());
                m_dictFlag.append(1);
                continue;
            }
            quint16 blen = readBE16(pCdic + 16 + off);
            int slen = blen & 0x7FFF;
            m_dictionary.append(cdicRec.mid(18 + off, slen));
            m_dictFlag.append((blen & 0x8000) ? 1 : 0);
        }
    }

    m_huffLoaded = !m_dictionary.isEmpty();
    return m_huffLoaded;
}

QByteArray MobiParser::huffUnpack(const unsigned char *src, int len, int depth)
{
    QByteArray out;
    if (depth > 32 || len <= 0 || m_dict1.size() != 256) return out;

    QByteArray buf((const char*)src, len);
    buf.append(QByteArray(8, '\0'));
    const unsigned char *d = (const unsigned char*)buf.constData();

    qint64 bitsLeft = (qint64)len * 8;
    int pos = 0;
    quint64 x = readBE64(d + pos);
    int n = 32;

    while (true) {
        if (n <= 0) {
            pos += 4;
            if (pos + 8 > buf.size()) break;
            x = readBE64(d + pos);
            n += 32;
        }
        quint32 code = (quint32)((x >> n) & 0xFFFFFFFFULL);
        const HuffEntry &e = m_dict1[code >> 24];
        int codelen = e.codelen;
        quint64 maxcode = e.maxcode;
        if (!e.term) {
            while (codelen < 32 && (quint64)code < m_mincode[codelen]) {
                ++codelen;
            }
            maxcode = m_maxcode[codelen];
        }
        if (codelen <= 0) break;
        n -= codelen;
        bitsLeft -= codelen;
        if (bitsLeft <= 0) break;

        quint64 diff = maxcode >= (quint64)code ? (maxcode - code) : 0;
        qint64 r = (qint64)(diff >> (32 - codelen));
        if (r < 0 || r >= m_dictionary.size()) break;

        if (!m_dictFlag[(int)r]) {
            m_dictFlag[(int)r] = 1; // guard against self-referencing phrases
            QByteArray raw = m_dictionary[(int)r];
            m_dictionary[(int)r] = huffUnpack((const unsigned char*)raw.constData(), raw.size(), depth + 1);
        }
        out.append(m_dictionary[(int)r]);
    }
    return out;
}

QByteArray MobiParser::decompressHuff(const unsigned char *src, int len)
{
    if (!m_huffLoaded || len <= 0) return QByteArray();
    return huffUnpack(src, len, 0);
}

QByteArray MobiParser::decompressRecord(const QByteArray &rec)
{
    int len = rec.size();
    if (len <= 0) return QByteArray();

    // Strip trailing entries according to extraFlags (lowest bit first).
    // Each entry ends with a backward variable-length integer whose value
    // already includes the bytes of the integer itself.
    if (m_extraFlags > 1) {
        for (int bit = 1; bit <= 15; ++bit) {
            if ((m_extraFlags >> bit) & 1) {
                int v = 0;
                int bitpos = 0;
                int pos = len;
                while (pos > 0) {
                    unsigned char b = (unsigned char)rec.at(--pos);
                    v |= (b & 0x7F) << bitpos;
                    bitpos += 7;
                    if ((b & 0x80) || bitpos >= 28) break;
                }
                len -= v;
                if (len <= 0) return QByteArray();
            }
        }
    }
    if (m_extraFlags & 1) {
        if (len > 0) {
            int mb = ((unsigned char)rec.at(len - 1) & 3) + 1;
            len -= mb;
        }
    }
    if (len <= 0) return QByteArray();

    const unsigned char *src = (const unsigned char*)rec.constData();

    if (m_compression == 1) { // No compression
        return rec.left(len);
    } else if (m_compression == 2) { // PalmDOC LZ77
        return decompressPalmDoc(src, len);
    } else if (m_compression == 17480) { // HUFF / CDIC
        return decompressHuff(src, len);
    }

    return rec.left(len);
}

QString MobiParser::extractChapterHeading(const QString &htmlChunk, int chapterIndex)
{
    QRegExp h1Rx = minimalRx("<h1[^>]*>([\\s\\S]*)</h1>", Qt::CaseInsensitive);
    if (h1Rx.indexIn(htmlChunk) != -1) {
        QString h1 = h1Rx.cap(1);
        h1.remove(QRegExp("<[^>]*>")).replace("&nbsp;", " ").trimmed();
        if (!h1.isEmpty() && h1.length() < 120) return h1;
    }

    QRegExp h2Rx = minimalRx("<h2[^>]*>([\\s\\S]*)</h2>", Qt::CaseInsensitive);
    if (h2Rx.indexIn(htmlChunk) != -1) {
        QString h2 = h2Rx.cap(1);
        h2.remove(QRegExp("<[^>]*>")).replace("&nbsp;", " ").trimmed();
        if (!h2.isEmpty() && h2.length() < 120) return h2;
    }

    QRegExp h3Rx = minimalRx("<h3[^>]*>([\\s\\S]*)</h3>", Qt::CaseInsensitive);
    if (h3Rx.indexIn(htmlChunk) != -1) {
        QString h3 = h3Rx.cap(1);
        h3.remove(QRegExp("<[^>]*>")).replace("&nbsp;", " ").trimmed();
        if (!h3.isEmpty() && h3.length() < 120) return h3;
    }

    if (chapterIndex == 0 && htmlChunk.contains("<img", Qt::CaseInsensitive) && htmlChunk.length() < 3000) {
        return "Bìa sách";
    }
    return QString("Chương %1").arg(chapterIndex + 1);
}

void MobiParser::parseHtmlIntoChapters(const QString &fullHtml)
{
    QStringList rawSections;

    // Pattern 1: Pagebreaks (<mbp:pagebreak/>, <div ... page-break-before...>)
    QRegExp pbRx("<(?:mbp:pagebreak|div[^>]*style=[\"'][^\"']*page-break-before:[^\"']*[\"']|p[^>]*style=[\"'][^\"']*page-break-before:[^\"']*[\"'])[^>]*>", Qt::CaseInsensitive);

    int pos = 0;
    int lastPos = 0;
    while ((pos = pbRx.indexIn(fullHtml, lastPos)) != -1) {
        QString chunk = fullHtml.mid(lastPos, pos - lastPos).trimmed();
        if (!chunk.isEmpty()) {
            rawSections.append(chunk);
        }
        lastPos = pos + pbRx.matchedLength();
    }
    if (lastPos < fullHtml.length()) {
        QString remaining = fullHtml.mid(lastPos).trimmed();
        if (!remaining.isEmpty()) {
            rawSections.append(remaining);
        }
    }

    // Pattern 2: Fallback to <h1> or <h2> splits if fewer than 2 pagebreaks
    if (rawSections.size() < 2) {
        rawSections.clear();
        QRegExp hRx("<h[12][^>]*>", Qt::CaseInsensitive);
        lastPos = 0;
        while ((pos = hRx.indexIn(fullHtml, lastPos + 1)) != -1) {
            QString chunk = fullHtml.mid(lastPos, pos - lastPos).trimmed();
            if (!chunk.isEmpty()) {
                rawSections.append(chunk);
            }
            lastPos = pos;
        }
        if (lastPos < fullHtml.length()) {
            QString remaining = fullHtml.mid(lastPos).trimmed();
            if (!remaining.isEmpty()) {
                rawSections.append(remaining);
            }
        }
    }

    // Pattern 3: Fallback chunking at paragraph boundaries for large monolithic text
    if (rawSections.size() < 2 && fullHtml.length() > 40000) {
        rawSections.clear();
        const int targetChunkSize = 35000;
        int curStart = 0;
        int totalLen = fullHtml.length();

        while (curStart < totalLen) {
            int targetEnd = curStart + targetChunkSize;
            if (targetEnd >= totalLen) {
                rawSections.append(fullHtml.mid(curStart).trimmed());
                break;
            }

            int pEnd = fullHtml.indexOf("</p>", targetEnd, Qt::CaseInsensitive);
            if (pEnd != -1 && pEnd - curStart < targetChunkSize * 1.5) {
                int splitPoint = pEnd + 4;
                rawSections.append(fullHtml.mid(curStart, splitPoint - curStart).trimmed());
                curStart = splitPoint;
            } else {
                int divEnd = fullHtml.indexOf("</div>", targetEnd, Qt::CaseInsensitive);
                if (divEnd != -1 && divEnd - curStart < targetChunkSize * 1.5) {
                    int splitPoint = divEnd + 6;
                    rawSections.append(fullHtml.mid(curStart, splitPoint - curStart).trimmed());
                    curStart = splitPoint;
                } else {
                    rawSections.append(fullHtml.mid(curStart, targetChunkSize).trimmed());
                    curStart += targetChunkSize;
                }
            }
        }
    }

    if (rawSections.isEmpty()) {
        rawSections.append(fullHtml);
    }

    // If MOBI/AZW3 has a cover record in EXTH/header and first HTML section has no image cover,
    // prepend a dedicated cover chapter so the cover displays inside the reader.
    int coverIdx = m_coverRecordIndex;
    if (coverIdx < 0 && m_firstImageIndex > 0 && m_firstImageIndex != 0xFFFFFFFFu) {
        coverIdx = (int)m_firstImageIndex;
    }
    bool firstHasCoverImg = !rawSections.isEmpty() && rawSections.first().contains("<img", Qt::CaseInsensitive) && rawSections.first().length() < 3000;
    if (!firstHasCoverImg && coverIdx >= 0 && coverIdx < m_recordOffsets.size() && !readRecord(coverIdx).isEmpty()) {
        BookChapter covCh;
        covCh.id = "mobi_cover";
        covCh.href = "mobi_cover";
        covCh.title = "Bìa sách";
        m_chapters.append(covCh);
        m_chapterHtmls.append(QString());

        EpubTocEntry covToc;
        covToc.title = "Bìa sách";
        covToc.chapterIndex = 0;
        covToc.anchor = QString();
        covToc.level = 0;
        covToc.href = covCh.href;
        m_tocEntries.append(covToc);
    }

    int baseIdx = m_chapters.size();
    bool hasCoverChapter = (!m_chapters.isEmpty() && m_chapters[0].id == "mobi_cover");
    for (int i = 0; i < rawSections.size(); ++i) {
        m_chapterHtmls.append(rawSections[i]);
        int chapNum = baseIdx + i;
        BookChapter ch;
        ch.id = QString("mobi_ch_%1").arg(chapNum);
        ch.href = ch.id;
        ch.title = extractChapterHeading(rawSections[i], hasCoverChapter ? i : (i + 1));
        if (!hasCoverChapter && i == 0 && rawSections[i].contains("<img", Qt::CaseInsensitive) && rawSections[i].length() < 3000) {
            ch.title = "Bìa sách";
        }
        m_chapters.append(ch);

        EpubTocEntry toc;
        toc.title = ch.title;
        toc.chapterIndex = chapNum;
        toc.anchor = QString();
        toc.level = 0;
        toc.href = ch.href;
        m_tocEntries.append(toc);
    }
}

QString MobiParser::getOrCacheImage(int recNum, const QString &cacheDir)
{
    if (m_firstImageIndex <= 0 || m_firstImageIndex == 0xFFFFFFFFu || recNum <= 0) return QString();
    int imageRecIdx = (int)m_firstImageIndex + recNum - 1;
    if (imageRecIdx < 0 || imageRecIdx >= m_recordOffsets.size()) return QString();

    QByteArray imgData = readRecord(imageRecIdx);
    if (imgData.isEmpty()) return QString();

    if (imgData.size() <= 4 * 1024 * 1024) {
        return bytesToDataUri(imgData);
    }

    bool isPng = imgData.startsWith("\x89PNG");
    QString ext = isPng ? "png" : "jpg";
    if (imgData.startsWith("GIF")) ext = "gif";
    else if (imgData.startsWith("BM")) ext = "bmp";

    // Determine cache target directory (prefer user-configured cache dir, fallback to temp)
    QString targetDir = cacheDir;
    if (targetDir.isEmpty()) {
        targetDir = QDir::tempPath() + "/libris_mobi";
    } else {
        targetDir = cacheDir + "/mobi";
    }

    QString hash = QCryptographicHash::hash(m_filePath.toUtf8(), QCryptographicHash::Md5).toHex().left(10);
    QString imgPath = QString("%1/%2_%3.%4").arg(targetDir, hash).arg(recNum).arg(ext);

    if (QDir().mkpath(targetDir)) {
        if (!QFile::exists(imgPath) || QFileInfo(imgPath).size() == 0) {
            QFile outFile(imgPath);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(imgData);
                outFile.close();
            }
        }
        if (QFile::exists(imgPath) && QFileInfo(imgPath).size() > 0) {
            return QString("file://%1").arg(imgPath);
        }
    }

    // Secondary disk cache: system temp folder
    QString fallbackDir = QDir::tempPath() + "/libris_mobi";
    if (QDir().mkpath(fallbackDir)) {
        QString fallbackPath = QString("%1/%2_%3.%4").arg(fallbackDir, hash).arg(recNum).arg(ext);
        if (!QFile::exists(fallbackPath) || QFileInfo(fallbackPath).size() == 0) {
            QFile outFile(fallbackPath);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(imgData);
                outFile.close();
            }
        }
        if (QFile::exists(fallbackPath) && QFileInfo(fallbackPath).size() > 0) {
            return QString("file://%1").arg(fallbackPath);
        }
    }

    return QString();
}

QString MobiParser::resolveEmbeddedImages(const QString &html, const QString &cacheDir)
{
    QString result = html;
    QHash<int, QString> resolvedImages;

    // Pattern 1: recindex=["']?([0-9]+)["']?
    QRegExp recRx("(?:recindex|image-index)\\s*=\\s*[\"']?([0-9]+)[\"']?", Qt::CaseInsensitive);
    int pos = 0;
    while ((pos = recRx.indexIn(result, pos)) != -1) {
        int recNum = recRx.cap(1).toInt();
        if (!resolvedImages.contains(recNum)) {
            QString uri = getOrCacheImage(recNum, cacheDir);
            if (!uri.isEmpty()) {
                resolvedImages.insert(recNum, uri);
            }
        }

        if (resolvedImages.contains(recNum)) {
            QString replacement = QString("src=\"%1\"").arg(resolvedImages.value(recNum));
            int lt = result.lastIndexOf('<', pos);
            int gt = result.indexOf('>', pos);
            if (lt != -1 && gt != -1 && pos < gt) {
                QString tagSlice = result.mid(lt, gt - lt + 1);
                QRegExp oldSrcRx("\\bsrc\\s*=\\s*(?:\"[^\"]*\"|'[^']*')", Qt::CaseInsensitive);
                int oldPos = oldSrcRx.indexIn(tagSlice);
                if (oldPos != -1 && (lt + oldPos) < pos) {
                    int rmLen = oldSrcRx.matchedLength();
                    result.remove(lt + oldPos, rmLen);
                    pos -= rmLen;
                }
            }
            result.replace(pos, recRx.matchedLength(), replacement);
            pos += replacement.length();
        } else {
            pos += recRx.matchedLength();
        }
    }

    // Pattern 2: (?:src|xlink:href)=["']kindle:(?:embed|flow):([0-9]+)[^"']*["']
    // Image ids in kindle:embed:XXXX are base-32 numbers (digits 0-9 then A-V), 1-based.
    QRegExp kindleRx("(src|xlink:href)\\s*=\\s*[\"']kindle:embed:([0-9A-Va-v]+)[^\"']*[\"']", Qt::CaseInsensitive);
    pos = 0;
    while ((pos = kindleRx.indexIn(result, pos)) != -1) {
        QString attrName = kindleRx.cap(1);
        bool b32ok = false;
        int recNum = kindleRx.cap(2).toInt(&b32ok, 32);
        if (!b32ok) recNum = 0;
        if (!resolvedImages.contains(recNum)) {
            QString uri = getOrCacheImage(recNum, cacheDir);
            if (!uri.isEmpty()) {
                resolvedImages.insert(recNum, uri);
            }
        }

        if (resolvedImages.contains(recNum)) {
            QString replacement = QString("%1=\"%2\"").arg(attrName, resolvedImages.value(recNum));
            result.replace(pos, kindleRx.matchedLength(), replacement);
            pos += replacement.length();
        } else {
            pos += kindleRx.matchedLength();
        }
    }

    return result;
}

QString MobiParser::extractCover(const QString &cacheDir)
{
    if (!m_coverPath.isEmpty() && QFile::exists(m_coverPath)) {
        return m_coverPath;
    }

    QDir dir(cacheDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString hash = QString(QCryptographicHash::hash(m_filePath.toUtf8(), QCryptographicHash::Md5).toHex());
    QString outPathJpg = cacheDir + "/" + hash + ".jpg";
    QString outPathPng = cacheDir + "/" + hash + ".png";

    if (QFile::exists(outPathJpg) && QFileInfo(outPathJpg).size() > 0) {
        m_coverPath = outPathJpg;
        return m_coverPath;
    }
    if (QFile::exists(outPathPng) && QFileInfo(outPathPng).size() > 0) {
        m_coverPath = outPathPng;
        return m_coverPath;
    }

    int coverIdx = m_coverRecordIndex;
    if (coverIdx < 0 && m_firstImageIndex > 0 && m_firstImageIndex < (quint32)m_recordOffsets.size()) {
        coverIdx = m_firstImageIndex;
    }

    if (coverIdx >= 0 && coverIdx < m_recordOffsets.size()) {
        QByteArray imgData = readRecord(coverIdx);
        if (!imgData.isEmpty()) {
            bool isPng = imgData.startsWith("\x89PNG");
            QString targetPath = isPng ? outPathPng : outPathJpg;
            QFile f(targetPath);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(imgData);
                f.close();
                m_coverPath = targetPath;
                return m_coverPath;
            }
        }
    }

    return QString();
}

QString MobiParser::getChapterContent(int index, const QString &cacheDir)
{
    if (index < 0 || index >= m_chapterHtmls.size()) {
        return QString();
    }

    if (index < m_chapters.size() && m_chapters[index].id == "mobi_cover") {
        QString coverSrc;
        int coverIdx = m_coverRecordIndex;
        if (coverIdx < 0 && m_firstImageIndex > 0 && m_firstImageIndex != 0xFFFFFFFFu) {
            coverIdx = (int)m_firstImageIndex;
        }
        if (coverIdx >= 0 && coverIdx < m_recordOffsets.size()) {
            QByteArray imgData = readRecord(coverIdx);
            if (!imgData.isEmpty() && imgData.size() <= 8 * 1024 * 1024) {
                coverSrc = bytesToDataUri(imgData);
            }
        }
        if (coverSrc.isEmpty()) {
            QString localCov = extractCover(cacheDir);
            coverSrc = fileToDataUri(localCov);
            if (coverSrc.isEmpty() && !localCov.isEmpty() && QFile::exists(localCov)) {
                coverSrc = QString("file://%1").arg(localCov);
            }
        }
        QString html;
        html += "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/>";
        html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"/>";
        html += "<style>";
        html += "html, body { width: 100%; height: 100%; margin: 0; padding: 0; overflow: hidden; }";
        html += ".cover-container { width: 100%; height: 100%; display: -webkit-box; -webkit-box-pack: center; -webkit-box-align: center; display: flex; justify-content: center; align-items: center; box-sizing: border-box; padding: 16px; text-align: center; }";
        html += ".cover-img { max-width: 90vw !important; max-height: 82vh !important; width: auto !important; height: auto !important; object-fit: contain; border-radius: 6px; box-shadow: 0 4px 18px rgba(0,0,0,0.6); display: block; margin: auto; }";
        html += "</style></head><body><div class=\"cover-container\">";
        if (!coverSrc.isEmpty()) {
            html += QString("<img class=\"cover-img\" src=\"%1\" alt=\"Cover\"/>").arg(coverSrc);
        }
        html += "</div></body></html>";
        return html;
    }

    if (m_resolvedChapterCache.contains(index)) {
        m_cacheLru.removeAll(index);
        m_cacheLru.append(index);
        return m_resolvedChapterCache.value(index);
    }

    QString rawHtml = m_chapterHtmls[index];
    if (!m_css.isEmpty() && !rawHtml.contains("<style", Qt::CaseInsensitive)) {
        int headIdx = rawHtml.indexOf("</head>", Qt::CaseInsensitive);
        if (headIdx != -1) {
            rawHtml.insert(headIdx, QString("<style type=\"text/css\">\n%1\n</style>\n").arg(m_css));
        } else {
            rawHtml.prepend(QString("<style type=\"text/css\">\n%1\n</style>\n").arg(m_css));
        }
    }

    QString resolved = resolveEmbeddedImages(rawHtml, cacheDir);

    // Keep LRU cache of 6 chapters in memory
    if (m_resolvedChapterCache.size() >= 6 && !m_cacheLru.isEmpty()) {
        int oldest = m_cacheLru.takeFirst();
        m_resolvedChapterCache.remove(oldest);
    }
    m_resolvedChapterCache.insert(index, resolved);
    m_cacheLru.append(index);

    return resolved;
}

QString MobiParser::rawChapterHtml(int index) const
{
    if (index < 0 || index >= m_chapterHtmls.size()) {
        return QString();
    }
    return m_chapterHtmls[index];
}
