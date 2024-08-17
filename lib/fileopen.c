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
 * Get the name of a file medium.
 *
 * @param medium Medium identifier, as a constant.
 * @return Textual name of the medium.
 */
CAHUTE_LOCAL(char const *) get_medium_name(int medium) {
    switch (medium) {
#ifdef CAHUTE_FILE_MEDIUM_POSIX
    case CAHUTE_FILE_MEDIUM_POSIX:
        return "POSIX";
#endif
#ifdef CAHUTE_FILE_MEDIUM_WIN32
    case CAHUTE_FILE_MEDIUM_WIN32:
        return "Win32";
#endif
    default:
        return "(unknown)";
    }
}

/**
 * Close a file medium.
 *
 * @param type Medium type.
 * @param state Medium state.
 */
CAHUTE_LOCAL(void)
close_medium(int type, union cahute_file_medium_state *state) {
    switch (type) {
#ifdef CAHUTE_FILE_MEDIUM_POSIX
    case CAHUTE_FILE_MEDIUM_POSIX:
        close(state->posix.fd);
        break;
#endif
#ifdef CAHUTE_FILE_MEDIUM_WIN32
    case CAHUTE_FILE_MEDIUM_WIN32:
        CloseHandle(state->windows.handle);
        break;
#endif
    default:
        msg(ll_warn,
            "No closing method for %s (%d) file medium.",
            get_medium_name(type),
            type);
    }
}

/**
 * Create a file out of a medium type and state.
 *
 * @param filep Pointer to the file to initialize.
 * @param medium_type Medium type.
 * @param medium_state Medium state.
 * @param medium_flags Medium flags.
 * @param file_flags Initial file flags.
 * @param file_size Size of the file.
 * @param extension Extension, or NULL if no extension was provided.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
open_file_from_medium(
    cahute_file **filep,
    int medium_type,
    union cahute_file_medium_state *medium_state,
    unsigned long medium_flags,
    unsigned long file_flags,
    unsigned long file_size,
    char const *extension
) {
    cahute_file *file = NULL;
    int err = CAHUTE_ERROR_UNKNOWN;

    if (!medium_type) {
        msg(ll_error, "Undefined medium type, this is a bug!");
        goto fail;
    }

    file =
        malloc(sizeof(cahute_file) + 32 + CAHUTE_FILE_MEDIUM_READ_BUFFER_SIZE);
    if (!file) {
        err = CAHUTE_ERROR_ALLOC;
        goto fail;
    }

    /* Initialize medium properties. */
    file->medium.type = medium_type;
    file->medium.flags = medium_flags;
    memcpy(&file->medium.state, medium_state, sizeof(*medium_state));
    file->medium.offset = 0;
    file->medium.read_offset = 0;
    file->medium.read_size = 0;
    file->medium.read_buffer = (cahute_u8 *)file + sizeof(cahute_file);
    file->medium.file_size = file_size;

    /* Ensure that the data buffer is aligned to 32 bytes, for mediums that
     * are sensitive to alignment. */
    file->medium.read_buffer +=
        (~(cahute_uintptr)file->medium.read_buffer & 31) + 1;

    /* Initialize other properties. */
    file->flags = file_flags;
    file->type = 0;
    file->extension[0] = '\0';

    if (extension && strlen(extension) < sizeof(file->extension))
        strcpy(file->extension, extension);

    *filep = file;
    return CAHUTE_OK;

fail:
    if (file)
        free(file);

    close_medium(medium_type, medium_state);
    return err;
}

