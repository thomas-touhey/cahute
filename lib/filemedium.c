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
#define CAHUTE_FILE_MEDIUM_WRITE_CHUNK_SIZE 4096

/* NUL bytes to write when skipping a stream that does not support seeking. */
CAHUTE_LOCAL_DATA(cahute_u8 const) null_buffer[1024] = {0};

/**
 * Read from the current offset in the file, using the medium specific
 * function.
 *
 * This reads at most ``CAHUTE_FILE_MEDIUM_READ_BUFFER_SIZE`` into the
 * read buffer, sets ``medium->read_offset`` and ``medium->read_size``
 * accordingly, and moves ``medium->offset`` to the offset right after
 * the current read, even in the case of cursor-less mediums.
 *
 * @param medium Medium from which to read.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
read_from_current_offset_in_medium(cahute_file_medium *medium) {
    cahute_u8 *read_buffer = medium->read_buffer;
    size_t read_size = CAHUTE_FILE_MEDIUM_READ_BUFFER_SIZE;
    size_t bytes_read = 0;

    /* The implementation must read data in ``read_buffer``,
     * for up to ``read_size`` (while the caller only requires ``size``,
     * although ``size`` may be larger than ``read_size``).
     * It must set ``bytes_read`` to the actual number of bytes
     * read this pass.
     *
     * A ``bytes_read`` value of 0 is interpreted later as an EOF. */
    switch (medium->type) {
#ifdef CAHUTE_FILE_MEDIUM_POSIX
    case CAHUTE_FILE_MEDIUM_POSIX: {
        cahute_ssize ret;

        ret = read(medium->state.posix.fd, read_buffer, read_size);
        if (ret < 0)
            switch (errno) {
            default:
                msg(ll_error,
                    "An error occurred while calling read(): %s (%d)",
                    strerror(errno),
                    errno);
                return CAHUTE_ERROR_UNKNOWN;
            }

        bytes_read = (size_t)ret;
    } break;
#endif

#ifdef CAHUTE_FILE_MEDIUM_WIN32
    case CAHUTE_FILE_MEDIUM_WIN32: {
        BOOL ret;
        DWORD received;

        ret = ReadFile(
            medium->state.windows.handle,
            read_buffer,
            read_size,
            &received,
            NULL
        );
        if (!ret) {
            log_windows_error("ReadFile", GetLastError());
            return CAHUTE_ERROR_UNKNOWN;
        }

        bytes_read = (size_t)received;
    } break;
#endif

    default:
        CAHUTE_RETURN_IMPL("No method available for reading the file.");
    }

    /* If we have arrived here, we consider the read offset to have been
     * modified, so we update it here. */
    medium->offset += bytes_read;
    medium->read_offset += medium->read_size;
    medium->read_size = bytes_read;

    if (!bytes_read) {
        /* An EOF was signalled, but should not have occurred! */
        msg(ll_error, "EOF signalled too early!");
        return CAHUTE_ERROR_UNKNOWN;
    }

    return CAHUTE_OK;
}

/**
 * Write in the current offset of the file.
 *
 * This writes ``data`` with the provided ``size`` into the medium, updates
 * the current read buffer if need be, and moves ``medium->offset`` to the
 * offset right after the current write, even in the case of cursor-less
 * mediums.
 *
 * @param medium Medium in which to write.
 * @param data Data to write.
 * @param sizep Pointer to the size of the data to write.
 *        This is set to the number of bytes actually written afterwards.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
write_to_current_offset_in_medium(
    cahute_file_medium *medium,
    void const *data,
    size_t *sizep
) {
    cahute_u8 const *write_buffer = data;
    size_t write_size = *sizep;
    size_t bytes_written = 0;

    if (write_size > CAHUTE_FILE_MEDIUM_WRITE_CHUNK_SIZE)
        write_size = CAHUTE_FILE_MEDIUM_WRITE_CHUNK_SIZE;

    /* The implementation must write data from ``write_buffer``,
     * for up to ``write_size``. It must set ``bytes_written`` to the
     * actual number of bytes written this pass.
     *
     * A ``bytes_written`` value of 0 is interpreted as an unknown
     * error. */
    switch (medium->type) {
#ifdef CAHUTE_FILE_MEDIUM_POSIX
    case CAHUTE_FILE_MEDIUM_POSIX: {
        cahute_ssize ret;

        ret = write(medium->state.posix.fd, write_buffer, write_size);
        if (ret < 0)
            switch (errno) {
            default:
                msg(ll_error,
                    "An error occurred while calling write(): %s (%d)",
                    strerror(errno),
                    errno);
                return CAHUTE_ERROR_UNKNOWN;
            }

        bytes_written = (size_t)ret;
    } break;
#endif

#ifdef CAHUTE_FILE_MEDIUM_WIN32
    case CAHUTE_FILE_MEDIUM_WIN32: {
        BOOL ret;
        DWORD written;

        ret = WriteFile(
            medium->state.windows.handle,
            write_buffer,
            write_size,
            &written,
            NULL
        );
        if (!ret) {
            log_windows_error("WriteFile", GetLastError());
            return CAHUTE_ERROR_UNKNOWN;
        }

        bytes_written = (size_t)written;
    } break;
#endif

    default:
        CAHUTE_RETURN_IMPL("No method available for writing into the file.");
    }

    if (!bytes_written || bytes_written > write_size) {
        /* This should not have occurred, it is considered a bug. */
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* If the read buffer overlaps with the data actually written, we
     * also want to update our read buffer with the written data if there
     * is an overlap.
     *
     * We first need to find the overlap between both positions.
     * Suppose the following diagram:
     *
     *              off1  off2  off3  off4
     *    (written)  |-----------|
     *   (read_buf)        |-----------|
     *
     * In this case, we need to copy the data from off2 to off3
     * in both buffers. Here:
     *
     * - off2 is the maximum value of both off1 and off2.
     * - off3 is the minimum value of both off3 and off4.
     * - Boundaries of the written buffer is done by doing
     *   ``[max(off1, off2) - off1, min(off3, off4) - off1]``.
     * - Boundaries of the read buffer is done by doing
     *   ``[max(off1, off2) - off2, min(off3, off4) - off2]``. */
    {
        size_t off1 = medium->offset;
        size_t off3 = medium->offset + bytes_written;
        size_t off2 = medium->read_offset;
        size_t off4 = medium->read_offset + medium->read_size;
        size_t loff = off1 > off2 ? off1 : off2;
        size_t roff = off3 < off4 ? off3 : off4;

        if (loff <= roff)
            memcpy(
                &medium->read_buffer[loff - off2],
                &write_buffer[loff - off1],
                roff - loff
            );
    }

    /* If we have arrived here, we consider the read offset to have been
     * modified, so we update it here. */
    medium->offset += bytes_written;

    *sizep = bytes_written;
    return CAHUTE_OK;
}

