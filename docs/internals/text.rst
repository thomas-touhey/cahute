Characters, encodings and conversions
=====================================

Cahute supports text encoding representation and conversions, and requires
several mechanisms to do so.

Character reference
-------------------

Cahute has its own database for representing CASIO's character tables, present
in the ``chars/chars.toml``. This file contains one entry per character in
one or both character tables, as a ``chars`` entry, with the following
properties:

``code``
    **(required)** Code of the targeted character tables, e.g. ``0x56`` or
    ``0xE560``.

``table``
    **(optional)** Targeted character table, among ``9860`` and ``legacy``.

    If this property is not set, the character is considered to belong to all
    character tables.

``code_legacy``
    **(optional)** Equivalent code in the legacy character table, if the
    table is not ``legacy`` and an equivalent exists.

``code_9860``
    **(optional)** Equivalent code in the fx-9860G character table, if the
    table is not ``9860`` and an equivalent exists.

``opcode``
    **(optional)** List of graph character codes the character resolves to for
    display purposes, if the character is an opcode.

    For example, this can be set to ``[0x66, 0xE5D6]``.

``unicode``
    **(optional)** Unicode character sequence, or list of character sequences,
    if the character has one or more equivalents as Unicode.

    A Unicode character sequence is represented as a list of integers in TOML.

    This can be set to a list of character sequences if e.g. the Unicode
    sequence has different forms depending on the source Unicode normalization
    form. **In this case, the NFC normalization must be placed first, since
    it will be used for conversions if the destination encoding is
    Unicode-based.**

    For example, the value can be set to the following:

    * For character ``0x45`` (``-``): ``[0x45]``.
    * For character ``0xE609`` (``é``): ``[[0xE9], [0x65, 0x301]]``
      *(NFC normalization first)*.

``cat``
    **(optional)** CAT sequence, or list of sequences, if the character has
    one or more euiqvalent sequences in the CAT data encoding.

    A sequence is represented as a string in TOML.

    If multiple sequences are provided, the first one will be used for
    conversions if the destination encoding is CAT.

    For example, the value can be set to the following:

    * For character ``0x21`` (``!``): ``["\\!mark", "!"]``.
    * For character ``0xF712``: ``"\\Send("``.

    .. note::

        Multiple character entries defining the same CAT sequence are allowed
        as long as they bear the same code, e.g. ``\LinearReg`` defined
        for both the legacy and fx-9860G character table.

Compiled character reference
----------------------------

The character reference present above is transpiled into a C source file,
``lib/chars.c``, by the Python script at ``chars/process_chars.py``.
The structures used and globals defined in this file are declared in
``lib/chars.h``, as included and used by ``lib/text.c`` which contains
the effective conversion and description utilities.

Character entries that can be gathered through character tables or parsing
trees are of the following format:

.. c:struct:: cahute_char_entry

    Character entry.

    .. c:member:: int code_legacy

        Code of the character in the legacy character table.

        Note that is is defined even if ``code_legacy`` is not defined in
        the character entry in ``chars.toml``, as long as ``table`` is
        defined to either ``legacy``, or not defined.

        If no code is available for the character in the legacy character
        table, this is set to 0.

    .. c:member:: int code_9860

        Code of the character in the fx-9860G character table.

        Note that is is defined even if ``code_9860`` is not defined in
        the character entry in ``chars.toml``, as long as ``table`` is
        defined to either ``9860``, or not defined.

        If no code is available for the character in the fx-9860G character
        table, this is set to 0.

    .. c:member:: cahute_u16 const *opcode

        Sequence of characters, in the same character table, the character
        resolves to for display purposes, if the character is an opcode.

    .. c:member:: size_t opcode_len

        Number of elements in :c:member:`opcode`.

        If this is set to 0, the character is not an opcode, and therefore,
        the aforementioned member should not be used.

    .. c:member:: cahute_u32 const *unicode

        Sequence of Unicode codepoints the character can be translated to,
        in Normalization Form D (NFD).

    .. c:member:: size_t unicode_len

        Number of elements in :c:member:`unicode`.

        If this is set to 0, the character does not have a Unicode translation,
        and therefore, the aforementioned member should not be used.

    .. c:member:: char const *cat

        Sequence of bytes the character can be translated into Catalog
        files; see :ref:`file-format-cat` for more information.

    .. c:member:: size_t cat_len

        Number of elements in :c:member:`cat`.

        If this is set to 0, the character does not have a CAT representation,
        and therefore, the aforementioned member should not be used.

Character entries are available through the following input / parsing oriented
utilities:

* ``cahute_chars_<table>_<leader>``: tables of 256
  :c:struct:`cahute_char_entry` pointers that are either ``NULL`` or
  defined, depending of if a character exists with that code with that
  multi-byte leader.

  For example, ``cahute_chars_9860_E5`` defines all 256 character entries
  from ``0xE500`` to ``0xE5FF`` included for the fx-9860G character table.
* Parsing trees.

Conversion logic
----------------

The conversion logic is implemented within :c:func:`cahute_convert_text`.
The structure of a conversion loop is always the same:

* Get the character from the source buffer.
* Convert the character from a table to another, if possible.
* Place the character into the destination buffer.

The available character conversion loops are the following:

* CASIO character based, where the intermediate product is a pointer to a
  :c:struct:`cahute_char_entry` structure.
* Unicode-based, where the intermediate product is a sequence of UTF-32
  encoded Unicode characters in host endianness.

.. _TOML: https://toml.io/en/
