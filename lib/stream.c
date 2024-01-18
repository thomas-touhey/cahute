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

/**
 * Skip data synchronously from the stream associated with the link.
 *
 * This function is guaranteed to skip exactly N bytes, or return an
 * error.
 *
 * @param link Link in which the stream data is defined.
 * @param size Size of the area to skip.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int) cahute_skip_from_link(cahute_link *link, size_t size) {
    if (size) {
        cahute_u8 buf[CAHUTE_LINK_STREAM_BUFFER_SIZE];
        int err;

        if (size >= CAHUTE_LINK_STREAM_BUFFER_SIZE) {
            do {
                err = cahute_read_from_link(
                    link,
                    buf,
                    CAHUTE_LINK_STREAM_BUFFER_SIZE
                );
                if (err)
                    return err;

                size -= CAHUTE_LINK_STREAM_BUFFER_SIZE;
            } while (size >= CAHUTE_LINK_STREAM_BUFFER_SIZE);
        }

        if (size && (err = cahute_read_from_link(link, buf, size)))
            return err;
    }

    return CAHUTE_OK;
}

/**
 * Read data synchronously from the stream associated with the link.
 *
 * This function is guaranteed to fill the buffer completely, or return
 * an error.
 *
 * @param link Link in which the stream data is defined.
 * @param buf Buffer in which to write the read data.
 * @param size Size to read into the buffer.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_read_from_link(cahute_link *link, cahute_u8 *buf, size_t size) {
    if (!size)
        return CAHUTE_OK;

    /* We first need to empty the link's stream buffer.
     * Note that this may fully satisfy the need presented by the caller. */
    {
        size_t left = link->stream_size - link->stream_start;

        if (size <= left) {
            memcpy(buf, &link->stream_buffer[link->stream_start], size);
            link->stream_start += size;
            return CAHUTE_OK;
        }

        if (left) {
            memcpy(buf, &link->stream_buffer[link->stream_start], left);
            buf += left;
            size -= left;
        }

        /* The link buffer is empty! We need to reset it. */
        link->stream_start = 0;
        link->stream_size = 0;
    }

    /* We need to complete the buffer here, by doing multiple passes on the
     * stream implementation specific read code until the caller's need is
     * fully satisfied.
     *
     * At each pass, we actually want to ensure that we always have
     * ``CAHUTE_LINK_STREAM_BUFFER_SIZE`` bytes available in the target
     * buffer.
     *
     * In order to accomplish this, for each pass, we actually determine
     * whether we want to write in the caller's buffer directly or in
     * the link's stream read buffer. */
    while (size) {
        cahute_u8 *dest;
        size_t target_size;    /* Size to ask for, optimistically. */
        size_t bytes_read = 0; /* Must be filled by the implementation. */
        int is_link_buffer;

        if (size >= CAHUTE_LINK_STREAM_BUFFER_SIZE) {
            is_link_buffer = 0;
            dest = buf;
            target_size = size;
        } else {
            is_link_buffer = 1;
            dest = &link->stream_buffer[0];
            target_size = CAHUTE_LINK_STREAM_BUFFER_SIZE;
        }

        /* The implementation must read data in ``dest``, for up to
         * ``target_size`` (while the caller only requires ``size``).
         * It must set ``bytes_read`` to the actual number of bytes read
         * this pass. */
        if (link->flags & CAHUTE_LINK_FLAG_SCSI) {
            cahute_u8 status_buf[16];
            cahute_u8 payload[16];
            size_t avail;
            int err, attempts;

            /* We use custom command C0 to poll status and get avail. bytes.
             * See :ref:`ums-command-c0` for more information.
             *
             * Note that it may take time for the calculator to "recharge"
             * the buffer, so we want to try several times in a row before
             * declaring there is no data available yet. */
            for (attempts = 3; --attempts;) {
                err = cahute_scsi_request_from_link(
                    link,
                    (cahute_u8 *)"\xC0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                    16,
                    status_buf,
                    16,
                    NULL
                );
                if (err)
                    return err;

                avail = (status_buf[6] << 8) | status_buf[7];
                if (!avail) {
                    if ((err = cahute_sleep(10)))
                        return err;

                    continue;
                }

                if (avail > target_size)
                    avail = target_size;

                break;
            }

            if (!attempts) {
                if ((err = cahute_sleep(200)))
                    return err;

                continue;
            }

            /* We now use custom command C1 to request avail. bytes.
             * See :ref:`ums-command-c1` for more information. */
            memcpy(
                payload,
                (cahute_u8 const *)"\xC1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                16
            );
            payload[6] = (avail >> 8) & 255;
            payload[7] = avail & 255;

            err = cahute_scsi_request_from_link(
                link,
                payload,
                16,
                dest,
                avail,
                NULL
            );
            if (err)
                return err;

            bytes_read = avail;
        } else {
            switch (link->stream) {
            case CAHUTE_LINK_STREAM_STDIO:
                bytes_read = fread(
                    dest,
                    1,
                    target_size,
                    link->stream_state.stdio.filep
                );

                if (!bytes_read)
                    return CAHUTE_ERROR_UNKNOWN;

                break;

#ifdef CAHUTE_LINK_STREAM_UNIX
            case CAHUTE_LINK_STREAM_UNIX: {
                ssize_t ret =
                    read(link->stream_state.posix.fd, dest, target_size);

                if (ret < 0)
                    switch (errno) {
                    case 0:
                        continue;

                    case ENODEV:
                    case EIO:
                        return CAHUTE_ERROR_GONE;

                    default:
                        msg(ll_fatal,
                            "Error was %d: %s",
                            errno,
                            strerror(errno));
                        return CAHUTE_ERROR_UNKNOWN;
                    }

                bytes_read = (size_t)ret;
            }

            break;
#endif

#ifdef CAHUTE_LINK_STREAM_WINDOWS
            case CAHUTE_LINK_STREAM_WINDOWS: {
                DWORD received;
                BOOL ret;

                ret = ReadFile(
                    link->stream_state.windows.handle,
                    dest,
                    target_size,
                    &received,
                    NULL
                );
                if (!ret) {
                    DWORD werr = GetLastError();
                    log_windows_error("ReadFile", werr);
                    return CAHUTE_ERROR_UNKNOWN;
                }

                bytes_read = (size_t)received;
            }

            break;
#endif

#ifdef CAHUTE_LINK_STREAM_LIBUSB
            case CAHUTE_LINK_STREAM_LIBUSB: {
                int libusberr;
                int received;

                libusberr = libusb_bulk_transfer(
                    link->stream_state.libusb.handle,
                    LIBUSB_ENDPOINT_IN | LIBUSB_TRANSFER_TYPE_BULK,
                    dest,
                    target_size,
                    &received,
                    0 /* Unlimited timeout by default. */
                );

                switch (libusberr) {
                case 0:
                    break;

                case LIBUSB_ERROR_PIPE:
                case LIBUSB_ERROR_NO_DEVICE:
                case LIBUSB_ERROR_IO:
                    msg(ll_error, "USB device is no longer available.");
                    return CAHUTE_ERROR_GONE;

                case LIBUSB_ERROR_TIMEOUT:
                    return CAHUTE_ERROR_TIMEOUT;

                default:
                    msg(ll_fatal,
                        "libusb error was %d: %s",
                        libusberr,
                        libusb_strerror(libusberr));
                    return CAHUTE_ERROR_UNKNOWN;
                }

                bytes_read = (size_t)received;
            }

            break;
#endif

            default:
                msg(ll_fatal,
                    "Unimplemented stream type 0x%08lX for read.",
                    link->stream);

                return CAHUTE_ERROR_IMPL;
            }
        }

        if (!bytes_read)
            continue;

        if (bytes_read >= size) {
            if (is_link_buffer) {
                memcpy(buf, &link->stream_buffer[0], size);
                link->stream_start = size;
                link->stream_size = bytes_read;
            }

            break;
        }

        if (is_link_buffer)
            memcpy(buf, &link->stream_buffer[0], bytes_read);

        buf += bytes_read;
        size -= bytes_read;
    }

    return CAHUTE_OK;
}

