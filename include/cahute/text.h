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

#ifndef CAHUTE_TEXT_H
#define CAHUTE_TEXT_H 1
#include "cdefs.h"

CAHUTE_BEGIN_NAMESPACE

#define CAHUTE_TEXT_ENCODING_LEGACY_8       1
#define CAHUTE_TEXT_ENCODING_LEGACY_16_HOST 2
#define CAHUTE_TEXT_ENCODING_LEGACY_16_BE   3
#define CAHUTE_TEXT_ENCODING_LEGACY_16_LE   4

#define CAHUTE_TEXT_ENCODING_9860_8       5
#define CAHUTE_TEXT_ENCODING_9860_16_HOST 6
#define CAHUTE_TEXT_ENCODING_9860_16_BE   7
#define CAHUTE_TEXT_ENCODING_9860_16_LE   8

#define CAHUTE_TEXT_ENCODING_CAT 10
#define CAHUTE_TEXT_ENCODING_CTF 11

#define CAHUTE_TEXT_ENCODING_UTF32_HOST 20
#define CAHUTE_TEXT_ENCODING_UTF32_BE   21
#define CAHUTE_TEXT_ENCODING_UTF32_LE   22
#define CAHUTE_TEXT_ENCODING_UTF8       23

CAHUTE_BEGIN_DECLS

CAHUTE_EXTERN(int)
cahute_convert_text(
    void **cahute__bufp,
    size_t *cahute__buf_sizep,
    void const **cahute__datap,
    size_t *cahute__data_sizep,
    int cahute__dest_encoding,
    int cahute__source_encoding
);

CAHUTE_EXTERN(int)
cahute_convert_to_utf8(
    char *cahute__buf,
    size_t cahute__buf_size,
    void const *cahute__data,
    size_t cahute__data_size,
    int cahute__encoding
);

CAHUTE_END_DECLS
CAHUTE_END_NAMESPACE

#endif /* CAHUTE_TEXT_H */
