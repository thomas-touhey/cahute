Links and medium internals
==========================

This document describes the internals behind links and mediums; see
:ref:`topic-links` for more information.

A link only requires one memory allocation (except for system resources that
are allocated / opened using different functions), and the medium
and the protocol are initialized together using link opening functions.

Mediums
-------

Mediums define a common set of interfaces that can be used by protocols to
communicate with the device or host.

A medium is represented by the following type:

.. c:struct:: cahute_link_medium

    Link medium representation.

    This structure is usually directly allocated with the link, i.e.
    :c:struct:`cahute_link` instance, and is accessed through ``link->medium``.

Medium interface
~~~~~~~~~~~~~~~~

Most mediums support a stream-like interface with the following functions:

.. c:function:: int cahute_receive_on_link_medium(cahute_link_medium *medium, \
    cahute_u8 *buf, size_t size, unsigned long first_timeout, \
    unsigned long next_timeout)

    Read exactly ``size`` bytes of data into the buffer.

    This function uses the medium read buffer to store any incoming excess
    data, for it to be processed first next time before using the underlying
    buffer to read more data.

    .. warning::

        This function does not provide the number of bytes that have been
        read in case of error (with the exception of
        :c:macro:`CAHUTE_ERROR_TIMEOUT_START`, which implies that no bytes
        have been read).

        This is to simplify as much as possible protocol
        implementations, but it also means that the medium should be
        considered irrecoverable in such cases.

    Errors to be expected from this function are the following:

    :c:macro:`CAHUTE_ERROR_TIMEOUT_START`
        The first byte was not received in a timely manner.
        This can only occur if ``first_timeout`` was not set to 0.

    :c:macro:`CAHUTE_ERROR_TIMEOUT`
        A byte past the first one was not received in a timely manner.
        This can only occur if ``next_timeout`` was not set to 0.

    :c:macro:`CAHUTE_ERROR_GONE`
        The device is no longer present, usually either because the USB
        cable has been unplugged on one end or the other, or the serial
        adapter has been unplugged from the host.

    :c:macro:`CAHUTE_ERROR_UNKNOWN`
        The medium-specific operations have yielded an error code that
        Cahute did not interpret. Some details can usually be found in
        the logs.

    :param medium: Link medium to receive data from.
    :param buf: Buffer in which to write the received data.
    :param size: Size of the data to write to the buffer.
    :param first_timeout: Maximum delay to wait before the first byte of the
        data, in milliseconds. If this is set to 0, the first byte will be
        awaited indefinitely.
    :param next_timeout: Maximum delay to wait between two bytes of the data,
        or before the last byte, in milliseconds. If this is set to 0, next
        bytes will be awaited indefinitely.
    :return: Error, or :c:macro:`CAHUTE_OK`.

.. c:function:: int cahute_send_on_link_medium(cahute_link_medium *medium, \
    cahute_u8 const *buf, size_t size)

    Write exactly ``size`` bytes of data to the link.

    Errors to be expected from this function are the following:

    :c:macro:`CAHUTE_ERROR_GONE`
        The device is no longer present, usually either because the USB
        cable has been unplugged on one end or the other, or the serial
        adapter has been unplugged from the host.

    :c:macro:`CAHUTE_ERROR_UNKNOWN`
        The medium-specific operations have yielded an error code that
        Cahute did not interpret. Some details can usually be found in
        the logs.

    :param medium: Link medium to send data to.
    :param buf: Buffer from which to read the data to send.
    :param size: Size of the data to read and send.
    :return: Error, or :c:macro:`CAHUTE_OK`.

Serial mediums such as :c:macro:`CAHUTE_LINK_MEDIUM_POSIX_SERIAL` or
:c:macro:`CAHUTE_LINK_MEDIUM_WIN32_SERIAL` support changing the parameters
of the serial link using the following function:

