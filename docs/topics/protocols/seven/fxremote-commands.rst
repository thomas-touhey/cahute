Known Protocol 7.00 command extensions in fxRemote
==================================================

These commands are used by fxRemote's Update.EXE program.

All commands described in this document have custom payloads.

.. _seven-fxremote-command-70:

``70`` "Write data"
-------------------

Write data to an address.

This command uses a ``8 + DL`` bytes payload, where ``DL`` is the size of the
data to write. The format of the payload is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - Destination Address
      - Address on the bus to which to write.
      - Big endian 32-bit number, e.g. ``0x88030000``.
    * - 4 (0x04)
      - 4 B
      - Data Length (*DL*)
      - Length of the data to write.
      - Big endian 32-bit number, e.g. ``0x000003FC``.
    * - 8 (0x08)
      - *DL* B
      - Data
      - Data to write to the address.
      - Binary data.

This command is used in the following use cases:

* :ref:`seven-fxremote-flash`.

.. _seven-fxremote-command-71:

``71`` "Copy data"
------------------

Copy data from an address to another.

This command uses a 12 bytes payload, of which the format is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - Destination Address
      - Address on the bus to write to.
      - Big endian 32-bit number, e.g. ``0xA0020000``.
    * - 4 (0x04)
      - 4 B
      - Data Length
      - Length of the data to copy from the source to the destination address.
      - Big endian 32-bit number, e.g. ``0x00010000``.
    * - 8 (0x08)
      - 4 B
      - Source Address
      - Address on the bus to read from.
      - Big endian 32-bit number, e.g. ``0x88030000``.

This command is used in the following use cases:

* :ref:`seven-fxremote-flash`.

.. _seven-fxremote-command-72:

``72`` "Clear a flash sector"
-----------------------------

Clear a flash sector.

This command uses a 4 bytes payload, of which the format is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - Address
      - Address of the flash sector to clear.
      - Big endian 32-bit number, e.g. ``0xA0010000``.

This command is used in the following use cases:

* :ref:`seven-fxremote-flash`.

.. _seven-fxremote-command-76:

``76`` "Get special data"
-------------------------

Get the 44 bytes at address ``0xA000FFC0``.

This command takes no payload.

The response to this command is actually made of **two packets** (instead of
one):

* An acknowledgement packet, of basic subtype (see :ref:`seven-ack-packet`);
* A data packet, containing the provided data (see :ref:`seven-data-packet`).

Example data for an SH4 calculator is the following:

.. code-block:: text

    0000 0007 0000 0000  ........
    0000 0000 0000 0000  ........
    5870 4C55 4D4D 6567  XpLUMMeg
    0000 5A5A 0000 001F  ..ZZ....
    0000 DF1E 1030 0B00  .....0..
    0000 2C00            ..,.

.. _seven-fxremote-command-78:

``78`` "Terminate flash procedure"
----------------------------------

Terminate the flash procedure.

This command takes no payload.

This command is used in the following use cases:

* :ref:`seven-fxremote-flash`.
