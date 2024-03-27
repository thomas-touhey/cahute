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
#include "picture.h"
#include <stdio.h>

CAHUTE_BEGIN_NAMESPACE
CAHUTE_DECLARE_TYPE(cahute_link)
CAHUTE_DECLARE_TYPE(cahute_device_info)
CAHUTE_DECLARE_TYPE(cahute_storage_entry)

/* Preprogrammed ROM information available. */
#define CAHUTE_DEVICE_INFO_FLAG_PREPROG 0x0001
/* Bootcode information available. */
#define CAHUTE_DEVICE_INFO_FLAG_BOOTCODE 0x0002
/* OS information available. */
#define CAHUTE_DEVICE_INFO_FLAG_OS 0x0004

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

typedef int(cahute_process_frame_func)(
    void *cahute__cookie,
    cahute_frame const *cahute__frame
);
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
 * Open a serial link.
 * ---
 * Protocol to open the link with. */

#define CAHUTE_SERIAL_PROTOCOL_MASK      0x0000000F
#define CAHUTE_SERIAL_PROTOCOL_AUTO      0x00000000 /* Protocol detection. */
#define CAHUTE_SERIAL_PROTOCOL_CASIOLINK 0x00000001 /* CASIOLINK. */
#define CAHUTE_SERIAL_PROTOCOL_SEVEN     0x00000007 /* Protocol 7.00. */
#define CAHUTE_SERIAL_PROTOCOL_SEVEN_OHP 0x00000009 /* Protocol 7.00 OHP. */

/* CASIOLINK variant to select. */

#define CAHUTE_SERIAL_CASIOLINK_VARIANT_MASK   0x00000070
#define CAHUTE_SERIAL_CASIOLINK_VARIANT_AUTO   0x00000010 /* Auto-detect. */
#define CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS40  0x00000020 /* CAS40. */
#define CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS50  0x00000030 /* CAS50. */
#define CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS100 0x00000040 /* CAS100. */

/* Stop bit settings (1 or 2). */

#define CAHUTE_SERIAL_STOP_MASK 0x00000300
#define CAHUTE_SERIAL_STOP_ONE  0x00000100 /* 1 stop bit. */
#define CAHUTE_SERIAL_STOP_TWO  0x00000200 /* 2 stop bits. */

/* Parity settings. */

#define CAHUTE_SERIAL_PARITY_MASK 0x00000C00
#define CAHUTE_SERIAL_PARITY_OFF  0x00000400 /* No parity. */
#define CAHUTE_SERIAL_PARITY_EVEN 0x00000800 /* Even parity. */
#define CAHUTE_SERIAL_PARITY_ODD  0x00000C00 /* Odd parity. */

/* XON/XOFF behaviour. */

#define CAHUTE_SERIAL_XONXOFF_MASK    0x00003000
#define CAHUTE_SERIAL_XONXOFF_DISABLE 0x00001000 /* Disable XON/XOFF */
#define CAHUTE_SERIAL_XONXOFF_ENABLE  0x00002000 /* Enable XON/XOFF */

/* DTR behaviour. */

#define CAHUTE_SERIAL_DTR_MASK      0x0000C000
#define CAHUTE_SERIAL_DTR_DISABLE   0x00004000
#define CAHUTE_SERIAL_DTR_ENABLE    0x00008000
#define CAHUTE_SERIAL_DTR_HANDSHAKE 0x0000C000

/* RTS behaviour. */

#define CAHUTE_SERIAL_RTS_MASK      0x00030000
#define CAHUTE_SERIAL_RTS_DISABLE   0x00010000
#define CAHUTE_SERIAL_RTS_ENABLE    0x00020000
#define CAHUTE_SERIAL_RTS_HANDSHAKE 0x00030000

/* Protocol-related behaviour specific flags. */

#define CAHUTE_SERIAL_RECEIVER 0x00100000 /* Start as passive/receiver. */
#define CAHUTE_SERIAL_NOCHECK  0x00200000 /* Disable the initial handshake. */
#define CAHUTE_SERIAL_NODISC   0x00400000 /* Disable platform discovery. */
#define CAHUTE_SERIAL_NOTERM   0x00800000 /* Disable the term handshake. */

CAHUTE_BEGIN_DECLS

CAHUTE_WUR CAHUTE_EXTERN(int) cahute_open_serial_link
    OF((cahute_link * *cahute__linkp,
        unsigned long cahute__flags,
        char const *cahute__name,
        unsigned long cahute__speed));

CAHUTE_END_DECLS

/* ---
 * Open a USB link.
 * --- */

