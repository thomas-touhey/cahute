Number formats
==============

There are multiple number formats among the protocols and formats around
CASIO calculators. The following sections describe some of them.

.. _number-format-casiolink-bcd:

CASIOLINK-style BCD
-------------------

Numbers with the CASIOLINK family of protocols and file formats are represented
using 10 bytes, i.e. 20 nibbles (groups of 4 bits).

Assuming we use 0-based indexing:

* Nibbles 0 to 15 (i.e. bytes 0 to 7) represent the mantissa, with one nibble
  per digit.
* Nibbles 16 and 17 (i.e. byte 8) contains the flags.
* Nibbles 18 and 19 (i.e. byte 9) is the exponent of the number, with one
  nibble per digit.

.. todo::

    What I wrote at the time about these numbers is these values for the
    flags, not sure how they combine:

    * 0x80 is "special", possibly meaning infinite values and such?
    * 0x50 is "negative", it said "the negative is two bits/flags (I don't
      know why)", so that may be great to determine.
    * 0x01 is "pow neg", meaning that the power is negative.

.. todo:: Provide an example here.

.. _number-format-fx9860g-bcd:

fx-9860G-style BCD
------------------

Numbers with the fx-9860G and compatible are represented using 12 bytes,
i.e. 24 nibbles, with only the first 9 bytes, i.e. 18 nibbles, being
significant.

Assuming we use 0-based indexing:

* Nibbles 0 to 2 represent the exponent and sign, with the following ranges:

  .. list-table::
      :header-rows: 1

      * - Raw range
        - Sign
        - Real range
      * - 000 - 499
        - ``+``
        - -99 - 399
      * - 500 - 999
        - ``-``
        - -99 - 399

* Nibbles 3 to 17 represent the mantissa, starting from 10^0.

Examples are the following:

.. list-table::
    :header-rows: 1
    :width: 100%

    * - Raw number (1 digit / nib.)
      - Sign
      - Exponent
      - Mantissa
      - Result
    * - ``600 123000000000000 xxxxxx``
      - ``-``
      - ``1`` (``600 - 100 - 99``)
      - ``.123``
      - ``-1.23``
    * - ``100 230400000000000 xxxxxx``
      - ``+``
      - ``1`` (``100 - 99``)
      - ``.2304``
      - ``2.304``
    * - ``098 456000000000000 xxxxxx``
      - ``-``
      - ``-1`` (``98 - 99``)
      - ``.456``
      - ``.0456``
    * - ``597 786000000000000 xxxxxx``
      - ``-``
      - ``-2`` (``597 - 500 - 99``)
      - ``.786``
      - ``-.00786``

.. _number-format-fx9860g-fraction:

fx-9860G-style fraction
-----------------------

Fractions with the fx-9860G and compatible are represented using 12 bytes,
i.e. 24 nibbles, with only the first 9 bytes, i.e. 18 nibbles, being
significant.

Assuming we use 0-based indexing:

* Nibble 0 represents the sign (``1`` for positive, ``6`` for negative);
* Nibble 1 is always ``A``;
* Nibble 2 is the number of nibbles to read after, minus 1.

Examples are the following:

.. list-table::
    :header-rows: 1
    :width: 100%

    * - Raw number (1 digit)
      - Sign
      - ``a``
      - ``b``
      - ``c``
      - Result
    * - ``6A 6 2 A 33 A 50 00000000 xxxxxx``
      - ``-``
      - ``2``
      - ``33``
      - ``50``
      - ``-2 * (33 / 50)``
    * - ``1A 7 2 A 33 A 500 0000000 xxxxxx``
      - ``+``
      - ``2``
      - ``33``
      - ``500``
      - ``2 * (33 / 500)``