.. c:function:: int cahute_set_serial_params_to_link_medium( \
    cahute_link_medium *medium, unsigned long flags, unsigned long speed)

    Set the serial parameters to the medium.

    Accepted flags are a subset of the flags for :c:func:`cahute_open_serial`:

    * ``CAHUTE_SERIAL_STOP_*`` (stop bits);
    * ``CAHUTE_SERIAL_PARITY_*`` (parity);
    * ``CAHUTE_SERIAL_XONXOFF_*`` (XON/XOFF software control);
    * ``CAHUTE_SERIAL_DTR_*`` (DTR hardware control);
    * ``CAHUTE_SERIAL_RTS_*`` (RTS hardware control).

    :param medium: Link medium to set the serial parameters to.
    :param flags: Flags to set to the medium.
    :param speed: Speed to set to the medium.
    :return: Error, or :c:macro:`CAHUTE_OK`.

USB Mass Storage mediums support an interface capable of making SCSI requests,
with the following functions:

.. c:function:: int cahute_scsi_request_to_link_medium( \
    cahute_link_medium *medium, cahute_u8 const *command,\
    size_t command_size, cahute_u8 const *data, size_t data_size, int *statusp)

    Emit an SCSI request to the medium, with or without data.

    :param medium: Link medium to send the command and optional payload to,
        and receive the status from.
    :param command: Command to send.
    :param command_size: Size of the command to send.
    :param data: Optional data to send along with the command.
        This can be set to ``NULL`` if ``data_size`` is set to 0.
    :param data_size: Size of the data to send along with the command.
    :param statusp: Pointer to the status code to set to the one returned by
        the device.
    :return: Error, or :c:macro:`CAHUTE_OK`.

.. c:function:: int cahute_scsi_request_from_link_medium( \
    cahute_link_medium *medium, cahute_u8 const *command, \
    size_t command_size, cahute_u8 *buf, size_t buf_size, int *statusp)

    Emit an SCSI request to the medium, while requesting data.

    :param medium: Link medium to send the command to, and receive the data
        and status from.
    :param command: Command to send.
    :param command_size: Size of the command to send.
    :param buf: Buffer to fill with the requested data.
    :param buf_size: Size of the data to request.
    :param statusp: Pointer to the status code to set to the one returned by
        the device.
    :return: Error, or :c:macro:`CAHUTE_OK`.

Available medium types
~~~~~~~~~~~~~~~~~~~~~~

Medium types are represented as ``CAHUTE_LINK_MEDIUM_*`` constants internally.

.. warning::

    The medium constants are only represented **if they are available on the
    current configuration**. This is a simple way for medium-specific
    implementations to be defined or not, with ``#ifdef``.

Available mediums are the following:

.. c:macro:: CAHUTE_LINK_MEDIUM_POSIX_SERIAL

    Serial medium using the POSIX STREAMS API, with a file descriptor (*fd*):

    * Closing using `close(2) <https://linux.die.net/man/2/close>`_;
    * Receiving uses `select(2) <https://linux.die.net/man/2/select>`_ and
      `read(2) <https://linux.die.net/man/2/read>`_;
    * Sending uses `write(2) <https://linux.die.net/man/2/write>`_;
    * Serial params setting uses
      `termios(3) <https://linux.die.net/man/3/termios>`_, including
      ``tcdrain()``, and
      `tty_ioctl(4) <https://linux.die.net/man/4/tty_ioctl>`_, especially
      ``TIOCMGET`` and ``TIOCMSET``.

    Only available on platforms considered POSIX, including Apple's OS X
    explicitely (since they do not define the ``__unix__`` constant like
    Linux does).

    Available protocols on this medium are the following:

    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_AUTO`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_NONE`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP`.

.. c:macro:: CAHUTE_LINK_MEDIUM_AMIGAOS_SERIAL

    Serial medium using AmigaOS serial I/O, as described in the
    `AmigaOS Serial Device Guide`_.

    Available protocols on this medium are the following:

    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_AUTO`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_NONE`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP`.

