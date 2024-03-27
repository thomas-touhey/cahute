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
#define DEFAULT_PROTOCOL_BUFFER_SIZE 524288 /* 512 KiB, max. for VRAM */

/* Other protocol flags for 'initialize_link_protocol()'. */
#define PROTOCOL_FLAG_NOCHECK  0x00000100 /* Should not send initial check. */
#define PROTOCOL_FLAG_NOTERM   0x00000200 /* Should not send termination. */
#define PROTOCOL_FLAG_NODISC   0x00000400 /* Should not run discovery. */
#define PROTOCOL_FLAG_RECEIVER 0x00000800 /* Act as a receiver. */

/**
 * Cookie for detection in the context of simple USB link opening.
 *
 * @param found_bus Bus of the found USB device; -1 if no device was found.
 * @param found_address Address relative to the bus of the found USB device;
 *        -1 if no device was found.
 * @param found_type Type of the found address, -1 if not found.
 * @param multiple Flag that, if set to 1, signifies that multiple devices have
 *        already been found.
 */
struct simple_usb_detection_cookie {
    int found_bus;
    int found_address;
    int found_type;
    int multiple;
};

/**
 * Get the name of a link protocol.
 *
 * @param protocol Protocol identifier, as a constant.
 * @return Textual name of the protocol.
 */
CAHUTE_INLINE(char const *) get_protocol_name(int protocol) {
    switch (protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_AUTO:
        return "Automatic (serial)";
    case CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK:
        return "CASIOLINK (serial)";
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
        return "Protocol 7.00 (serial)";
    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP:
        return "Protocol 7.00 Screenstreaming (serial)";
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        return "Protocol 7.00 (USB)";
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP:
        return "Protocol 7.00 Screenstreaming (USB)";
    case CAHUTE_LINK_PROTOCOL_UMS:
        return "USB Mass Storage";
    case CAHUTE_LINK_PROTOCOL_UMS_OHP:
        return "USB Mass Storage (Screenstreaming)";
    default:
        return "(unknown)";
    }
}

/**
 * Initialize a link's protocol state.
 *
 * @param link Link to initialize.
 * @param flags Flags to initialize the link's protocol state with.
 * @param protocol Protocol to select.
 * @param casiolink_variant CASIOLINK variant to use, if the protocol is either
 *        automatic or CASIOLINK.
 * @return Cahute error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_LOCAL(int)
initialize_link_protocol(
    cahute_link *link,
    unsigned long flags,
    int casiolink_variant
) {
    struct cahute_casiolink_state *casiolink_state;
    struct cahute_seven_state *seven_state;
    struct cahute_seven_ohp_state *seven_ohp_state;
    int err, protocol = link->protocol;

    msg(ll_info, "Using %s.", get_protocol_name(protocol));
    msg(ll_info,
        "Playing the role of %s.",
        flags & PROTOCOL_FLAG_RECEIVER ? "receiver / passive side"
                                       : "sender / active side");

    if (~flags & PROTOCOL_FLAG_NOTERM)
        link->flags |= CAHUTE_LINK_FLAG_TERMINATE;
    if (flags & PROTOCOL_FLAG_RECEIVER)
        link->flags |= CAHUTE_LINK_FLAG_RECEIVER;

    switch (protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK:
        casiolink_state = &link->protocol_state.casiolink;

        if (link->protocol_buffer_capacity < CASIOLINK_MINIMUM_BUFFER_SIZE) {
            msg(ll_fatal,
                "CASIOLINK implementation expected a minimum protocol "
                "buffer capacity of %" CAHUTE_PRIuSIZE
                ", got %" CAHUTE_PRIuSIZE ".",
                CASIOLINK_MINIMUM_BUFFER_SIZE,
                link->protocol_buffer_capacity);
            return CAHUTE_ERROR_UNKNOWN;
        }

        casiolink_state->variant = casiolink_variant;

        if (~flags & PROTOCOL_FLAG_NOCHECK) {
            err = cahute_casiolink_initiate(link);
            if (err)
                return err;
        }

        break;

    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
        seven_state = &link->protocol_state.seven;

        if (link->protocol_buffer_capacity < SEVEN_MINIMUM_BUFFER_SIZE) {
            msg(ll_fatal,
                "Protocol 7.00 implementation expected a minimum protocol "
                "buffer capacity of %" CAHUTE_PRIuSIZE
                ", got %" CAHUTE_PRIuSIZE ".",
                SEVEN_MINIMUM_BUFFER_SIZE,
                link->protocol_buffer_capacity);
            return CAHUTE_ERROR_UNKNOWN;
        }

        seven_state->flags = 0;
        seven_state->last_packet_type = -1;
        seven_state->last_packet_subtype = -1;

        if (~flags & PROTOCOL_FLAG_NOCHECK
            && (err = cahute_seven_initiate(link)))
            return err;

        if (~flags & PROTOCOL_FLAG_NODISC
            && (err = cahute_seven_discover(link)))
            return err;

        break;

    case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP:
    case CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP:
    case CAHUTE_LINK_PROTOCOL_UMS_OHP:
        seven_ohp_state = &link->protocol_state.seven_ohp;

        /* No need to guarantee a minimum protocol buffer size here;
         * all writes to the protocol buffer will check for its capacity! */
        seven_ohp_state->last_packet_type = -1;
        memset(seven_ohp_state->last_packet_subtype, 0, 5);
        seven_ohp_state->picture_format = -1;
        seven_ohp_state->picture_width = -1;
        seven_ohp_state->picture_height = -1;
        break;

    default:
        msg(ll_error, "Protocol did not have an initialization routine!.");
        return CAHUTE_ERROR_IMPL;
    }

    return CAHUTE_OK;
}

