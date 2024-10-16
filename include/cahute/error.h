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

#ifndef CAHUTE_ERROR_H
#define CAHUTE_ERROR_H 1
#include "cdefs.h"

CAHUTE_BEGIN_NAMESPACE
CAHUTE_BEGIN_DECLS

/* WARNING: When adding error codes to this file, you must also add:
 *
 * - The related documentation in ``docs/headers/cahute/error.rst``;
 * - The related entry in the ``cahute_get_error_name()`` definition
 *   in ``lib/misc.c``. */

#define CAHUTE_OK 0x0000 /* No error has occurred. */

#define CAHUTE_ERROR_UNKNOWN    0x0001 /* An unknown error has occurred. */
#define CAHUTE_ERROR_ABORT      0x0002 /* Operation was aborted by user. */
#define CAHUTE_ERROR_IMPL       0x0003 /* A feature was unimplemented. */
#define CAHUTE_ERROR_ALLOC      0x0004 /* A memory allocation has failed. */
#define CAHUTE_ERROR_PRIV       0x0005 /* Insufficient privileges were found. */
#define CAHUTE_ERROR_BUSY       0x0006 /* Device is currently busy. */
#define CAHUTE_ERROR_INT        0x0007 /* Interrupted by a callback. */
#define CAHUTE_ERROR_SIZE       0x0008 /* Some received data was too big. */
#define CAHUTE_ERROR_TRUNC      0x0009 /* Input was truncated. */
#define CAHUTE_ERROR_INVALID    0x000A /* Invalid data or behaviour. */
#define CAHUTE_ERROR_INCOMPAT   0x000B /* Incompatibility detected. */
#define CAHUTE_ERROR_TERMINATED 0x000C /* Input or link was terminated. */

#define CAHUTE_ERROR_NOT_FOUND     0x0101 /* Device could not be found. */
#define CAHUTE_ERROR_TOO_MANY      0x0102 /* Too Many Devices found. */
#define CAHUTE_ERROR_GONE          0x0103 /* Device is gone, I/O has failed. */
#define CAHUTE_ERROR_TIMEOUT_START 0x0104 /* A timeout has occurred on 1st. */
#define CAHUTE_ERROR_TIMEOUT       0x0105 /* A timeout has occurred on next. */
#define CAHUTE_ERROR_CORRUPT       0x0106 /* Corrupted packet. */
#define CAHUTE_ERROR_IRRECOV       0x0107 /* Irrecoverable link */

#define CAHUTE_ERROR_NOOW 0x0201 /* File was not overwritten. */

CAHUTE_EXTERN(char const *) cahute_get_error_name(int cahute__code);

CAHUTE_END_DECLS
CAHUTE_END_NAMESPACE

#endif /* CAHUTE_ERROR_H */
