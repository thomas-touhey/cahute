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

#if defined(__WINDOWS__)
# include <windows.h>

CAHUTE_EXTERN(int) cahute_sleep(unsigned long ms) {
    Sleep(ms);
    return CAHUTE_OK;
}

CAHUTE_EXTERN(int) cahute_monotonic(unsigned long *msp) {
    *msp = GetTickCount();
    return CAHUTE_OK;
}

#elif UNIX_ENABLED
# include <unistd.h>
# include <time.h>

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

    *msp = res.tv_sec * 1000 + res.tv_nsec / 1000000;
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