/**
 * De-initialize a link's protocol state.
 *
 * @param link Link to deinitialize.
 * @return Cahute error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_LOCAL(int) deinitialize_link_protocol(cahute_link *link) {
    if (!(link->flags
          & (CAHUTE_LINK_FLAG_IRRECOVERABLE | CAHUTE_LINK_FLAG_TERMINATED
             | CAHUTE_LINK_FLAG_GONE))) {
        switch (link->protocol) {
        case CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK:
            if (link->flags & CAHUTE_LINK_FLAG_TERMINATE)
                cahute_casiolink_terminate(link);

            break;

        case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
        case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
            if (link->flags & CAHUTE_LINK_FLAG_TERMINATE)
                cahute_seven_terminate(link);

            break;
        }
    }

    return CAHUTE_OK;
}

/**
 * Open a link over a serial medium.
 *
 * @param linkp Pointer to the link to set with the opened link.
 * @param flags Flags to open the link and underlying medium with.
 * @param name_or_path Name or path of the serial port.
 * @param speed Speed in bauds with which to open the underlying medium.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_open_serial_link(
    cahute_link **linkp,
    unsigned long flags,
    char const *name_or_path,
    unsigned long speed
) {
    cahute_link *link = NULL;
    unsigned long protocol_flags = 0;
    unsigned long unsupported_flags;
    int protocol, casiolink_variant = 0, err = CAHUTE_OK;

    unsupported_flags =
        flags
        & ~(CAHUTE_SERIAL_PROTOCOL_MASK | CAHUTE_SERIAL_CASIOLINK_VARIANT_MASK
            | CAHUTE_SERIAL_STOP_MASK | CAHUTE_SERIAL_PARITY_MASK
            | CAHUTE_SERIAL_XONXOFF_MASK | CAHUTE_SERIAL_DTR_MASK
            | CAHUTE_SERIAL_RTS_MASK | CAHUTE_SERIAL_RECEIVER
            | CAHUTE_SERIAL_NOCHECK | CAHUTE_SERIAL_NODISC
            | CAHUTE_SERIAL_NOTERM);

    if (unsupported_flags) {
        msg(ll_error,
            "Unsupported flags %lu for serial link.",
            unsupported_flags);
        return CAHUTE_ERROR_IMPL;
    }

    switch (flags & CAHUTE_SERIAL_PROTOCOL_MASK) {
    case CAHUTE_SERIAL_PROTOCOL_AUTO:
        /* If we are to be the sender or active side, and are not allowed
         * to initiate the connection, we cannot test different things,
         * therefore this cannot be used with ``CAHUTE_SERIAL_NOCHECK``. */
        if ((~flags & CAHUTE_SERIAL_RECEIVER)
            && (flags & CAHUTE_SERIAL_NOCHECK)) {
            msg(ll_error,
                "We need the check flow to try and determine the protocol.");
            return CAHUTE_ERROR_IMPL;
        }

        protocol = CAHUTE_LINK_PROTOCOL_SERIAL_AUTO;

        /* TODO */
        msg(ll_error, "Automatic protocol detection is not yet implemented.");
        return CAHUTE_ERROR_IMPL;

    case CAHUTE_SERIAL_PROTOCOL_CASIOLINK:
        protocol = CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK;
        break;

    case CAHUTE_SERIAL_PROTOCOL_SEVEN:
        if (flags & CAHUTE_SERIAL_RECEIVER) {
            /* TODO */
            msg(ll_error,
                "Protocol 7.00 passive side is not supported for now.");
            return CAHUTE_ERROR_IMPL;
        }

        protocol = CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN;
        break;

    case CAHUTE_SERIAL_PROTOCOL_SEVEN_OHP:
        if (~flags & CAHUTE_SERIAL_RECEIVER) {
            /* TODO */
            msg(ll_error, "Only receiver is supported for screenstreaming.");
            return CAHUTE_ERROR_IMPL;
        }

        protocol = CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP;
        break;

    default:
        msg(ll_error,
            "Invalid value for protocol: 0x%08lX!",
            flags & CAHUTE_SERIAL_PROTOCOL_MASK);
        return CAHUTE_ERROR_IMPL;
    }

    if (protocol == CAHUTE_LINK_PROTOCOL_SERIAL_AUTO
        || protocol == CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK) {
        switch (flags & CAHUTE_SERIAL_CASIOLINK_VARIANT_MASK) {
        case 0:
            /* By default, if not provided, make the same choice as
             * the fx-9860G in compatibility mode, use CAS50 if sender
             * or detect if receiver. */
            if (flags & CAHUTE_SERIAL_RECEIVER)
                casiolink_variant = CAHUTE_CASIOLINK_VARIANT_AUTO;
            else
                casiolink_variant = CAHUTE_CASIOLINK_VARIANT_CAS50;

            break;

        case CAHUTE_SERIAL_CASIOLINK_VARIANT_AUTO:
            if (protocol == CAHUTE_LINK_PROTOCOL_SERIAL_AUTO
                && (~flags & CAHUTE_SERIAL_RECEIVER)) {
                msg(ll_error,
                    "Automatic data payload format detection is impossible "
                    "without receiver mode.");
                return CAHUTE_ERROR_IMPL;
            }
            break;

        case CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS40:
            casiolink_variant = CAHUTE_CASIOLINK_VARIANT_CAS40;
            break;

        case CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS50:
            casiolink_variant = CAHUTE_CASIOLINK_VARIANT_CAS50;
            break;

        case CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS100:
            casiolink_variant = CAHUTE_CASIOLINK_VARIANT_CAS100;
            break;

        default:
            msg(ll_error,
                "Invalid value for CASIOLINK variant: 0x%08lX!",
                flags & CAHUTE_SERIAL_CASIOLINK_VARIANT_MASK);
            return CAHUTE_ERROR_IMPL;
        }
    }

    switch (flags & CAHUTE_SERIAL_STOP_MASK) {
    case 0:
        /* We use a default value depending on the protocol and variant. */
        if (protocol == CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN
            || protocol == CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP
            || (protocol == CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK
                && casiolink_variant == CAHUTE_CASIOLINK_VARIANT_CAS100))
            flags |= CAHUTE_SERIAL_STOP_TWO;
        else
            flags |= CAHUTE_SERIAL_STOP_ONE;

        break;

    case CAHUTE_SERIAL_STOP_ONE:
    case CAHUTE_SERIAL_STOP_TWO:
        /* Valid values! */
        break;

    default:
        msg(ll_error,
            "Invalid value for stop bits: 0x%08lX!",
            flags & CAHUTE_SERIAL_STOP_MASK);
        return CAHUTE_ERROR_IMPL;
    }

    if ((flags & CAHUTE_SERIAL_PARITY_MASK) == 0) {
        /* We disable parity checks by default.
         * There is no other invalid value for parity. */
        flags |= CAHUTE_SERIAL_PARITY_OFF;
    }

    switch (flags & CAHUTE_SERIAL_XONXOFF_MASK) {
    case 0:
        /* We disable XON/XOFF software control by default. */
        flags |= CAHUTE_SERIAL_XONXOFF_DISABLE;
        break;

    case CAHUTE_SERIAL_XONXOFF_DISABLE:
    case CAHUTE_SERIAL_XONXOFF_ENABLE:
        /* Valid values! */
        break;

    default:
        msg(ll_error,
            "Invalid value for XON/XOFF flags: 0x%08lX!",
            flags & CAHUTE_SERIAL_XONXOFF_MASK);
        return CAHUTE_ERROR_IMPL;
    }

    if ((flags & CAHUTE_SERIAL_DTR_MASK) == 0) {
        /* We disable DTR hardware control by default. */
        flags |= CAHUTE_SERIAL_DTR_DISABLE;
    }

    if ((flags & CAHUTE_SERIAL_RTS_MASK) == 0) {
        /* We disable RTS hardware control by default. */
        flags |= CAHUTE_SERIAL_RTS_DISABLE;
    }

    switch (speed) {
    case 0:
        /* We use a default value depending on the protocol. */
        if (protocol == CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK
            && casiolink_variant == CAHUTE_CASIOLINK_VARIANT_CAS100)
            speed = 38400;
        else if (protocol == CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK && casiolink_variant == CAHUTE_CASIOLINK_VARIANT_CAS40)
            speed = 4800;
        else
            speed = 9600;

        break;

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
        /* Valid values! */
        break;

    default:
        msg(ll_error, "Unsupported baud rate %lu for serial link.", speed);
        return CAHUTE_ERROR_IMPL;
    }

