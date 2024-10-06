File internals
==============

This document describes the internals behind files; see :ref:`topic-files` for
more information about the user-facing abstractions.

A file only requires one memory allocation (except for system resources that
are allocated / opened using different functions), and the medium is
initialized with the file opening functions.

Mediums
-------

Mediums define a common set of interfaces that can be used by the rest of
the library to read or write on the opened file, thereby exploiting the
system interfaces.

A medium is represented by the following type:

.. c:struct:: cahute_file_medium

    File medium representation.

    This structure is usually directly allocated with the file, i.e.
    :c:type:`cahute_file` instance, and is accessed through ``file->medium``.

    .. c:member:: int type

        Type of medium, among the ``CAHUTE_FILE_MEDIUM_*`` constants documented
        in :ref:`internals-file-available-mediums`.

    .. c:member:: unsigned long flags

        Flags, which represent the kind of operations the underlying medium
        can do, influencing how the generic parts of the medium interactions
        will behave, among:

        .. c:macro:: CAHUTE_FILE_MEDIUM_FLAG_WRITE

            Whether the medium is writable, i.e. writing to the medium type
            is implemented and the underlying resources have been opened
            in a way that allows writing.

        .. c:macro:: CAHUTE_FILE_MEDIUM_FLAG_READ

            Whether the medium is readable, i.e. reading to the medium type
            is implemented and the underlying resources have been opened
            in a way that allows reading.

        .. c:macro:: CAHUTE_FILE_MEDIUM_FLAG_SEEK

            Whether the medium is seekable, i.e. seeking on the medium type
            is implemented and the underlying resources have been opened
            in a way that allows seeking.

        .. c:macro:: CAHUTE_FILE_MEDIUM_FLAG_SIZE

            Whether the size has been computed on opening the medium, i.e.
            :c:member:`cahute_file_medium.file_size` is exploitable.

    .. c:member:: unsigned long offset

        Current offset of the underlying resource, if relevant.
        This is used on writing to a stream, whether seekable or not,
        and on refreshing the read buffer if need be.

    .. c:member:: unsigned long file_size

        File size, computed when the file was being opened.

    .. c:member:: unsigned long read_offset

        Offset of the current read buffer.

    .. c:member:: size_t read_size

        Size of the data contained within the read buffer.

        .. warning::

            Note that this only represents the number of bytes that are
            actually set to something exploitable in :c:member:`read_buffer`,
            **not** the read buffer capacity.

    .. c:member:: cahute_u8 *read_buffer

        Read buffer, for which the main purpose is to serve as a cache.

        In normal circumstances, this buffer is
        :c:macro:`CAHUTE_FILE_MEDIUM_READ_BUFFER_SIZE` bytes long.

    .. c:member:: union cahute_file_medium_state state

        State of the file medium, which contains the underlying resources
        depending on the file medium type.

Medium interface
~~~~~~~~~~~~~~~~

Mediums support a generic memory read/write interface with the following
functions:

.. c:function:: int cahute_read_from_file_medium(cahute_file_medium *medium, \
    unsigned long off, cahute_u8 *buf, size_t size)

    Read from the file medium, starting at a provided offset.

    Errors to be expected from this function are the following:

    :c:macro:`CAHUTE_ERROR_TRUNC`
        The parameters would lead to moving out-of-bounds, or reading at
        least one byte out-of-bounds.

    :param medium: Medium from which to read.
    :param off: Offset at which to start reading.
    :param buf: Buffer in which to store the read data.
    :param size: Size of the data to read.
    :return: Error, or :c:macro:`CAHUTE_OK`.

.. c:function:: int cahute_write_to_file_medium(cahute_file_medium *medium, \
    unsigned long off, void const *data, size_t size)

    Write to the file medium, starting at a provided offset.

    Errors to be expected from this function are the following:

    :c:macro:`CAHUTE_ERROR_SIZE`
        The parameters would lead to moving out-of-bounds, or writing at
        least one byte out-of-bounds.

    :param medium: Medium into which to write.
    :param off: Offset at which to start writing.
    :param data: Data to write.
    :param size: Size of the data to write.
    :return: Error, or :c:macro:`CAHUTE_OK`.

Internal medium logic
~~~~~~~~~~~~~~~~~~~~~

The internal logic for file mediums is implemented in ``lib/filemedium.c``.
While the medium interface presents a memory-like interface, most internal
mediums actually work using streams with a current offset that is updated
when making a read, write or seek operation.

.. note::

    This implementation is optimized for reading with increasing file offsets,
    since the rationale behind most file formats allows us to do this.

This section documents the internal logics behind the interface functions.

