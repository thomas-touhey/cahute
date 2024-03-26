.. _file-format-casiolink:

CASIOLINK file formats
======================

CASIOLINK files are main memory archives for CASIO calculators. They usually
end with the ``.CAS`` extension.

The file is a series of main memory files directly concatenated, where every
file is constituted of the following:

* A header, composed of:

  * A ``:`` (0x3A) byte.
  * A header, of CAS40 or CAS50 type, including a checksum.
* Zero, one, or more data parts, depending on the header, composed of:

  * A ``:`` (0x3A) byte.
  * A contents, of the size provided by the header.
  * A checksum (1 byte).

The header format depends on the model of the calculator from or for which
the file was made.

.. _casiolink-cas40:

CAS40 main memory file format
-----------------------------

This header format is used on pre-1996 calculators, and ``.cas`` files produced
by these calculators. Such calculators include:

* CASIO fx-7700G (1991-1993);
* CASIO fx-9700GH (1995-1997);
* CASIO CFX-9800G (1995-1996).

The format of such headers is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Type (*TY*)
      - File type.
      - See following sections.
    * - 2 (0x02)
      - 36 B
      - Type-specific data
      - Data for which the format is specific to the type.
      - See the type description for the format of this component.
        If the type-specific data is less than 36 bytes, the rest is filled
        with ``\xFF``.
    * - 38 (0x26)
      - 1 B
      - Checksum (*CS*)
      -
      -

.. _casiolink-cas40-end:

``\x17\xFF`` CAS40 End
~~~~~~~~~~~~~~~~~~~~~~

This header represents an end of sequence. It is only used with the CASIOLINK
protocol, when using the CAS40 header format.

This is not followed by any data parts.

.. _casiolink-cas40-aa:

``AA`` CAS40 Dynamic Graphs
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-ad:

``AD`` CAS40 All Memories
~~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-al:

``AL`` CAS40 All
~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-am:

``AM`` CAS40 Variable Memories
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-bu:

``BU`` CAS40 Backup
~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-dc:

``DC`` CAS40 Color Screenshot
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Type-specific data for such files are the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 1 B
      - Height (*H*)
      -
      - 8-bit unsigned integer.

        Usually set to 64, which can be translated as ``@`` (hence some
        programs matching ``DC@``).
    * - 1 (0x01)
      - 1 B
      - Width (*W*)
      -
      - 8-bit unsigned integer.
    * - 2 (0x02)
      - 4 B
      - Screenshot format
      -
      - Screenshot format, among the following:

        .. list-table::
            :header-rows: 1

            * - Value
              - Description
            * - ``\x11UWF``
              - :ref:`picture-format-1bit-multiple-cas50`
    * - 6 (0x06)
      - 1 B
      - Sheet count
      -
      - Should be set to ``\x03``.

This is followed by 3 data parts, each representing a monochrome picture with
a one-byte prefix representing the color.

.. _casiolink-cas40-dd:

``DD`` CAS40 Monochrome Screenshot
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Type-specific data for such files are the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 1 B
      - Height (*H*)
      -
      - 8-bit unsigned integer.

        Usually set to 64, which can be translated as ``@`` (hence some
        programs matching ``DD@``).
    * - 1 (0x01)
      - 1 B
      - Width (*W*)
      -
      - 8-bit unsigned integer.
    * - 2 (0x02)
      - 4 B
      - Screenshot format
      -
      - Screenshot format, among the following:

        .. list-table::
            :header-rows: 1

            * - Value
              - Description
            * - ``\x10DWF``
              - :ref:`picture-format-1bit-cas50`.

This is followed by a single data part representing the monochrome picture.

.. _casiolink-cas40-dm:

``DM`` CAS40 Defined Memories
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-en:

``EN`` CAS40 Single Editor Program
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Type-specific data for such files are the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 1 B
      - Reserved.
      -
      - Should be set to ``\xFF``.
    * - 1 (0x01)
      - 2 B
      - Data Length (*DL*)
      - Length of the program, plus 2 (i.e. you must subtract 2 from this
        number before transmitting)
      - Big endian 16-bit unsigned integer.
    * - 3 (0x03)
      - 2 B
      - Reserved.
      -
      - Should be set to ``\xFF``.
    * - 5 (0x05)
      - 12 B
      - File name (*FN*)
      - Name of the file for an editor program.
      - ``HELLO\xFF\xFF\xFF\xFF\xFF\xFF\xFF``
    * - 17 (0x11)
      - 12 B
      - File password (*FP*)
      - Password of the file for an editor program.
      - ``WORLD\xFF\xFF\xFF\xFF\xFF\xFF\xFF``

.. todo:: Find out what data parts are sent here!

.. _casiolink-cas40-f1:
.. _casiolink-cas40-f6:

``F1`` / ``F6`` CAS40 F-Memory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-fn:

``FN`` CAS40 Multiple Editor Programs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This file type is actually the same as :ref:`casiolink-cas40-en`, except
it is in a context where multiple files exist.

.. todo::

    CaS also supports ``FP`` as a CAS40 file type in the ``FN`` loop.
    Maybe this should be placed in another section?

.. todo:: Find out what data parts are sent here!

.. _casiolink-cas40-ga:

``GA`` CAS40 Graph
~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-gf:

``GF`` CAS40 Graph Zoom
~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-gr:

``GR`` CAS40 Graph Range
~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-gt:

``GT`` CAS40 Function Table
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-ma:

``MA`` CAS40 Matrix
~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-pd:

``PD`` CAS40 Polynomial Equation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-p1:

``P1`` CAS40 Single Numbered Program
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Type-specific data for such files are the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 1 B
      - Reserved.
      -
      - Should be set to ``\0``.
    * - 1 (0x01)
      - 2 B
      - Data Length (*DL*)
      - Length of the program, plus 2 (i.e. you must subtract 2 from this
        number before transmitting)
      - Big endian 16-bit unsigned integer.
    * - 3 (0x03)
      - 1 B
      - Program Type (*PT*)
      - Type of the program.
      - One of the following:

        .. list-table::
            :header-rows: 1

            * - Value
              - Type
            * - ``0x02``
              - Store-Stats Data
            * - ``0x04``
              - Matrix
            * - ``0x10``
              - Standard Deviation
            * - ``0x20``
              - Linear Regression
            * - ``0x40``
              - Base-n
            * - ``0x80``
              - Draw stats graph
    * - 4 (0x04)
      - 1 B
      - Reserved.
      -
      - Should be set to ``\0``.

This is followed by a single data part containing the program's content.

.. _casiolink-cas40-pz:

``PZ`` CAS40 Multiple Numbered Programs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This file contains all 38 numbered programs from the program.

Type-specific data for such files are the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 1 B
      - Reserved.
      -
      - Should be set to ``\0``.
    * - 1 (0x01)
      - 2 B
      - Data Length (*DL*)
      - Total data length for all programs, plus 2 (i.e. you must subtract 2
        from this number before transmitting)
      - Big endian 16-bit unsigned integer.
    * - 3 (0x03)
      - 2 B
      - Reserved.
      -
      - Should be set to ``\0``.

This is followed by 2 data parts:

* A part of 190 bytes, used to include 38 times the type-specific data from
  ``P1`` (for 38 programs).
* A part containing data for all 38 programs concatenated, for which the
  length is equal to *DL* - 2.

See :ref:`casiolink-cas40-p1` for more information.

.. _casiolink-cas40-rt:

``RT`` CAS40 Recursion Table
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-sd:

``SD`` CAS40 Simultaneous Equations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-sr:

``SR`` CAS40 Paired Variable Data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas40-ss:

``SS`` CAS40 Single Variable Data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas50:

CAS50 main memory file format
-----------------------------

This header format is used on post-1996 calculators up until 2004, excluding
the AlgebraFX and compatible, and ``.cas`` files produced by these calculators.
Such calculators include:

* CASIO CFX-9850G (1996-1998);
* CASIO CFX-9950G (1996-1998);
* CASIO fx-9750G (1997-1999).

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
      - Basic purpose of the packet
      - ``END\0``
    * - 4 (0x04)
      - 2 B
      - File Type (*FT*)
      - File type, used by ``TXT`` packets.
      - ``PG``
    * - 6 (0x06)
      - 4 B
      - Size (*S*)
      - Size of the data accompanying the header (big endian).

        For most data, this is either set to 0 if there are no data part, or
        the size of the data part plus 2 otherwise. However, some types
        override this behaviour to use it elsewhere.
      - ``\0\0\0\xFF``
    * - 10 (0x0A)
      - 8 B
      - File Name (*FN*)
      - Name of the file, with unset bytes being set to ``\xFF``.
      - ``HELLO\xFF\xFF\xFF``
    * - 18 (0x12)
      - 8 B
      - Alternative File Type (*AFT*)
      - Alternative file type used for some packet types, notably variables.
      - ``VariableR\x0A``
    * - 26 (0x1A)
      - 8 B
      - File Password (*FP*)
      - Password of the file, with unset bytes being set to ``\xFF``.
      - ``WORLD\xFF\xFF\xFF``
    * - 34 (0x22)
      - 2 B
      - Base, if the file is a program.
      - ``BN`` for Base programs, ``NL`` otherwise.
      - ``BN``
    * - 36 (0x24)
      - 6 B
      - Backup Size (*BS*) *(?)*
      - Size of the backup (big endian).
      - ``\0\x10\0\0\0\0``
    * - 42 (0x2A)
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

.. _casiolink-cas50-end:

``END\xFF`` CAS50 End
~~~~~~~~~~~~~~~~~~~~~

This header represents an end of sequence. It is only used with the CASIOLINK
protocol, when using the CAS50 header format.

This is not followed by any data parts.

.. _casiolink-cas50-fnc:

``FNC\0`` CAS50 Function
~~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas50-img:

``IMG\0`` CAS50 Image
~~~~~~~~~~~~~~~~~~~~~

Such packets carry over a main memory picture file.

.. list-table::
    :header-rows: 1

    * - Subtype (*ST*) value
      - Description
    * - ``PC``
      - Picture.

.. _casiolink-cas50-mem:

``MEM\0`` CAS50 Backup
~~~~~~~~~~~~~~~~~~~~~~

Such packets carry over a backup.

.. list-table::
    :header-rows: 1

    * - Subtype (*ST*) value
      - Description
    * - ``BU``
      - Backup.

.. _casiolink-cas50-req:

``REQ\0`` CAS50 Request
~~~~~~~~~~~~~~~~~~~~~~~

.. todo:: Describe this.

.. _casiolink-cas50-txt:

``TXT\0`` CAS50 Textual File
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Such packets carry over a main memory textual file.

.. list-table::
    :header-rows: 1

    * - Subtype (*ST*) value
      - Description
    * - ``PG``
      - Program.

.. _casiolink-cas50-val:

``VAL\0`` CAS50 Value
~~~~~~~~~~~~~~~~~~~~~

Such packets carry over a variable. Particularities for this packet are:

* *FN* should be set to the variable name (?).
* *AFT* should be set to ``VariableR\x0A``.
