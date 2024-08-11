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

#include "xfer9860.h"
#include "common.h"

static char const error_notfound[] =
    "Could not connect to the calculator.\n"
    "- Is it plugged in and in receive mode?\n"
    "- Have you tried changing the cable?\n";

/**
 * Display device information.
 *
 * @param link Link to use to obtain the information.
 * @return Cahute error.
 */
static int print_device_info(cahute_link *link) {
    unsigned long capacity;
    int err;

    if ((err = cahute_request_storage_capacity(link, "fls0", &capacity)))
        return err;

    /* 1572864 is a special value taken from xfer9860.
     * It may not be accurate for the device. */
    printf(
        "Storage memory: %lu%% (%luo) available.\n",
        100 * capacity / 1572864,
        capacity
    );

    return 0;
}

/**
 * Main entry point to the program.
 *
 * @param argc Argument count.
 * @param argv Argument values.
 * @return Exit code for the program.
 */
int main(int argc, char **argv) {
    struct args args;
    cahute_link *link = NULL;
    int err = 0, ret;

    /* Since xfer9860 has no logging level of any kind, we disable logging
     * entirely here. */
    cahute_set_log_level(CAHUTE_LOGLEVEL_NONE);

    if (!parse_args(argc, argv, &args))
        return 0;
    if ((err = cahute_open_simple_usb_link(&link, 0)))
        goto fail;

    switch (args.operation) {
    case OPERATION_UPLOAD:
        err = cahute_send_file_to_storage(
            link,
            CAHUTE_SEND_FILE_FLAG_FORCE | CAHUTE_SEND_FILE_FLAG_OPTIMIZE,
            NULL,
            args.distant_target_name,
            "fls0",
            args.local_source_file,
            NULL,
            NULL,
            NULL,
            NULL
        );
        break;

    case OPERATION_DOWNLOAD:
        err = cahute_request_file_from_storage(
            link,
            NULL,
            args.distant_source_name,
            "fls0",
            args.local_target_path,
            CAHUTE_PATH_TYPE_CLI,
            NULL,
            NULL
        );
        break;

    case OPERATION_OPTIMIZE:
        err = cahute_optimize_storage(link, "fls0");
        break;

    case OPERATION_INFO:
        err = print_device_info(link);
        break;

    default:
        err = CAHUTE_ERROR_IMPL;
        break;
    }

fail:
    if (link)
        cahute_close_link(link);
    if (args.local_source_file)
        cahute_close_file(args.local_source_file);

    ret = 1;
    switch (err) {
    case 0:
        ret = 0;
        break;

    case CAHUTE_ERROR_ABORT:
        break;

    case CAHUTE_ERROR_IMPL:
        fprintf(stderr, "The operation was not implemented yet.\n");
        break;

    case CAHUTE_ERROR_TOO_MANY:
        fprintf(stderr, "Too many found calculators, please only keep one.\n");
        break;

    case CAHUTE_ERROR_NOT_FOUND:
        fprintf(stderr, error_notfound);
        break;

    default:
        fprintf(stderr, "An unknown error has occurred.\n");
    }

    return ret;
}
