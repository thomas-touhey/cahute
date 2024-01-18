.. _ums-custom-commands:

Custom SCSI commands for CASIO's USB Mass Storage implementation
================================================================

CASIO makes use of the ``C0h`` to ``FFh`` `vendor-specific range`_ to
implement its own SCSI commands.

.. _ums-command-c0:

``0xC0`` -- Poll status
-----------------------

This command is a 16-byte command that is run to poll the device's status.
The command format is the following:

.. list-table::

    * - Offset
      - Size
      - Description
      - Value
    * - 0 (0x00)
      - 1 B
      - Command code.
      - ``0xC0``
    * - 1 (0x01)
      - 15 B
      - Reserved.
      - Must be set to ``0x00``.

The command prompts the device to answer with a 16-byte device status,
with the following format:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Description
      - Value
    * - 0 (0x00)
      - 1 B
      - Unknown.
      - Set to ``0xD0``.
    * - 1 (0x01)
      - 5 B
      - Reserved.
      - Set to ``0x00``.
    * - 6 (0x06)
      - 2 B
      - Amount of available bytes to be requested.
      - Big endian 16-bit integer.
    * - 8 (0x08)
      - 2 B
      - Reserved.
      - Set to ``0x00``.
    * - 10 (0x0A)
      - 2 B
      - Activity status.
      - Big endian 16-bit integer.
    * - 12 (0x0C)
      - 4 B
      - Reserved.
      - Set to ``0x00``.

.. _ums-command-c1:

``0xC1`` -- Request available data
----------------------------------

This command is a 16-byte long command that is run to read available data.
The command format is the following:

.. list-table::

    * - Offset
      - Size
      - Description
      - Value
    * - 0 (0x00)
      - 1 B
      - Command code.
      - ``0xC1``
    * - 1 (0x01)
      - 5 B
      - Reserved.
      - Set to ``0x00``.
    * - 6 (0x06)
      - 2 B
      - Requested bytes count.
      - Big endian 16-bit integer.
    * - 8 (0x08)
      - 8 B
      - Reserved.
      - Set to ``0x00``.

The command prompts the device to answer with the requested data.

.. _ums-command-c2:

``0xC2`` -- Send data
---------------------

This command is a 16-byte long command that is run to write data to the
calculator. The command format is the following:

.. list-table::

    * - Offset
      - Size
      - Description
      - Value
    * - 0 (0x00)
      - 1 B
      - Command code.
      - ``0xC2``
    * - 1 (0x01)
      - 5 B
      - Reserved.
      - Set to ``0x00``.
    * - 6 (0x06)
      - 2 B
      - Bytes count.
      - Big endian 16-bit integer.
    * - 8 (0x08)
      - 8 B
      - Reserved.
      - Set to ``0x00``.

The command should be accompanied with the data to send.

.. _Vendor-specific range: https://en.wikipedia.org/wiki/SCSI_command#SCSI_command_lengths
