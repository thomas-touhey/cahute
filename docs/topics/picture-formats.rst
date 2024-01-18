Picture formats
===============

This document describes the various picture formats used by CASIO or other
software surrounding its calculators in protocols and file formats.

.. _picture-format-1bit:

1bpp monochrome picture format
------------------------------

This format is the basic frame format for fx-9860G calculators and compatible.
It is used for internal VRAM representations, as well as for screenstreaming.

In this format, every pixel is represented as a bit, i.e. one byte contains
8 pixels. An off bit (``0b0``) represents a white pixel, and an
on bit (``0b1``) represents a black pixel.

The picture is organized by row first, column second.

If the width is not divisible by 8, then the last bits of the last
byte of the row are unused (fill bits), and the next row starts at the
beginning of the next byte.

For computing the size of such pictures, one must compute the number of bytes
a row occupies (usually ``ceil(width / 8)``), then multiply it by the number
of rows.

In Cahute, this format is represented by
:c:macro:`CAHUTE_PICTURE_FORMAT_1BIT_MONO`.

.. _picture-format-2bit-dual:

Dual 1bpp gray picture format
-----------------------------

This format is one of the possible screen streaming formats for fx-CG
(Prizm) calculators, using the ``1RM2`` algorithm.

This format is basically two pictures using :ref:`picture-format-1bit`
placed back-to-back (with alignment), where the obtained colours are
the following:

.. list-table::
    :header-rows: 1

    * - Bit from picture 1/2
      - Bit from picture 2/2
      - Obtained colour (``0xRRGGBB``)
    * - ``0b0``
      - ``0b0``
      - ``0xFFFFFF``
    * - ``0b0``
      - ``0b1``
      - ``0xAAAAAA``
    * - ``0b1``
      - ``0b0``
      - ``0x777777``
    * - ``0b1``
      - ``0b1``
      - ``0x000000``

For computing the size of such pictures, one must compute the number of bytes
a picture occupies, and multiply it by two, i.e. ``2 * ceil(width / 8)``.

In Cahute, this format is represented by
:c:macro:`CAHUTE_PICTURE_FORMAT_1BIT_DUAL`.

.. _picture-format-4bit-rgb-packed:

4bpp packed RGB picture format
------------------------------

This format is one of the possible screen streaming formats for fx-CG
(Prizm) calculators, using the ``1RC3`` algorithm.

In this format, every pixel is represented by a nibble (group of
4 consecutive bits), where, from high to low order:

- If the first bit is on (``0b1``), then the red component is set.
- If the next bit is on (``0b1``), then the green component is set.
- If the next bit is on (``0b1``), then the blue component is set.
- The last bit is an alignment bit, and can be ignored.

The picture is organized by row first, column second.

If the width is not divisible by 2, then the last pixel of every odd row
and the first pixel of every even row share the same byte, and if the height
is also odd, then the last 4 bits of the picture will be present as alignment.

For computing the size of such pictures, one must compute the number of
pixels, divide it by two and round to the next integer, i.e.
``ceil(width * height / 2)``.

In Cahute, this format is represented by
:c:macro:`CAHUTE_PICTURE_FORMAT_4BIT_RGB_PACKED`.

.. _picture-format-r5g6b5:

R5G6B5 picture format
---------------------

This format is the basic frame format for fx-CG (Prizm) calculators.
It is used for internal VRAM representations, as well as for screenstreaming
using the ``1RC2`` algorithm.

In this format, every pixel is represented by a 16-bit integer represented
using big endian, where, from high to low order:

- The first 5 bits represent the high 5 bits of the red component.
- The next 6 bits represent the high 6 bits of the green component.
- The last 5 bits represent the high 5 bits of the blue component.

The picture is organized by row first, column second.

The size of such pictures is the number of pixels multiplied by 2.

In Cahute, this format is represented by
:c:macro:`CAHUTE_PICTURE_FORMAT_16BIT_R5G6B5`.
