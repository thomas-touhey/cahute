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

#include "cas.h"

/**
 * Read data.
 *
 * @param args Parsed command-line to use to read.
 * @param datap Pointer to the data to read from the input.
 * @return Return code.
 */
CAHUTE_LOCAL(int) read_data(struct args const *args, cahute_data **datap) {
    cahute_link *link;
    int err, ret = 1;

    switch (args->in.type) {
    case MEDIUM_COM:
        err = cahute_open_serial_link(
            &link,
            CAHUTE_SERIAL_RECEIVER | args->in.data.com.serial_flags,
            args->in.path,
            args->in.data.com.serial_speed
        );
        if (err) {
            fprintf(stderr, "Could not open the serial link (0x%04X).\n", err);
            goto fail;
        }

        while (1) {
            err = cahute_receive_data(link, datap, 0);
            if (!err) {
                /* Attach the received data to the existing data, and go
                 * to the end of the received data. */
                for (; *datap; datap = &(*datap)->cahute_data_next)
                    ;

                continue;
            }

            *datap = NULL;
            if (err == CAHUTE_ERROR_TERMINATED)
                break;

            fprintf(stderr, "Could not receive data (0x%04X).\n", err);
            cahute_close_link(link);
            goto fail;
        }

        cahute_close_link(link);
        ret = 0;
        break;
    }

fail:
    return ret;
}

/**
 * List data types.
 *
 * @param args Parsed arguments.
 * @param data Data to list.
 * @return Return code.
 */
CAHUTE_LOCAL(int)
list_data_types(struct args const *args, cahute_data const *data) {
    /* TODO: Use character conversion to ensure that we have
     * terminal-compatible strings here. */
    printf("\n");
    for (; data; data = data->cahute_data_next) {
        switch (data->cahute_data_type) {
        case CAHUTE_DATA_TYPE_PROGRAM: {
            size_t name_size =
                data->cahute_data_content.cahute_data_content_program
                    .cahute_data_content_program_name_size;
            size_t program_size =
                data->cahute_data_content.cahute_data_content_program
                    .cahute_data_content_program_size;

            printf(
                name_size ? "%zu bytes \tProgram \"%.*s\".\n"
                          : "%zu bytes \tProgram.\n",
                program_size,
                (int)name_size,
                (char const *)
                    data->cahute_data_content.cahute_data_content_program
                        .cahute_data_content_program_name
            );
        } break;

        default:
            printf("  UNKNOWN TYPE %d\n", data->cahute_data_type);
        }
    }

    return 0;
}

/**
 * List data.
 *
 * @param args Parsed arguments.
 * @param data Data to list.
 * @return Return code.
 */
CAHUTE_LOCAL(int) list_data(struct args const *args, cahute_data const *data) {
    /* TODO */
    return 0;
}

/**
 * Write data.
 *
 * @param args Parsed arguments.
 * @param data Data to list.
 * @return Return code.
 */
CAHUTE_LOCAL(int)
write_data(struct args const *args, cahute_data const *data) {
    /* TODO */
    fprintf(stderr, "Output not implemented.\n");
    return 1;
}

/**
 * Main function.
 *
 * @param ac Argument count.
 * @param av Argument values.
 */
int main(int ac, char **av) {
    struct args args;
    cahute_data *data = NULL;
    int ret = 1;

    if (!parse_args(ac, av, &args))
        return 0;

    ret = read_data(&args, &data);
    if (ret)
        goto end;

    /* TODO: Operate 'before' conversions here. */

    if (args.should_list_types) {
        ret = list_data_types(&args, data);
        if (ret)
            goto end;
    }

    if (args.should_list_files) {
        ret = list_data(&args, data);
        if (ret)
            goto end;
    }

    /* TODO: Operate 'after' conversions here. */

    if (args.should_output) {
        /* TODO: Original CaS actually decides automatic output type here:
         * - If one 'raw' file, use the '.bin' format;
         * - If one screenshot ('SSMono' or 'SSCol'), use the '.bmp' format;
         * - Otherwise, use the '.ctf' format. */

        ret = write_data(&args, data);
        if (ret)
            goto end;
    }

    ret = 0;

end:
    cahute_destroy_data(data);
    free_args(&args);
    return ret;
}
