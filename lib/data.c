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
 * Create a program.
 *
 * @param datap Pointer to the data to allocate.
 * @param encoding Text encoding used for the name, password and content.
 * @param name Name of the program.
 * @param name_size Size of the program name.
 * @param password Password of the program.
 * @param password_size Size of the password program.
 * @param content Content of the program.
 * @param size Size of the program content.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_create_program(
    cahute_data **datap,
    int encoding,
    void const *name,
    size_t name_size,
    void const *password,
    size_t password_size,
    void const *content,
    size_t size
) {
    cahute_data *data;
    struct cahute__data_content_program *program;
    cahute_u8 *buf;

    data = malloc(sizeof(cahute_data) + name_size + password_size + size + 12);
    if (!data)
        return CAHUTE_ERROR_ALLOC;

    buf = (cahute_u8 *)&data[1];

    data->cahute_data_next = NULL;
    data->cahute_data_type = CAHUTE_DATA_TYPE_PROGRAM;

    program = &data->cahute_data_content.cahute_data_content_program;
    program->cahute_data_content_program_encoding = encoding;
    program->cahute_data_content_program_name_size = name_size;
    program->cahute_data_content_program_password_size = password_size;
    program->cahute_data_content_program_size = size;

    if (name_size) {
        program->cahute_data_content_program_name = buf;

        if (name)
            memcpy(buf, name, name_size);
        else
            memset(buf, 0, name_size);

        buf[name_size] = 0;
        buf[name_size + 1] = 0;
        buf[name_size + 2] = 0;
        buf[name_size + 3] = 0;

        buf += name_size + 4;
    } else
        program->cahute_data_content_program_name = NULL;

    if (password_size) {
        program->cahute_data_content_program_password = buf;

        if (password)
            memcpy(buf, password, password_size);
        else
            memset(buf, 0, password_size);

        buf[password_size] = 0;
        buf[password_size + 1] = 0;
        buf[password_size + 2] = 0;
        buf[password_size + 3] = 0;

        buf += password_size + 4;
    } else
        program->cahute_data_content_program_password = NULL;

    program->cahute_data_content_program_content = buf;

    if (size) {
        if (content)
            memcpy(buf, content, size);
        else
            memset(buf, 0, size);

        buf += size;
    }

    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;

    *datap = data;
    return CAHUTE_OK;
}

/**
 * Destroy / deallocate data.
 *
 * @param data Data to deallocate.
 */
CAHUTE_EXTERN(void) cahute_destroy_data(cahute_data *data) {
    while (data) {
        cahute_data *to_free = data;

        data = data->cahute_data_next;
        free(to_free);
    }
}
