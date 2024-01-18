/* ****************************************************************************
 * Copyright (C) 2024 Thomas Touhey <thomas@touhey.fr>
 *
 * This software is governed by the CeCILL 2.1 license under French law and
 * abiding by the rules of distribution of free software. You can use, modify
 * and/or redistribute the software under the terms of the CeCILL 2.1 license
 * as circulated by CEA, CNRS and INRIA at the following
 * URL: https://cecill.info
 *
 * As a counterpart to the access to the source code and rights to copy, modify
 * and redistribute granted by the license, users are provided only with a
 * limited warranty and the software's author, the holder of the economic
 * rights, and the successive licensors have only limited liability.
 *
 * In this respect, the user's attention is drawn to the risks associated with
 * loading, using, modifying and/or developing or reproducing the software by
 * the user in light of its specific status of free software, that may mean
 * that it is complicated to manipulate, and that also therefore means that it
 * is reserved for developers and experienced professionals having in-depth
 * computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling
 * the security of their systems and/or data to be ensured and, more generally,
 * to use and operate it in the same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL 2.1 license and that you accept its terms.
 * ************************************************************************* */

#define CAHUTE_NO_ENDIAN
#include "internals.h"

/**
 * Transform big endian 16-bit integer to host endianness.
 *
 * This is a dynamic fallback to static inline functions that may be defined
 * in "cdefs.h", and is only used in last resort.
 *
 * @param x Integer in big endian.
 * @return Integer in host endianness.
 */
CAHUTE_EXTERN(cahute_u16) cahute_be16toh(cahute_u16 x) {
#ifdef cahute_macro_be16toh
    return cahute_macro_be16toh(x);
#else
    union {
        cahute_u8 b8[2];
        cahute_u16 b16;
    } tmp;

    tmp.b16 = x;
    return (tmp.b8[0] << 8) | tmp.b8[1];
#endif
}

/**
 * Transform little endian 16-bit integer to host endianness.
 *
 * This is a dynamic fallback to static inline functions that may be defined
 * in "cdefs.h", and is only used in last resort.
 *
 * @param x Integer in little endian.
 * @return Integer in host endianness.
 */
CAHUTE_EXTERN(cahute_u16) cahute_le16toh(cahute_u16 x) {
#ifdef cahute_macro_le16toh
    return cahute_macro_le16toh(x);
#else
    union {
        cahute_u8 b8[2];
        cahute_u16 b16;
    } tmp;

    tmp.b16 = x;
    return (tmp.b8[1] << 8) | tmp.b8[0];
#endif
}

/**
 * Transform big endian 32-bit integer to host endianness.
 *
 * This is a dynamic fallback to static inline functions that may be defined
 * in "cdefs.h", and is only used in last resort.
 *
 * @param x Integer in big endian.
 * @return Integer in host endianness.
 */
CAHUTE_EXTERN(cahute_u32) cahute_be32toh(cahute_u32 x) {
#ifdef cahute_macro_be32toh
    return cahute_macro_be32toh(x);
#else
    union {
        cahute_u8 b8[4];
        cahute_u32 b32;
    } tmp;

    tmp.b32 = x;
    return (tmp.b8[0] << 24) | (tmp.b8[1] << 16) | (tmp.b8[2] << 8)
           | tmp.b8[3];
#endif
}

/**
 * Transform little endian 32-bit integer to host endianness.
 *
 * This is a dynamic fallback to static inline functions that may be defined
 * in "cdefs.h", and is only used in last resort.
 *
 * @param x Integer in little endian.
 * @return Integer in host endianness.
 */
CAHUTE_EXTERN(cahute_u32) cahute_le32toh(cahute_u32 x) {
#ifdef cahute_macro_le32toh
    return cahute_macro_le32toh(x);
#else
    union {
        cahute_u8 b8[4];
        cahute_u32 b32;
    } tmp;

    tmp.b32 = x;
    return (tmp.b8[3] << 24) | (tmp.b8[2] << 16) | (tmp.b8[1] << 8)
           | tmp.b8[0];
#endif
}

/**
 * Transform 16-bit integer in host endianness to big endian.
 *
 * This is a dynamic fallback to static inline functions that may be defined
 * in "cdefs.h", and is only used in last resort.
 *
 * @param x Integer in host endianness.
 * @return Integer in big endian.
 */
CAHUTE_EXTERN(cahute_u16) cahute_htobe16(cahute_u16 x) {
#ifdef cahute_macro_htobe16
    return cahute_macro_htobe16(x);
#else
    union {
        cahute_u8 b8[2];
        cahute_u16 b16;
    } tmp;

    tmp.b8[0] = x >> 8;
    tmp.b8[1] = x & 0xFF;
    return tmp.b16;
#endif
}

/**
 * Transform 16-bit integer in host endianness to little endian.
 *
 * This is a dynamic fallback to static inline functions that may be defined
 * in "cdefs.h", and is only used in last resort.
 *
 * @param x Integer in host endianness.
 * @return Integer in little endian.
 */
CAHUTE_EXTERN(cahute_u16) cahute_htole16(cahute_u16 x) {
#ifdef cahute_macro_htole16
    return cahute_macro_htole16(x);
#else
    union {
        cahute_u8 b8[2];
        cahute_u16 b16;
    } tmp;

    tmp.b8[0] = x & 0xFF;
    tmp.b8[1] = x >> 8;
    return tmp.b16;
#endif
}

/**
 * Transform 32-bit integer in host endianness to big endian.
 *
 * This is a dynamic fallback to static inline functions that may be defined
 * in "cdefs.h", and is only used in last resort.
 *
 * @param x Integer in host endianness.
 * @return Integer in big endian.
 */
CAHUTE_EXTERN(cahute_u32) cahute_htobe32(cahute_u32 x) {
#ifdef cahute_macro_htobe32
    return cahute_macro_htobe32(x);
#else
    union {
        cahute_u8 b8[4];
        cahute_u32 b32;
    } tmp;

    tmp.b8[0] = (x >> 24);
    tmp.b8[1] = (x >> 16) & 0xFF;
    tmp.b8[2] = (x >> 8) & 0xFF;
    tmp.b8[3] = x & 0xFF;
    return tmp.b32;
#endif
}

/**
 * Transform 32-bit integer in host endianness to little endian.
 *
 * This is a dynamic fallback to static inline functions that may be defined
 * in "cdefs.h", and is only used in last resort.
 *
 * @param x Integer in host endianness.
 * @return Integer in little endian.
 */
CAHUTE_EXTERN(cahute_u32) cahute_htole32(cahute_u32 x) {
#ifdef cahute_macro_htole32
    return cahute_macro_htole32(x);
#else
    union {
        cahute_u8 b8[4];
        cahute_u32 b32;
    } tmp;

    tmp.b8[0] = x & 0xFF;
    tmp.b8[1] = (x >> 8) & 0xFF;
    tmp.b8[2] = (x >> 16) & 0xFF;
    tmp.b8[3] = (x >> 24);
    return tmp.b32;
#endif
}
