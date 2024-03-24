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

#include "p7.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "options.h"

static char const version_message[] =
    "p7 - from Cahute v" CAHUTE_VERSION
    " (licensed under CeCILL 2.1)\n"
    "\n"
    "This is free software; see the source for copying conditions.\n"
    "There is NO warranty; not even for MERCHANTABILITY or\n"
    "FITNESS FOR A PARTICULAR PURPOSE.";

static char const help_main[] =
    "Usage: %s\n"
    "          [--version|-v] [--help|-h] [-l|--log <level>]\n"
    "          [--com <device>] [--use <params>] [--set <params>] [--reset]\n"
    "          [--no-init] [--no-exit]\n"
    "          <subcommand> [options...]\n"
    "\n"
    "Subcommands you can use are:\n"
    "   help          Display the help page of the command.\n"
    "   version       Display the version message.\n"
    "   list-devices  List available serial devices.\n"
    "   info          Get information about the calculator.\n"
    "   idle          Do nothing once the link is established.\n"
    "   send          Send a file to the calculator.\n"
    "   get           Get a file from the calculator.\n"
    "   copy          Copy a file into another on the calculator.\n"
    "   delete        Delete a file on the calculator.\n"
    "   list          List files on the distant filesystem.\n"
    "   reset         Reset the flash memory.\n"
    "   optimize      Optimize the distant filesystem.\n"
    "\n"
    "General options:\n"
    "  -h, --help        Display the help page of the (sub)command and quit.\n"
    "  -v, --version     Display the version message and quit.\n"
    "  -l, --log <level> Logging level to set (default: %s).\n"
    "                    One of: info, warning, error, fatal, none.\n"
    "\n"
    "Link-related options:\n"
    "  --com <device>    Path or name of the serial device with which to\n"
    "                    communicate. If this option isn't used, the\n"
    "                    program will use USB to find the calculator.\n "
    " --use <settings>  Serial settings to use, when the link is established\n"
    "                    over a serial link (i.e. when used with `--com`).\n"
    "                    For example, \"9600N2\" represents 9600 bauds, no\n"
    "                    parity, and two stop bits.\n"
    "  --set <settings>  Serial settings to negotiate with the calculator\n"
    "                    (when used with `--com`).\n"
    "                    The string has the same format than for `--use`.\n"
    "  --reset           Shorthand option for `--set 9600N2`.\n"
    "  --no-init         Disable the initiation handshake when the link is\n"
    "                    established, for chaining multiple p7 subcommands.\n"
    "  --no-exit         Disable the termination handshake when the link is\n"
    "                    closed, for chaining multiple p7 subcommands.\n"
    "\n"
    "Type \"%s <subcommand> --help\" for some help "
    "about the subcommand.\n"
    "\n"
    "For guides, topics and reference, consult the documentation:\n"
    "    " CAHUTE_URL
    "\n"
    "\n"
    "For reporting issues and vulnerabilities, consult the following guide:\n"
    "    " CAHUTE_ISSUES_URL "\n";

#define SUBCOMMAND_FOOTER \
    "\nType \"%s --help\" for other subcommands and general options.\n"

static char const help_list_devices[] =
    "Usage: %s list-devices\n"
    "List serial devices.\n" SUBCOMMAND_FOOTER;

static char const help_info[] =
    "Usage: %s info\n"
    "Get information about the calculator.\n" SUBCOMMAND_FOOTER;

static char const help_idle[] =
    "Usage: %s idle\n"
    "Do nothing while the link is active.\n"
    "\n"
    "This subcommand is useful when chaining p7 subcommands, to dedicate a\n"
    "p7 call to only initiate or terminate the link, or negotiate new\n"
    "serial settings.\n" SUBCOMMAND_FOOTER;

static char const help_send[] =
    "Usage: %s send [options...] <local file>\n"
    "Send a file to the calculator.\n"
    "\n"
    "Available options are:\n"
    "  -#                Display a nice progress bar.\n"
    "  -f, --force       Force overwriting if relevant.\n"
    "  -o, --output <name>\n"
    "                    Output filename on the calculator.\n"
    "                    By default, the output file name is the base name\n"
    "                    of the provided local file path.\n"
    "  -d, --directory <dir>\n"
    "                    On-calc directory name in which the file will be\n"
    "                    stored. By default, the file is stored at root.\n"
    "  --storage <abc0>  Storage device with which to interact (fls0,\n"
    "                    crd0). By default, this option is set to "
    "'" DEFAULT_STORAGE "'.\n" SUBCOMMAND_FOOTER;

