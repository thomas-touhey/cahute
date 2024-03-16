``<cahute/link.h>`` -- Calculator link resource and methods for Cahute
======================================================================

This header declares link-related utilities for Cahute.

Type definitions
----------------

.. c:struct:: cahute_device_info

    Device information.

    .. c:member:: unsigned long cahute_device_info_flags

        Flags for the link information.

        .. c:macro:: CAHUTE_DEVICE_INFO_FLAG_PREPROG

            Preprogrammed ROM information available.

        .. c:macro:: CAHUTE_DEVICE_INFO_FLAG_BOOTCODE

            Bootcode information available.

        .. c:macro:: CAHUTE_DEVICE_INFO_FLAG_OS

            OS information available.

    .. c:member:: unsigned long cahute_device_info_rom_capacity

        Preprogrammed ROM capacity, in KiB.

        Only available if the :c:macro:`CAHUTE_DEVICE_INFO_FLAG_PREPROG`
        flag is set.

    .. c:member:: cahute_os_version cahute_device_info_rom_version

        Preprogrammed ROM version.

        Only available if the :c:macro:`CAHUTE_DEVICE_INFO_FLAG_PREPROG`
        flag is set.

    .. c:member:: unsigned long cahute_device_info_flash_rom_capacity

        Flash ROM capacity, in KiB.

    .. c:member:: unsigned long cahute_device_info_ram_capacity

        RAM capacity, in KiB.

    .. c:member:: cahute_version cahute_device_info_bootcode_version

        Bootcode version.

        Only available if the :c:macro:`CAHUTE_DEVICE_INFO_FLAG_BOOTCODE`
        flag is set.

    .. c:member:: unsigned long cahute_device_info_bootcode_offset

        Bootcode offset.

        Only available if the :c:macro:`CAHUTE_DEVICE_INFO_FLAG_BOOTCODE`
        flag is set.

    .. c:member:: unsigned long cahute_device_info_bootcode_size

        Bootcode size, in KiB.

        Only available if the :c:macro:`CAHUTE_DEVICE_INFO_FLAG_BOOTCODE`
        flag is set.

    .. c:member:: cahute_version cahute_device_info_os_version

        OS version.

        Only available if the :c:macro:`CAHUTE_DEVICE_INFO_FLAG_OS` flag
        is set.

    .. c:member:: unsigned long cahute_device_info_os_offset

        OS offset.

        Only available if the :c:macro:`CAHUTE_DEVICE_INFO_FLAG_OS` flag
        is set.

    .. c:member:: unsigned long cahute_device_info_os_size

        OS size, in KiB.

        Only available if the :c:macro:`CAHUTE_DEVICE_INFO_FLAG_OS` flag
        is set.

    .. c:member:: char const *cahute_device_info_product_id

        Null-terminated product identifier, up to 16 characters.

    .. c:member:: char const *cahute_device_info_username

        Null-terminated username, up to 20 characters long.

    .. c:member:: char const *cahute_device_info_organisation

        Null-terminated organisation, up to 20 characters long.

    .. c:member:: char const *cahute_device_info_hwid

        Null-terminated hardware identifier, up to 8 characters.

    .. c:member:: char const *cahute_device_info_cpuid

        Null-terminated hardware platform identifier, up to 16 characters.

.. c:struct:: cahute_frame

    Screenstreaming frame details for screenstreaming.

    .. c:member:: int cahute_frame_width

        Width of the frame, in pixels.

    .. c:member:: int cahute_frame_height

        Height of the frame, in pixels.

    .. c:member:: int cahute_frame_format

        Format of the frame, as a ``CAHUTE_PICTURE_FORMAT_*`` constant.
        See :ref:`header-cahute-picture` for more information.

    .. c:member:: cahute_u8 const *cahute_frame_data

        Frame contents encoded with the format described above.

