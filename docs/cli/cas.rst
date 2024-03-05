.. _cas:

``CaS`` command-line reference
==============================

CaS is a originally a transfer and conversion program made by Tom Wheeley and
Tom Lynn in 1997. It systematically operates the following way:

* It reads data from an input, which is either a file or a calculator connected
  using a serial port.
* It applies conversions "before" listing.
* If listing is enabled by using option ``-l``, it lists data using the
  provided formatting flags.
* It applies conversions "after" listing.
* If output is enabled by using option ``-o``, tt writes data to an output,
  which is either a file or a calculator connected using a serial port.

The syntax of the command is the following:

.. code-block:: text

    CaS [options...] <input file or device name>

The command-line parsing in CaS is different from other utilities, in:

* **Short options do not combine** within the same single dash argument
  component, i.e. ``-HV`` will ignore the ``V``.
* Both short and long options take an attribute, a parameter, or both at
  the same time. The attribute can be set using one of the following
  syntaxes:

  .. code-block:: text

      -o<attribute>
      -o=<attribute>
      -o:<attribute>
      --long-option=<attribute>
      --long-option:<attribute>

  While the option parameter must be passed in the next argument in the
  array, i.e.:

  .. code-block:: text

      -o <parameter>
      --long-option <parameter>

  Both can be combined with CaS, e.g. you can have the following:

  .. code-block:: text

      -o<attribute> <parameter>
      -o:<attribute> <parameter>
      --long-option:<attribute> <parameter>

  For convenience, attributes will be present below with the ``-o=<attribute>``
  and ``--option=<attribute>`` formats.

Default options are defined in the casrc file; see :ref:`file-format-casrc`
for more information.

General options are the following:

``-h``, ``-?``
    *Long form:* ``--help``

    Display the help message and exit.

``-V``
    *Long form:* ``--version``

    Display the version message and exit.

``-v``
    *Long form:* ``--verbose``

    Whether to display the CaS banner or not before doing anythng else.

``-d[=<file>]``
    *Long form:* ``-debug[=<file>]``

    Whether to display debug information.

    By default, this information is printed on standard error, but it can
    also be placed in a file (truncating it first).

Pipeline-related options are the following:

``-i=[<format>,]<args>``
    *Long forms:* ``--input=[<format>,]<args>``,
    ``--infile=[<format>,]<args>``

    Input type and additional components to apply to the ``in`` setting,
    before merging it to the ``in.<format>`` queue.

    Setting this option **clears** the ``in`` setting before defining it.

    See :ref:`file-format-casrc-component-format` and
    :ref:`file-format-casrc-available-settings` for more information.

    If no format setting is provided, the format of the source file or device
    will be automatically detected if possible.

``-c=<conversion>``
    *Long form:* ``--convert=<conversion>``

    Conversions to apply **before** listing, of the form
    ``<source type>-<destination type>``.

``-l[=<format>]``
    *Long forms:* ``--list[=<format>]``, ``--display[=<format>]``

    Additional components to apply to the ``list`` setting, before merging
    it to the ``list`` queue.

    Setting this option enables listing in the command execution.

    See :ref:`file-format-casrc-component-format` and
    :ref:`file-format-casrc-available-settings` for more information.

``-t``
    *Long form:* ``--terse``

    Whether to display the types of the files in the input, even if
    listing is disabled.

``-C=<conversion>``
    *Long form:* ``--convert-after=<conversion>``

    Conversions to apply **after** listing, of the form
    ``<source type>-<destination type>``.

``-o[=[<format>,]<args>] <file or device name>``
    *Long forms:* ``--output[=[<format>,]<args>] <file or device name>``,
    ``--outfile[=[<format>,]<args>] <file or device name>``

    Output type and additional components to apply to the ``out`` setting,
    before merging it to the ``out.<format>`` queue.

    Setting this option has the following effects:

    * It **clears** the ``out`` setting before defining it.
    * It enables output in the command execution.

    See :ref:`file-format-casrc-component-format` and
    :ref:`file-format-casrc-available-settings` for more information.

Other options are the following:

``-e``
    *Long form:* ``--castle``

    *(deprecated)* Communicate with the Castle IDE to format the input
    and, if enabled, the output.

``-p``
    *Long form:* ``--pager``

    Whether to use a pager_.

``-m=<model>``
    *Long form:* ``--model=<model>``

    Calculator model with which to interact, as properties to apply to the
    ``model`` setting as a list of components.

    Setting this option **clears** the ``model`` setting before defining it.

    See :ref:`file-format-casrc-component-format` and
    :ref:`file-format-casrc-model-setting` for more information.

.. _Pager: https://en.wikipedia.org/wiki/Terminal_pager
