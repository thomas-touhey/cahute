File formats
============

Cahute aims at handling all file formats surrounding CASIO calculators, either
defined by CASIO themselves or by software made by the community.

Note that there are several ways of exploring file formats by CASIO, depending
on what your entrypoint is. The sections in this document

By format
---------

The file formats described by topic are the following:

.. toctree::
    :maxdepth: 2

    file-formats/addin-cg
    file-formats/addin-fx
    file-formats/calculator-text-format
    file-formats/casiolink
    file-formats/casrc
    file-formats/catalog
    file-formats/eact
    file-formats/fkeys-cg
    file-formats/fkeys-fx
    file-formats/fx-program
    file-formats/graphcard
    file-formats/lang-cg
    file-formats/lang-fx
    file-formats/mainmem
    file-formats/picture-cg
    file-formats/picture-cp

By extension
------------

The file formats described by extension are the following:

.. list-table::
    :header-rows: 1
    :width: 100%

    * - Extension
      - Description
    * - ``.C2P``
      - :ref:`file-format-c2p`
    * - ``.CAS``
      - :ref:`file-format-casiolink`
    * - ``.CASRC``
      - :ref:`file-format-casrc`
    * - ``.CAT``
      - :ref:`file-format-cat`
    * - ``.CTF``
      - :ref:`file-format-ctf`
    * - ``.FXP``
      - :ref:`file-format-fxp`
    * - ``.G1A``
      - :ref:`file-format-g1a`
    * - ``.G1E``
      - :ref:`file-format-g1e`
    * - ``.G1L``
      - :ref:`file-format-g1l`
    * - ``.G1M``
      - :ref:`file-format-mainmem`
    * - ``.G2R``
      - :ref:`file-format-mainmem`
    * - ``.G3A``
      - :ref:`file-format-g3a`
    * - ``.G3L``
      - :ref:`file-format-g3l`
    * - ``.G3M``
      - :ref:`file-format-mainmem`
    * - ``.G3P``
      - :ref:`file-format-g3p`
    * - ``.GRC``
      - :ref:`file-format-graphcard`

.. _file-format-standardheader:

By CASIO container subtype
--------------------------

Multiple file formats by CASIO use what has been named a "standard header"
by the community. Said header is 32 (0x20) bytes long, and is obfuscated by
using bitwise NOT.

The format of this header is the following:

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

Some subtypes, mostly those for fx-CP and fx-CG models, have a "standard
subheader", which has the following format:

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
      -
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

Known types for this container format are the following:

.. list-table::
    :header-rows: 1
    :width: 100%

    * - *FT* (StandardHeader)
      - *FT* (StandardSubheader)
      - Type
    * - ``CASIO\0\0\0c2p\0\0\0``
      - *N/A*
      - :ref:`file-format-c2p`
    * - ``Ly755   \x2C\0\x01\0\x01\0``
      - ``0x02``
      - :ref:`file-format-g3n`
    * - ``Ly755   \x2C\0\x01\0\x01\0``
      - ``0x04``
      - :ref:`file-format-g3l`
    * - ``USBPower\x12\0\x10\0\x10\0``
      - *N/A*
      - :ref:`file-format-g1l`
    * - ``USBPower\x2C\0\x01\0\x01\0``
      - ``0x01``
      - :ref:`file-format-g3a`
    * - ``USBPower\x31\0\x10\0\x10\0``
      - *N/A*
      - :ref:`file-format-mainmem` (fx-CP)
    * - ``USBPower\x49\0\x10\0\x10\0``
      - *N/A*
      - :ref:`file-format-g1e`
    * - ``USBPower\x62\0\x10\0\x10\0``
      - *N/A*
      - :ref:`file-format-mainmem` (fx-9860G)
    * - ``USBPower\x75\0\x10\0\x10\0``
      - *N/A*
      - :ref:`file-format-mainmem` (fx-CG)
    * - ``USBPower\x7D\0\x10\0\x10\0``
      - *N/A*
      - :ref:`file-format-g3p`
    * - ``USBPower\xF3\0\x10\0\x10\0``
      - *N/A*
      - :ref:`file-format-g1a`

.. todo::

    Document the ``\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF``
    case, in which we need to use the extension instead of the format within
    the file.
