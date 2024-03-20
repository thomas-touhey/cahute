CASIO Container Format
======================

Multiple file formats by CASIO use what has been named a "standard header" by
the community. Said header is 32 (0x20) bytes long, and is obfuscated using
bitwise NOT.

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 14 B
      - File Type (*FT*)
      - Type of the file.
      - See the subsections below.
    * - 14 (0x0E)
      - 1 B
      - Control 1 (*C1*)
      - First control byte.
      - Should be set to ``(FS + 0x41) & 0xFF``.
    * - 15 (0x0F)
      - 1 B
      - Reserved
      -
      - Should be set to ``\0``.
    * - 16 (0x10)
      - 4 B
      - File Size (*FS*)
      -
      - Big endian 32-bit integer.
    * - 20 (0x14)
      - 1 B
      - Control 2 (*C2*)
      - Second control byte.
      - Should be set to ``(FS + 0xB8) & 0xFF``.
    * - 21 (0x15)
      - 7 B
      - Reserved
      -
      - Should be set to ``\0``.
    * - 28 (0x1C)
      - 2 B
      - Obfuscation Options
      -
      -
    * - 30 (0x1E)
      - 2 B
      - Object Count (*OC*)
      -
      - Big endian 16-bit integer.

Some formats have a standard subheader, with the following formats:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - Checksum (*CS*)
      -
      - Big endian 32-bit integer.
    * - 4 (0x04)
      - 1 B
      - File type (*FT*)
      -
      - Value set according to the file type:

        .. list-table::
            :header-rows: 1

            * - Value
              - Description
            * - ``0x00``
              - Picture (not used with this header).
            * - ``0x01``
              - Add-in.
            * - ``0x02``
              - Function keys.
            * - ``0x04``
              - Language files.
    * - 5 (0x05)
      - 1 B
      - Target platform (*FP*)
      -
      - Value set according to the targeted platform:

        .. list-table::
            :header-rows: 1

            * - Value
              - Description
            * - ``0x00``
              - fx-CP.
            * - ``0x01``
              - fx-CG (Prizm).
    * - 6 (0x06)
      - 4 B
      - Reserved.
      -
      - Set to ``\0``.
    * - 10 (0x0A)
      - 4 B
      - Data Size (*DS*)
      -
      - Big endian 32-bit integer.
    * - 14 (0x0E)
      - 4 B
      - Control value (*C3*)
      -
      - Big endian 32-bit integer.

        This should be set depending on the platform to:

        * For the fx-CP, ``FS - 0x1000 - 4``.
        * For the fx-CG, ``FS - 0x7000 - 4``.
    * - 18 (0x12)
      - 4 B
      - Reserved.
      -
      - Set to ``\0``.
    * - 22 (0x16)
      - 4 B
      - Message Zone Size (*MZS*)
      -
      - Big endian 32-bit integer.

        **Unreliable**, may and should be set to 0.
    * - 26 (0x1A)
      - 6 B
      - Targeted Add-in Platform (*AP*)
      -
      - For C1A, should be set to ``GY437``.
    * - 32 (0x20)
      - 28 B
      - Title (*T*)
      - Title or language name.
      -
    * - 60 (0x3C)
      - 4 B
      - File Size (*FS*)
      -
      - Big endian 32-bit integer.
    * - 64 (0x40)
      - 11 B
      - Internal Add-in Name (*IN*)
      -
      -
    * - 75 (0x4B)
      - 192 B
      - Language Labels
      -
      - 8 times a 24 bytes long string.
    * - 267 (0x10B)
      - 1 B
      - e-Activity strip flag (*EAS*)
      -
      - Available values are the following:

        .. list-table::
            :header-rows: 1

            * - Value
              - Description
            * - ``0x00``
              - The e-Activity can't be used.
            * - ``0x01``
              - The e-Activity can be used.
    * - 268 (0x10C)
      - 4 B
      - Reserved
      -
      - Set to ``\0``.
    * - 272 (0x110)
      - 10 B
      - Add-in version.
      -
      -
    * - 282 (0x11A)
      - 2 B
      - Reserved.
      -
      - Set to ``\0``.
    * - 284 (0x11C)
      - 14 B
      - Timestamp
      -
      - ``YYYY.MMDD.HHmm``-formatted date.