.. c:struct:: cahute_storage_entry

    Entry when listing the contents of a storage device or directory.

    .. c:member:: char const *cahute_storage_entry_directory

        If the entry is a directory, the name of the directory.

        If the entry is a file, the optional name of the directory in
        which the file is present; this can be set to ``NULL`` if the
        file is present at root.

    .. c:member:: char const *cahute_storage_entry_name

        If the entry is a directory, this is set to ``NULL``.

        If the entry is a file, the file name.

    .. c:member:: unsigned long cahute_storage_entry_size

        Size in bytes of the file.

.. c:struct:: cahute_link

    Link to a calculator, that can be used to run operations on the
    calculator, or receive data such as screenstreaming data.

    This type is opaque, and such resources must be created using
    :c:func:`cahute_open_usb_link` or :c:func:`cahute_open_serial_link`.

.. c:type:: int (cahute_process_frame_func)(void *cookie, \
    cahute_frame const *frame)

    Function that can be called when a frame has been received in a
    screenstreaming mode.

    See :c:func:`cahute_receive_screen` for more information.

.. c:type:: int (cahute_confirm_overwrite_func)(void *cookie)

    Function that can be called to confirm overwrite.

    See :c:func:`cahute_send_file_to_storage` for more information.

.. c:type:: int (cahute_list_storage_entry_func)(void *cookie, \
    cahute_storage_list_entry const *entry)

    Function that can be called for every storage device entry.

    See :c:func:`cahute_list_storage_entries` for more information.

.. c:type:: int (cahute_progress_func)(void *cookie, unsigned long step,\
    unsigned long total)

    Function that can be called to display progress, when step ``step`` out
    of ``total`` has just finished.

    See :c:func:`cahute_send_file_to_storage` and
    :c:func:`cahute_request_file_from_storage` for more information.

Function declarations
---------------------

.. c:function:: int cahute_open_serial_link(cahute_link **linkp, \
    unsigned long flags, char const *name, unsigned long speed)

    Open a link over a serial modem.

    .. warning::

        In case of error, the value of ``*linkp`` mustn't be used nor freed.

    Since serial links do not offer any metadata, the protocol to use on the
    serial link must be selected manually, amongst the following:

    .. c:macro:: CAHUTE_SERIAL_PROTOCOL_SEVEN

        Use Protocol 7.00.

        See :ref:`protocol-seven` for more information.

    Since the number of stop bits may be selectable on the calculator, it
    can also be selected manually, amongst the following:

    .. c:macro:: CAHUTE_SERIAL_STOP_ONE

        Use 1 stop bit.

    .. c:macro:: CAHUTE_SERIAL_STOP_TWO

        Use 2 stop bits (*by default*).

    Since the parity may also be selectable on the calculator, it can also
    be selected manually, amongst the following:

    .. c:macro:: CAHUTE_SERIAL_PARITY_OFF

        Disable parity checks (*by default*).

    .. c:macro:: CAHUTE_SERIAL_PARITY_EVEN

        Use even parity checks.

    .. c:macro:: CAHUTE_SERIAL_PARITY_ODD

        Use odd parity checks.

    .. note::

        fx-9860G calculators and derivatives, i.e. the ones you will be the
        most likely to encounter, use protocol 7.00 with 2 stop bits and
        no parity when establishing a new connection.

        The LINK app on such calculators does not allow to change these
        settings, and the only way for the link to use different settings
        is if the connection has already been established and command
        :ref:`seven-command-02` was issued to change the serial link
        parameters for the current connection.

    If the device uses XON/XOFF software control, it can also be selected
    manually, amongst the following:

    .. c:macro:: CAHUTE_SERIAL_XONXOFF_DISABLE

        Disable XON/XOFF software control (*by default*).

    .. c:macro:: CAHUTE_SERIAL_XONXOFF_ENABLE

        Enable XON/XOFF software control.

    If the device uses DTR and the cable supports it, it can be selected
    manually as well, amongst the following:

    .. c:macro:: CAHUTE_SERIAL_DTR_DISABLE

        Disable DTR (*by default*).

    .. c:macro:: CAHUTE_SERIAL_DTR_ENABLE

        Enable DTR.

    .. c:macro:: CAHUTE_SERIAL_DTR_HANDSHAKE

        Enable DTR, and require a handshake to be done.

    If the device uses RTS and the cable supports it, it can also be
    selected manually, amongst the following:

    .. c:macro:: CAHUTE_SERIAL_RTS_DISABLE

        Disable RTS (*by default*).

    .. c:macro:: CAHUTE_SERIAL_RTS_ENABLE

        Enable RTS.

    .. c:macro:: CAHUTE_SERIAL_RTS_HANDSHAKE

        Enable RTS, and require a handshake to be done.

    Protocol-specific behaviour can be tweaked using the following flags:

    .. c:macro:: CAHUTE_SERIAL_NOCHECK

        If this flag is provided, the initial handshake will not be
        done when the link is established on the underlying medium.

        This flag is mostly useful when resuming a connection initiated by
        another process, or when the passive process does not require or
        implement the initial handshake.

        It is only effective when using protocol 7.00.
        See :ref:`protocol-seven` for more information.

    .. c:macro:: CAHUTE_SERIAL_NODISC

        If this flag is provided, command :ref:`seven-command-01` is
        not issued once the link is established to get the device information.

        This flag is mostly useful when dealing with bootcode or custom
        link implementations that may not have implemented this command.
        It is not recommended when communicating with the LINK application
        since it enables Cahute to predict which commands will be
        unavailable without crashing the link.

        It is only effective when using protocol 7.00.
        See :ref:`protocol-seven` for more information.

    .. c:macro:: CAHUTE_SERIAL_NOTERM

        If this flag is provided, the terminate flow is not run when
        the link is closed.

        This flag is mostly useful to let the connection still opened for
        other processes to run commands. Combined with
        :c:macro:`CAHUTE_SERIAL_NOCHECK`, it allows running multiple shell
        commands on the same connection.

        It is only effective when using protocol 7.00.
        See :ref:`protocol-seven` for more information.

    .. c:macro:: CAHUTE_SERIAL_OHP

        If this flag is provided, the calculator is assumed to use the link
        for screenstreaming purposes.

        For example, with the fx-9860G and compatible, this prompts the link
        to use Protocol 7.00 Screenstreaming instead of Protocol 7.00.

        See :ref:`protocol-seven-ohp` for more information.

    :param linkp: The pointer to set the opened link to.
    :param flags: The flags to set to the serial link.
    :param name: The name or path of the serial link to open.
    :param speed: The speed (in bauds) to open the serial link with, or ``0``
        to select the default serial speed.
    :return: The error, or 0 if the operation was successful.