.. c:macro:: CAHUTE_LINK_MEDIUM_WIN32_SERIAL

    Serial medium using the Windows API, with a |HANDLE|_ and
    `Overlapped I/O`_:

    * Closing uses |CloseHandle|_;
    * Receiving uses |ReadFile|_ and |WaitForSingleObject|_, and depending
      on whether the second function succeeded or not, either
      |GetOverlappedResult|_ or |CancelIo|_, to ensure we don't have any
      buffer writes post-freeing the link;
    * Sending uses |WriteFile|_ and |WaitForSingleObject|_, and depending
      on whether the second function succeeded or not, either
      |GetOverlappedResult|_ or |CancelIo|_, to ensure we don't have any
      buffer reads post-freeing the link;
    * Serial params setting uses |SetCommState|_.

    For more information, see `Serial Communications in Win32`_.

    Available protocols on this medium are the following:

    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_AUTO`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_NONE`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP`.

.. c:macro:: CAHUTE_LINK_MEDIUM_WIN32_CESG

    USB device used as a host through CASIO's CESG502 driver using the
    Windows API.

    As described in :ref:`usb-detection-windows`, we must detect if the
    device driver is CESG502 or a libusb-compatible driver by using
    SetupAPI_ or CfgMgr32_, and use this medium in the first case.

    It is used with a |HANDLE|_ and `Overlapped I/O`_:

    * Closing uses |CloseHandle|_;
    * Receiving uses |ReadFile|_ and |WaitForSingleObject|_, and depending
      on whether the second function succeeded or not, either
      |GetOverlappedResult|_ or |CancelIo|_, to ensure we don't have any
      buffer writes post-freeing the link;
    * Sending uses |WriteFile|_ and |WaitForSingleObject|_, and depending
      on whether the second function succeeded or not, either
      |GetOverlappedResult|_ or |CancelIo|_, to ensure we don't have any
      buffer reads post-freeing the link.

    Note that CESG502 waits for calculator input by default, and always
    requires a buffer bigger than the actual input it receives (4 KiB is
    usually enough). It also abstracts away whether it using bulk transfers
    directly, or USB Mass Storage, into a stream interface; this however
    does not allow you to make SCSI requests directly.

    Available protocols on this medium are the following:

    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_NONE`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_CASIOLINK`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_SEVEN`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP`.

.. c:macro:: CAHUTE_LINK_MEDIUM_WIN32_UMS

    USB Mass Storage device used as a host using the Windows API.

    It is used with a |HANDLE|_:

    * Closing uses |CloseHandle|_;
    * Requesting using SCSI uses |DeviceIoControl|_ with
      |IOCTL_SCSI_PASS_THROUGH_DIRECT|_.

    Available protocols on this medium are the following:

    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_NONE`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_MASS_STORAGE`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP`.

.. c:macro:: CAHUTE_LINK_MEDIUM_LIBUSB

    USB device used as a host through libusb, with bulk transport.

    It is used with a |libusb_device_handle|_, opened using a
    |libusb_context|_:

    * Closing uses |libusb_close|_ on the device handle, and |libusb_exit|_
      on the libusb context;
    * Receiving and sending uses |libusb_bulk_transfer|_.

    Available protocols on this medium are the following:

    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_NONE`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_CASIOLINK`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_SEVEN`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP`.

.. c:macro:: CAHUTE_LINK_MEDIUM_LIBUSB_UMS

    USB device used as a host through libusb, implementing USB Mass Storage
    (UMS) with Bulk-only transport.

    As for :c:macro:`CAHUTE_LINK_MEDIUM_LIBUSB`, it is used with a
    |libusb_device_handle|_, opened using a |libusb_context|_:

    * Closing uses |libusb_close|_ on the device handle, and |libusb_exit|_
      on the libusb context;
    * Requesting using SCSI uses |libusb_bulk_transfer|_ with manual reading
      and writing of the Command Block Wrapper (CBW) and
      Command Status Wrapper (CSW).

    See `USB Mass Storage Class, Bulk-Only Transport`_ for more information
    on CBW and CSW format and protocol in general.

    Available protocols on this medium are the following:

    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_NONE`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_MASS_STORAGE`;
    * :c:macro:`CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP`.

Protocols
---------

Protocols define what operations and logics are available, and how to
implement these operations and logics.

All protocols may use the **data buffer**, which is in the link directly,
which serves at storing raw data or screen data received using the protocol.

