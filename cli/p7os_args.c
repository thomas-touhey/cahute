/* ****************************************************************************
 * Copyright (C) 2017, 2024 Thomas Touhey <thomas@touhey.fr>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "p7os.h"
#include "options.h"
#include "common.h"

extern size_t const cahute_fxremote_update_exe_size;
extern cahute_u8 const cahute_fxremote_update_exe[];

static char const version_message[] =
    "p7os - from Cahute v" CAHUTE_VERSION
    " (licensed under CeCILL 2.1)\n"
    "\n"
    "This is free software; see the source for copying conditions.\n"
    "There is NO warranty; not even for MERCHANTABILITY or\n"
    "FITNESS FOR A PARTICULAR PURPOSE.\n";

static char const help_main[] =
    "Usage: %s\n"
    "            [--help|-h] [--version|-v]\n"
    "            [--no-prepare] [--uexe <path>]\n"
    "            <subcommand> [options...]\n"
    "\n"
    "This program interacts with a CASIO calculator's firmware.\n"
    "Keep in mind that using it is HIGHLY DANGEROUS and could easily brick "
    "your\n"
    "calculator if you aren't careful enough. AVOID USING IT IF YOU DO NOT\n"
    "KNOW WHAT YOU'RE DOING.\n"
    "\n"
    "Subcommands you can use are :\n"
    "   prepare-only      Set-up the update program, but leave it for other\n"
    "                     programs to interact with it.\n"
    "   get               Get the OS image.\n"
    "   flash             Flash the OS image.\n"
    "\n"
    "General options:\n"
    "  -h, --help        Display the help page of the (sub)command and quit.\n"
    "  -v, --version     Display the version message and quit.\n"
    "  -l <level>, --log <level>\n"
    "                    The library log level (default: %s).\n"
    "                    One of: info, warning, error, fatal, none.\n"
    "  -#                Display a nice progress bar.\n"
    "  --no-prepare      Use the current environment, instead of uploading "
    "one.\n"
    "  -u, --uexe <path> Use a custom update program.\n"
    "                    If `--no-prepare` is not given, this option is\n"
    "                    required.\n"
    "\n"
    "Type \"%s <subcommand> --help\" for some help about a subcommand.\n"
    "\n"
    "For guides, topics and reference, consult the documentation:\n"
    "    " CAHUTE_URL
    "\n"
    "\n"
    "For reporting issues and vulnerabilities, consult the following guide:\n"
    "    " CAHUTE_ISSUES_URL "\n";

#define SUBCOMMAND_FOOTER \
    "\nType \"%s --help\" for other subcommands and general options.\n"

static char const help_prepare_only[] =
    "Usage: %s prepare-only\n"
    "Upload and run the Update.EXE on the calculator for further operations.\n"
    "This must be used before any other p7os operation.\n" SUBCOMMAND_FOOTER;

static char const help_get[] =
    "Usage: %s get [-o <os.bin>]\n"
    "Get the calculator OS image.\n"
    "\n"
    "Options are :\n"
    "  -o <os.bin>       Where to store the image (default is "
    "\"os.bin\")\n" SUBCOMMAND_FOOTER;

static char const help_flash[] =
    "Usage: %s flash <rom.bin>\n"
    "Flash the calculator's OS image.\n"
    "\n"
    "Available options:\n"
    "  --erase-flash     Instead of 0xA0270000 the last erase addr is "
    "0xA0400000.\n" SUBCOMMAND_FOOTER;

/**
 * Short option definitions.
 */
static struct short_option const short_options[] = {
    {'h', 0},
    {'v', 0},
    {'l', OPTION_FLAG_PARAMETER_REQUIRED},
    {'u', OPTION_FLAG_PARAMETER_REQUIRED},
    {'o', OPTION_FLAG_PARAMETER_REQUIRED},
    {'#', 0},

    SHORT_OPTION_SENTINEL
};

/**
 * Long option definitions.
 */
static struct long_option const long_options[] = {
    {"help", 0, 'h'},
    {"version", 0, 'v'},
    {"log", OPTION_FLAG_PARAMETER_REQUIRED, 'l'},
    {"no-prepare", 0, 'n'},
    {"erase-flash", 0, 'e'},
    {"uexe", OPTION_FLAG_PARAMETER_REQUIRED, 'u'},
    {"output", OPTION_FLAG_PARAMETER_REQUIRED, 'o'},

    LONG_OPTION_SENTINEL
};

/**
 * Parse the command-line arguments.
 *
 * @param argc Argument count.
 * @param argv Argument values.
 * @param args Parsed arguments.
 * @return 0 if invalid, 1 if ok.
 */