#if defined(CAHUTE_LINK_STREAM_UNIX)
    int fd;

    fd = open(name_or_path, O_NOCTTY | O_RDWR);
    if (fd < 0)
        switch (errno) {
        case ENODEV:
        case ENOENT:
        case ENXIO:
        case EPIPE:
        case ESPIPE:
            msg(ll_error, "Could not open serial device: %s", strerror(errno));
            return CAHUTE_ERROR_NOT_FOUND;

        case EACCES:
            return CAHUTE_ERROR_PRIV;

        default:
            msg(ll_error, "Unknown error: %s (%d)", strerror(errno), errno);
            return CAHUTE_ERROR_UNKNOWN;
        }
#elif defined(CAHUTE_LINK_STREAM_WINDOWS)
    HANDLE handle = INVALID_HANDLE_VALUE;
    DWORD werr;

    handle = CreateFile(
        name_or_path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (handle == INVALID_HANDLE_VALUE)
        switch (werr = GetLastError()) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_DEV_NOT_EXIST:
            return CAHUTE_ERROR_NOT_FOUND;

        case ERROR_ACCESS_DENIED:
            return CAHUTE_ERROR_PRIV;

        default:
            log_windows_error("CreateFile", werr);
            return CAHUTE_ERROR_UNKNOWN;
        }
