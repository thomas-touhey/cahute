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

/* Full check packets. */
CAHUTE_LOCAL_DATA(cahute_u8 const)
seven_check_packet[] = {5, '0', '0', '0', '7', '0'};
CAHUTE_LOCAL_DATA(cahute_u8 const)
seven_ack_packet[] = {6, '0', '0', '0', '7', '0'};
CAHUTE_LOCAL_DATA(cahute_u8 const) casiolink_start_packet[] = {0x16};

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
 * Determine the protocol for a serial link as a receiver.
 *
 * @param link Link to initialize.
 * @param protocolp Pointer to the protocol to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
determine_protocol_as_receiver(cahute_link *link, int *protocolp) {
    cahute_u8 buf[6];
    size_t received = 1;
    int err;

    msg(ll_info, "Waiting for input to determine the protocol.");

    err = cahute_read_from_link(link, buf, 1, 0, 0);
    if (err)
        goto fail;

    if (buf[0] == 0x05) {
        /* This is the beginning of a Protocol 7.00 check packet.
         * We want to read the rest of the packet to ensure that
         * everything is correct. */
        err = cahute_read_from_link(link, &buf[1], 5, 0, 0);
        if (err)
            goto fail;

        received = 6;
        if (!memcmp(buf, seven_check_packet, 6)) {
            /* That's a check packet! We can answer with an ACK, then
             * set the protocol to Protocol 7.00. */
            err = cahute_write_to_link(link, seven_ack_packet, 6);
            if (err)
                goto fail;

            *protocolp = CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN;
            return CAHUTE_OK;
        }
    } else if (buf[0] == 0x0B) {
        /* This is the beginning of a Protocol 7.00 Screenstreaming packet.
         * We don't want to read the rest of the packet, the receiving routine
         * for Protocol 7.00 screenstreaming will realign itself. */
        *protocolp = CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP;
        return CAHUTE_OK;
    } else if (buf[0] == 0x16) {
        /* This is a CASIOLINK start packet.
         * We can answer with an 'established' packet and set the protocol
         * to CASIOLINK. */
        buf[0] = 0x13;

        err = cahute_write_to_link(link, buf, 1);
        if (err)
            goto fail;

        *protocolp = CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK;
        return CAHUTE_OK;
    }

    msg(ll_error, "Unable to determine a protocol out of the following:");
    mem(ll_error, buf, received);

    err = CAHUTE_ERROR_UNKNOWN;
fail:
    if (err == CAHUTE_ERROR_TIMEOUT_START)
        err = CAHUTE_ERROR_TIMEOUT;
    return err;
}

/**
 * Determine the protocol for a serial link as a sender or control.
 *
 * @param link Link to initialize.
 * @param protocolp Pointer to the protocol to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
determine_protocol_as_sender(cahute_link *link, int *protocolp) {
    cahute_u8 buf[6];
    size_t received = 1;
    int err, attempts;

    for (attempts = 3; attempts; attempts--) {
        /* Try writing a Protocol 7.00 check packet to see if we get an
         * answer. */
        msg(ll_info, "Sending the Protocol 7.00 check packet:");
        mem(ll_info, seven_check_packet, 6);

        err = cahute_write_to_link(link, seven_check_packet, 6);
        if (err)
            return err;

        err = cahute_read_from_link(link, buf, 1, 100, 0);
        if (!err)
            break;
        else if (err != CAHUTE_ERROR_TIMEOUT_START)
            return err;

        /* Try writing a CASIOLINK start packet to see if we get an answer. */
        msg(ll_info, "Sending the CASIOLINK check packet:");
        mem(ll_info, casiolink_start_packet, 1);
        err = cahute_write_to_link(link, casiolink_start_packet, 1);
        if (err)
            return err;

        err = cahute_read_from_link(link, buf, 1, 300, 0);
        if (!err)
            break;
        else if (err != CAHUTE_ERROR_TIMEOUT_START)
            return err;
    }

    if (!attempts) {
        msg(ll_error, "No answer detected, protocol could not be determined.");
        return CAHUTE_ERROR_NOT_FOUND;
    }

    if (buf[0] == 0x06) {
        /* This is the beginning of a Protocol 7.00 ack packet.
         * We want to read the rest of the packet to ensure that
         * everything is correct. */
        err = cahute_read_from_link(link, &buf[1], 5, 0, 0);
        if (err)
            goto fail;

        received = 6;
        if (!memcmp(buf, seven_ack_packet, 6)) {
            /* That's a check packet! We can answer with an ACK, then
             * set the protocol to Protocol 7.00. */
            *protocolp = CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN;
            return CAHUTE_OK;
        }
    } else if (buf[0] == 0x13) {
        /* This is a CASIOLINK start packet.
         * We can answer with an 'established' packet and set the protocol
         * to CASIOLINK. */
        *protocolp = CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK;
        return CAHUTE_OK;
    }

    msg(ll_error,
        "Unable to determine a protocol out of the received packet:");
    mem(ll_error, buf, received);
    err = CAHUTE_ERROR_UNKNOWN;

