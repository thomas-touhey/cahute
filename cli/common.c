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
#include <string.h>

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
