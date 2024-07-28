/* ****************************************************************************
 * Copyright (C) 2016-2017, 2024 Thomas Touhey <thomas@touhey.fr>
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

#include "p7screen.h"
#include <stdlib.h>
#include <ctype.h>
#include "options.h"

static char const version_message[] =
    "p7screen - from Cahute v" CAHUTE_VERSION
    " (licensed under CeCILL 2.1)\n"
    "\n"
    "This is free software; see the source for copying conditions.\n"
    "There is NO warranty; not even for MERCHANTABILITY or\n"
    "FITNESS FOR A PARTICULAR PURPOSE.";

static char const help_message[] =
    "Usage: %s\n"
    "          [--help|-h] [--version|-v]\n"
    "\n"
    "Displays the streamed screen from a CASIO calculator connected by USB.\n"
    "\n"
    "Options are:\n"
    "  -h, --help        Display this help page\n"
    "  -v, --version     Displays the version\n"
    "  -l, --log <level> Logging level to set (default: %s).\n"
    "                    One of: info, warning, error, fatal, none.\n"
    "  --com <device>    Path or name of the serial device with which to\n"
    "                    communicate. If this option isn't used, the\n"
    "                    program will use USB to find the calculator.\n "
    " --use <settings>  Serial settings to use, when the link is established\n"
    "                    over a serial link (i.e. when used with `--com`).\n"
    "                    For example, \"9600N2\" represents 9600 bauds, no\n"
    "                    parity, and two stop bits.\n"
    "  -z, --zoom <zoom> Change the zoom (1 to 16)\n"
    "                    By default, the zoom will be %d.\n"
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
    {'v', 0},
    {'z', OPTION_FLAG_PARAMETER_REQUIRED},
    {'l', OPTION_FLAG_PARAMETER_REQUIRED},

    SHORT_OPTION_SENTINEL
};

/**
 * Long options definitions.
 */
static struct long_option const long_options[] = {
    {"help", 0, 'h'},
    {"version", 0, 'v'},
    {"zoom", OPTION_FLAG_PARAMETER_REQUIRED, 'z'},
    {"com", OPTION_FLAG_PARAMETER_REQUIRED, 'c'},
    {"use", OPTION_FLAG_PARAMETER_REQUIRED, 'U'},
    {"log", OPTION_FLAG_PARAMETER_REQUIRED, 'l'},

    LONG_OPTION_SENTINEL
};

/**
 * Parse command-line parameters, and handle help and version messages.
 *
 * @param argc Argument count, as provided to main().
 * @param argv Argument values, as provided to main().
 * @param args Parsed argument structure to feed for use by the caller.
 * @return Whether parameters were successfully parsed (1), or not (0).
 */
int parse_args(int argc, char **argv, struct args *args) {
    struct option_parser_state state;
    char const *command = argv[0];
    int help = 0, zoom, err, option, optopt;
    char *optarg;

    /* Default parsed arguments. */
    args->zoom = DEFAULT_ZOOM;
    args->serial_flags = 0;
    args->serial_speed = 0;
    args->serial_name = NULL;

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
        case 'h':
            /* -h, --help: display the help message and quit. */
            help = 1;
            break;

        case 'v':
            /* -v, --version: display the version message and quit. */
            puts(version_message);
            return 0;

        case 'c':
            /* --com: set the serial port. */
            args->serial_name = optarg;
            break;

        case 'U':
            /* --use: use serial settings. */
            err = parse_serial_attributes(
                optarg,
                &args->serial_flags,
                &args->serial_speed
            );
            if (err) {
                fprintf(stderr, "-u, --use: invalid format!\n");
                return 0;
            }

            break;

        case 'z':
            /* --zoom: set the zoom as an integer between 1 and 16. */
            zoom = atoi(optarg);
            if (zoom < 1 || zoom > 16) {
                fprintf(stderr, "-z, --zoom: should be between 1 and 16\n");
                return 0;
            }

            args->zoom = zoom;
            break;

        case 'l':
            /* -l, --log: set the logging level. */
            set_log_level(optarg);
            break;

        case GETOPT_FAIL:
            /* Erroneous option usage. */
            if (optopt == 'z')
                fprintf(stderr, "-z, --zoom: expected an argument\n");
            else
                /* We ignore unknown options. */
                break;

            return 0;
        }
    }

    update_positional_parameters(&state, &argc, &argv);

    /* p7screen is used without parameters.
     * If there is any, we want to print the help and quit. */
    if (argc)
        help = 1;

    /* If we want to display the help message, do it here! */
    if (help) {
        printf(help_message, command, get_current_log_level(), DEFAULT_ZOOM);
        return 0;
    }

    return 1;
}
