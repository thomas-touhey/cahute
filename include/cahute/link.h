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

#ifndef CAHUTE_LINK_H
#define CAHUTE_LINK_H 1
#include "cdefs.h"
#include "file.h"
#include "picture.h"
#include <stdio.h>

CAHUTE_BEGIN_NAMESPACE
CAHUTE_BEGIN_DECLS

CAHUTE_DECLARE_TYPE(cahute_link)
CAHUTE_DECLARE_TYPE(cahute_device_info)
CAHUTE_DECLARE_TYPE(cahute_storage_entry)

/* Preprogrammed ROM information available. */
#define CAHUTE_DEVICE_INFO_FLAG_PREPROG 0x0001UL
/* Bootcode information available. */
#define CAHUTE_DEVICE_INFO_FLAG_BOOTCODE 0x0002UL
/* OS information available. */
#define CAHUTE_DEVICE_INFO_FLAG_OS 0x0004UL

struct cahute_device_info {
    unsigned long cahute_device_info_flags;

    /* Preprogrammed ROM information. */
    unsigned long cahute_device_info_rom_capacity;
    char const *cahute_device_info_rom_version;

    /* Flash ROM and RAM information. */
    unsigned long cahute_device_info_flash_rom_capacity;
    unsigned long cahute_device_info_ram_capacity;

    /* Bootcode information. */
    char const *cahute_device_info_bootcode_version;
    unsigned long cahute_device_info_bootcode_offset;
    unsigned long cahute_device_info_bootcode_size;

    /* OS information. */
    char const *cahute_device_info_os_version;
    unsigned long cahute_device_info_os_offset;
    unsigned long cahute_device_info_os_size;

    /* Other information. */
    char const *cahute_device_info_product_id;
    char const *cahute_device_info_username;
    char const *cahute_device_info_organisation;
    char const *cahute_device_info_hwid;
    char const *cahute_device_info_cpuid;
};

typedef int(cahute_confirm_overwrite_func)(void *cahute__cookie);

struct cahute_storage_entry {
    char const *cahute_storage_entry_directory;
    char const *cahute_storage_entry_name;
    unsigned long cahute_storage_entry_size;
};

typedef int(cahute_list_storage_entry_func)(
    void *cahute__cookie,
    cahute_storage_entry const *cahute__entry
);

typedef void(cahute_progress_func)(
    void *cahute__cookie,
    unsigned long cahute__quot,
    unsigned long cahute__denom
);

/* ---
 * Link management.
 * ---
 * Protocol to open a serial link with. */

#define CAHUTE_SERIAL_PROTOCOL_MASK      0x0000000FUL
#define CAHUTE_SERIAL_PROTOCOL_AUTO      0x00000000UL /* Protocol detection. */
#define CAHUTE_SERIAL_PROTOCOL_NONE      0x00000001UL /* Generic protocol. */
#define CAHUTE_SERIAL_PROTOCOL_CASIOLINK 0x00000002UL /* CASIOLINK. */
#define CAHUTE_SERIAL_PROTOCOL_SEVEN     0x00000003UL /* Protocol 7.00. */
#define CAHUTE_SERIAL_PROTOCOL_SEVEN_OHP 0x00000004UL /* Protocol 7.00 OHP. */

/* CASIOLINK variant to select. */

#define CAHUTE_SERIAL_CASIOLINK_VARIANT_MASK   0x00000070UL
#define CAHUTE_SERIAL_CASIOLINK_VARIANT_AUTO   0x00000010UL /* Auto-detect. */
#define CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS40  0x00000020UL /* CAS40. */
#define CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS50  0x00000030UL /* CAS50. */
#define CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS100 0x00000040UL /* CAS100. */
#define CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS300 0x00000050UL /* CAS300. */

/* Stop bit settings (1 or 2). */

#define CAHUTE_SERIAL_STOP_MASK 0x00000300UL
#define CAHUTE_SERIAL_STOP_ONE  0x00000100UL /* 1 stop bit. */
#define CAHUTE_SERIAL_STOP_TWO  0x00000200UL /* 2 stop bits. */

/* Parity settings. */

