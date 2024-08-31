``<cahute/detection.h>`` -- Device detection for Cahute
=======================================================

This header declares device detection related utilities for Cahute.

Type definitions
----------------

.. c:struct:: cahute_serial_detection_entry

    Available serial port that can be opened using
    :c:func:`cahute_open_serial_link`.

    .. c:member:: char const *cahute_serial_detection_entry_name

        Null-terminated name or path of the serial port.

.. c:struct:: cahute_usb_detection_entry

    Available USB device that can be opened using
    :c:func:`cahute_open_usb_link`.

    .. c:member:: int cahute_usb_detection_entry_bus

        USB bus number of the entry.

    .. c:member:: int cahute_usb_detection_entry_address

        USB address number of the entry.

    .. c:member:: int cahute_usb_detection_entry_type

        Entry type, amongst the following:

        .. c:macro:: CAHUTE_USB_DETECTION_ENTRY_TYPE_CAS300

            The device is a Classpad 300 / 330 (+) or compatible calculator,
            using the CAS300 variant of the CASIOLINK protocol over
            bulk transfers.

        .. c:macro:: CAHUTE_USB_DETECTION_ENTRY_TYPE_SEVEN

            The device is an fx-9860G or compatible calculator, using
            Protocol 7.00 or Protocol 7.00 Screenstreaming over
            bulk transfers.

            Note that the fx-CG calculators sometimes use identify as an
            fx-9860G for OS updating or some types of screenstreaming.

            See :ref:`protocol-seven` and :ref:`protocol-seven-ohp`
            for more details.

        .. c:macro:: CAHUTE_USB_DETECTION_ENTRY_TYPE_SCSI

            The device is a USB Mass Storage device with proprietary
            extensions to communicate using Protocol 7.00 or
            Protocol 7.00 Screenstreaming.

            See :ref:`protocol-ums` for more details.

        See :ref:`usb-detection` for more information.

.. c:type:: int (*cahute_detect_serial_entry_func)(void *cookie, \
    cahute_serial_detection_entry const *entry)

    Function that can be called back with a serial detection entry.
    See :c:func:`cahute_detect_serial` for more information.

.. c:type:: int (*cahute_detect_usb_entry_func)(void *cookie, \
    cahute_usb_detection_entry const *entry)

    Function that can be called back with a USB detection entry.
    See :c:func:`cahute_detect_usb` for more information.

Function declarations
---------------------

.. c:function:: int cahute_detect_serial( \
    cahute_detect_serial_entry_func *func, void *cookie)

    Detect available serial devices.

    For every found entry, the provided function is called with its cookie
    and details regarding the serial entry, represented by its
    ``entry`` parameter of :c:type:`cahute_serial_detection_entry` type.

    If the callback returns a non-zero value, it signals the current function
    to stop and return the :c:macro:`CAHUTE_ERROR_INT` error.

    :param func: Function to call with every entry.
    :param cookie: Cookie to pass to the function.
    :return: The error, or 0 if the operation was successful.

.. c:function:: int cahute_detect_usb(cahute_detect_usb_entry_func *func, \
    void *cookie)

    Detect available USB devices.

    For every found entry, the provided function is called with its cookie
    and details regarding the USB entry, represented by its
    ``entry`` parameter of :c:type:`cahute_usb_detection_entry` type.

    If the callback returns a non-zero value, it signals the current function
    to stop and return the :c:macro:`CAHUTE_ERROR_INT` error.

    :param func: Function to call with every entry.
    :param cookie: Cookie to pass to the function.
    :return: The error, or 0 if the operation was successful.
