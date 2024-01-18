.. _protocol-ums:

USB Mass Storage (UMS) and proprietary extensions for fx-CG calculators
=======================================================================

In order to behave like any other USB-attached storage with their "USB Key"
communications mode, fx-CG calculators implement `USB Mass Storage`_ (UMS)
with `Bulk-Only Transport`_.

The following sections describe CASIO's usage of UMS in this context.

.. note::

    Extensions to the SCSI protocol embedded within CASIO's UMS implementation
    still support a stream interface for communicating using Protocol 7.00
    or Protocol 7.00 Screenstreaming over UMS; see
    :ref:`ums-custom-commands` for more information.

.. toctree::
    :maxdepth: 1

    ums/custom-commands

.. _USB Mass Storage: https://en.wikipedia.org/wiki/USB_mass_storage_device_class
.. _Bulk-Only Transport: https://www.usb.org/sites/default/files/usbmassbulk_10.pdf
