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

#include "internals.h"
#define CHECK_SENDER   0x00000001 /* Check that a link is not a receiver. */
#define CHECK_RECEIVER 0x00000002 /* Check that a link is a receiver. */

/* On some platforms, for directories, ftell() returns an insanely high
 * number that may be platform-specific, e.g. 9223372036854775807 (2^63 - 1),
 * which would correspond to the highest positive value of a long if defined
 * on 64 bits. In order to detect such cases in a reasonably
 * platform-independent manner, we want to cap the size of any file that
 * gets stored into memory.
 *
 * See ``read_file_contents()`` for more details on the usage of this
 * constant. */
#define REASONABLE_FILE_CONTENT_LIMIT 134217728 /* 128 MiB */

/**
 * Check a link's state.
 *
 * @param link Link to check.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_LOCAL(int) cahute_check_link(cahute_link *link, unsigned long flags) {
    if (link->medium.flags & CAHUTE_LINK_MEDIUM_FLAG_GONE)
        return CAHUTE_ERROR_GONE;
    if (link->flags & CAHUTE_LINK_FLAG_IRRECOVERABLE)
        return CAHUTE_ERROR_IRRECOV;
    if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
        return CAHUTE_ERROR_TERMINATED;

    if ((flags & CHECK_SENDER) && (link->flags & CAHUTE_LINK_FLAG_RECEIVER))
        return CAHUTE_ERROR_UNKNOWN;
    if ((flags & CHECK_RECEIVER) && (~link->flags & CAHUTE_LINK_FLAG_RECEIVER))
        return CAHUTE_ERROR_UNKNOWN;

    return CAHUTE_OK;
}

/* ---
 * Data transfer operations.
 * --- */

/**
 * Receive data.
 *
 * @param link Link to the device.
 * @param datap Pointer to the data to set.
 * @param timeout Timeout to receive the data in.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_receive_data(
    cahute_link *link,
    cahute_data **datap,
    unsigned long timeout
) {
    int err;

    err = cahute_check_link(link, CHECK_RECEIVER);
    if (err)
        return err;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK:
        return cahute_casiolink_receive_data(link, datap, timeout);

    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_receive_data(link, datap, timeout);

    default:
        CAHUTE_RETURN_IMPL("No data reception method available.");
    }
}

/**
 * Get a screen through screenstreaming or else.
 *
 * @param link Link to the device.
 * @param framep Pointer to the frame to define.
 * @param timeout Timeout.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_receive_screen(
    cahute_link *link,
    cahute_frame **framep,
    unsigned long timeout
) {
    cahute_frame *frame = &link->stored_frame;
    int err;

    err = cahute_check_link(link, CHECK_RECEIVER);
    if (err)
        return err;

    *framep = frame;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK:
        return cahute_casiolink_receive_screen(link, frame, timeout);

    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP:
        return cahute_seven_ohp_receive_screen(link, frame, timeout);

    default:
        CAHUTE_RETURN_IMPL("No screen reception method available.");
    }
}

/* ---
 * Control operations.
 * --- */

