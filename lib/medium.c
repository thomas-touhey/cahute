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
#if WIN32_ENABLED
# include <ntddscsi.h>
#endif

/**
 * Read data synchronously from the medium associated with the link.
 *
 * This function is guaranteed to fill the buffer completely, or return
 * an error.
 *
 * If any timeout is provided as 0, the corresponding timeout will be
 * unlimited, i.e. the function will wait indefinitely.
 *
 * @param medium Link medium from which to read.
 * @param buf Buffer in which to write the read data. Can be NULL if we only
 *        want to skip data from the link medium.
 * @param size Size to read into the buffer.
 * @param first_timeout Timeout before the first byte is received,
 *        in milliseconds.
 * @param next_timeout Timeout in-between any byte past the first one,
 *        in milliseconds.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_read_from_link_medium(
    cahute_link_medium *medium,
    cahute_u8 *buf,
    size_t size,
    unsigned long first_timeout,
    unsigned long next_timeout
) {
    size_t original_size = size; /* For logging. */
    size_t bytes_read;
    unsigned long timeout = first_timeout;
    unsigned long iteration_timeout = first_timeout; /* For logging. */
    unsigned long start_time, first_time = 0, last_time;
    int timeout_error = CAHUTE_ERROR_TIMEOUT_START;
    int err;

    if (!size)
        return CAHUTE_OK;

    /* We first need to empty the medium's read buffer.
     * Note that this may fully satisfy the need presented by the caller. */
    {
        size_t left = medium->read_size - medium->read_start;

        if (size <= left) {
            if (buf)
                memcpy(buf, &medium->read_buffer[medium->read_start], size);

            medium->read_start += size;
            return CAHUTE_OK;
        }

        if (left) {
            if (buf) {
                memcpy(buf, &medium->read_buffer[medium->read_start], left);
                buf += left;
            }

            size -= left;
        }

        /* The medium buffer is empty! We need to reset it. */
        medium->read_start = 0;
        medium->read_size = 0;
    }

    /* Set ``bytes_read`` to 1 so the first round of the loop actually
     * attempts at reading and does not remove time from the time. */
    bytes_read = 1;

    err = cahute_monotonic(&start_time);
    if (err)
        return err;

    last_time = start_time;

    /* We need to complete the buffer here, by doing multiple passes on the
     * medium implementation specific read code until the caller's need is
     * fully satisfied.
     *
     * At each pass, we actually want to ensure that we always have
     * ``CAHUTE_LINK_MEDIUM_READ_BUFFER_SIZE`` bytes available in the target
     * buffer.
     *
     * In order to accomplish this, for each pass, we actually determine
     * whether we want to write in the caller's buffer directly or in
     * the medium's read buffer. */
    while (size) {
        cahute_u8 *dest;
        size_t target_size; /* Size to ask for, optimistically. */

        /* If no bytes have been read since last time, we actually need to
         * remove the difference using the monotonic clock! */
        if (!bytes_read && timeout) {
            unsigned long current_time;

            err = cahute_monotonic(&current_time);
            if (err)
                return err;

            if (current_time - last_time >= timeout)
                goto time_out;

            timeout -= current_time - last_time;
            last_time = current_time;
        }

        bytes_read = 0;

        /* NOTE: Historically here, we used to write directly to the
         * destination buffer if the output was big enough. However, the
         * medium sometimes requires aligned buffers, and the medium read
         * buffer is guaranteed to be aligned at the 8-byte mark. */
        dest = medium->read_buffer;
        target_size = CAHUTE_LINK_MEDIUM_READ_BUFFER_SIZE;

        /* The implementation must read data in ``dest``, for up to
         * ``target_size`` (while the caller only requires ``size``).
         * It must set ``bytes_read`` to the actual number of bytes read
         * this pass. */
        switch (medium->type) {
#ifdef CAHUTE_LINK_MEDIUM_POSIX_SERIAL
        case CAHUTE_LINK_MEDIUM_POSIX_SERIAL: {
            cahute_ssize ret;

            if (timeout > 0) {
                /* Use select() to wait for input to be present. */
                fd_set read_fds, write_fds, except_fds;
                struct timeval timeout_tv;
                int select_ret;

                FD_ZERO(&read_fds);
                FD_ZERO(&write_fds);
                FD_ZERO(&except_fds);
                FD_SET(medium->state.posix.fd, &read_fds);

                timeout_tv.tv_sec = timeout / 1000;
                timeout_tv.tv_usec = (timeout % 1000) * 1000;

                select_ret = select(
                    medium->state.posix.fd + 1,
                    &read_fds,
                    &write_fds,
                    &except_fds,
                    &timeout_tv
                );

                switch (select_ret) {
                case 1:
                    /* Input is ready for us to read! */
                    break;

                case 0:
                    goto time_out;

                default:
                    msg(ll_error,
                        "An error occurred while calling select() %s (%d)",
                        strerror(errno),
                        errno);
                    return CAHUTE_ERROR_UNKNOWN;
                }
            }

            ret = read(medium->state.posix.fd, dest, target_size);

            if (ret < 0)
                switch (errno) {
                case 0:
                    continue;

                case ENODEV:
                case EIO:
                    medium->flags |= CAHUTE_LINK_MEDIUM_FLAG_GONE;
                    return CAHUTE_ERROR_GONE;

                default:
                    msg(ll_error,
                        "An error occurred while calling read() %s (%d)",
                        strerror(errno),
                        errno);
                    return CAHUTE_ERROR_UNKNOWN;
                }

            bytes_read = (size_t)ret;
        }

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
        {
            BOOL ret;

            /* If a read operation is not already in progress, we want to
             * initiate it now. */
            if (!medium->state.windows.read_in_progress) {
                medium->state.windows.received = 0;
                ret = ReadFile(
                    medium->state.windows.handle,
                    dest,
                    target_size,
                    &medium->state.windows.received,
                    &medium->state.windows.overlapped
                );

                if (!ret) {
                    DWORD werr = GetLastError();

                    if (werr == ERROR_IO_PENDING)
                        medium->state.windows.read_in_progress = 1;
                    else {
                        log_windows_error("ReadFile", werr);
                        return CAHUTE_ERROR_UNKNOWN;
                    }
                }
            }

            /* If a read operation is in progress, i.e. either if it has been
             * initiated in a previous read or if it has been initiated before
             * and has not returned immediately, we want to check on it. */
            if (medium->state.windows.read_in_progress) {
                ret = WaitForSingleObject(
                    medium->state.windows.overlapped.hEvent,
                    timeout ? timeout : INFINITE
                );
                switch (ret) {
                case WAIT_OBJECT_0:
                    medium->state.windows.read_in_progress = 0;
                    ret = GetOverlappedResult(
                        medium->state.windows.handle,
                        &medium->state.windows.overlapped,
                        &medium->state.windows.received,
                        FALSE
                    );

                    if (!ret) {
                        DWORD werr = GetLastError();
                        if (werr == ERROR_GEN_FAILURE)
                            return CAHUTE_ERROR_GONE;

                        log_windows_error("GetOverlappedResult", werr);
                        return CAHUTE_ERROR_UNKNOWN;
                    }
                    break;

                case WAIT_TIMEOUT:
                    /* Read will still be in progress for next time we come
                     * back to this function. */
                    goto time_out;

                default:
                    log_windows_error("WaitForSingleObject", GetLastError());

                    return CAHUTE_ERROR_UNKNOWN;
                }
            }

            bytes_read = (size_t)medium->state.windows.received;
        }

        break;
#endif

#if defined(CAHUTE_LINK_MEDIUM_AMIGAOS_SERIAL)
        case CAHUTE_LINK_MEDIUM_AMIGAOS_SERIAL: {
            struct timerequest *timer;
            struct IOExtSer *io = medium->state.amigaos_serial.io;
            struct MsgPort *timer_msgport,
                *serial_msgport = medium->state.amigaos_serial.msg_port;
            cahute_u32 signals = 0;
            int has_serial = 0;

            err = cahute_get_amiga_timer(&timer_msgport, &timer);
            if (err)
                return err;

            /* Run two operations at once:
             * - Read into the buffer.
             * - Start a timer to the currently requested timeout. */
            io->IOSer.io_Command = CMD_READ;
            io->IOSer.io_Length = target_size;
            io->IOSer.io_Data = (APTR)dest;

            SendIO((struct IORequest *)io);
            signals = (1L << serial_msgport->mp_SigBit) | SIGBREAKF_CTRL_C;

            if (timeout > 0) {
                timer->tr_time.tv_secs = timeout / 1000;
                timer->tr_time.tv_micro = timeout % 1000 * 1000;
                timer->tr_node.io_Command = TR_ADDREQUEST;

                SendIO((struct IORequest *)timer);
                signals |= (1L << timer_msgport->mp_SigBit);
            }

            if (CheckIO((struct IORequest *)io)) {
                /* Request has terminated immediately.
                 * Note that we may have started a timer request for nothing
                 * here, but we want to have this CheckIO() call as close as
                 * possible to the Wait() to avoid race conditions as much
                 * as possible. */
                signals = 0;
                has_serial = 1;
            } else {
                /* We want to wait only if the request has not finished
                 * immediately. */
                signals = Wait(signals);
                has_serial = CheckIO((struct IORequest *)io) ? 1 : 0;
            }

            if (timeout > 0) {
                if (!CheckIO((struct IORequest *)timer))
                    AbortIO((struct IORequest *)timer);

                WaitIO((struct IORequest *)timer);
            }

            /* Wait for either completion and clearing of serial read, or
             * for cancellation of I/O request.
             * This is required for refreshing the buffer and 'io_Actual'. */
            if (!has_serial)
                AbortIO((struct IORequest *)io);

            WaitIO((struct IORequest *)io);

            if (signals & SIGBREAKF_CTRL_C)
                return CAHUTE_ERROR_ABORT;
            else if (!has_serial)
                goto time_out;

            if (io->IOSer.io_Error) {
                msg(ll_error,
                    "Error %d occurred while reading from device.",
                    io->IOSer.io_Error);
                return CAHUTE_ERROR_UNKNOWN;
            }

            /* I/O request was completed, we want to read the contents. */
            if (io->IOSer.io_Error) {
                msg(ll_error,
                    "Error %d occurred while reading from device.",
                    io->IOSer.io_Error);
                return CAHUTE_ERROR_UNKNOWN;
            }

            bytes_read = io->IOSer.io_Actual;
        } break;
#endif

#ifdef CAHUTE_LINK_MEDIUM_LIBUSB
        case CAHUTE_LINK_MEDIUM_LIBUSB: {
            int libusberr;
            int received;

            libusberr = libusb_bulk_transfer(
                medium->state.libusb.handle,
                medium->state.libusb.bulk_in,
                dest,
                target_size,
                &received,
                timeout
            );

            switch (libusberr) {
            case 0:
                break;

            case LIBUSB_ERROR_PIPE:
            case LIBUSB_ERROR_NO_DEVICE:
            case LIBUSB_ERROR_IO:
                msg(ll_error, "USB device is no longer available.");
                medium->flags |= CAHUTE_LINK_MEDIUM_FLAG_GONE;
                return CAHUTE_ERROR_GONE;

            case LIBUSB_ERROR_TIMEOUT:
                goto time_out;

            default:
                msg(ll_error,
                    "libusb_bulk_transfer returned %d: %s",
                    libusberr,
                    libusb_error_name(libusberr));
                if (libusberr == LIBUSB_ERROR_OVERFLOW)
                    msg(ll_error, "Required buffer size was %d.", received);
                return CAHUTE_ERROR_UNKNOWN;
            }

            bytes_read = (size_t)received;
        } break;
#endif

#if defined(CAHUTE_LINK_MEDIUM_WIN32_UMS) \
    || defined(CAHUTE_LINK_MEDIUM_LIBUSB_UMS)
# if defined(CAHUTE_LINK_MEDIUM_WIN32_UMS)
        case CAHUTE_LINK_MEDIUM_WIN32_UMS:
# endif
# if defined(CAHUTE_LINK_MEDIUM_LIBUSB_UMS)
        case CAHUTE_LINK_MEDIUM_LIBUSB_UMS:
# endif
        {
            cahute_u8 status_buf[16];
            cahute_u8 payload[16];
            size_t avail;

            /* We use custom command C0 to poll status and get avail. bytes.
             * See :ref:`ums-command-c0` for more information.
             *
             * Note that it may take time for the calculator to "recharge"
             * the buffer, so we want to try several times in a row before
             * declaring there is no data available yet. */
            err = cahute_scsi_request_from_link_medium(
                medium,
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
                err = cahute_sleep(10);
                if (err)
                    return err;

                continue;
            }

            /* NOTE: The target size here should always at least have 4 MiB,
             * which means this condition should actually never evaluate
             * to true. */
            if (avail > target_size)
                avail = target_size;

            /* We now use custom command C1 to request avail. bytes.
             * See :ref:`ums-command-c1` for more information. */
            memcpy(
                payload,
                (cahute_u8 const *)"\xC1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                16
            );
            payload[6] = (avail >> 8) & 255;
            payload[7] = avail & 255;

            err = cahute_scsi_request_from_link_medium(
                medium,
                payload,
                16,
                dest,
                avail,
                NULL
            );
            if (err)
                return err;

            bytes_read = avail;
        } break;
#endif

        default:
            CAHUTE_RETURN_IMPL("No method available for reading.");
        }

        if (!bytes_read)
            continue;

        /* At least one byte has been read in this iteration; we can reset
         * the timeout to the next timeout. */
        timeout = next_timeout;
        iteration_timeout = next_timeout;
        timeout_error = CAHUTE_ERROR_TIMEOUT;

        if (!first_time) {
            err = cahute_monotonic(&first_time);
            if (err)
                return err;

            last_time = first_time;
        } else {
            err = cahute_monotonic(&last_time);
            if (err)
                return err;
        }

        if (bytes_read >= size) {
            if (buf)
                memcpy(buf, medium->read_buffer, size);

            medium->read_start = size;
            medium->read_size = bytes_read;

            break;
        }

        if (buf) {
            memcpy(buf, medium->read_buffer, bytes_read);
            buf += bytes_read;
        }

        size -= bytes_read;
    }

    if (!cahute_monotonic(&last_time)) {
        if (first_time > start_time + 20) {
            msg(ll_info,
                "Read %" CAHUTE_PRIuSIZE
                " bytes in %lums (after waiting %lums).",
                original_size + medium->read_size - medium->read_start,
                last_time - first_time,
                first_time - start_time);
        } else {
            msg(ll_info,
                "Read %" CAHUTE_PRIuSIZE " bytes in %lums.",
                original_size + medium->read_size - medium->read_start,
                last_time - start_time);
        }
    }

    return CAHUTE_OK;

