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
#include <limits.h>

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
     (_MSC_VER >= (CAHUTE__MAJ) * 100 + (CAHUTE__MIN))
#else
# define CAHUTE_MSC_PREREQ(CAHUTE__MAJ, CAHUTE__MIN) 0
#endif

/* Export the function to be used in an extern context. */
#define CAHUTE_EXTERN(TYPE) extern TYPE

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

/* Define the cahute_u8 type. */
#if CHAR_BIT != 8
# error "Char type is expected to be 8 bits long."
#endif

typedef unsigned char cahute_u8;

#define CAHUTE_P8 "hh"

#define CAHUTE_PRIu8 CAHUTE_P8 "u"
#define CAHUTE_PRIx8 CAHUTE_P8 "x"
#define CAHUTE_PRIX8 CAHUTE_P8 "X"

CAHUTE_END_NAMESPACE

#endif /* CAHUTE_CDEFS_H */
