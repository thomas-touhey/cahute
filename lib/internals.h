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
#include <cahute.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64) \
    || defined(__WINDOWS__)
# define WINDOWS_ENABLED 1
#else
# define WINDOWS_ENABLED 0
#endif

#if WINDOWS_ENABLED
# define UNIX_ENABLED 0
#elif defined(__unix__) && __unix__
# define UNIX_ENABLED 1
#else
# define UNIX_ENABLED 0
#endif

#if UNIX_ENABLED
# include <fcntl.h>
# include <sys/ioctl.h>
# include <sys/stat.h>
# include <termios.h>
# include <unistd.h>
#endif

#include <libusb.h>
#define LIBUSB_ENABLED 1

CAHUTE_EXTERN(int) cahute_sleep(unsigned long ms);

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

#if defined(__cplusplus) \
    ? CAHUTE_GNUC_PREREQ(2, 6) \
    : !defined(__STRICT_ANSI__) && CAHUTE_GNUC_PREREQ(2, 4)
# define CAHUTE_LOGFUNC __PRETTY_FUNCTION__
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
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

#if WINDOWS_ENABLED
/**
 * Log a Windows API error.
 *
 * This is implemented as a separate function to the rest, because gathering
 * an error message for a given error code is quite lengthy.
 *
 * @param func_name Name of the Windows API function that returned the
 *        error.
 * @param code Windows API error code that was actually returned.
 */
CAHUTE_INLINE(void) log_windows_error(char const *func_name, DWORD code) {
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
        msg(ll_error, "Error 0x%08lX occurred in %s.", code, func_name);
        return;
    }

    buf[buf_size] = '\0';
    msg(ll_error, "Error 0x%08lX occurred in %s: %s", code, func_name, buf);
}
#endif

/* ---
 * Link internals.
 * --- */

#define CAHUTE_LINK_STREAM_BUFFER_SIZE 2048

/* Flags that can be present on a link at runtime. */
#define CAHUTE_LINK_FLAG_CLOSE_STREAM   0x00000001
#define CAHUTE_LINK_FLAG_CLOSE_PROTOCOL 0x00000002
#define CAHUTE_LINK_FLAG_TERMINATE      0x00000004 /* Should terminate. */
#define CAHUTE_LINK_FLAG_TERMINATED     0x00000008 /* Was terminated! */
#define CAHUTE_LINK_FLAG_SCSI           0x00000010 /* Using SCSI. */
#define CAHUTE_LINK_FLAG_SERIAL         0x00000020 /* Using serial. */
#define CAHUTE_LINK_FLAG_IRRECOVERABLE  0x00000040 /* Cannot recover. */

/* Stream types allowed. */
#define CAHUTE_LINK_STREAM_STDIO 0x00000001
#if UNIX_ENABLED
# define CAHUTE_LINK_STREAM_UNIX 0x00000002
#endif
#if WINDOWS_ENABLED
# define CAHUTE_LINK_STREAM_WINDOWS 0x00000003
#endif
#if LIBUSB_ENABLED
# define CAHUTE_LINK_STREAM_LIBUSB 0x00000004
#endif

/* Protocol selection for 'initialize_link_protocol()'. */
#define CAHUTE_LINK_PROTOCOL_CAS40     0x00000001
#define CAHUTE_LINK_PROTOCOL_CAS50     0x00000002
#define CAHUTE_LINK_PROTOCOL_CAS100    0x00000003
#define CAHUTE_LINK_PROTOCOL_SEVEN     0x00000004
#define CAHUTE_LINK_PROTOCOL_SEVEN_OHP 0x00000005

/**
 * stdio stream state.
 *
 * @property filep File pointer.
 */
struct cahute_link_stdio {
    FILE *filep;
};

#if UNIX_ENABLED
/**
 * POSIX unistd stream state.
 *
 * @property fd File descriptor on the opened stream.
 */
struct cahute_link_posix {
    int fd;
};
#endif

#if WINDOWS_ENABLED
/**
 * Windows API stream state.
 *
 * @property handle File handle.
 */
struct cahute_link_windows {
    HANDLE handle;
};
#endif

#if LIBUSB_ENABLED
/**
 * libusb device stream state.
 *
 * @property context libusb context to close once the link is closed.
 * @property handle libusb device handle which to use to make USB requests.
 */
struct cahute_link_libusb {
    libusb_context *context;
    libusb_device_handle *handle;
};
#endif

/**
 * Stream state, to be used depending on the link flags regarding the stream.
 *
 * @property stdio Stream state if the selected stream type is stdio.
 * @property posix Stream state if the selected stream type is POSIX.
 * @property windows Stream state if the selected stream type is Windows.
 * @property libusb Stream state if the selected stream type is libusb.
 */
union cahute_link_stream_state {
    struct cahute_link_stdio stdio;
#if UNIX_ENABLED
    struct cahute_link_posix posix;
#endif
#if WINDOWS_ENABLED
    struct cahute_link_windows windows;
#endif
#if LIBUSB_ENABLED
    struct cahute_link_libusb libusb;
#endif
};

/* Absolute minimum buffer size for Protocol 7.00. */
#define SEVEN_MINIMUM_BUFFER_SIZE 1024

/* Size of the raw device information buffer for Protocol 7.00.
 * This actually varies between devices: the fx-9860G use 164 bytes,
 * the fx-CG use 188 bytes. */
#define SEVEN_RAW_DEVICE_INFO_BUFFER_SIZE 200

/* Flag to describe whether device information has been requested. */
#define SEVEN_FLAG_DEVICE_INFO_REQUESTED 0x00000001