#define CAHUTE_USB_NOCHECK  0x0001 /* Disable the initial handshake. */
#define CAHUTE_USB_NODISC   0x0002 /* Disable platform discovery. */
#define CAHUTE_USB_NOTERM   0x0004 /* Disable the terminating handshake. */
#define CAHUTE_USB_RECEIVER 0x0010 /* Act as the receiver. */
#define CAHUTE_USB_OHP      0x0020 /* Use screen streaming mode. */

CAHUTE_BEGIN_DECLS

CAHUTE_WUR CAHUTE_EXTERN(int) cahute_open_usb_link
    OF((cahute_link * *cahute__linkp,
        unsigned long cahute__flags,
        int cahute__bus,
        int cahute__address));

CAHUTE_WUR CAHUTE_EXTERN(int) cahute_open_simple_usb_link
    OF((cahute_link * *cahute__linkp, unsigned long cahute__flags));

CAHUTE_END_DECLS

/* ---
 * Common link operations.
 * --- */

CAHUTE_BEGIN_DECLS

CAHUTE_EXTERN(void) cahute_close_link OF((cahute_link * cahute__link));

CAHUTE_EXTERN(int)
cahute_get_device_info
    OF((cahute_link * cahute__link, cahute_device_info **cahute__infop));

CAHUTE_EXTERN(int)
cahute_negotiate_serial_params
    OF((cahute_link * cahute__link,
        unsigned long cahute__flags,
        unsigned long cahute__speed));

CAHUTE_EXTERN(int)
cahute_receive_screen
    OF((cahute_link * cahute__link,
        cahute_process_frame_func *cahute__callback,
        void *cahute__cookie));

CAHUTE_EXTERN(int)
cahute_request_storage_capacity
    OF((cahute_link * link, char const *storage, unsigned long *capacityp));

#define CAHUTE_SEND_FILE_FLAG_FORCE    0x00000001 /* Force overwrite. */
#define CAHUTE_SEND_FILE_FLAG_OPTIMIZE 0x00000002 /* Automatically optimize. */

CAHUTE_EXTERN(int)
cahute_send_file_to_storage
    OF((cahute_link * cahute__link,
        unsigned long cahute__flags,
        char const *cahute__directory,
        char const *cahute__name,
        char const *cahute__storage,
        FILE *cahute__filep,
        cahute_confirm_overwrite_func *cahute__overwrite_func,
        void *cahute__overwrite_cookie,
        cahute_progress_func *cahute__progress_func,
        void *cahute__progress_cookie));

CAHUTE_EXTERN(int)
cahute_request_file_from_storage
    OF((cahute_link * cahute__link,
        char const *cahute__directory,
        char const *cahute__name,
        char const *cahute__storage,
        FILE *cahute__filep,
        cahute_progress_func *cahute__progress_func,
        void *cahute__progress_cookie));

CAHUTE_EXTERN(int)
cahute_copy_file_on_storage
    OF((cahute_link * link,
        char const *source_directory,
        char const *source_name,
        char const *target_directory,
        char const *target_name,
        char const *storage));

CAHUTE_EXTERN(int)
cahute_delete_file_from_storage
    OF((cahute_link * cahute__link,
        char const *cahute__directory,
        char const *cahute__name,
        char const *cahute__storage));

CAHUTE_EXTERN(int)
cahute_list_storage_entries
    OF((cahute_link * cahute__link,
        char const *cahute__storage,
        cahute_list_storage_entry_func *cahute__callback,
        void *cahute__cookie));

CAHUTE_EXTERN(int)
cahute_reset_storage
    OF((cahute_link * cahute__link, char const *cahute__storage));

CAHUTE_EXTERN(int)
cahute_optimize_storage
    OF((cahute_link * cahute__link, char const *cahute__storage));

CAHUTE_EXTERN(int)
cahute_backup_rom
    OF((cahute_link * cahute__link,
        cahute_u8 **cahute__romp,
        size_t *cahute__sizep,
        cahute_progress_func *cahute__progress_func,
        void *cahute__progress_cookie));

CAHUTE_EXTERN(int)
cahute_upload_and_run_program
    OF((cahute_link * cahute__link,
        cahute_u8 const *cahute__program,
        size_t cahute__program_size,
        unsigned long cahute__load_address,
        unsigned long cahute__start_address,
        cahute_progress_func *cahute__progress_func,
        void *cahute__progress_cookie));

#define CAHUTE_FLASH_FLAG_RESET_SMEM 0x00000001 /* Also erase the SMEM. */

CAHUTE_EXTERN(int)
cahute_flash_system_using_fxremote_method
    OF((cahute_link * cahute__link,
        unsigned long cahute__flags,
        cahute_u8 const *cahute__system,
        size_t cahute__system_size));

CAHUTE_END_DECLS

CAHUTE_END_NAMESPACE

#endif /* CAHUTE_LINK_H */