/**
 * Populate an existing file structure with memory related data.
 *
 * NOTE: This is an internal function only.
 *
 * WARNING: cahute_close_file() MUST NOT be called with such a resource.
 *
 * @param file File structure to populate.
 * @param buf Buffer to read or write from.
 * @param size Size of the buffer.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(void)
cahute_populate_file_from_memory(
    cahute_file *file,
    cahute_u8 *buf,
    size_t size
) {
    file->flags = 0;
    file->type = 0;
    file->extension[0] = 0;
    file->medium.type = CAHUTE_FILE_MEDIUM_NONE;
    file->medium.flags =
        (CAHUTE_FILE_MEDIUM_FLAG_WRITE | CAHUTE_FILE_MEDIUM_FLAG_READ
         | CAHUTE_FILE_MEDIUM_FLAG_SEEK | CAHUTE_FILE_MEDIUM_FLAG_SIZE);
    file->medium.offset = 0;
    file->medium.read_offset = 0;
    file->medium.read_size = size;
    file->medium.file_size = (unsigned long)size;
    file->medium.read_buffer = buf;
}

/**
 * Open a file on the current system, for reading.
 *
 * @param filep Pointer to the file to open.
 * @param path Path to the file to open.
 * @param path_type Type of the path.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_open_file_for_reading(
    cahute_file **filep,
    void const *path,
    int path_type
) {
    int medium_type;
    char extension[10];
    unsigned long file_size;
    union cahute_file_medium_state medium_state;

    /* The implementation specific should populate the medium, medium state
     * and file size. */
#if defined(CAHUTE_FILE_MEDIUM_POSIX)
    {
        int fd;
        off_t off;

        if (path_type != CAHUTE_PATH_TYPE_POSIX)
            CAHUTE_RETURN_IMPL("Path type must be POSIX.");

        fd = open(path, O_RDONLY | O_NOCTTY);
        if (fd < 0) {
            switch (errno) {
            case ENOENT:
                msg(ll_error, "Could not open file: %s", strerror(errno));
                return CAHUTE_ERROR_NOT_FOUND;

            case EACCES:
                return CAHUTE_ERROR_PRIV;

            default:
                msg(ll_error, "Unknown error: %s (%d)", strerror(errno), errno
                );
                return CAHUTE_ERROR_UNKNOWN;
            }
        }

        off = lseek(fd, 0, SEEK_END);

        if (off == (off_t)-1) {
            close(fd);
            switch (errno) {
            default:
                msg(ll_error,
                    "An error occurred while calling lseek(): %s (%d)",
                    strerror(errno),
                    errno);
                return CAHUTE_ERROR_UNKNOWN;
            }
        }

        if (off > CAHUTE_MAX_FILE_OFFSET) {
            msg(ll_warn,
                "File size %lu is longer than maximum offset %lu",
                (unsigned long)off,
                CAHUTE_MAX_FILE_OFFSET);
            close(fd);
            return CAHUTE_ERROR_SIZE;
        }

        file_size = (unsigned long)off;

        off = lseek(fd, 0, SEEK_SET);
        if (off == (off_t)-1) {
            close(fd);
            switch (errno) {
            default:
                msg(ll_error,
                    "An error occurred while calling lseek(): %s (%d)",
                    strerror(errno),
                    errno);
                return CAHUTE_ERROR_UNKNOWN;
            }
        }

        medium_type = CAHUTE_FILE_MEDIUM_POSIX;
        medium_state.posix.fd = fd;
    }
