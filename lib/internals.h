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

#ifndef INTERNALS_H
#define INTERNALS_H 1

/* For Microsoft Windows, we want to explicitely select the target system to
 * avoid breaking compatibility if possible.
 * See the following for more information:
 *
 * https://learn.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt */
#define WINVER 0x0501 /* Windows XP */

#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__)
# define WIN32_ENABLED 1
#else
# define WIN32_ENABLED 0
#endif

#if defined(__DJGPP) || defined(__DJGPP__)
# define DJGPP_ENABLED 1
#else
# define DJGPP_ENABLED 0
#endif

#if WIN32_ENABLED
# define POSIX_ENABLED 0
#elif defined(__unix__) && __unix__ \
    || (defined(__APPLE__) || defined(__MACH__))
# define POSIX_ENABLED 1
#else
# define POSIX_ENABLED 0
#endif

#if defined(AMIGA) || defined(__amigaos__)
# define AMIGAOS_ENABLED 1
#else
# define AMIGAOS_ENABLED 0
#endif

#if AMIGAOS_ENABLED
# include <exec/types.h>
# include <exec/errors.h>
# include <exec/io.h>
# include <exec/ports.h>
# include <dos/dos.h>
# include <proto/exec.h>
# include <devices/serial.h>
# include <devices/timer.h>
#endif

#include <cahute.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if POSIX_ENABLED
# include <fcntl.h>
# include <sys/ioctl.h>
# include <sys/stat.h>
# include <termios.h>
# include <unistd.h>
#endif

#if LIBUSB_ENABLED
# include <libusb.h>
#endif

#include <compat.h>

CAHUTE_DECLARE_TYPE(cahute_link_medium)
CAHUTE_DECLARE_TYPE(cahute_file_medium)
CAHUTE_DECLARE_TYPE(cahute_casiolink_data_description)

#if AMIGAOS_ENABLED
CAHUTE_EXTERN(int)
cahute_get_amiga_timer(
    struct MsgPort **msg_portp,
    struct timerequest **timerp
);
#endif

/* ---
 * Endianess management.
 * --- */

CAHUTE_EXTERN(cahute_u16) cahute_be16toh(cahute_u16 cahute__x);
CAHUTE_EXTERN(cahute_u16) cahute_le16toh(cahute_u16 cahute__x);
CAHUTE_EXTERN(cahute_u32) cahute_be32toh(cahute_u32 cahute__x);
CAHUTE_EXTERN(cahute_u32) cahute_le32toh(cahute_u32 cahute__x);

CAHUTE_EXTERN(cahute_u16) cahute_htobe16(cahute_u16 cahute__x);
CAHUTE_EXTERN(cahute_u16) cahute_htole16(cahute_u16 cahute__x);
CAHUTE_EXTERN(cahute_u32) cahute_htobe32(cahute_u32 cahute__x);
CAHUTE_EXTERN(cahute_u32) cahute_htole32(cahute_u32 cahute__x);

/* Try to get native macros. */
#if defined(__APPLE__)
# include <libkern/OSByteOrder.h>
# define cahute_macro_be16toh(CAHUTE__X) OSSwapBigToHostInt16(CAHUTE__X)
# define cahute_macro_le16toh(CAHUTE__X) OSSwapLittleToHostInt16(CAHUTE__X)
# define cahute_macro_be32toh(CAHUTE__X) OSSwapBigToHostInt32(CAHUTE__X)
# define cahute_macro_le32toh(CAHUTE__X) OSSwapLittleToHostInt32(CAHUTE__X)
# define cahute_macro_htobe16(CAHUTE__X) OSSwapHostToBigInt16(CAHUTE__X)
# define cahute_macro_htole16(CAHUTE__X) OSSwapHostToLittleInt16(CAHUTE__X)
# define cahute_macro_htobe32(CAHUTE__X) OSSwapHostToBigInt32(CAHUTE__X)
# define cahute_macro_htole32(CAHUTE__X) OSSwapHostToLittleInt32(CAHUTE__X)
#elif defined(__OpenBSD__)
# include <sys/endian.h>
# define cahute_macro_be16toh(CAHUTE__X) be16toh(CAHUTE__X)
# define cahute_macro_le16toh(CAHUTE__X) le16toh(CAHUTE__X)
# define cahute_macro_be32toh(CAHUTE__X) be32toh(CAHUTE__X)
# define cahute_macro_le32toh(CAHUTE__X) le32toh(CAHUTE__X)
# define cahute_macro_htobe16(CAHUTE__X) htobe16(CAHUTE__X)
# define cahute_macro_htole16(CAHUTE__X) htole16(CAHUTE__X)
# define cahute_macro_htobe32(CAHUTE__X) htobe32(CAHUTE__X)
# define cahute_macro_htole32(CAHUTE__X) htole32(CAHUTE__X)
#elif defined(__GLIBC__) && defined(__USE_MISC)
# include <endian.h>
# define cahute_macro_be16toh(CAHUTE__X) be16toh(CAHUTE__X)
# define cahute_macro_le16toh(CAHUTE__X) le16toh(CAHUTE__X)
# define cahute_macro_be32toh(CAHUTE__X) be32toh(CAHUTE__X)
# define cahute_macro_le32toh(CAHUTE__X) le32toh(CAHUTE__X)
# define cahute_macro_htobe16(CAHUTE__X) htobe16(CAHUTE__X)
# define cahute_macro_htole16(CAHUTE__X) htole16(CAHUTE__X)
# define cahute_macro_htobe32(CAHUTE__X) htobe32(CAHUTE__X)
# define cahute_macro_htole32(CAHUTE__X) htole32(CAHUTE__X)
#endif

