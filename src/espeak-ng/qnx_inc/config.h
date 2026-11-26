#ifndef ESPEAK_NG_QNX_CONFIG_H
#define ESPEAK_NG_QNX_CONFIG_H

#define HAVE_MKSTEMP 1
#define USE_ASYNC 0
#define USE_KLATT 1
#define USE_LIBPCAUDIO 0
#define USE_LIBSONIC 0
#define USE_MBROLA 0
#define USE_SPEECHPLAYER 0
#define PACKAGE_VERSION "1.52.0"

#ifndef __has_include
#define __has_include(x) 0
#endif

#endif /* ESPEAK_NG_QNX_CONFIG_H */