#define CAHUTE_SERIAL_PARITY_MASK 0x00000C00UL
#define CAHUTE_SERIAL_PARITY_OFF  0x00000400UL /* No parity. */
#define CAHUTE_SERIAL_PARITY_EVEN 0x00000800UL /* Even parity. */
#define CAHUTE_SERIAL_PARITY_ODD  0x00000C00UL /* Odd parity. */

/* XON/XOFF behaviour. */

#define CAHUTE_SERIAL_XONXOFF_MASK    0x00003000UL
#define CAHUTE_SERIAL_XONXOFF_DISABLE 0x00001000UL /* Disable XON/XOFF */
#define CAHUTE_SERIAL_XONXOFF_ENABLE  0x00002000UL /* Enable XON/XOFF */

/* DTR behaviour. */

#define CAHUTE_SERIAL_DTR_MASK      0x0000C000UL
#define CAHUTE_SERIAL_DTR_DISABLE   0x00004000UL
#define CAHUTE_SERIAL_DTR_ENABLE    0x00008000UL
#define CAHUTE_SERIAL_DTR_HANDSHAKE 0x0000C000UL

/* RTS behaviour. */

#define CAHUTE_SERIAL_RTS_MASK      0x00030000UL
#define CAHUTE_SERIAL_RTS_DISABLE   0x00010000UL
#define CAHUTE_SERIAL_RTS_ENABLE    0x00020000UL
#define CAHUTE_SERIAL_RTS_HANDSHAKE 0x00030000UL

/* Protocol-related behaviour specific flags. */

#define CAHUTE_SERIAL_RECEIVER 0x00100000UL /* Start as passive/receiver. */
#define CAHUTE_SERIAL_NOCHECK  0x00200000UL /* Disable initial handshake. */
#define CAHUTE_SERIAL_NODISC   0x00400000UL /* Disable platform discovery. */
#define CAHUTE_SERIAL_NOTERM   0x00800000UL /* Disable term handshake. */

/* USB flags. */

#define CAHUTE_USB_NOCHECK  0x00000001UL /* Disable initial handshake. */
#define CAHUTE_USB_NODISC   0x00000002UL /* Disable platform discovery. */
#define CAHUTE_USB_NOTERM   0x00000004UL /* Disable terminating handshake. */
#define CAHUTE_USB_RECEIVER 0x00000010UL /* Act as the receiver. */
#define CAHUTE_USB_OHP      0x00000020UL /* Use screen streaming mode. */
#define CAHUTE_USB_NOPROTO  0x00000040UL /* Open a generic device. */

/* Simple USB filter */

#define CAHUTE_USB_FILTER_MASK   0x000F0000UL
#define CAHUTE_USB_FILTER_ANY    0x00000000UL
#define CAHUTE_USB_FILTER_CAS300 0x00010000UL
#define CAHUTE_USB_FILTER_SEVEN  0x00020000UL
#define CAHUTE_USB_FILTER_SERIAL 0x00030000UL
#define CAHUTE_USB_FILTER_UMS    0x00040000UL

CAHUTE_WUR CAHUTE_EXTERN(int) cahute_open_serial_link(
    cahute_link **cahute__linkp,
    unsigned long cahute__flags,
    char const *cahute__name,
    unsigned long cahute__speed
);

CAHUTE_WUR CAHUTE_EXTERN(int) cahute_open_usb_link(
    cahute_link **cahute__linkp,
    unsigned long cahute__flags,
    int cahute__bus,
    int cahute__address
);

CAHUTE_WUR CAHUTE_EXTERN(int) cahute_open_simple_usb_link(
    cahute_link **cahute__linkp,
    unsigned long cahute__flags
);

CAHUTE_EXTERN(void) cahute_close_link(cahute_link *cahute__link);

/* ---
 * Link medium access.
 * --- */

CAHUTE_EXTERN(int)
cahute_receive_on_link(
    cahute_link *cahute__link,
    cahute_u8 *cahute__buf,
    size_t cahute__size,
    unsigned long cahute__first_timeout,
    unsigned long cahute__next_timeout
);

CAHUTE_EXTERN(int)
cahute_send_on_link(
    cahute_link *cahute__link,
    cahute_u8 const *cahute__buf,
    size_t cahute__size
);

CAHUTE_EXTERN(int)
cahute_set_serial_params_to_link(
    cahute_link *cahute__link,
    unsigned long cahute__flags,
    unsigned long cahute__speed
);

