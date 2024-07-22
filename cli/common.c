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

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* On some platforms, for directories, ftell() returns an insanely high
 * number that may be platform-specific, e.g. 9223372036854775807 (2^63 - 1),
 * which would correspond to the highest positive value of a long if defined
 * on 64 bits. In order to detect such cases in a reasonably
 * platform-independent manner, we want to cap the size of any file that
 * gets stored into memory.
 *
 * See ``read_file_contents()`` for more details on the usage of this
 * constant. */
#define REASONABLE_FILE_CONTENT_LIMIT 134217728 /* 128 MiB */

/**
 * Get the current logging level as a string.
 *
 * @return Logging level name.
 */
extern char const *get_current_log_level(void) {
    int loglevel = cahute_get_log_level();

    switch (loglevel) {
    case CAHUTE_LOGLEVEL_INFO:
        return "info";
    case CAHUTE_LOGLEVEL_WARNING:
        return "warning";
    case CAHUTE_LOGLEVEL_ERROR:
        return "error";
    case CAHUTE_LOGLEVEL_FATAL:
        return "fatal";
    default:
        return "(none)";
    }
}

/**
 * Set the current logging level as a string.
 *
 * @param loglevel Name of the loglevel to set.
 */
extern void set_log_level(char const *loglevel) {
    int value = CAHUTE_LOGLEVEL_NONE;

    if (!strcmp(loglevel, "info"))
        value = CAHUTE_LOGLEVEL_INFO;
    else if (!strcmp(loglevel, "warning"))
        value = CAHUTE_LOGLEVEL_WARNING;
    else if (!strcmp(loglevel, "error"))
        value = CAHUTE_LOGLEVEL_ERROR;
    else if (!strcmp(loglevel, "fatal"))
        value = CAHUTE_LOGLEVEL_FATAL;

    cahute_set_log_level(value);
}

/**
 * Print content from an encoding into a destination one.
 *
 * @param data Data to convert on-the-fly.
 * @param data_size Size of the data to convert.
 * @param encoding Encoding of the data.
 * @param dest_encoding Encoding to display the data as.
 */
extern void print_content(
    void const *data,
    size_t data_size,
    int encoding,
    int dest_encoding
) {
    cahute_u8 buf[128], *p;
    size_t p_size;
    int err;

    while (1) {
        p = buf;
        p_size = sizeof(buf);
        err = cahute_convert_text(
            (void **)&p,
            &p_size,
            &data,
            &data_size,
            dest_encoding,
            encoding
        );
        if (p_size < sizeof(buf)) {
            fwrite(buf, sizeof(buf) - p_size, 1, stdout);
            if (!err || err == CAHUTE_ERROR_TERMINATED)
                return;

            if (err == CAHUTE_ERROR_SIZE)
                continue;

            break;
        }

        if (!err)
            return;
        break; /* Including CAHUTE_ERROR_SIZE. */
    }

    fprintf(stdout, "<CONVERSION FAILED: 0x%04X>", err);
}

/**
 * Read file contents into a buffer.
 *
 * @param path Path to the file to read.
 * @param datap Pointer to the data to allocate and populate.
 * @param sizep Pointer to the data size to populate.
 * @return 0 if ok, other otherwise.
 */
extern int
read_file_contents(char const *path, cahute_u8 **datap, size_t *sizep) {
    cahute_u8 *data = NULL;
    size_t size;
    FILE *fp;

    fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Unable to open the file: %s\n", strerror(errno));
        goto fail;
    }

    if (fseek(fp, 0, SEEK_END)) {
        fprintf(
            stderr,
            "Unable to seek to the end of the file: %s\n",
            strerror(errno)
        );
        goto fail;
    }

    size = ftell(fp);
    if (size > REASONABLE_FILE_CONTENT_LIMIT) {
        fprintf(
            stderr,
            "Unable to open the file: file too big (over 128MiB) or "
            "unsupported file type (e.g. directory)\n"
        );
        goto fail;
    }

    if (fseek(fp, 0, SEEK_SET)) {
        fprintf(
            stderr,
            "Unable to seek to the start of the file: %s\n",
            strerror(errno)
        );
        goto fail;
    }

    if (!size) {
        fprintf(stderr, "File cannot be empty!\n");
        goto fail;
    }

    data = malloc(size);
    if (!data) {
        fprintf(stderr, "malloc() failed.\n");
        goto fail;
    }

    if (!fread(data, size, 1, fp)) {
        fprintf(stderr, "Could not read file data: %s\n", strerror(errno));
        goto fail;
    }

    fclose(fp);

    *datap = data;
    *sizep = size;
    return 0;

fail:
    if (fp)
        fclose(fp);
    if (data)
        free(data);
    return 1;
}

/**
 * Get a line and allocate it.
 *
 * Source: https://github.com/ivanrad/getline
 *
 * @param sp Pointer to the string to allocate.
 * @param np Pointer to the gathered size.
 * @param delim Delimiter.
 * @param filep File pointer.
 * @return Size of the obtained line.
 */
cahute_ssize
portable_getdelim(char **lineptr, size_t *n, int delim, FILE *stream) {
    char *cur_pos, *new_lineptr;
    size_t new_lineptr_len;
    int c;

    if (lineptr == NULL || n == NULL || stream == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (*lineptr == NULL) {
        *n = 128; /* Initial length. */
        if ((*lineptr = (char *)malloc(*n)) == NULL) {
            errno = ENOMEM;
            return -1;
        }
    }

    cur_pos = *lineptr;
    for (;;) {
        c = getc(stream);

        if (ferror(stream) || (c == EOF && cur_pos == *lineptr))
            return -1;

        if (c == EOF)
            break;

        if ((*lineptr + *n - cur_pos) < 2) {
            if (CAHUTE_SSIZE_MAX / 2 < *n) {
#ifdef EOVERFLOW
                errno = EOVERFLOW;
#else
                errno = ERANGE; /* no EOVERFLOW defined */
#endif
                return -1;
            }
            new_lineptr_len = *n * 2;

            if ((new_lineptr = (char *)realloc(*lineptr, new_lineptr_len))
                == NULL) {
                errno = ENOMEM;
                return -1;
            }
            cur_pos = new_lineptr + (cur_pos - *lineptr);
            *lineptr = new_lineptr;
            *n = new_lineptr_len;
        }

        *cur_pos++ = (char)c;

        if (c == delim)
            break;
    }

    *cur_pos = '\0';
    return (cahute_ssize)(cur_pos - *lineptr);
}
