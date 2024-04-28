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

#ifndef CAHUTE_LOGGING_H
#define CAHUTE_LOGGING_H 1
#include "cdefs.h"

CAHUTE_BEGIN_NAMESPACE
CAHUTE_BEGIN_DECLS

typedef void(cahute_log_func)(
    void *cahute__cookie,
    int cahute__level,
    char const *cahute__func,
    char const *cahute__message
);

CAHUTE_EXTERN(int) cahute_get_log_level(void);
CAHUTE_EXTERN(void) cahute_set_log_level(int cahute__level);

CAHUTE_EXTERN(int)
cahute_set_log_func(cahute_log_func *cahute__func, void *cahute__cookie);
CAHUTE_EXTERN(void) cahute_reset_log_func(void);

CAHUTE_END_DECLS
CAHUTE_END_NAMESPACE

#endif /* CAHUTE_LOGGING_H */