/* CAHUTE_NO_ENDIAN may be defined by cdefs.c to be able to define the
 * functions prototyped above. */
#ifndef CAHUTE_NO_ENDIAN
# ifdef cahute_macro_be16toh
#  define cahute_be16toh(CAHUTE__X) cahute_macro_be16toh(CAHUTE__X)
# endif
# ifdef cahute_macro_le16toh
#  define cahute_le16toh(CAHUTE__X) cahute_macro_le16toh(CAHUTE__X)
# endif
# ifdef cahute_macro_be32toh
#  define cahute_be32toh(CAHUTE__X) cahute_macro_be32toh(CAHUTE__X)
# endif
# ifdef cahute_macro_le32toh
#  define cahute_le32toh(CAHUTE__X) cahute_macro_le32toh(CAHUTE__X)
# endif
# ifdef cahute_macro_htobe16
#  define cahute_htobe16(CAHUTE__X) cahute_macro_htobe16(CAHUTE__X)
# endif
# ifdef cahute_macro_htole16
#  define cahute_htole16(CAHUTE__X) cahute_macro_htole16(CAHUTE__X)
# endif
# ifdef cahute_macro_htobe32
#  define cahute_htobe32(CAHUTE__X) cahute_macro_htobe32(CAHUTE__X)
# endif
# ifdef cahute_macro_htole32
#  define cahute_htole32(CAHUTE__X) cahute_macro_htole32(CAHUTE__X)
# endif
#endif

/* ---
 * Logging internals.
 * --- */

CAHUTE_EXTERN(void)
cahute_log_message(
    int cahute__loglevel,
    char const *cahute__func,
    char const *cahute__format,
    ...
);
CAHUTE_EXTERN(void)
cahute_log_memory(
    int cahute__loglevel,
    char const *cahute__func,
    void const *cahute__memory,
    size_t cahute__size
);

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
# define CAHUTE_LOGFUNC __func__
#elif !defined(__STRICT_ANSI__) && CAHUTE_GNUC_PREREQ(2, 0)
# define CAHUTE_LOGFUNC __FUNCTION__
#else
# define CAHUTE_LOGFUNC NULL
#endif

#define ll_info  10, CAHUTE_LOGFUNC
#define ll_warn  20, CAHUTE_LOGFUNC
#define ll_error 30, CAHUTE_LOGFUNC
#define ll_fatal 40, CAHUTE_LOGFUNC

#define msg cahute_log_message
#define mem cahute_log_memory

/* Macro to print a message and return CAHUTE_ERROR_IMPL.
 * This is necessary to avoid having to track down exactly what was not
 * implemented in the chain using the message.
 * Usage of this macro is enforced with pre-commit. */
#define CAHUTE_RETURN_IMPL(MESSAGE) \
    { \
        msg(ll_error, MESSAGE); \
        return CAHUTE_ERROR_IMPL /* Comment to prevent match by hook. */; \
    } \
    (void)0 /* Force introducing a semicolon. */

#if WIN32_ENABLED
/**
 * Log a Windows API error.
 *
 * This is implemented as a separate function to the rest, because gathering
 * an error message for a given error code is quite lengthy.
 *
 * @param func_name Name of the function from which the log is emitted.
 * @param win_func Name of the Windows API function that returned the
 *        error.
 * @param code Windows API error code that was actually returned.
 */
