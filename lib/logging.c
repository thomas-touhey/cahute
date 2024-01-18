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
#include <stdarg.h>
#include <time.h>

CAHUTE_LOCAL_DATA(char const * const)
hexadecimal_alphabet = "0123456789ABCDEF";
CAHUTE_LOCAL_DATA(int) current_log_level = CAHUTE_DEFAULT_LOGLEVEL;

/**
 * Get the string corresponding to the given log level.
 *
 * @param loglevel Log level to get the string for.
 * @return String corresponding to the given loglevel.
 */
CAHUTE_LOCAL(char const *) get_loglevel_string(int loglevel) {
    switch (loglevel) {
    case CAHUTE_LOGLEVEL_INFO:
        return "info";
    case CAHUTE_LOGLEVEL_WARNING:
        return "warning";
    case CAHUTE_LOGLEVEL_ERROR:
        return "error";
    case CAHUTE_LOGLEVEL_FATAL:
        return "fatal";
    case CAHUTE_LOGLEVEL_NONE:
        return "(none)";
    default:
        return "(unknown)";
    }
}

/**
 * Get the current log level.
 *
 * This is used to expose the static log level variable to the user, as part
 * of the public API.
 *
 * @return Current log level.
 */
CAHUTE_EXTERN(int) cahute_get_log_level(void) {
    return current_log_level;
}

/**
 * Set the current log level.
 *
 * This is used to expose the static log level variable for setting to the
 * user, as part of the public API.
 *
 * @param loglevel Log level to set.
 */
CAHUTE_EXTERN(void) cahute_set_log_level(int loglevel) {
    current_log_level = loglevel;
}

/**
 * Put the logging prefix corresponding to the given logging level on
 * standard error, with the provided data.
 *
 * This produces an output alike the following:
 *
 *     [cahute info] This is a log without a function.
 *     [cahute info] user_func: This is a log with a user function.
 *     [cahute info] open_usb: This is a log with an internal function.
 *
 * Note that in the case of Cahute functions, we remove the "cahute_" prefix
 * for easier reading.
 *
 * @param loglevel Logging level at which to put the prefix.
 * @param func Optional name of the function for which the log is emitted.
 */
CAHUTE_LOCAL(void) put_log_prefix(int loglevel, char const *func) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timebuf[100];
    char levelbuf[20];

    sprintf(
        timebuf,
        "%04d-%02d-%02d %02d:%02d:%02d",
        1900 + tm->tm_year,
        1 + tm->tm_mon,
        tm->tm_mday,
        tm->tm_hour,
        tm->tm_min,
        tm->tm_sec
    );
    sprintf(levelbuf, "cahute %s", get_loglevel_string(loglevel));

    if (!func) {
        fprintf(stderr, "\r[%s %14s] ", timebuf, levelbuf);
        return;
    }

    if (!strncmp(func, "cahute_", 7))
        func = &func[7];

    fprintf(stderr, "\r[%s %14s] %s: ", timebuf, levelbuf, func);
}

/**
 * Output a log message with parameters for a given log level.
 *
 * This is the function called behind the "msg((fmt, ...))" macro defined
 * in the common internals for the library.
 *
 * @param loglevel Logging level at which to emit the message.
 * @param func Optional function name for which to emit the message.
 * @param format Format string to evaluate with the parameters.
 * @param ... Optional parameters for formatting.
 */
CAHUTE_EXTERN(void)
cahute_log_message(int loglevel, char const *func, char const *format, ...) {
    va_list va;

    va_start(va, format);
    if (current_log_level <= loglevel) {
        put_log_prefix(loglevel, func);
        vfprintf(stderr, format, va);
        fputc('\n', stderr);
    }
    va_end(va);
}

/**
 * Output a memory area in displayable format for a given log level.
 *
 * This is the function called behind the "mem((data, data_size))" macro
 * defined in the common internals for the library.
 *
 * @param loglevel Logging level at which to emit the messages.
 * @param func Optional function name for which to emit the message.
 * @param mem Pointer to the memory area to present in the messages.
 * @param size Size of the memory area to present in the messages.
 */
CAHUTE_EXTERN(void)
cahute_log_memory(
    int loglevel,
    char const *func,
    void const *mem,
    size_t size
) {
    char linebuf[80];
    cahute_u8 const *p;
    size_t offset = 0;

    if (current_log_level > loglevel)
        return;

    if (!size) {
        put_log_prefix(loglevel, func);
        fprintf(stderr, "(nothing)\n");
        return;
    }

    memcpy(linebuf, "00000000  0000 0000 0000 0000  ", 31);
    for (p = mem; size > 0; offset += 8) {
        char *s = linebuf;

        *s++ = hexadecimal_alphabet[(offset >> 28) & 15];
        *s++ = hexadecimal_alphabet[(offset >> 24) & 15];
        *s++ = hexadecimal_alphabet[(offset >> 20) & 15];
        *s++ = hexadecimal_alphabet[(offset >> 16) & 15];
        *s++ = hexadecimal_alphabet[(offset >> 12) & 15];
        *s++ = hexadecimal_alphabet[(offset >> 8) & 15];
        *s++ = hexadecimal_alphabet[(offset >> 4) & 15];
        *s++ = hexadecimal_alphabet[offset & 15];

        s += 2;

        /* Fill in the hexadecimal part. */
        {
            cahute_u8 const *pc = p;
            size_t l;

            for (l = 0; l < 8; l++) {
                if (l < size) {
                    *s++ = hexadecimal_alphabet[(*pc >> 4) & 15];
                    *s++ = hexadecimal_alphabet[*pc++ & 15];
                } else {
                    *s++ = ' ';
                    *s++ = ' ';
                }

                s += l & 1; /* After an odd index, add a space. */
            }
        }

        ++s; /* Extra space. */

        /* Fill in the ASCII part. */
        {
            int l;

            for (l = 0; l < 8; l++, p++, size--) {
                if (!size)
                    break;

                if (isprint(*p))
                    *s++ = *p;
                else
                    *s++ = '.';
            }
        }

        *s++ = '\n';
        *s = '\0';

        put_log_prefix(loglevel, func);
        fputs(linebuf, stderr);
    }
}