fail:
    if (err == CAHUTE_ERROR_TIMEOUT_START)
        err = CAHUTE_ERROR_TIMEOUT;
    return err;
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

    if (~flags & PROTOCOL_FLAG_NOTERM)
        link->flags |= CAHUTE_LINK_FLAG_TERMINATE;
    if (flags & PROTOCOL_FLAG_RECEIVER)
        link->flags |= CAHUTE_LINK_FLAG_RECEIVER;

    if (protocol == CAHUTE_LINK_PROTOCOL_SERIAL_AUTO) {
        if (flags & PROTOCOL_FLAG_RECEIVER)
            err = determine_protocol_as_receiver(link, &protocol);
        else
            err = determine_protocol_as_sender(link, &protocol);

        if (err)
            return err;

        /* The protocol has been found using automatic discovery, by tweaking
         * the check handshake! It should not be re-done. */
        link->protocol = protocol;
        flags |= PROTOCOL_FLAG_NOCHECK;
    }

    msg(ll_info, "Using %s.", get_protocol_name(protocol));
    msg(ll_info,
        "Playing the role of %s.",
        flags & PROTOCOL_FLAG_RECEIVER ? "receiver / passive side"
                                       : "sender / active side");

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

        if (~flags & PROTOCOL_FLAG_NOCHECK) {
            err = cahute_seven_initiate(link);
            if (err)
                return err;
        }

        if (~flags & PROTOCOL_FLAG_RECEIVER
            && (~flags & PROTOCOL_FLAG_NODISC)) {
            err = cahute_seven_discover(link);
            if (err)
                return err;
        }

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
        CAHUTE_RETURN_IMPL("No initialization routine for the protocol.");
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
             | CAHUTE_LINK_FLAG_GONE | CAHUTE_LINK_FLAG_RECEIVER))) {
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

#if WINDOWS_ENABLED
# include <initguid.h>
# include <cfgmgr32.h>
# include <usbiodef.h>
# include <devpkey.h>
# define ASCII_HEX_TO_NIBBLE(C) ((C) >= 'A' ? (C) - 'A' + 10 : (C) - '0')

/**
 * Parse device location for a Windows USB device.
 *
 * The expected device location format is something like
 * "Port_#0002.Hub_#000D".
 *
 * @param raw Raw device location for the Windows USB device.
 * @param busp Pointer to the bus to define.
 * @param addressp Pointer to the address to define.
 * @return 0 if no error, other otherwise.
 */
CAHUTE_LOCAL(int)
parse_winusb_device_location(WCHAR const *raw, int *busp, int *addressp) {
    if (raw[0] != 'P' || raw[1] != 'o' || raw[2] != 'r' || raw[3] != 't'
        || raw[4] != '_' || raw[5] != '#' || !isxdigit(raw[6])
        || !isxdigit(raw[7]) || !isxdigit(raw[8]) || !isxdigit(raw[9])
        || raw[10] != '.' || raw[11] != 'H' || raw[12] != 'u' || raw[13] != 'b'
        || raw[14] != '_' || raw[15] != '#' || !isxdigit(raw[16])
        || !isxdigit(raw[17]) || !isxdigit(raw[18]) || !isxdigit(raw[19])
        || raw[20])
        return CAHUTE_ERROR_UNKNOWN;

    *addressp = (ASCII_HEX_TO_NIBBLE(raw[6]) << 24)
                | (ASCII_HEX_TO_NIBBLE(raw[7]) << 16)
                | (ASCII_HEX_TO_NIBBLE(raw[8]) << 8)
                | ASCII_HEX_TO_NIBBLE(raw[9]);
    *busp = (ASCII_HEX_TO_NIBBLE(raw[16]) << 24)
            | (ASCII_HEX_TO_NIBBLE(raw[17]) << 16)
            | (ASCII_HEX_TO_NIBBLE(raw[18]) << 8)
            | ASCII_HEX_TO_NIBBLE(raw[19]);
    return 0;
}
#endif

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
    int protocol, casiolink_variant = CAHUTE_CASIOLINK_VARIANT_AUTO,
                  err = CAHUTE_OK;

    unsupported_flags =
        flags
        & ~(CAHUTE_SERIAL_PROTOCOL_MASK | CAHUTE_SERIAL_CASIOLINK_VARIANT_MASK
            | CAHUTE_SERIAL_STOP_MASK | CAHUTE_SERIAL_PARITY_MASK
            | CAHUTE_SERIAL_XONXOFF_MASK | CAHUTE_SERIAL_DTR_MASK
            | CAHUTE_SERIAL_RTS_MASK | CAHUTE_SERIAL_RECEIVER
            | CAHUTE_SERIAL_NOCHECK | CAHUTE_SERIAL_NODISC
            | CAHUTE_SERIAL_NOTERM);

    if (unsupported_flags)
        CAHUTE_RETURN_IMPL("At least one unsupported flag was present.");

    switch (flags & CAHUTE_SERIAL_PROTOCOL_MASK) {
    case CAHUTE_SERIAL_PROTOCOL_AUTO:
        /* If we are to be the sender or active side, and are not allowed
         * to initiate the connection, we cannot test different things,
         * therefore this cannot be used with ``CAHUTE_SERIAL_NOCHECK``. */
        if ((~flags & CAHUTE_SERIAL_RECEIVER)
            && (flags & CAHUTE_SERIAL_NOCHECK)) {
            msg(ll_error, "We need the check flow to determine the protocol.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        protocol = CAHUTE_LINK_PROTOCOL_SERIAL_AUTO;
        break;

    case CAHUTE_SERIAL_PROTOCOL_CASIOLINK:
        protocol = CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK;
        break;

    case CAHUTE_SERIAL_PROTOCOL_SEVEN:
        /* TODO */
        if (flags & CAHUTE_SERIAL_RECEIVER)
            CAHUTE_RETURN_IMPL(
                "Protocol 7.00 passive side is not supported for now."
            );

        protocol = CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN;
        break;

    case CAHUTE_SERIAL_PROTOCOL_SEVEN_OHP:
        /* TODO */
        if (~flags & CAHUTE_SERIAL_RECEIVER)
            CAHUTE_RETURN_IMPL(
                "Only receiver is supported for screenstreaming."
            );

        protocol = CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP;
        break;

    default:
        CAHUTE_RETURN_IMPL("Unsupported serial protocol.");
    }

    if (protocol == CAHUTE_LINK_PROTOCOL_SERIAL_AUTO
        || protocol == CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK) {
        switch (flags & CAHUTE_SERIAL_CASIOLINK_VARIANT_MASK) {
        case 0:
            /* By default, if not provided, make the same choice as
             * the fx-9860G in compatibility mode, use CAS50 if sender
             * or detect if receiver. */
            if (~flags & CAHUTE_SERIAL_RECEIVER)
                casiolink_variant = CAHUTE_CASIOLINK_VARIANT_CAS50;

            break;

        case CAHUTE_SERIAL_CASIOLINK_VARIANT_AUTO:
            if (protocol == CAHUTE_LINK_PROTOCOL_SERIAL_AUTO
                && (~flags & CAHUTE_SERIAL_RECEIVER)) {
                msg(ll_error,
                    "Automatic data payload format detection is impossible "
                    "without receiver mode.");
                return CAHUTE_ERROR_UNKNOWN;
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
            CAHUTE_RETURN_IMPL("Unsupported CASIOLINK variant.");
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
        CAHUTE_RETURN_IMPL("Unsupported value for stop bits.");
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
        CAHUTE_RETURN_IMPL("Unsupported XON/XOFF mode.");
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
        CAHUTE_RETURN_IMPL("Unsupported serial speed.");
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

    /* Read timeouts will be managed by using WaitForMultipleObjects().
     * Here we only need to configure the timeouts to return immediately. */
    {
        COMMTIMEOUTS timeouts;

        timeouts.ReadIntervalTimeout = MAXDWORD;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.ReadTotalTimeoutConstant = 0;
        timeouts.WriteTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 0;

        if (!SetCommTimeouts(handle, &timeouts)) {
            log_windows_error("SetCommTimeouts", GetLastError());
            CloseHandle(handle);
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    /* We only want events to be set if we are receiving a byte. */
    if (!SetCommMask(handle, EV_RXCHAR)) {
        log_windows_error("SetCommMask", GetLastError());
        CloseHandle(handle);
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* TODO: Implement timeouts with Windows streams. */
    msg(ll_warn, "Streams using the Windows API do not support timeouts.");
#else
    CAHUTE_RETURN_IMPL("No serial device opening method available.");
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
    link->stream_state.windows.is_cesg = 0;
#endif

    link->serial_flags = flags
                         & (CAHUTE_SERIAL_STOP_MASK | CAHUTE_SERIAL_PARITY_MASK
                            | CAHUTE_SERIAL_XONXOFF_MASK
                            | CAHUTE_SERIAL_DTR_MASK | CAHUTE_SERIAL_RTS_MASK);
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
    int i, err = CAHUTE_ERROR_UNKNOWN;

#if WINDOWS_ENABLED
    HANDLE cesg_handle = INVALID_HANDLE_VALUE;
    PWSTR device_interface_list = NULL, device_interface;
    ULONG device_interface_list_size;
    CONFIGRET cret;
    DWORD werr;
#endif

#if defined(CAHUTE_LINK_STREAM_LIBUSB)
    libusb_context *context = NULL;
    libusb_device **device_list = NULL;
    struct libusb_config_descriptor *config_descriptor = NULL;
    libusb_device_handle *device_handle = NULL;
    int device_count, libusberr, bulk_in = -1, bulk_out = -1;
#endif

    if (flags & CAHUTE_USB_OHP) {
        /* TODO */
        if (~flags & CAHUTE_USB_RECEIVER)
            CAHUTE_RETURN_IMPL("Sender mode not available for screenstreaming."
            );

        protocol_flags |= PROTOCOL_FLAG_RECEIVER;
    } else if (flags & CAHUTE_USB_RECEIVER)
        CAHUTE_RETURN_IMPL("Receiver mode not available for data protocols.");

#if WINDOWS_ENABLED
# define DEV_INTERFACE_DETAIL_DATA_SIZE 1024
    /* The device may actuallydevice_interface_list be managed by the CESG502
     * driver. We need to explore USB devices manually to find out.
     *
     * This uses the more portable CfgMgr32 API rather than SetupApi,
     * more specifically the "Get a list of interfaces, get the device exposing
     * each interface, and get a property from the device" use case, as
     * described here:
     *
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/install/
     * porting-from-setupapi-to-cfgmgr32 */
    cret = CM_Get_Device_Interface_List_SizeW(
        &device_interface_list_size,
        (LPGUID)&GUID_DEVINTERFACE_USB_DEVICE,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
    );
    if (cret != CR_SUCCESS) {
        msg(ll_error,
            "CM_Get_Device_Interface_List_SizeW returned error 0x%08lX.",
            cret);
        goto fail;
    }

    device_interface_list = (PWSTR)HeapAlloc(
        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        device_interface_list_size * sizeof(WCHAR)
    );
    if (!device_interface_list) {
        log_windows_error("HeapAlloc", GetLastError());
        err = CAHUTE_ERROR_ALLOC;
        goto fail;
    }

    cret = CM_Get_Device_Interface_ListW(
        (LPGUID)&GUID_DEVINTERFACE_USB_DEVICE,
        NULL,
        device_interface_list,
        device_interface_list_size,
        CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
    );
    if (cret != CR_SUCCESS) {
        msg(ll_error,
            "CM_Get_Device_Interface_ListW returned error 0x%08lX.",
            cret);
        goto fail;
    }

    for (device_interface = device_interface_list; *device_interface;
         device_interface += wcslen(device_interface) + 1) {
        WCHAR device_id[64];
        BYTE property_buffer[2048];
        DEVPROPTYPE property_type;
        DEVINST device_instance;
        ULONG property_size;

        /* Get the device identifier, as an interface property. */
        property_size = sizeof(device_id);
        cret = CM_Get_Device_Interface_PropertyW(
            device_interface,
            &DEVPKEY_Device_InstanceId,
            &property_type,
            (BYTE *)device_id,
            &property_size,
            0
        );
        if (cret != CR_SUCCESS) {
            msg(ll_error,
                "CM_Get_Device_Interface_PropertyW returned error 0x%08lX.",
                cret);
            goto fail;
        }

        if (property_type != DEVPROP_TYPE_STRING) {
            msg(ll_error, "Expected a string for the device identifier.");
            goto fail;
        }

        /* Get the device behind the interface. */
        cret = CM_Locate_DevNodeW(
            &device_instance,
            device_id,
            CM_LOCATE_DEVNODE_NORMAL
        );
        if (cret == CR_NO_SUCH_DEVNODE)
            continue;

        if (cret != CR_SUCCESS) {
            msg(ll_error, "CM_Locate_DevNodeW returned error 0x%08lX.", cret);
            goto fail;
        }

        /* Get location information in the form of a string.
         * This returns a string of the form "Port_#0002.Hub_#0001", from
         * which we can actually extract the same bus and address numbers
         * as from libusb. */
        property_size = sizeof(property_buffer);
        cret = CM_Get_DevNode_PropertyW(
            device_instance,
            &DEVPKEY_Device_LocationInfo,
            &property_type,
            (PBYTE)property_buffer,
            &property_size,
            0
        );
        if (cret != CR_SUCCESS) {
            msg(ll_error,
                "CM_Get_DevNode_PropertyW returned error 0x%08lX.",
                cret);
            goto fail;
        }

        if (property_type != DEVPROP_TYPE_STRING) {
            msg(ll_warn,
                "Unexpected type 0x%08lX for location info, skipping the "
                "entry.",
                property_type);
            continue;
        }

        {
            int detected_bus, detected_address, ret;

            ret = parse_winusb_device_location(
                (WCHAR *)property_buffer,
                &detected_bus,
                &detected_address
            );
            if (ret) {
                msg(ll_warn,
                    "Unable to gather location info, skipping the entry.");
                msg(ll_warn, "Location info was: %ls", property_buffer);
                continue;
            }

            if (detected_bus != bus || detected_address != address)
                continue;
        }

        /* Get the device driver, to check if we have a device managed by
         * CESG502 instead of a libusb-compatible driver. */
        property_size = sizeof(property_buffer);
        cret = CM_Get_DevNode_PropertyW(
            device_instance,
            &DEVPKEY_Device_Driver,
            &property_type,
            (PBYTE)property_buffer,
            &property_size,
            0
        );
        if (cret != CR_SUCCESS) {
            msg(ll_error,
                "CM_Get_DevNode_PropertyW returned error 0x%08lX.",
                cret);
            goto fail;
        }

        if (property_type != DEVPROP_TYPE_STRING) {
            msg(ll_warn,
                "Unexpected type 0x%08lX for driver key, skipping the "
                "entry.",
                property_type);
            break;
        }

        /* Check that the driver is the CESG502 driver.
         * This value has been found experimentally, although the real value
         * actually ends with "\0002", which may be the version. */
        if (memcmp(
                property_buffer,
                L"{36fc9e60-c465-11cf-8056-444553540000}",
                38 * sizeof(WCHAR)
            )) {
            msg(ll_warn, "Not an known CESG502 driver: %.38ls", property_buffer
            );
            break;
        }

        /* The device has been found and is running the CESG502 driver! */
        protocol = CAHUTE_LINK_PROTOCOL_USB_SEVEN;

        msg(ll_info, "Opening the following CESG502 interface:");
        msg(ll_info, "%ls", device_interface);

        cesg_handle = CreateFileW(
            device_interface,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        werr = GetLastError();

        /* In any case, we want to free the device interface list now, as we
         * no longer have a use for it. */
        HeapFree(GetProcessHeap(), 0, device_interface_list);
        device_interface_list = NULL;

        /* We can now process any error that occurred with CreateFile(). */
        if (cesg_handle == INVALID_HANDLE_VALUE)
            switch (werr) {
            case ERROR_ACCESS_DENIED:
                err = CAHUTE_ERROR_PRIV;
                goto fail;

            default:
                log_windows_error("CreateFile", werr);
                goto fail;
            }

        /* TODO: Implement timeouts with Windows streams. */
        msg(ll_warn, "Streams using the Windows API do not support timeouts.");
        goto ready;
    }

    msg(ll_info,
        "Device was either not found or found but not running the CESG502 "
        "driver; falling back on libusb.");

    HeapFree(GetProcessHeap(), 0, device_interface_list);
    device_interface_list = NULL;
#endif

#if defined(CAHUTE_LINK_STREAM_LIBUSB)
    if (libusb_init(&context)) {
        msg(ll_fatal, "Could not create a libusb context.");
        goto fail;
    }

    device_count = libusb_get_device_list(context, &device_list);
    if (device_count < 0) {
        msg(ll_fatal, "Could not get a device list.");
        goto fail;
    }

    for (i = 0; i < device_count; i++) {
        struct libusb_device_descriptor device_descriptor;
        struct libusb_interface_descriptor const *interface_descriptor;
        struct libusb_endpoint_descriptor const *endpoint_descriptor;
        int interface_class = 0, j;

        if (libusb_get_bus_number(device_list[i]) != bus
            || libusb_get_device_address(device_list[i]) != address)
            continue;

        err = CAHUTE_ERROR_INCOMPAT;
        if (libusb_get_device_descriptor(device_list[i], &device_descriptor))
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
                device_list[i],
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
            j = interface_descriptor->bNumEndpoints;
             j;
             endpoint_descriptor++, j--) {
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

        libusberr = libusb_open(device_list[i], &device_handle);
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

    goto ready;
#endif

    msg(ll_error, "No method available for opening an USB device.");
    err = CAHUTE_ERROR_IMPL;
    goto fail;

ready:
    link = malloc(sizeof(cahute_link) + DEFAULT_PROTOCOL_BUFFER_SIZE);
    if (!link) {
        err = CAHUTE_ERROR_ALLOC;
        goto fail;
    }

#if WINDOWS_ENABLED
    if (cesg_handle != INVALID_HANDLE_VALUE) {
        link->flags = CAHUTE_LINK_FLAG_CLOSE_STREAM;
        link->stream = CAHUTE_LINK_STREAM_WINDOWS;
        link->stream_state.windows.handle = cesg_handle;
        link->stream_state.windows.is_cesg = 1;

        /* The link takes control of the handle. */
        cesg_handle = INVALID_HANDLE_VALUE;

        goto prepared;
    }
#endif

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
    goto prepared;
#endif

    err = CAHUTE_ERROR_IMPL;
    goto fail;

prepared:
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
#if WINDOWS_ENABLED
    if (cesg_handle != INVALID_HANDLE_VALUE)
        CloseHandle(cesg_handle);
    if (device_interface_list)
        HeapFree(GetProcessHeap(), 0, device_interface_list);
#endif

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
