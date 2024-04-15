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

#ifndef CAHUTE_CDEFS_H
#define CAHUTE_CDEFS_H 1
#include <cahute/config.h>
#include <stddef.h>

/* C++ declaration and namespace management. */
#ifdef __cplusplus
# define CAHUTE_BEGIN_NAMESPACE namespace "cahute" {
# define CAHUTE_BEGIN_DECLS     extern "C" {
# define CAHUTE_END_DECLS       }
# define CAHUTE_END_NAMESPACE   }
#else
# define CAHUTE_BEGIN_NAMESPACE
# define CAHUTE_BEGIN_DECLS
# define CAHUTE_END_DECLS
# define CAHUTE_END_NAMESPACE
#endif

CAHUTE_BEGIN_NAMESPACE

/* Macro to check the library version. */
#define CAHUTE_PREREQ(CAHUTE__MAJ, CAHUTE__MIN) \
    ((CAHUTE__MAJ) > (CAHUTE_MAJOR) \
     || ((CAHUTE__MAJ) == (CAHUTE_MAJOR) && (CAHUTE__MIN) >= (CAHUTE_MINOR)))

/* ---
 * Compiler and language specific macros.
 * --- */

/* Macro to check if we have at least a specific version of GCC. */
#if defined(CAHUTE_GNUC_PREREQ)
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)
# define CAHUTE_GNUC_PREREQ(CAHUTE__MAJ, CAHUTE__MIN) \
     ((__GNUC__ << 16) + __GNUC_MINOR__ \
      >= ((CAHUTE__MAJ) << 16) + (CAHUTE__MIN))
#else
# define CAHUTE_GNUC_PREREQ(CAHUTE__MAJ, CAHUTE__MIN) 0
#endif

/* Macro to check if we have at least a specific version of MSC. */
#if defined(CAHUTE_MSC_PREREQ) || defined(_MSC_VER)
# define CAHUTE_MSC_PREREQ(CAHUTE__MAJ, CAHUTE__MIN) \
     (_MSC_VER >= (CAHUTE__MAJ) * 1000 + (CAHUTE__MIN))
#else
# define CAHUTE_MSC_PREREQ(CAHUTE__MAJ, CAHUTE__MIN) 0
#endif

/* Export whether the function is deprecated. */
#if CAHUTE_GNUC_PREREQ(3, 0)
# define CAHUTE_DEPRECATED __attribute__((deprecated))
#else
# define CAHUTE_DEPRECATED
#endif

/* Warn if the result is unused (Warn Unused Result). */
#if CAHUTE_GNUC_PREREQ(4, 0)
# define CAHUTE_WUR __attribute__((warn_unused_result))
#elif CAHUTE_MSC_PREREQ(17, 0)
# include <sal.h>
# define CAHUTE_WUR _Check_return_
#else
# define CAHUTE_WUR
#endif

/* Export the function to be used in an extern context. */
#define CAHUTE_EXTERN(TYPE) extern TYPE

/* Make a function local. */
#define CAHUTE_LOCAL(TYPE) static TYPE

/* Make a function local and inline. */
#define CAHUTE_INLINE(TYPE) static inline TYPE

/* Make some data local. */
#define CAHUTE_LOCAL_DATA(TYPE) static TYPE

/* Request non-null parameters.
 * CAHUTE_NNP is to be used inline, CAHUTE_NNP_ATTR is to be used after the
 * function declaration. Both should be defined for portability.
 *
 * For example:
 *
 *     int my_function(char const CAHUTE_NNPTR(my_string)) CAHUTE_NONNULL((1));
 *
 * Which can give any of, depending on the environment:
 *
 *     int my_function(char const my_string[static 1]);
 *     int my_function(char const *my_string) __attribute__((nonnull (1)));
 *     int my_function(char const *my_string);
 */
/* TODO: Define these in some cases. */
#define CAHUTE_NNPTR(NAME) *NAME
#define CAHUTE_NONNULL(INDEXES)

/* Declare a structure. */
#define CAHUTE_DECLARE_TYPE(NAME) \
    struct NAME; \
    typedef struct NAME NAME;

/* ---
 * Integer definitions.
 * --- */

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
# include <inttypes.h>
# include <stdint.h>