CAHUTE_INLINE(void)
cahute__log_win_error(
    char const *func_name,
    char const *win_func,
    DWORD code
) {
    char buf[1024];
    DWORD buf_size;

    buf_size = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf,
        1023,
        NULL
    );

    if (!buf_size) {
        cahute_log_message(
            30,
            func_name,
            "Error 0x%08lX occurred in %s.",
            code,
            win_func
        );
        return;
    }

    buf[buf_size] = '\0';
    cahute_log_message(
        30,
        func_name,
        "Error 0x%08lX occurred in %s: %s",
        code,
        win_func,
        buf
    );
}

# define log_windows_error(FUNC, CODE) \
     cahute__log_win_error(CAHUTE_LOGFUNC, FUNC, CODE)
#endif

/* ---
 * Link internals.
 * --- */

#define CAHUTE_LINK_MEDIUM_READ_BUFFER_SIZE 32768U

/* Flags that can be present on a medium at runtime. */
#define CAHUTE_LINK_MEDIUM_FLAG_GONE 0x00000001UL /* No longer available. */

/* Flags that can be present on a link at runtime. */
#define CAHUTE_LINK_FLAG_CLOSE_MEDIUM 0x00000001UL
#define CAHUTE_LINK_FLAG_TERMINATE    0x00000002UL /* Should terminate. */
#define CAHUTE_LINK_FLAG_RECEIVER     0x00000004UL /* Act as a receiver. */

#define CAHUTE_LINK_FLAG_TERMINATED    0x00000200UL /* Was terminated! */
#define CAHUTE_LINK_FLAG_IRRECOVERABLE 0x00000400UL /* Cannot recover. */
#define CAHUTE_LINK_FLAG_ALMODE        0x00000800UL /* CAS40 AL received. */

/* Medium types allowed. */
#if POSIX_ENABLED
# define CAHUTE_LINK_MEDIUM_POSIX_SERIAL 1
#endif
#if WIN32_ENABLED
# define CAHUTE_LINK_MEDIUM_WIN32_SERIAL 2
# define CAHUTE_LINK_MEDIUM_WIN32_CESG   3
# define CAHUTE_LINK_MEDIUM_WIN32_UMS    4
#endif
#if LIBUSB_ENABLED
# define CAHUTE_LINK_MEDIUM_LIBUSB     5
# define CAHUTE_LINK_MEDIUM_LIBUSB_UMS 6
#endif
#if AMIGAOS_ENABLED
# define CAHUTE_LINK_MEDIUM_AMIGAOS_SERIAL 7
#endif

/* Protocol selection for 'initialize_link_protocol()'. */
#define CAHUTE_LINK_PROTOCOL_SERIAL_AUTO      0
#define CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK 1
#define CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN     2
#define CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP 3
#define CAHUTE_LINK_PROTOCOL_USB_SEVEN        4
#define CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP    5
#define CAHUTE_LINK_PROTOCOL_USB_MASS_STORAGE 6

/* CASIOLINK variant selection for the same function. */
#define CAHUTE_CASIOLINK_VARIANT_AUTO   0
#define CAHUTE_CASIOLINK_VARIANT_CAS40  1
#define CAHUTE_CASIOLINK_VARIANT_CAS50  2
#define CAHUTE_CASIOLINK_VARIANT_CAS100 3

#if defined(CAHUTE_LINK_MEDIUM_POSIX_SERIAL)
/**
 * POSIX medium state.
 *
 * @property fd File descriptor on the opened medium.
 */
struct cahute_link_posix_medium_state {
    int fd;
};
#endif

#if defined(CAHUTE_LINK_MEDIUM_WIN32_SERIAL) \
    || defined(CAHUTE_LINK_MEDIUM_WIN32_CESG)
/**
 * Windows API medium state.
 *
 * @property handle Device handle.
 * @property overlapped Overlapped I/O adapter.
 * @property read_in_progress Whether a read is currently in progress or not.
 */
struct cahute_link_windows_medium_state {
    HANDLE handle;
    OVERLAPPED overlapped;
    DWORD received;
    int read_in_progress;
};
#endif

#if defined(CAHUTE_LINK_MEDIUM_AMIGAOS_SERIAL)
/**
 * AmigaOS serial device medium state.
 *
 * @property msg_port Message port with which the device was opened.
 * @property io IO structure of the device.
 */
struct cahute_link_amigaos_serial_medium_state {
    struct MsgPort *msg_port;
    struct IOExtSer *io;
};
#endif