static char const help_get[] =
    "Usage: %s get [options...] <on-calc filename>\n"
    "Request a file from the calculator.\n"
    "\n"
    "Available options are:\n"
    "  -#                Display a nice progress bar.\n"
    "  -o, --output <name>\n"
    "                    Output local file path, absolute or relative to\n"
    "                    the working directory. By default, the file is\n"
    "                    stored in the working directory with the name\n"
    "                    it had on the calculator.\n"
    "  -d, --directory <dir>\n"
    "                    On-calc directory name from which to get the file.\n"
    "                    By default, the file is retrieved from root.\n"
    "  --storage <abc0>  Storage device with which to interact (fls0,\n"
    "                    crd0). By default, this option is set to "
    "'" DEFAULT_STORAGE "'.\n" SUBCOMMAND_FOOTER;

static char const help_copy[] =
    "Usage: %s copy [options...] <source file> <dest file>\n"
    "Copy a file into the other on the calculator.\n"
    "\n"
    "Available options are:\n"
    "  -#                Display a nice progress bar.\n"
    " -d, --directory <srcdir>\n"
    "                    On-calc directory name in which the source file is\n"
    "                    located. By default, root is used.\n"
    " -t, --to <dstdir>  On-calc directory name in which the file should be\n"
    "                    copied to. By default, root is used.\n"
    "  --storage <abc0>  Storage device with which to interact (fls0,\n"
    "                    crd0). By default, this option is set to "
    "'" DEFAULT_STORAGE "'.\n" SUBCOMMAND_FOOTER;

static char const help_delete[] =
    "Usage: %s delete [options...] <on-calc filename>\n"
    "Delete a file on the calculator.\n"
    "\n"
    "Available options are:\n"
    "  -d, --directory <dir>\n"
    "                    On-calc directory name from which to delete the\n"
    "                    file. By default, the file is deleted from root.\n"
    "  --storage <abc0>  Storage device with which to interact (fls0,\n"
    "                    crd0). By default, this option is set to "
    "'" DEFAULT_STORAGE "'.\n" SUBCOMMAND_FOOTER;

static char const help_list[] =
    "Usage: %s list [options...]\n"
    "List files on the distant filesystem.\n"
    "\n"
    "Available options are:\n"
    "  -d, --directory <dir>\n"
    "                    On-calc directory name from which to list\n"
    "                    files. By default, files are listed from every\n"
    "                    directory, including root.\n"
    "  --storage <abc0>  Storage device with which to interact (fls0,\n"
    "                    crd0). By default, this option is set to "
    "'" DEFAULT_STORAGE "'.\n" SUBCOMMAND_FOOTER;

static char const help_reset[] =
    "Usage: %s reset\n"
    "Reset the distant filesystem.\n"
    "\n"
    "Available options are:\n"
    "  --storage <abc0>  Storage device with which to interact (fls0,\n"
    "                    crd0). By default, this option is set to "
    "'" DEFAULT_STORAGE "'.\n" SUBCOMMAND_FOOTER;

static char const help_optimize[] =
    "Usage: %s optimize\n"
    "Optimize the distant filesystem.\n"
    "\n"
    "Available options are:\n"
    "  --storage <abc0>  Storage device with which to interact (fls0,\n"
    "                    crd0). By default, this option is set to "
    "'" DEFAULT_STORAGE "'.\n" SUBCOMMAND_FOOTER;

/**
 * Short options definitions.
 */
static struct short_option const short_options[] = {
    {'h', 0},
    {'v', 0},
    {'f', 0},
    {'o', OPTION_FLAG_PARAMETER_REQUIRED},
    {'d', OPTION_FLAG_PARAMETER_REQUIRED},
    {'t', OPTION_FLAG_PARAMETER_REQUIRED},
    {'l', OPTION_FLAG_PARAMETER_REQUIRED},
    {'#', 0},

    SHORT_OPTION_SENTINEL
};

/**
 * Long options definitions.
 */
static struct long_option const long_options[] = {
    {"help", 0, 'h'},
    {"version", 0, 'v'},
    {"com", OPTION_FLAG_PARAMETER_REQUIRED, 'c'},
    {"storage", OPTION_FLAG_PARAMETER_REQUIRED, 's'},
    {"force", 0, 'f'},
    {"output", OPTION_FLAG_PARAMETER_REQUIRED, 'o'},
    {"directory", OPTION_FLAG_PARAMETER_REQUIRED, 'd'},
    {"to", OPTION_FLAG_PARAMETER_REQUIRED, 't'},
    {"no-init", 0, 'i'},
    {"no-start", 0, 'i'},
    {"no-exit", 0, 'e'},
    {"no-term", 0, 'e'},
    {"set", OPTION_FLAG_PARAMETER_REQUIRED, 'S'},
    {"reset", 0, 'R'},
    {"use", OPTION_FLAG_PARAMETER_REQUIRED, 'U'},
    {"log", OPTION_FLAG_PARAMETER_REQUIRED, 'l'},

