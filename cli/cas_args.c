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
#include "casrc.h"
#include "options.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#define BANNER \
    "CaS - from Cahute v" CAHUTE_VERSION " (licensed under CeCILL 2.1)"

static char const version_message[] = BANNER
    "\n"
    "\n"
    "This is free software; see the source for copying conditions.\n"
    "There is NO warranty; not even for MERCHANTABILITY or\n"
    "FITNESS FOR A PARTICULAR PURPOSE.\n";

static char const help_message[] =
    "Usage: %s\n"
    "          [-h] [-V] [-v] [-d[=<file>]] [-p] [-m=<model>]\n"
    "          [-i=[<format>,]<args>] [-o[=[<format>,]<args>] <file or device"
    "path>]\n"
    "          [-c=<conversion>] [-C=<conversion>] [-l[=<args>]] [-t]\n"
    "          <input file or device path>\n"
    "\n"
    "General options:\n"
    "  -h, --help        Display the help page of the (sub)command and quit.\n"
    "  -V, --version     Display the version message and quit.\n"
    "  -v, --verbose     Display the utility version before all.\n"
    "  -d[=<file path>], --debug[=<file path>]\n"
    "                    Allow debug logs, and optionally place them in the\n"
    "                    provided file.\n"
    "\n"
    "Pipeline-related configuration:\n"
    "\n"
    "  -i=[<format>],<args>, --input=[<format>],<args>\n"
    "                    Set the format and optional parameters for the\n"
    "                    input.\n"
    "  -c=<conv>[, ...], --convert=<conv>[, ...]\n"
    "                    Operate one or more conversions between input and\n"
    "                    optional listing.\n"
    "  -l[=<args>], --list[=<args>]\n"
    "                    Enable file contents listing, and optionnally set\n"
    "                    the general listing options.\n"
    "  -t, --terse       Enable file type listing.\n"
    "  -C=<conv>[, ...], --convert-after=<conv>[, ...]\n"
    "                    Operate one or more conversions between optional\n"
    "                    listing and output.\n"
    "  -o[=[<format>,]<args>] <file or device path>,\n"
    "  --output[=[<format>,]<args>] <file or device path>\n"
    "                    Enable output, and set the output file or device\n"
    "                    path, optional format and parameters.\n"
    "\n"
    "Other options:\n"
    "  -p, --pager\n"
    "                    Invoke a terminal pager to view the list.\n"
    "  -m=<model>, --model=<model>\n"
    "                    Model of the calculator for or with which to \n"
    "                    operate the file or serial port manipulations.\n"
    "\n"
    "For guides, topics and reference, consult the documentation:\n"
    "    " CAHUTE_URL
    "\n"
    "\n"
    "For reporting issues and vulnerabilities, consult the following guide:\n"
    "    " CAHUTE_ISSUES_URL "\n";

/* Short options. */
static struct short_option const short_options[] = {
    {'h', 0},
    {'?', 0},
    {'V', 0},
    {'i', OPTION_FLAG_ATTRIBUTE_REQUIRED},
    {'o', OPTION_FLAG_PARAMETER_REQUIRED | OPTION_FLAG_ATTRIBUTE_OPTIONAL},
    {'l', OPTION_FLAG_ATTRIBUTE_OPTIONAL},
    {'m', OPTION_FLAG_ATTRIBUTE_REQUIRED},
    {'c', OPTION_FLAG_ATTRIBUTE_REQUIRED},
    {'C', OPTION_FLAG_ATTRIBUTE_REQUIRED},
    {'t', 0},
    {'e', 0},
    {'p', 0},
    {'d', OPTION_FLAG_ATTRIBUTE_OPTIONAL},
    {'v', 0},

    SHORT_OPTION_SENTINEL
};

