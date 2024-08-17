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
#define DATA_TYPE_PROGRAM 0x01

/**
 * Decode an MCS file.
 *
 * @param final_datap Pointer to the data to allocate.
 * @param group Name of the group of the file to decode.
 * @param group_size Size of the name of the group of the file to decode.
 * @param directory Name of the directory of the file to decode.
 * @param directory_size Size of the name of the directory of the file
 *        to decode.
 * @param name Name of the file to decode.
 * @param name_size Size of the name of the file to decode.
 * @param file File from which to read content.
 * @param content_offset Offset from which to read the file content.
 * @param content_size Size of the content to read from the file.
 * @param data_type Type of the data to decode.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_mcs_decode_data(
    cahute_data **final_datap,
    cahute_u8 const *group,
    size_t group_size,
    cahute_u8 const *directory,
    size_t directory_size,
    cahute_u8 const *name,
    size_t name_size,
    cahute_file *file,
    unsigned long content_offset,
    size_t content_size,
    int data_type
) {
    cahute_data *data = NULL;
    cahute_data **datap = &data;
    cahute_u8 const *p;
    int err;

    /* For the group, directory and name, we want to ensure that there is no
     * 0xFF or 0x00 sentinel in the strings. */
    if (group_size && (p = memchr(group, 0xFF, group_size)))
        group_size = (size_t)(p - (cahute_u8 const *)group);
    if (group_size && (p = memchr(group, 0x00, group_size)))
        group_size = (size_t)(p - (cahute_u8 const *)group);

    if (directory_size && (p = memchr(directory, 0xFF, directory_size)))
        directory_size = (size_t)(p - (cahute_u8 const *)directory);
    if (directory_size && (p = memchr(directory, 0x00, directory_size)))
        directory_size = (size_t)(p - (cahute_u8 const *)directory);

    if (name_size && (p = memchr(name, 0xFF, name_size)))
        name_size = (size_t)(p - (cahute_u8 const *)name);
    if (name_size && (p = memchr(name, 0x00, name_size)))
        name_size = (size_t)(p - (cahute_u8 const *)name);

    msg(ll_info, "Data Type: 0x%02X", data_type);
    msg(ll_info, "Directory Name: %.*s", directory_size, directory);
    msg(ll_info, "Data Name: %.*s", name_size, name);
    msg(ll_info, "Group Name: %.*s", group_size, group);

    if (data_type == DATA_TYPE_PROGRAM) {
        cahute_u8 program_header[10];

        /* We have a program. */
        if (content_size < 10) {
            msg(ll_error, "Expected at least 10 bytes!");
            return CAHUTE_ERROR_UNKNOWN;
        }

        err = cahute_read_from_file(file, content_offset, program_header, 10);
        if (err)
            return err;

        err = cahute_create_program_from_file(
            datap,
            CAHUTE_TEXT_ENCODING_9860_8,
            name,
            name_size,
            program_header,
            8,
            file,
            content_offset + 10,
            content_size - 10
        );
        if (err)
            return err;

        goto data_ready;
    }

    /* TODO */
    CAHUTE_RETURN_IMPL("MCS file not implemented.");

data_ready:
    while (*datap)
        datap = &(*datap)->cahute_data_next;

    *datap = *final_datap;
    *final_datap = data;
    return CAHUTE_OK;
}