    LONG_OPTION_SENTINEL
};

/**
 * Parse serial attributes.
 *
 * The raw string format is "{speed}{parity}{stop bits}", where:
 *
 * - Speed is expressed in bauds, e.g. "9600".
 * - Parity is either "E" for even, "O" for odd and "N" for disabled.
 * - Stop bits is either "1" or "2".
 *
 * An example string is "9600N2" for 9600 bauds, no parity and 2 stop bits.
 *
 * @param raw Raw serial attributes.
 * @param flagsp Pointer to the flags to define.
 * @param speedp Pointer to the speed to define.
 * @return Whether an error has occurred (1) or not (0).
 */
static int parse_serial_attributes(
    char const *raw,
    unsigned long *flagsp,
    unsigned long *speedp
) {
    char const *s;
    unsigned long speed = 0;
    unsigned long flags = 0;

    for (s = raw; *s >= '0' && *s <= '9'; s++)
        speed = speed * 10 + *s - '0';


    if (s == raw)
        return 1;

    switch (speed) {
    case 300:
    case 600:
    case 1200:
    case 2400:
    case 4800:
    case 9600:
    case 19200:
    case 38400:
    case 57600:
    case 115200:
    case 230400:
    case 460800:
        break;

    default:
        return 1;
    }

    if (*s == 'N')
        flags |= CAHUTE_SERIAL_PARITY_OFF;
    else if (*s == 'E')
        flags |= CAHUTE_SERIAL_PARITY_EVEN;
    else if (*s == 'O')
        flags |= CAHUTE_SERIAL_PARITY_ODD;
    else
        return 1;

    s++;
    if (*s == '1')
        flags |= CAHUTE_SERIAL_STOP_ONE;
    else if (*s == '2')
        flags |= CAHUTE_SERIAL_STOP_TWO;
    else
        return 1;

    s++;
    if (*s) /* String should be terminated. */
        return 1;

    *speedp = speed;
    *flagsp = flags;
    return 0;
}

/**
 * Check an on-calc directory name.
 *
 * @param name Directory name to check.
 * @return 1 if all checks pass, 0 otherwise.
 */
static inline int check_directory_name(char const *name) {
    size_t n;

    if (!name)
        return 1;

    n = strnlen(name, 9);
    if (n > 8)
        return 0;

    for (; n--; name++)
        if (!isascii(*name) || *name == '/' || *name == '\\'
            || (!isgraph(*name) && !isblank(*name)))
            return 0;

    return 1;
}

/**
 * Check an on-calc file name.
 *
 * @param name Directory name to check.
 * @return 1 if all checks pass, 0 otherwise.
 */
static inline int check_file_name(char const *name) {
    size_t n;

    if (!name)
        return 1;

    n = strnlen(name, 13);
    if (n > 12)
        return 0;

    for (; n--; name++)
        if (!isascii(*name) || *name == '/' || *name == '\\'
            || (!isgraph(*name) && !isblank(*name)))
            return 0;

    return 1;
}

/**
 * Parse command-line parameters, and handle help and version messages.
 *
 * Note that since we use option parsing, the argv array is actually
 * reorganized to move positional parameters at the end of the array,
 * hence why argv is of "char **" type, and not "char const * const *".
 *
 * @param argc Argument count, as provided to main().
 * @param argv Argument values, as provided to main().
 * @param args Parsed argument structure to feed for use by caller.
 * @return Whether parameters were successfully parsed (1), or not (0).
 */
