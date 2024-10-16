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

#ifndef CAHUTE_PATH_H
#define CAHUTE_PATH_H 1
#include "cdefs.h"

CAHUTE_BEGIN_NAMESPACE
CAHUTE_BEGIN_DECLS

#define CAHUTE_PATH_TYPE_POSIX         1
#define CAHUTE_PATH_TYPE_DOS           2
#define CAHUTE_PATH_TYPE_WIN32_ANSI    3
#define CAHUTE_PATH_TYPE_WIN32_UNICODE 4

#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__)
# define CAHUTE_PATH_TYPE_CLI CAHUTE_PATH_TYPE_WIN32_ANSI
#elif defined(__DJGPP) || defined(__DJGPP)
# define CAHUTE_PATH_TYPE_CLI CAHUTE_PATH_TYPE_DOS
#else
# define CAHUTE_PATH_TYPE_CLI CAHUTE_PATH_TYPE_POSIX
#endif

CAHUTE_EXTERN(int)
cahute_find_path_extension(
    char *cahute__buf,
    size_t cahute__buf_size,
    void const *cahute__path,
    int cahute__path_type
);

CAHUTE_END_DECLS
CAHUTE_END_NAMESPACE

#endif /* CAHUTE_PATH_H */