#if defined(CAHUTE_LINK_MEDIUM_WIN32_UMS)
/**
 * Windows API UMS (SCSI) medium state.
 *
 * @property handle Device handle.
 */
struct cahute_link_windows_ums_medium_state {
    HANDLE handle;
};
#endif

#if defined(CAHUTE_LINK_MEDIUM_LIBUSB)
/**
 * libusb device medium state.
 *
 * @property context libusb context to close once the link is closed.
 * @property handle libusb device handle which to use to make USB requests.
 * @property bulk_in Bulk IN endpoint address to use for reading.
 * @property bulk_out Bulk OUT endpoint address to use for writing.
 */
struct cahute_link_libusb_medium_state {
    libusb_context *context;
    libusb_device_handle *handle;
    int bulk_in;
    int bulk_out;
};
#endif

/**
 * Medium state, to be used depending on the link flags regarding the medium.
 *
 * @property posix Medium state if the selected medium type is POSIX_SERIAL.
 * @property windows Medium state if the selected medium type is WIN32_SERIAL
 *           or WIN32_CESG.
 * @property windows_ums Medium state if the selected medium type is
 *           WIN32_UMS (SCSI over a Windows HANDLE).
 * @property libusb Medium state if the selected medium type is LIBUSB.
 * @property amigaos_serial Medium state if the selected medium type is
 *           AMIGAOS_SERIAL.
 */
union cahute_link_medium_state {
#if defined(CAHUTE_LINK_MEDIUM_POSIX_SERIAL)
    struct cahute_link_posix_medium_state posix;
#endif
#if defined(CAHUTE_LINK_MEDIUM_WIN32_SERIAL) \
    || defined(CAHUTE_LINK_MEDIUM_WIN32_CESG)
    struct cahute_link_windows_medium_state windows;
#endif
#if defined(CAHUTE_LINK_MEDIUM_WIN32_UMS)
    struct cahute_link_windows_ums_medium_state windows_ums;
#endif
#if defined(CAHUTE_LINK_MEDIUM_LIBUSB)
    struct cahute_link_libusb_medium_state libusb;
#endif
#if defined(CAHUTE_LINK_MEDIUM_AMIGAOS_SERIAL)
    struct cahute_link_amigaos_serial_medium_state amigaos_serial;
#endif
};

/**
 * Medium-related information.
 *
 * @property type Medium type, as any ``CAHUTE_LINK_MEDIUM_*`` constant
 *           representing the medium state to use and how to use it.
 * @property flags Flags for the medium.
 * @property state State of the specific medium to use, e.g. opened
 *           handles and contexts to close at link closing.
 *           The read buffer is not included within this property.
 * @property serial_flags Current serial flags, as or'd
 *           ``CAHUTE_SERIAL_FLAG_*`` constants.
 * @property serial_speed Current serial speed.
 * @property read_buffer Buffer for reading from the medium in a
 *           stream-like interface. See ``cahute_receive_on_link_medium``
 *           definition for more information.
 *           Guaranteed to be aligned to a multiple of 32 bytes.
 * @property read_start Offset at which the unread data starts within
 *           the read buffer for the medium.
 * @property read_size Number of unread bytes in the read buffer for the
 *           medium, starting at the offset stored in ``read_start``.
 */
struct cahute_link_medium {
    int type;
    unsigned int flags;

    unsigned long serial_flags;
    unsigned long serial_speed;

    union cahute_link_medium_state state;

    /* Read buffer. See ``cahute_receive_on_link_medium`` definition for more
     * information. */
    size_t read_start, read_size;
    cahute_u8 *read_buffer;
};

/* Absolute minimum buffer size for CASIOLINK. */
#define CASIOLINK_MINIMUM_BUFFER_SIZE 50

/* Raw device information size for CASIOLINK, most specifically the
 * CAS100 variant. */
#define CASIOLINK_RAW_DEVICE_INFO_BUFFER_SIZE 33

/* Flags to describe whether device information was obtained or not. */
#define CASIOLINK_FLAG_DEVICE_INFO_OBTAINED 0x00000001UL

/* Maximum size of raw data that can come from an extended packet.
 * Calculators support data packets with up to 256 raw bytes (512 encoded
 * bytes), but fxRemote uses payloads that go up to 1028 raw bytes
 * (2056 encoded bytes). */
#define SEVEN_MAX_PACKET_DATA_SIZE         1028
#define SEVEN_MAX_ENCODED_PACKET_DATA_SIZE 2056 /* Max data size x 2. */
#define SEVEN_MAX_PACKET_SIZE              2066 /* Enc. data size + 10. */