/* Long options. */
static struct long_option const long_options[] = {
    {"help", 0, 'h'},
    {"version", 0, 'V'},
    {"input", OPTION_FLAG_ATTRIBUTE_REQUIRED, 'i'},
    {"infile", OPTION_FLAG_ATTRIBUTE_REQUIRED, 'i'},
    {"output",
     OPTION_FLAG_PARAMETER_REQUIRED | OPTION_FLAG_ATTRIBUTE_OPTIONAL,
     'o'},
    {"outfile",
     OPTION_FLAG_PARAMETER_REQUIRED | OPTION_FLAG_ATTRIBUTE_OPTIONAL,
     'o'},
    {"list", OPTION_FLAG_ATTRIBUTE_OPTIONAL, 'l'},
    {"display", OPTION_FLAG_ATTRIBUTE_OPTIONAL, 'l'},
    {"model", OPTION_FLAG_ATTRIBUTE_REQUIRED, 'm'},
    {"convert", OPTION_FLAG_ATTRIBUTE_REQUIRED, 'c'},
    {"convert-after", OPTION_FLAG_ATTRIBUTE_REQUIRED, 'C'},
    {"terse", 0, 't'},
    {"castle", 0, 'e'},
    {"pager", 0, 'p'},
    {"debug", OPTION_FLAG_ATTRIBUTE_OPTIONAL, 'd'},
    {"verbose", 0, 'v'},

    LONG_OPTION_SENTINEL
};

/**
 * Logging function used when a debug file is set.
 *
 * This callback prints the logs on the file pointer provided as the
 * cookie, with a format resembling the following output:
 *
 *     info: Without a function.
 *     warning: With a user function.
 *     error: With an int. function.
 *
 * @param filep File pointer.
 * @param level Log level for the given message.
 * @param func Name of the function.
 * @param message Formatted message.
 */
static void log_to_debug_file(
    FILE *filep,
    int level,
    char const *func,
    char const *message
) {
    char const *level_name;

    switch (level) {
    case CAHUTE_LOGLEVEL_INFO:
        level_name = "info";
        break;
    case CAHUTE_LOGLEVEL_WARNING:
        level_name = "warning";
        break;
    case CAHUTE_LOGLEVEL_ERROR:
        level_name = "error";
        break;
    case CAHUTE_LOGLEVEL_FATAL:
        level_name = "fatal";
        break;
    case CAHUTE_LOGLEVEL_NONE:
        level_name = "(none)";
        break;
    default:
        level_name = "(unknown)";
    }

    fprintf(filep, "%s: %s\n", level_name, message);
}

/**
 * Decode a file type.
 *
 * @param raw Raw file type.
 * @param typep Type pointer.
 * @return 0 if no errors, other otherwise.
 */
static int decode_file_type(char *raw, int *typep) {
    char *p;

    for (p = raw; *p; p++)
        *p = tolower(*p);

    if (!strcmp(raw, "ssmono"))
        *typep = FILE_TYPE_SSMONO;
    else if (!strcmp(raw, "sscol"))
        *typep = FILE_TYPE_SSCOL;
    else if (!strcmp(raw, "oldprog"))
        *typep = FILE_TYPE_OLDPROG;
    else if (!strcmp(raw, "editor"))
        *typep = FILE_TYPE_EDITOR;
    else
        return 1;

    return 0;
}

/**
 * Decode a conversion.
 *
 * @param raw Raw conversion.
 * @param srcp Source type.
 * @param dstp Destination type.
 * @return 0 if no errors, other otherwise.
 */
static int decode_conversion(char *raw, int *srcp, int *dstp) {
    char *p;

    p = strchr(raw, ',');
    if (p)
        *p = '\0';

    p = strchr(raw, '-');
    if (!p)
        return 1;

    *p = '\0';
    return (decode_file_type(raw, srcp) || decode_file_type(p + 1, dstp));
}

/**
 * Parse options regarding a medium (for input or output).
 *
 * @param db Database to parse medium data from.
 * @param prefix Prefix, either "in" or "out".
 * @param medium Medium to set.
 * @param path File or device path.
 * @return 0 if no errors were encountered, other otherwise.
 */
