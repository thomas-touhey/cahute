.. _file-format-casrc:

casrc configuration file
========================

The ``casrc`` file is the "run commands" file for ``CaS`` (see :ref:`cas`).
It defines two elements:

* Settings, as sets of key/value pairs named "properties".
* Macros, as sequences of "property diffs" composed of the following:

  - The name of the property to update.
  - Whether to set or unset the property.
  - If the property is to set, the value to set to the property.

For example, a macro can be defined with ``first=value,second,no-third``,
where the produced property diffs will be the following:

* The ``first`` property should be set to ``value``.
* The ``second`` property should be set to the empty string.
* The ``third`` property should be unset.

Well-known locations
--------------------

The casrc file is present in the following locations:

* On DOS, it is located in the same directory as the CaS executable.
* On POSIX-compatible systems, it is either located in ``~/.casrc``, or if
  not found, in the system directory, for example ``/etc/system.casrc``.

File format
-----------

The casrc file defines macros and settings only using property diffs.
It is line-oriented, using LF (U+000A) as line separators.

Line format
~~~~~~~~~~~

The format of a line is the following:

.. code-block:: abnf

    ; A comment cannot be placed at the end of a macro or setting definition.
    line = *space [macro-definition / setting-definition / comment] LF

    ; A comment starts with "#" or ";".
    comment = %x23 / %x3B *(%x01-09 / %x0B-7E) ; Any non-LF character.

    ; Macro and setting values have the same format, and take all of the line
    ; until the LF.
    macro-definition = "macro" 1*space name (1*space / *space "=" / *space ":") value
    setting-definition = name (1*space / *space "=" / *space ":") value

    ; Macro and setting name cannot contain space.
    ; Macro names can technically contain commas, but adding such characters
    ; will make them unusable, since a component value cannot contain
    ; a comma.
    name = 1*(%x01-08 / %x0E-1F / %x21-7E)

    ; Values are comma-separated components, which in turn are
    ; either macro names or property diffs.
    value = *(*WSP [component *WSP] ",") *WSP [component *WSP]

    ; For line parsing or format verifying purposes, a component can be
    ; any sequence that doesn't include a comma and its surrounding
    ; spaces or horizontal tabulations.
    ; A component is allowed to be empty, which makes it define the empty
    ; property to an empty value.
    component = *(%x01-09 / %x0B-2C / %x2E-7E) (%x01-08 / %x0B-1F / %x21-2C / %x2E-7E)

    ; Space is defined as a character for which isspace() is true for the
    ; C locale, i.e. space, form-feed ('\f'), newline ('\n'), carriage
    ; return ('\r'), horizontal tab ('\t') and vertical tab ('\v').
    space = %x09-0D / %x20

Macro and setting names are **case insensitive**, and components
reference previously set macros.

.. _file-format-casrc-component-format:

Component format
~~~~~~~~~~~~~~~~

In casrc files, a component is one of:

* A macro expansion, if the full component is the name of a macro.
* Otherwise, a property diff.

A property diff has the following format:

.. code-block:: abnf

    ; Note that whitespace at the beginning and end of the component have
    ; already been trimmed from line parsing.
    property-diff = name [*WSP ("=" / ":") * WSP value]

    ; Exclude U+000A (LF), U+003A (":"), U+003D ("="), and forbid
    ; U+0009 (\t) and U+0020 (SP) to be the last character.
    ; Empty name is valid.
    name = [*(%x01-09 / %x0B-3C / %x3E-7E) (%x01-08 / %x0B-1F / %x21-3C / %x3E-7E)]

    ; The value however can contain any character except U+000A (LF), and is
    ; also allowed to be empty.
    value = *(%x01-09 / %x0B-7E)

If the name of the property in the diff starts with ``no-``, then the prefix
is removed and the property diff instructs to "unset" the property.
Otherwise, the property diff instructs to "set" the property to the value.

If no value is present, it is considered to be the empty string.

Example property diffs are the following:

``msg = hello world``
    Set the ``msg`` property to ``hello world``.

``empty``, ``empty=``
    Set the ``empty`` property to the empty string.

``no-superheroes``, ``no-superheroes= ignored``
    Unset the ``superheroes`` property.

File examples
~~~~~~~~~~~~~

An example casrc file is the following:

.. code-block:: text

    macro my-macro msg=hello world,wow
    macro my-second-macro oh-yeah, my-macro

    ; this is a full line comment
    my-key the-value
    my-second-key       my-macro
    my-third.key  some=things,my-second-macro
    my-third.key no-some

The casrc file above defines the following settings:

* ``my-key``: ``the-value=``;
* ``my-second-key``: ``msg=hello world``, ``wow=``.
* ``my-third.key``: ``oh-yeah=``, ``msg=hello world``, ``wow=``
  (since ``some=things`` is cancelled out by ``no-some`` at the end).

.. _file-format-casrc-available-settings:

Available settings and properties
---------------------------------

This section inventories known keys.

.. _file-format-casrc-model-setting:

``model`` -- Calculator model
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Properties are tested on their presence rather than on their value,
in the following order:

``fx7700``, ``cfx7700``, ``7700``, ``fx7``, ``cfx7``, ``7``
    CASIO fx-7700G (1991 - 1993).

``fx9700``, ``cfx9700``, ``9700``, ``fx9``, ``cfx9``, ``9``
    CASIO FX-9700GH (1995 - 1997).

``fx9750``, ``cfx9750``, ``9750``
    CASIO FX-9750G (1997 - 1999).

``fx9800``, ``cfx9800``, ``9800``, ``fx8``, ``cfx8``, ``8``
    CASIO CFX-9800G (1995 - 1996).

