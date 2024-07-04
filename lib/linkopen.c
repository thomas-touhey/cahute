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
#define DEFAULT_DATA_BUFFER_SIZE 524288 /* 512 KiB, max. for VRAM */

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
    case CAHUTE_LINK_PROTOCOL_USB_MASS_STORAGE:
        return "USB Mass Storage";
    default:
        return "(unknown)";
    }
}

/**
 * Get the name of a link medium.
 *
 * @param medium Medium identifier, as a constant.
 * @return Textual name of the medium.
 */
CAHUTE_LOCAL(char const *) get_medium_name(int medium) {
    switch (medium) {
#ifdef CAHUTE_LINK_MEDIUM_POSIX_SERIAL
    case CAHUTE_LINK_MEDIUM_POSIX_SERIAL:
        return "Serial (POSIX)";
#endif
#ifdef CAHUTE_LINK_MEDIUM_WIN32_SERIAL
    case CAHUTE_LINK_MEDIUM_WIN32_SERIAL:
        return "Serial (Win32)";
#endif
#ifdef CAHUTE_LINK_MEDIUM_WIN32_CESG
    case CAHUTE_LINK_MEDIUM_WIN32_CESG:
        return "CESG502 (Win32)";
#endif
#ifdef CAHUTE_LINK_MEDIUM_WIN32_UMS
    case CAHUTE_LINK_MEDIUM_WIN32_UMS:
        return "USB Mass Storage (Win32)";
#endif
#ifdef CAHUTE_LINK_MEDIUM_LIBUSB
    case CAHUTE_LINK_MEDIUM_LIBUSB:
        return "USB Bulk (libusb)";
#endif
#ifdef CAHUTE_LINK_MEDIUM_LIBUSB_UMS
    case CAHUTE_LINK_MEDIUM_LIBUSB_UMS:
        return "USB Mass Storage (libusb)";
#endif
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

    do {
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
             * We don't want to read the rest of the packet, the receiving
             * routine for Protocol 7.00 screenstreaming will realign
             * itself. */
            *protocolp = CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP;
            return CAHUTE_OK;
        } else if (buf[0] == 0x10) {
            /* This is an unknown protocol that is tried by the calculator
             * on both USB and serial when transmitting. If we just ignore
             * this, the calculator will try Protocol 7.00 and CASIOLINK
             * eventually. */
            continue;
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

        break;
    } while (1);

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
init_link(cahute_link *link, unsigned long flags, int casiolink_variant) {
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

    msg(ll_info,
        "Using %s over %s.",
        get_protocol_name(protocol),
        get_medium_name(link->medium));
    msg(ll_info,
        "Playing the role of %s.",
        flags & PROTOCOL_FLAG_RECEIVER ? "receiver / passive side"
                                       : "sender / active side");

    switch (protocol) {
    case CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK:
        casiolink_state = &link->protocol_state.casiolink;

        if (link->data_buffer_capacity < CASIOLINK_MINIMUM_BUFFER_SIZE) {
            msg(ll_fatal,
                "CASIOLINK implementation expected a minimum data "
                "buffer capacity of %" CAHUTE_PRIuSIZE
                ", got %" CAHUTE_PRIuSIZE ".",
                CASIOLINK_MINIMUM_BUFFER_SIZE,
                link->data_buffer_capacity);
            return CAHUTE_ERROR_UNKNOWN;
        }

        casiolink_state->flags = 0;
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
        seven_state->flags = 0;
        seven_state->last_packet_type = -1;
        seven_state->last_packet_subtype = -1;
        seven_state->last_packet_data_size = 0;
        seven_state->raw_device_info_size = 0;

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
        seven_ohp_state = &link->protocol_state.seven_ohp;

        /* No need to guarantee a minimum data buffer size here;
         * all writes to the data buffer will check for its capacity! */
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

#if WIN32_ENABLED && LIBUSB_ENABLED
# include <cfgmgr32.h>
# include <usbiodef.h>
# include <initguid.h>
# include <devguid.h>
# include <ddk/wdmguid.h>
# define HEXDIGIT(C) \
     ((C) >= 'a' ? (C) - 'a' + 10 : (C) >= 'A' ? (C) - 'A' + 10 : (C) - '0')

/**
 * Decode a GUID from a string.
 *
 * An example string is "{4d36e967-e325-11ce-bfc1-08002be10318}".
 *
 * @param guid Pointer to the GUID to set.
 * @param raw Raw GUID to parse.
 * @return 1 if parsing has failed, 0 otherwise.
 */
CAHUTE_INLINE(int) decode_guid(GUID *guid, char const *raw) {
    if (raw[0] != '{' || !isxdigit(raw[1]) || !isxdigit(raw[2])
        || !isxdigit(raw[3]) || !isxdigit(raw[4]) || !isxdigit(raw[5])
        || !isxdigit(raw[6]) || !isxdigit(raw[7]) || !isxdigit(raw[8])
        || raw[9] != '-' || !isxdigit(raw[10]) || !isxdigit(raw[11])
        || !isxdigit(raw[12]) || !isxdigit(raw[13]) || raw[14] != '-'
        || !isxdigit(raw[15]) || !isxdigit(raw[16]) || !isxdigit(raw[17])
        || !isxdigit(raw[18]) || raw[19] != '-' || !isxdigit(raw[20])
        || !isxdigit(raw[21]) || !isxdigit(raw[22]) || !isxdigit(raw[23])
        || raw[24] != '-' || !isxdigit(raw[25]) || !isxdigit(raw[26])
        || !isxdigit(raw[27]) || !isxdigit(raw[28]) || !isxdigit(raw[29])
        || !isxdigit(raw[30]) || !isxdigit(raw[31]) || !isxdigit(raw[32])
        || !isxdigit(raw[33]) || !isxdigit(raw[34]) || !isxdigit(raw[35])
        || !isxdigit(raw[36]) || raw[37] != '}') {
        msg(ll_error, "Unable to decode GUID: %s", raw);
        return 1;
    }

    guid->Data1 = cahute_htole32(
        (HEXDIGIT(raw[1]) << 28) | (HEXDIGIT(raw[2]) << 24)
        | (HEXDIGIT(raw[3]) << 20) | (HEXDIGIT(raw[4]) << 16)
        | (HEXDIGIT(raw[5]) << 12) | (HEXDIGIT(raw[6]) << 8)
        | (HEXDIGIT(raw[7]) << 4) | HEXDIGIT(raw[8])
    );
    guid->Data2 = cahute_htole16(
        (HEXDIGIT(raw[10]) << 12) | (HEXDIGIT(raw[11]) << 8)
        | (HEXDIGIT(raw[12]) << 4) | HEXDIGIT(raw[13])
    );
    guid->Data3 = cahute_htole16(
        (HEXDIGIT(raw[15]) << 12) | (HEXDIGIT(raw[16]) << 8)
        | (HEXDIGIT(raw[17]) << 4) | HEXDIGIT(raw[18])
    );
    guid->Data4[0] = (HEXDIGIT(raw[20]) << 4) | HEXDIGIT(raw[21]);
    guid->Data4[1] = (HEXDIGIT(raw[22]) << 4) | HEXDIGIT(raw[23]);

    /* We skip ``raw[24]`` because it's a dash. */
    guid->Data4[2] = (HEXDIGIT(raw[25]) << 4) | HEXDIGIT(raw[26]);
    guid->Data4[3] = (HEXDIGIT(raw[27]) << 4) | HEXDIGIT(raw[28]);
    guid->Data4[4] = (HEXDIGIT(raw[29]) << 4) | HEXDIGIT(raw[30]);
    guid->Data4[5] = (HEXDIGIT(raw[31]) << 4) | HEXDIGIT(raw[32]);
    guid->Data4[6] = (HEXDIGIT(raw[33]) << 4) | HEXDIGIT(raw[34]);
    guid->Data4[7] = (HEXDIGIT(raw[35]) << 4) | HEXDIGIT(raw[36]);
    return 0;
}

/**
 * Find a volume interface associated with the provided device identifier.
 *
 * @param path Path to the device interface to fill.
 * @param path_size Size of the path.
 * @param device_id Device identifier.
 * @param guid Device interface GUID to look for.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int)
find_win32_interface(
    char *path,
    size_t path_size,
    char *device_id,
    LPGUID guid
) {
    DWORD property_size = 0;
    CONFIGRET cret;

    cret = CM_Get_Device_Interface_List_SizeA(
        &property_size,
        guid,
        device_id,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT
    );
    if (cret != CR_SUCCESS) {
        msg(ll_error,
            "CM_Get_Device_Interface_List_SizeA returned error "
            "0x%08lX.",
            cret);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (property_size > path_size) {
        /* Relevant device interfaces encountered in the wild do
         * not have such a big device interface name, we can skip
         * the entry. */
        return CAHUTE_ERROR_SIZE;
    }

    cret = CM_Get_Device_Interface_ListA(
        guid,
        device_id,
        path,
        property_size,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT
    );
    if (cret != CR_SUCCESS) {
        msg(ll_error,
            "CM_Get_Device_Interface_ListA returned error "
            "0x%08lX.",
            cret);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (!path[0]) {
        /* Missing at least one interface, we want to ignore the
         * current USB device. */
        return CAHUTE_ERROR_NOT_FOUND;
    }

    return CAHUTE_OK;
}

/**
 * Find a USB device for the provided interface type.
 *
 * We use the more portable CfgMgr32 API rather than SetupApi.
 *
 * The full extent of the Unified Device Property Model is not available
 * until Windows Vista, and we aim at keeping Windows XP compatibility,
 * so we use registry properties on devices, and start of by querying devices
 * first rather than device interfaces first, since querying a device from
 * a device interface is a device interface property, which is not available
 * on Windows XP.
 *
 * The provided physical port was obtained using ``libusb_get_port_number``,
 * which is set to the device address on Windows. We can find out the USB
 * device by checking that the bus type (``CM_DRP_BUSTYPEGUID``) is USB,
 * and that the device address (``CM_DRP_ADDRESS``) corresponds to the
 * libusb port number.
 *
 * For volumes, while on Windows XP the volume is a child device to the
 * USB device, on Windows 11 the volume is in a completely different tree,
 * being attached to the volume manager (volmgr). However, on both,
 * we can use Bus Relations to get the disk drive from the USB device,
 * then the volume from the disk drive.
 *
 * This function has the following steps:
 *
 *   Step 1. Find the USB device corresponding to the calculator.
 *   Step 2. If the loaded driver is CESG, get the USB device interface path
 *           corresponding to the USB device.
 *   Step 3. Get the disk drive device corresponding to the USB device through
 *           Bus Relations.
 *   Step 4. Get the volume device corresponding to the disk drive device
 *           through Bus Relations.
 *   Step 5. Get the volume device interface path corresponding to the
 *           volume device.
 *
 * @param path Path to the device interface to fill.
 * @param path_size Size of the path.
 * @param mediump Medium type to define.
 * @param addr Physical port of the device.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int)
find_win32_usb_device(char *path, size_t path_size, int *mediump, DWORD addr) {
    DEVINST device_instance;
    char *usb_device_id_list = NULL, *usb_device_id;
    BYTE property[64];
    char disk_drive_id_buf[300], *disk_drive_id;
    char volume_id_buf[300], *volume_id;
    ULONG property_type = 0;
    ULONG property_size = 0;
    CONFIGRET cret;
    GUID guid;
    int err = CAHUTE_ERROR_UNKNOWN;

    /* ---
     * Step 1. Find the USB device corresponding to the calculator.
     * ---
     * The final device identifier will be accessible through
     * ``usb_device_id``.
     *
     * Since the device was found using libusb, not finding it here results
     * rightfully in a CAHUTE_ERROR_UNKNOWN. */

    cret = CM_Get_Device_ID_List_SizeA(
        &property_size,
        NULL,
        CM_GETIDLIST_FILTER_NONE
    );
    if (cret != CR_SUCCESS) {
        msg(ll_error,
            "CM_Get_Device_ID_List_SizeA returned error 0x%08lX.",
            cret);
        goto fail;
    }

    usb_device_id_list =
        (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, property_size);
    if (!usb_device_id_list) {
        log_windows_error("HeapAlloc", GetLastError());
        err = CAHUTE_ERROR_ALLOC;
        goto fail;
    }

    cret = CM_Get_Device_ID_ListA(
        NULL,
        usb_device_id_list,
        property_size,
        CM_GETIDLIST_FILTER_NONE
    );
    if (cret != CR_SUCCESS) {
        msg(ll_error, "CM_Get_Device_ID_ListA returned error 0x%08lX.", cret);
        goto fail;
    }

    for (usb_device_id = usb_device_id_list; *usb_device_id;
         usb_device_id += strlen(usb_device_id) + 1) {
        /* Get the device behind the interface. */
        cret = CM_Locate_DevNodeA(
            &device_instance,
            usb_device_id,
            CM_LOCATE_DEVNODE_NORMAL
        );
        if (cret == CR_NO_SUCH_DEVINST)
            continue;

        if (cret != CR_SUCCESS) {
            msg(ll_error, "CM_Locate_DevNodeA returned error 0x%08lX.", cret);
            goto fail;
        }

        /* We want to check that the device or any of its parents is actually
         * the USB device we're looking for. */
        property_size = sizeof(property);
        cret = CM_Get_DevNode_Registry_PropertyA(
            device_instance,
            CM_DRP_BUSTYPEGUID,
            &property_type,
            (PBYTE)property,
            &property_size,
            0
        );
        if (cret == CR_NO_SUCH_VALUE)
            continue; /* Virtual device, we need to check the parent. */

        if (cret != CR_SUCCESS) {
            msg(ll_error,
                "CM_Get_DevNode_Registry_PropertyA with property "
                "CM_DRP_BUSTYPEGUID returned error 0x%08lX.",
                cret);
            goto fail;
        }

        if (property_type != REG_BINARY
            || property_size != sizeof(GUID_BUS_TYPE_USB)) {
            msg(ll_warn,
                "Unexpected type 0x%08lX or size %luo for bus type key.",
                property_type,
                property_size);
            goto fail;
        }

        if (memcmp(property, &GUID_BUS_TYPE_USB, sizeof(GUID_BUS_TYPE_USB))) {
            /* The bus type is not USB, we do not have the
             * correct device. */
            continue;
        }

        /* Get the device address, to check if it corresponds to the
         * address we have previously found. */
        property_size = sizeof(property);
        cret = CM_Get_DevNode_Registry_PropertyA(
            device_instance,
            CM_DRP_ADDRESS,
            &property_type,
            (PBYTE)property,
            &property_size,
            0
        );
        if (cret != CR_SUCCESS) {
            msg(ll_error,
                "CM_Get_DevNode_Registry_PropertyA with property "
                "CM_DRP_ADDRESS returned error 0x%08lX.",
                cret);
            goto fail;
        }

        if (property_type != REG_DWORD) {
            msg(ll_warn,
                "Unexpected type 0x%08lX for device address key.",
                property_type);
            goto fail;
        }

        if (*(DWORD *)property != addr)
            continue;

        /* ---
         * Step 2. If the loaded driver is CESG, get the USB device interface
         *         path corresponding to the USB device.
         * ---
         * We use the "Service" interface property to identify the driver,
         * since CESG502 uses a driver key that is normally forbidden to
         * independent hardware vendors (IHVs). */

        property_size = sizeof(property);
        cret = CM_Get_DevNode_Registry_PropertyA(
            device_instance,
            CM_DRP_SERVICE,
            &property_type,
            (PBYTE)property,
            &property_size,
            0
        );
        if (cret == CR_NO_SUCH_VALUE)
            continue; /* No driver installed. */

        if (cret != CR_SUCCESS) {
            msg(ll_error,
                "CM_Get_DevNode_Registry_PropertyA returned error "
                "0x%08lX.",
                cret);
            goto fail;
        }

        if (property_type == REG_SZ && property_size == 6
            && !memcmp(property, "PVUSB", 6)) {
            /* The medium type is CESG, since we must interact with it. */
            *mediump = CAHUTE_LINK_MEDIUM_WIN32_CESG;
            err = find_win32_interface(
                path,
                path_size,
                usb_device_id,
                (LPGUID)&GUID_DEVINTERFACE_USB_DEVICE
            );

            goto fail;
        }

        *mediump = CAHUTE_LINK_MEDIUM_WIN32_UMS;

        /* From here, the expected type is VOLUME.
         * ---
         * Step 3. Get the disk drive device corresponding to the USB device
         *         through Bus Relations.
         * --- */

        cret = CM_Get_Device_ID_ListA(
            usb_device_id,
            disk_drive_id_buf,
            sizeof(disk_drive_id_buf),
            CM_GETIDLIST_FILTER_BUSRELATIONS
        );
        if (cret == CR_BUFFER_SMALL) {
            msg(ll_error,
                "Sub device id buffer size was not big enough for USB "
                "device bus relations.");
            err = CAHUTE_ERROR_SIZE;
            goto fail;
        }

        if (cret != CR_SUCCESS) {
            msg(ll_error,
                "CM_Get_Device_ID_ListA (disk drive) returned error "
                "0x%08lX.",
                cret);
            goto fail;
        }

        for (disk_drive_id = disk_drive_id_buf; *disk_drive_id;
             disk_drive_id += strlen(disk_drive_id) + 1) {
            DEVINST disk_drive_device_instance;

            /* Get the device behind the interface. */
            cret = CM_Locate_DevNodeA(
                &disk_drive_device_instance,
                disk_drive_id,
                CM_LOCATE_DEVNODE_NORMAL
            );
            if (cret == CR_NO_SUCH_DEVINST)
                continue;

            if (cret != CR_SUCCESS) {
                msg(ll_error,
                    "CM_Locate_DevNodeA (disk drive) returned error 0x%08lX.",
                    cret);
                goto fail;
            }

            /* Check that the class of the device is a disk drive. */
            property_size = sizeof(property);
            cret = CM_Get_DevNode_Registry_PropertyA(
                disk_drive_device_instance,
                CM_DRP_CLASSGUID,
                &property_type,
                (PBYTE)property,
                &property_size,
                0
            );
            if (cret == CR_NO_SUCH_VALUE)
                continue;

            if (cret != CR_SUCCESS) {
                msg(ll_error,
                    "CM_Get_DevNode_Registry_PropertyA (disk drive) with "
                    "property CM_DRP_CLASSGUID returned error 0x%08lX.",
                    cret);
                goto fail;
            }

            if (property_type == REG_SZ && property_size == 39) {
                if (decode_guid(&guid, (char const *)property))
                    goto fail;
            } else if (property_type != REG_BINARY || property_size != sizeof(GUID_DEVCLASS_DISKDRIVE))
                memcpy(&guid, property, sizeof(GUID));
            else {
                msg(ll_warn,
                    "Unexpected type 0x%08lX or size %luo for device "
                    "class key.",
                    property_type,
                    property_size);
                goto fail;
            }

            if (memcmp(
                    &guid,
                    &GUID_DEVCLASS_DISKDRIVE,
                    sizeof(GUID_DEVCLASS_DISKDRIVE)
                )) {
                /* The device is not a disk drive. */
                continue;
            }

            /* ---
             * Step 4. Get the volume device corresponding to the disk drive
             *         device through Bus Relations.
             * --- */

            cret = CM_Get_Device_ID_ListA(
                disk_drive_id,
                volume_id_buf,
                sizeof(volume_id_buf),
                CM_GETIDLIST_FILTER_BUSRELATIONS
            );

            if (cret == CR_BUFFER_SMALL) {
                msg(ll_error,
                    "Sub device id buffer size was not big enough for disk "
                    "drive bus relations.");
                err = CAHUTE_ERROR_SIZE;
                goto fail;
            }

            if (cret != CR_SUCCESS) {
                msg(ll_error,
                    "CM_Get_Device_ID_ListA (volume) returned error 0x%08lX.",
                    cret);
                goto fail;
            }

            for (volume_id = volume_id_buf; *volume_id;
                 volume_id += strlen(volume_id) + 1) {
                DEVINST volume_device_instance;

                /* Get the device behind the interface. */
                cret = CM_Locate_DevNodeA(
                    &volume_device_instance,
                    volume_id,
                    CM_LOCATE_DEVNODE_NORMAL
                );
                if (cret == CR_NO_SUCH_DEVINST)
                    continue;

                if (cret != CR_SUCCESS) {
                    msg(ll_error,
                        "CM_Locate_DevNodeA (volume) returned error 0x%08lX.",
                        cret);
                    goto fail;
                }

                /* Check that the class of the device is a volume. */
                property_size = sizeof(property);
                cret = CM_Get_DevNode_Registry_PropertyA(
                    volume_device_instance,
                    CM_DRP_CLASSGUID,
                    &property_type,
                    (PBYTE)property,
                    &property_size,
                    0
                );
                if (cret == CR_NO_SUCH_VALUE)
                    continue;

                if (cret != CR_SUCCESS) {
                    msg(ll_error,
                        "CM_Get_DevNode_Registry_PropertyA (volume) with "
                        "property CM_DRP_CLASSGUID returned error 0x%08lX.",
                        cret);
                    goto fail;
                }

                if (property_type == REG_SZ && property_size == 39) {
                    if (decode_guid(&guid, (char const *)property))
                        goto fail;
                } else if (property_type != REG_BINARY || property_size != sizeof(GUID))
                    memcpy(&guid, property, sizeof(GUID));
                else {
                    msg(ll_warn,
                        "Unexpected type 0x%08lX or size %luo for device "
                        "class key.",
                        property_type,
                        property_size);
                    goto fail;
                }

                if (memcmp(
                        property,
                        &GUID_DEVCLASS_VOLUME,
                        sizeof(GUID_DEVCLASS_VOLUME)
                    )) {
                    /* The device is not a volume. */
                    continue;
                }

                /* ---
                 * Step 5. Get the volume device interface path corresponding
                 *         to the volume device.
                 * --- */

                err = find_win32_interface(
                    path,
                    path_size,
                    volume_id,
                    (LPGUID)&GUID_DEVINTERFACE_VOLUME
                );
                if (!err)
                    goto end;
            }
        }
    }

    /* No device has been found! */
    msg(ll_error, "No device for USB port number %ld was found.", addr);
    goto fail;

end:
    err = CAHUTE_OK;

fail:
    if (usb_device_id_list)
        HeapFree(GetProcessHeap(), 0, usb_device_id_list);

    return err;
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
        /* If we are not allowed to initiate the connection, we cannot test
         * different things, therefore this cannot be used with
         * ``CAHUTE_SERIAL_NOCHECK``. */
        if (flags & CAHUTE_SERIAL_NOCHECK) {
            msg(ll_error, "We need the check flow to determine the protocol.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        protocol = CAHUTE_LINK_PROTOCOL_SERIAL_AUTO;
        break;

    case CAHUTE_SERIAL_PROTOCOL_CASIOLINK:
        protocol = CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK;
        break;

    case CAHUTE_SERIAL_PROTOCOL_SEVEN:
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

#if defined(CAHUTE_LINK_MEDIUM_POSIX_SERIAL)
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
#elif defined(CAHUTE_LINK_MEDIUM_WIN32_SERIAL)
    HANDLE handle = INVALID_HANDLE_VALUE;
    HANDLE overlapped_event_handle = INVALID_HANDLE_VALUE;
    DWORD werr;

    handle = CreateFile(
        name_or_path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
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

    /* Create the overlapped event. */
    overlapped_event_handle = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (overlapped_event_handle == INVALID_HANDLE_VALUE) {
        log_windows_error("CreateEvent", GetLastError());
        CloseHandle(handle);
        return CAHUTE_ERROR_UNKNOWN;
    }
#else
    CAHUTE_RETURN_IMPL("No serial device opening method available.");
#endif

    link = malloc(
        sizeof(cahute_link) + 32 + CAHUTE_LINK_MEDIUM_READ_BUFFER_SIZE
        + DEFAULT_DATA_BUFFER_SIZE
    );
    if (!link)
        err = CAHUTE_ERROR_ALLOC;

#if defined(CAHUTE_LINK_MEDIUM_POSIX_SERIAL)
    if (err) {
        close(fd);
        return err;
    }

    link->flags = CAHUTE_LINK_FLAG_CLOSE_MEDIUM;
    link->medium = CAHUTE_LINK_MEDIUM_POSIX_SERIAL;
    link->medium_state.posix.fd = fd;
#elif defined(CAHUTE_LINK_MEDIUM_WIN32_SERIAL)
    if (err) {
        CloseHandle(handle);
        return err;
    }

    link->flags = CAHUTE_LINK_FLAG_CLOSE_MEDIUM;
    link->medium = CAHUTE_LINK_MEDIUM_WIN32_SERIAL;
    link->medium_state.windows.handle = handle;

    SecureZeroMemory(
        &link->medium_state.windows.overlapped,
        sizeof(OVERLAPPED)
    );
    link->medium_state.windows.overlapped.hEvent = overlapped_event_handle;
#endif

    link->serial_flags = 0;
    link->serial_speed = 0;
    link->protocol = protocol;
    link->medium_read_start = 0;
    link->medium_read_size = 0;
    link->medium_read_buffer = (cahute_u8 *)link + sizeof(cahute_link);

    /* Ensure that the data buffer is aligned correctly, for sensitive
     * mediums such as Win32 SCSI devices. */
    link->medium_read_buffer +=
        (~(uintptr_t)link->medium_read_buffer & 31) + 1;

    link->data_buffer =
        link->medium_read_buffer + CAHUTE_LINK_MEDIUM_READ_BUFFER_SIZE;
    link->data_buffer_size = 0;
    link->data_buffer_capacity = DEFAULT_DATA_BUFFER_SIZE;
    link->cached_device_info = NULL;

    /* The link is now considered opened, with protocol uninitialized.
     * We want to set the serial parameters to the medium now. */
    err = cahute_set_serial_params_to_link(
        link,
        flags
            & (CAHUTE_SERIAL_STOP_MASK | CAHUTE_SERIAL_PARITY_MASK
               | CAHUTE_SERIAL_XONXOFF_MASK | CAHUTE_SERIAL_DTR_MASK
               | CAHUTE_SERIAL_RTS_MASK),
        speed
    );
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

    err = init_link(link, protocol_flags, casiolink_variant);
    if (err) {
        cahute_close_link(link);
        return err;
    }

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
#if LIBUSB_ENABLED
    libusb_context *context = NULL;
    libusb_device **device_list = NULL;
    struct libusb_config_descriptor *config_descriptor = NULL;
    libusb_device_handle *device_handle = NULL;
    cahute_link *link = NULL;
    int device_count, i, libusberr, bulk_in = -1, bulk_out = -1;
    int libusb_medium = CAHUTE_LINK_MEDIUM_LIBUSB;
    int protocol = CAHUTE_LINK_PROTOCOL_USB_SEVEN;
    unsigned long protocol_flags = 0;
    int err = CAHUTE_ERROR_UNKNOWN;

# if WIN32_ENABLED
    HANDLE ums_handle = INVALID_HANDLE_VALUE;
    HANDLE cesg_handle = INVALID_HANDLE_VALUE;
    HANDLE overlapped_event_handle = INVALID_HANDLE_VALUE;
# endif

    if (flags & CAHUTE_USB_OHP) {
        /* TODO */
        if (~flags & CAHUTE_USB_RECEIVER)
            CAHUTE_RETURN_IMPL("Sender mode not available for screenstreaming."
            );

        protocol_flags |= PROTOCOL_FLAG_RECEIVER;
    } else if (flags & CAHUTE_USB_RECEIVER)
        CAHUTE_RETURN_IMPL("Receiver mode not available for data protocols.");

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

        if (interface_class == 8)
            libusb_medium = CAHUTE_LINK_MEDIUM_LIBUSB_UMS;
        else if (interface_class == 255)
            libusb_medium = CAHUTE_LINK_MEDIUM_LIBUSB;
        else {
            msg(ll_error, "Unsupported interface class %d", interface_class);
            goto fail;
        }

        if (flags & CAHUTE_USB_OHP)
            protocol = CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP;
        else if (libusb_medium == CAHUTE_LINK_MEDIUM_LIBUSB_UMS)
            protocol = CAHUTE_LINK_PROTOCOL_USB_MASS_STORAGE;

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

        case LIBUSB_ERROR_NOT_SUPPORTED:
# if WIN32_ENABLED
        {
            char device_interface[300];
            int win32_medium;

            err = find_win32_usb_device(
                device_interface,
                sizeof(device_interface),
                &win32_medium,
                libusb_get_port_number(device_list[i])
            );

            if (!err)
                switch (win32_medium) {
                case CAHUTE_LINK_MEDIUM_WIN32_UMS:
                    /* The device is a volume on which we should make
                     * synchronous SCSI requests using DeviceIoControl(). */
                    ums_handle = CreateFileA(
                        device_interface,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                    );

                    if (ums_handle == INVALID_HANDLE_VALUE) {
                        DWORD werr = GetLastError();

                        if (werr == ERROR_ACCESS_DENIED)
                            err = CAHUTE_ERROR_PRIV;
                        else
                            log_windows_error("CreateFileA", werr);

                        goto fail;
                    }

                    goto ready;

                case CAHUTE_LINK_MEDIUM_WIN32_CESG:
                    /* The device is a USB device opened using CESG502. */
                    cesg_handle = CreateFileA(
                        device_interface,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                        NULL
                    );

                    if (cesg_handle == INVALID_HANDLE_VALUE) {
                        DWORD werr = GetLastError();

                        if (werr == ERROR_ACCESS_DENIED)
                            err = CAHUTE_ERROR_PRIV;
                        else
                            log_windows_error("CreateFileA", werr);

                        goto fail;
                    }

                    /* Create the overlapped event. */
                    overlapped_event_handle =
                        CreateEvent(NULL, TRUE, FALSE, NULL);
                    if (overlapped_event_handle == INVALID_HANDLE_VALUE) {
                        log_windows_error("CreateEvent", GetLastError());
                        goto fail;
                    }

                    goto ready;
                }
        }
# endif

            msg(ll_error,
                "libusb_open returned %d: %s",
                libusberr,
                libusb_error_name(libusberr));
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

    if (protocol == CAHUTE_LINK_PROTOCOL_USB_SEVEN) {
        /* Calculators running 1.x OSes with Protocol 7.00 support may need a
         * push to enable communicating using Protocol 7.00, in the form of
         * a vendor-specific request documented in fxReverse. */

        msg(ll_info, "Running vendor-specific interface request 0x01.");
        libusberr = libusb_control_transfer(
            device_handle,
            0x41,   /* Vendor-specific interface request. */
            0x01,   /* Request code 0x01 */
            0x0000, /* wValue is unused */
            0x0000, /* wIndex is unused also */
            NULL,
            0,  /* No data transfer. */
            300 /* 300ms should be more than enough. */
        );

        switch (libusberr) {
        case 0:
            break;

        default:
            msg(ll_fatal,
                "libusb_control_transfer with vendor-specific interface "
                "request 0x01 caused error %d: %s",
                libusberr,
                libusb_error_name(libusberr));
            goto fail;
        }
    }

    goto ready;

ready:
    /* Allocate the link.
     * The 8 additional bytes are there for alignment purposes. */
    link = malloc(
        sizeof(cahute_link) + 32 + CAHUTE_LINK_MEDIUM_READ_BUFFER_SIZE
        + DEFAULT_DATA_BUFFER_SIZE
    );
    if (!link) {
        err = CAHUTE_ERROR_ALLOC;
        goto fail;
    }

# if WIN32_ENABLED
    if (ums_handle != INVALID_HANDLE_VALUE) {
        link->flags = CAHUTE_LINK_FLAG_CLOSE_MEDIUM;
        link->medium = CAHUTE_LINK_MEDIUM_WIN32_UMS;
        link->medium_state.windows_ums.handle = ums_handle;

        /* The link takes control of the handle. */
        ums_handle = INVALID_HANDLE_VALUE;
        goto prepared;
    }

    if (cesg_handle != INVALID_HANDLE_VALUE) {
        link->flags = CAHUTE_LINK_FLAG_CLOSE_MEDIUM;
        link->medium = CAHUTE_LINK_MEDIUM_WIN32_CESG;
        link->medium_state.windows.handle = cesg_handle;

        SecureZeroMemory(
            &link->medium_state.windows.overlapped,
            sizeof(OVERLAPPED)
        );
        link->medium_state.windows.overlapped.hEvent = overlapped_event_handle;

        /* The link takes control of the handle. */
        cesg_handle = INVALID_HANDLE_VALUE;
        overlapped_event_handle = INVALID_HANDLE_VALUE;
        goto prepared;
    }
# endif

    link->flags = CAHUTE_LINK_FLAG_CLOSE_MEDIUM;
    link->medium = libusb_medium;
    link->medium_state.libusb.context = context;
    link->medium_state.libusb.handle = device_handle;
    link->medium_state.libusb.bulk_in = bulk_in;
    link->medium_state.libusb.bulk_out = bulk_out;

    /* Since the link now takes control over the context and device handle,
     * in case of failure, we must not free them here. */
    context = NULL;
    device_handle = NULL;

    msg(ll_info, "Bulk in endpoint address is: 0x%02X", bulk_in);
    msg(ll_info, "Bulk out endpoint address is: 0x%02X", bulk_out);

# if WIN32_ENABLED
prepared:
# endif
    link->serial_flags = 0;
    link->serial_speed = 0;
    link->protocol = protocol;
    link->medium_read_start = 0;
    link->medium_read_size = 0;
    link->medium_read_buffer = (cahute_u8 *)link + sizeof(cahute_link);

    /* Ensure that the data buffer is aligned to 8 bytes, for sensitive
     * mediums such as Win32 SCSI devices. */
    link->medium_read_buffer +=
        (~(uintptr_t)link->medium_read_buffer & 31) + 1;

    link->data_buffer =
        link->medium_read_buffer + CAHUTE_LINK_MEDIUM_READ_BUFFER_SIZE;
    link->data_buffer_size = 0;
    link->data_buffer_capacity = DEFAULT_DATA_BUFFER_SIZE;
    link->cached_device_info = NULL;

    if (flags & CAHUTE_USB_NOCHECK)
        protocol_flags |= PROTOCOL_FLAG_NOCHECK;
    if (flags & CAHUTE_USB_NODISC)
        protocol_flags |= PROTOCOL_FLAG_NODISC;
    if (flags & CAHUTE_USB_NOTERM)
        protocol_flags |= PROTOCOL_FLAG_NOTERM;

    if ((err = init_link(link, protocol_flags, 0)))
        goto fail;

    *linkp = link;
    return CAHUTE_OK;

fail:
# if WIN32_ENABLED
    if (overlapped_event_handle != INVALID_HANDLE_VALUE)
        CloseHandle(overlapped_event_handle);
    if (cesg_handle != INVALID_HANDLE_VALUE)
        CloseHandle(cesg_handle);
    if (ums_handle != INVALID_HANDLE_VALUE)
        CloseHandle(ums_handle);
# endif

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

    return err;
#else
    CAHUTE_RETURN_IMPL("No method available for opening an USB device.");
#endif
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

    if ((link->flags & CAHUTE_LINK_FLAG_TERMINATE)
        && !(
            link->flags
            & (CAHUTE_LINK_FLAG_IRRECOVERABLE | CAHUTE_LINK_FLAG_TERMINATED
               | CAHUTE_LINK_FLAG_GONE | CAHUTE_LINK_FLAG_RECEIVER)
        )) {
        switch (link->protocol) {
        case CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK:
            cahute_casiolink_terminate(link);
            break;

        case CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN:
        case CAHUTE_LINK_PROTOCOL_USB_SEVEN:
            cahute_seven_terminate(link);
            break;
        }
    }

    if (link->flags & CAHUTE_LINK_FLAG_CLOSE_MEDIUM) {
        switch (link->medium) {
#ifdef CAHUTE_LINK_MEDIUM_POSIX_SERIAL
        case CAHUTE_LINK_MEDIUM_POSIX_SERIAL:
            close(link->medium_state.posix.fd);
            break;
#endif

#if defined(CAHUTE_LINK_MEDIUM_WIN32_CESG) \
    || defined(CAHUTE_LINK_MEDIUM_WIN32_SERIAL)
# if defined(CAHUTE_LINK_MEDIUM_WIN32_CESG)
        case CAHUTE_LINK_MEDIUM_WIN32_CESG:
# endif
# if defined(CAHUTE_LINK_MEDIUM_WIN32_SERIAL)
        case CAHUTE_LINK_MEDIUM_WIN32_SERIAL:
# endif
            CancelIo(link->medium_state.windows.handle);
            CloseHandle(link->medium_state.windows.overlapped.hEvent);
            CloseHandle(link->medium_state.windows.handle);
            break;
#endif

#ifdef CAHUTE_LINK_MEDIUM_WIN32_UMS
        case CAHUTE_LINK_MEDIUM_WIN32_UMS:
            CloseHandle(link->medium_state.windows.handle);
            break;
#endif

#ifdef CAHUTE_LINK_MEDIUM_LIBUSB
        case CAHUTE_LINK_MEDIUM_LIBUSB:
            libusb_close(link->medium_state.libusb.handle);
            if (link->medium_state.libusb.context)
                libusb_exit(link->medium_state.libusb.context);
            break;
#endif
        }
    }

    free(link);
}