/**
 * Write data synchronously to the stream associated with the given link.
 *
 * There is no write buffering specific to Cahute: the buffer is directly
 * written to the underlying stream.
 *
 * @param link Link from which to get the stream data.
 * @param buf Buffer to write to the link.
 * @param size Size of the buffer to write.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_write_to_link(cahute_link *link, cahute_u8 const *buf, size_t size) {
    if (!size)
        return CAHUTE_OK;

    do {
        size_t bytes_written = 0;

        /* Implementations here must write the contents of 'buf'
         * (of size 'size'), and place the number of written bytes in
         * the 'bytes_written' variable.
         *
         * This way, if only a partial write was achieved, the
         * implementation-specific write function can be called again. */
        if (link->flags & CAHUTE_LINK_FLAG_SCSI) {
            size_t to_send = size > 0xFFFF ? 0xFFFF : size;
            cahute_u8 payload[16], status_buf[16];
            int err;

            /* We use custom command C0 to poll status and get avail. bytes.
             * See :ref:`ums-command-c0` for more information. */
            err = cahute_scsi_request_from_link(
                link,
                (cahute_u8 *)"\xC0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                16,
                status_buf,
                16,
                NULL
            );
            if (err)
                return err;

            /* We use custom command C2 to send data.
             * See :ref:`ums-command-c2` for more information. */
            memcpy(
                payload,
                (cahute_u8 const *)"\xC2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                16
            );
            payload[6] = (to_send >> 8) & 255;
            payload[7] = to_send & 255;

            err = cahute_scsi_request_to_link(
                link,
                payload,
                16,
                buf,
                to_send,
                NULL
            );
            if (err)
                return err;

            bytes_written = to_send;
        } else {
            switch (link->stream) {
            case CAHUTE_LINK_STREAM_STDIO:
                bytes_written =
                    fwrite(buf, 1, size, link->stream_state.stdio.filep);
                if (!bytes_written)
                    return CAHUTE_ERROR_UNKNOWN;

                break;

#ifdef CAHUTE_LINK_STREAM_UNIX
            case CAHUTE_LINK_STREAM_UNIX: {
                ssize_t ret;

                ret = write(link->stream_state.posix.fd, buf, size);
                if (ret < 0)
                    switch (errno) {
                    case ENODEV:
                        return CAHUTE_ERROR_GONE;

                    default:
                        msg(ll_fatal,
                            "errno was %d: %s",
                            errno,
                            strerror(errno));
                        return CAHUTE_ERROR_UNKNOWN;
                    }

                bytes_written = (size_t)ret;
            } break;
#endif

#ifdef CAHUTE_LINK_STREAM_WINDOWS
            case CAHUTE_LINK_STREAM_WINDOWS: {
                DWORD sent;
                BOOL ret;

                ret = WriteFile(
                    link->stream_state.windows.handle,
                    buf,
                    size,
                    &sent,
                    NULL
                );
                if (!ret) {
                    DWORD werr = GetLastError();
                    log_windows_error("WriteFile", werr);
                    return CAHUTE_ERROR_UNKNOWN;
                }

                bytes_written = (size_t)sent;
            }

            break;
#endif

#ifdef CAHUTE_LINK_STREAM_LIBUSB
            case CAHUTE_LINK_STREAM_LIBUSB: {
                int libusberr;
                int sent;

                libusberr = libusb_bulk_transfer(
                    link->stream_state.libusb.handle,
                    LIBUSB_ENDPOINT_OUT | LIBUSB_TRANSFER_TYPE_ISOCHRONOUS,
                    (cahute_u8 *)buf,
                    size,
                    &sent,
                    0 /* Unlimited timeout. */
                );

                switch (libusberr) {
                case 0:
                    break;

                case LIBUSB_ERROR_PIPE:
                case LIBUSB_ERROR_NO_DEVICE:
                case LIBUSB_ERROR_IO:
                    msg(ll_error, "USB device is no longer available.");
                    return CAHUTE_ERROR_GONE;

                default:
                    msg(ll_fatal,
                        "libusb error was %d: %s",
                        libusberr,
                        libusb_strerror(libusberr));
                    return CAHUTE_ERROR_UNKNOWN;
                }

                bytes_written = (size_t)sent;
            }

            break;
#endif

            default:
                msg(ll_fatal,
                    "Unimplemented stream type 0x%08lX for write.",
                    link->stream);

                return CAHUTE_ERROR_IMPL;
            }
        }

        if (bytes_written >= size)
            break;

        buf += bytes_written;
        size -= bytes_written;
    } while (size);

    return CAHUTE_OK;
}