/* ---
 * Device metadata access.
 * --- */

CAHUTE_EXTERN(int)
cahute_get_device_info(
    cahute_link *cahute__link,
    cahute_device_info **cahute__infop
);

/* ---
 * Data transfer operations.
 * --- */

CAHUTE_EXTERN(int)
cahute_receive_data(
    cahute_link *cahute__link,
    cahute_data **cahute__datap,
    unsigned long cahute__timeout
);

CAHUTE_EXTERN(int)
cahute_receive_screen(
    cahute_link *cahute__link,
    cahute_frame **cahute__framep,
    unsigned long cahute__timeout
);

/* ---
 * Control operations.
 * --- */

CAHUTE_EXTERN(int)
cahute_negotiate_serial_params(
    cahute_link *cahute__link,
    unsigned long cahute__flags,
    unsigned long cahute__speed
);

CAHUTE_EXTERN(int)
cahute_request_storage_capacity(
    cahute_link *cahute__link,
    char const *cahute__storage,
    unsigned long *cahute__capacityp
);

#define CAHUTE_SEND_FILE_FLAG_FORCE    0x00000001UL /* Force overwrite. */
#define CAHUTE_SEND_FILE_FLAG_OPTIMIZE 0x00000002UL /* Auto. optimize. */
#define CAHUTE_SEND_FILE_FLAG_DELETE   0x00000004UL /* Delete first. */

CAHUTE_EXTERN(int)
cahute_send_file_to_storage(
    cahute_link *cahute__link,
    unsigned long cahute__flags,
    char const *cahute__directory,
    char const *cahute__name,
    char const *cahute__storage,
    cahute_file *cahute__file,
    cahute_confirm_overwrite_func *cahute__overwrite_func,
    void *cahute__overwrite_cookie,
    cahute_progress_func *cahute__progress_func,
    void *cahute__progress_cookie
);

CAHUTE_EXTERN(int)
cahute_request_file_from_storage(
    cahute_link *cahute__link,
    char const *cahute__directory,
    char const *cahute__name,
    char const *cahute__storage,
    void const *cahute__path,
    int cahute__path_type,
    cahute_progress_func *cahute__progress_func,
    void *cahute__progress_cookie
);

CAHUTE_EXTERN(int)
cahute_copy_file_on_storage(
    cahute_link *cahute__link,
    char const *cahute__source_directory,
    char const *cahute__source_name,
    char const *cahute__target_directory,
    char const *cahute__target_name,
    char const *cahute__storage
);

CAHUTE_EXTERN(int)
cahute_delete_file_from_storage(
    cahute_link *cahute__link,
    char const *cahute__directory,
    char const *cahute__name,
    char const *cahute__storage
);

CAHUTE_EXTERN(int)
cahute_list_storage_entries(
    cahute_link *cahute__link,
    char const *cahute__storage,
    cahute_list_storage_entry_func *cahute__callback,
    void *cahute__cookie
);

CAHUTE_EXTERN(int)
cahute_reset_storage(cahute_link *cahute__link, char const *cahute__storage);

CAHUTE_EXTERN(int)
cahute_optimize_storage(
    cahute_link *cahute__link,
    char const *cahute__storage
);

CAHUTE_EXTERN(int)
cahute_backup_rom(
    cahute_link *cahute__link,
    cahute_u8 **cahute__romp,
    size_t *cahute__sizep,
    cahute_progress_func *cahute__progress_func,
    void *cahute__progress_cookie
);

CAHUTE_EXTERN(int)
cahute_upload_and_run_program(
    cahute_link *cahute__link,
    cahute_u8 const *cahute__program,
    size_t cahute__program_size,
    unsigned long cahute__load_address,
    unsigned long cahute__start_address,
    cahute_progress_func *cahute__progress_func,
    void *cahute__progress_cookie
);

#define CAHUTE_FLASH_FLAG_RESET_SMEM 0x00000001UL /* Also erase the SMEM. */

CAHUTE_EXTERN(int)
cahute_flash_system_using_fxremote_method(
    cahute_link *cahute__link,
    unsigned long cahute__flags,
    cahute_u8 const *cahute__system,
    size_t cahute__system_size
);

CAHUTE_END_DECLS
CAHUTE_END_NAMESPACE

#endif /* CAHUTE_LINK_H */
