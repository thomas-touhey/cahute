.. _cas100-data-types:

CAS100 data types
=================

.. todo:: Write this!

The format of such headers is the following:

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
      - Basic content of the packet
      - ``MDL1``
    * - 4 (0x04)
      - 34 B
      - Type-specific data
      - Data for which the format is specific to the type.
      - See the type description for the format of this component.
        If the type-specific data is less than 34 bytes, the rest is filled
        with ``\xFF``.
    * - 38 (0x26)
      - 1 B
      - Checksum (*CS*)
      - Checksum for the packet.
      -

.. _cas100-header-adn1:

``ADN1`` headers
----------------

These packets seem to be used to send data.

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - Data Type (*DT*)
      -
      - ``INF1`` (System), ``FR00`` (Segment), ``MSG1`` (Language)
    * - 4 (0x04)
      - 4 B
      - ?
      -
      - Integer (little endian), e.g. ``0x40000`` (256 * 1024)
    * - 8 (0x08)
      - 4 B
      - ?
      -
      - Integer (little endian), e.g. ``0x80000000``
    * - 12 (0x0C)
      - 4 B
      - ?
      -
      - Integer (little endian), e.g. ``0x200`` (512).

.. _cas100-header-adn2:

``ADN2`` headers
----------------

Unknown purpose.

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - Data Type (*DT*)
      -
      - ``INF1`` (System), ``FR00`` (Segment), ``MSG1`` (Language)
    * - 4 (0x04)
      - 4 B
      - ?
      -
      - Integer (little endian), e.g. ``0x00000000`` (0).
    * - 8 (0x08)
      - 4 B
      - ?
      -
      - Integer (little endian), e.g. ``0x100`` (256).

.. _cas100-header-bku1:

``BKU1`` headers
----------------

Unknown purpose.

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - Data Type (*DT*)
      -
      - ``RAMS``, ``RAMI``, ``RAM1``
    * - 4 (0x04)
      - 4 B
      - ?
      -
      - Big endian 32-bit integer, e.g. ``0xE000``.

.. _cas100-header-end1:

``END1`` headers
----------------

These packets are sent at the end of the communication.

They do not use additional data.

.. _cas100-header-fcl1:

``FCL1`` headers
----------------

Unknown purpose.

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - Data Type (*DT*)
      -
      - ``S000``

.. _cas100-header-fmv1:

``FMV1`` headers
----------------

Unknown purpose.

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - Data Type (*DT*)
      -
      - ``FR00``
    * - 4 (0x04)
      - 8 B
      - Data Type 2 (*DT2*)
      -
      - ``FR00`` (sic.)

.. _cas100-header-mcs1:

``MCS1`` headers
----------------

These packets contain main memory data.

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 3 B
      - Reserved
      -
      - Set to ``\0``.
    * - 3 (0x03)
      - 2 B
      - File size
      -
      - Big-endian 16-bit integer (?).
    * - 5 (0x05)
      - 1 B
      - Data type
      -
      - 8-bit integer, among the following:

        .. list-table::
            :header-rows: 1

            * - Data type
              - Description
            * - ``0x01``
              - Program
    * - 6 (0x06)
      - 8 B
      - Data name
      -
      - ``0xFF`` optionally terminated string.
    * - 14 (0x0E)
      - 8 B
      - Group name
      -
      - ``0xFF`` optionally terminated string.

.. _cas100-header-mdl1:

``MDL1`` headers
----------------

These packets contain initialization data for the CAS100 variant of the
CASIOLINK protocol, with calculator model information.

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 6 B
      - Model Identification (*M*)
      -
      - ``ZX945\0``
    * - 6 (0x06)
      - 6 B
      - Baud speed (*BS*)
      - ?
      - ASCII-DEC, e.g. ``038400``
    * - 12 (0x0C)
      - 1 B
      - Parity (*PAR*)
      - ?
      - ``N``, ``E`` or ``O``
    * - 13 (0x0D)
      - 4 B
      - OS Version (*VER*)
      -
      - ``1.00``
    * - 17 (0x11)
      - 4 B
      - Flash ROM capacity
      -
      - Little endian 32-bit integer, e.g. ``0x100000`` (1048576, 1 MiB).
    * - 21 (0x15)
      - 4 B
      - RAM capacity
      -
      - Little endian 32-bit integer, e.g. ``0x40000`` (262144, 256 KiB).
    * - 25 (0x19)
      - 4 B
      - Unknown
      -
      - Little endian 32-bit integer, e.g. ``0x10000`` (65536, 64 KiB).
    * - 29 (0x1D)
      - 4 B
      - Unknown
      -
      - 4-char string, e.g. ``"0x07"`` (``0x30``, ``0x78``, ``0x30``,
        ``0x37``).

.. _cas100-header-req1:

``REQ1`` headers
----------------

These packets seem to be used to request information.

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - *Data Type* (*DT*)
      -
      - ``INF1`` (System), ``FR00`` (Segment), ``MSG1`` (Language)

.. _cas100-header-req2:

``REQ2`` headers
----------------

Unknown purpose.

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - *Data Type* (*DT*)
      -
      - ``INF1`` (System), ``FR00``-``FR09`` (Segment), ``MSG1`` (Language),
        ``MR04`` (?)
    * - 4 (0x04)
      - 4 B
      - ?
      -
      - Integer (little endian), e.g. 0.
    * - 8 (0x08)
      - 4 B
      - ?
      -
      - Integer (little endian), e.g. ``0x20000000`` (512 * 1024 * 1024)

.. _cas100-header-set1:

``SET1`` headers
----------------

Unknown purpose.

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - ?
      -
      - ``\x30\x01``
    * - 2 (0x02)
      - 8 B
      - ?
      -
      - ``0xFF`` optionally terminated string, e.g. ``Y=Data``.