/**
 * Update the serial parameters for the current link.
 *
 * @param link Link for which to update the serial parameters.
 * @param flags Serial flags to set to the current link.
 * @param speed Serial speed to set to the current link.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_negotiate_serial_params(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
) {
    unsigned long unsupported_flags = 0;
    unsigned long new_serial_flags = 0;
    int err;

    if (!speed)
        speed = link->medium.serial_speed;

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
        break;

    default:
        msg(ll_info, "Provided speed is %lu bauds.", speed);
        CAHUTE_RETURN_IMPL("Unsupported baud rate for the serial link.");
    }

    /* We want to check if there are unsupported flags, that is:
     *
     * - An invalid value for the stop bits flags.
     * - An invalid value for the parity flags.
     * - Any other unassigned flag we don't recognize. */
    unsupported_flags =
        flags & ~(CAHUTE_SERIAL_STOP_MASK | CAHUTE_SERIAL_PARITY_MASK);

    if ((flags & CAHUTE_SERIAL_STOP_MASK) == 0) {
        /* For ease of use of the flags by the protocol-specific function,
         * we actually want to take the current parameters. */
        flags |= link->medium.serial_flags & CAHUTE_SERIAL_STOP_MASK;
    } else if ((flags & CAHUTE_SERIAL_STOP_MASK) == CAHUTE_SERIAL_STOP_ONE) {
    } else if ((flags & CAHUTE_SERIAL_STOP_MASK) != CAHUTE_SERIAL_STOP_TWO)
        unsupported_flags |= flags & CAHUTE_SERIAL_STOP_MASK;

    if ((flags & CAHUTE_SERIAL_PARITY_MASK) == 0) {
        /* For ease of use of the flags by the protocol-specific function,
         * we actually want to take the current parameters. */
        flags |= link->medium.serial_flags & CAHUTE_SERIAL_PARITY_MASK;
    } /* No possible invalid value, 3+1 value in 2 bits. */

    if (unsupported_flags)
        CAHUTE_RETURN_IMPL("At least one unsupported flag was present.");

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    /* We want the complete serial flags here. */
    new_serial_flags = link->medium.serial_flags;
    new_serial_flags &= ~(CAHUTE_SERIAL_STOP_MASK | CAHUTE_SERIAL_PARITY_MASK);
    new_serial_flags |= flags;

    /* Now that our flags and speed has been validated, we can call our
     * protocol-specific function. */
    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
        err = cahute_seven_negotiate_serial_params(
            link,
            new_serial_flags,
            speed
        );
        if (err)
            return err;

        break;

    default:
        CAHUTE_RETURN_IMPL("Operation not supported by the link protocol.");
    }

    err = cahute_set_serial_params_to_link(link, new_serial_flags, speed);
    if (err) {
        /* We have successfully negociated with the device to switch
         * serial settings but have not managed to change settings
         * ourselves. We can no longer communicate with the device,
         * hence can no longer negotiate the serial settings back.
         * Therefore, we consider the link to be irrecoverable. */
        msg(ll_error,
            "Could not set the serial params; that makes our connection "
            "irrecoverable!");
        link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
        return err;
    }

    /* Wait until the new serial parameters have been applied by the device. */
    err = cahute_sleep(50);
    if (err)
        return err;

    return CAHUTE_OK;
}

/**
 * Get the device information regarding a given link.
 *
 * NOTE: This function does not execute any operations on the underlying
 *       medium, but instead relies on data obtained through initial
 *       discovery.
 *
 * @param link Link for which to get the information.
 * @param infop Pointer to the information pointer to set.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_get_device_info(cahute_link *link, cahute_device_info **infop) {
    int err;

    /* If the link already has cached device information, we return it.
     * NOTE: This is the reason why the user MUST NOT free the device
     * information, as indicated in the documentation!! */
    if (!link->cached_device_info) {
        err = cahute_check_link(link, CHECK_SENDER);
        if (err)
            return err;

        switch (link->protocol) {
        case CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK:
            /* With CASIOLINK, we may have received device information at
             * some point. */
            err = cahute_casiolink_make_device_info(
                link,
                &link->cached_device_info
            );
            if (err)
                return err;

            break;

        case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
        case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
            /* With Protocol 7.00, we may already have device information
             * from discovery, and want to make it into a generic
             * device information structure. */
            err =
                cahute_seven_make_device_info(link, &link->cached_device_info);
            if (err)
                return err;

            break;

        default:
            /* With other protocols, we don't have a way to get device
             * information as of today. */
            CAHUTE_RETURN_IMPL("Operation not supported by the link protocol."
            );
        }
    }

    *infop = link->cached_device_info;
    return CAHUTE_OK;
}