#else
    msg(ll_error, "No method for opening a serial stream available.");
    return CAHUTE_ERROR_IMPL;
#endif

    if (!(link = malloc(sizeof(cahute_link) + DEFAULT_PROTOCOL_BUFFER_SIZE)))
        err = CAHUTE_ERROR_ALLOC;

#if defined(CAHUTE_LINK_STREAM_UNIX)
    if (err) {
        close(fd);
        return err;
    }

    link->flags = CAHUTE_LINK_FLAG_CLOSE_STREAM;
    link->stream = CAHUTE_LINK_STREAM_UNIX;
    link->stream_state.posix.fd = fd;
#elif defined(CAHUTE_LINK_STREAM_WINDOWS)
    if (err) {
        CloseHandle(handle);
        return err;
    }

    link->flags = CAHUTE_LINK_FLAG_CLOSE_STREAM;
    link->stream = CAHUTE_LINK_STREAM_WINDOWS;
    link->stream_state.windows.handle = handle;
#endif

    link->serial_flags =
        flags & (CAHUTE_SERIAL_STOP_MASK | CAHUTE_SERIAL_PARITY_MASK);
    link->serial_speed = speed;
    link->protocol = protocol;
    link->protocol_buffer = (cahute_u8 *)link + sizeof(cahute_link);
    link->protocol_buffer_size = 0;
    link->protocol_buffer_capacity = DEFAULT_PROTOCOL_BUFFER_SIZE;
    link->stream_start = 0;
    link->stream_size = 0;
    link->cached_device_info = NULL;

    /* The link is now considered opened, with protocol uninitialized.
     * We want to set the serial parameters to the stream now. */
    err = cahute_set_serial_params_to_link(link, link->serial_flags, speed);
    if (err) {
        cahute_close_link(link);
        return err;
    }

    /* Now let's initialize the protocol. */
    if (flags & CAHUTE_SERIAL_NOCHECK)
        protocol_flags |= PROTOCOL_FLAG_NOCHECK;
    if (flags & CAHUTE_SERIAL_NODISC)
        protocol_flags |= PROTOCOL_FLAG_NODISC;
    if (flags & CAHUTE_SERIAL_NOTERM)
        protocol_flags |= PROTOCOL_FLAG_NOTERM;
    if (flags & CAHUTE_SERIAL_RECEIVER)
        protocol_flags |= PROTOCOL_FLAG_RECEIVER;

    err = initialize_link_protocol(link, protocol_flags, casiolink_variant);
    if (err) {
        cahute_close_link(link);
        return err;
    }

    link->flags |= CAHUTE_LINK_FLAG_CLOSE_PROTOCOL;

    *linkp = link;
    return CAHUTE_OK;
}

