.. _file-format-graphcard:

GraphCard file format
=====================

The GraphCard_ was an external storage device for CASIO graphing calculators
made by Util-Pocket, a now defunct vendor.
It communicated with the calculator using a serial cable, and stored a
data bulk on an SD card under the form of GraphCard main memory archives,
which usually end with the ``.GRC`` file name extension.

In order to transfer one or more main memory files to the GraphCard, you could
simply send them from the calculator. However, in order to receive the same
bulk from the GraphCard, the user would need to download a special
``CARDLIST`` program, modify it to select the bulks to download, and
send the file; then the GraphCard would try to send the requested files.

A ``.GRC`` is the representation of a main memory archive, as a bulk of
contents. Every memory file in the archive is composed of the following:

* A big endian, 16-bit unsigned integer representing the size of the next
  entry.
* A main memory file of the data for which the length is the one above,
  of CASIOLINK format; see :ref:`file-format-casiolink` for more information.

.. _GraphCard:
    https://web.archive.org/web/20230125095116/
    http://www.util-pocket.com/casio/index.htm#graphcard
