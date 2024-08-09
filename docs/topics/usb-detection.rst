.. _usb-detection:

USB detection for CASIO calculators
===================================

When looking for CASIO calculators, the following vendor and product
identifiers (VID/PID) can be used:

.. list-table::
    :header-rows: 1

    * - ``idVendor``
      - ``idProduct``
      - ``bInterfaceClass``
      - ``bInterfaceSubclass``
      - ``bInterfaceProtocol``
      - Description
    * - ``07cf``
      - ``6101``
      - ``255`` (Vendor-Specific)
      - ``0``
      - ``0``
      - Graph 35+/75/85/95, fx-9860G Slim, Classpad 300
    * - ``07cf``
      - ``6102``
      - ``8`` (Mass Storage)
      - ``0``
      - ``0``
      - Classpad 330+, fx-CG20, fx-CP400, fx-CP400+E

The interface class ``255`` is used when the device presents protocol 7.00
directly, while the interface class ``8`` is used in "USB key" mode, i.e.
when the file system and main memory are presented using SCSI.

.. warning::

    Some older fx-9860G derivatives using OS 1.x require a specific USB control
    transfer to be run before Protocol 7.00 can be used; see
    :ref:`seven-init-link` for more information.

.. note::

    For reference, the following USB serial cables have also be encountered
    in the wild:

    .. list-table::
        :header-rows: 1

        * - ``idVendor``
          - ``idProduct``
          - Description
        * - ``0711``
          - ``0230``
          - SB-88 serial cable (official CASIO cable).
        * - ``0bda``
          - ``5606``
          - Util-Pocket (defunct alternative vendor) serial cable.
            Uses an USB serial converter from FTDI_.

    Note however that these should be used through the system's serial
    bus interface rather than directly.

.. _usb-device-enabling:

Vendor-specific USB device enabling
-----------------------------------

Over USB links, some older fx-9860G derivatives, such as the fx-9860G Slim
running OS 1.x, require a special USB control flow to be executed before they
can send or receive any data. This can manifest differently depending on the
application protocol:

* With Protocol 7.00, the calculator will not answer the initial check.
* With Protocol 7.00 Screenstreaming, the calculator will freeze until the
  control flow is made, since it is attempting to send screen data.

This control flow has the following properties:

* ``bmRequestType`` set to ``0x41``, to designate a vendor-specific
  interface request with no incoming data transfer;
* ``bRequest`` set to ``0x01``, as it is the command that enables
  Protocol 7.00 data transfers.
* Both ``wValue`` and ``wIndex`` set to ``0x0000``.
* No data transfer.

Using libusb_, this can be done using the following excerpt:

.. code-block:: c

    libusb_control_transfer(
        device_handle,
        0x41,  /* bmRequestType */
        0x01,  /* bRequest */
        0x0000,  /* wValue */
        0x0000,  /* wIndex */
        NULL,
        0,
        300
    );

Ideally, this flow is run by a driver that can be used as soon as the
calculator is connected to the host. Otherwise, it means that the calculator
may freeze until a transfer utility is used, such as one of Cahute's
command-line utilities.

.. _usb-detection-windows:

Driver detection on Microsoft Windows
-------------------------------------

Any user program using the Windows API (Win32) requires a kernel driver to
communicate with the calculator through USB. This kernel driver can be one
of:

* CASIO's **CESG502** driver, which supports both bulk transport (fx-9860G) and
  transport using proprietary UMS commands (fx-CP, fx-CG).
* A `libusb-compatible kernel driver`_, including WinUSB.

CESG502 is distributed with `FA-124`_, and is necessary for CASIO's software
to successfully detect and communicate calculators connected using USB.
This means it is necessary for any user program that co-exists with it
to work with CASIO's driver.

It can be detected using libusb, but cannot be opened using the same tool;
one must use detection with SetupAPI_ or cfgmgr32_, check that the device
driver is CESG502, and if it's the case, open and use the device using
fileapi_ (``CreateFile``, ``ReadFile``, ``WriteFile``, ``CloseFile``).

.. note::

    It is possible to access device instance properties on Windows OSes
    before Vista, e.g. Windows XP; see `Accessing Device Instance Properties
    (Prior to Windows Vista)`_ for more information.

It uses ``{36fc9e60-c465-11cf-8056-444553540000}``, the same GUID as
generic USB devices, which is normally forbidden for Independent
Hardware Vendors (IHV) such as CASIO, so **this key cannot be used to
uniquely identify the driver**.

Cahute currently matches the service (``CM_DRP_SERVICE``) to ``PVUSB``,
since this is the value encountered in the wild.

.. |DEVPKEY_Device_Driver| replace:: ``DEVPKEY_Device_Driver``

.. _FTDI: https://ftdichip.com/
.. _libusb: https://libusb.info/
.. _libusb-compatible kernel driver:
    https://github.com/libusb/libusb/wiki/
    Windows#user-content-Driver_Installation
.. _SetupAPI:
    https://learn.microsoft.com/en-us/windows-hardware/drivers/install/setupapi
.. _cfgmgr32:
    https://learn.microsoft.com/en-us/windows/win32/api/cfgmgr32/
.. _fileapi: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/
.. _DEVPKEY_Device_Driver:
    https://learn.microsoft.com/en-us/windows-hardware/drivers/install/
    devpkey-device-driver
.. _FA-124:
    https://www.planet-casio.com/Fr/logiciels/voir_un_logiciel_casio.php
    ?showid=16
.. _Accessing Device Instance Properties (Prior to Windows Vista):
    https://learn.microsoft.com/en-us/windows-hardware/drivers/install/
    accessing-device-instance-spdrp-xxx-properties
