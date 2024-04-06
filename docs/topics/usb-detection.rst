.. _usb-detection:

USB detection for CASIO calculators
===================================

When looking for CASIO calculators, the following vendor and product
identifiers (VID/PID) can be used:

.. list-table::
    :header-rows: 1

    * - Vendor ID
      - Product ID
      - Interface class
      - Description
    * - ``07cf``
      - ``6101``
      - ``255`` (Vendor-Specific)
      - Graph 35+/75/85/95, fx-9860G Slim, Classpad 300
    * - ``07cf``
      - ``6102``
      - ``8`` (Mass Storage)
      - Classpad 330+, fx-CG20, fx-CP400, fx-CP400+E

The interface class ``255`` is used when the device presents protocol 7.00
directly, while the interface class ``8`` is used in "USB key" mode, i.e.
when the file system and main memory are presented using SCSI.

.. note::

    For reference, the following USB serial cables have also be encountered
    in the wild:

    .. list-table::
        :header-rows: 1

        * - Vendor ID
          - Product ID
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

It can be detected by checking if the device driver, using device
property key |DEVPKEY_Device_Driver|_ or ``SPDRP_DRIVER``, starts with
``{36fc9e60-c465-11cf-8056-444553540000}``.

.. |DEVPKEY_Device_Driver| replace:: ``DEVPKEY_Device_Driver``

.. _FTDI: https://ftdichip.com/
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
