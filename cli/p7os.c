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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "p7os.h"
#include "common.h"

/**
 * Request user confirmation for a flashing operation interactively.
 *
 * @return 1 if the flashing is confirmed, 0 otherwise.
 */
static int confirm_flash() {
    char line[12];

    printf("Flashing is DANGEROUS and may brick your calculator.\n");
    printf("It also voids any warranty you have on it, and is entirely\n");
    printf("YOUR responsibility, and not anyone else's.\n\n");
    printf("Are you sure you want to do it? ([n]/y) ");

    if (!fgets(line, 10, stdin))
        return 0;

    return line[0] == 'y' || line[0] == 'Y';
}

/**
 * Display progress.
 *
 * @param initp Pointer to an integer (as a cookie) to set to 1 if the
 *        function has been called.
 * @param step Index of the latest accomplished step.
 * @param total Total number of steps to accomplish.
 */
static void
display_progress(int *initp, unsigned long step, unsigned long total) {
    char buf[50];
    unsigned long i, percent = 10000 * step / total;

    *initp = 1;
    sprintf(
        buf,
        "\r|---------------------------------------| %02lu.%02lu%%",
        (percent / 100) % 100,
        percent % 100
    );

    for (i = 39 * step / total; i--;)
        buf[2 + i] = '#';

    fputs(buf, stdout);
    fflush(stdout);
}

/**
 * Open a link depending on the parsed command-line.
 *
 * This function also takes care of changing the serial attributes, if the
 * opened link is on a serial medium.
 *
 * @param linkp Pointer to the link to initialize.
 * @param args Parsed parameters to base ourselves on.
 * @return Cahute error, or CAHUTE_OK if everything is ok.
 */
static int open_link(cahute_link **linkp, struct args const *args) {
    cahute_link *link = NULL;
    unsigned long flags;
    int err;

    (void)args;
    flags = CAHUTE_USB_FILTER_SEVEN;
    if ((err = cahute_open_simple_usb_link(&link, flags)))
        return err;

    *linkp = link;
    return 0;
}

/**
 * Open a link for fxRemote depending on the parsed command-line.
 *
 * This function also takes care of changing the serial attributes, if the
 * opened link is on a serial medium.
 *
 * @param linkp Pointer to the link to initialize.
 * @param args Parsed parameters to base ourselves on.
 * @return Cahute error, or CAHUTE_OK if everything is ok.
 */
static int open_fxremote_link(cahute_link **linkp, struct args const *args) {
    cahute_link *link = NULL;
    unsigned long flags;
    int err;

    (void)args;
    flags = CAHUTE_USB_NOCHECK | CAHUTE_USB_NODISC | CAHUTE_USB_NOTERM
            | CAHUTE_USB_FILTER_SEVEN;
    if ((err = cahute_open_simple_usb_link(&link, flags)))
        return err;

    *linkp = link;
    return 0;
}

/**
 * Main function.
 *
 * @param ac Argument count.
 * @param av Argument values.
 */
int main(int ac, char **av) {
    struct args args;
    cahute_link *link = NULL;
    cahute_u8 *rom = NULL;
    size_t rom_size;
    int ret = 0, err = 0;
    int progress_displayed = 0;

    if (!parse_args(ac, av, &args))
        return 0;

    if (args.upload_uexe) {
        err = open_link(&link, &args);
        if (err)
            goto end;

        if (!args.uexe_allocated_data) {
            cahute_device_info *info;

            /* We are uploading the fxRemote Update.EXE.
             * However, to be sure of what we're doing, we should actually
             * check that we are sending it to a compatible calculator. */
            err = cahute_get_device_info(link, &info);
            if (err)
                goto end;

            if (memcmp(info->cahute_device_info_hwid, "Gy36200", 7)
                && memcmp(info->cahute_device_info_hwid, "Gy36300", 7)) {
                fprintf(stderr, "Incompatible calculator detected!\n");
                fprintf(stderr, "This should only be used with Gy362 or\n");
                fprintf(stderr, "Gy363 calculator models.\n");
                goto end;
            }
        }

        err = cahute_upload_and_run_program(
            link,
            args.uexe_data,
            args.uexe_size,
            0x88024000,
            0x88024000,
            NULL,
            NULL
        );
        if (err)
            goto end;

        cahute_close_link(link);
        link = NULL;
    }

    switch (args.command) {
    case COMMAND_NONE:
        /* Nothing to do! */
        break;

    case COMMAND_BACKUP:
        err = open_link(&link, &args);
        if (err)
            goto end;

        err = cahute_backup_rom(
            link,
            &rom,
            &rom_size,
            args.display_progress ? (cahute_progress_func *)&display_progress
                                  : 0,
            &progress_displayed
        );
        if (err) {
            rom = NULL;
            goto end;
        }

        if (!fwrite(rom, rom_size, 1, args.output_fp)) {
            fprintf(
                stderr,
                "Could not write to the output file: %s\n",
                strerror(errno)
            );
            goto end;
        }
        break;

    case COMMAND_FLASH:
        if (!confirm_flash())
            goto end;

        err = open_fxremote_link(&link, &args);
        if (err)
            goto end;

        err = cahute_flash_system_using_fxremote_method(
            link,
            args.erase_flash ? CAHUTE_FLASH_FLAG_RESET_SMEM : 0,
            args.system_data,
            args.system_size
        );
        if (err)
            goto end;

        cahute_close_link(link);
        link = NULL;
        break;

    default:
        fprintf(stderr, "Command %d not implemented.\n", args.command);
        ret = 1;
    }

end:
    if (progress_displayed)
        puts(err ? "\b\b\b\b\b\bError !" : "\b\b\b\b\b\bComplete.");

    if (rom)
        free(rom);
    if (link)
        cahute_close_link(link);

    if (err) {
        if (err != CAHUTE_ERROR_ABORT)
            fprintf(stderr, "Error 0x%02X has occurred.\n", err);

        ret = 1;
    }

    free_args(&args);
    return ret;
}
