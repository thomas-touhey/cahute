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
 * @param datap Pointer to the data to allocate.
 * @param group Name of the group of the file to decode.
 * @param group_size Size of the name of the group of the file to decode.
 * @param directory Name of the directory of the file to decode.
 * @param directory_size Size of the name of the directory of the file
 *        to decode.
 * @param name Name of the file to decode.
 * @param name_size Size of the name of the file to decode.
 * @param content Content of the file to decode.
 * @param content_size Size of the content of the file to decode.
 * @param data_type Type of the data to decode.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_mcs_decode_data(
    cahute_data **datap,
    cahute_u8 const *group,
    size_t group_size,
    cahute_u8 const *directory,
    size_t directory_size,
    cahute_u8 const *name,
    size_t name_size,
    cahute_u8 const *content,
    size_t content_size,
    int data_type
) {
    if (data_type == DATA_TYPE_PROGRAM) {
        /* We have a program. */
        if (content_size < 10) {
            msg(ll_error, "Expected at least 10 bytes!");
            return CAHUTE_ERROR_UNKNOWN;
        }

        return cahute_create_program(
            datap,
            CAHUTE_TEXT_ENCODING_FONTCHARACTER_VARIABLE,
            name,
            name_size,
            content,
            strnlen((char const *)content, 8),
            &content[10],
            content_size - 10
        );
    }

    /* TODO */
    msg(ll_info, "Data Type: %02X", data_type);
    msg(ll_info, "Directory Name: %.*s", directory_size, directory);
    msg(ll_info, "Data Name: %.*s", name_size, name);
    msg(ll_info, "Group Name: %.*s", group_size, group);
    msg(ll_info, "Raw file contents:");
    mem(ll_info, content, content_size);

    CAHUTE_RETURN_IMPL("MCS file not implemented.");
}
