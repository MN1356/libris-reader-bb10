#ifndef REGEXUTIL_HPP
#define REGEXUTIL_HPP

#include <QRegExp>
#include <QString>

// Qt 4.8's QRegExp does not support lazy quantifiers (*?, +?, ??). Patterns that
// use them fail to compile and never match. Build the expression with the lazy
// operators removed and enable minimal matching instead.
inline QRegExp minimalRx(const QString &pattern, Qt::CaseSensitivity cs = Qt::CaseSensitive)
{
    QRegExp rx(pattern, cs);
    rx.setMinimal(true);
    return rx;
}

#endif // REGEXUTIL_HPP