/**
 * Set serial parameters.
 *
 * @param link Link for which to set serial parameters.
 * @param flags Serial parameters to set.
 * @param speed Speed to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_set_serial_params_to_link(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
) {
    if (~link->flags & CAHUTE_LINK_FLAG_SERIAL) {
        msg(ll_error, "Cannot set serial parameters on a non-serial link.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    switch (link->stream) {
#ifdef CAHUTE_LINK_STREAM_UNIX
    case CAHUTE_LINK_STREAM_UNIX: {
        struct termios term;
        speed_t termios_speed;
        unsigned int status, original_status;

        switch (speed) {
        case 300:
            termios_speed = B300;
            break;
        case 600:
            termios_speed = B600;
            break;
        case 1200:
            termios_speed = B1200;
            break;
        case 2400:
            termios_speed = B2400;
            break;
        case 4800:
            termios_speed = B4800;
            break;
        case 9600:
            termios_speed = B9600;
            break;
        case 19200:
            termios_speed = B19200;
            break;
        case 38400:
            termios_speed = B38400;
            break;
        case 57600:
            termios_speed = B57600;
            break;
        case 115200:
            termios_speed = B115200;
            break;
        default:
            msg(ll_error, "Speed unsupported by termios: %lu", speed);
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (tcgetattr(link->stream_state.posix.fd, &term) < 0) {
            msg(ll_error,
                "Could not get serial attributes: %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }

        cfsetispeed(&term, termios_speed);
        cfsetospeed(&term, termios_speed);

        /* Most of the flags' usage here is taken from previous work on
         * libcasio. */
        term.c_iflag &=
            ~(IGNBRK | IGNCR | BRKINT | PARMRK | ISTRIP | INLCR | ICRNL
              | IGNPAR | IXON | IXOFF);
        if ((flags & CAHUTE_SERIAL_XONXOFF_MASK)
            == CAHUTE_SERIAL_XONXOFF_ENABLE) {
            term.c_iflag |= IXON | IXOFF;
            term.c_cc[VSTART] = 0x11; /* XON */
            term.c_cc[VSTOP] = 0x13;  /* XOFF */
            term.c_cc[VMIN] = 0;
        }

        term.c_oflag = 0;
        term.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

        term.c_cflag &= ~(PARENB | PARODD | CSTOPB | CSIZE);
        term.c_cflag |= CREAD | CS8;

        switch (flags & CAHUTE_SERIAL_PARITY_MASK) {
        case CAHUTE_SERIAL_PARITY_EVEN:
            term.c_cflag |= PARENB;
            break;

        case CAHUTE_SERIAL_PARITY_ODD:
            term.c_cflag |= PARENB | PARODD;
            break;
        }

        if ((flags & CAHUTE_SERIAL_STOP_MASK) == CAHUTE_SERIAL_STOP_TWO)
            term.c_cflag |= CSTOPB;

        if (tcsetattr(link->stream_state.posix.fd, TCSANOW, &term)) {
            msg(ll_error,
                "Could not get serial attributes: %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }

        /* Also set the DTR/RTS mode. */
        if (ioctl(link->stream_state.posix.fd, TIOCMGET, &status) >= 0)
            status = 0;

        original_status = status;
        switch (flags & CAHUTE_SERIAL_DTRRTS_MASK) {
        case CAHUTE_SERIAL_DTRRTS_ENABLE:
        case CAHUTE_SERIAL_DTRRTS_HANDSHAKE:
            status |= TIOCM_DTR | TIOCM_RTS;
            break;

        default:
            status &= ~(TIOCM_DTR | TIOCM_RTS);
            break;
        }

        if (status != original_status
            && ioctl(link->stream_state.posix.fd, TIOCMSET, &status) < 0) {
            msg(ll_error, "Could not set DTR/RTS mode.");
            return CAHUTE_ERROR_UNKNOWN;
        }
    } break;
#endif

#ifdef CAHUTE_LINK_STREAM_WINDOWS
    case CAHUTE_LINK_STREAM_WINDOWS: {
        DCB dcb;
        DWORD dcb_speed;

        switch (speed) {
        case 300:
            dcb_speed = CBR_300;
            break;
        case 600:
            dcb_speed = CBR_600;
            break;
        case 1200:
            dcb_speed = CBR_1200;
            break;
        case 2400:
            dcb_speed = CBR_2400;
            break;
        case 4800:
            dcb_speed = CBR_4800;
            break;
        case 9600:
            dcb_speed = CBR_9600;
            break;
        case 19200:
            dcb_speed = CBR_19200;
            break;
        case 38400:
            dcb_speed = CBR_38400;
            break;
        case 57600:
            dcb_speed = CBR_57600;
            break;
        case 115200:
            dcb_speed = CBR_115200;
            break;

        default:
            msg(ll_error, "Speed unsupported by Windows API: %lu", speed);
            return CAHUTE_ERROR_UNKNOWN;
        }

        SecureZeroMemory(&dcb, sizeof(DCB));
        dcb.DCBlength = sizeof(DCB);
        if (!GetCommState(link->stream_state.windows.handle, &dcb)) {
            log_windows_error("GetCommState", GetLastError());
            return CAHUTE_ERROR_UNKNOWN;
        }

        dcb.BaudRate = dcb_speed;
        dcb.ByteSize = 8;

        switch (flags & CAHUTE_SERIAL_PARITY_MASK) {
        case CAHUTE_SERIAL_PARITY_EVEN:
            dcb.fParity = 1;
            dcb.Parity = EVENPARITY;
            break;

        case CAHUTE_SERIAL_PARITY_ODD:
            dcb.fParity = 1;
            dcb.Parity = ODDPARITY;
            break;

        default:
            dcb.fParity = 0;
            break;
        }

        switch (flags & CAHUTE_SERIAL_STOP_MASK) {
        case CAHUTE_SERIAL_STOP_ONE:
            dcb.StopBits = ONESTOPBIT;
            break;

        case CAHUTE_SERIAL_STOP_TWO:
            dcb.StopBits = TWOSTOPBITS;
            break;
        }

        switch (flags & CAHUTE_SERIAL_XONXOFF_MASK) {
        case CAHUTE_SERIAL_XONXOFF_ENABLE:
            dcb.fInX = 1;
            dcb.fOutX = 1;
            dcb.XonChar = 0x13;
            dcb.XoffChar = 0x11;
            dcb.XonLim = 0;
            dcb.XoffLim = 0;
            break;

        default:
            dcb.fInX = 0;
            dcb.fOutX = 0;
        }

        switch (flags & CAHUTE_SERIAL_DTRRTS_MASK) {
        case CAHUTE_SERIAL_DTRRTS_ENABLE:
            dcb.fDtrControl = DTR_CONTROL_ENABLE;
            dcb.fRtsControl = RTS_CONTROL_ENABLE;
            break;

        case CAHUTE_SERIAL_DTRRTS_HANDSHAKE:
            dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
            dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
            break;

        default:
            dcb.fDtrControl = DTR_CONTROL_DISABLE;
            dcb.fRtsControl = RTS_CONTROL_DISABLE;
            break;
        }

        if (!SetCommState(link->stream_state.windows.handle, &dcb)) {
            log_windows_error("SetCommState", GetLastError());
            return CAHUTE_ERROR_UNKNOWN;
        }
    } break;
#endif

    default:
        msg(ll_error,
            "Setting serial parameters is not implemented for stream type %d.",
            link->stream);
        return CAHUTE_ERROR_IMPL;
    }

    return CAHUTE_OK;
}