/* Size of the raw device information buffer for Protocol 7.00.
 * This actually varies between devices: the fx-9860G use 164 bytes,
 * the fx-CG use 188 bytes. */
#define SEVEN_RAW_DEVICE_INFO_BUFFER_SIZE 200

/* Flag to describe whether device information has been requested. */
#define SEVEN_FLAG_DEVICE_INFO_REQUESTED 0x00000001UL

/**
 * CASIOLINK peer state.
 *
 * @property flags Flags for the CASIOLINK peer state.
 * @property variant Variant with which to force data frame interpretation.
 * @property last_variant Variant for the last data frame.
 * @property raw_device_info Raw device information buffer, so that data
 *           can be extracted later if actual device information is requested.
 */
struct cahute_casiolink_state {
    unsigned long flags;

    int variant;
    int last_variant;

    cahute_u8 raw_device_info[CASIOLINK_RAW_DEVICE_INFO_BUFFER_SIZE];
};

/**
 * Protocol 7.00 peer state.
 *
 * @property flags Flags for the Protocol 7.00 peer state.
 * @property last_command Code of the last executed command. Protocol 7.00
 *           requires the code of the corresponding command to be placed as
 *           the subtype of subsequent data packets.
 * @property last_packet_type Type of the last received packet, or -1 if not
 *           available.
 * @property last_packet_subtype Subtype of the last received packet, or -1
 *           if not available.
 * @property last_packet_data Buffer to the last packet data.
 * @property last_packet_data_size Size of the last packet data.
 * @property raw_device_info Raw device information buffer, so that data can
 *           be extracted later if actual device information is requested.
 * @property raw_device_info_size Raw device information size (not capacity).
 */
struct cahute_seven_state {
    unsigned long flags;

    int last_command;

    int last_packet_type;
    int last_packet_subtype;

    size_t last_packet_data_size;
    size_t raw_device_info_size;

    cahute_u8 last_packet_data[SEVEN_MAX_PACKET_DATA_SIZE];
    cahute_u8 raw_device_info[SEVEN_RAW_DEVICE_INFO_BUFFER_SIZE];
};

/**
 * Protocol 7.00 screenstreaming receiver state.
 *
 * If reception of a frame packet has been successful, the data buffer
 * will contain the frame data.
 *
 * @property last_packet_type Type of the last received packet, or -1 if not
 *           available.
 * @property last_packet_subtype Subtype of the last received packet, if
 *           relevant.
 * @property picture_format Type of the last received picture, as a
 *           ``CAHUTE_PICTURE_FORMAT_*`` constant.
 * @property picture_width Width of the last picture in pixels, -1
 *           if not relevant.
 * @property picture_height Height of the last picture in pixels, -1
 *           if not relevant.
 */
struct cahute_seven_ohp_state {
    int last_packet_type;
    int picture_format;
    int picture_width;
    int picture_height;

    cahute_u8 last_packet_subtype[5];
};

/**
 * Link protocol client state, to be used depending on the protocol selected
 * in the link flags.
 *
 * @property casiolink CASIOLINK peer state.
 * @property seven Protocol 7.00 peer state.
 * @property seven_ohp Protocol 7.00 screenstreaming receiver state.
 */
union cahute_link_protocol_state {
    struct cahute_casiolink_state casiolink;
    struct cahute_seven_state seven;
    struct cahute_seven_ohp_state seven_ohp;
};

/**
 * Internal link representation.
 *
 * @property flags Link flags, as OR'd ``CAHUTE_LINK_FLAG_*`` constants.
 * @property protocol Protocol type, as any ``CAHUTE_LINK_PROTOCOL_*`` constant
 *           representing the protocol state to use.
 * @property medium Medium-related information.
 * @property protocol_state State of the specific protocol to use, e.g.
 *           current role in the protocol and details regarding the last
 *           received packet.
 *           The protocol data buffer is not included within this property.
 * @property cached_device_info Device information, if it has been requested
 *           at least once, so it can be free'd when the link is closed.
 * @property data_buffer General-purpose buffer for the protocol
 *           implementation to use. This can contain payloads, frame data,
 *           etc.
 * @property data_buffer_size Size of the data currently present within
 *           the data buffer, in bytes.
 * @property data_buffer_capacity Total amount of data the data buffer
 *           can contain, in bytes.
 */
struct cahute_link {
    unsigned long flags;
    int protocol;

    cahute_link_medium medium;
    union cahute_link_protocol_state protocol_state;

    cahute_device_info *cached_device_info;