:c:func:`cahute_read_from_file_medium`
    First, we check if there is an intersection between our current read
    buffer and the requested data on the left boundary. If there is, we copy
    the intersection into the user-provided buffer.

    If there is still some data to read, this means we need to refresh the
    read buffer at least once, i.e. we need to move the underlying cursor
    to match the first byte we want to read. If the cursor is not already at
    the correct position, this is done by one of these methods:

    * If seeking is supported, we seek to that offset.
    * Otherwise, if the targeted offset is after the current offset, we
      read and ignore bytes from the underlying stream.
    * Otherwise, we fail, since we can't seek backwards in the stream.

    Once this is done, we do :c:macro:`CAHUTE_FILE_MEDIUM_READ_BUFFER_SIZE`
    bytes long reads until the user-provided buffer has been completely
    filled.

:c:func:`cahute_write_to_file_medium`
    There is no write buffering, so we directly want to check that we're
    at the right offset on the underlying cursor. If the cursor is not already
    at the correct position, this is done by one of these methods:

    * If seeking is supported, we seek to that offset.
    * Otherwise, if the targeted offset is after the current offset, we
      write zeroes and ignore bytes from the underlying stream.
    * Otherwise, we fail, since we can't seek backwards in the stream.

    Once this is done, we do :c:macro:`CAHUTE_FILE_MEDIUM_WRITE_CHUNK_SIZE`
    bytes long writes until the user-provided buffer has been completely
    written.

    We also check if there is an intersection between the user-provided
    boundaries and the read buffer boundaries, and if it's the case, write
    the user-provided data to the correct offset in the read buffer to ensure
    reads from the same offsets will return the updated data, and not the
    data before the write.

.. _internals-file-available-mediums:

Available medium types
~~~~~~~~~~~~~~~~~~~~~~

File medium types are represented as ``CAHUTE_FILE_MEDIUM_*`` constants
internally.

.. warning::

    The file medium constants are only represented **if they are available in
    the current configuration**. This is a simple way for medium-specific
    implementations to be defined or not, with ``#ifdef``.

Available mediums are the following:

.. c:macro:: CAHUTE_FILE_MEDIUM_NONE

    Internal in-memory file medium; see :ref:`internals-file-inmem` for
    more information.

.. c:macro:: CAHUTE_FILE_MEDIUM_POSIX

    POSIX file API medium, with a file descriptor (*fd*):

    * Closing using `close(2) <https://linux.die.net/man/2/close>`_;
    * Reading uses `read(2) <https://linux.die.net/man/2/read>`_;
    * Writing uses `write(2) <https://linux.die.net/man/2/write>`_;
    * Seeking uses `lseek(2) <https://linux.die.net/man/2/lseek>`_.

    Only available on platforms considered POSIX, including Apple’s OS X
    explicitely (since they do not define the ``__unix__`` constant like
    Linux does).

.. c:macro:: CAHUTE_FILE_MEDIUM_WIN32

    Serial medium using the Windows API, with a file |HANDLE|_:

    * Closing uses |CloseHandle|_;
    * Reading uses |ReadFile|_;
    * Writing uses |WriteFile|_;
    * Seeking uses |SetFilePointer|_.

    Only available with Windows.

.. _internals-file-inmem:

Internal in-memory file medium
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to work for both files and protocols, some functions such as
:c:func:`cahute_casiolink_decode_data` or :c:func:`cahute_mcs_decode_data`
take a :c:type:`cahute_file` instance. For abstracting memory buffers coming
from protocols as in-memory files, the following internal function is available
within the library:

.. c:function:: void cahute_populate_file_from_memory(cahute_file *file, \
    cahute_u8 *buf, size_t size)

    Populate a file handle from a buffer and a size.

    :param file: File object to populate.
    :param buf: Buffer to abstract as a file.
    :param size: Size of the buffer to abstract.

In order to avoid having too many memory allocations and since
:c:type:`cahute_file` is not opaque within the library, this utility can be
used to populate a statically defined file object which can then be transmitted
to other functions using files to decode data. It is also not necessary to
call :c:func:`cahute_close_file` in such a case.

For example, with :c:func:`cahute_casiolink_decode_data`:

.. code-block:: c

    cahute_file file;
    unsigned long offset = 0;

    cahute_populate_file_from_memory(&file, my_buf, my_buf_size);
    err = cahute_casiolink_decode_data(datap, &file, &offset, my_variant, 1);
    ...

In order to work, this abuses the read buffer to not be
:c:macro:`CAHUTE_FILE_MEDIUM_READ_BUFFER_SIZE` bytes long, but sized to
the whole "file", representing the buffer, with the read buffer actually being
the provided buffer **directly** with the :c:macro:`CAHUTE_FILE_MEDIUM_NONE`.

It abuses existing manipulations of the read buffer to read directly from the
read buffer, mirror written data in the read buffer to just write into the
buffer at the provided offset, and not have any side effects, i.e. the
operations become the following:

:c:func:`cahute_read_from_file_medium`
    There is **always** an intersection between our current read buffer
    and the requested data on the left boundary, so we read from the "read"
    buffer to copy data to the user-provided buffer.

