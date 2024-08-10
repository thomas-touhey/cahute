.. _file-format-g1a:

fx-9860G add-ins
================

An fx-9860G add-in is a native application that can run on fx-9860G and
derivatives. It can be identified using the ``G1A`` extension, and uses
the CASIO container with subtype ``USBPower\xF3\0\x10\0\x10\0``; see
:ref:`file-format-standardheader` for more information.

Past the CASIO container header, such files start with a 480-bytes long header
formatted the following way:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 8 B
      - Internal Name
      - Internal name starting with ``@``, used for referencing the add-in
        from another add-in on the calculator.
      - Zero-padded string, e.g. ``@APP\0\0\0\0``
    * - 8 (0x08)
      - 4 B
      - E-Strip Count
      -
      - Big endian 32-bit integer.
    * - 12 (0x0C)
      - 4 B
      - Reserved
      -
      - Should be set to ``\0``.
    * - 16 (0x10)
      - 12 B
      - Add-In Version
      - Version to be displayed under *SYSTEM > VERSION*.
      - NUL-padded ``MM.mm.dddd``, e.g. ``01.23.4567\0\0``.
    * - 28 (0x1C)
      - 16 B
      - Creation Date
      -
      - NUL-padded ``YYYY.MMDD.HHMM``, e.g. ``2024.0327.2327\0\0``.
    * - 44 (0x2C)
      - 68 B
      - Menu Icon
      - Icon to be displayed in the main menu.
      - Monochrome picture encoded using :ref:`picture-format-1bit`.
    * - 112 (0x70)
      - 324 B
      - Reserved
      -
      - Should be set to ``\0``.
    * - 436 (0x1B4)
      - 8 B
      - Add-In Title
      - Title to be displayed under *SYSTEM > VERSION*.
      - NUL-padded string.
    * - 444 (0x1BC)
      - 20 B
      - Reserved
      -
      - Should be set to ``\0``.
    * - 464 (0x1D0)
      - 4 B
      - File Size
      - Size of the add-in contents following the header.
      - Big endian 32-bit integer.
    * - 468 (0x1D4)
      - 12 B
      - Reserved
      -
      - Should be set to ``\0``.

.. todo:: Describe this.
