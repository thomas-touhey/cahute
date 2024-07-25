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
/* Internal compatibility header. */
#ifndef COMPAT_H
#define COMPAT_H 1
#include <cahute/cdefs.h>

/* Make a function local. */
#define CAHUTE_LOCAL(TYPE) static TYPE

/* Make a function local and inline. */
#define CAHUTE_INLINE(TYPE) static inline TYPE

/* Make some data local. */
#define CAHUTE_LOCAL_DATA(TYPE) static TYPE

#if CAHUTE_MSC_PREREQ(12, 0)
# include <BaseTsd.h>

typedef SSIZE_T cahute_ssize;
typedef UINT_PTR cahute_uintptr;
typedef INT8 cahute_i8;
typedef UINT16 cahute_u16;
typedef INT16 cahute_i16;
typedef UINT32 cahute_u32;
typedef INT32 cahute_i32;

# if (UINT16) - 1 == UINT_MAX
#  define CAHUTE_P16 ""
# else
#  define CAHUTE_P16 "h"
# endif

# if (UINT32) - 1 == UINT_MAX
#  define CAHUTE_P32 ""
# else
#  define CAHUTE_P32 "l"
# endif
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
# include <inttypes.h>
# include <stdint.h>

/* `stdint.h` and `inttypes.h` are standard C99 headers.
 * `stdint.h` provides the `uintN_t` types, and
 * `inttypes.h` provides the `PRI[uxX]N` macros. */
typedef int8_t cahute_i8;
typedef uint16_t cahute_u16;
typedef int16_t cahute_i16;
typedef uint32_t cahute_u32;
typedef int32_t cahute_i32;

# define CAHUTE_PRIu16 PRIu16
# define CAHUTE_PRIx16 PRIx16
# define CAHUTE_PRIX16 PRIX16
# define CAHUTE_PRIu32 PRIu32
# define CAHUTE_PRIx32 PRIx32
# define CAHUTE_PRIX32 PRIX32

# define CAHUTE_SSIZE_MAX   SSIZE_MAX
# define CAHUTE_UINTPTR_MAX UINTPTR_MAX
#else /* C89 */
/* Here, we ought to do some C89 hacking.
 * We'll use the `limits.h` definitions to try and guess which one of the
 * default types are the 8-bit, 16-bit and 32-bit integer. */

typedef signed char cahute_i8;

/* 16-bit integer. */

# if (USHRT_MAX > 0xffffUL)
#  error "No 16-bit type!"
# endif
# define CAHUTE_P16 "h"
typedef unsigned short cahute_u16;
typedef signed short cahute_i16;

/* 32-bit integer. */

# if (UINT_MAX == 0xffffffffUL)
#  define CAHUTE_P32 ""
typedef unsigned int cahute_u32;
typedef signed int cahute_i32;
# elif (ULONG_MAX == 0xffffffffUL)
#  define CAHUTE_P32 "l"
typedef unsigned long cahute_u32;
typedef signed long cahute_i32;
# else
/* There is nothing between `char` and `short`, and `char` is always
 * byte-wide;
 *
 * `long long` is not defined in C89, and even if it can be used as a
 * compiler extension for C89, it is supposed to be 64-bit or more.
 * So basically we're running out of options here. */
#  error "No 32-bit type!"
# endif

# if SIZEOF_SIZE_T == 1
typedef cahute_i8 cahute_ssize;
# elif SIZEOF_SIZE_T == 2
typedef cahute_i16 cahute_ssize;
# elif SIZEOF_SIZE_T == 4
typedef cahute_i32 cahute_ssize;
# elif SIZEOF_SIZE_T == 8
typedef long long cahute_ssize;
# else
#  error "Cannot define cahute_ssize for the determined type size."
# endif

# if SIZEOF_UINTPTR_T == 1
typedef cahute_u8 cahute_uintptr;
# elif SIZEOF_UINTPTR_T == 2
typedef cahute_u16 cahute_uintptr;
# elif SIZEOF_UINTPTR_T == 4
typedef cahute_u32 cahute_uintptr;
# elif SIZEOF_UINTPTR_T == 8
typedef unsigned long long cahute_uintptr;
# else
#  error "Cannot define cahute_uintptr for the determined type size."
# endif
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__)
# define CAHUTE_PSIZE "I"
#else
# define CAHUTE_PSIZE "z"
#endif

#ifndef CAHUTE_PRIu16
# define CAHUTE_PRIu16 CAHUTE_P16 "u"
#endif
#ifndef CAHUTE_PRIx16
# define CAHUTE_PRIx16 CAHUTE_P16 "x"
#endif
#ifndef CAHUTE_PRIX16
# define CAHUTE_PRIX16 CAHUTE_P16 "X"
#endif
#ifndef CAHUTE_PRIu32
# define CAHUTE_PRIu32 CAHUTE_P32 "u"
#endif
#ifndef CAHUTE_PRIx32
# define CAHUTE_PRIx32 CAHUTE_P32 "x"
#endif
#ifndef CAHUTE_PRIX32
# define CAHUTE_PRIX32 CAHUTE_P32 "X"
#endif
#ifndef CAHUTE_PRIuSIZE
# define CAHUTE_PRIuSIZE CAHUTE_PSIZE "u"
#endif
#ifndef CAHUTE_PRIxSIZE
# define CAHUTE_PRIxSIZE CAHUTE_PSIZE "x"
#endif
#ifndef CAHUTE_PRIXSIZE
# define CAHUTE_PRIXSIZE CAHUTE_PSIZE "X"
#endif

#ifndef CAHUTE_SSIZE_MAX
# define CAHUTE_SSIZE_MAX (cahute_ssize)(~((size_t)-1 >> 1) >> 1)
#endif
#ifndef CAHUTE_UINTPTR_MAX
# define CAHUTE_UINTPTR_MAX ((cahute_uintptr)-1)
#endif
#endif /* COMPAT_H */