.. _file-format-c2p:

``CASIO\0\0\0c2p\0\0\0`` fx-CP picture
--------------------------------------

.. todo:: Describe this.

.. _file-format-g3l:

``Ly755   \x2C\0\x01\0\x01\0`` fx-CG language file
--------------------------------------------------

.. todo:: Describe this.

.. _file-format-g1l:

``USBPower\x12\0\x10\0\x10\0`` fx-9860G language file
-----------------------------------------------------

.. todo:: Describe this.

.. _file-format-g3a:

``USBPower\x2C\0\x01\0\x01\0`` fx-CG add-in
-------------------------------------------

.. todo:: Describe this.

.. _file-format-g1m:

``USBPower\x31\0\x10\0\x10\0`` fx-9860G main memory archive
-----------------------------------------------------------

Such files contain files from the calculator's main memory, including
settings, programs, lists, pictures, captures, matrixes, and so on.
This format is common to fx-9860G, fx-CP and fx-CG main memory archives.

Main memory files for these platforms have a group, a directory, a name
and a numeric type. The archive organizes the main memory files per group,
with a header per group followed by every file in the group with their
own header.

The format of an example of such a file is the following:

* Standard Header (32 B), referencing M + N objects
* Group 1 Header (20 B), referencing M objects
* File 1 Header (24 B)
* File 1 Contents
* File 2 Header (24 B)
* File 2 Contents
* ...
* File M Header (24 B)
* File M Contents
* Group 2 Header (20 B), referencing N objects
* File M + 1 Header (24 B)
* File M + 1 Contents
* ...
* File M + N Header (24 B)
* File M + N Contents

The number of main memory files is stored in the standard header, in the
*OC* field.

.. warning::

    Note that *OC* represents the number of main memory files, **not** the
    number of groups, i.e. at least ``M + N`` in the !

The Group Header format is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 16 B
      - Group Name (*GN*)
      -
      - NUL-padded string, e.g. ``PROGRAM\0\0\0\0\0\0\0\0\0``
    * - 16 (0x10)
      - 4 B
      - Group Count (*GC*)
      - Number of files within the group.
      - Big endian 32-bit integer.

The File Header format is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 8 B
      - Directory Name (*DN*)
      -
      - NUL-padded string.
    * - 8 (0x08)
      - 8 B
      - File Name (*FN*)
      -
      - NUL-padded string.
    * - 16 (0x10)
      - 1 B
      - File Type (*FT*)
      -
      - 8-bit integer.
    * - 17 (0x11)
      - 4 B
      - File Length (*FL*)
      -
      - Big endian 32-bit integer.
    * - 21 (0x15)
      - 3 B
      - Reserved
      -
      -

.. todo:: Main memory file formats?

.. _file-format-g1e:

``USBPower\x49\0\x10\0\x10\0`` fx-9860G e-activity
--------------------------------------------------

.. todo:: Describe this.

.. _file-format-g2r:

``USBPower\x62\0\x10\0\x10\0`` fx-CP main memory archive
--------------------------------------------------------

This format is equivalent to the equivalent format for the fx-9860G, but
the signature and extension signify that the main memory files contained
within may be specific to the fx-CP, or have been made on an fx-CP.

See :ref:`file-format-g1m` for more information.

.. _file-format-g3m:

``USBPower\x75\0\x10\0\x10\0`` fx-CG main memory archive
--------------------------------------------------------

This format is equivalent to the equivalent format for the fx-9860G, but
the signature and extension signify that the main memory files contained
within may be specific to the fx-CG, or have been made on an fx-CG.

See :ref:`file-format-g1m` for more information.

.. _file-format-g3p:

``USBPower\x7D\0\x10\0\x10\0`` fx-CG picture
--------------------------------------------

.. todo:: Describe this.

.. _file-format-g1a:

``USBPower\xF3\0\x10\0\x10\0`` fx-9860G add-in
----------------------------------------------

Such files contain add-in, i.e. native programs to be run on the fx-9860G.
They start with a 480-bytes long header formatted the following way:

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

``\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF`` Undefined
----------------------------------------------------------------------

Undefined extension; this needs to be determined using the extension.
