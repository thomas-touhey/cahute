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

#if UNIX_ENABLED
# include <dirent.h>

/**
 * Obtain the maximum path size on the current OS.
 *
 * @return Maximum path size.
 */
CAHUTE_INLINE(size_t) unix_path_max(void) {
    ssize_t path_max;

# ifdef PATH_MAX
    path_max = PATH_MAX;
# else
    path_max = pathconf("/", _PC_PATH_MAX);
    if (path_max <= 0)
        path_max = 4096;
# endif

    return (size_t)++path_max;
}

/**
 * Check if all characters in the strings are digits between 0 and 9.
 *
 * @param s String to check.
 * @return 1 if all characters are decimal digits, 0 otherwise. */
CAHUTE_INLINE(int) all_numbers(char const *s) {
    for (; *s; s++)
        if (*s < '0' || *s > '9')
            return 0;

    return 1;
}
#endif

/**
 * Detect serial entries available to Cahute.
 *
 * @param func User function to call back with every serial entry.
 * @param cookie Cookie to pass to the user function.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_detect_serial(
    cahute_detect_serial_entry_func CAHUTE_NNPTR(func),
    void *cookie
) CAHUTE_NONNULL(1) {
#if UNIX_ENABLED
    DIR *dp;
    struct dirent *dr;
    struct stat st;
    cahute_serial_detection_entry entry;
    size_t path_max = unix_path_max();
    char *fullbuf = malloc(path_max * 2);
    int err = CAHUTE_OK;

    if (!fullbuf)
        return CAHUTE_ERROR_ALLOC;

    /* First method: iterate over links in ``/dev/serial/by-id/``, which is
     * populated by a system udev rule if present (Linux-only). */
    dp = opendir("/dev/serial/by-id/");
    if (dp) {
        char *buf = fullbuf, *devbuf = &fullbuf[path_max];
        char *end = &buf[18];
        ssize_t rl;

        strcpy(buf, "/dev/serial/by-id/");

        while (1) {
            if (!(dr = readdir(dp)))
                break;

            /* The entry is expected to be a link.
             * We want to check that and, if it's the case, get the absolute
             * path of the linked serial device. */
            strcpy(end, dr->d_name);
            if (lstat(buf, &st) || (st.st_mode & S_IFMT) != S_IFLNK)
                continue;

            rl = readlink(buf, devbuf, path_max);
            if (rl < 0)
                continue;

            if (devbuf[0] != '/') {
                /* Device path is relative to the directory, concatenate the
                 * original device and return. */
                strcpy(end, devbuf);
                if (!(realpath(buf, devbuf)))
                    continue;
            }

            entry.cahute_serial_detection_entry_name = devbuf;
            if (func(cookie, &entry)) {
                err = CAHUTE_ERROR_INT;
                break;
            }
        }

        closedir(dp);
        free(fullbuf);

        return err;
    }

    /* Fallback method: iterate over known USB serial devices in "/dev"
     * directly:
     *
     * - On MacOS / OS X: "cu.*" (e.g. "cu.usbmodem621").
     * - On FreeBSD: "cuadX" (e.g. "cuad0").
     * - On Linux: "ttyUSBX" (e.g. "ttyUSB1"). */
    dp = opendir("/dev/");
    if (dp) {
        char *buf = fullbuf;
        char *end = &buf[5];

        strcpy(buf, "/dev/");

        while (1) {
            if (!(dr = readdir(dp)))
                break;

            if (strncmp(dr->d_name, "cu.", 3)
                && (strncmp(dr->d_name, "cuad", 4)
                    || !all_numbers(&dr->d_name[4]))
                && (strncmp(dr->d_name, "ttyUSB", 6)
                    || !all_numbers(&dr->d_name[6])))
                continue;

            strcpy(end, dr->d_name);
            entry.cahute_serial_detection_entry_name = buf;
            if (func(cookie, &entry)) {
                err = CAHUTE_ERROR_INT;
                break;
            }
        }

        closedir(dp);
        free(fullbuf);

        return err;
    }

    free(fullbuf);
#endif

