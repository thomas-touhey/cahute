CAS40 packet format
===================

The header format for a CAS40 packet is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Type (*FT*)
      - File type.
      - See following sections.
    * - 2 (0x02)
      - 5 B
      - Type-specific data (*TD*).
      - Binary type-specific data.
      - ``\x00\x01\x00\x00\x00`` for a 0x100-long program.
    * - 7 (0x07)
      - 12 B
      - File name (*FN*)
      - Name of the file for an editor program.
      - ``HELLO\xFF\xFF\xFF\xFF\xFF\xFF\xFF``
    * - 19 (0x13)
      - 12 B
      - File password (*FP*)
      - Password of the file for an editor program.
      - ``WORLD\xFF\xFF\xFF\xFF\xFF\xFF\xFF``
    * - 31 (0x1F)
      - 7 B
      - Reserved
      - Should be set to ``\xFF``
      - ``\xFF\xFF\xFF\xFF\xFF...``

.. todo:: Describe file types here?
