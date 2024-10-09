.. _cas40-data-types:

CAS40 data types
================

Data in the CAS40 protocol is composed a header, followed by data parts,
all prefixed with ``0x3A``.

Headers are always 40 bytes long, and of the following format:

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
      - 2 B
      - Type (*TY*)
      - File type.
      - See following sections.
    * - 3 (0x03)
      - 36 B
      - Type-specific data
      - Data for which the format is specific to the type.
      - See the type description for the format of this component.
        If the type-specific data is less than 36 bytes, the rest is filled
        with ``\xFF``.
    * - 39 (0x27)
      - 1 B
      - Checksum (*CS*)
      -
      -

The type-specific data and count, size and format of data packets sent after
the header depend on the type.

See the following subsections for more information.

.. _cas40-header-al-end:

``\x17\x17`` AL End
-------------------

This header represents an end of sequence when in ``AL`` mode. It is only
used with the CASIOLINK protocol, when using the CAS40 header format, and
when the ``AL`` data type has been sent and received at least once;
see :ref:`cas40-al-mode` for more information.

This is not followed by any data parts.

.. _cas40-header-end:

``\x17\xFF`` End
----------------

This header represents an end of sequence. It is only used with the CASIOLINK
protocol, when using the CAS40 header format.

This is not followed by any data parts.

.. note::

    This prefix is common to all sentinels in the CAS40 variant, i.e.
    headers and data parts, of the size corresponding to the expected
    data part size.

.. note::

    This data type does not end the communication when ``AL`` mode has been
    enabled; see :ref:`cas40-al-mode`.

.. _cas40-header-a1:

``A1`` Dynamic Graph
--------------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 1 B
      - Reserved
      -
      - Set to ``\0``.
    * - 2 (0x02)
      - 2 B
      - Length (*L*)
      - Size of the data part, plus 2.
      - 8-bit integer.

This is followed by 1 data part of *L - 2* bytes, being the definition of the
dynamic graph with a ``\xFF`` sentinel.

This data type is final.

.. _cas40-header-aa:

``AA`` Dynamic Graph in Bulk
----------------------------

This file type is actually the same as :ref:`cas40-header-a1`, except
it is in a context where multiple editor programs are being sent, i.e.
the data type is non-final.

.. _cas40-header-ad:

``AD`` All Variable Memories
----------------------------

This data type contains all variable memories currently defined on the
device.

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Unknown
      -
      - Set to ``"CA"``.
    * - 2 (0x02)
      - 2 B
      - Count (*C*)
      - Number of elements, including the sentinel.
      - Big endian 16-bit integer.

There are *C* times data parts of 22 bytes each, the last one being
the sentinel, with the following data:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Type
      - Data part type.
      - ``\0\0`` for the cells, ``\x17\xFF`` for the sentinel.
    * - 2 (0x02)
      - 10 B
      - Value (real part)
      -
      - :ref:`number-format-casiolink-bcd`
    * - 12 (0x0C)
      - 10 B
      - Value (imaginary part)
      -
      - :ref:`number-format-casiolink-bcd`

This data type is final.

.. _cas40-header-al:

``AL`` All
----------

This data type signals that the calculator is about to send all of its data.

This does does have type-specific data, and is not followed by any data parts.

.. note::

    If this data type is received at least once, it means that all final
    data types become non-final, and that a special sentinel header is
    required; see :ref:`cas40-al-mode` for more information.

.. _cas40-header-am:

``AM`` Variable Memories
------------------------

This is equivalent to :ref:`cas40-header-ad`, but only returns variables
designated using a letter or symbol.

.. _cas40-header-bu:

``BU`` Backup
-------------

Type-specific data for such files are the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 7 B
      - Backup Type (*BUT*)
      -
      - Backup type, among:

        * ``TYPEA00``: fx-9700GH style backup (32768 bytes).
        * ``TYPEA02``: CFX-9800G style backup (32768 bytes).

There is one data part, for which the size depends on the backup type.

This data type is final.

.. _cas40-header-dc:

``DC`` Color Screenshot
-----------------------

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
      - 1 B
      - Screenshot format
      -
      - Screenshot format, among the following:

        .. list-table::
            :header-rows: 1

            * - Value
              - Description
            * - ``\x11``
              - :ref:`picture-format-1bit-multiple-cas50`
    * - 3 (0x03)
      - 1 B
      - Direction (*DR*)
      -
      - ``U`` (?)
    * - 4 (0x04)
      - 1 B
      - Byte Direction
      -
      - ``W`` (?)
    * - 5 (0x05)
      - 1 B
      - Bit Weight (*BW*)
      -
      - ``F`` (?)
    * - 6 (0x06)
      - 1 B
      - Sheet count
      -
      - Should be set to ``\x03``.

.. todo:: Document the role of the different fields here!

This is followed by 3 data parts, each representing a monochrome picture with
a one-byte prefix representing the color.

This data type is final.

.. _cas40-header-dd:

``DD`` Monochrome Screenshot
----------------------------

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
      - 1 B
      - Screenshot format
      -
      - Screenshot format, among the following:

        .. list-table::
            :header-rows: 1

            * - Value
              - Description
            * - ``\x10``
              - :ref:`picture-format-1bit-cas50`.
    * - 3 (0x03)
      - 1 B
      - Direction (*DR*)
      -
      - ``D`` (?)
    * - 4 (0x04)
      - 1 B
      - Byte Direction
      -
      - ``W`` (?)
    * - 5 (0x05)
      - 1 B
      - Bit Weight (*BW*)
      -
      - ``F`` (?)

.. todo:: Document the role of the different fields here!

This is followed by a single data part representing the monochrome picture.

This data type is final.

.. _cas40-header-dm:

``DM`` Defined Memories
-----------------------

This is equivalent to :ref:`cas40-header-ad`, but only returns defined
memories.

.. _cas40-header-en:

``EN`` Single Editor Program
----------------------------

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
      - Reserved
      -
      - Set to ``\0``.
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

This is followed by a single program being the program's content.

This data type is final.

.. _cas40-header-ep:

``EP`` Single Password Protected Editor Program
-----------------------------------------------

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
      - Reserved
      -
      - Set to ``\0``.
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

This is followed by a single program being the program's content.

This data type is final.

.. _cas40-header-f1:

``F1`` Single Function
----------------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 1 B
      - Reserved
      -
      - Set to ``\0``.
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
      - Should be set to ``\0``.

This is followed by a single data part being the program's content.

This data type is final.

.. _cas40-header-f6:

``F6`` Multiple Functions
-------------------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 1 B
      - Reserved
      -
      - Set to ``\0``.
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
      - Should be set to ``\0``.
    * - 5 (0x05)
      - 2 B
      - Function 1 Length (*FL1*)
      -
      - Big endian 16-bit length of the function 1 definition.
    * - 7 (0x07)
      - 2 B
      - Function 2 Length (*FL2*)
      -
      - Big endian 16-bit length of the function 2 definition.
    * - 9 (0x09)
      - 2 B
      - Function 3 Length (*FL3*)
      -
      - Big endian 16-bit length of the function 3 definition.
    * - 11 (0x0B)
      - 2 B
      - Function 4 Length (*FL4*)
      -
      - Big endian 16-bit length of the function 4 definition.
    * - 13 (0x0D)
      - 2 B
      - Function 5 Length (*FL5*)
      -
      - Big endian 16-bit length of the function 5 definition.
    * - 15 (0x0F)
      - 2 B
      - Function 6 Length (*FL6*)
      -
      - Big endian 16-bit length of the function 6 definition.

This is followed by a single data part with the contents of all of the
functions.

This data type is final.

.. _cas40-header-fn:

``FN`` Single Editor Program in Bulk
------------------------------------

This file type is actually the same as :ref:`cas40-header-en`, except
it is in a context where multiple editor programs are being sent, i.e.
the data is non-final.

.. _cas40-header-fp:

``FP`` Single Password Protected Editor Program in Bulk
-------------------------------------------------------

This file type is actually the same as :ref:`cas40-header-ep`, except
it is in a context where multiple editor programs are being sent, i.e.
the data is non-final.

.. _cas40-header-g1:

``G1`` Graph Function
---------------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 1 B
      - Reserved
      -
      - Set to ``\0``.
    * - 1 (0x01)
      - 2 B
      - Length (*L*)
      - Length of the contents, plus two.
      - Big-endian 16-bit integer.
    * - 3 (0x03)
      - 2 B
      - Unknown
      -
      - Set to ``\0`` by default.
    * - 5 (0x05)
      - 2 B
      - Type (*T*)
      -
      - Big-endian 16-bit integer, for which the values are:

        .. list-table::
            :header-rows: 1

            * - Value
              - Description
            * - ``0x0000``
              - Unset
            * - ``0x0100``
              - Rect (``Y=...X``)
            * - ``0x0102``
              - Pol (``r=...θ``), with optional ``0xF6`` (``,``) separator.
            * - ``0x0103``
              - Parm (``Xt=...T``)
            * - ``0x0104``
              - Ineq (``Y>...X``)
            * - ``0x0105``
              - Ineq (``Y<...X``)
            * - ``0x0106``
              - Ineq (``Y≥...X``)
            * - ``0x0107``
              - Ineq (``Y≤...X``)

There is exactly 1 data part of *L* - 2 bytes, containing the source of
the Graph Function.

This data type is final.

.. _cas40-header-ga:

``GA`` Graph Function in Bulk
-----------------------------

This file type is actually the same as :ref:`cas40-header-g1`, except
it is in a context where multiple graph functions are being sent, i.e.
the data is non-final.

.. _cas40-header-gf:

``GF`` Factor
-------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Unknown
      -
      - Set to ``"RA"``.
    * - 2 (0x02)
      - 2 B
      - Unknown
      -
      - Set to ``\x00\x02``.

There is exactly 1 data part of 22 bytes, of the following format:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Reserved
      -
      - Set to ``\0``.
    * - 2 (0x02)
      - 10 B
      - Xfact
      -
      - :ref:`number-format-casiolink-bcd`
    * - 12 (0x0C)
      - 10 B
      - Yfact
      -
      - :ref:`number-format-casiolink-bcd`

This data type is final.

.. _cas40-header-gr:

``GR`` Range
------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Unknown
      -
      - Set to ``"RA"``.
    * - 2 (0x02)
      - 2 B
      - Unknown
      -
      - Set to ``\x00\x09``.

There is exactly 1 data part of 92 bytes, of the following format:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Reserved
      -
      - Set to ``\0``.
    * - 2 (0x02)
      - 10 B
      - Xmin
      -
      - :ref:`number-format-casiolink-bcd`
    * - 12 (0x0C)
      - 10 B
      - Xmax
      -
      - :ref:`number-format-casiolink-bcd`
    * - 22 (0x16)
      - 10 B
      - Xscale
      -
      - :ref:`number-format-casiolink-bcd`
    * - 32 (0x20)
      - 10 B
      - Ymin
      -
      - :ref:`number-format-casiolink-bcd`
    * - 42 (0x2A)
      - 10 B
      - Ymax
      -
      - :ref:`number-format-casiolink-bcd`
    * - 52 (0x34)
      - 10 B
      - Yscale
      -
      - :ref:`number-format-casiolink-bcd`
    * - 62 (0x3E)
      - 10 B
      - Tmin, θmin
      -
      - :ref:`number-format-casiolink-bcd`
    * - 72 (0x48)
      - 10 B
      - Tmax, θmax
      -
      - :ref:`number-format-casiolink-bcd`
    * - 82 (0x52)
      - 10 B
      - Tpitch, θpitch
      -
      - :ref:`number-format-casiolink-bcd`

This data type is final.

.. _cas40-header-gt:

``GT`` Function Table
---------------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Reserved
      -
      - Set to ``"RA"``.
    * - 2 (0x02)
      - 2 B
      - Length (*L*)
      - Length of the function definition, plus two.
      - Big endian 16-bit integer.
    * - 4 (0x04)
      - 2 B
      - Count (*C*)
      -
      - Big endian 16-bit integer.
    * - 6 (0x06)
      - 2 B
      - Unknown
      -
      - Set to ``\0\0``.

There is *C* + 2 data parts, where:

* The first data part is the source function from which the table is
  computed, which is *L - 2* bytes long and includes a sentinel (``\xFF``).
* The second data part are the table properties, which are 32 bytes long.
  They have the following format:

  .. list-table::
      :header-rows: 1

      * - Offset
        - Size
        - Field name
        - Description
        - Values
      * - 0 (0x00)
        - 2 B
        - Reserved
        -
        - Set to ``\0\0``.
      * - 2 (0x02)
        - 10 B
        - Start
        -
        - :ref:`number-format-casiolink-bcd`
      * - 12 (0x0C)
        - 10 B
        - End
        -
        - :ref:`number-format-casiolink-bcd`
      * - 22 (0x16)
        - 10 B
        - Pitch
        -
        - :ref:`number-format-casiolink-bcd`

