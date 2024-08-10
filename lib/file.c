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
 * Get the size of the file referenced by a file object.
 *
 * @param file File object.
 * @param sizep Pointer to the size to set.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_get_file_size(cahute_file *file, unsigned long *sizep) {
    if (~file->medium.flags & CAHUTE_FILE_MEDIUM_FLAG_SIZE)
        CAHUTE_RETURN_IMPL("File does not support size computation.");

    *sizep = file->medium.file_size;
    return CAHUTE_OK;
}

/**
 * Read data from a file.
 *
 * @param file File object.
 * @param off Offset at which to read data.
 * @param buf Buffer in which to write the result.
 * @param size Size of the data to read.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_read_from_file(
    cahute_file *file,
    unsigned long off,
    void *buf,
    size_t size
) {
    return cahute_read_from_file_medium(&file->medium, off, buf, size);
}

/**
 * Write data to a file.
 *
 * @param file File object.
 * @param off Offset at which to write data.
 * @param data Data to write.
 * @param size Size of the data to write.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_write_to_file(
    cahute_file *file,
    unsigned long off,
    void const *data,
    size_t size
) {
    return cahute_write_to_file_medium(&file->medium, off, data, size);
}

/**
 * Determine the file type and store it within the file.
 *
 * @param file File object.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int) cahute_examine_file(cahute_file *file) {
    cahute_u8 buf[32];
    int err;

    /* Read the first 32 (0x20) bytes to try to find a StandardHeader. */
    err = cahute_read_from_file(file, 0, buf, 32);
    if (err == CAHUTE_ERROR_TRUNC)
        goto examined;
    else if (err)
        return err;

    if (buf[0] == 0x3A) {
        /* We can safely assume a CASIOLINK archive. */
        file->type = CAHUTE_FILE_TYPE_CASIOLINK;
        goto examined;
    }

    /* TODO: There are many, many more file types to test for. */

examined:
    file->flags |= CAHUTE_FILE_FLAG_EXAMINED;
    return CAHUTE_OK;
}

/**
 * Guess the file type.
 *
 * @param file File object.
 * @param typep Type to set with the found file type.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int) cahute_guess_file_type(cahute_file *file, int *typep) {
    if (~file->flags & CAHUTE_FILE_FLAG_EXAMINED) {
        int err = cahute_examine_file(file);

        if (err)
            return err;
    }

    if (!file->type)
        return CAHUTE_ERROR_NOT_FOUND;

    *typep = file->type;
    return CAHUTE_OK;
}
