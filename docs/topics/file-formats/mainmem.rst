.. _file-format-mainmem:

Main memory archives
====================

A main memory archive is a collection of main memory files for the fx-9860G,
fx-CP and/or fx-CG. It can be identified using:

* The extension, among:

  - ``G1M`` (fx-9860G without settings);
  - ``G1R`` (fx-9860G with settings);
  - ``G2R`` (fx-CP with settings);
  - ``G3M`` (fx-CG without settings);
  - ``G3R`` (fx-CG with settings).
* The CASIO container subtype (see :ref:`file-format-standardheader`), among:

  - ``USBPower\x62\0\x10\0\x10\0`` (fx-9860G with or without settings);
  - ``USBPower\x31\0\x10\0\x10\0`` (fx-CP with or without settings);
  - ``USBPower\x75\0\x10\0\x10\0`` (fx-CG with or without settings).

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
