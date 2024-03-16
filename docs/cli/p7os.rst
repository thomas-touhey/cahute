.. _p7os:

``p7os`` command line reference
===============================

p7os is an experimental utility originally provided with p7utils_ in July 2017,
then fixed in libcasio_ (unreleased). It is used to interact with the
calculator's system, to run update programs.

.. warning::

    Usage of this utility is **dangerous**, and may brick your calculator.
    Ensure that you know what you're doing before running it.

    This interface is provided by compatibility with libp7 / libcasio, and
    must not be changed to bring new features or change the syntax of
    existing ones.

Available options for all subcommands are the following:

``-h``, ``--help``
    Display the help message for the command or subcommand and quit.

``-v``, ``--version``
    Display the version message and quit.

``-l``, ``--log``
    Logging level to set the library as, as any of ``info``, ``warning``,
    ``error``, ``fatal``, ``none``.

    This option may allow contributors to visualize the effects of their
    contribution better, and allows users to produce a full output to join
    to an issue.

    See :ref:`logging` for more information.

``-#``
    Flag to enable displaying of a loading bar to show transfer or
    procedure progress.

``--no-prepare``
    Do not upload the Update.EXE, suppose the correct one has already been
    uploaded and run by a previous command.

Invalid options are ignored by p7os. If an option is provided several times,
only the latest occurrence will be taken into account.

See the following sections for subcommand specification.

.. _p7os-prepare-only:

``prepare-only`` subcommand reference
-------------------------------------

This subcommand is used to only upload and run an Update.EXE file, and
not run any other options after.

The syntax is the following:

.. code-block:: text

    p7os prepare-only

This subcommand does not provide additional options.

.. _p7os-flash:

``flash`` subcommand reference
------------------------------

This subcommand is used to flash a new OS onto a calculator.

The syntax is the following:

.. code-block:: text

    p7os flash <os.bin>

Available options are the following:

``--erase-flash``
    Erase the flash.

.. _p7os-get:

``get`` subcommand reference
----------------------------

This subcommand is used to backup

The syntax is the following:

.. code-block:: text

    p7 get [-o <os.bin>]

Available options are the following:

``-o <os.bin>``, ``--output <os.bin>``
    Path to the image to build.

.. _p7utils: https://git.planet-casio.com/cake/p7utils
.. _libcasio: https://git.planet-casio.com/Lailouezzz/libcasio
.. _Thomas Touhey: https://thomas.touhey.fr/
