CAS50 packet format
===================

All CAS50 packets are introduced by a single byte, which defines the basic
purpose of the packet and the format and size of the payload following it.

.. list-table::
    :header-rows: 1

    * - Value
      - Name
      - Description
      - Payload
    * - ``0x06``
      - ``ACKNOWLEDGE``
      - The receiver accepts the data packet, confirms overwrite, or the
        sender confirms overwrite.
      - *(none)*
    * - ``0x13``
      - ``ESTABLISHED``
      - The receiver acknowledges connection establishment.
      - *(none)*
    * - ``0x15``
      - ``ABORT``
      - The sender requests an interactive connection, or aborts overwrite
        if an overwrite confirmation is requested.
      - *(none)*
    * - ``0x16``
      - ``START``
      - The sender requests a non-interactive connection.
      - *(none)*
    * - ``0x21``
      - ``ALREADY_EXISTS``
      - The receiver requests overwrite confirmation from the sender.
      - *(none)*
    * - ``0x24`` (or ``0x00``)
      - ``INVALID_DATA_TYPE``
      - The receiver rejects the data packet due to an invalid data type,
        or because it is out of memory.
      - *(none)*
    * - ``0x2b``
      - ``INVALID_CHECKSUM``
      - The received rejects the packet due to an invalid checksum.
        The transfer aborts.
      - *(none)*
    * - ``0x3a``
      - ``DATA``
      - Data payload.
      - :ref:`cas50-data-packets`
    * - ``0x51``
      - ``INVALID_DATA``
      - The receiver aborts the transfer due to errors in the header.
      - *(none)*

.. _cas50-data-packets:

Data packet payload
-------------------

Data packet payloads, with the notable exception of the type (*T*) field which
is specific to the protocol, mirror the internal format in the calculator's
main memory, including the header.

The format of the payload header is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - Type (*T*)
      - Basic purpose of the packet
      - ``END\0``
    * - 2 (0x02)
      - 2 B
      - File Type (*FT*)
      - File type, used by ``TXT`` packets.
      - ``PG``
    * - 4 (0x04)
      - 4 B
      - File Size (*FS*)
      - Size of the data accompanying the header (big endian).
      - ``\x00\x00\x00\xFF``
    * - 8 (0x08)
      - 8 B
      - File Name (*FN*)
      - Name of the file, with unset bytes being set to ``\xFF``.
      - ``HELLO\xFF\xFF\xFF``
    * - 16 (0x10)
      - 8 B
      - Alternative File Type (*AFT*)
      - Alternative file type used for some packet types, notably variables.
      - ``VariableR\x0A``
    * - 24 (0x18)
      - 8 B
      - File Password (*FP*)
      - Password of the file, with unset bytes being set to ``\xFF``.
      - ``WORLD\xFF\xFF\xFF``
    * - 32 (0x20)
      - 2 B
      - Base, if the file is a program.
      - ``BN`` for Base programs, ``NL`` otherwise.
      - ``BN``
    * - 34 (0x22)
      - 6 B
      - Backup Size (*BS*) *(?)*
      - Size of the backup (big endian).
      - ``\x00\x10\x00\x00\x00\x00``
    * - 40 (0x28)
      - 6 B
      - (unknown)
      - Unknown, filled with ``\xFF``.
      - ``\xFF\xFF\xFF\xFF\xFF\xFF``
    * - 48 (0x30)
      - 2 B
      - Checksum (*CS*)
      - Checksum (big endian).
      - ``\x12\x34``

Note that any field not used by the packet type should be set to ``\xFF``.

``END\xFF`` packets
~~~~~~~~~~~~~~~~~~~

.. todo:: Describe the packet's role.

All fields other than the type aren't used, and should be set to ``\xFF``.

``TXT\0`` packets
~~~~~~~~~~~~~~~~~

Such packets carry over a main memory textual file.

.. list-table::
    :header-rows: 1

    * - Subtype (*ST*) value
      - Description
    * - ``PG``
      - Program.

``IMG\0`` packets
~~~~~~~~~~~~~~~~~

Such packets carry over a main memory picture file.

.. list-table::
    :header-rows: 1

    * - Subtype (*ST*) value
      - Description
    * - ``PC``
      - Picture.

``MEM\0`` packets
~~~~~~~~~~~~~~~~~

Such packets carry over a backup.

.. list-table::
    :header-rows: 1

    * - Subtype (*ST*) value
      - Description
    * - ``BU``
      - Backup.

``VAL\0`` packets
~~~~~~~~~~~~~~~~~

Such packets carry over a variable. Particularities for this packet are:

* *FN* should be set to the variable name (?).
* *AFT* should be set to ``VariableR\x0A``.