time_out:
    msg(ll_error,
        "Hit a timeout of %lums after reading %" CAHUTE_PRIuSIZE
        "/%" CAHUTE_PRIuSIZE " bytes.",
        iteration_timeout,
        original_size - size,
        original_size);
    return timeout_error;
}

/**
 * Write data synchronously to the medium associated with the given medium.
 *
 * There is no write buffering specific to Cahute: the buffer is directly
 * written to the underlying medium.
 *
 * @param medium Link medium to write data to.
 * @param buf Buffer to write to the medium.
 * @param size Size of the buffer to write.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_write_to_link_medium(
    cahute_link_medium *medium,
    cahute_u8 const *buf,
    size_t size
) {
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
        switch (medium->type) {
#ifdef CAHUTE_LINK_MEDIUM_POSIX_SERIAL
        case CAHUTE_LINK_MEDIUM_POSIX_SERIAL: {
            cahute_ssize ret;

            ret = write(medium->state.posix.fd, buf, size);
            if (ret < 0)
                switch (errno) {
                case ENODEV:
                    medium->flags |= CAHUTE_LINK_MEDIUM_FLAG_GONE;
                    return CAHUTE_ERROR_GONE;

                default:
                    msg(ll_fatal, "errno was %d: %s", errno, strerror(errno));
                    return CAHUTE_ERROR_UNKNOWN;
                }

            bytes_written = (size_t)ret;
        } break;
#endif

#if defined(CAHUTE_LINK_MEDIUM_WIN32_CESG) \
    || defined(CAHUTE_LINK_MEDIUM_WIN32_SERIAL)
# if defined(CAHUTE_LINK_MEDIUM_WIN32_CESG)
        case CAHUTE_LINK_MEDIUM_WIN32_CESG:
# endif
# if defined(CAHUTE_LINK_MEDIUM_WIN32_SERIAL)
        case CAHUTE_LINK_MEDIUM_WIN32_SERIAL:
# endif
        {
            DWORD sent;
            BOOL ret;

            ret = WriteFile(
                medium->state.windows.handle,
                buf,
                size,
                &sent,
                &medium->state.windows.overlapped
            );
            if (!ret) {
                DWORD werr = GetLastError();

                if (werr == ERROR_IO_PENDING) {
                    ret = WaitForSingleObject(
                        medium->state.windows.overlapped.hEvent,
                        INFINITE
                    );
                    switch (ret) {
                    case WAIT_OBJECT_0:
                        ret = GetOverlappedResult(
                            medium->state.windows.handle,
                            &medium->state.windows.overlapped,
                            &sent,
                            FALSE
                        );
                        if (!ret) {
                            werr = GetLastError();
                            if (werr == ERROR_GEN_FAILURE)
                                return CAHUTE_ERROR_GONE;

                            log_windows_error("GetOverlappedResult", werr);
                            return CAHUTE_ERROR_UNKNOWN;
                        }
                        break;

                    default:
                        log_windows_error(
                            "WaitForSingleObject",
                            GetLastError()
                        );
                        return CAHUTE_ERROR_UNKNOWN;
                    }
                } else {
                    log_windows_error("WriteFile", werr);
                    return CAHUTE_ERROR_UNKNOWN;
                }
            }

            bytes_written = (size_t)sent;
        }

        break;
#endif

#if defined(CAHUTE_LINK_MEDIUM_AMIGAOS_SERIAL)
        case CAHUTE_LINK_MEDIUM_AMIGAOS_SERIAL: {
            struct IOExtSer *io = medium->state.amigaos_serial.io;

            io->IOSer.io_Length = size;
            io->IOSer.io_Data = (cahute_u8 *)buf; /* Explicit non-const. */
            io->IOSer.io_Command = CMD_WRITE;
            if (DoIO((struct IORequest *)io)) {
                msg(ll_error, "Unable to set the serial parameters!");
                return CAHUTE_ERROR_UNKNOWN;
            }

            bytes_written = size;
        } break;