.. c:function:: int cahute_open_usb_link(cahute_link **linkp, \
    unsigned long flags, int bus, int address)

    Open a link with a USB device.

    .. warning::

        In case of error, the value of ``*linkp`` mustn't be used nor freed.

    The protocol to use is determined using the USB device metadata.
    See :ref:`usb-detection` for more information.

    Protocol-specific behaviour can be tweaked using the following flags:

    .. c:macro:: CAHUTE_USB_NOCHECK

        If this flag is provided, the initial handshake will not be
        done when the link is established on the underlying medium.

        This flag is mostly useful when resuming a connection initiated by
        another process, or when the passive process does not require or
        implement the initial handshake.

        It is only effective when using protocol 7.00.
        See :ref:`protocol-seven` for more information.

    .. c:macro:: CAHUTE_USB_NODISC

        If this flag is provided, command :ref:`seven-command-01` is
        not issued once the link is established to get the device information.

        This flag is mostly useful when dealing with bootcode or custom
        link implementations that may not have implemented this command.
        It is not recommended when communicating with the LINK application
        since it enables Cahute to predict which commands will be
        unavailable without crashing the link.

        It is only effective when using protocol 7.00.
        See :ref:`protocol-seven` for more information.

    .. c:macro:: CAHUTE_USB_NOTERM

        If this flag is provided, the terminate flow is not run when
        the link is closed.

        This flag is mostly useful to let the connection still opened for
        other processes to run commands. Combined with
        :c:macro:`CAHUTE_USB_NOCHECK`, it allows running multiple shell
        commands on the same connection.

        It is only effective when using protocol 7.00.
        See :ref:`protocol-seven` for more information.

    .. c:macro:: CAHUTE_USB_OHP

        If this flag is provided, the calculator is assumed to use the link
        for screenstreaming purposes.

        For example, with the fx-9860G and compatible, this prompts the link
        to use Protocol 7.00 Screenstreaming instead of Protocol 7.00.

        See :ref:`protocol-seven-ohp` for more information.

    :param linkp: The pointer to set the opened link to.
    :param flags: The flags to set to the serial link.
    :param bus: The bus number of the USB calculator to open a link with.
    :param address: The device number of the calculator to open a link with.
    :return: The error, or 0 if the operation was successful.

