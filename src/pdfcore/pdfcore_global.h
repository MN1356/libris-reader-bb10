#ifndef __PDFCORE_GLOBAL_H__
#define __PDFCORE_GLOBAL_H__

/**
 * @file pdfcore_global.h
 * @author Libris Team
 * @brief Global library header
 */

#include <qglobal.h>

/** @def NSRREADERCORE_SHARED Library export/import macro */
#ifdef NSRREADERCORE_LIBRARY
#  define NSRREADERCORE_SHARED	Q_DECL_EXPORT
#else
#  define NSRREADERCORE_SHARED	Q_DECL_IMPORT
#endif

#endif /* __PDFCORE_GLOBAL_H__ */
