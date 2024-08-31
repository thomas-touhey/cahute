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

#ifndef CAHUTE_DETECTION_H
#define CAHUTE_DETECTION_H 1
#include "cdefs.h"

CAHUTE_BEGIN_NAMESPACE
CAHUTE_BEGIN_DECLS

CAHUTE_DECLARE_TYPE(cahute_serial_detection_entry)
CAHUTE_DECLARE_TYPE(cahute_usb_detection_entry)

struct cahute_serial_detection_entry {
    char const *cahute_serial_detection_entry_name;
};

#define CAHUTE_USB_DETECTION_ENTRY_TYPE_SEVEN 1
#define CAHUTE_USB_DETECTION_ENTRY_TYPE_SCSI  2

struct cahute_usb_detection_entry {
    int cahute_usb_detection_entry_bus;
    int cahute_usb_detection_entry_address;
    int cahute_usb_detection_entry_type;
};

typedef int(cahute_detect_serial_entry_func)(
    void *cahute__cookie,
    cahute_serial_detection_entry const *cahute__entry
);

typedef int(cahute_detect_usb_entry_func)(
    void *cahute__cookie,
    cahute_usb_detection_entry const *cahute__entry
);

/* ---
 * List available devices.
 * --- */

CAHUTE_EXTERN(int)
cahute_detect_serial(
    cahute_detect_serial_entry_func CAHUTE_NNPTR(cahute__func),
    void *cahute__cookie
) CAHUTE_NONNULL(1);

CAHUTE_EXTERN(int)
cahute_detect_usb(
    cahute_detect_usb_entry_func CAHUTE_NNPTR(cahute__func),
    void *cahute__cookie
) CAHUTE_NONNULL(1);

CAHUTE_END_DECLS
CAHUTE_END_NAMESPACE

#endif /* CAHUTE_DETECTION_H */