/**
 * Open a link over a USB medium.
 *
 * @param linkp Pointer to the link to set with the opened link.
 * @param flags Flags to open the link and underlying medium with.
 * @param bus USB bus number of the device to open.
 * @param address USB address number of the device to open, relative to the
 *        bus number.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_open_usb_link(
    cahute_link **linkp,
    unsigned long flags,
    int bus,
    int address
) {
    cahute_link *link = NULL;
    int protocol;
    unsigned long protocol_flags = 0;
    int err = CAHUTE_ERROR_UNKNOWN;

    if (flags & CAHUTE_USB_OHP) {
        if (~flags & CAHUTE_USB_RECEIVER) {
            msg(ll_error,
                "Cahute does not support acting as a sender for OHP yet.");
            return CAHUTE_ERROR_IMPL;
        }

        protocol_flags |= PROTOCOL_FLAG_RECEIVER;
    } else if (flags & CAHUTE_USB_RECEIVER) {
        msg(ll_error,
            "Cahute does not supporting acting as a receiver for control yet."
        );
        return CAHUTE_ERROR_IMPL;
    }

#if defined(CAHUTE_LINK_STREAM_LIBUSB)
    libusb_context *context = NULL;
    libusb_device **device_list = NULL;
    struct libusb_config_descriptor *config_descriptor = NULL;
    libusb_device_handle *device_handle = NULL;
    int device_count, id, libusberr, bulk_in = -1, bulk_out = -1;

    if (libusb_init(&context)) {
        msg(ll_fatal, "Could not create a libusb context.");
        goto fail;
    }

    device_count = libusb_get_device_list(context, &device_list);
    if (device_count < 0) {
        msg(ll_fatal, "Could not get a device list.");
        goto fail;
    }

    for (id = 0; id < device_count; id++) {
        struct libusb_device_descriptor device_descriptor;
        struct libusb_interface_descriptor const *interface_descriptor;
        struct libusb_endpoint_descriptor const *endpoint_descriptor;
        int interface_class = 0, i;

        if (libusb_get_bus_number(device_list[id]) != bus
            || libusb_get_device_address(device_list[id]) != address)
            continue;

        err = CAHUTE_ERROR_INCOMPAT;
        if (libusb_get_device_descriptor(device_list[id], &device_descriptor))
            goto fail;

        if (device_descriptor.idVendor != 0x07cf)
            goto fail;
        if (device_descriptor.idProduct != 0x6101
            && device_descriptor.idProduct != 0x6102)
            goto fail;

        /* We want to check the interface class of the default configuration:
         *
         * - If it's 8 (Mass Storage), then we are facing an SCSI device.
         * - If it's 255 (Vendor-Specific), then we are facing a P7 device. */
        if (libusb_get_active_config_descriptor(
                device_list[id],
                &config_descriptor
            ))
            goto fail;

        if (config_descriptor->bNumInterfaces != 1
            || config_descriptor->interface[0].num_altsetting != 1)
            goto fail;

        interface_descriptor = config_descriptor->interface[0].altsetting;
        interface_class = interface_descriptor->bInterfaceClass;

        if (interface_class == 8) {
            if (flags & CAHUTE_USB_OHP)
                protocol = CAHUTE_LINK_PROTOCOL_UMS_OHP;
            else
                protocol = CAHUTE_LINK_PROTOCOL_UMS;
        } else if (interface_class == 255) {
            if (flags & CAHUTE_USB_OHP)
                protocol = CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP;
            else
                protocol = CAHUTE_LINK_PROTOCOL_USB_SEVEN;
        } else
            goto fail;

        /* Find bulk in and out endpoints.
         * This search is in case they vary between host platforms. */
        for (endpoint_descriptor = interface_descriptor->endpoint,
            i = interface_descriptor->bNumEndpoints;
             i;
             endpoint_descriptor++, i--) {
            if ((endpoint_descriptor->bmAttributes & 3)
                != LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK)
                continue;

            switch (endpoint_descriptor->bEndpointAddress & 128) {
            case LIBUSB_ENDPOINT_OUT:
                bulk_out = endpoint_descriptor->bEndpointAddress;
                break;
            case LIBUSB_ENDPOINT_IN:
                bulk_in = endpoint_descriptor->bEndpointAddress;
                break;
            }
        }

        if (bulk_in < 0) {
            msg(ll_error, "Bulk in endpoint could not be found.");
            goto fail;
        }

        if (bulk_out < 0) {
            msg(ll_error, "Bulk out endpoint could not be found.");
            goto fail;
        }

        libusberr = libusb_open(device_list[id], &device_handle);
        switch (libusberr) {
        case 0:
            break;

        case LIBUSB_ERROR_ACCESS:
            err = CAHUTE_ERROR_PRIV;
            goto fail;

        default:
            msg(ll_error,
                "libusb_open returned %d: %s",
                libusberr,
                libusb_error_name(libusberr));
            goto fail;
        }

        break;
    }

    err = CAHUTE_ERROR_UNKNOWN;
    libusb_free_device_list(device_list, 1);
    device_list = NULL;

    if (config_descriptor) {
        libusb_free_config_descriptor(config_descriptor);
        config_descriptor = NULL;
    }

    if (!device_handle) {
        err = CAHUTE_ERROR_NOT_FOUND;
        goto fail;
    }

    /* Disconnect any kernel driver, if any. */
    libusberr = libusb_detach_kernel_driver(device_handle, 0);

    switch (libusberr) {
    case 0:
    case LIBUSB_ERROR_NOT_SUPPORTED:
    case LIBUSB_ERROR_NOT_FOUND:
        break;

    case LIBUSB_ERROR_ACCESS:
        /* On MacOS / OS X, we actually require an entitlement guaranteed
         * by code signing, and that costs money, so we just don't
         * detach the kernel driver and try to use the device directly. */
        msg(ll_warn, "Kernel driver could not be detached due to access.");
        break;

    case LIBUSB_ERROR_NO_DEVICE:
        err = CAHUTE_ERROR_NOT_FOUND;
        goto fail;

    default:
        msg(ll_fatal,
            "libusb_detach_kernel_driver returned %d: %s",
            libusberr,
            libusb_error_name(libusberr));
        goto fail;
    }

    /* Claim the interface. */
    libusberr = libusb_claim_interface(device_handle, 0);
    switch (libusberr) {
    case 0:
        break;

    case LIBUSB_ERROR_NO_DEVICE:
    case LIBUSB_ERROR_NOT_FOUND:
        err = CAHUTE_ERROR_NOT_FOUND;
        goto fail;

    case LIBUSB_ERROR_ACCESS:
        /* Same entitlement problems on MacOS / OS X. */
        msg(ll_warn, "Interface could not be claimed due to access.");
        break;

    case LIBUSB_ERROR_BUSY:
        msg(ll_info, "Another program/driver has claimed the interface.");
        err = CAHUTE_ERROR_PRIV;
        goto fail;

    default:
        msg(ll_fatal,
            "libusb_claim_interface returned %d: %s",
            libusberr,
            libusb_error_name(libusberr));
        goto fail;
    }