    /* Raw data buffer, used by the protocol implementation to store raw data.
     * This can be of varying length depending on the protocol in use.
     * The buffer is allocated in the same block as the link. */
    cahute_u8 *data_buffer;
    size_t data_buffer_size, data_buffer_capacity;

    /* Stored frame, so that screen reception does not use dynamic
     * memory allocation for every frame. */
    cahute_frame stored_frame;
};

/* ---
 * File internals.
 * --- */

#define CAHUTE_FILE_MEDIUM_READ_BUFFER_SIZE 4096U

#define CAHUTE_MAX_FILE_OFFSET 2147483647

#define CAHUTE_FILE_FLAG_CLOSE_MEDIUM 1 /* Whether to close the medium. */
#define CAHUTE_FILE_FLAG_EXAMINED     2 /* Whether file type was examined. */

#define CAHUTE_FILE_MEDIUM_FLAG_WRITE 0x00000001 /* Can write to medium. */
#define CAHUTE_FILE_MEDIUM_FLAG_READ  0x00000002 /* Can read from medium. */
#define CAHUTE_FILE_MEDIUM_FLAG_SEEK  0x00000004 /* Can seek on medium. */
#define CAHUTE_FILE_MEDIUM_FLAG_SIZE  0x00000008 /* File size is avail. */

/* Special medium which does not implement a read, write or seek method.
 * It actually directly uses the read buffer, and keeps everything in
 * memory using it. */
#define CAHUTE_FILE_MEDIUM_NONE 0

#if POSIX_ENABLED
# define CAHUTE_FILE_MEDIUM_POSIX 1
#endif

#if WIN32_ENABLED
# define CAHUTE_FILE_MEDIUM_WIN32 2
#endif

#if defined(CAHUTE_FILE_MEDIUM_POSIX)
/**
 * POSIX file medium state.
 *
 * @property fd File descriptor on the opened file.
 */
struct cahute_file_posix_medium_state {
    int fd;
};
#endif

#if defined(CAHUTE_FILE_MEDIUM_WIN32)
/**
 * Windows API medium state.
 *
 * @property handle File handle.
 */
struct cahute_file_windows_medium_state {
    HANDLE handle;
};
#endif

/**
 * File medium state, to be used depending on the file flags.
 *
 * @property posix Medium state if the selected medium type is POSIX.
 * @property windows Medium state if the selected medium type is WIN32.
 */
union cahute_file_medium_state {
#if defined(CAHUTE_FILE_MEDIUM_POSIX)
    struct cahute_file_posix_medium_state posix;
#endif
#if defined(CAHUTE_FILE_MEDIUM_WIN32)
    struct cahute_file_windows_medium_state windows;
#endif
};

/**
 * File medium related information.
 *
 * @property type Medium type, as any ``CAHUTE_FILE_MEDIUM_*`` constant.
 * @property write Whether the medium is writable or not.
 * @property flags Medium flags.
 * @property offset Current offset on the underlying medium.
 * @property state State of the specific medium to use, e.g. opened handles
 *           and contexts to close at file closing.
 * @property read_offset Current offset for the read buffer.
 * @property read_size Number of unread bytes in the read buffer for the
 *           medium, starting at the offset stored in ``read_start``.
 * @property read_buffer Buffer for reading from the medium in a stream-like
 *           interface. See ``cahute_read_from_file_medium`` definition
 *           for more information.
 *           Guaranteed to be aligned to a multiple of 32 bytes.
 * @property file_size File size computed when the file was opened.
 */
struct cahute_file_medium {
    int type;

    unsigned long flags;
    unsigned long offset;
    unsigned long read_offset;
    unsigned long file_size;
    size_t read_size;

    cahute_u8 *read_buffer;
    union cahute_file_medium_state state;
};

/**
 * File related information.
 *
 * @property flags Flags.
 * @property medium Medium.
 * @property type Found file type.
 *           If flag CAHUTE_FILE_FLAG_EXAMINED is present and this is
 *           set to 0, this means that the file has been examined but no
 *           known type was found.
 * @property extension Extension normalized in ASCII lowercase, for later use
 *           in guessing, if found in the file name.
 */
struct cahute_file {
    unsigned long flags;
    cahute_file_medium medium;
    int type;
    char extension[5];
};

/* Internal function to declare a file for a memory buffer, without having
 * to use dynamic memory. */
CAHUTE_EXTERN(void)
cahute_populate_file_from_memory(
    cahute_file *file,
    cahute_u8 *buf,
    size_t size
);

/* ---
 * Miscellaneous functions, defined in misc.c
 * --- */