int parse_args(int argc, char **argv, struct args *args) {
    struct option_parser_state state;
    char const *command = argv[0], *subcommand;
    char const *uexe_path = NULL, *output_path = "os.bin";
    char *optarg;
    int option, optopt, help = 0, version = 0;

    /* Default parsed arguments. */
    args->command = COMMAND_NONE;
    args->upload_uexe = 1;
    args->erase_flash = 0;
    args->display_progress = 0;
    args->uexe_data = cahute_fxremote_update_exe;
    args->uexe_allocated_data = NULL;
    args->uexe_size = cahute_fxremote_update_exe_size;
    args->system_data = NULL;
    args->system_size = 0;
    args->output_fp = NULL;

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
            version = 1;
            break;

        case 'l':
            /* -l, --log: set the logging level. */
            set_log_level(optarg);
            break;

        case 'n':
            /* --no-prepare: Disable preparation. */
            args->upload_uexe = 0;
            break;

        case 'e':
            /* --erase-flash: Erase the flash memory before setting it. */
            args->erase_flash = 1;
            break;

        case 'u':
            /* -u, --uexe: Path to the Update.EXE to upload. */
            uexe_path = optarg;
            break;

        case 'o':
            /* -o, --output: Path to the output. */
            output_path = optarg;
            break;

        case '#':
            /* -#: enable progress bar display. */
            args->display_progress = 1;
            break;

        case GETOPT_FAIL:
            /* Erroneous option usage. */
            if (optopt == 'l')
                fprintf(stderr, "-l, --log: expected an argument\n");
            else if (optopt == 'u')
                fprintf(stderr, "-u, --uexe: expected an argument\n");
            else if (optopt == 'o')
                fprintf(stderr, "-o, --output: expected an argument\n");
            else
                /* We ignore unknown options. */
                break;

            return 0;
        }
    }

    if (version) {
        printf(version_message);
        return 0;
    }

    update_positional_parameters(&state, &argc, &argv);
    if (!argc || !strcmp(argv[0], "help")) {
        printf(help_main, command, get_current_log_level(), command);
        return 0;
    }

    subcommand = argv[0];
    argc--;
    argv++;
    if (!strcmp(subcommand, "version")) {
        printf(version_message);
        return 0;
    }

    if (!strcmp(subcommand, "prepare-only")) {
        /* We also display the help message if '--no-prepare' has been passed,
         * because seriously, 'prepare-only --no-prepare'? That's silly. */
        if (help || argc != 0 || !args->upload_uexe) {
            printf(help_prepare_only, command, command);
            return 0;
        }

        args->command = COMMAND_NONE;
    } else if (!strcmp(subcommand, "get")) {
        if (help || argc != 0) {
            printf(help_get, command, command);
            return 0;
        }

        args->command = COMMAND_BACKUP;
        args->output_fp = fopen(output_path, "wb");
        args->upload_uexe = 0;
        if (!args->output_fp) {
            fprintf(
                stderr,
                "Could not open the output: %s\n",
                strerror(errno)
            );
            goto fail;
        }
    } else if (!strcmp(subcommand, "flash")) {
        if (help || argc != 1) {
            printf(help_flash, command, command);
            return 0;
        }

        args->command = COMMAND_FLASH;

        if (read_file_contents(
                argv[0],
                &args->system_data,
                &args->system_size
            ))
            goto fail;
    } else {
        /* The subcommand is unknown. */
        printf(help_main, command, get_current_log_level(), command);
        return 0;
    }

    if (!uexe_path) {
        /* No update.exe to read. */
    } else if (!args->upload_uexe) {
        fprintf(
            stderr,
            "warning: update.exe path passed, but ignored since --no-prepare "
            "is present.\n"
        );
    } else {
        if (read_file_contents(
                uexe_path,
                &args->uexe_allocated_data,
                &args->uexe_size
            ))
            goto fail;

        if (args->uexe_size > 65536) {
            fprintf(
                stderr,
                "Update.exe is too big (should be less than 64 KiB, is "
                "%" CAHUTE_PRIuSIZE "B)\n",
                args->uexe_size
            );
            goto fail;
        }

        args->uexe_data = args->uexe_allocated_data;
    }

    return 1;

fail:
    free_args(args);
    return 0;
}

/**
 * Free the args.
 *
 * @param args Parsed argument pointer.
 */
void free_args(struct args *args) {
    if (args->output_fp)
        fclose(args->output_fp);
    if (args->system_data)
        free(args->system_data);
    if (args->uexe_allocated_data)
        free(args->uexe_allocated_data);

    args->output_fp = NULL;
    args->system_data = NULL;
    args->uexe_allocated_data = NULL;
}
