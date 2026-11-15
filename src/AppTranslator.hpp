#ifndef APPTRANSLATOR_HPP
#define APPTRANSLATOR_HPP

#include <QTranslator>
#include <QMap>
#include <QString>

class AppTranslator : public QTranslator {
    Q_OBJECT

public:
    explicit AppTranslator(QObject *parent = 0);

    void setLanguage(const QString &lang);
    QString currentLanguage() const;

    virtual QString translate(const char *context, const char *sourceText,
                              const char *disambiguation = 0) const;
    virtual bool isEmpty() const;

private:
    QString m_language; // "vi" or "en"
    QMap<QString, QString> m_en;
    QMap<QString, QString> m_vi;

    void initTranslations();
    void add(const char *source, const char *en, const char *vi);
};

#endif // APPTRANSLATOR_HPP