``fx9850``, ``cfx9850``, ``9850``, ``fx5``, ``cfx5``, ``5``
    CASIO CFX-9850G (1996 - 1998).

``fx9950``, ``cfx9950``, ``9950``
    CASIO CFX-9950G (1996 - 1998).

``any``, ``*``
    Generic model.

``in``, ``out`` -- Input or output configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``ctf``
    If set, the input or output will be a local file with
    Calculator Text Format (CTF).

``dos``
    If set, the input or output will be DOS text (only
    recommended with ``-l``).

``cas``
    If set, the input or output will be a CASIOLINK program (``.cas``).

``fxp``
    If set, the input or output will be an FXP program (``.fxp``).

``bmp``
    If set, the input or output will be an a `Bitmap (.bmp) image`_.

``gif``
    If set, the input or output will be a `GIF image`_.

``com``
    If set, the input or output will be a serial port.

``tokfile``
    Token file path, relative to the casrc.

``in.ctf``, ``out.ctf`` -- CTF-specific input or output configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These settings are merged with the ``in`` or ``out`` configuration if the
selected input or output format is Calculator Text Format (CTF).

``glossary``
    If set, write the glossary to the output file.

``nice``
    Use the "nice" token (``TOKEN_OUT | TOKEN_NICE``) instead of
    the "str" token (``TOKEN_OUT | TOKEN_STR``).

    .. todo:: Add detail regarding this!

``in.cas``, ``out.cas`` -- CASIOLINK-specific input or output configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These settings are merged with the ``in`` or ``out`` configuration if the
selected input or output format is CASIOLINK (CAS).

``7700``, ``9700``, ``9800``
    If any is set, read or write with compatibility with CASIOLINK files
    produced for the CASIO fx-7700G.

``9750``, ``9850``, ``9950``
    If any is set, read or write with compatibility with CASIOLINK files
    produced for the CASIO fx-9750G.

``raw``, ``uncooked``
    If any is set, read or write with compatibility with
    raw CASIOLINK files.

``status``
    If set, emit a message when a block is read or written.

``order``
    Order in which to write a fx-9700G ``DC`` frame.

    .. todo:: Add details regarding this!

``in.fxp``, ``out.fxp`` -- FXP-specific input or output configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These settings are merged with the ``in`` or ``out`` configuration if the
selected input or output format is FXP.

No specific properties are available for this format.

``in.bmp``, ``out.bmp`` -- Bitmap-specific input or output configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These settings are merged with the ``in`` or ``out`` configuration if the
selected input or output format is Bitmap.

``inv``, ``inverse``
    If set, whether to read or write BMP in inverse.

``in.gif``, ``out.gif`` -- GIF-specific input or output configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These settings are merged with the ``in`` or ``out`` configuration if the
selected input or output format is GIF.

``inv``, ``inverse``
    If set, whether to write the GIF in inverse.

``in.com``, ``out.com`` -- Serial-specific input or output configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These settings are merged with the ``in`` or ``out`` configuration if the
selected input or output format is a serial port.

``7700``, ``9700``, ``9800``
    If set, use the fx-7700G header and payload format.

``9750``, ``9850``, ``9950``
    If set, use the fx-9750G header and payload format.

``raw``
    If set, use the raw header and payload format.

``parity``
    Parity to set to the serial connection:

    * ``even`` or ``e``: even parity.
    * ``odd`` or ``o``: odd parity.
    * ``none`` or ``n``: no parity.

    If none of the above are matched, the parity is set to even.

``baud``
    Baud rate to set to the serial connection, as exact string matches:

    * ``1200``: 1200 bauds.
    * ``2400``: 2400 bauds.
    * ``4800``: 4800 bauds.
    * ``9600``: 9600 bauds.

    By default, the baud rate is set to 9600 bauds.

``dtr``
    If set, enable DTR on the serial connection.

``rts``
    If set, enable RTS on the serial connection.

``pause``
    If set, require an interactive confirmation before initializing
    the communication with the calculator.

``inline``
    If set, initialize the communication with 0x15 ("inline") or
    0x16 ("ready").

    .. todo:: Link to the right section for this!

``overwrite``
    If set, in case of conflict, automatically overwrite.

``in.bin``, ``out.bin`` -- Binary-specific input or output configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These settings are merged with the ``in`` or ``out`` configuration if the
selected input or output format is raw binary.

No specific properties are available for this format.

``list`` -- List configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These settings are used for listing an obtained dump, or listing any entry
within this dump.

``dump``
    Use the dump format for listing contents.

``list.<type>`` -- Type-specific list configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The settings from both ``list`` and the ``list.<type>``, where ``<type>``
is the type of the entry that is being listed, are merged before listing
an entry.

``spc``, ``space``
    Use the space format for numbers, i.e. add a space before.

``num``
    Number format in the output.

    Available values are the following:

    ``oct``, ``octal``
        Use the octal format for numbers, and add a space before.

    ``dec``, ``decimal``
        Use the decimal format for numbers, and add a space before.

    ``hex``, ``hexadecimal``
        Use the hexadecimal format for numbers, and add a space before.

``oct``, ``octal``
    If set, equivalent to setting ``num=oct``.

``dec``, ``decimal``
    If set, equivalent to setting ``num=dec``.

``hex``, ``hexadecimal``
    If set, equivalent to setting ``num=hex``.

``nice``
    Whether to use nice display.

    .. todo:: Write an example here!

``pw``, ``password``
    Whether to display the password for the program, or not.

``shownext``
    If set, this shows the next segment of data.

    .. todo:: What does that mean?

.. _Bitmap (.bmp) image: https://en.wikipedia.org/wiki/BMP_file_format
.. _GIF image: https://en.wikipedia.org/wiki/GIF
