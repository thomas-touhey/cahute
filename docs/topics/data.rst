Main memory data
================

Main memory files are the files present on the calculator's main memory, which
is present on all graphic CASIO calculators since the fx-7000G at least.
They can be programs, captures, and so on, and are small enough to fit in
the main memory, usually stored in RAM in a filesystem no bigger than 64 KiB.

Most elements in the CASIO ecosystem are thought around exchanging these
files:

* The CASIOLINK protocol is about sending and receiving main memory files.
  See :ref:`protocol-casiolink` for more information;
* Part of the commands in Protocol 7.00 are about sending and receiving
  main memory files, and interacting with the main memory in general.
  See :ref:`protocol-seven-casio-commands` for more information;
* A lot of the file formats documented here are about containing one or
  more main memory files:

  - CASIOLINK files are a binary collection of main memory files as represented
    using their protocol counterpart. See :ref:`file-format-casiolink` for
    more information;
  - CAT and CTF are textual equivalents of the previous, one being official
    and the other being defined by the community. See :ref:`file-format-cat`
    and :ref:`file-format-ctf` for more information;
  - MCS (Main Control Structure) files, with the G1M/G1R/G2M/G2R/G3M/G3R
    extensions, are a binary collection of main memory files for the fx-9860G
    and compatible. See :ref:`file-format-g1m`, :ref:`file-format-g2r`
    and :ref:`file-format-g3m` for more information.

Main memory files usually have an associated data type and a format.

.. todo:: Catalog known main memory file natures here: program, ...

.. _data-program:

Programs
--------

.. todo:: Write about this.