int parse_args(int argc, char **argv, struct args *args) {
    struct option_parser_state state;
    char const *command = argv[0], *subcommand;
    char **params;
    char const *o_directory = NULL;
    char const *o_target_directory = NULL;
    char const *o_output = NULL;
    char const *o_storage = DEFAULT_STORAGE;
    char *optarg;
    int option, optopt, help = 0, err, param_count;

    /* Default parsed arguments.
     * By default, the serial speed is defined as 9600N2. */
    args->command = COMMAND_IDLE;
    args->nice_display = 0;

    args->serial_flags = CAHUTE_SERIAL_PARITY_OFF | CAHUTE_SERIAL_STOP_TWO;
    args->serial_speed = 9600;
    args->new_serial_flags = CAHUTE_SERIAL_PARITY_OFF | CAHUTE_SERIAL_STOP_TWO;
    args->new_serial_speed = 9600;
    args->no_init = 0;
    args->no_term = 0;
    args->change_serial = 0;
    args->serial_name = NULL;

    args->storage_name = NULL;
    args->distant_source_directory_name = NULL;
    args->distant_source_name = NULL;
    args->distant_target_directory_name = NULL;
    args->distant_target_name = NULL;
    args->force = 0;

    args->local_source_path = NULL;
    args->local_target_path = NULL;
    args->local_source_fp = NULL;
    args->local_target_fp = NULL;

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
            /* -h, --help: display the help message and quit.
             * This is not done immediately because the displayed help
             * message depends on the parameters. */
            help = 1;
            break;

        case 'v':
            /* -v, --version: display the version message and quit. */
            puts(version_message);
            return 0;

        case 'f':
            /* -f, --force: overwrite without asking. */
            args->force = 1;
            break;

        case '#':
            /* -#: enable the loading bar. */
            args->nice_display = 1;
            break;

        case 'l':
            /* -l, --log: set the logging level. */
            set_log_level(optarg);
            break;

        case 'o':
            /* -o, --output: set the output filename.
             *
             * This can have multiple meanings depending on the subcommand:
             *
             * - With 'send' as 'distant_target_name'.
             * - With 'get', as 'distant_source_name. */
            o_output = optarg;
            break;

        case 'd':
            /* -d, --directory: set the output directory on the calculator.
             *
             * This can have multiple meanings depending on the subcommand:
             *
             * - With 'send', as 'distant_target_directory_name'.
             * - With 'get' and 'copy', as 'distant_source_directory_name'. */
            o_directory = optarg;
            break;

        case 't':
            /* -t, --to: set the destination directory for copy.
             *
             * This is only the 'distant_target_directory_name' for 'copy'. */
            o_target_directory = optarg;
            break;

        case 'c':
            /* --com: set the serial port. */
            args->serial_name = optarg;
            break;

        case 's':
            /* --storage: set the storage. */
            o_storage = optarg;
            break;

        case 'i':
            /* --no-init: disable link initialization. */
            args->no_init = 1;
            break;

        case 'e':
            /* --no-exit: disable link termination. */
            args->no_term = 1;
            break;

        case 'U':
            /* --use: use initial serial settings. */
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

        case 'S':
            /* --set: set serial settings to negotiate with the calculator. */
            err = parse_serial_attributes(
                optarg,
                &args->new_serial_flags,
                &args->new_serial_speed
            );
            if (err) {
                fprintf(stderr, "-s, --set: invalid format!\n");
                return 0;
            }

            args->change_serial = 1;
            break;

        case 'R':
            /* --reset: reset serial settings. */
            args->new_serial_flags =
                CAHUTE_SERIAL_PARITY_OFF | CAHUTE_SERIAL_STOP_TWO;
            args->new_serial_speed = 9600;
            args->change_serial = 1;
            break;

        case GETOPT_FAIL:
            /* Erroneous option usage. */
            if (optopt == 'o')
                fprintf(stderr, "-o, --output: expected an argument\n");
            else if (optopt == 'd')
                fprintf(stderr, "-d, --directory: expected an argument\n");
            else if (optopt == 't')
                fprintf(stderr, "-t, --to: expected an argument\n");
            else if (optopt == 'c')
                fprintf(stderr, "--com: expected an argument\n");
            else if (optopt == 's')
                fprintf(stderr, "--storage: expected an argument\n");
            else
                /* We ignore unknown options. */
                break;

            return 0;
        }
    }

    update_positional_parameters(&state, &param_count, &params);
    if (!param_count || !strcmp(params[0], "help")) {
        printf(help_main, command, get_current_log_level(), command);
        return 0;
    }

    subcommand = params[0];
    params++;
    param_count--;

    if (!strcmp(subcommand, "version")) {
        puts(version_message);
        return 0;
    }

    if (!strcmp(subcommand, "list-devices")) {
        if (help || param_count != 0) {
            printf(help_list_devices, command, command);
            return 0;
        }

        args->command = COMMAND_LIST_SERIAL;
    } else if (!strcmp(subcommand, "send")) {
        if (help || param_count != 1) {
            printf(help_send, command, command);
            return 0;
        }

        if (!o_output) {
            /* By default, we want to determine the output name from the
             * local path. */
            o_output = strrchr(params[0], '/');
            if (!o_output)
                o_output = params[0];
        }

        args->command = COMMAND_SEND;
        args->storage_name = o_storage;
        args->local_source_path = params[0];
        args->distant_target_directory_name = o_directory;
        args->distant_target_name = o_output;
    } else if (!strcmp(subcommand, "get")) {
        if (help || param_count != 1) {
            printf(help_get, command, command);
            return 0;
        }

        if (o_output == NULL)
            /* By default, we want the local name to be the same as the
             * distant file name. */
            o_output = params[0];

        args->command = COMMAND_GET;
        args->storage_name = o_storage;
        args->distant_source_directory_name = o_directory;
        args->distant_source_name = params[0];

        if (!strcmp(o_output, "-")) /* Standard output. */
            args->local_target_fp = stdout;
        else
            args->local_target_path = o_output;
    } else if (!strcmp(subcommand, "copy") || !strcmp(subcommand, "cp")) {
        if (help || param_count != 2) {
            printf(help_copy, command, command);
            return 0;
        }

        args->command = COMMAND_COPY;
        args->storage_name = o_storage;
        args->distant_source_directory_name = o_directory;
        args->distant_source_name = params[0];
        args->distant_target_directory_name = o_target_directory;
        args->distant_target_name = params[1];
    } else if (!strcmp(subcommand, "delete") || !strcmp(subcommand, "del")) {
        if (help || param_count != 1) {
            printf(help_delete, command, command);
            return 0;
        }

        args->command = COMMAND_DELETE;
        args->storage_name = o_storage;
        args->distant_target_directory_name = o_directory;
        args->distant_target_name = params[0];
    } else if (!strcmp(subcommand, "list") || !strcmp(subcommand, "ls")) {
        if (help || param_count != 0) {
            printf(help_list, command, command);
            return 0;
        }

        args->command = COMMAND_LIST;
        args->storage_name = o_storage;
        args->distant_target_directory_name = o_directory;
    } else if (!strcmp(subcommand, "reset")) {
        if (help || param_count != 0) {
            printf(help_reset, command, command);
            return 0;
        }

        args->command = COMMAND_RESET;
        args->storage_name = o_storage;
    } else if (!strcmp(subcommand, "optimize")) {
        if (help || param_count != 0) {
            printf(help_optimize, command, command);
            return 0;
        }

        args->command = COMMAND_OPTIMIZE;
        args->storage_name = o_storage;
    } else if (!strcmp(subcommand, "info")) {
        if (help || param_count != 0) {
            printf(help_info, command, command);
            return 0;
        }

        args->command = COMMAND_INFO;
    } else if (!strcmp(subcommand, "idle") || !strcmp(subcommand, "laze")) {
        if (help || param_count != 0) {
            printf(help_idle, command, command);
            return 0;
        }

        args->command = COMMAND_IDLE;
    } else {
        /* The subcommand is unknown. */
        printf(help_main, command, get_current_log_level(), command);
        return 0;
    }

    if (args->storage_name
        && (strlen(args->storage_name) != 4 || !isascii(args->storage_name[0])
            || !islower(args->storage_name[0])
            || !isascii(args->storage_name[1])
            || !islower(args->storage_name[1])
            || !isascii(args->storage_name[2])
            || !isascii(args->storage_name[2])
            || !isascii(args->storage_name[3])
            || !isdigit(args->storage_name[3]))) {
        fprintf(stderr, "Invalid storage name format.\n");
        return 0;
    }

    if (!check_directory_name(args->distant_source_directory_name)) {
        fprintf(stderr, "Invalid source directory name format.\n");
        return 0;
    }

    if (!check_file_name(args->distant_source_name)) {
        fprintf(stderr, "Invalid source file name format.\n");
        return 0;
    }

    if (!check_directory_name(args->distant_target_directory_name)) {
        fprintf(stderr, "Invalid target directory name format.\n");
        return 0;
    }

    if (!check_file_name(args->distant_target_name)) {
        fprintf(stderr, "Invalid target file name format.\n");
        return 0;
    }

    /* Open the local source path if a path is given. */
    if (args->local_source_path && !args->local_source_fp) {
        args->local_source_fp = fopen(args->local_source_path, "r");
        if (!args->local_source_fp) {
            fprintf(
                stderr,
                "Can't open '%s': %s\n",
                args->local_source_path,
                strerror(errno)
            );
            return 0;
        }
    }

    /* Open the local target path if a path is given. */
    if (args->local_target_path && !args->local_target_fp) {
        args->local_target_fp = fopen(args->local_target_path, "w+");
        if (!args->local_target_fp) {
            fprintf(
                stderr,
                "Can't open '%s': %s\n",
                args->local_target_path,
                strerror(errno)
            );
            if (args->local_source_path && args->local_source_fp)
                fclose(args->local_source_fp);

            return 0;
        }
    }

    return 1;
}
