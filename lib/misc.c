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
 * Get the name of a given error.
 *
 * @param code Code of the error to give the name for.
 * @return Name of the error.
 */
CAHUTE_EXTERN(char const *) cahute_get_error_name(int code) {
    switch (code) {
    case CAHUTE_OK:
        return "CAHUTE_OK";
    case CAHUTE_ERROR_UNKNOWN:
        return "CAHUTE_ERROR_UNKNOWN";
    case CAHUTE_ERROR_IMPL:
        return "CAHUTE_ERROR_IMPL";
    case CAHUTE_ERROR_ALLOC:
        return "CAHUTE_ERROR_ALLOC";
    case CAHUTE_ERROR_PRIV:
        return "CAHUTE_ERROR_PRIV";
    case CAHUTE_ERROR_INT:
        return "CAHUTE_ERROR_INT";
    case CAHUTE_ERROR_SIZE:
        return "CAHUTE_ERROR_SIZE";
    case CAHUTE_ERROR_TRUNC:
        return "CAHUTE_ERROR_TRUNC";
    case CAHUTE_ERROR_INVALID:
        return "CAHUTE_ERROR_INVALID";
    case CAHUTE_ERROR_INCOMPAT:
        return "CAHUTE_ERROR_INCOMPAT";
    case CAHUTE_ERROR_TERMINATED:
        return "CAHUTE_ERROR_TERMINATED";
    case CAHUTE_ERROR_NOT_FOUND:
        return "CAHUTE_ERROR_NOT_FOUND";
    case CAHUTE_ERROR_TOO_MANY:
        return "CAHUTE_ERROR_TOO_MANY";
    case CAHUTE_ERROR_GONE:
        return "CAHUTE_ERROR_GONE";
    case CAHUTE_ERROR_TIMEOUT_START:
        return "CAHUTE_ERROR_TIMEOUT_START";
    case CAHUTE_ERROR_TIMEOUT:
        return "CAHUTE_ERROR_TIMEOUT";
    case CAHUTE_ERROR_CORRUPT:
        return "CAHUTE_ERROR_CORRUPT";
    case CAHUTE_ERROR_IRRECOV:
        return "CAHUTE_ERROR_IRRECOV";
    case CAHUTE_ERROR_NOOW:
        return "CAHUTE_ERROR_NOOW";
    default:
        return "(unknown)";
    }
}

#if WIN32_ENABLED
# include <windows.h>

CAHUTE_EXTERN(int) cahute_sleep(unsigned long ms) {
    Sleep(ms);
    return CAHUTE_OK;
}

CAHUTE_EXTERN(int) cahute_monotonic(unsigned long *msp) {
    *msp = GetTickCount();
    return CAHUTE_OK;
}

#elif POSIX_ENABLED

CAHUTE_EXTERN(int) cahute_sleep(unsigned long ms) {
    struct timespec requested_timestamp;

    requested_timestamp.tv_sec = ms / 1000;
    requested_timestamp.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&requested_timestamp, NULL);
    return CAHUTE_OK;
}

CAHUTE_EXTERN(int) cahute_monotonic(unsigned long *msp) {
    struct timespec res;
    int ret;

    ret = clock_gettime(
# ifdef CLOCK_BOOTTIME
        CLOCK_BOOTTIME,
# else
        CLOCK_MONOTONIC,
# endif
        &res
    );

    if (ret) {
        msg(ll_error,
            "An error occurred while calling clock_gettime(): %s (%d)",
            strerror(errno),
            errno);
        return CAHUTE_ERROR_UNKNOWN;
    }

    *msp = (unsigned long)res.tv_sec * 1000
           + (unsigned long)res.tv_nsec / 1000000;
    return CAHUTE_OK;
}

#else

CAHUTE_EXTERN(int) cahute_sleep(unsigned long ms) {
    CAHUTE_RETURN_IMPL("No method available for sleeping.");
}

CAHUTE_EXTERN(int) cahute_monotonic(unsigned long *msp) {
    CAHUTE_RETURN_IMPL("No method available for getting monotonic time.");
}

#endif