#elif defined(CAHUTE_FILE_MEDIUM_WIN32)
    {
        HANDLE handle = INVALID_HANDLE_VALUE;
        DWORD dwoff, werr;

        if (path_type == CAHUTE_PATH_TYPE_DOS
            || path_type == CAHUTE_PATH_TYPE_WIN32_ANSI)
            handle = CreateFileA(
                path,
                GENERIC_READ,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
        else if (path_type == CAHUTE_PATH_TYPE_WIN32_UNICODE)
            handle = CreateFileW(
                path,
                GENERIC_READ,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
        else
            CAHUTE_RETURN_IMPL("Path type must be Win32 or DOS compatible.");

        if (handle == INVALID_HANDLE_VALUE) {
            switch (werr = GetLastError()) {
            case ERROR_FILE_NOT_FOUND:
                return CAHUTE_ERROR_NOT_FOUND;

            case ERROR_ACCESS_DENIED:
                return CAHUTE_ERROR_PRIV;

            default:
                log_windows_error("CreateFile", werr);
                return CAHUTE_ERROR_UNKNOWN;
            }
        }

        dwoff = SetFilePointer(handle, 0, NULL, FILE_END);
        if (dwoff == INVALID_SET_FILE_POINTER) {
            log_windows_error("SetFilePointer", GetLastError());
            CloseHandle(handle);
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (dwoff > CAHUTE_MAX_FILE_OFFSET) {
            msg(ll_warn,
                "File size %lu is longer than maximum offset %lu",
                (unsigned long)dwoff,
                CAHUTE_MAX_FILE_OFFSET);
            CloseHandle(handle);
            return CAHUTE_ERROR_SIZE;
        }

        file_size = (unsigned long)dwoff;

        dwoff = SetFilePointer(handle, 0, NULL, FILE_BEGIN);
        if (dwoff == INVALID_SET_FILE_POINTER) {
            log_windows_error("SetFilePointer", GetLastError());
            CloseHandle(handle);
            return CAHUTE_ERROR_UNKNOWN;
        }

        medium_type = CAHUTE_FILE_MEDIUM_WIN32;
        medium_state.windows.handle = handle;
    }
#else
    CAHUTE_RETURN_IMPL("No file opening method available.");
#endif

    /* Try to find the path extension.
     * If this method fails, the error is just ignored. */
    cahute_find_path_extension(extension, sizeof(extension), path, path_type);

    return open_file_from_medium(
        filep,
        medium_type,
        &medium_state,
        CAHUTE_FILE_MEDIUM_FLAG_READ | CAHUTE_FILE_MEDIUM_FLAG_SEEK
            | CAHUTE_FILE_MEDIUM_FLAG_SIZE,
        CAHUTE_FILE_FLAG_CLOSE_MEDIUM,
        file_size,
        extension
    );
}

/**
 * Open a file on the current system, for exporting.
 *
 * @param filep Pointer to the file to open.
 * @param file_size File size to set to the file, if creating.
 * @param path Path to the file to open.
 * @param path_type Type of the path.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_open_file_for_export(
    cahute_file **filep,
    unsigned long file_size,
    void const *path,
    int path_type
) {
    int medium_type;
    char extension[10];
    union cahute_file_medium_state medium_state;

    if (file_size > CAHUTE_MAX_FILE_OFFSET) {
        msg(ll_error,
            "Provided size %lu is more than the maximum file size %lu",
            file_size,
            CAHUTE_MAX_FILE_OFFSET);
        return CAHUTE_ERROR_SIZE;
    }

    /* The implementation specific should populate the medium, medium state
     * and file size. */
#if defined(CAHUTE_FILE_MEDIUM_POSIX)
    {
        int fd;

        if (path_type != CAHUTE_PATH_TYPE_POSIX)
            CAHUTE_RETURN_IMPL("Path type must be POSIX.");

        fd = open(
            path,
            O_CREAT | O_RDWR | O_NOCTTY,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
        );
        if (fd < 0) {
            switch (errno) {
            case ENOENT:
                msg(ll_error, "Could not open file: %s", strerror(errno));
                return CAHUTE_ERROR_NOT_FOUND;

            case EACCES:
                return CAHUTE_ERROR_PRIV;

            default:
                msg(ll_error, "Unknown error: %s (%d)", strerror(errno), errno
                );
                return CAHUTE_ERROR_UNKNOWN;
            }
        }

        if (ftruncate(fd, (off_t)file_size)) {
            close(fd);
            switch (errno) {
            default:
                msg(ll_error,
                    "An error occurred while calling lseek(): %s (%d)",
                    strerror(errno),
                    errno);
                return CAHUTE_ERROR_UNKNOWN;
            }
        }

        medium_type = CAHUTE_FILE_MEDIUM_POSIX;
        medium_state.posix.fd = fd;
    }
#elif defined(CAHUTE_FILE_MEDIUM_WIN32)
    {
        HANDLE handle = INVALID_HANDLE_VALUE;
        DWORD dwoff, werr;

        if (path_type == CAHUTE_PATH_TYPE_DOS
            || path_type == CAHUTE_PATH_TYPE_WIN32_ANSI)
            handle = CreateFileA(
                path,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
        else if (path_type == CAHUTE_PATH_TYPE_WIN32_UNICODE)
            handle = CreateFileW(
                path,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
        else
            CAHUTE_RETURN_IMPL("Path type must be Win32 or DOS compatible.");

        if (handle == INVALID_HANDLE_VALUE)
            switch (werr = GetLastError()) {
            case ERROR_FILE_NOT_FOUND:
                return CAHUTE_ERROR_NOT_FOUND;

            case ERROR_ACCESS_DENIED:
                return CAHUTE_ERROR_PRIV;

            default:
                log_windows_error("CreateFile", werr);
                return CAHUTE_ERROR_UNKNOWN;
            }

        dwoff = SetFilePointer(handle, file_size, NULL, FILE_BEGIN);
        if (dwoff == INVALID_SET_FILE_POINTER) {
            log_windows_error("SetFilePointer", GetLastError());
            CloseHandle(handle);
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (!SetEndOfFile(handle)) {
            log_windows_error("SetEndOfFile", GetLastError());
            CloseHandle(handle);
            return CAHUTE_ERROR_UNKNOWN;
        }

        dwoff = SetFilePointer(handle, 0, NULL, FILE_BEGIN);
        if (dwoff == INVALID_SET_FILE_POINTER) {
            log_windows_error("SetFilePointer", GetLastError());
            CloseHandle(handle);
            return CAHUTE_ERROR_UNKNOWN;
        }

        medium_type = CAHUTE_FILE_MEDIUM_WIN32;
        medium_state.windows.handle = handle;
    }
#else
    CAHUTE_RETURN_IMPL("No file opening method available.");
#endif

    /* Try to find the path extension.
     * If this method fails, the error is just ignored. */
    cahute_find_path_extension(extension, sizeof(extension), path, path_type);

    return open_file_from_medium(
        filep,
        medium_type,
        &medium_state,
        CAHUTE_FILE_MEDIUM_FLAG_READ | CAHUTE_FILE_MEDIUM_FLAG_WRITE
            | CAHUTE_FILE_MEDIUM_FLAG_SEEK | CAHUTE_FILE_MEDIUM_FLAG_SIZE,
        CAHUTE_FILE_FLAG_CLOSE_MEDIUM,
        file_size,
        extension
    );
}

/**
 * Open standard output as a file.
 *
 * @param filep Pointer to the file to create.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int) cahute_open_stdout(cahute_file **filep) {
    int medium_type;
    union cahute_file_medium_state medium_state;
    unsigned long file_flags = 0; /* By default, do not close the medium. */

#if defined(CAHUTE_FILE_MEDIUM_POSIX)
    medium_type = CAHUTE_FILE_MEDIUM_POSIX;
    medium_state.posix.fd = 1;
#elif defined(CAHUTE_FILE_MEDIUM_WIN32)
    {
        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

        if (handle == INVALID_HANDLE_VALUE) {
            log_windows_error("GetStdHandle", GetLastError());
            return CAHUTE_ERROR_UNKNOWN;
        }

        medium_type = CAHUTE_FILE_MEDIUM_WIN32;
        medium_state.windows.handle = handle;
    }
#else
    CAHUTE_RETURN_IMPL("No file opening method available.");
#endif

    return open_file_from_medium(
        filep,
        medium_type,
        &medium_state,
        CAHUTE_FILE_MEDIUM_FLAG_WRITE,
        file_flags,
        0,
        NULL
    );
}

/**
 * Close the file object.
 *
 * @param file File to close.
 */
CAHUTE_EXTERN(void) cahute_close_file(cahute_file *file) {
    if (!file)
        return;

    msg(ll_info, "Closing the file.");
    if (file->flags & CAHUTE_FILE_FLAG_CLOSE_MEDIUM)
        close_medium(file->medium.type, &file->medium.state);

    free(file);
}
