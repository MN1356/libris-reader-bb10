#ifndef BOOKCHAPTER_HPP
#define BOOKCHAPTER_HPP

#include <QString>

struct BookChapter {
    QString id;
    QString href;
    QString title;
};

// Backwards-compatible alias for existing EPUB codebase
typedef BookChapter EpubChapter;

#endif // BOOKCHAPTER_HPP