static int parse_medium_params(
    struct casrc_database *db,
    char const *prefix,
    struct medium *medium,
    char const *path
) {
    struct casrc_setting *dstg = NULL;
    struct casrc_setting *ostg = get_casrc_setting(db, prefix);
    cahute_file *file = NULL;
    int err;

    {
        char const *type_suffix = NULL;
        unsigned long file_type;
        char buf[10];

        /* TODO: Add the flags/format tip for opening the file. */
        if (get_casrc_setting_property(NULL, ostg, "ctf")) {
            medium->type = MEDIUM_FILE;
        } else if (get_casrc_setting_property(NULL, ostg, "cas")) {
            medium->type = MEDIUM_FILE;
        } else if (get_casrc_setting_property(NULL, ostg, "fxp")) {
            medium->type = MEDIUM_FILE;
        } else if (get_casrc_setting_property(NULL, ostg, "bmp")) {
            medium->type = MEDIUM_FILE;
        } else if (get_casrc_setting_property(NULL, ostg, "gif")) {
            medium->type = MEDIUM_FILE;
        } else if (get_casrc_setting_property(NULL, ostg, "com")) {
            medium->type = MEDIUM_COM;
        } else if (path && (!memcmp(path, "/dev/", 5) || (!memcmp(path, "COM", 3) && isdigit(path[3])))) {
            medium->type = MEDIUM_COM;
        } else {
            fprintf(stderr, "Missing medium type for %s.\n", prefix);
            return 1;
        }

        if (medium->type == MEDIUM_COM)
            type_suffix = "com";
        else if (!strcmp(prefix, "in")) {
            err = cahute_open_file_for_reading(
                &file,
                0,
                path,
                CAHUTE_PATH_TYPE_CLI
            );
            if (err) {
                fprintf(
                    stderr,
                    "Could not open input file (%s).\n",
                    cahute_get_error_name(err)
                );
                return 1;
            }

            err = cahute_guess_file_type(file, &file_type);
            if (err)
                file_type = CAHUTE_FILE_TYPE_UNKNOWN;

            medium->data.file.file = file;
            medium->data.file.type = file_type;

            switch (file_type) {
            case CAHUTE_FILE_TYPE_MAINMEM:
                /* No options related to fx-9860G main memory archives yet. */
                break;

            case CAHUTE_FILE_TYPE_CTF:
                type_suffix = "ctf";
                break;

            case CAHUTE_FILE_TYPE_CASIOLINK:
                type_suffix = "cas";
                break;

            case CAHUTE_FILE_TYPE_FXPROGRAM:
                type_suffix = "fxp";
                break;

            case CAHUTE_FILE_TYPE_BITMAP:
                type_suffix = "bmp";
                break;

            case CAHUTE_FILE_TYPE_GIF:
                type_suffix = "gif";
                break;

            default:
                fprintf(stderr, "Could not determine input file type.\n");
                return 1;
            }
        } else {
            /* TODO: We need to support extension detection in either the
             * library or here. */
            fprintf(stderr, "File output is not supported yet.\n");
            goto fail;
        }

        if (type_suffix) {
            sprintf(buf, "%s.%s", prefix, type_suffix);
            dstg = get_casrc_setting(db, buf);
        }
    }

    switch (medium->type) {
    case MEDIUM_FILE:
        switch (medium->data.file.type) {
        case CAHUTE_FILE_TYPE_CTF:
            medium->data.file.options.ctf.glossary =
                get_casrc_setting_property(dstg, ostg, "glossary") != NULL;
            medium->data.file.options.ctf.nice =
                get_casrc_setting_property(dstg, ostg, "nice") != NULL;
            break;

        case CAHUTE_FILE_TYPE_CASIOLINK:
            if (get_casrc_setting_property(dstg, ostg, "7700")
                || get_casrc_setting_property(dstg, ostg, "9700")
                || get_casrc_setting_property(dstg, ostg, "9800"))
                medium->data.file.options.cas.header_format =
                    HEADER_FORMAT_CAS40;
            else if (
                get_casrc_setting_property(dstg, ostg, "9750")
                || get_casrc_setting_property(dstg, ostg, "9850")
                || get_casrc_setting_property(dstg, ostg, "9950")
            )
                medium->data.file.options.cas.header_format =
                    HEADER_FORMAT_CAS50;
            else if (
                get_casrc_setting_property(dstg, ostg, "raw")
                || get_casrc_setting_property(dstg, ostg, "uncooked")
            )
                medium->data.file.options.cas.header_format =
                    HEADER_FORMAT_RAW;
            else
                medium->data.file.options.cas.header_format =
                    HEADER_FORMAT_UNKNOWN;

            medium->data.file.options.cas.status =
                get_casrc_setting_property(dstg, ostg, "status") != NULL;
            break;

        case CAHUTE_FILE_TYPE_BITMAP:
            medium->data.file.options.bmp.inverse =
                (get_casrc_setting_property(dstg, ostg, "inv") != NULL
                 && get_casrc_setting_property(dstg, ostg, "inverse") != NULL);
            break;

        case CAHUTE_FILE_TYPE_GIF:
            medium->data.file.options.gif.inverse =
                (get_casrc_setting_property(dstg, ostg, "inv") != NULL
                 && get_casrc_setting_property(dstg, ostg, "inverse") != NULL);
            break;
        }
        break;

    case MEDIUM_COM: {
        char const *raw_speed = get_casrc_setting_property(dstg, ostg, "baud");
        char const *raw_parity =
            get_casrc_setting_property(dstg, ostg, "parity");
        char const *raw_stop = get_casrc_setting_property(dstg, ostg, "stop");

        medium->data.com.serial_speed = 0;
        medium->data.com.serial_flags = 0;

        if (raw_speed) {
            unsigned long speed = atol(raw_speed);

            if (speed != 1200 && speed != 2400 && speed != 4800
                && speed != 9600 && speed != 19200 && speed != 38400
                && speed != 57600 && speed != 115200) {
                fprintf(
                    stderr,
                    "Invalid property baud=%s for %s.\n",
                    raw_speed,
                    prefix
                );
                return 1;
            }

            medium->data.com.serial_speed = speed;
        }

        if (!raw_parity)
            medium->data.com.serial_flags |= CAHUTE_SERIAL_PARITY_OFF;
        else if (raw_parity[0] == 'e' || raw_parity[0] == 'E')
            medium->data.com.serial_flags |= CAHUTE_SERIAL_PARITY_EVEN;
        else if (raw_parity[0] == 'o' || raw_parity[0] == 'O')
            medium->data.com.serial_flags |= CAHUTE_SERIAL_PARITY_ODD;
        else
            medium->data.com.serial_flags |= CAHUTE_SERIAL_PARITY_OFF;

        if (!raw_stop) { /* Extended */
        } else if (!strcmp(raw_stop, "1"))
            medium->data.com.serial_flags |= CAHUTE_SERIAL_STOP_ONE;
        else if (!strcmp(raw_stop, "2"))
            medium->data.com.serial_flags |= CAHUTE_SERIAL_STOP_TWO;
        else {
            fprintf(
                stderr,
                "Invalid property stop=%s for %s.\n",
                raw_stop,
                prefix
            );
            return 1;
        }

        if (get_casrc_setting_property(dstg, ostg, "dtr"))
            medium->data.com.serial_flags |= CAHUTE_SERIAL_DTR_HANDSHAKE;
        else
            medium->data.com.serial_flags |= CAHUTE_SERIAL_DTR_DISABLE;

        if (get_casrc_setting_property(dstg, ostg, "rts"))
            medium->data.com.serial_flags |= CAHUTE_SERIAL_RTS_HANDSHAKE;
        else
            medium->data.com.serial_flags |= CAHUTE_SERIAL_RTS_DISABLE;

        if (get_casrc_setting_property(dstg, ostg, "7700")
            || get_casrc_setting_property(dstg, ostg, "9700")
            || get_casrc_setting_property(dstg, ostg, "9800"))
            medium->data.com.serial_flags |=
                CAHUTE_SERIAL_PROTOCOL_CASIOLINK
                | CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS40;
        else if (get_casrc_setting_property(dstg, ostg, "9750")
         || get_casrc_setting_property(dstg, ostg, "9850")
         || get_casrc_setting_property(dstg, ostg, "9950"))
            medium->data.com.serial_flags |=
                CAHUTE_SERIAL_PROTOCOL_CASIOLINK
                | CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS50;
        else if (get_casrc_setting_property(dstg, ostg, "afx")) /* Extended */
            medium->data.com.serial_flags |=
                CAHUTE_SERIAL_PROTOCOL_CASIOLINK
                | CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS100;
        else if (get_casrc_setting_property(dstg, ostg, "cp") /* Extended */
         || get_casrc_setting_property(dstg, ostg, "cp300")
         || get_casrc_setting_property(dstg, ostg, "cp330")
         || get_casrc_setting_property(dstg, ostg, "cp330+"))
            medium->data.com.serial_flags |=
                CAHUTE_SERIAL_PROTOCOL_CASIOLINK
                | CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS300;

        medium->data.com.pause =
            get_casrc_setting_property(dstg, ostg, "pause") != NULL;
        medium->data.com.inline_protocol =
            get_casrc_setting_property(dstg, ostg, "inline") != NULL;
        medium->data.com.overwrite =
            get_casrc_setting_property(dstg, ostg, "overwrite") != NULL;
    } break;
    }

    return 0;

fail:
    if (file)
        cahute_close_file(file);

    return 1;
}

