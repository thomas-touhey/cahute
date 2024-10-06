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
#include <stdlib.h>
#include <string.h>

static char const error_notimplemented[] =
    "The requested operation was not implemented yet.\n";

static char const error_notfound[] =
    "Could not connect to the calculator.\n"
    "- Is it plugged in and in receive mode?\n"
    "- Have you tried changing the cable?\n";

static char const error_toomany[] =
    "Too many calculators connected by USB, please only have one connected.\n";

static char const error_disconnected[] =
    "Lost connexion to the calculator!\n"
    "Please reconnect the calculator, rerun receive mode and try again.\n";

static char const error_noaccess[] =
    "Could not get access to the calculator.\n"
    "Install the appropriate udev rule, or run as root.\n";

static char const error_busy[] =
    "The calculator is currently being used by another process.\n"
    "Please terminate that other process, then re-run the command.\n";

static char const error_unsupported[] =
    "The command is unsupported by the calculator.\n"
    "- Does the calculator have mass storage?\n"
    "- Does its OS allow the use of it?\n"
    "- Is it in Receive Mode (and not in OS Update)?\n";

static char const error_unplanned[] =
    "The calculator didn't act as planned.\n"
    "Stop receive mode on calculator and start it again before "
    "re-running %s.\n";

/**
 * Print a serial device.
 *
 * @param firstp Whether the first entry has already been printed or not,
 *        as a cookie.
 * @param entry Entry to display.
 */
static int
print_serial_device(int *firstp, cahute_serial_detection_entry const *entry) {
    if (!*firstp) {
        printf("Available devices:\n\n");
        *firstp = 1;
    }

    printf("- %s\n", entry->cahute_serial_detection_entry_name);
    return 0;
}

/**
 * Print a storage entry for file and directory listing.
 *
 * @param cookie (unused)
 * @param entry Entry.
 * @return 0, so that the file listing goes to the end.
 */