/* `stdint.h` and `inttypes.h` are standard C99 headers.
 * `stdint.h` provides the `uintN_t` types, and
 * `inttypes.h` provides the `PRI[uxX]N` macros. */

typedef uint8_t cahute_u8;
typedef uint16_t cahute_u16;
typedef uint32_t cahute_u32;

# define CAHUTE_PRIu8  PRIu8
# define CAHUTE_PRIx8  PRIx8
# define CAHUTE_PRIX8  PRIX8
# define CAHUTE_PRIu16 PRIu16
# define CAHUTE_PRIx16 PRIx16
# define CAHUTE_PRIX16 PRIX16
# define CAHUTE_PRIu32 PRIu32
# define CAHUTE_PRIx32 PRIx32
# define CAHUTE_PRIX32 PRIX32

#else /* C89 */
# include <limits.h>

/* Here, we ought to do some C89 hacking.
 * We'll use the `limits.h` definitions to try and guess which one of the
 * default types are the 8-bit, 16-bit and 32-bit integer. */

# define CAHUTE_P8 "hh"
typedef unsigned char cahute_u8;

/* 16-bit integer. */

# if (USHRT_MAX > 0xffffUL)
#  error "No 16-bit type, exiting!"
# endif
# define CAHUTE_P16 "h"
typedef unsigned short cahute_u16;

/* 32-bit integer. */

# if (UINT_MAX == 0xffffffffUL)
#  define CAHUTE_P32 ""
typedef unsigned int cahute_u32;
# elif (ULONG_MAX == 0xffffffffUL)
#  define CAHUTE_P32 "l"
typedef unsigned long cahute_u32;
# else

/* There is nothing between `char` and `short`, and `char` is always
 * byte-wide;
 *
 * `long long` is not defined in C89, and even if it can be used as a
 * compiler extension for C89, it is supposed to be 64-bit or more.
 * So basically we're running out of options here. */

#  error "No 32-bit type, exiting!"
# endif

# define CAHUTE_PRIu8  CAHUTE_P8 "u"
# define CAHUTE_PRIx8  CAHUTE_P8 "x"
# define CAHUTE_PRIX8  CAHUTE_P8 "X"
# define CAHUTE_PRIu16 CAHUTE_P16 "u"
# define CAHUTE_PRIx16 CAHUTE_P16 "x"
# define CAHUTE_PRIX16 CAHUTE_P16 "X"
# define CAHUTE_PRIu32 CAHUTE_P32 "u"
# define CAHUTE_PRIx32 CAHUTE_P32 "x"
# define CAHUTE_PRIX32 CAHUTE_P32 "X"

#endif

/* printf definition for `size_t`. */

#if defined(_WIN64)
# define CAHUTE_PRIuSIZE "llu"
# define CAHUTE_PRIxSIZE "llx"
# define CAHUTE_PRIXSIZE "llX"
#elif defined(_WIN32)
# define CAHUTE_PRIuSIZE "u"
# define CAHUTE_PRIxSIZE "x"
# define CAHUTE_PRIXSIZE "X"
#else
# define CAHUTE_PRIuSIZE "zu"
# define CAHUTE_PRIxSIZE "zx"
# define CAHUTE_PRIXSIZE "zX"
#endif

/* ---
 * Endianess management.
 * --- */

CAHUTE_BEGIN_DECLS

CAHUTE_EXTERN(cahute_u16) cahute_be16toh(cahute_u16 cahute__x);
CAHUTE_EXTERN(cahute_u16) cahute_le16toh(cahute_u16 cahute__x);
CAHUTE_EXTERN(cahute_u32) cahute_be32toh(cahute_u32 cahute__x);
CAHUTE_EXTERN(cahute_u32) cahute_le32toh(cahute_u32 cahute__x);

CAHUTE_EXTERN(cahute_u16) cahute_htobe16(cahute_u16 cahute__x);
CAHUTE_EXTERN(cahute_u16) cahute_htole16(cahute_u16 cahute__x);
CAHUTE_EXTERN(cahute_u32) cahute_htobe32(cahute_u32 cahute__x);
CAHUTE_EXTERN(cahute_u32) cahute_htole32(cahute_u32 cahute__x);

CAHUTE_END_DECLS

