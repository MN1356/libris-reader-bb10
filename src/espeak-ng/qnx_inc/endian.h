#ifndef ESPEAK_NG_QNX_ENDIAN_H
#define ESPEAK_NG_QNX_ENDIAN_H

#include <stdint.h>

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif

#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#ifndef htole16
#define htole16(x) ((uint16_t)(x))
#endif

#ifndef le16toh
#define le16toh(x) ((uint16_t)(x))
#endif

#ifndef htole32
#define htole32(x) ((uint32_t)(x))
#endif

#ifndef le32toh
#define le32toh(x) ((uint32_t)(x))
#endif

#endif /* ESPEAK_NG_QNX_ENDIAN_H */