#else
    err = CAHUTE_ERROR_IMPL;
    goto fail;
#endif

    link = malloc(sizeof(cahute_link) + DEFAULT_PROTOCOL_BUFFER_SIZE);
    if (!link) {
        err = CAHUTE_ERROR_ALLOC;
        goto fail;
    }

#if defined(CAHUTE_LINK_STREAM_LIBUSB)
    link->flags = CAHUTE_LINK_FLAG_CLOSE_STREAM;
    link->stream = CAHUTE_LINK_STREAM_LIBUSB;
    link->stream_state.libusb.context = context;
    link->stream_state.libusb.handle = device_handle;
    link->stream_state.libusb.bulk_in = bulk_in;
    link->stream_state.libusb.bulk_out = bulk_out;

    /* Since the link now takes control over the context and device handle,
     * in case of failure, we must not free them here. */
    context = NULL;
    device_handle = NULL;

    msg(ll_info, "Bulk in endpoint address is: 0x%02X", bulk_in);
    msg(ll_info, "Bulk out endpoint address is: 0x%02X", bulk_out);
#else
    err = CAHUTE_ERROR_IMPL;
#endif

    link->serial_flags = 0;
    link->serial_speed = 0;
    link->protocol = protocol;
    link->protocol_buffer = (cahute_u8 *)link + sizeof(cahute_link);
    link->protocol_buffer_size = 0;
    link->protocol_buffer_capacity = DEFAULT_PROTOCOL_BUFFER_SIZE;
    link->stream_start = 0;
    link->stream_size = 0;
    link->cached_device_info = NULL;

    if (flags & CAHUTE_USB_NOCHECK)
        protocol_flags |= PROTOCOL_FLAG_NOCHECK;
    if (flags & CAHUTE_USB_NODISC)
        protocol_flags |= PROTOCOL_FLAG_NODISC;
    if (flags & CAHUTE_USB_NOTERM)
        protocol_flags |= PROTOCOL_FLAG_NOTERM;

    if ((err = initialize_link_protocol(link, protocol_flags, 0)))
        goto fail;

    link->flags |= CAHUTE_LINK_FLAG_CLOSE_PROTOCOL;
    *linkp = link;

    return CAHUTE_OK;

