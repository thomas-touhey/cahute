Text encodings
==============

CASIO uses specific character tables on their calculators.


.. note::
    These tables or encodings are sometimes named ``FONTCHARACTER`` by the
    community, since that was the name of the type that could contain a
    character code in the fx-9860G SDK published by CASIO in 2004.

In these tables, every code can represent either:

* Control characters, e.g. ``0x000A`` (newline);
* Graph characters, e.g. ``0x0023`` (``#``);
* Operation codes, or "opcodes" for short, e.g. ``0xF710`` (``Locate``).

.. note::

    All of these types will be named "characters" in this section.

CASIO has had two separate character tables following similar logics:

* The legacy character table, applied on calculators up to the fx-9860G,
  excluded.
* The fx-9860G character table, applied on all compatible calculators
  post-2004, including fx-CG and fx-CP calculators.

Both have the same multi-byte leader logic, i.e. characters have a "lead"
character within a specific set, then a code relative to this set.
Sets for the above character tables are the following:

* For legacy: ``0x00``, ``0x7F``, ``0xF7``.
* For fx-9860G: ``0x00``, ``0x7F``, ``0xE5``, ``0xE6``, ``0xE7``, ``0xF7``,
  ``0xF9``.

It is important to distinguish both, as while a lot of characters are
common between both tables, some characters have been removed or replaced
from one to the other, and the legacy table uses some of the fx-9860G table's
multi-byte leaders as characters.

The following sections will present the character encodings and associated
tables used within and surrounding CASIO calculators.

.. _text-encoding-fc8:

Variable width encoding
-----------------------

This encoding can be used with either the legacy or fx-9860G character table.
Every character is represented with either one or two bytes, depending on
the first byte of the sequence:

* If the first byte is a multi-byte leader for the character table, the
  sequence is two-bytes long;
* Otherwise, the sequence is one-byte long.

For example, take the encoded sequence ``\x12\xE5\xAB``:

* With the legacy character table, since none of the characters are multi-byte
  leaders, the sequence represents three characters ``0x0012``, ``0x00E5``,
  ``0x00AB``.
* With the fx-9860G character table, ``\xE5`` is a multi-byte leader, which
  means that the sequence represents two characters ``0x0012`` and ``0xE5AB``.

.. _text-encoding-fc16:

Fixed-width encoding
--------------------

This encoding can be used with either the legacy or fx-9860G character table.
Every character is represented using two bytes, using either big or little
endian.

For example, take the sequence of characters ``0x0012`` and ``0xE5AB``:

* If using big endian, the encoded sequence will be ``\x00\x12\xE5\xAB``;
* If using little endian, the encoded sequence will be ``\x12\x00\xAB\xE5``.

.. _text-encoding-cat:

CAT data encoding
-----------------

This encoding can be used with both the legacy or fx-9860G character table,
and represents every supported character with an ASCII-compatible character
sequence.

Some example sequences are the following:

* The legacy or fx-9860G character ``0x0040`` (``-``) is represented in CAT
  data encoding using the ASCII sequence ``-``;
* The legacy or fx-9860G character ``0xF718`` is represented in CAT data
  encoding using the ASCII sequence ``\ClrText``;
* The legacy character ``0x00E6`` is represented in CAT data encoding using
  the ASCII sequence ``CL``.

.. _text-encoding-ctf:

CTF data encoding
-----------------

.. todo:: Write this. ASCII-based.

.. _text-encoding-utf32:

UTF-32 encoding
---------------

Cahute supports the `UTF-32`_ fixed-length encoding without
Byte-Order Mark (BOM), with big and little endiannesses.

.. _text-encoding-utf8:

UTF-8 encoding
--------------

Cahute supports the `UTF-8`_ variable-length encoding without
Byte-Order Mark (BOM).

.. _UTF-32: https://en.wikipedia.org/wiki/UTF-32
.. _UTF-8: https://en.wikipedia.org/wiki/UTF-8