* The next *C* data parts are the cells, which are 22 bytes long.
  They have the following format:

  .. list-table::
      :header-rows: 1

      * - Offset
        - Size
        - Field name
        - Description
        - Values
      * - 0 (0x00)
        - 2 B
        - Reserved
        -
        - Set to ``\0\0``.
      * - 2 (0x02)
        - 10 B
        - X
        -
        - :ref:`number-format-casiolink-bcd`
      * - 12 (0x0C)
        - 10 B
        - Y
        -
        - :ref:`number-format-casiolink-bcd`

This data type is final.

.. _cas40-header-m1:

``M1`` Single Matrix
--------------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Reserved
      -
      - Set to ``"RA"``.
    * - 2 (0x02)
      - 1 B
      - Width (*W*)
      -
      - Width of the matrix.
    * - 3 (0x03)
      - 1 B
      - Height (*H*)
      -
      - Height of the matrix.

There are *W* times *H* + 1 data parts of 14 bytes each, the last one being
the sentinel, with the following data:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Type
      - Data part type.
      - ``\0\0`` for the cells, ``\x17\xFF`` for the sentinel.
    * - 2 (0x02)
      - 1 B
      - X coordinate (*X*)
      - Horizontal coordinate of the cell, starting at 1.
      - 8-bit integer.
    * - 3 (0x03)
      - 1 B
      - Y coordinate (*Y*)
      - Vertical coordinate of the cell, starting at 1.
      - 8-bit integer.
    * - 4 (0x04)
      - 10 B
      - Value (*V*)
      - Value contained by the cell.
      - :ref:`number-format-casiolink-bcd`

This data type is final.

.. _cas40-header-ma:

``MA`` Single Matrix in Bulk
----------------------------

Equivalent to :ref:`cas40-header-m1`, except:

* There are *W* times *H* data parts instead of *W* times *H*, as the
  sentinel is not present;
* The data type is not final.

.. _cas40-header-p1:

``P1`` Single Numbered Program
------------------------------

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

This data type is final.

.. _cas40-header-pd:

``PD`` Polynomial Equation
--------------------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Reserved
      -
      - Set to ``"RA"``.
    * - 2 (0x02)
      - 2 B
      - Degree (*D*)
      -
      - Big endian 16-bit integer.

The contents depends on the degree (*D*) field:

* For degree 2, there is 1 data part which is 32 bytes long, and contains the
  components of the ``ax²+bx+c=0`` equation, in the following format:

  .. list-table::
      :header-rows: 1

      * - Offset
        - Size
        - Field name
        - Description
        - Values
      * - 0 (0x00)
        - 2 B
        - Reserved
        -
        - Set to ``\0\0``
      * - 2 (0x02)
        - 10 B
        - a
        -
        - :ref:`number-format-casiolink-bcd`
      * - 12 (0x0C)
        - 10 B
        - b
        -
        - :ref:`number-format-casiolink-bcd`
      * - 22 (0x16)
        - 10 B
        - c
        -
        - :ref:`number-format-casiolink-bcd`

* For degree 3, there is 1 data part which is 42 bytes long, and contains the
  components of the ``ax³+bx²+cx+d=0`` equation, in the following format:

  .. list-table::
      :header-rows: 1

      * - Offset
        - Size
        - Field name
        - Description
        - Values
      * - 0 (0x00)
        - 2 B
        - Reserved
        -
        - Set to ``\0\0``
      * - 2 (0x02)
        - 10 B
        - a
        -
        - :ref:`number-format-casiolink-bcd`
      * - 12 (0x0C)
        - 10 B
        - b
        -
        - :ref:`number-format-casiolink-bcd`
      * - 22 (0x16)
        - 10 B
        - c
        -
        - :ref:`number-format-casiolink-bcd`
      * - 32 (0x20)
        - 10 B
        - d
        -
        - :ref:`number-format-casiolink-bcd`

This data type is final.

.. _cas40-header-pz:

``PZ`` Multiple Numbered Programs
---------------------------------

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

See :ref:`cas40-header-p1` for more information.

This data type is final.

.. _cas40-header-rt:

``RT`` Recursion Table
----------------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Reserved
      -
      - Set to ``"RA"``.
    * - 2 (0x02)
      - 2 B
      - Length (*L*)
      - Length of the function definition, plus two.
      - Big endian 16-bit integer.
    * - 4 (0x04)
      - 2 B
      - Count (*C*)
      -
      - Big endian 16-bit integer.
    * - 6 (0x06)
      - 2 B
      - Unknown
      -
      - Set to ``\0\0``.

There is *C* + 2 data parts, where:

* The first data part is the source function from which the table is
  computed, which is *L - 2* bytes long and includes a sentinel (``\xFF``).
* The second data part are the table properties, which are 22 bytes long.
  They have the following format:

  .. list-table::
      :header-rows: 1

      * - Offset
        - Size
        - Field name
        - Description
        - Values
      * - 0 (0x00)
        - 2 B
        - Reserved
        -
        - Set to ``\0\0``.
      * - 2 (0x02)
        - 10 B
        - nStart
        -
        - :ref:`number-format-casiolink-bcd`
      * - 12 (0x0C)
        - 10 B
        - nEnd
        -
        - :ref:`number-format-casiolink-bcd`

* The next *C* data parts are the cells, which are 32 bytes long.
  They have the following format:

  .. list-table::
      :header-rows: 1

      * - Offset
        - Size
        - Field name
        - Description
        - Values
      * - 0 (0x00)
        - 2 B
        - Reserved
        -
        - Set to ``\0\0``.
      * - 2 (0x02)
        - 10 B
        - n
        -
        - :ref:`number-format-casiolink-bcd`
      * - 12 (0x0C)
        - 10 B
        - an
        -
        - :ref:`number-format-casiolink-bcd`
      * - 22 (0x16)
        - 10 B
        - Σan
        -
        - :ref:`number-format-casiolink-bcd`

This data type is final.

.. _cas40-header-sd:

``SD`` Simultaneous Equations
-----------------------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Reserved
      -
      - Set to ``"RA"``.
    * - 2 (0x02)
      - 1 B
      - Width (*W*)
      -
      - 8-bit integer.
    * - 3 (0x03)
      - 1 B
      - Height (*H*)
      -
      - 8-bit integer.

There is *W* * *H* + 1 data parts, each 14 bytes long, of the following format:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Type
      - Data part type.
      - ``\0\0`` for the cells, ``\x17\xFF`` for the sentinel.
    * - 2 (0x02)
      - 1 B
      - X
      - Horizontal coordinate in the matrix, starting from 1.
      - 8-bit integer.
    * - 3 (0x03)
      - 1 B
      - Y
      - Vertical coordinate in the matrix, starting from 1.
      - 8-bit integer.
    * - 4 (0x02)
      - 10 B
      - Value for the cell.
      -
      - :ref:`number-format-casiolink-bcd`

This data type is final.

.. _cas40-header-sr:

``SR`` Paired Variable Data
---------------------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Reserved
      -
      - Set to ``"RA"``.
    * - 2 (0x02)
      - 2 B
      - Count (*C*)
      - Number of elements, including the sentinel.
      - Big endian 16-bit integer.

There are *C* times data parts of 32 bytes each, the last one being
the sentinel, with the following data:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Type
      - Data part type.
      - ``\0\0`` for the cells, ``\x17\xFF`` for the sentinel.
    * - 2 (0x02)
      - 10 B
      - X value
      -
      - :ref:`number-format-casiolink-bcd`
    * - 12 (0x0C)
      - 10 B
      - Y value
      -
      - :ref:`number-format-casiolink-bcd`
    * - 22 (0x16)
      - 10 B
      - f value
      -
      - :ref:`number-format-casiolink-bcd`

This data type is final.

.. _cas40-header-ss:

``SS`` Single Variable Data
---------------------------

Type-specific data is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Reserved
      -
      - Set to ``"RA"``.
    * - 2 (0x02)
      - 2 B
      - Count (*C*)
      - Number of elements, including the sentinel.
      - Big endian 16-bit integer.

There are *C* + 1 data parts of 22 bytes each, the last one being
the sentinel, with the following data:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Type
      - Data part type.
      - ``\0\0`` for the cells, ``\x17\xFF`` for the sentinel.
    * - 2 (0x02)
      - 10 B
      - X value
      -
      - :ref:`number-format-casiolink-bcd`
    * - 12 (0x0C)
      - 10 B
      - f value
      -
      - :ref:`number-format-casiolink-bcd`

This data type is final.