#if WINDOWS_ENABLED
# define SERIAL_COMM_HKEY "HARDWARE\\DEVICEMAP\\SERIALCOMM"
    cahute_serial_detection_entry entry;
    char value[1024];
    BYTE data[1024];
    DWORD werr, i = 0;
    HKEY hkey;
    int err = CAHUTE_OK;

    msg(ll_info, "Opening HKEY_LOCAL_MACHINE\\" SERIAL_COMM_HKEY);
    if ((werr = RegOpenKey(HKEY_LOCAL_MACHINE, SERIAL_COMM_HKEY, &hkey))) {
        log_windows_error("RegOpenKey", werr);
        return CAHUTE_ERROR_UNKNOWN;
    }

    while (1) {
        DWORD data_size = 1023;
        DWORD value_size = 1023;
        DWORD entry_type;

        switch (werr = RegEnumValue(
                    hkey,
                    i++,
                    value,
                    &value_size,
                    NULL,
                    &entry_type,
                    data,
                    &data_size
                )) {
        case 0:
            break;

        case ERROR_NO_MORE_ITEMS:
            goto end;

        case ERROR_MORE_DATA:
            /* Our buffers weren't big enough. Tough luck!
                 * Let's go to the next entry and just ignore this one. */
            continue;

        default:
            log_windows_error("RegEnumValue", werr);
            RegCloseKey(hkey);
            return CAHUTE_ERROR_UNKNOWN;
        }

        /* We only want REG_SZ entries. */
        if (entry_type != REG_SZ)
            continue;

        /* The 'data' field contains what we expect! */
        data[data_size] = '\0';

        entry.cahute_serial_detection_entry_name = (char *)data;
        if (func(cookie, &entry)) {
            err = CAHUTE_ERROR_INT;
            break;
        }
    }

end:
    RegCloseKey(hkey);

    return err;
#endif

    return CAHUTE_ERROR_IMPL;
}

/**
 * Detect USB entries available to Cahute.
 *
 * @param func User function to call back with every USB entry.
 * @param cookie Cookie to pass to the user function.
 * @return Error, or CAHUTE_OK if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_detect_usb(
    cahute_detect_usb_entry_func CAHUTE_NNPTR(func),
    void *cookie
) CAHUTE_NONNULL(1) {
#if LIBUSB_ENABLED
    libusb_context *context = NULL;
    libusb_device **device_list = NULL;
    cahute_usb_detection_entry entry;
    int device_count, id, err = CAHUTE_OK;

    if (libusb_init(&context)) {
        msg(ll_fatal, "Could not create a libusb context.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    device_count = libusb_get_device_list(context, &device_list);
    if (device_count < 0) {
        msg(ll_fatal, "Could not get a device list.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    for (id = 0; id < device_count; id++) {
        struct libusb_device_descriptor device_descriptor;
        struct libusb_config_descriptor *config_descriptor;
        int interface_class = 0;

        if (libusb_get_device_descriptor(device_list[id], &device_descriptor))
            continue;

        if (device_descriptor.idVendor != 0x07cf
            || (device_descriptor.idProduct != 0x6101
                && device_descriptor.idProduct != 0x6102))
            continue;

        if (libusb_get_active_config_descriptor(
                device_list[id],
                &config_descriptor
            ))
            continue;

        if (config_descriptor->bNumInterfaces == 1
            && config_descriptor->interface[0].num_altsetting == 1)
            interface_class =
                config_descriptor->interface[0].altsetting[0].bInterfaceClass;

        libusb_free_config_descriptor(config_descriptor);

        if (interface_class == 8)
            entry.cahute_usb_detection_entry_type =
                CAHUTE_USB_DETECTION_ENTRY_TYPE_SCSI;
        else if (interface_class == 255)
            entry.cahute_usb_detection_entry_type =
                CAHUTE_USB_DETECTION_ENTRY_TYPE_SEVEN;
        else
            continue;

        entry.cahute_usb_detection_entry_bus =
            libusb_get_bus_number(device_list[id]);
        entry.cahute_usb_detection_entry_address =
            libusb_get_device_address(device_list[id]);

        if (func(cookie, &entry)) {
            err = CAHUTE_ERROR_INT;
            break;
        }
    }

    libusb_free_device_list(device_list, 1);
    libusb_exit(context);
    return err;
#else
    msg(ll_fatal, "No USB detection method enabled.");
    return CAHUTE_ERROR_IMPL;
#endif
}
