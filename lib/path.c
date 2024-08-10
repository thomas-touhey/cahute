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

#include "internals.h"

/**
 * Find the extension in a given path.
 *
 * @param buf Buffer in which to write the found extension.
 * @param buf_size Size of the buffer.
 * @param path Path from which to find the extension.
 * @param path_type Type of the path from which to find the extension.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_find_path_extension(
    char *buf,
    size_t buf_size,
    void const *path,
    int path_type
) {
    /* In any case, if no extension is found, we want to make it clear to
     * the caller. */
    *buf = '\0';

    switch (path_type) {
    case CAHUTE_PATH_TYPE_POSIX: {
        char const *p;
        char *q;
        size_t ext_len;

        if (!*(char const *)path) /* Empty path. */
            return CAHUTE_ERROR_NOT_FOUND;

        /* NOTE: We assume ASCII/UTF-8 for POSIX paths for now.
         * We should probably stop assuming this at some point. */
        for (p = (char const *)path + strlen(path) - 1;
             p > buf && *p != '/' && *p != '.';
             p--)
            ;

        if (*p != '.') {
            /* The end is not an extension. */
            return CAHUTE_ERROR_NOT_FOUND;
        }

        ext_len = strlen(++p);
        if (ext_len + 1 > buf_size)
            return CAHUTE_ERROR_SIZE;

        for (q = buf; *p; p++, q++)
            *q = tolower(*p);

        *q = '\0';
    } break;

    case CAHUTE_PATH_TYPE_DOS:
    case CAHUTE_PATH_TYPE_WIN32_ANSI: {
        char const *p;
        char *q;
        size_t ext_len;

        if (!*(char const *)path) /* Empty path. */
            return CAHUTE_ERROR_NOT_FOUND;

        for (p = (char const *)path + strlen(path) - 1;
             p > buf && *p != '/' && *p != '\\' && *p != '.';
             p--)
            ;

        if (*p != '.') {
            /* The end is not an extension. */
            return CAHUTE_ERROR_NOT_FOUND;
        }

        ext_len = strlen(++p);
        if (ext_len + 1 > buf_size)
            return CAHUTE_ERROR_SIZE;

        for (q = buf; *p; p++, q++)
            *q = tolower(*p);

        *q = '\0';
    } break;

    default:
        CAHUTE_RETURN_IMPL("Could not get extension for path type.");
    }

    return CAHUTE_OK;
}
