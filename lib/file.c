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
#define EXAMINE(CAHUTE_FILE) \
    { \
        if (~(CAHUTE_FILE)->flags & CAHUTE_FILE_FLAG_EXAMINED) { \
            int examine_err = cahute_examine_file((CAHUTE_FILE)); \
\
            if (examine_err) \
                return examine_err; \
        } \
    } \
    (void)0

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
    cahute_u8 buf[32], *p, *q;
    cahute_u8 std[32];
    int err;

    /* Read the first 32 (0x20) bytes to try to find a StandardHeader. */
    err = cahute_read_from_file(file, 0, buf, 32);
    if (err == CAHUTE_ERROR_TRUNC)
        goto examined;
    else if (err)
        return err;

    /* We can try to determine a standard header. */
    for (p = &buf[31], q = &std[31]; p >= buf; p--, q--)
        *q = ~*p & 255;

    if (!memcmp(std, "USBPower\x62\0\x10\0\x10\0", 14)    /* G1M, G1R */
        || !memcmp(std, "USBPower\x31\0\x10\0\x10\0", 14) /* G2M, G2R */
        || !memcmp(std, "USBPower\x75\0\x10\0\x10\0", 14) /* G3M, G3R */
    ) {
        file->type = CAHUTE_FILE_TYPE_MAINMEM;
        goto examined;
    }

    /* We can try to see if we have a CASIOLINK archive. */
    if (buf[0] == 0x3A) {
        file->type = CAHUTE_FILE_TYPE_CASIOLINK;
        goto examined;
    }

    /* TODO: There are many, many more file types to test for. */

    /* TODO: By compatibility with CaS, the following extensions must be
     * matched:
     *
     * - '.ctf', '.txt': CTF;
     * - '.fxp': FX-Program;
     * - '.bmp': Bitmap;
     * - '.gif': GIF. */

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
CAHUTE_EXTERN(int)
cahute_guess_file_type(cahute_file *file, unsigned long *typep) {
    *typep = CAHUTE_FILE_TYPE_UNKNOWN;

    EXAMINE(file);

    if (!file->type)
        return CAHUTE_ERROR_NOT_FOUND;

    *typep = file->type;
    return CAHUTE_OK;
}

/* ---
 * Decode data from a file.
 * --- */

/**
 * Get data from a CASIOLINK main memory archive.
 *
 * @param file File object.
 * @param final_datap Pointer to the data to create with the read data.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_get_data_from_casiolink_file(
    cahute_file *file,
    cahute_data **final_datap
) {
    unsigned long offset = 0, file_size = 0;
    cahute_data *data = NULL;
    cahute_data **datap = &data;
    int err;

    err = cahute_get_file_size(file, &file_size);
    if (err)
        return err;

    while (offset < file_size) {
        err = cahute_casiolink_decode_data(
            datap,
            file,
            &offset,
            CAHUTE_CASIOLINK_VARIANT_AUTO,
            1
        );
        if (err && err != CAHUTE_ERROR_IMPL)
            goto fail;

        while (*datap)
            datap = &(*datap)->cahute_data_next;
    }

    if (data) {
        *datap = *final_datap;
        *final_datap = data;
    }

    return CAHUTE_OK;

fail:
    cahute_destroy_data(data);
    return err;
}

/**
 * Get data from a standard main memory archive.
 *
 * @param file File object.
 * @param final_datap Pointer to the data to create with the read data.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_get_data_from_mainmem_file(
    cahute_file *file,
    cahute_data **final_datap
) {
    cahute_u8 header[32];
    cahute_u8 group_header[20];
    cahute_u8 file_header[24];
    cahute_data *data = NULL;
    cahute_data **datap = &data;
    unsigned long count, group_count = 0, offset = 0;
    int err;

    err = cahute_read_from_file(file, 0, header, sizeof(header));
    if (err)
        goto fail;

    offset = 32;
    count = ((~header[30] & 255) << 8) | (~header[31] & 255);
    while (count) {
        err = cahute_read_from_file(
            file,
            offset,
            group_header,
            sizeof(group_header)
        );
        if (err)
            goto fail;

        offset += sizeof(group_header);
        group_count = (group_header[16] << 24) | (group_header[17] << 16)
                      | (group_header[18] << 8) | group_header[19];

        msg(ll_info, "(0x%04lX) Group header:", offset - sizeof(group_header));
        mem(ll_info, group_header, sizeof(group_header));

        for (; group_count; group_count--) {
            unsigned long data_size;

            err = cahute_read_from_file(
                file,
                offset,
                file_header,
                sizeof(file_header)
            );
            if (err)
                goto fail;

            offset += sizeof(file_header);
            data_size = (file_header[17] << 24) | (file_header[18] << 16)
                        | (file_header[19] << 8) | file_header[20];

            msg(ll_info, "File header:");
            mem(ll_info, file_header, sizeof(file_header));
            msg(ll_info, "  Data size: %" CAHUTE_PRIuSIZE, data_size);

            err = cahute_mcs_decode_data(
                datap,
                group_header,
                16,
                file_header,
                8,
                &file_header[8],
                8,
                file,
                offset,
                data_size,
                file_header[16]
            );
            offset += data_size;
            count--;

            if (err == CAHUTE_ERROR_IMPL)
                continue;
            else if (err)
                goto fail;

            while (*datap)
                datap = &(*datap)->cahute_data_next;
        }
    }

    if (data) {
        *datap = *final_datap;
        *final_datap = data;
    }

    return CAHUTE_OK;

fail:
    cahute_destroy_data(data);
    return err;
}

/**
 * Get data from the file.
 *
 * @param file File object.
 * @param datap Pointer to the data to create with the read data.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_get_data_from_file(cahute_file *file, cahute_data **datap) {
    cahute_data *data = NULL;
    int err;

    EXAMINE(file);

    switch (file->type) {
    case CAHUTE_FILE_TYPE_CASIOLINK:
        err = cahute_get_data_from_casiolink_file(file, &data);
        if (err)
            goto fail;

        break;

    case CAHUTE_FILE_TYPE_MAINMEM:
        err = cahute_get_data_from_mainmem_file(file, &data);
        if (err)
            goto fail;

        break;

    default:
        msg(ll_error,
            "Invalid file type 0x%02X for extracting data from the file.",
            file->type);
        return CAHUTE_ERROR_INVALID;
    }

    *datap = data;
    return CAHUTE_OK;

fail:
    cahute_destroy_data(data);
    return err;
}