/* Try to get native macros. */
#if defined(__APPLE__)
# include <libkern/OSByteOrder.h>
# define cahute_macro_be16toh(CAHUTE__X) OSSwapBigToHostInt16(CAHUTE__X)
# define cahute_macro_le16toh(CAHUTE__X) OSSwapLittleToHostInt16(CAHUTE__X)
# define cahute_macro_be32toh(CAHUTE__X) OSSwapBigToHostInt32(CAHUTE__X)
# define cahute_macro_le32toh(CAHUTE__X) OSSwapLittleToHostInt32(CAHUTE__X)
# define cahute_macro_htobe16(CAHUTE__X) OSSwapHostToBigInt16(CAHUTE__X)
# define cahute_macro_htole16(CAHUTE__X) OSSwapHostToLittleInt16(CAHUTE__X)
# define cahute_macro_htobe32(CAHUTE__X) OSSwapHostToBigInt32(CAHUTE__X)
# define cahute_macro_htole32(CAHUTE__X) OSSwapHostToLittleInt32(CAHUTE__X)
#elif defined(__OpenBSD__)
# include <sys/endian.h>
# define cahute_macro_be16toh(CAHUTE__X) be16toh(CAHUTE__X)
# define cahute_macro_le16toh(CAHUTE__X) le16toh(CAHUTE__X)
# define cahute_macro_be32toh(CAHUTE__X) be32toh(CAHUTE__X)
# define cahute_macro_le32toh(CAHUTE__X) le32toh(CAHUTE__X)
# define cahute_macro_htobe16(CAHUTE__X) htobe16(CAHUTE__X)
# define cahute_macro_htole16(CAHUTE__X) htole16(CAHUTE__X)
# define cahute_macro_htobe32(CAHUTE__X) htobe32(CAHUTE__X)
# define cahute_macro_htole32(CAHUTE__X) htole32(CAHUTE__X)
#elif defined(__GLIBC__) && defined(__USE_MISC)
# include <endian.h>
# define cahute_macro_be16toh(CAHUTE__X) be16toh(CAHUTE__X)
# define cahute_macro_le16toh(CAHUTE__X) le16toh(CAHUTE__X)
# define cahute_macro_be32toh(CAHUTE__X) be32toh(CAHUTE__X)
# define cahute_macro_le32toh(CAHUTE__X) le32toh(CAHUTE__X)
# define cahute_macro_htobe16(CAHUTE__X) htobe16(CAHUTE__X)
# define cahute_macro_htole16(CAHUTE__X) htole16(CAHUTE__X)
# define cahute_macro_htobe32(CAHUTE__X) htobe32(CAHUTE__X)
# define cahute_macro_htole32(CAHUTE__X) htole32(CAHUTE__X)
#endif

/* CAHUTE_NO_ENDIAN may be defined by cdefs.c to be able to define the
 * functions prototyped above. */
#ifndef CAHUTE_NO_ENDIAN
# ifdef cahute_macro_be16toh
#  define cahute_be16toh(CAHUTE__X) cahute_macro_be16toh(CAHUTE__X)
# endif
# ifdef cahute_macro_le16toh
#  define cahute_le16toh(CAHUTE__X) cahute_macro_le16toh(CAHUTE__X)
# endif
# ifdef cahute_macro_be32toh
#  define cahute_be32toh(CAHUTE__X) cahute_macro_be32toh(CAHUTE__X)
# endif
# ifdef cahute_macro_le32toh
#  define cahute_le32toh(CAHUTE__X) cahute_macro_le32toh(CAHUTE__X)
# endif
# ifdef cahute_macro_htobe16
#  define cahute_htobe16(CAHUTE__X) cahute_macro_htobe16(CAHUTE__X)
# endif
# ifdef cahute_macro_htole16
#  define cahute_htole16(CAHUTE__X) cahute_macro_htole16(CAHUTE__X)
# endif
# ifdef cahute_macro_htobe32
#  define cahute_htobe32(CAHUTE__X) cahute_macro_htobe32(CAHUTE__X)
# endif
# ifdef cahute_macro_htole32
#  define cahute_htole32(CAHUTE__X) cahute_macro_htole32(CAHUTE__X)
# endif
#endif

CAHUTE_END_NAMESPACE

#endif /* CAHUTE_CDEFS_H */