/**
 * Emit an SCSI request to a link, while sending or receiving data.
 *
 * This is the internal function behind :c:func:`cahute_scsi_request_to_link`
 * and :c:func:`cahute_scsi_request_from_link`, however it must not be used
 * directly by any other function.
 *
 * @param link Link to which to emit the SCSI request.
 * @param command Command to emit to the link, of 6, 10, 12 or 16 bytes.
 * @param command_size Size of the command to emit to the link.
 * @param buf Optional data buffer to either send or receive.
 * @param buf_size Size of the data or capacity of the data buffer.
 * @param is_send Whether the data should be sent or received.
 * @param statusp Pointer to the SCSI status to set to the received one.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_scsi_request(
    cahute_link *link,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int is_send,
    int *statusp
) {
    int status = 0;

    if (!is_send && !buf_size) {
        msg(ll_error, "buf_size must be > 0 for reception!");
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* The stream-specific implementation must store the status in the
     * ``status`` variable (NOT ``*statusp``). */
    switch (link->stream) {
#ifdef CAHUTE_LINK_STREAM_LIBUSB
    case CAHUTE_LINK_STREAM_LIBUSB: {
        cahute_u8 cbw_buf[32];
        int libusberr, sent = 0;

        /* Produce a CBW as described in Bulk-Only Transport,
         * and send it. */
        memset(cbw_buf, 0, 31);
        memcpy(cbw_buf, "USBCABCD", 8);
        memcpy(&cbw_buf[15], command, command_size);
        cbw_buf[8] = buf_size & 0xFF;
        cbw_buf[9] = (buf_size >> 8) & 0xFF;
        cbw_buf[10] = (buf_size >> 16) & 0xFF;
        cbw_buf[11] = (buf_size >> 24) & 0xFF;
        cbw_buf[14] = command_size;

        if (!is_send)
            cbw_buf[12] |= 128;

        libusberr = libusb_bulk_transfer(
            link->stream_state.libusb.handle,
            LIBUSB_ENDPOINT_OUT | LIBUSB_TRANSFER_TYPE_ISOCHRONOUS,
            (cahute_u8 *)cbw_buf,
            31,
            &sent,
            0 /* Unlimited timeout. */
        );

        switch (libusberr) {
        case 0:
            break;

        case LIBUSB_ERROR_PIPE:
        case LIBUSB_ERROR_NO_DEVICE:
        case LIBUSB_ERROR_IO:
            msg(ll_error, "USB device is no longer available.");
            return CAHUTE_ERROR_GONE;

        default:
            msg(ll_fatal,
                "libusb error was %d: %s",
                libusberr,
                libusb_strerror(libusberr));
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

        if (!buf_size) {
            /* Nothing to be sent, nothing to be received. */
        } else if (is_send) {
            int libusberr, sent = 0;

            libusberr = libusb_bulk_transfer(
                link->stream_state.libusb.handle,
                LIBUSB_ENDPOINT_OUT | LIBUSB_TRANSFER_TYPE_ISOCHRONOUS,
                buf,
                (int)buf_size,
                &sent,
                0 /* Unlimited timeout. */
            );

            switch (libusberr) {
            case 0:
                break;

            case LIBUSB_ERROR_PIPE:
            case LIBUSB_ERROR_NO_DEVICE:
            case LIBUSB_ERROR_IO:
                msg(ll_error, "USB device is no longer available.");
                return CAHUTE_ERROR_GONE;

            default:
                msg(ll_fatal,
                    "libusb error was %d: %s",
                    libusberr,
                    libusb_strerror(libusberr));
                return CAHUTE_ERROR_UNKNOWN;
            }
        } else {
            do {
                int libusberr, recv = 0;

                libusberr = libusb_bulk_transfer(
                    link->stream_state.libusb.handle,
                    LIBUSB_ENDPOINT_IN | LIBUSB_TRANSFER_TYPE_BULK,
                    buf,
                    (int)buf_size,
                    &recv,
                    0 /* Unlimited timeout. */
                );
                switch (libusberr) {
                case 0:
                    break;

                case LIBUSB_ERROR_PIPE:
                case LIBUSB_ERROR_NO_DEVICE:
                case LIBUSB_ERROR_IO:
                    msg(ll_error, "USB device is no longer available.");
                    return CAHUTE_ERROR_GONE;

                default:
                    msg(ll_fatal,
                        "libusb error was %d: %s",
                        libusberr,
                        libusb_strerror(libusberr));
                    return CAHUTE_ERROR_UNKNOWN;
                }

                if (!recv)
                    continue;

                buf += recv;
                buf_size -= recv;
            } while (buf_size);
        }

        /* Receive the CSW (status). */
        {
            cahute_u8 csw_buf[13], *csw = csw_buf;
            size_t csw_size = 13;
            int libusberr, recv = 0;

            do {
                libusberr = libusb_bulk_transfer(
                    link->stream_state.libusb.handle,
                    LIBUSB_ENDPOINT_IN | LIBUSB_TRANSFER_TYPE_BULK,
                    csw,
                    (int)csw_size,
                    &recv,
                    0 /* Unlimited timeout. */
                );
                switch (libusberr) {
                case 0:
                    break;

                case LIBUSB_ERROR_PIPE:
                case LIBUSB_ERROR_NO_DEVICE:
                case LIBUSB_ERROR_IO:
                    msg(ll_error, "USB device is no longer available.");
                    return CAHUTE_ERROR_GONE;

                default:
                    msg(ll_fatal,
                        "libusb error was %d: %s",
                        libusberr,
                        libusb_strerror(libusberr));
                    return CAHUTE_ERROR_UNKNOWN;
                }

                csw += recv;
                csw_size -= recv;
            } while (csw_size);

            if (memcmp(csw_buf, "USBSABCD", 8)) {
                msg(ll_error, "Unknown or unrecognized UMS CSW:");
                mem(ll_error, csw_buf, 13);
                return CAHUTE_ERROR_CORRUPT;
            }

            status = csw_buf[12];
        }
        break;
#endif

    default:
        msg(ll_fatal,
            "Unimplemented stream type 0x%08lX for SCSI requests.",
            link->stream);

        return CAHUTE_ERROR_IMPL;
    }

    if (statusp)
        *statusp = status;
    return CAHUTE_OK;
}

/**
 * Emit an SCSI request to a link, with or without data.
 *
 * Note that ``*statusp`` may be NULL!
 *
 * @param link Link to which to emit the SCSI request.
 * @param command Command to emit to the link, of 6, 10, 12 or 16 bytes.
 * @param command_size Size of the command to emit to the link.
 * @param data Optional data to append to the command.
 * @param data_size Size of the optional data to append to the command.
 * @param statusp Pointer to the SCSI status to set to the received one.
 */
CAHUTE_EXTERN(int)
cahute_scsi_request_to_link(
    cahute_link *link,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 const *data,
    size_t data_size,
    int *statusp
) {
    return cahute_scsi_request(
        link,
        command,
        command_size,
        (cahute_u8 *)data, /* Explicit removal of const. */
        data_size,
        1,
        statusp
    );
}

/**
 * Emit an SCSI request to a link, and receive data.
 *
 * NOTE: ``*statusp`` may be NULL.
 *
 * @param link Link to which to emit the SCSI request.
 * @param command Command to emit to the link, of 6, 10, 12 or 16 bytes.
 * @param command_size Size of the command to emit to the link.
 * @param buf Buffer to fill with the command's result.
 * @param buf_size Buffer capacity to not go past.
 * @param statusp Pointer to the SCSI status to set to the received one.
 */
CAHUTE_EXTERN(int)
cahute_scsi_request_from_link(
    cahute_link *link,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int *statusp
) {
    return cahute_scsi_request(
        link,
        command,
        command_size,
        buf,
        buf_size,
        0,
        statusp
    );
}
