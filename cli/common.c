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

#include "common.h"
#include <stdio.h>
#include <string.h>

/**
 * Cookie for the USB finder.
 *
 * @property should_display Whether the program should display the options
 *           if multiple are found.
 * @property multiple Boolean set to 1 if multiple devices were already
 *           detected.
 * @property last_bus Bus of the last found device, -1 if uninitialized.
 * @property last_address Address of the last found device, -1 if
 *           uninitialized.
 * @property last_type Type of the last found device, -1 if uninitialized.
 */
struct usb_device_finder_cookie {
    int should_display;
    int multiple;
    int last_bus;
    int last_address;
    int last_type;
};

/**
 * USB finder callback.
 *
 * @param cookie Cookie of the USB device finder.
 */
static int find_usb_device(
    struct usb_device_finder_cookie *cookie,
    cahute_usb_detection_entry const *entry
) {
    if (cookie->last_bus >= 0) {
        if (!cookie->multiple && cookie->should_display) {
            fprintf(stderr, "Multiple USB calculators were found:\n");
            fprintf(
                stderr,
                "  Bus %03d Device %03d: %s\n",
                cookie->last_bus,
                cookie->last_address,
                cookie->last_type == CAHUTE_USB_DETECTION_ENTRY_TYPE_SEVEN
                    ? "fx-9860G compatible calculator (Protocol 7.00)"
                    : "fx-CG compatible calculator (SCSI)"
            );
        }

        cookie->multiple = 1;
        if (cookie->should_display) {
            fprintf(
                stderr,
                "  Bus %03d Device %03d: %s\n",
                entry->cahute_usb_detection_entry_bus,
                entry->cahute_usb_detection_entry_address,
                entry->cahute_usb_detection_entry_type
                        == CAHUTE_USB_DETECTION_ENTRY_TYPE_SEVEN
                    ? "fx-9860G compatible calculator (Protocol 7.00)"
                    : "fx-CG compatible calculator (SCSI)"
            );
        }
    }

    cookie->last_bus = entry->cahute_usb_detection_entry_bus;
    cookie->last_address = entry->cahute_usb_detection_entry_address;
    cookie->last_type = entry->cahute_usb_detection_entry_type;

    return 0;
}

/**
 * Find out the USB bus and address of the only USB device.
 *
 * @param should_display Whether the program should display the options, if
 *        multiple are present.
 * @param busp Pointer to the bus to define.
 * @param addressp Pointer to the address to define.
 * @return Cahute error (> 0), -1 if multiple devices were found, or 0 if ok.
 */
extern int find_usb_calculator(int should_display, int *busp, int *addressp) {
    struct usb_device_finder_cookie cookie;
    int err;

    cookie.should_display = should_display;
    cookie.last_bus = -1;
    cookie.last_address = -1;
    cookie.last_type = -1;
    cookie.multiple = 0;

    err = cahute_detect_usb(
        (cahute_detect_usb_entry_func *)find_usb_device,
        &cookie
    );
    if (err)
        return err;

    if (cookie.last_bus < 0)
        return CAHUTE_ERROR_NOT_FOUND;
    if (cookie.multiple)
        return CAHUTE_ERROR_TOO_MANY;

    *busp = cookie.last_bus;
    *addressp = cookie.last_address;
    return 0;
}

/**
 * Get the current logging level as a string.
 *
 * @return Logging level name.
 */
extern char const *get_current_log_level(void) {
    int loglevel = cahute_get_log_level();

    switch (loglevel) {
    case CAHUTE_LOGLEVEL_INFO:
        return "info";
    case CAHUTE_LOGLEVEL_WARNING:
        return "warning";
    case CAHUTE_LOGLEVEL_ERROR:
        return "error";
    case CAHUTE_LOGLEVEL_FATAL:
        return "fatal";
    default:
        return "(none)";
    }
}

/**
 * Set the current logging level as a string.
 *
 * @param loglevel Name of the loglevel to set.
 */
extern void set_log_level(char const *loglevel) {
    int value = CAHUTE_LOGLEVEL_NONE;

    if (!strcmp(loglevel, "info"))
        value = CAHUTE_LOGLEVEL_INFO;
    else if (!strcmp(loglevel, "warning"))
        value = CAHUTE_LOGLEVEL_WARNING;
    else if (!strcmp(loglevel, "error"))
        value = CAHUTE_LOGLEVEL_ERROR;
    else if (!strcmp(loglevel, "fatal"))
        value = CAHUTE_LOGLEVEL_FATAL;

    cahute_set_log_level(value);
}