Available protocols are:

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_AUTO

    Automatic protocol detection on a serial medium.

    Note that this doesn't outlive link protocol initialization, and gets
    replaced by the actual protocol afterwards; see
    :ref:`internals-link-protocol-initialization` for more details.

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_NONE

    No protocol on a serial medium.

    This can be selected by the user in order to use the medium functions
    more directly, through the ones referenced in
    :ref:`header-cahute-link-medium`.

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK

    CASIOLINK protocol over a serial medium.

    See :ref:`protocol-casiolink` for more information.

    Note that in this case, the CASIOLINK variant is set in the
    ``protocol_state.casiolink.variant`` property of the link.

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN

    Protocol 7.00 over a serial medium.

    See :ref:`protocol-seven` for more information.

    This differs from :c:macro:`CAHUTE_LINK_PROTOCOL_USB_SEVEN` by the
    availability of command :ref:`seven-command-02`.

.. c:macro:: CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP

    Protocol 7.00 Screenstreaming over a serial medium.

    See :ref:`protocol-seven-ohp` for more information.

.. c:macro:: CAHUTE_LINK_PROTOCOL_USB_NONE

    No protocol on a USB medium.

    This can be selected by the user in order to use the medium functions
    more directly, through the ones referenced in
    :ref:`header-cahute-link-medium`.

.. c:macro:: CAHUTE_LINK_PROTOCOL_USB_CASIOLINK

    CASIOLINK over USB bulk transport.

    See :ref:`protocol-casiolink` for more information.

.. c:macro:: CAHUTE_LINK_PROTOCOL_USB_SEVEN

    Protocol 7.00 over USB bulk transport or USB Mass Storage or
    USB Mass Storage commands.

    See :ref:`protocol-seven` and :ref:`protocol-ums` for more information.

.. c:macro:: CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP

    Protocol 7.00 Screenstreaming over USB bulk transport or USB Mass Storage
    extended commands.

    See :ref:`protocol-seven-ohp` and :ref:`protocol-ums` for more information.

.. c:macro:: CAHUTE_LINK_PROTOCOL_USB_MASS_STORAGE

    USB Mass Storage without extensions.

.. _internals-link-open:

Opening behaviours
------------------

In this section, we will describe the behaviour of link opening functions.

:c:func:`cahute_open_serial_link`
    This function first validates all params to ensure compatibility, e.g.
    throws an error in case of unsupported flag, speed, or combination.

    .. note::

        The protocol is selected, depending on the flags, to one of the
        following:

        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_AUTO`;
        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_NONE`;
        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_CASIOLINK`;
        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN`;
        * :c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_SEVEN_OHP`.

    Then, depending on the platform:

    * On POSIX and compatible, it will attempt to open the serial device
      using `open(2) <https://linux.die.net/man/2/open>`_.
      If this succeeds, the medium of the created link will be set to
      :c:macro:`CAHUTE_LINK_MEDIUM_POSIX_SERIAL`;
    * On Windows, it will attempt to open the serial device using
      |CreateFile|_, then, if it succeeds, call |SetCommTimeouts|_
      with ``ReadTimeoutInterval`` set to ``MAXDWORD`` in order to only read
      what is directly available, and create the event for the overlapped
      object using |CreateEvent|_. If this succeeds, the medium of the
      created link will be set to :c:macro:`CAHUTE_LINK_MEDIUM_WIN32_SERIAL`;
    * Otherwise, it will return :c:macro:`CAHUTE_ERROR_IMPL`.

    If the underlying medium has successfully been opened, it will allocate
    the link and call :c:func:`cahute_set_serial_params_to_link` to set
    the initial serial parameters to it.

    It will then initialize the protocol using the common protocol
    initialization procedure; see
    :ref:`internals-link-protocol-initialization`.