/**
 * Request the currently available capacity on the given storage device.
 *
 * @param link Link to use to send the file.
 * @param storage Name of the storage device.
 * @param capacityp Capacity to fill.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_request_storage_capacity(
    cahute_link *link,
    char const *storage,
    unsigned long *capacityp
) {
    int err;

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_request_storage_capacity(link, storage, capacityp);

    default:
        CAHUTE_RETURN_IMPL("Operation not supported by the link protocol.");
    }
}

/**
 * Send a file to the calculator's storage.
 *
 * @param link Link to use to send the file.
 * @param flags Usage flags.
 * @param directory Name of the directory to place the file into,
 *        NULL if at root.
 * @param name Name of the file to place the file as.
 * @param storage Storage on which to place the file.
 * @param filep File pointer.
 * @param overwrite_func Function to call to confirm overwrite.
 * @param overwrite_cookie Cookie to pass to the overwrite confirmation
 *        function.
 * @param progress_func Function to call to signify progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_send_file_to_storage(
    cahute_link *link,
    unsigned long flags,
    char const *directory,
    char const *name,
    char const *storage,
    FILE *filep,
    cahute_confirm_overwrite_func *overwrite_func,
    void *overwrite_cookie,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    size_t file_size;
    unsigned long unsupported_flags =
        (flags
         & ~(CAHUTE_SEND_FILE_FLAG_FORCE | CAHUTE_SEND_FILE_FLAG_OPTIMIZE
             | CAHUTE_SEND_FILE_FLAG_DELETE));
    int err;

    if (unsupported_flags) {
        msg(ll_error, "Unsupported flags: 0x%08lX", unsupported_flags);
        return CAHUTE_ERROR_UNKNOWN;
    }

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    /* Compute the file size. */
    if (fseek(filep, 0L, SEEK_END) < 0) {
        msg(ll_fatal,
            "Cannot seek on the provided file pointer; is it a seekable "
            "standard stream?");
        return CAHUTE_ERROR_UNKNOWN;
    }

    file_size = (size_t)ftell(filep);
    if (file_size > REASONABLE_FILE_CONTENT_LIMIT) {
        msg(ll_error,
            "file too big (over 128MiB) or unsupported file type (e.g. "
            "directory)");
        return CAHUTE_ERROR_UNKNOWN;
    }

    rewind(filep);

    /* Send the file using the protocol. */
    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_send_file_to_storage(
            link,
            flags,
            directory,
            name,
            storage,
            filep,
            file_size,
            overwrite_func,
            overwrite_cookie,
            progress_func,
            progress_cookie
        );

    default:
        CAHUTE_RETURN_IMPL("Operation not supported by the link protocol.");
    }
}

