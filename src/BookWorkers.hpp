#ifndef BOOKWORKERS_HPP
#define BOOKWORKERS_HPP

#include <QThread>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QSet>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QSettings>
#include <QCryptographicHash>
#include <QMetaType>
#include "EpubParser.hpp"
#include "TxtParser.hpp"
#include "Fb2Parser.hpp"

struct SearchParams {
    QString query;
    QString format;
    QString filePath;
    QString currentPdfText;
    int currentChapterIndex;
    int maxTotalResults;
    QString scope;

    SearchParams() : currentChapterIndex(0), maxTotalResults(150), scope("full") {}
};

class SearchWorker : public QThread
{
    Q_OBJECT
public:
    explicit SearchWorker(const SearchParams &params, QObject *parent = 0);
    ~SearchWorker();
    void cancel();

signals:
    void searchCompleted(const QVariantList &results);

protected:
    void run();

private:
    SearchParams m_params;
    volatile bool m_cancelled;

    static QString extractSnippet(const QString &text, int matchPos, int matchLen, const QString &query = QString());
};

class ScanWorker : public QThread
{
    Q_OBJECT
public:
    explicit ScanWorker(const QStringList &searchRoots, const QString &cacheDir, QObject *parent = 0);
    ~ScanWorker();
    void cancel();

signals:
    void scanCompleted(const QList<QVariantMap> &allBooks);

protected:
    void run();

private:
    void scanDir(const QString &dirPath, int depth, QSettings &settings, QSet<QString> &visitedDirs, QSet<QString> &addedPaths, QList<QVariantMap> &books);
    void extractSeriesFromTitleOrFile(const QString &title, const QString &rawFileName, QString &outSeries, int &outSeriesIndex);

    QStringList m_searchRoots;
    QString m_cacheDir;
    volatile bool m_cancelled;
};

Q_DECLARE_METATYPE(QList<QVariantMap>)

#endif // BOOKWORKERS_HPP
