Specific formats for Protocol 7.00
==================================

Protocol 7.00 is made so that the data is always printable.
There only notable exception for this is the basic packet type, i.e. the first
byte of all basic packets, which in most if not all cases is lesser
than 0x20.

In all other places, the protocol takes specific dispositions to make the
packets printable, described in the following sections.

.. _seven-ascii-hex:

ASCII-HEX
~~~~~~~~~

Numbers are represented with ASCII-HEX, i.e. by providing the ASCII-encoded
version of their hexadecimal representation. For example, 3886 (0xF2E) is
represented as ``0F2E`` (0x30, 0x46, 0x32, 0x45) on four bytes, or
``00000F2E`` (0x30, 0x30, 0x30, 0x30, 0x30, 0x46, 0x32, 0x45) on eight.

.. _seven-5c-padding:

0x5C padding
~~~~~~~~~~~~

Raw data may be escaped using "0x5C padding", i.e.:

* Characters in the 0x00-0x1F range have 0x20 added to their value, and
  are prefixed by ``\`` (0x5C). For example, 0x06 becomes ``\&`` (0x5C,
  0x26).
* Character ``\`` (0x5C) itself is prefixed by another ``\`` (0x5C),
  but does not have a bump, i.e. ``\`` (0x5C) becomes ``\\`` (0x5C).

When decode such padded data, if a ``\`` (0x5C) is encountered, there is
necessarily a character afterwards. The obtained character for the group of
two characters depends on the second character:

* If the second character is ``\`` (0x5C), then the result must be
  ``\`` (0x5C).
* Otherwise, the resulting byte is the ASCII code of the second character,
  to which we substract 0x20. For example, if ``\&`` (0x5C, 0x26) is
  encountered, the final byte must be 0x26 minus 0x20, i.e. 0x06.