/**
 * Request for a file from a storage on the calculator.
 *
 * @param link Link to the device.
 * @param directory Optional name of the directory.
 * @param name Name of the file.
 * @param storage Name of the storage device.
 * @param filep File pointer to write to.
 * @param progress_func Function to call to signify progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_request_file_from_storage(
    cahute_link *link,
    char const *directory,
    char const *name,
    char const *storage,
    FILE *filep,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    int err;

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_request_file_from_storage(
            link,
            directory,
            name,
            storage,
            filep,
            progress_func,
            progress_cookie
        );

    default:
        CAHUTE_RETURN_IMPL("Operation not supported by the link protocol.");
    }
}

/**
 * Request for a file to be copied on the calculator.
 *
 * @param link Link on which to make the request.
 * @param source_directory Directory in which the source file is present.
 * @param source_name Name of the source file.
 * @param target_directory Directory in which the copy should be placed into.
 * @param target_name Name of the copied file.
 * @param storage Name of the storage device.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_copy_file_on_storage(
    cahute_link *link,
    char const *source_directory,
    char const *source_name,
    char const *target_directory,
    char const *target_name,
    char const *storage
) {
    int err;

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_copy_file_on_storage(
            link,
            source_directory,
            source_name,
            target_directory,
            target_name,
            storage
        );

    default:
        CAHUTE_RETURN_IMPL("Operation not supported by the link protocol.");
    }
}

/**
 * Request for a file to be deleted from a storage device on the calculator.
 *
 * @param link Link to use to send the file.
 * @param directory Directory name in which to delete the file.
 * @param name Name of the file to delete.
 * @param storage Name of the storage device.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_delete_file_from_storage(
    cahute_link *link,
    char const *directory,
    char const *name,
    char const *storage
) {
    int err;

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_delete_file_from_storage(
            link,
            directory,
            name,
            storage
        );

    default:
        CAHUTE_RETURN_IMPL("Operation not supported by the link protocol.");
    }
}

/**
 * List files and directories on a storage device on the calculator.
 *
 * @param link Link to the device.
 * @param storage Name of the storage device.
 * @param callback Callback function.
 * @param cookie Cookie to pass to the callback function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_list_storage_entries(
    cahute_link *link,
    char const *storage,
    cahute_list_storage_entry_func *callback,
    void *cookie
) {
    int err;

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_list_storage_entries(
            link,
            storage,
            callback,
            cookie
        );

    default:
        CAHUTE_RETURN_IMPL("Operation not supported by the link protocol.");
    }
}

/**
 * Reset a storage device on the calculator.
 *
 * @param link Link to the device.
 * @param storage Storage to reset.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_reset_storage(cahute_link *link, char const *storage) {
    int err;

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_reset_storage(link, storage);

    default:
        CAHUTE_RETURN_IMPL("Operation not supported by the link protocol.");
    }
}

/**
 * Request for a storage device to be optimized by the calculator.
 *
 * @param link Link to use to send the file.
 * @param storage Storage on which to place the file.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_optimize_storage(cahute_link *link, char const *storage) {
    int err;

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_optimize_storage(link, storage);

    default:
        CAHUTE_RETURN_IMPL("Operation not supported by the link protocol.");
    }
}

/**
 * Backup the ROM from the calculator.
 *
 * @param link Link to the calculator.
 * @param romp Pointer to the ROM to allocate.
 * @param sizep Pointer to the ROM size to define.
 * @param progress_func Function to call to signify progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_backup_rom(
    cahute_link *link,
    cahute_u8 **romp,
    size_t *sizep,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    int err;

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_backup_rom(
            link,
            romp,
            sizep,
            progress_func,
            progress_cookie
        );

    default:
        CAHUTE_RETURN_IMPL("Operation not supported by the link protocol.");
    }
}

/**
 * Upload and run a program on the calculator.
 *
 * @param link Link to the calculator.
 * @param program Program to upload and run.
 * @param program_size Size to the program to upload and run.
 * @param load_address Address at which to load the program.
 * @param start_address Address at which to start the program.
 * @param progress_func Function to call to signify progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_upload_and_run_program(
    cahute_link *link,
    cahute_u8 const *program,
    size_t program_size,
    unsigned long load_address,
    unsigned long start_address,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    int err;

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_upload_and_run_program(
            link,
            program,
            program_size,
            load_address,
            start_address,
            progress_func,
            progress_cookie
        );

    default:
        CAHUTE_RETURN_IMPL("Operation not supported by the link protocol.");
    }
}

/**
 * Flash using the fxRemote method.
 *
 * @param link Link to the calculator.
 * @param flags Flags.
 * @param system System image to flash.
 * @param system_size Size of the system image to flash.
 */
CAHUTE_EXTERN(int)
cahute_flash_system_using_fxremote_method(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 const *system,
    size_t system_size
) {
    int err;

    err = cahute_check_link(link, CHECK_SENDER);
    if (err)
        return err;

    switch (link->protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return cahute_seven_flash_system_using_fxremote_method(
            link,
            flags,
            system,
            system_size
        );

    default:
        CAHUTE_RETURN_IMPL("Operation not supported by the link protocol.");
    }
}