:c:func:`cahute_open_usb_link`
    This function first validates all params to ensure compatibility, e.g.
    throws an error in case of unsupported flag or combination.

    If libusb support has been disabled, the function returns
    :c:macro:`CAHUTE_ERROR_IMPL`.

    Otherwise, on all platforms, this function creates a context using
    |libusb_init|_, gets the device list using |libusb_get_device_list|_,
    and finds one matching the provided bus and address numbers using
    |libusb_get_bus_number|_ and |libusb_get_device_address|_ on every entry.

    If a matching device is found, the configuration is obtained using
    |libusb_get_device_descriptor|_ and |libusb_get_active_config_descriptor|_,
    in order to:

    * Get the vendor (VID) and product (PID) identifiers, to ensure they match
      one of the known combinations for CASIO calculators.
    * Get the interface class (``bInterfaceClass``) to determine the protocol
      and medium type.
    * In both cases, ensure that the bulk IN and OUT endpoints exist, and
      get their endpoint identifiers.

    .. note::

        While historical implementations of CASIO's protocols using libusb
        hardcode 0x82 as Bulk IN and 0x01 as Bulk OUT, this has proven to
        change on other platforms such as OS X; see `#3 (comment 1823215641)
        <https://gitlab.com/cahuteproject/cahute/-/issues/3#note_1823215641>`_
        for more context.

    The interface class and :c:macro:`CAHUTE_USB_OHP` flag presence to
    protocol and medium type mapping is the following:

    .. list-table::
        :header-rows: 1
        :width: 100%

        * - (in) Intf. class
          - (in) ``bcdUSB``
          - (in) ``OHP`` flag
          - (out) Medium
          - (out) Protocol
        * - 8
          -
          - absent
          - :c:macro:`CAHUTE_LINK_MEDIUM_LIBUSB_UMS`
          - :c:macro:`CAHUTE_LINK_PROTOCOL_USB_MASS_STORAGE`
        * - 8
          -
          - present
          - :c:macro:`CAHUTE_LINK_MEDIUM_LIBUSB_UMS`
          - :c:macro:`CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP`
        * - 255
          - ``0x100``
          - present
          - **FAIL**
          - **FAIL**
        * - 255
          - ``0x100``
          - absent
          - :c:macro:`CAHUTE_LINK_MEDIUM_LIBUSB`
          - :c:macro:`CAHUTE_LINK_PROTOCOL_USB_CASIOLINK` w/
            :c:macro:`CAHUTE_CASIOLINK_VARIANT_CAS300`
        * - 255
          -
          - absent
          - :c:macro:`CAHUTE_LINK_MEDIUM_LIBUSB`
          - :c:macro:`CAHUTE_LINK_PROTOCOL_USB_SEVEN`
        * - 255
          -
          - present
          - :c:macro:`CAHUTE_LINK_MEDIUM_LIBUSB`
          - :c:macro:`CAHUTE_LINK_PROTOCOL_USB_SEVEN_OHP`

    See :ref:`usb-detection` for more information.

    .. warning::

        If :c:macro:`CAHUTE_USB_NOPROTO` flag is passed, the medium is kept,
        but the protocol is replaced by
        :c:macro:`CAHUTE_LINK_PROTOCOL_USB_NONE`.

    Once all metadata has been gathered, the function opens the device using
    |libusb_open|_, and attempt to claim its interface using
    |libusb_claim_interface|_ and |libusb_detach_kernel_driver|_.

    .. note::

        Access errors, i.e. any of these two functions returning
        ``LIBUSB_ERROR_ACCESS``, are ignored, since libusb is still
        able to communicate with the device on some platforms afterwards.

        See `#3 <https://gitlab.com/cahuteproject/cahute/-/issues/3>`_
        for more context.

    If the device opening yields ``LIBUSB_ERROR_NOT_SUPPORTED``,
    it means that the device is running a driver that is not supported by
    libusb.

        On Windows, in this case, we look for a USB device with a device
        address equal to the libusb port number, obtained using
        |libusb_get_port_number|_, then:

        * If the underlying driver to the device is identified as CESG502,
          we use the USB device interface as a
          :c:macro:`CAHUTE_LINK_MEDIUM_WIN32_CESG` medium;
        * Otherwise, we look for disk drive then volume devices via bus
          relations, and use the volume device interface as a
          :c:macro:`CAHUTE_LINK_MEDIUM_WIN32_UMS` medium.

    Once all is done, the link is created with the selected medium and
    protocol. The function will then initialize the protocol using the
    common protocol initialization procedure; see
    :ref:`internals-link-protocol-initialization`.