/**
 * Protocol 7.00 peer state.
 *
 * The protocol buffer will contain the unpadded raw data accompanying
 * the packet.
 *
 * With usual packets, the packet's content will need to be decoded after
 * the receiving.
 *
 * @property flags Flags for the Protocol 7.00 peer state.
 * @property last_command Code of the last executed command. Protocol 7.00
 *           requires the code of the corresponding command to be placed as
 *           the subtype of subsequent data packets.
 * @property last_packet_type Type of the last received packet, or -1 if not
 *           available.
 * @property last_packet_subtype Subtype of the last received packet, or -1
 *           if not available.
 * @property raw_device_info Raw device information buffer, so that data can
 *           be extracted later if actual device information is requested.
 * @property raw_device_info Raw device information size (not capacity).
 */
struct cahute_seven_state {
    unsigned long flags;

    int last_command;

    int last_packet_type;
    int last_packet_subtype;

    cahute_u8 raw_device_info[SEVEN_RAW_DEVICE_INFO_BUFFER_SIZE];
    size_t raw_device_info_size;
};

/**
 * Protocol 7.00 screenstreaming receiver state.
 *
 * If reception of a frame packet has been successful, the protocol buffer
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
 * @property seven Protocol 7.00 peer state.
 * @property seven_ohp Protocol 7.00 screenstreaming receiver state.
 */
union cahute_link_protocol_state {
    struct cahute_seven_state seven;
    struct cahute_seven_ohp_state seven_ohp;
};

/**
 * Internal link representation.
 *
 * @property flags Link flags, as OR'd ``CAHUTE_LINK_FLAG_*`` constants.
 * @property stream Stream type, as any ``CAHUTE_LINK_STREAM_*`` constant
 *           representing the stream state to use.
 * @property protocol Protocol type, as any ``CAHUTE_LINK_PROTOCOL_*`` constant
 *           representing the protocol state to use.
 * @property serial_flags Current serial flags, as or'd
 *           ``CAHUTE_SERIAL_FLAG_*`` constants.
 * @property serial_speed Current serial speed.
 * @property stream_state State of the specific stream to use, e.g. opened
 *           handles and contexts to close at link closing.
 *           The read buffer is not included within this property.
 * @property protocol_state State of the specific protocol to use, e.g.
 *           current role in the protocol and details regarding the last
 *           received packet.
 *           The protocol data buffer is not included within this property.
 * @property protocol_buffer General-purpose buffer for the protocol
 *           implementation to use. This can contain payloads, frame data,
 *           etc.
 * @property protocol_buffer_size Size of the data currently present within
 *           the protocol buffer, in bytes.
 * @property protocol_buffer_capacity Total amount of data the protocol buffer
 *           can contain, in bytes.
 * @property stream_buffer Read buffer for the stream utilities to use.
 *           See ``cahute_read`` definition for more information.
 * @property stream_start Offset at which the unread data starts within
 *           the stream buffer.
 * @property stream_size Number of unread bytes in the stream buffer, starting
 *           at the offset stored in ``stream_start``.
 * @property cached_device_info Device information, if it has been requested
 *           at least once, so it can be free'd when the stream is closed.
 */
struct cahute_link {
    unsigned long flags;
    int stream, protocol;
    unsigned long serial_flags;
    unsigned long serial_speed;

    union cahute_link_stream_state stream_state;
    union cahute_link_protocol_state protocol_state;

    cahute_device_info *cached_device_info;

    /* Protocol buffer, used by the protocol implementation.
     * This can be of varying length depending on the protocol in use.
     * The buffer is allocated in the same block as the link. */
    cahute_u8 *protocol_buffer;
    size_t protocol_buffer_size, protocol_buffer_capacity;

    /* Read buffer. See ``cahute_read`` definition for more information. */
    size_t stream_start, stream_size;
    cahute_u8 stream_buffer[CAHUTE_LINK_STREAM_BUFFER_SIZE];
};

/* ---
 * Link stream functions, defined in stream.c
 * --- */

CAHUTE_EXTERN(int)
cahute_read_from_link(cahute_link *link, cahute_u8 *buf, size_t size);

CAHUTE_EXTERN(int) cahute_skip_from_link(cahute_link *link, size_t size);

CAHUTE_EXTERN(int)
cahute_write_to_link(cahute_link *link, cahute_u8 const *buf, size_t size);

CAHUTE_EXTERN(int)
cahute_set_serial_params_to_link(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
);

CAHUTE_EXTERN(int)
cahute_scsi_request_to_link(
    cahute_link *link,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 const *data,
    size_t data_size,
    int *statusp
);

CAHUTE_EXTERN(int)
cahute_scsi_request_from_link(
    cahute_link *link,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int *statusp
);

/* ---
 * Protocol 7.00 functions, defined in seven.c
 * --- */

CAHUTE_EXTERN(int) cahute_seven_initiate(cahute_link *link);

CAHUTE_EXTERN(int) cahute_seven_terminate(cahute_link *link);

CAHUTE_EXTERN(int) cahute_seven_discover(cahute_link *link);

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
    FILE *filep,
    size_t file_size,
    cahute_confirm_overwrite_func *overwrite_func,
    void *overwrite_cookie,
    cahute_progress_func *progress_func,
    void *cookie_func
);

CAHUTE_EXTERN(int)
cahute_seven_request_file_from_storage(
    cahute_link *link,
    char const *directory,
    char const *name,
    char const *storage,
    FILE *filep,
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

/* ---
 * Protocol 7.00 Screenstreaming functions, defined in seven_ohp.c
 * --- */

CAHUTE_EXTERN(int)
cahute_seven_ohp_get_screen(
    cahute_link *link,
    cahute_process_frame_func *callback,
    void *cookie
);

#endif /* INTERNALS_H */
