.. _topic-files:

Files
=====

Cahute supports interacting with the local filesystem to read and export files.
This abstraction allows command-line utilities to be portable, as well as
support all file formats surrounding CASIO calculators, documented in
:ref:`topic-file-formats`.

All files are represented by the :c:type:`cahute_file` type. They are opened
and used differently depending on the situations, but must always be closed
after usage using :c:func:`cahute_close_file`.

.. note::

    A file is considered to be handled by Cahute exclusively.
    When reading, the size is computed once on opening the file, then
    considered to stay the same during the file handle lifetime.

File reading
------------

For reading a file using Cahute, one must first open the file using
:c:func:`cahute_open_file`. If this function succeeds, many operations
can then be used, as described in the following subsections.

Manual file reading
~~~~~~~~~~~~~~~~~~~

One can read raw data from the file by using the following functions:

* :c:func:`cahute_get_file_size` to get the computed file size;
* :c:func:`cahute_read_from_file` to read bytes from the file, starting at a
  provided offset.

.. note::

    :c:func:`cahute_get_file_size` and :c:func:`cahute_read_from_file`
    use ``unsigned long`` to represent file sizes and offset.

    The value is guaranteed to fit, since if the file was too large for the
    size to fit in an ``unsigned long``, :c:func:`cahute_open_file` would
    have failed with :c:macro:`CAHUTE_ERROR_SIZE`.

Type guessing
~~~~~~~~~~~~~

One can use :c:func:`cahute_guess_file_type` to determine the file type,
as :ref:`one of the CAHUTE_FILE_TYPE_* macros <cahute-file-type-macros>`.

Main memory data reading
~~~~~~~~~~~~~~~~~~~~~~~~

One can use :c:func:`cahute_get_data_from_file` to extract main memory data
from the file, as described in :ref:`topic-main-memory-data`.

Raw file creation
-----------------

For creating a raw file using Cahute, one must first create and open the
file using :c:func:`cahute_create_file`, while specifying the path and size
of the file to create.

.. warning::

    For performance and compatibility reasons, the resulting file contents is
    not guaranteed to contain anything particular. If this method is used to
    create an all-zeroes file, the contents of the file must be manually set.

From here, the following functions can be used:

* :c:func:`cahute_get_file_size`, to retrieve the file size provided at
  file creation;
* :c:func:`cahute_write_to_file` to write bytes in the file, starting at
  a provided offset.

.. note::

    For command-line utilities that support writing the output directly to
    standard output, :c:func:`cahute_open_stdout` can also be used to create
    a file handle. There are however two limitations this brings:

    * :c:func:`cahute_get_file_size` will not be available anymore;
    * Only increasing offsets will be accepted with such a file handle, and
      skipping bytes will mean the skipped bytes will be emitted as zeroes.
