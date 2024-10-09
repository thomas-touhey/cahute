.. _file-format-casiolink:

CASIOLINK archives
==================

CASIOLINK archives are main memory archives for CASIO calculators. They usually
end with the ``.CAS`` extension.

The file is a series of main memory files directly concatenated, where every
file is constituted of the following:

* A header, composed of:

  * A ``:`` (0x3A) byte.
  * A header, of CAS40 or CAS50 type, including a checksum.
* Zero, one, or more data parts, depending on the header, composed of:

  * A ``:`` (0x3A) byte.
  * A contents, of the size provided by the header.
  * A checksum (1 byte).

The header format depends on the model of the calculator from or for which
the file was made.

CAS40 main memory file format
-----------------------------

This header format is used on pre-1996 calculators, and ``.cas`` files produced
by these calculators. Such calculators include:

* CASIO fx-7700G (1991-1993);
* CASIO fx-9700GH (1995-1997);
* CASIO CFX-9800G (1995-1996).

See :ref:`cas40-data-types` for more information.

CAS50 main memory file format
-----------------------------

This header format is used on post-1996 calculators up until 2004, excluding
the AlgebraFX and compatible, and ``.cas`` files produced by these calculators.
Such calculators include:

* CASIO CFX-9850G (1996-1998);
* CASIO CFX-9950G (1996-1998);
* CASIO fx-9750G (1997-1999).

See :ref:`cas50-data-types` for more information.