:c:func:`cahute_write_to_file_medium`
    We do not move any underlying cursor or have any side-effect.

    There is **always** an intersection between the user-provided boundaries
    and the read buffer boundaries, so we write the user-provided data to the
    correct offset in the read buffer to ensure reads from the same offsets
    will return the updated data, and not the data before the write.

File opening behaviours
-----------------------

In this section, we will describe the behaviour of file opening functions.

:c:func:`cahute_open_file`
    Depending on the platform:

    * On POSIX and compatible, it attempts at opening the file
      using `open(2) <https://linux.die.net/man/2/open>`_.
      If this succeeds, it calls
      `lseek(2) <https://linux.die.net/man/2/lseek>`_ to seek 0 bytes from
      ``SEEK_END``, which returns the current file size, then uses the
      same function to seek 0 bytes from ``SEEK_SET``.

      The created file handle will have the :c:macro:`CAHUTE_FILE_MEDIUM_POSIX`
      medium type.
    * On Win32, it attempts at opening the file using |CreateFile|_.
      If this succeeds, it calls |SetFilePointer|_ to seek 0 bytes from
      ``FILE_END``, which returns the current file size, then uses the
      same function to seek 0 bytes from ``FILE_BEGIN``.

      The created file handle will have the :c:macro:`CAHUTE_FILE_MEDIUM_WIN32`
      medium type.
    * Otherwise, it will return :c:macro:`CAHUTE_ERROR_IMPL`.

    If the obtained file size is too big, i.e. more than
    :c:macro:`CAHUTE_MAX_FILE_OFFSET`, the function will fail with
    error :c:macro:`CAHUTE_ERROR_SIZE`.

:c:func:`cahute_create_file`
    Depending on the platform:

    * On POSIX and compatible, it attempts at creating and opening the file
      using `open(2) <https://linux.die.net/man/2/open>`_.
      If this suceeds, it calls
      `ftruncate(2) <https://linux.die.net/man/2/ftruncate>`_ to set
      the file size explicitely to the provided size.

      The created file handle will have the :c:macro:`CAHUTE_FILE_MEDIUM_POSIX`
      medium type.
    * On Win32, it attempts at creating and opening the file using
      |CreateFile|_.
      If this succeeds, it calls |SetFilePointer|_ to seek the provided file
      size from ``FILE_BEGIN``, calls |SetEndOfFile|_ to set the file size
      explicitely, then uses |SetFilePointer|_ again to seek to ``FILE_BEGIN``
      again.

      The created file handle will have the :c:macro:`CAHUTE_FILE_MEDIUM_WIN32`
      medium type.
    * Otherwise, it will return :c:macro:`CAHUTE_ERROR_IMPL`.

:c:func:`cahute_open_stdout`
    Depending on the platform:

    * On POSIX and compatible, it creates a file handle with medium type
      :c:macro:`CAHUTE_FILE_MEDIUM_POSIX` and *fd* set to ``1``.
    * On Win32, it calls |GetStdHandle|_ with ``STD_OUTPUT_HANDLE``.

      The created file handle will have the :c:macro:`CAHUTE_FILE_MEDIUM_WIN32`
      medium type.
    * Otherwise, it will return :c:macro:`CAHUTE_ERROR_IMPL`.

File metadata retrieval
-----------------------

The :c:type:`cahute_file` contains caching for the file metadata retrieval,
namely:

* The retrieved type, using one of the ``CAHUTE_FILE_TYPE_*`` macros defined
  in :ref:`header-cahute-file`;
* The extension extracted from the provided path.

For any of the file reading functions that requires file type and metadata,
if the :c:macro:`CAHUTE_FILE_FLAG_EXAMINED` flag is not present in the
file flags yet, the :c:func:`cahute_examine_file` function is called to
determine it and set the flag.

.. |HANDLE| replace:: ``HANDLE``
.. |CreateFile| replace:: ``CreateFile``
.. |GetStdHandle| replace:: ``GetStdHandle``
.. |ReadFile| replace:: ``ReadFile``
.. |WriteFile| replace:: ``WriteFile``
.. |SetFilePointer| replace:: ``WriteFile``
.. |SetEndOfFile| replace:: ``SetEndOfFile``
.. |CloseHandle| replace:: ``CloseHandle``

.. _HANDLE:
    https://learn.microsoft.com/en-us/windows/win32/sysinfo/handles-and-objects
.. _CreateFile:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-createfilea
.. _GetStdHandle:
    https://learn.microsoft.com/en-us/windows/console/getstdhandle
.. _ReadFile:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-readfile
.. _WriteFile:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-writefile
.. _SetFilePointer:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-setfilepointer
.. _SetEndOfFile:
    https://learn.microsoft.com/en-us/windows/win32/api/
    fileapi/nf-fileapi-setendoffile
.. _CloseHandle:
    https://learn.microsoft.com/en-us/windows/win32/api/
    handleapi/nf-handleapi-closehandle
