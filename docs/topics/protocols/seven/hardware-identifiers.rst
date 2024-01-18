.. _seven-hardware-identifiers:

Known hardware identifiers for Protocol 7.00
============================================

All known Protocol 7.00 environments support command :ref:`seven-command-01`.
Several people have used this command to obtain information using their
calculator in different link modes; this document inventories the results.

Bootcodes
---------

.. list-table::
    :header-rows: 1

    * - Hardware identifier
      - Devices
      - Link
    * - ``Gy362000``
      - fx-7400GII, fx-9750G(II), fx-9860GII SD
      - yes
    * - ``Gy363000``
      - fx-7400GII-2, fx-9750GII-2, fx-9860G(II(-2)), fx-9860G slim,
        fx-9860GII-2, Graph 35+E, Graph 75+E
      - no
    * - ``Gy490000``
      - fx-7400GII-2
      - yes

Bootcodes do **not** support restart-related commands :ref:`seven-command-00`
and :ref:`seven-command-06`.
Bootcodes with ``Link`` support command :ref:`seven-command-02`.

Devices
-------

.. list-table::
    :header-rows: 1

    * - Hardware identifier
      - Devices
      - Backup
    * - ``Gy362006``
      - fx-9750GII, Graph 35+ (SH3)
      - no
    * - ``Gy362007``
      - fx-9750GII-2, Graph 35+ (SH4), Graph 35+E (OS < 2.05)
      - no
    * - ``Gy36200F``
      - Graph 35+E
      - no
    * - ``Gy363006``
      - fx-9860GII, Graph 75 (SH3)
      - yes
    * - ``Gy363007``
      - fx-9860GII-2, Graph 75 (SH4), Graph 95 (SH4)
      - no
    * - ``Gy36300F``
      - Graph 75+E
      - no
    * - ``Ly755000``
      - fx-CG20, Prizm
      - no

All devices support main memory (:ref:`seven-command-20` to
:ref:`seven-command-33`) and reset related commands (:ref:`seven-command-00`
and :ref:`seven-command-06`).

If there is at least one filesystem, flash commands
(:ref:`seven-command-40` to :ref:`seven-command-4E`, and
:ref:`seven-command-51`) are supported.
Two calculators that have different filesystems may have the same hardware
identifier, so it cannot be used for this (e.g. Graph 75 and Graph 95, the
latter having an SD port).

Devices with ``Backup`` support backup commands (:ref:`seven-command-4F`,
:ref:`seven-command-50`, and :ref:`seven-command-52` to
:ref:`seven-command-55`).