#endif

#ifdef CAHUTE_LINK_MEDIUM_LIBUSB
        case CAHUTE_LINK_MEDIUM_LIBUSB: {
            int libusberr;
            int sent;

            libusberr = libusb_bulk_transfer(
                medium->state.libusb.handle,
                medium->state.libusb.bulk_out,
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
                medium->flags |= CAHUTE_LINK_MEDIUM_FLAG_GONE;
                return CAHUTE_ERROR_GONE;

            default:
                msg(ll_error,
                    "libusb_bulk_transfer returned %d: %s",
                    libusberr,
                    libusb_error_name(libusberr));
                return CAHUTE_ERROR_UNKNOWN;
            }

            bytes_written = (size_t)sent;
        }

        break;
#endif

#if defined(CAHUTE_LINK_MEDIUM_WIN32_UMS) \
    || defined(CAHUTE_LINK_MEDIUM_LIBUSB_UMS)
# if defined(CAHUTE_LINK_MEDIUM_WIN32_UMS)
        case CAHUTE_LINK_MEDIUM_WIN32_UMS:
# endif
# if defined(CAHUTE_LINK_MEDIUM_LIBUSB_UMS)
        case CAHUTE_LINK_MEDIUM_LIBUSB_UMS:
# endif
        {
            size_t to_send = size > 0xFFFF ? 0xFFFF : size;
            cahute_u8 payload[16], status_buf[16];
            int err;

            /* We use custom command C0 to poll status and get avail. bytes.
             * See :ref:`ums-command-c0` for more information. */
            err = cahute_scsi_request_from_link_medium(
                medium,
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

            err = cahute_scsi_request_to_link_medium(
                medium,
                payload,
                16,
                buf,
                to_send,
                NULL
            );
            if (err)
                return err;

            bytes_written = to_send;
        } break;
#endif

        default:
            CAHUTE_RETURN_IMPL("No method available for writing.");
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
 * @param medium Link medium on which to set the serial parameters.
 * @param flags Serial parameters to set.
 * @param speed Speed to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_set_serial_params_to_link_medium(
    cahute_link_medium *medium,
    unsigned long flags,
    unsigned long speed
) {
    if (medium->serial_flags == flags && medium->serial_speed == speed)
        return CAHUTE_OK;

    switch (medium->type) {
#ifdef CAHUTE_LINK_MEDIUM_POSIX_SERIAL
    case CAHUTE_LINK_MEDIUM_POSIX_SERIAL: {
        struct termios term;
        speed_t termios_speed;

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
# ifdef B9600
        case 9600:
            termios_speed = B9600;
            break;
# endif
# ifdef B19200
        case 19200:
            termios_speed = B19200;
            break;
# endif
# ifdef B38400
        case 38400:
            termios_speed = B38400;
            break;
# endif
# ifdef B57600
        case 57600:
            termios_speed = B57600;
            break;
# endif
# ifdef B115200
        case 115200:
            termios_speed = B115200;
            break;
# endif
        default:
            msg(ll_error, "Speed unsupported by termios: %lu", speed);
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (tcdrain(medium->state.posix.fd)) {
            msg(ll_error,
                "Could not wait until data has been written: %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (tcgetattr(medium->state.posix.fd, &term) < 0) {
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

        if (tcsetattr(medium->state.posix.fd, TCSANOW, &term)) {
            msg(ll_error,
                "Could not get serial attributes: %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }

# if defined(TIOCM_DTR) || defined(TIOCM_RTS)
        {
            unsigned int status, original_status;

            /* Also set the DTR/RTS mode. */
            if (ioctl(medium->state.posix.fd, TIOCMGET, &status) >= 0)
                status = 0;

            original_status = status;

#  if defined(TIOCM_DTR)
            switch (flags & CAHUTE_SERIAL_DTR_MASK) {
            case CAHUTE_SERIAL_DTR_ENABLE:
            case CAHUTE_SERIAL_DTR_HANDSHAKE:
                status |= TIOCM_DTR;
                break;

            default:
                status &= ~TIOCM_DTR;
                break;
            }
#  endif

#  if defined(TIOCM_RTS)
            switch (flags & CAHUTE_SERIAL_RTS_MASK) {
            case CAHUTE_SERIAL_RTS_ENABLE:
            case CAHUTE_SERIAL_RTS_HANDSHAKE:
                status |= TIOCM_RTS;
                break;

            default:
                status &= ~TIOCM_RTS;
                break;
            }
#  endif

            if (status != original_status
                && ioctl(medium->state.posix.fd, TIOCMSET, &status) < 0) {
                msg(ll_error, "Could not set DTR/RTS mode.");
                return CAHUTE_ERROR_UNKNOWN;
            }
        }
# endif
    } break;
#endif

#ifdef CAHUTE_LINK_MEDIUM_WIN32_SERIAL
    case CAHUTE_LINK_MEDIUM_WIN32_SERIAL: {
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
        if (!GetCommState(medium->state.windows.handle, &dcb)) {
            log_windows_error("GetCommState", GetLastError());
            return CAHUTE_ERROR_UNKNOWN;
        }

        dcb.BaudRate = dcb_speed;
        dcb.ByteSize = 8;
        dcb.fOutxCtsFlow = 0;
        dcb.fOutxDsrFlow = 0;
        dcb.fDsrSensitivity = 0;
        dcb.fNull = 0;

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

        dcb.fTXContinueOnXoff = 0;
        dcb.XonChar = 0x13;
        dcb.XoffChar = 0x11;
        dcb.XonLim = 0;
        dcb.XoffLim = 0;

        switch (flags & CAHUTE_SERIAL_XONXOFF_MASK) {
        case CAHUTE_SERIAL_XONXOFF_ENABLE:
            dcb.fInX = 1;
            dcb.fOutX = 1;
            break;

        default:
            dcb.fInX = 0;
            dcb.fOutX = 0;
        }

        switch (flags & CAHUTE_SERIAL_DTR_MASK) {
        case CAHUTE_SERIAL_DTR_ENABLE:
            dcb.fDtrControl = DTR_CONTROL_ENABLE;
            break;

        case CAHUTE_SERIAL_DTR_HANDSHAKE:
            dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
            break;

        default:
            dcb.fDtrControl = DTR_CONTROL_DISABLE;
        }

        switch (flags & CAHUTE_SERIAL_RTS_MASK) {
        case CAHUTE_SERIAL_RTS_ENABLE:
            dcb.fRtsControl = RTS_CONTROL_ENABLE;
            break;

        case CAHUTE_SERIAL_RTS_HANDSHAKE:
            dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
            break;

        default:
            dcb.fRtsControl = RTS_CONTROL_DISABLE;
        }

        if (!SetCommState(medium->state.windows.handle, &dcb)) {
            log_windows_error("SetCommState", GetLastError());
            return CAHUTE_ERROR_UNKNOWN;
        }
    } break;
#endif

#if defined(CAHUTE_LINK_MEDIUM_AMIGAOS_SERIAL)
    case CAHUTE_LINK_MEDIUM_AMIGAOS_SERIAL: {
        struct IOExtSer *io = medium->state.amigaos_serial.io;

        io->io_CtlChar = 0x00001311;
        io->io_RBufLen = 1024;
        io->io_ExtFlags = 0;
        io->io_Baud = speed;
        io->io_BrkTime = 250000;
        io->io_TermArray.TermArray0 = 0;
        io->io_TermArray.TermArray1 = 0;
        io->io_ReadLen = 8;
        io->io_WriteLen = 8;
        io->io_SerFlags = SERF_SHARED;
        io->io_Status = 0;

        switch (flags & CAHUTE_SERIAL_XONXOFF_MASK) {
        case CAHUTE_SERIAL_XONXOFF_DISABLE:
            io->io_SerFlags |= SERF_XDISABLED;
            break;
        }

        switch (flags & CAHUTE_SERIAL_STOP_MASK) {
        case CAHUTE_SERIAL_STOP_ONE:
            io->io_StopBits = 1;
            break;

        case CAHUTE_SERIAL_STOP_TWO:
            io->io_StopBits = 2;
            break;
        }

        switch (flags & CAHUTE_SERIAL_PARITY_MASK) {
        case CAHUTE_SERIAL_PARITY_EVEN:
            io->io_SerFlags |= SERF_PARTY_ON;
            break;

        case CAHUTE_SERIAL_PARITY_ODD:
            io->io_SerFlags |= SERF_PARTY_ON | SERF_PARTY_ODD;
            break;
        }

        /* TODO: AmigaOS doesn't manage DTR/RTS directly, which means we
         * may have to manage it here manually! */

        io->IOSer.io_Command = SDCMD_SETPARAMS;
        if (DoIO((struct IORequest *)io)) {
            msg(ll_error, "Unable to set the serial parameters!");
            return CAHUTE_ERROR_UNKNOWN;
        }
    } break;
#endif

    default:
        CAHUTE_RETURN_IMPL("No method available for setting serial params.");
    }

    medium->serial_flags = flags;
    medium->serial_speed = speed;
    return CAHUTE_OK;
}

/**
 * Emit an SCSI request to a link medium, while sending or receiving data.
 *
 * This is the internal function behind :c:func:`cahute_scsi_request_to_link`
 * and :c:func:`cahute_scsi_request_from_link`, however it must not be used
 * directly by any other function.
 *
 * @param medium Link medium to which to emit the SCSI request.
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
    cahute_link_medium *medium,
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

    /* The medium-specific implementation must store the status in the
     * ``status`` variable (NOT ``*statusp``). */
    switch (medium->type) {
#ifdef CAHUTE_LINK_MEDIUM_WIN32_UMS
    case CAHUTE_LINK_MEDIUM_WIN32_UMS: {
        SCSI_PASS_THROUGH_DIRECT req;
        DWORD wret, werr, wcnt;

        SecureZeroMemory(&req, sizeof(SCSI_PASS_THROUGH_DIRECT));
        req.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
        req.TimeOutValue = 30;
        req.CdbLength = command_size;
        memcpy(req.Cdb, command, command_size);

        if (!is_send) {
            req.DataIn = SCSI_IOCTL_DATA_IN;
            req.DataBuffer = buf;
            req.DataTransferLength = buf_size;
        } else if (buf_size) {
            req.DataIn = SCSI_IOCTL_DATA_OUT;
            req.DataBuffer = buf;
            req.DataTransferLength = buf_size;
        } else {
            req.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
            req.DataBuffer = NULL;
            req.DataTransferLength = 0;
        }

        wret = DeviceIoControl(
            medium->state.windows.handle,
            IOCTL_SCSI_PASS_THROUGH_DIRECT,
            &req,
            sizeof(req),
            &req,
            sizeof(req),
            &wcnt,
            NULL
        );

        if (!wret) {
            werr = GetLastError();

            if (werr == ERROR_SEM_TIMEOUT)
                return CAHUTE_ERROR_GONE;

            log_windows_error("DeviceIoControl", werr);
            return CAHUTE_ERROR_UNKNOWN;
        }

        status = (int)req.ScsiStatus;
    } break;
#endif

#ifdef CAHUTE_LINK_MEDIUM_LIBUSB_UMS
    case CAHUTE_LINK_MEDIUM_LIBUSB_UMS: {
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
            medium->state.libusb.handle,
            medium->state.libusb.bulk_out,
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
            medium->flags |= CAHUTE_LINK_MEDIUM_FLAG_GONE;
            return CAHUTE_ERROR_GONE;

        default:
            msg(ll_error,
                "libusb_bulk_transfer returned %d: %s",
                libusberr,
                libusb_error_name(libusberr));
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

        if (!buf_size) {
            /* Nothing to be sent, nothing to be received. */
        } else if (is_send) {
            int libusberr, sent = 0;

            libusberr = libusb_bulk_transfer(
                medium->state.libusb.handle,
                medium->state.libusb.bulk_out,
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
                medium->flags |= CAHUTE_LINK_MEDIUM_FLAG_GONE;
                return CAHUTE_ERROR_GONE;

            default:
                msg(ll_error,
                    "libusb_bulk_transfer returned %d: %s",
                    libusberr,
                    libusb_error_name(libusberr));
                return CAHUTE_ERROR_UNKNOWN;
            }
        } else {
            do {
                int libusberr, recv = 0;

                libusberr = libusb_bulk_transfer(
                    medium->state.libusb.handle,
                    medium->state.libusb.bulk_in,
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
                    medium->flags |= CAHUTE_LINK_MEDIUM_FLAG_GONE;
                    return CAHUTE_ERROR_GONE;

                default:
                    msg(ll_error,
                        "libusb_bulk_transfer returned %d: %s",
                        libusberr,
                        libusb_error_name(libusberr));
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
                    medium->state.libusb.handle,
                    medium->state.libusb.bulk_in,
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
                    medium->flags |= CAHUTE_LINK_MEDIUM_FLAG_GONE;
                    return CAHUTE_ERROR_GONE;

                default:
                    msg(ll_error,
                        "libusb_bulk_transfer returned %d: %s",
                        libusberr,
                        libusb_error_name(libusberr));
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
        CAHUTE_RETURN_IMPL("No method available for making an SCSI request.");
    }

    if (statusp)
        *statusp = status;
    return CAHUTE_OK;
}

/**
 * Emit an SCSI request to a link medium, with or without data.
 *
 * Note that ``*statusp`` may be NULL!
 *
 * @param medium Link medium to which to emit the SCSI request.
 * @param command Command to emit to the medium, of 6, 10, 12 or 16 bytes.
 * @param command_size Size of the command to emit to the medium.
 * @param data Optional data to append to the command.
 * @param data_size Size of the optional data to append to the command.
 * @param statusp Pointer to the SCSI status to set to the received one.
 */
CAHUTE_EXTERN(int)
cahute_scsi_request_to_link_medium(
    cahute_link_medium *medium,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 const *data,
    size_t data_size,
    int *statusp
) {
    return cahute_scsi_request(
        medium,
        command,
        command_size,
        (cahute_u8 *)data, /* Explicit removal of const. */
        data_size,
        1,
        statusp
    );
}

/**
 * Emit an SCSI request to a link medium, and receive data.
 *
 * NOTE: ``*statusp`` may be NULL.
 *
 * @param medium Link medium to which to emit the SCSI request.
 * @param command Command to emit to the medium, of 6, 10, 12 or 16 bytes.
 * @param command_size Size of the command to emit to the medium.
 * @param buf Buffer to fill with the command's result.
 * @param buf_size Buffer capacity to not go past.
 * @param statusp Pointer to the SCSI status to set to the received one.
 */
CAHUTE_EXTERN(int)
cahute_scsi_request_from_link_medium(
    cahute_link_medium *medium,
    cahute_u8 const *command,
    size_t command_size,
    cahute_u8 *buf,
    size_t buf_size,
    int *statusp
) {
    return cahute_scsi_request(
        medium,
        command,
        command_size,
        buf,
        buf_size,
        0,
        statusp
    );
}