fail:
#if defined(CAHUTE_LINK_STREAM_LIBUSB)
    if (link)
        cahute_close_link(link);
    if (config_descriptor)
        libusb_free_config_descriptor(config_descriptor);
    if (device_list)
        libusb_free_device_list(device_list, 1);
    if (device_handle)
        libusb_close(device_handle);
    if (context)
        libusb_exit(context);
#endif
    return err;
}

/**
 * Get the name of a USB detection entry type.
 *
 * @param type Type identifier.
 * @return Name of the type.
 */
CAHUTE_INLINE(char const *) get_usb_detection_type_name(int type) {
    switch (type) {
    case CAHUTE_USB_DETECTION_ENTRY_TYPE_SEVEN:
        return "fx-9860G compatible calculator (Protocol 7.00)";

    case CAHUTE_USB_DETECTION_ENTRY_TYPE_SCSI:
        return "fx-CG compatible calculator (SCSI)";

    default:
        return "unknown";
    }
}

/**
 * Simple USB detection callback.
 *
 * @param cookie Simple USB detection cookie.
 * @param entry USB entry.
 */
CAHUTE_LOCAL(int)
cahute_find_simple_usb_device(
    struct simple_usb_detection_cookie *cookie,
    cahute_usb_detection_entry const *entry
) {
    if (cookie->found_bus >= 0) {
        /* A device was already found, which means there are at least two
         * connected devices! */
        if (!cookie->multiple) {
            cookie->multiple = 1;
            msg(ll_error, "Multiple devices were found:");
            msg(ll_error,
                "- %03d:%03d: %s",
                cookie->found_bus,
                cookie->found_address,
                get_usb_detection_type_name(cookie->found_type));
        }

        msg(ll_error,
            "- %03d:%03d: %s",
            entry->cahute_usb_detection_entry_bus,
            entry->cahute_usb_detection_entry_address,
            get_usb_detection_type_name(entry->cahute_usb_detection_entry_type)
        );

        return 0;
    }

    cookie->found_bus = entry->cahute_usb_detection_entry_bus;
    cookie->found_address = entry->cahute_usb_detection_entry_address;
    cookie->found_type = entry->cahute_usb_detection_entry_type;
    return 0;
}