/**
 * Parse options regarding a list.
 *
 * @param db Database to parse medium data from.
 * @param name Name of the variable.
 * @param fmt Format parameters.
 * @return 0 if no error, other otherwise.
 */
static int parse_list_params(
    struct casrc_database *db,
    char const *name,
    struct list_format *fmt
) {
    struct casrc_setting *dstg, *ostg;

    {
        char settingbuf[30];

        sprintf(settingbuf, "list.%s", name);
        dstg = get_casrc_setting(db, settingbuf);
        ostg = get_casrc_setting(db, "list");
    }

    {
        char const *numformat;

        numformat = get_casrc_setting_property(dstg, ostg, "num");
        if (!numformat)
            numformat = "";

        if (get_casrc_setting_property(dstg, ostg, "hex")
            || get_casrc_setting_property(dstg, ostg, "hexadecimal")
            || !strcmp(numformat, "hex") || !strcmp(numformat, "hexadecimal"))
            fmt->number_format = NUMBER_FORMAT_HEX;
        else if (
            get_casrc_setting_property(dstg, ostg, "dec")
            || get_casrc_setting_property(dstg, ostg, "decimal")
            || !strcmp(numformat, "dec")
            || !strcmp(numformat, "decimal")
        )
            fmt->number_format = NUMBER_FORMAT_DEC;
        else if (
            get_casrc_setting_property(dstg, ostg, "oct")
            || get_casrc_setting_property(dstg, ostg, "octal")
            || !strcmp(numformat, "oct")
            || !strcmp(numformat, "octal")
        )
            fmt->number_format = NUMBER_FORMAT_OCT;
        else if (
            get_casrc_setting_property(dstg, ostg, "spc")
            || get_casrc_setting_property(dstg, ostg, "space")
        )
            fmt->number_format = NUMBER_FORMAT_SPACE;
        else
            fmt->number_format = NUMBER_FORMAT_BASIC;
    }

    fmt->nice = get_casrc_setting_property(dstg, ostg, "nice") != NULL;
    fmt->password =
        (get_casrc_setting_property(dstg, ostg, "pw") != NULL
         || get_casrc_setting_property(dstg, ostg, "pw") != NULL);

    return 0;
}

