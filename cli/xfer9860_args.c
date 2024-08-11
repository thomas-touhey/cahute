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
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "options.h"

static char const about_message[] =
    "xfer9860 - from Cahute v" CAHUTE_VERSION
    " (licensed under CeCILL 2.1)\n"
    "\n"
    "This utility is a reimplementation of the utility originally made\n"
    "by Andreas Bertheussen, Manuel Naranjo and Bruno L. Alata in 2007.\n"
    "\n"
    "This is free software; see the source for copying conditions.\n"
    "There is NO warranty; not even for MERCHANTABILITY or\n"
    "FITNESS FOR A PARTICULAR PURPOSE.\n";

static char const help_message[] =
    "Usage: %s [-h] [-a] [-t <throttle>] ...\n"
    "fx-9860G (SD) communication utility.\n"
    "\n"
    "Usage:\n"
    "    xfer9860 -u <local file path> <file name>\n"
    "        Upload the file as <file name> on the calculator's main\n"
    "        storage device.\n"
    "\n"
    "    xfer9860 -d <file name> <local file path>\n"
    "        Download the file named <file name> from the calculator's\n"
    "        main storage device.\n"
    "\n"
    "    xfer9860 -i\n"
    "        Show information about the connected calculator.\n"
    "\n"
    "    xfer9860 -o\n"
    "        Optimize the calculator's main storage device.\n"
    "\n"
    "Available options are:\n"
    "    -h             Show this help message and exit.\n"
    "    -a             Show the about message and exit.\n"
    "    -t <throttle>  Select the throttle in seconds, i.e. maximum\n"
    "                   delay between two packets.\n"
    "\n"
    "For guides, topics and reference, consult the documentation:\n"
    "    " CAHUTE_URL
    "\n"
    "\n"
    "For reporting issues and vulnerabilities, consult the following guide:\n"
    "    " CAHUTE_ISSUES_URL "\n";

/**
 * Short options definitions.
 */
static struct short_option const short_options[] = {
    {'h', 0},
    {'a', 0},
    {'t', OPTION_FLAG_PARAMETER_REQUIRED},
    {'u', OPTION_FLAG_PARAMETER_REQUIRED},
    {'d', OPTION_FLAG_PARAMETER_REQUIRED},
    {'i', 0},
    {'o', 0},

    SHORT_OPTION_SENTINEL
};

/**
 * Long options definitions.
 */
static struct long_option const long_options[] = {LONG_OPTION_SENTINEL};

/**
 * Parse the command-line parameters into a parsed arguments structure.
 *
 * @param argc Argument count.
 * @param argv Argument values.
 * @param args Parsed arguments structure to define.
 * @return 1 if the parameters have been parsed successfully, 0 otherwise.
 */
int parse_args(int argc, char **argv, struct args *args) {
    struct option_parser_state state;
    char const *command_path = argv[0];
    char *optarg;
    int option, optopt, about = 0, help = 0, multiple_operations = 0, err;

    args->operation = 0;
    args->throttle = 0;
    args->distant_source_name = NULL;
    args->distant_target_name = NULL;
    args->local_source_path = NULL;
    args->local_target_path = NULL;
    args->local_source_file = NULL;

    init_option_parser(
        &state,
        GETOPT_STYLE_POSIX,
        short_options,
        long_options,
        argc,
        argv
    );
    while (parse_next_option(&state, &option, &optopt, NULL, &optarg)) {
        switch (option) {
        case 't':
            /* Original command used 'atoi()' here. */
            args->throttle = atoi(optarg);
            break;

        case 'a':
            about = 1;
            break;

        case 'h':
            help = 1;
            break;

        case 'u':
            /* Upload mode! */
            if (args->operation)
                multiple_operations = 1;

            args->operation = OPERATION_UPLOAD;
            args->distant_target_name = optarg;
            break;

        case 'd':
            /* Download mode! */
            if (args->operation)
                multiple_operations = 1;

            args->operation = OPERATION_DOWNLOAD;
            args->distant_source_name = optarg;
            break;

        case 'o':
            /* Optimize mode! */
            if (args->operation)
                multiple_operations = 1;

            args->operation = OPERATION_OPTIMIZE;
            break;

        case 'i':
            /* Information mode! */
            if (args->operation)
                multiple_operations = 1;

            args->operation = OPERATION_INFO;
            break;

        default:
            /* xfer9860 abandons when it receives an unknown option.
             * We want to display the help message! */
            help = 1;
            goto process_params;
        }
    }

process_params:
    update_positional_parameters(&state, &argc, &argv);

    if (about) {
        fprintf(stderr, about_message);
        return 0;
    }

    if (multiple_operations || !args->operation)
        help = 1;
    else if (args->operation == OPERATION_UPLOAD) {
        if (argc != 1)
            help = 1;
        else
            args->local_source_path = argv[0];
    } else if (args->operation == OPERATION_DOWNLOAD) {
        if (argc != 1)
            help = 1;
        else
            args->local_target_path = argv[0];
    } else {
        if (argc != 0)
            help = 1;
    }

    if (help) {
        fprintf(stderr, help_message, command_path);
        return 0;
    }

    if (args->distant_target_name
        && portable_strnlen(args->distant_target_name, 13) > 12) {
        fprintf(
            stderr,
            "The destination filename is too long: %s\n"
            "Filesystem only supports 12 characters.",
            args->distant_target_name
        );
        return 0;
    }

    if (args->distant_source_name
        && portable_strnlen(args->distant_source_name, 13) > 12) {
        fprintf(
            stderr,
            "The source filename is too long: %s\n"
            "Filesystem only supports 12 characters.",
            args->distant_source_name
        );
        return 0;
    }

    if (args->local_source_path) {
        err = cahute_open_file_for_reading(
            &args->local_source_file,
            args->local_source_path,
            CAHUTE_PATH_TYPE_CLI
        );
        if (err) {
            fprintf(
                stderr,
                "Unable to open file: %s\n",
                args->local_source_path
            );
            return 0;
        }
    }

    return 1;
}