static int
print_storage_entry(void *cookie, cahute_storage_entry const *entry) {
    char const *directory = entry->cahute_storage_entry_directory;
    char const *name = entry->cahute_storage_entry_name;
    char formatted_name[30];

    (void)cookie;
    snprintf(
        formatted_name,
        28,
        "%s%s%s",
        directory ? directory : "",
        directory ? "/" : "",
        name ? name : ""
    );
    printf(
        "%-27.27s %10luo\n",
        formatted_name,
        entry->cahute_storage_entry_size
    );
    return 0;
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
 * Request user confirmation for an overwrite interactively.
 *
 * @param cookie (unused)
 * @return 1 if the overwrite is confirmed, 0 otherwise.
 */
static int confirm_overwrite(void *cookie) {
    char line[12];

    (void)cookie;

    printf("It looks like the file already exists on the calculator.\n");
    printf("Overwrite? ([n]/y) ");

    if (!fgets(line, 10, stdin))
        return 0;

    return line[0] == 'y' || line[0] == 'Y';
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
    int err;
    unsigned long flags;

    if (args->serial_name) {
        /* The user has selected a serial link! */
        flags = args->serial_flags | CAHUTE_SERIAL_PROTOCOL_CASIOLINK
                | CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS300;
        if (args->no_init)
            flags |= CAHUTE_SERIAL_NOCHECK;
        if (args->no_term)
            flags |= CAHUTE_SERIAL_NOTERM;

        err = cahute_open_serial_link(
            &link,
            flags,
            args->serial_name,
            args->serial_speed
        );
        if (err)
            return err;

        if (args->change_serial) {
            /* We want to change the serial settings as part of the
             * opening procedure. If this fails, we need to actually close
             * the link. */
            err = cahute_negotiate_serial_params(
                link,
                args->new_serial_flags,
                args->new_serial_speed
            );
            if (err) {
                cahute_close_link(link);
                return err;
            }
        }

        *linkp = link;
        return 0;
    }

    flags = CAHUTE_USB_FILTER_SERIAL | CAHUTE_USB_SEVEN;
    if (args->no_init)
        flags |= CAHUTE_USB_NOCHECK;
    if (args->no_term)
        flags |= CAHUTE_USB_NOTERM;

    if ((err = cahute_open_simple_usb_link(&link, flags)))
        return err;

    *linkp = link;
    return 0;
}

/**
 * Display device information.
 *
 * @param link Link to use to obtain the information.
 * @return Cahute error.
 */
static int print_device_info(cahute_link *link) {
    cahute_device_info *info;
    int err;

    if ((err = cahute_get_device_info(link, &info)))
        return err;

    /* Wiped out things */
    if (~info->cahute_device_info_flags & CAHUTE_DEVICE_INFO_FLAG_PREPROG)
        fprintf(
            stderr,
            "Warning: Preprogrammed ROM information looks wiped out!\n"
        );
    if (~info->cahute_device_info_flags & CAHUTE_DEVICE_INFO_FLAG_BOOTCODE)
        fprintf(stderr, "Warning: Bootcode information looks wiped out!\n");
    if (~info->cahute_device_info_flags & CAHUTE_DEVICE_INFO_FLAG_OS)
        fprintf(stderr, "Warning: OS information looks wiped out!\n");
    if (!info->cahute_device_info_username[0])
        fprintf(stderr, "Warning: Username is not set.\n");

    printf(
        "CPU ID (probably out of date): %s\n",
        info->cahute_device_info_cpuid
    );
    printf("Environnement ID: %s\n", info->cahute_device_info_hwid);
    if (info->cahute_device_info_product_id[0])
        printf("Product ID: %s\n", info->cahute_device_info_product_id);

    /* Preprogrammed ROM */
    if (info->cahute_device_info_flags & CAHUTE_DEVICE_INFO_FLAG_PREPROG) {
        printf(
            "Preprogrammed ROM version: %s",
            info->cahute_device_info_rom_version
        );
        printf(
            "\nPreprogrammed ROM capacity: %luKiB\n",
            info->cahute_device_info_rom_capacity / 1024
        );
    }

    /* ROM and RAM */
    printf(
        "ROM capacity: %luKiB\n",
        info->cahute_device_info_flash_rom_capacity / 1024
    );
    if (info->cahute_device_info_ram_capacity > 0)
        printf(
            "RAM capacity: %luKiB\n",
            info->cahute_device_info_ram_capacity / 1024
        );

    /* Bootcode */
    if (info->cahute_device_info_flags & CAHUTE_DEVICE_INFO_FLAG_BOOTCODE) {
        printf(
            "Bootcode version: %s\n",
            info->cahute_device_info_bootcode_version
        );
        if (info->cahute_device_info_bootcode_offset > 0)
            printf(
                "Bootcode offset: 0x%08lX\n",
                info->cahute_device_info_bootcode_offset
            );
        if (info->cahute_device_info_bootcode_size > 0)
            printf(
                "Bootcode size: %luKiB\n",
                info->cahute_device_info_bootcode_size / 1024
            );
    }

    /* OS */
    if (info->cahute_device_info_flags & CAHUTE_DEVICE_INFO_FLAG_OS) {
        printf("OS version: %s\n", info->cahute_device_info_os_version);
        if (info->cahute_device_info_os_offset > 0)
            printf("OS offset: 0x%08lX\n", info->cahute_device_info_os_offset);
        if (info->cahute_device_info_os_size > 0)
            printf(
                "OS size: %luKiB\n",
                info->cahute_device_info_os_size / 1024
            );
    }

    /* Miscallenous information */
    if (info->cahute_device_info_username[0])
        printf("Username: %s\n", info->cahute_device_info_username);
    if (info->cahute_device_info_organisation[0])
        printf("Organisation: %s\n", info->cahute_device_info_organisation);

    return 0;
}

/**
 * Main function.
 *
 * @param ac Argument count.
 * @param av Argument values.
 */
int main(int ac, char **av) {
    cahute_link *link = NULL;
    struct args args;
    unsigned long flags;
    int err, progress_displayed = 0;

    if (!parse_args(ac, av, &args))
        return 0;

    if (args.command == COMMAND_LIST_SERIAL) {
        /* The "list-devices" command does not require a link, and as such,
         * is processed here independently from the others. */
        int first = 0;

        err = cahute_detect_serial(
            (cahute_detect_serial_entry_func *)print_serial_device,
            &first
        );
        if (err)
            goto fail;

        if (!first)
            fprintf(stderr, "Could not find any devices.\n");

        return 0;
    }

    /* Open the link using the provided args. */
    err = open_link(&link, &args);
    if (err)
        goto fail;

    switch (args.command) {
    case COMMAND_INFO:
        err = print_device_info(link);
        break;

    case COMMAND_IDLE:
        /* Nothing to do! */
        break;

    case COMMAND_SEND:
        flags = CAHUTE_SEND_FILE_FLAG_OPTIMIZE;
        if (args.force)
            flags |=
                CAHUTE_SEND_FILE_FLAG_FORCE | CAHUTE_SEND_FILE_FLAG_DELETE;

        err = cahute_send_file_to_storage(
            link,
            flags,
            args.distant_target_directory_name,
            args.distant_target_name,
            args.storage_name,
            args.local_source_file,
            &confirm_overwrite,
            NULL,
            args.nice_display ? (cahute_progress_func *)&display_progress : 0,
            &progress_displayed
        );
        break;

    case COMMAND_GET:
        err = cahute_request_file_from_storage(
            link,
            args.distant_source_directory_name,
            args.distant_source_name,
            args.storage_name,
            args.local_target_path,
            CAHUTE_PATH_TYPE_CLI,
            args.nice_display ? (cahute_progress_func *)&display_progress : 0,
            &progress_displayed
        );
        break;

    case COMMAND_COPY:
        err = cahute_copy_file_on_storage(
            link,
            args.distant_source_directory_name,
            args.distant_source_name,
            args.distant_target_directory_name,
            args.distant_target_name,
            args.storage_name
        );
        break;

    case COMMAND_DELETE:
        err = cahute_delete_file_from_storage(
            link,
            args.distant_target_directory_name,
            args.distant_target_name,
            args.storage_name
        );
        break;

    case COMMAND_LIST:
        err = cahute_list_storage_entries(
            link,
            args.storage_name,
            &print_storage_entry,
            NULL
        );
        break;

    case COMMAND_RESET:
        err = cahute_reset_storage(link, args.storage_name);
        break;

    case COMMAND_OPTIMIZE:
        err = cahute_optimize_storage(link, args.storage_name);
        break;

    default:
        err = CAHUTE_ERROR_IMPL;
        break;
    }

    if (err && err != CAHUTE_ERROR_NOOW)
        goto fail;

    if (progress_displayed)
        puts("\b\b\b\b\b\bTransfer complete.");

    cahute_close_link(link);

    if (args.local_source_file)
        cahute_close_file(args.local_source_file);

    return 0;

fail:
    if (progress_displayed)
        puts("\b\b\b\b\b\bError !");

    /* If a link has been initialized, we want to close it. */
    if (link)
        cahute_close_link(link);

    /* If files have been opened when parsing args, we want to close them. */
    if (args.local_source_file)
        cahute_close_file(args.local_source_file);

    /* If a local target path was defined, we want to remove it. */
    if (args.local_target_path)
        remove(args.local_target_path);

    /* And now, to display an error corresponding to the obtained error. */
    switch (err) {
    case CAHUTE_ERROR_ABORT:
        break;

    case CAHUTE_ERROR_IMPL:
        fprintf(stderr, error_notimplemented);
        break;

    case CAHUTE_ERROR_PRIV:
        fprintf(stderr, error_noaccess);
        break;

    case CAHUTE_ERROR_BUSY:
        fprintf(stderr, error_busy);
        break;

    case CAHUTE_ERROR_NOT_FOUND:
        fprintf(stderr, error_notfound);
        break;

    case CAHUTE_ERROR_TOO_MANY:
        fprintf(stderr, error_toomany);
        break;

    case CAHUTE_ERROR_INCOMPAT:
        fprintf(stderr, error_unsupported);
        break;

    case CAHUTE_ERROR_GONE:
    case CAHUTE_ERROR_TERMINATED:
        fprintf(stderr, error_disconnected);
        break;

    default:
        fprintf(stderr, error_unplanned, av[0]);
    }

    return 1;
}