/**
 * Move to a given offset, and ensure that we can read or write a given size.
 *
 * @param medium File medium object.
 * @param off Offset at which to move.
 * @param size Size to ensure that we can read or write.
 * @param write The move is made for writing, and not for reading, therefore
 *        the offset must be exact and not within the window.
 * @return Error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
move_to_offset(
    cahute_file_medium *medium,
    unsigned long off,
    size_t size,
    int write
) {
    unsigned long new_off = off;
    int err;

    if (off > CAHUTE_MAX_FILE_OFFSET - size) {
        /* Offsets above CAHUTE_MAX_FILE_OFFSET are not supported, therefore
         * we prefer to fail explicitely here. */
        msg(ll_error,
            "Cannot %s %" CAHUTE_PRIuSIZE
            "at offset %lu, since it would "
            "cause the file offset to reach undefined values.",
            write ? "write" : "read",
            size,
            off);
        return CAHUTE_ERROR_SIZE;
    }

    if ((medium->flags & CAHUTE_FILE_MEDIUM_FLAG_SIZE)
        && off > medium->file_size - size) {
        /* Our file interface requires setting the file size explicitely
         * if writing further than the current file size. */
        msg(ll_error,
            "Cannot %s %" CAHUTE_PRIuSIZE
            "at offset %lu, since it would "
            "cause the file offset to go past the %lu file size.",
            write ? "write" : "read",
            size,
            off);
        return CAHUTE_ERROR_SIZE;
    }

    /* If we're already at the right offset, there is no need to explicitely
     * move here. */
    if (off == medium->offset)
        return CAHUTE_OK;

    if (~medium->flags & CAHUTE_FILE_MEDIUM_FLAG_SEEK) {
        /* If the offset we're trying to read is actually further in the
         * file, we can read or write up until the offset.
         *
         * NOTE: Cursorless mediums are expected to have this flag set,
         * and have their type be a no-op later in this function. */
        if (off < medium->offset) {
            msg(ll_error, "Medium does not support seeking.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (write)
            while (medium->offset < off) {
                size_t write_size = off - medium->offset;

                if (write_size > sizeof(null_buffer))
                    write_size = sizeof(null_buffer);

                err = write_to_current_offset_in_medium(
                    medium,
                    null_buffer,
                    &write_size
                );
                if (err)
                    return err;
            }
        else
            while (medium->offset < off) {
                /* NOTE: Since we don't want to control the size here
                 * to avoid making two read() syscalls instead of one,
                 * it means that we may need to compensate in
                 * ``cahute_read_from_file_medium()`` to actually check
                 * the read buffer again after moving the offset. */
                err = read_from_current_offset_in_medium(medium);
                if (err)
                    return err;
            }

        return CAHUTE_OK;
    }

    /* The implementation must ask for relocation to ``off``, and
     * if the new offset is obtained, set it to ``new_off``. */
    switch (medium->type) {
#ifdef CAHUTE_FILE_MEDIUM_POSIX
    case CAHUTE_FILE_MEDIUM_POSIX: {
        off_t loff = (off_t)off;
        medium->read_offset = new_off;

        off_t new_loff = lseek(medium->state.posix.fd, loff, SEEK_SET);

        if (new_loff == (off_t)-1)
            switch (errno) {
            case EOVERFLOW:
                /* off_t may be 16-bits on some platforms, but we support
                * up to 32-bit offsets here, so we can safely ignore this. */
                break;

            default:
                msg(ll_error,
                    "An error occurred while calling lseek(): %s (%d)",
                    strerror(errno),
                    errno);
                return CAHUTE_ERROR_UNKNOWN;
            }
        else
            new_off = new_loff;
    } break;
#endif

#ifdef CAHUTE_FILE_MEDIUM_WIN32
    case CAHUTE_FILE_MEDIUM_WIN32: {
        DWORD dwoff = (DWORD)off;
        DWORD dwnewoff = SetFilePointer(
            medium->state.windows.handle,
            dwoff,
            NULL,
            FILE_BEGIN
        );

        if (dwnewoff == INVALID_SET_FILE_POINTER) {
            log_windows_error("SetFilePointer", GetLastError());
            return CAHUTE_ERROR_UNKNOWN;
        } else
            new_off = dwnewoff;
    } break;
#endif

    default:
        CAHUTE_RETURN_IMPL("No method available for seeking in the file.");
    }

    /* The offset may have been automatically been adjusted to the end of
     * the file, which means an automatic end of file. */
    medium->offset = new_off;

    return CAHUTE_OK;
}

/**
 * Read data from a file.
 *
 * @param medium File medium object.
 * @param off Offset at which to read data.
 * @param buf Buffer in which to write the result.
 * @param size Size of the data to read.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_read_from_file_medium(
    cahute_file_medium *medium,
    unsigned long off,
    cahute_u8 *buf,
    size_t size
) {
    int err;

    if (~medium->flags & CAHUTE_FILE_MEDIUM_FLAG_READ) {
        msg(ll_error, "File is not readable.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (!size)
        return CAHUTE_OK;

    if (!buf) {
        /* As opposed to cahute_read_from_link_medium(), this function does
         * not support "skipping", as it takes an "off" parameter to do
         * exactly that. This may however cause some confusion, so we want
         * to catch this explicitely. */
        msg(ll_error,
            "cahute_read_from_file_medium() requires a non-NULL buffer!");
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* Check if we can determine at least part of the data using our current
     * read buffer. */
    if (off < medium->read_offset + medium->read_size
        && off >= medium->read_offset) {
        size_t start_offset = off - medium->read_offset;
        size_t to_copy = medium->read_size;

        /* We want to copy what exists from the current read buffer. */
        if (to_copy > size) {
            memcpy(buf, &medium->read_buffer[start_offset], size);
            return CAHUTE_OK;
        }

        memcpy(buf, &medium->read_buffer[start_offset], to_copy);
        off += to_copy;
        buf += to_copy;
        size -= to_copy;
    }

    /* There is still content to be read, so we want to move the cursor to
     * the offset we want to read now. */
    err = move_to_offset(medium, off, size, 0);
    if (err == CAHUTE_ERROR_SIZE)
        return CAHUTE_ERROR_TRUNC;
    else if (err)
        return err;

    /* While moving to the offset, we may have gone further than the actual
     * expected offset, so we need to check here if we have some bytes to
     * complete from the current read buffer. */
    if (off < medium->read_offset + medium->read_size
        && off >= medium->read_offset) {
        size_t start_offset = off - medium->read_offset;
        size_t to_copy = medium->read_size;

        /* We want to copy what exists from the current read buffer. */
        if (to_copy > size) {
            memcpy(buf, &medium->read_buffer[start_offset], size);
            return CAHUTE_OK;
        }

        memcpy(buf, &medium->read_buffer[start_offset], to_copy);
        off += to_copy;
        buf += to_copy;
        size -= to_copy;
    }

    /* We can discard the current read buffer in any case. */
    medium->read_offset = medium->offset;
    medium->read_size = 0;

    /* We need to complete the buffer here, by doing multiple passes on the
     * medium implementation specific read code until the caller's need
     * is fully satisfied. */
    while (size) {
        err = read_from_current_offset_in_medium(medium);
        if (err)
            return err;

        if (medium->read_size >= size) {
            memcpy(buf, medium->read_buffer, size);
            break;
        }

        memcpy(buf, medium->read_buffer, medium->read_size);
        buf += medium->read_size;
        size -= medium->read_size;
    }

    return CAHUTE_OK;
}

/**
 * Write to a file medium.
 *
 * @param medium File medium to which to write to.
 * @param offset Offset at which to write data.
 * @param data Data to write.
 * @param size Size of the data to write.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_write_to_file_medium(
    cahute_file_medium *medium,
    unsigned long offset,
    void const *data,
    size_t size
) {
    int err;

    if (~medium->flags & CAHUTE_FILE_MEDIUM_FLAG_WRITE) {
        msg(ll_error, "File is not writable.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (!size)
        return CAHUTE_OK;

    err = move_to_offset(medium, offset, size, 1);
    if (err)
        return err;

    while (size) {
        size_t write_size = size;

        err = write_to_current_offset_in_medium(medium, data, &write_size);
        if (err)
            return err;

        data = (cahute_u8 const *)data + write_size;
        size -= write_size;
    }

    return CAHUTE_OK;
}