:c:func:`cahute_open_simple_usb_link`
    This function is a convenience function, using mostly public functions
    to work:

    * It detects available USB devices using :c:func:`cahute_detect_usb`.
      It only picks USB devices matching the provided filter(s) which,
      if non-zero, act as an accepted device type mask, i.e.:

      * If :c:macro:`CAHUTE_USB_FILTER_CAS300` is set, devices identifying
        as a Classpad 300 / 330 (+) are accepted;
      * If :c:macro:`CAHUTE_USB_FILTER_SEVEN` is set, devices identifying
        as a Protocol 7.00 / Protocol 7.00 Screenstreaming device are
        accepted;
      * If :c:macro:`CAHUTE_USB_FILTER_UMS` is set, devices identifying
        as an USB Mass Storage speaking calculator are accepted.

      If this function finds no matching devices, it sleeps and retries until
      it has no attempts left. If it finds multiple, it fails with error
      :c:macro:`CAHUTE_ERROR_TOO_MANY`.
    * It opens the found USB device using :c:func:`cahute_open_usb_link`.

    It used to be to the program or library to define by itself, and was in
    the guides, but this behaviour is found in most simple scripts that
    use the Cahute library, so it was decided to include it within the library.

.. _internals-link-protocol-initialization:

Protocol initialization
~~~~~~~~~~~~~~~~~~~~~~~

The common protocol initialization procedure is defined by a function named
``init_link`` in ``link/open.c``.

First of all, if the selected protocol is
:c:macro:`CAHUTE_LINK_PROTOCOL_SERIAL_AUTO`, the communication initialization
is used to determine the protocol in which both devices should communicate.

.. note::

    Since the initialization step is necessary for automatic protocol
    discovery to take place, the :c:macro:`CAHUTE_SERIAL_NOCHECK` flag
    is forbidden with :c:macro:`CAHUTE_SERIAL_PROTOCOL_AUTO`.
    This is described in :c:func:`cahute_open_serial_link`'s flags description.

Then, the initialization sequence is run depending on the protocol and role
(sender or receiver, depending on the presence of the
:c:macro:`CAHUTE_SERIAL_RECEIVER` :c:macro:`CAHUTE_USB_RECEIVER` in the flags
of the original function).

.. |HANDLE| replace:: ``HANDLE``
.. |CreateFile| replace:: ``CreateFile``
.. |SetCommTimeouts| replace:: ``SetCommTimeouts``
.. |CreateEvent| replace:: ``CreateEvent``
.. |ReadFile| replace:: ``ReadFile``
.. |WriteFile| replace:: ``WriteFile``
.. |WaitForSingleObject| replace:: ``WaitForSingleObject``
.. |GetOverlappedResult| replace:: ``GetOverlappedResult``
.. |CancelIo| replace:: ``CancelIo``
.. |CloseHandle| replace:: ``CloseHandle``
.. |SetCommState| replace:: ``SetCommState``
.. |DeviceIoControl| replace:: ``DeviceIoControl``
.. |IOCTL_SCSI_PASS_THROUGH_DIRECT| replace:: ``IOCTL_SCSI_PASS_THROUGH_DIRECT``

.. |libusb_context| replace:: ``libusb_context``
.. |libusb_init| replace:: ``libusb_init``
.. |libusb_exit| replace:: ``libusb_exit``
.. |libusb_device_handle| replace:: ``libusb_device_handle``
.. |libusb_get_device_list| replace:: ``libusb_get_device_list``
.. |libusb_get_bus_number| replace:: ``libusb_get_bus_number``
.. |libusb_get_device_address| replace:: ``libusb_get_device_address``
.. |libusb_get_device_descriptor| replace:: ``libusb_get_device_descriptor``
.. |libusb_get_port_number| replace:: ``libusb_get_port_number``
.. |libusb_get_active_config_descriptor|
   replace:: ``libusb_get_active_config_descriptor``
.. |libusb_detach_kernel_driver| replace:: ``libusb_detach_kernel_driver``
.. |libusb_claim_interface| replace:: ``libusb_claim_interface``
.. |libusb_open| replace:: ``libusb_open``
.. |libusb_close| replace:: ``libusb_close``
.. |libusb_bulk_transfer| replace:: ``libusb_bulk_transfer``

.. _HANDLE:
    https://learn.microsoft.com/en-us/windows/win32/sysinfo/handles-and-objects