.. c:function:: void cahute_close_link(cahute_link *link)

    Close and free a link.

    :param link: The link to close.

.. c:function:: int cahute_get_device_info(cahute_link *link, \
    cahute_device_info **infop)

    Gather information on the device (calculator or other).

    .. warning::

        In all cases, ``*infop`` **musn't be freed**.
        In case of error, ``*infop`` mustn't be used.

    :param link: The link on which to gather information.
    :param infop: The pointer to set to the information to.
    :return: The error, or 0 if the operation was successful.

.. c:function:: int cahute_negotiate_serial_params(cahute_link *link, \
    unsigned long flags, unsigned long speed)

    Negotiate new parameters for a serial link, and update the medium
    parameters to these.

    The accepted flags here are among ``CAHUTE_SERIAL_STOP_*`` and
    ``CAHUTE_SERIAL_PARITY_*``.

    :param link: Link to which to define the new attributes.
    :param flags: New flags to set to the serial link.
    :param speed: New speed to set to the serial link.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_receive_screen(cahute_link *link, \
    cahute_process_frame_func *callback, void *cookie)

    Receive screen continuously, using screenstreaming, and call the provided
    function for every received frame.

    If the provided function returns any non-zero value, the process is
    interrupted.

    :param link: Link with which to receive screen frames.
    :param callback: Function to call for every received frame.
    :param cookie: Cookie to provide to the function on every call.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_request_storage_capacity(cahute_link *link, \
    char const *storage, unsigned long *capacityp)

    Request the currently available capacity on the provided storage device.

    If this function returns an error, the contents of ``*capacityp`` is
    left intact, and may be undefined.

    :param link: Link to the device.
    :param storage: Name of the storage device for which to get the
        currently available capacity.
    :param capacityp: Pointer to the capacity to fill.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_send_file_to_storage(cahute_link *link, \
    unsigned long flags, char const *directory, char const *name, \
    char const *storage, FILE *filep, \
    cahute_confirm_overwrite_func *overwrite_func, void *overwrite_cookie, \
    cahute_progress_func *progress_func, void *progress_cookie)

    Send a file to a storage device on the calculator.

    .. note::

        The provided ``filep`` parameter will be used for reading and
        **seeking**, in order to estimate the file size.

    If an overwrite confirmation function is provided and the calculator
    requests confirmation for overwriting, said function is called. Then:

    * If the function returns ``0``, the overwrite is rejected, therefore
      the file is not sent.
    * If the function returns any other value, the overwrite is confirmed,
      and the function transfers the file over.

    Flags that can be set to this function are the following:

    .. c:macro:: CAHUTE_SEND_FILE_FLAG_FORCE

        If this flag is set, overwrite is forced without using the
        callback functions.

    .. c:macro:: CAHUTE_SEND_FILE_FLAG_OPTIMIZE

        If this flag is set, the available capacity in the targeted storage
        is requested. If it is not considered enough to store the file, an
        optimize command will be issued beforehand.

    See :ref:`seven-send-file-to-storage` for the use case with Protocol 7.00.

    :param link: Link to the device.
    :param flags: Flags for the function.
    :param directory: Name of the directory in which to place the file,
        or ``NULL`` if the file should be placed at root.
    :param name: Name of the file in the target storage device.
    :param storage: Name of the storage device on which to place the file.
    :param filep: Standard FILE pointer to read file data and estimate
        file size from.
    :param overwrite_func: Pointer to the overwrite function to call.
        If this is set to ``NULL``, the overwrite will be systematically
        rejected if requested by the calculator.
    :param overwrite_cookie: Cookie to pass to the overwrite
        confirmation function.
    :param progress_func: Pointer to the optional progress function to call
        once for every step in the transfer process.
    :param progress_cookie: Cookie to pass to the progress function.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_request_file_from_storage(cahute_link *link, \
    char const *directory, char const *name, char const *storage, \
    FILE *filep, cahute_progress_func *progress_func, void *progress_cookie)

    Request a file from a storage device on the calculator.

    See :ref:`seven-request-file-from-storage` for the use case with
    Protocol 7.00.

    :param link: Link to the device.
    :param directory: Name of the directory from which to request the file,
        or ``NULL`` if the file should be placed at root.
    :param name: Name of the file to request.
    :param storage: Name of the storage device from which to request the file.
    :param filep: Standard FILE pointer to write file data.
    :param progress_func: Pointer to the optional progress function to call
        once for every step in the transfer process.
    :param progress_cookie: Cookie to pass to the progress function.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_copy_file_on_storage(cahute_link *link, \
    char const *source_directory, char const *source_name, \
    char const *target_directory, char const *target_name, char const *storage)

    Copy a file on a storage device on the calculator.

    See :ref:`seven-copy-file-on-storage` for the use case with
    Protocol 7.00.

    :param link: Link to the device.
    :param source_directory: Name of the directory in which to retrieve
        the source file.
    :param source_name: Name of the source file to copy.
    :param target_directory: Name of the directory in which to create
        the copy.
    :param target_name: Name of the copy.
    :param storage: Name of the storage device on which to copy.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_delete_file_from_storage(cahute_link *link, \
    char const *directory, char const *name, char const *storage)

    Delete a file from a storage device on the calculator.

    See :ref:`seven-delete-file-on-storage` for the use case with
    Protocol 7.00.

    :param link: Link to the device.
    :param directory: Name of the directory from which to delete the file,
        or ``NULL`` if the file should be placed at root.
    :param name: Name of the file to delete.
    :param storage: Name of the storage device from which to delete the file.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_list_storage_entries(cahute_link *link, \
    char const *storage, cahute_list_storage_entry_func *callback, \
    void *cookie)

    List files and directories on a storage device on the calculator.

    For every entry, the callback function is called. If it returns a value
    other than ``0``, the file listing is interrupted.

    See :ref:`seven-list-files-on-storage` for the use case with
    Protocol 7.00.

    :param link: Link to the device.
    :param storage: Storage on which to list files and directories.
    :param func: Function to call back with every found entry.
    :param cookie: Cookie to pass to the function.
    :return: Error, or 0 if the operation was unsuccessful.

