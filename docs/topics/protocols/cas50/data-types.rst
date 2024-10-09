.. _cas50-data-types:

CAS50 data types
================

Data in the CAS50 protocol is composed a header, followed by data parts,
all prefixed with ``0x3A``.

Headers are always 50 bytes long, and of the following format:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 1 B
      - Prefix
      - Header prefix.
      - ``\x3A``
    * - 1 (0x01)
      - 4 B
      - Format (*FMT*)
      - Format of the following data packets.
      -
    * - 5 (0x05)
      - 44 B
      - Format-specific data
      -
      - See the type description for the format of this component.
        If the type-specific data is less than 44 bytes, the rest is filled
        with ``\xFF``.
    * - 49 (0x31)
      - 1 B
      - Checksum (*CS*)
      -
      - ``\x12``

The type-specific data, and count, size and format of data packets sent after
the header depend on the format (*FMT*).

See the following subsections for more information.

.. _cas50-header-end:

``END\xFF`` End
---------------

This header represents an end of sequence. It is only used with the CASIOLINK
protocol, when using the CAS50 header format.

This is not followed by any data parts.

.. _cas50-header-fnc:

``FNC\0`` Function
------------------

.. todo:: Describe this.

.. _cas50-header-img:

``IMG\0`` Image
---------------

.. todo:: Describe this more.

.. _cas50-header-mem:

``MEM\0`` Memory Dump
---------------------

Such packets carry over a memory dump.

Format-specific data for this format is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Data Type (*DT*)
      - Data type, presenting the nature of the data depending on the format.
      - ``PG``
    * - 2 (0x02)
      - 4 B
      - Size (*S*)
      - Size of the data accompanying the header.
      - Big endian 16-bit unsigned integer.

Known data types for this format are:

.. list-table::
    :header-rows: 1

    * - Data Type (*DT*)
      - Description
    * - ``BU``
      - Backup.

.. todo:: Describe this more. Notably, there is more to the header!

.. _cas50-header-req:

``REQ\0`` Request
-----------------

.. todo:: Describe this.

.. _cas50-header-txt:

``TXT\0`` Textual File
----------------------

Such packets carry over a main memory textual file.

Format-specific data for this format is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Data Type (*DT*)
      - Data type, presenting the nature of the data depending on the format.
      - ``PG``
    * - 2 (0x02)
      - 4 B
      - Size (*S*)
      - Size of the data accompanying the header (big endian).

        For most data, this is either set to 0 if there are no data part, or
        the size of the data part plus 2 otherwise. However, some types
        override this behaviour to use it elsewhere.
      - ``\0\0\0\xFF``
    * - 6 (0x06)
      - 8 B
      - File Name (*FN*)
      - Name of the file, with unset bytes being set to ``\xFF``.
      - ``HELLO\xFF\xFF\xFF``
    * - 14 (0x0E)
      - 8 B
      - Reserved
      -
      - Set to ``\xFF``
    * - 22 (0x16)
      - 8 B
      - File Password (*FP*)
      - Password of the file, with unset bytes being set to ``\xFF``.
      - ``WORLD\xFF\xFF\xFF``
    * - 30 (0x1E)
      - 2 B
      - Option 1.
      - ``BN`` for Base programs, ``NL`` otherwise.
      - ``BN``
    * - 32 (0x20)
      - 2 B
      - Option 2
      -
      - Set to ``\xFF``
    * - 34 (0x22)
      - 2 B
      - Option 3
      -
      - Set to ``\xFF``
    * - 36 (0x24)
      - 2 B
      - Option 4
      -
      - Set to ``\xFF``

Known data types for this format are the following:

.. list-table::
    :header-rows: 1

    * - Data Type (*DT*)
      - Description
    * - ``PG``
      - Program.

.. _cas50-header-val:

``VAL\0`` Value
---------------

Such packets carry over one or more values.

Format-specific data for this format is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Data Type (*DT*)
      - Data type, presenting the nature of the data depending on the format.
      - ``PG``
    * - 2 (0x02)
      - 2 B
      - Height (*H*)
      -
      - Big endian 16-bit unsigned integer.
    * - 4 (0x04)
      - 2 B
      - Width (*W*)
      - Width of the array. Set to 0 for lists (i.e. only width is used).
      - Big endian 16-bit unsigned integer.
    * - 6 (0x06)
      - 8 B
      - Reserved
      -
      - Set to ``\xFF``.
    * - 14 (0x0E)
      - 8 B
      - Unknown
      -
      - ``VariableR\x0A``

Known data types for this format are the following:

.. list-table::
    :header-rows: 1

    * - Data Type (*DT*)
      - Description
    * - ``MT``
      - Matrix
    * - ``LT``
      - List

Every data payload represents a value in the collection, using a 14-byte
format composed of the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Y coordinate (*Y*)
      -
      - Big endian 16-bit unsigned integer.
    * - 2 (0x02)
      - 2 B
      - X coordinate (*X*)
      -
      - Big endian 16-bit unsigned integer.
    * - 4 (0x04)
      - 10 B
      - Value (*V*)
      -
      - :ref:`number-format-casiolink-bcd`

.. todo:: Check the format and its order!