.. _Overlapped I/O:
    https://learn.microsoft.com/en-us/windows/win32/sync/
    synchronization-and-overlapped-input-and-output
.. _CreateFile:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-createfilea
.. _SetCommTimeouts:
    https://learn.microsoft.com/en-us/windows/win32/api/
    winbase/nf-winbase-setcommtimeouts
.. _CreateEvent:
    https://learn.microsoft.com/en-us/windows/win32/api/
    synchapi/nf-synchapi-createeventa
.. _ReadFile:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-readfile
.. _WriteFile:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-writefile
.. _WaitForSingleObject:
    https://learn.microsoft.com/en-us/windows/win32/api/
    synchapi/nf-synchapi-waitforsingleobject
.. _GetOverlappedResult:
    https://learn.microsoft.com/en-us/windows/win32/api/
    ioapiset/nf-ioapiset-getoverlappedresult
.. _CancelIo:
    https://learn.microsoft.com/en-us/windows/win32/fileio/cancelio
.. _CloseHandle:
    https://learn.microsoft.com/en-us/windows/win32/api/
    handleapi/nf-handleapi-closehandle
.. _SetCommState:
    https://learn.microsoft.com/en-us/windows/win32/api/
    winbase/nf-winbase-setcommstate
.. _DeviceIoControl:
    https://learn.microsoft.com/en-us/windows/win32/api/
    ioapiset/nf-ioapiset-deviceiocontrol
.. _IOCTL_SCSI_PASS_THROUGH_DIRECT:
    https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddscsi/
    ni-ntddscsi-ioctl_scsi_pass_through_direct
.. _Serial Communications in Win32:
    https://learn.microsoft.com/en-us/previous-versions/ms810467(v=msdn.10)

.. _SetupAPI:
    https://learn.microsoft.com/en-us/windows-hardware/drivers/install/setupapi
.. _cfgmgr32:
    https://learn.microsoft.com/en-us/windows/win32/api/cfgmgr32/

.. _libusb_context:
    https://libusb.sourceforge.io/api-1.0/group__libusb__lib.html
    #ga4ec088aa7b79c4a9599e39bf36a72833
.. _libusb_init:
    https://libusb.sourceforge.io/api-1.0/group__libusb__lib.html
    #ga7deaef521cfb1a5b3f8d6c01be11a795
.. _libusb_exit:
    https://libusb.sourceforge.io/api-1.0/group__libusb__lib.html
    #gadc174de608932caeb2fc15d94fa0844d
.. _libusb_device_handle:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #ga7df95821d20d27b5597f1d783749d6a4
.. _libusb_get_device_list:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #gac0fe4b65914c5ed036e6cbec61cb0b97
.. _libusb_get_bus_number:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #gaf2718609d50c8ded2704e4051b3d2925
.. _libusb_get_device_address:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #gab6d4e39ac483ebaeb108f2954715305d
.. _libusb_get_port_number:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #ga14879a0ea7daccdcddb68852d86c00c4
.. _libusb_get_device_descriptor:
    https://libusb.sourceforge.io/api-1.0/group__libusb__desc.html
    #ga5e9ab08d490a7704cf3a9b0439f16f00
.. _libusb_get_active_config_descriptor:
    https://libusb.sourceforge.io/api-1.0/group__libusb__desc.html
    #ga425885149172b53b3975a07629c8dab3
.. _libusb_detach_kernel_driver:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #ga5e0cc1d666097e915748593effdc634a
.. _libusb_claim_interface:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #gaee5076addf5de77c7962138397fd5b1a
.. _libusb_open:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #ga3f184a8be4488a767b2e0ae07e76d1b0
.. _libusb_close:
    https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html
    #ga779bc4f1316bdb0ac383bddbd538620e
.. _libusb_bulk_transfer:
    https://libusb.sourceforge.io/api-1.0/group__libusb__syncio.html
    #ga2f90957ccc1285475ae96ad2ceb1f58c

.. _AmigaOS Serial Device Guide:
    https://wiki.amigaos.net/wiki/Serial_Device
.. _USB Mass Storage Class, Bulk-Only Transport:
    https://www.usb.org/sites/default/files/usbmassbulk_10.pdf