.. c:function:: int cahute_reset_storage(cahute_link *link, \
    char const *storage)

    Request a reset of the provided storage device by the calculator.

    See :ref:`seven-reset-storage` for the use case with Protocol 7.00.

    :param link: Link to the device.
    :param storage: Name of the storage device.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_optimize_storage(cahute_link *link, \
    char const *storage)

    Request optimization for the provided storage device by the calculator.

    See :ref:`seven-optimize-storage` for the use case with Protocol 7.00.

    :param link: Link to the device.
    :param storage: Name of the storage device.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_upload_and_run_program(cahute_link *link, \
    cahute_u8 const *program, size_t program_size, \
    unsigned long load_address, unsigned long start_address, \
    cahute_progress_func *progress_func, void *progress_cookie)

    Upload and run a program on the calculator.

    See :ref:`seven-upload-and-run` for the use case with Protocol 7.00.

    :param link: Link to the device.
    :param program: Pointer to the program to upload and run.
    :param program_size: Size of the program to upload and run.
    :param load_address: Remote address at which to load the program.
    :param start_address: Remote address at which to start the program.
    :param progress_func: Pointer to the optional progress function to call
        once for every step in the transfer process.
    :param progress_cookie: Cookie to pass to the progress function.
    :return: Error, or 0 if the operation was successful.
