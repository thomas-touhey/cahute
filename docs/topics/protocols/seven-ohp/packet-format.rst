Protocol 7.00 Screenstreaming packet format
===========================================

This document defines the packet formats for Protocol 7.00 Screenstreaming.

.. note::

    In places, most notably in some subheaders and with the checksum, the
    protocol uses the same ASCII-HEX encoding as for Protocol 7.00;
    see :ref:`seven-ascii-hex` for more information.

    Fields represented using ASCII-HEX will reference the section from
    Protocol 7.00 packet format directly.

All packets with Protocol 7.00 Screenstreaming packet have the

The packet mode for this mode is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 1 B
      - Type (*T*)
      - Basic purpose of the packet
      - 0x00 to 0x1F
    * - 1 (0x01)
      - 5 B
      - Subtype (*ST*)
      - Subtype for the packet.
      - 5-char ASCII string depending on the type.
    * - 6 (0x06)
      - *N* B
      - Payload (*D*)
      - Payload, for which the format depends on the format.
      -
    * - 6 + *N* (0x06 + *N*)
      - 2 B
      - Checksum (*CS*)
      - Checksum for the packet.
      - 2-char :ref:`seven-ascii-hex` value, e.g. ``5B``.

The checksum is computed :ref:`the same way as for Protocol 7.00
<seven-checksum>`, that is to say, it is obtained by summing all bytes from
*ST* to *D* included (i.e. the entire packet except *T* and *CS*), and
adding 1 to its bitwise complement.

The interpretation of the packet depends on the value of *T*.
See the following sections for more information.

.. note::

    The header does not specify the total size of the packet, which implies
    that it must be computed using the type, subtype and eventual subheader.
    This, unfortunately, makes it more susceptible to corruption.

.. _seven-ohp-ack-packet:

``0x06`` -- Acknowledgement packet
----------------------------------

The acknowledgement packet can be used to accept a synchronization, so that
the sender starts sending frame packets instead of check packets. See
:ref:`seven-ohp-acknowledge` for more information.

The only known subtype of this packet is ``02001``.

.. _seven-ohp-frame-packet:

``0x0B`` -- Frame packet
------------------------

The frame packet contains a frame of the screen.

``TYP01`` frame format
~~~~~~~~~~~~~~~~~~~~~~

Such packets are sent by the fx-9860G and compatible on screenstreaming
mode, and are of the following format:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 6 (0x06)
      - 1024 B
      - Frame (*F*)
      - Frame data, as a 128x64 1-bit monochrome picture; see
        :ref:`picture-format-1bit` for more information.
      -

.. _seven-screen-typz1:

``TYPZ1`` frame format
~~~~~~~~~~~~~~~~~~~~~~

Such packets are sent by the fx-CG series on screenstreaming modes, and are
of the following format:

.. warning::

  In this table, the offset starts at 6 instead of 0.
  The subheader itself is only 18 bytes long!

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 6 (0x06)
      - 6 B
      - Frame Length (*FL*)
      - Length of the *F* field.
      - 6-char :ref:`seven-ascii-hex` value, e.g. ``001234``.
    * - 12 (0x0C)
      - 4 B
      - Height (*H*)
      - Height of the frame, in pixels.
      - 4-char :ref:`seven-ascii-hex` value, e.g. ``0123``.
    * - 16 (0x10)
      - 4 B
      - Width (*W*)
      - Width of the frame, in pixels.
      - 4-char :ref:`seven-ascii-hex` value, e.g. ``0123``.
    * - 20 (0x14)
      - 4B
      - Frame Format (*FF*)
      - Format of the frame, i.e. picture encoding in use.
      - ``1RC2``, ``1RC3``, ``1RM2``
    * - 24 (0x18)
      - *FL* B
      - Frame (*F*)
      - Frame data.
      -

The frame formats are the following:

.. list-table::
    :header-rows: 1

    * - *FF* field value
      - Format
      - Expected size
    * - ``1RC2``
      - :ref:`picture-format-r5g6b5`
      - ``width * height * 2``
    * - ``1RC3``
      - :ref:`picture-format-4bit-rgb-packed`
      - ``ceil(width * height / 2)``
    * - ``1RM2``
      - :ref:`picture-format-2bit-dual`
      - ``2 * height * ceil(width / 8)``

``TYPZ2`` frame format
~~~~~~~~~~~~~~~~~~~~~~

Such packets can also be sent by the fx-CG series on screenstreaming modes,
and are of the following format:

.. warning::

  In this table, the offset starts at 6 instead of 0.
  The subheader itself is only 20 bytes long!

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 6 (0x06)
      - 8 B
      - Frame Length (*FL*)
      - Length of the *F* field.
      - 8-char :ref:`seven-ascii-hex` value, e.g. ``00001234``.
    * - 14 (0x0E)
      - 4 B
      - Height (*H*)
      - Height of the frame, in pixels.
      - 4-char :ref:`seven-ascii-hex` value, e.g. ``0123``.
    * - 18 (0x12)
      - 4 B
      - Width (*W*)
      - Width of the frame, in pixels.
      - 4-char :ref:`seven-ascii-hex` value, e.g. ``0123``.
    * - 22 (0x16)
      - 4B
      - Frame Format (*FF*)
      - Format of the frame, i.e. picture encoding in use.
      - ``1RC2``, ``1RC3``, ``1RM2``
    * - 26 (0x1A)
      - *FL* B
      - Frame (*F*)
      - Frame data.
      -

.. note::

    This is roughly equivalent to the :ref:`seven-screen-typz1`, with
    the *FL* field having been changed from 6 to 8 chars to allow
    larger frames.

The frame formats are the same as for :ref:`seven-screen-typz1`.

.. _seven-ohp-check-packet:

``0x16`` -- Check packet
------------------------

Check packets are sent by the sender to request an acknowledgement from the
receiver. See :ref:`seven-ohp-acknowledge` for more information.

The only known subtype for this mode is ``CAL00``.
