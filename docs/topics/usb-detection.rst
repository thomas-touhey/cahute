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

.. _FTDI: https://ftdichip.com/