/**
 * Open a link over a detected USB medium.
 *
 * @param linkp Pointer to the link to set with the opened link.
 * @param flags Flags to open the link and underlying medium with.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_open_simple_usb_link(cahute_link **linkp, unsigned long flags) {
    struct simple_usb_detection_cookie cookie;
    int attempts_left, err;

    for (attempts_left = 5; attempts_left; attempts_left--) {
        if (attempts_left < 5) {
            msg(ll_warn, "Calculator not found, retrying in 1 second.");

            err = cahute_sleep(1000);
            if (err)
                return err;
        }

        cookie.found_bus = -1;
        cookie.found_address = -1;
        cookie.found_type = -1;
        cookie.multiple = 0;

        err = cahute_detect_usb(
            (cahute_detect_usb_entry_func *)&cahute_find_simple_usb_device,
            &cookie
        );
        if (err)
            return err;

        if (cookie.multiple)
            return CAHUTE_ERROR_TOO_MANY;

        if (cookie.found_bus < 0)
            continue;

        return cahute_open_usb_link(
            linkp,
            flags,
            cookie.found_bus,
            cookie.found_address
        );
    }

    return CAHUTE_ERROR_NOT_FOUND;
}

/**
 * Close and free a link.
 *
 * @param link Link to close and free.
 */
CAHUTE_EXTERN(void) cahute_close_link(cahute_link *link) {
    if (!link)
        return;

    msg(ll_info, "Closing the link.");

    if (link->cached_device_info)
        free(link->cached_device_info);

    if (link->flags & CAHUTE_LINK_FLAG_CLOSE_PROTOCOL)
        deinitialize_link_protocol(link);

    if (link->flags & CAHUTE_LINK_FLAG_CLOSE_STREAM) {
        switch (link->stream) {
#ifdef CAHUTE_LINK_STREAM_UNIX
        case CAHUTE_LINK_STREAM_UNIX:
            close(link->stream_state.posix.fd);
            break;
#endif

#ifdef CAHUTE_LINK_STREAM_WINDOWS
        case CAHUTE_LINK_STREAM_WINDOWS:
            CloseHandle(link->stream_state.windows.handle);
            break;
#endif

#ifdef CAHUTE_LINK_STREAM_LIBUSB
        case CAHUTE_LINK_STREAM_LIBUSB:
            libusb_close(link->stream_state.libusb.handle);
            if (link->stream_state.libusb.context)
                libusb_exit(link->stream_state.libusb.context);
            break;
#endif
        }
    }

    free(link);
}
