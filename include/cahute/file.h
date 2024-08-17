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

#ifndef CAHUTE_FILE_H
#define CAHUTE_FILE_H 1
#include "cdefs.h"
#include "data.h"
#include "path.h"

CAHUTE_BEGIN_NAMESPACE
CAHUTE_DECLARE_TYPE(cahute_file)

#define CAHUTE_FILE_TYPE_UNKNOWN    0
#define CAHUTE_FILE_TYPE_ADDIN_CG   1
#define CAHUTE_FILE_TYPE_ADDIN_FX   2
#define CAHUTE_FILE_TYPE_BITMAP     4
#define CAHUTE_FILE_TYPE_CASIOLINK  8
#define CAHUTE_FILE_TYPE_CTF        16
#define CAHUTE_FILE_TYPE_EACT_FX    32
#define CAHUTE_FILE_TYPE_FKEY_FX    64
#define CAHUTE_FILE_TYPE_FKEY_CG    128
#define CAHUTE_FILE_TYPE_FXPROGRAM  256
#define CAHUTE_FILE_TYPE_GIF        512
#define CAHUTE_FILE_TYPE_LANG_CG    1024
#define CAHUTE_FILE_TYPE_LANG_FX    2048
#define CAHUTE_FILE_TYPE_MAINMEM    4096
#define CAHUTE_FILE_TYPE_PICTURE_CG 8192
#define CAHUTE_FILE_TYPE_PICTURE_CP 16384

CAHUTE_BEGIN_DECLS

CAHUTE_EXTERN(int)
cahute_open_file_for_reading(
    cahute_file **cahute__filep,
    unsigned long cahute__flags,
    void const *cahute__path,
    int cahute__path_type
);

CAHUTE_EXTERN(int)
cahute_open_file_for_export(
    cahute_file **cahute__filep,
    unsigned long cahute__size,
    void const *cahute__path,
    int cahute__path_type
);

CAHUTE_EXTERN(int) cahute_open_stdout(cahute_file **cahute__filep);

CAHUTE_EXTERN(int)
cahute_get_file_size(cahute_file *cahute__file, unsigned long *cahute__sizep);

CAHUTE_EXTERN(int)
cahute_read_from_file(
    cahute_file *cahute__file,
    unsigned long cahute__offset,
    void *cahute__buf,
    size_t cahute__size
);

CAHUTE_EXTERN(int)
cahute_write_to_file(
    cahute_file *cahute__file,
    unsigned long cahute__offset,
    void const *cahute__data,
    size_t cahute__data_size
);

CAHUTE_EXTERN(int)
cahute_guess_file_type(
    cahute_file *cahute__file,
    unsigned long *cahute__typep
);

CAHUTE_EXTERN(int)
cahute_get_data_from_file(
    cahute_file *cahute__file,
    cahute_data **cahute__datap
);

CAHUTE_EXTERN(void) cahute_close_file(cahute_file *cahute__filep);

CAHUTE_END_DECLS
CAHUTE_END_NAMESPACE

#endif /* CAHUTE_FILE_H */