/**
 * Parse the command-line for CaS.
 *
 * @param argc Argument count.
 * @param argv Argument values.
 * @param args Parsed parameters.
 * @return Whether command-line decoding has worked (1) or not (0).
 */
int parse_args(int argc, char **argv, struct args *args) {
    struct option_parser_state state;
    struct casrc_database *db;
    char *optarg, *optattr;
    int option, optopt, help = 0, version = 0;
    char const *command = argv[0];
    char const *debug_path = NULL;
    char *raw_input_attr = NULL;
    char *raw_output_attr = NULL;
    char *raw_list_attr = NULL;
    char *raw_model_attr = NULL;

    args->model = MODEL_UNKNOWN;
    args->should_list_files = 0;
    args->should_list_types = 0;
    args->verbose = 0;
    args->should_output = 0;
    args->pager = 0;
    args->conversions = NULL;
    args->in.type = MEDIUM_UNKNOWN;
    args->out.type = MEDIUM_UNKNOWN;
    args->debug_fp = NULL;

    cahute_set_log_level(CAHUTE_LOGLEVEL_FATAL);

    init_option_parser(
        &state,
        GETOPT_STYLE_CAS,
        short_options,
        long_options,
        argc,
        argv
    );

    while (parse_next_option(&state, &option, &optopt, &optattr, &optarg)) {
        switch (option) {
        case 'h':
        case '?':
            help = 1;
            break;

        case 'V':
            version = 1;
            break;

        case 'i':
            if (raw_input_attr) {
                fprintf(stderr, "-i, --input: duplicate option\n");
                help = 1;
                break;
            }

            raw_input_attr = optattr;
            break;

        case 'o':
            if (args->should_output) {
                fprintf(stderr, "-o, --output: duplicate option\n");
                help = 1;
                break;
            }

            args->should_output = 1;
            raw_output_attr = optattr;
            args->out.path = optarg;
            break;

        case 'l':
            if (args->should_list_files) {
                fprintf(stderr, "-l, --list: duplicate option\n");
                help = 1;
                break;
            }

            args->should_list_files = 1;
            raw_list_attr = optattr;
            args->should_list_files = 1;
            break;

        case 'm':
            if (raw_model_attr) {
                fprintf(stderr, "-m, --model: duplicate option\n");
                help = 1;
                break;
            }

            raw_model_attr = optattr;
            break;

        case 'C':
        case 'c': {
            struct conversion *conv;
            int from, to;

            if (decode_conversion(optattr, &from, &to)) {
                if (option == 'c')
                    fprintf(stderr, "-c, --convert: invalid format\n");
                else
                    fprintf(stderr, "-C, --convert-after: invalid format\n");

                break;
            }

            conv = malloc(sizeof(struct conversion));
            if (!conv) {
                fprintf(stderr, "malloc() for conversion failed.\n");
                goto fail;
            }

            conv->next = args->conversions;
            conv->source_type = from;
            conv->dest_type = to;
            conv->after = (option == 'C');

            args->conversions = conv;
        } break;

        case 't':
            args->should_list_types = 1;
            break;

        case 'v':
            args->verbose = 1;
            break;

        case 'd':
            cahute_set_log_level(CAHUTE_LOGLEVEL_INFO);
            if (optattr)
                debug_path = optattr;
            break;

        case 'e':
            fprintf(
                stderr,
                "Communication with the Castle IDE is disabled.\n"
            );
            break;

        case 'p':
            args->pager = 1;
            break;

        case GETOPT_FAIL:
            if (optopt == 'i')
                fprintf(stderr, "-i, --input: missing attribute\n");
            else if (optopt == 'c')
                fprintf(stderr, "-c, --convert: missing attribute\n");
            else if (optopt == 'C')
                fprintf(stderr, "-C, --convert-after: missing attribute\n");
            else if (optopt == 'm')
                fprintf(stderr, "-m, --model: missing attribute\n");
            else if (optopt == 'o')
                fprintf(
                    stderr,
                    "-o, --output: missing attribute or parameter\n"
                );
            else
                break;

            help = 1;
        }
    }

    update_positional_parameters(&state, &argc, &argv);
    if (argc != 1)
        help = 1;
    else
        args->in.path = argv[0];

    if (version) {
        fprintf(stderr, version_message);
        goto fail;
    }

    if (help) {
        fprintf(stderr, help_message, command);
        goto fail;
    }

    if (args->verbose)
        fprintf(stderr, BANNER ".\n");

    if (debug_path) {
        args->debug_fp = fopen(debug_path, "wb");
        if (!args->debug_fp) {
            fprintf(
                stderr,
                "Could not open debug file: %s\n",
                strerror(errno)
            );
            goto fail;
        }

        cahute_set_log_func(
            (cahute_log_func *)&log_to_debug_file,
            args->debug_fp
        );
    }

    if (create_casrc_database(&db)) {
        fprintf(stderr, "Could not create the casrc database.\n");
        goto fail;
    }

    if (load_default_casrc(db)) {
        fprintf(stderr, "Could not load the system casrc.\n");
        destroy_casrc_database(db);
        goto fail;
    }

    if (raw_input_attr
        && define_casrc_setting(db, &db->settings, "in", raw_input_attr, 1)) {
        fprintf(stderr, "Could not load the input properties.\n");
        destroy_casrc_database(db);
        goto fail;
    }

    if (raw_output_attr
        && define_casrc_setting(
            db,
            &db->settings,
            "out",
            raw_output_attr,
            1
        )) {
        fprintf(stderr, "Could not load the output properties.\n");
        destroy_casrc_database(db);
        goto fail;
    }

    if (raw_list_attr
        && define_casrc_setting(db, &db->settings, "list", raw_list_attr, 1)) {
        fprintf(stderr, "Could not load the listing properties.\n");
        destroy_casrc_database(db);
        goto fail;
    }

    if (raw_model_attr
        && define_casrc_setting(
            db,
            &db->settings,
            "model",
            raw_model_attr,
            1
        )) {
        fprintf(stderr, "Could not load the model properties.\n");
        destroy_casrc_database(db);
        goto fail;
    }

    /* Detect the model. */
    if (get_casrc_property(db, "model", "fx7700")
        || get_casrc_property(db, "model", "cfx7700")
        || get_casrc_property(db, "model", "7700")
        || get_casrc_property(db, "model", "fx7")
        || get_casrc_property(db, "model", "cfx7")
        || get_casrc_property(db, "model", "7"))
        args->model = MODEL_7700;
    else if (
        get_casrc_property(db, "model", "fx9700")
        || get_casrc_property(db, "model", "cfx9700")
        || get_casrc_property(db, "model", "9700")
        || get_casrc_property(db, "model", "fx9")
        || get_casrc_property(db, "model", "cfx9")
        || get_casrc_property(db, "model", "9")
    )
        args->model = MODEL_9700;
    else if (
        get_casrc_property(db, "model", "fx9750")
        || get_casrc_property(db, "model", "cfx9750")
        || get_casrc_property(db, "model", "9750")
    )
        args->model = MODEL_9750;
    else if (
        get_casrc_property(db, "model", "fx9800")
        || get_casrc_property(db, "model", "cfx9800")
        || get_casrc_property(db, "model", "9800")
        || get_casrc_property(db, "model", "fx8")
        || get_casrc_property(db, "model", "cfx8")
        || get_casrc_property(db, "model", "8")
    )
        args->model = MODEL_9800;
    else if (
        get_casrc_property(db, "model", "fx9850")
        || get_casrc_property(db, "model", "cfx9850")
        || get_casrc_property(db, "model", "9850")
        || get_casrc_property(db, "model", "fx5")
        || get_casrc_property(db, "model", "cfx5")
        || get_casrc_property(db, "model", "5")
    )
        args->model = MODEL_9850;
    else if (
        get_casrc_property(db, "model", "fx9950")
        || get_casrc_property(db, "model", "cfx9950")
        || get_casrc_property(db, "model", "9950")
    )
        args->model = MODEL_9950;
    else if (
        get_casrc_property(db, "model", "any")
        || get_casrc_property(db, "model", "*")
    )
        args->model = MODEL_ANY;
    else
        args->model = MODEL_UNKNOWN;

    /* Parse in/out params. */
    if (parse_medium_params(db, "in", &args->in, args->in.path)
        || (args->should_output
            && parse_medium_params(db, "out", &args->out, args->out.path)))
        goto fail;

    if (args->should_list_files
        && (parse_list_params(db, "oldprog", &args->list.oldprog)
            || parse_list_params(db, "editor", &args->list.editor)
            || parse_list_params(db, "fn", &args->list.fn)
            || parse_list_params(db, "ssmono", &args->list.ssmono)
            || parse_list_params(db, "sscol", &args->list.sscol)
            || parse_list_params(db, "varmem", &args->list.varmem)
            || parse_list_params(db, "defmem", &args->list.defmem)
            || parse_list_params(db, "allmem", &args->list.allmem)
            || parse_list_params(db, "sd", &args->list.sd)
            || parse_list_params(db, "lr", &args->list.lr)
            || parse_list_params(db, "matrix", &args->list.matrix)
            || parse_list_params(db, "rectab", &args->list.rectab)
            || parse_list_params(db, "fntab", &args->list.fntab)
            || parse_list_params(db, "poly", &args->list.poly)
            || parse_list_params(db, "simul", &args->list.simul)
            || parse_list_params(db, "zoom", &args->list.zoom)
            || parse_list_params(db, "dyna", &args->list.dyna)
            || parse_list_params(db, "graphs", &args->list.graphs)
            || parse_list_params(db, "range", &args->list.range)
            || parse_list_params(db, "backup", &args->list.backup)
            || parse_list_params(db, "end", &args->list.end)
            || parse_list_params(db, "raw", &args->list.raw)
            || parse_list_params(db, "text", &args->list.text)
            || parse_list_params(db, "desc", &args->list.desc)))
        goto fail;

    /* We do not need the database in the parent, since all members have
     * been parsed here. */
    destroy_casrc_database(db);

    return 1;

fail:
    free_args(args);
    return 0;
}

/**
 * Free allocated components in the arguments..
 *
 * @param args Parsed arguments in which elements to free are present.
 */
void free_args(struct args *args) {
    struct conversion *conv;

    if (args->in.type == MEDIUM_FILE)
        cahute_close_file(args->in.data.file.file);

    if (args->debug_fp) {
        cahute_reset_log_func();
        fclose(args->debug_fp);
    }

    for (conv = args->conversions; conv;) {
        struct conversion *conv_to_free = conv;

        conv = conv->next;
        free(conv_to_free);
    }
}
