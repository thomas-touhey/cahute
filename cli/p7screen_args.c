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
#include <getopt.h>

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
 * Short options for getopt_long().
 */
char const *short_options = "hvz:l:";

/**
 * Long options for getopt_long().
 */
struct option const long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {"zoom", required_argument, NULL, 'z'},
    {"log", required_argument, NULL, 'l'},
    {NULL, 0, NULL, 0}
};

/**
 * Parse command-line parameters, and handle help and version messages.
 *
 * Note that since we use getopt_long(), the argv array is actually
 * reorganized to move positional parameters at the end of the array, hence
 * why argv is of "char **" type, and not "char const * const *".
 *
 * @param argc Argument count, as provided to main().
 * @param argv Argument values, as provided to main().
 * @param args Parsed argument structure to feed for use by the caller.
 * @return Whether parameters were successfully parsed (1), or not (0).
 */
int parse_args(int argc, char **argv, struct args *args) {
    int help = 0, zoom, option;

    /* Default parsed arguments. */
    args->zoom = DEFAULT_ZOOM;

    opterr = 0;

    while (1) {
        option = getopt_long(argc, argv, short_options, long_options, NULL);
        if (option < 0)
            break;

        switch (option) {
        case 'h':
            /* -h, --help: display the help message and quit. */
            help = 1;
            break;

        case 'v':
            /* -v, --version: display the version message and quit. */
            puts(version_message);
            return 0;

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

        case '?':
            /* Erroneous option usage. */
            if (optopt == 'z')
                fprintf(stderr, "-z, --zoom: expected an argument\n");
            else
                /* We ignore unknown options. */
                break;

            return 0;
        }
    }

    /* p7screen is used without parameters.
        * If there is any, we want to print the help and quit. */
    if (argc - optind)
        help = 1;

    /* If we want to display the help message, do it here! */
    if (help) {
        printf(help_message, argv[0], get_current_log_level(), DEFAULT_ZOOM);
        return 0;
    }

    return 1;
}