CAHUTE_EXTERN(int) cahute_sleep(unsigned long ms);
CAHUTE_EXTERN(int) cahute_monotonic(unsigned long *msp);

/* ---
 * Link medium functions, defined in linkmedium.c
 * --- */

CAHUTE_EXTERN(int)
cahute_receive_on_link_medium(
    cahute_link_medium *medium,
    cahute_u8 *buf,
    size_t size,
    unsigned long first_timeout,
    unsigned long next_timeout
);

CAHUTE_EXTERN(int)
cahute_send_on_link_medium(
    cahute_link_medium *medium,
    cahute_u8 const *buf,
    size_t size
);

CAHUTE_EXTERN(int)
cahute_set_serial_params_to_link_medium(
    cahute_link_medium *medium,
    unsigned long flags,
    unsigned long speed
);

CAHUTE_EXTERN(int)
cahute_scsi_request_to_link_medium(
    cahute_link_medium *medium,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 const *data,
    size_t data_size,
    int *statusp
);

CAHUTE_EXTERN(int)
cahute_scsi_request_from_link_medium(
    cahute_link_medium *medium,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int *statusp
);

/* ---
 * File medium functions, defined in filemedium.c
 * --- */

CAHUTE_EXTERN(int)
cahute_read_from_file_medium(
    cahute_file_medium *medium,
    unsigned long off,
    cahute_u8 *buf,
    size_t size
);

CAHUTE_EXTERN(int)
cahute_write_to_file_medium(
    cahute_file_medium *medium,
    unsigned long offset,
    void const *data,
    size_t size
);

/* ---
 * Data management, defined in data.c
 * --- */

CAHUTE_EXTERN(int)
cahute_create_program_from_file(
    cahute_data **datap,
    int encoding,
    void const *name,
    size_t name_size,
    void const *password,
    size_t password_size,
    cahute_file *file,
    unsigned long content_offset,
    size_t content_size
);

/* ---
 * CASIOLINK header and file format management, defined in casiolink.c
 * --- */

#define CAHUTE_CASIOLINK_DATA_FLAG_END    0x00000001 /* Ends communication. */
#define CAHUTE_CASIOLINK_DATA_FLAG_FINAL  0x00000002 /* Final. */
#define CAHUTE_CASIOLINK_DATA_FLAG_AL     0x00000004 /* Starts AL mode. */
#define CAHUTE_CASIOLINK_DATA_FLAG_AL_END 0x00000008 /* Ends AL mode. */
#define CAHUTE_CASIOLINK_DATA_FLAG_NO_LOG 0x00000010 /* Do not log part. */
#define CAHUTE_CASIOLINK_DATA_FLAG_MDL    0x00000020 /* Is CAS100 MDL data. */

/**
 * Data description to be determined from a header.
 *
 * This allows, in the CASIOLINK protocol implementation, to separate reading
 * and acknowledging over the link from the file decoding part.
 * It can be determined from a header and variant using the
 * ``cahute_casiolink_determine_data_description()`` function.
 *
 * A few examples of such structure are the following:
 *
 * ``{part_count=0}``
 *     No data part accompanying the header.
 *
 * ``{part_count=1, last_part_repeat=1, part_sizes=[55]}
 *     One data part of size 55 bytes accompanying the header.
 *
 * ``{part_count=2, last_part_repeat=1, part_sizes=[56, 57]}``
 *     Two data parts of respective sizes 56 and 57 bytes accompanying the
 *     header.
 *
 * ``{part_count=2, last_part_repeat=3, part_sizes=[32, 16]}``
 *     Four data parts, of respective sizes 32, 16, 16 and 16 bytes
 *     accompanying the header.
 *
 * @param flags Data description flags, using ``CAHUTE_CASIOLINK_DATA_FLAG_*``
 *        values.
 * @param packet_type Packet type (first byte of the packet) to be expected
 *        with the data parts.
 * @param part_count Number of part sizes used in the ``part_sizes`` array.
 * @param last_part_repeat How much times the last part is repeated.
 * @param part_sizes Distinct part sizes.
 */
struct cahute_casiolink_data_description {
    unsigned long flags;
    int packet_type;
    size_t part_count;
    size_t last_part_repeat;
    size_t part_sizes[5];
};

CAHUTE_EXTERN(int)
cahute_casiolink_determine_header_variant(cahute_u8 const *data);

CAHUTE_EXTERN(int)
cahute_casiolink_determine_data_description(
    cahute_u8 const *data,
    int variant,
    cahute_casiolink_data_description *desc
);

