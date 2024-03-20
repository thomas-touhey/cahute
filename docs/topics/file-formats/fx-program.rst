.. _file-format-fxp:

FX Program (FXP)
================

This format is a community format described and implemented by Bob Parish
in 1996, along with a Microsoft Quick Basic program. It is also supported by
CaS.

It can only contain an unnamed and unnumbered program, represented using
the following grammar:

.. code-block:: abnf

    file = length LF *(byte LF) '0' LF

    length = 1*%x30-39
    byte = 1*%x30-39

The content is a CASIOLINK data part for a program, meaning:

* The first byte is ``:`` (``0x3A``);
* The last byte is the checksum of the second until the one before the
  checksum.
* The rest is the program.