CAHUTE_EXTERN(int)
cahute_casiolink_decode_data(
    cahute_data **datap,
    cahute_file *file,
    unsigned long *offsetp,
    int variant,
    int check_data
);

/* ---
 * CASIOLINK protocol and file format functions, defined in casiolink.c
 * --- */

CAHUTE_EXTERN(int) cahute_casiolink_initiate(cahute_link *link);

CAHUTE_EXTERN(int) cahute_casiolink_terminate(cahute_link *link);

CAHUTE_EXTERN(int)
cahute_casiolink_receive_data(
    cahute_link *link,
    cahute_data **datap,
    unsigned long timeout
);

CAHUTE_EXTERN(int)
cahute_casiolink_receive_screen(
    cahute_link *link,
    cahute_frame *frame,
    unsigned long timeout
);

CAHUTE_EXTERN(int)
cahute_casiolink_make_device_info(
    cahute_link *link,
    cahute_device_info **infop
);

/* ---
 * Protocol 7.00 functions, defined in seven.c
 * --- */

CAHUTE_EXTERN(int) cahute_seven_initiate(cahute_link *link);

CAHUTE_EXTERN(int) cahute_seven_terminate(cahute_link *link);

CAHUTE_EXTERN(int) cahute_seven_discover(cahute_link *link);

CAHUTE_EXTERN(int)
cahute_seven_receive_data(
    cahute_link *link,
    cahute_data **datap,
    unsigned long timeout
);

CAHUTE_EXTERN(int)
cahute_seven_negotiate_serial_params(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
);

CAHUTE_EXTERN(int)
cahute_seven_make_device_info(cahute_link *link, cahute_device_info **infop);

CAHUTE_EXTERN(int)
cahute_seven_request_storage_capacity(
    cahute_link *link,
    char const *storage,
    unsigned long *capacityp
);

CAHUTE_EXTERN(int)
cahute_seven_send_file_to_storage(
    cahute_link *link,
    unsigned long flags,
    char const *directory,
    char const *name,
    char const *storage,
    cahute_file *file,
    cahute_confirm_overwrite_func *overwrite_func,
    void *overwrite_cookie,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_EXTERN(int)
cahute_seven_request_file_from_storage(
    cahute_link *link,
    char const *directory,
    char const *name,
    char const *storage,
    void const *path,
    int path_type,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_EXTERN(int)
cahute_seven_copy_file_on_storage(
    cahute_link *link,
    char const *source_directory,
    char const *source_name,
    char const *target_directory,
    char const *target_name,
    char const *storage
);

CAHUTE_EXTERN(int)
cahute_seven_delete_file_from_storage(
    cahute_link *link,
    char const *directory,
    char const *name,
    char const *storage
);

CAHUTE_EXTERN(int)
cahute_seven_list_storage_entries(
    cahute_link *link,
    char const *storage,
    cahute_list_storage_entry_func *callback,
    void *cookie
);

CAHUTE_EXTERN(int)
cahute_seven_reset_storage(cahute_link *link, char const *storage);

CAHUTE_EXTERN(int)
cahute_seven_optimize_storage(cahute_link *link, char const *storage);

CAHUTE_EXTERN(int)
cahute_seven_backup_rom(
    cahute_link *link,
    cahute_u8 **romp,
    size_t *sizep,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_EXTERN(int)
cahute_seven_upload_and_run_program(
    cahute_link *link,
    cahute_u8 const *program,
    size_t program_size,
    unsigned long load_address,
    unsigned long start_address,
    cahute_progress_func *progress_func,
    void *progress_cookie
);

CAHUTE_EXTERN(int)
cahute_seven_flash_system_using_fxremote_method(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 const *system,
    size_t system_size
);

/* ---
 * Protocol 7.00 Screenstreaming functions, defined in seven_ohp.c
 * --- */

CAHUTE_EXTERN(int)
cahute_seven_ohp_receive_screen(
    cahute_link *link,
    cahute_frame *frame,
    unsigned long timeout
);

/* ---
 * MCS encoding and decoding functions, defined in mcs.c
 * --- */

CAHUTE_EXTERN(int)
cahute_mcs_decode_data(
    cahute_data **datap,
    cahute_u8 const *group,
    size_t group_size,
    cahute_u8 const *directory,
    size_t directory_size,
    cahute_u8 const *name,
    size_t name_size,
    cahute_file *file,
    unsigned long content_offset,
    size_t content_size,
    int data_type
);

#endif /* INTERNALS_H */
