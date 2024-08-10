.. _header-cahute-file:

``<cahute/file.h>`` -- File related utilities for Cahute
========================================================

Macro definitions
-----------------

``CAHUTE_FILE_TYPE_*`` are constants representing the type of an opened file.

.. c:macro:: CAHUTE_FILE_TYPE_ADDIN_CG

    fx-CG add-in; see :ref:`file-format-g3a` for more information.

.. c:macro:: CAHUTE_FILE_TYPE_ADDIN_FX

    fx add-in; see :ref:`file-format-g1a` for more information.

.. c:macro:: CAHUTE_FILE_TYPE_CASIOLINK

    CASIOLINK archive, that can be used as a main memory archive;
    see :ref:`file-format-casiolink` for more information.

.. c:macro:: CAHUTE_FILE_TYPE_EACT_FX

    fx e-Activity; see :ref:`file-format-g1e` for more information.

.. c:macro:: CAHUTE_FILE_TYPE_FKEY_FX

    fx function keys file; see :ref:`file-format-g1n` for more information.

.. c:macro:: CAHUTE_FILE_TYPE_FKEY_CG

    fx-CG function keys file; see :ref:`file-format-g3n` for more information.

.. c:macro:: CAHUTE_FILE_TYPE_LANG_CG

    fx-CG language file; see :ref:`file-format-g3l` for more information.

.. c:macro:: CAHUTE_FILE_TYPE_LANG_FX

    fx language file; see :ref:`file-format-g1l` for more information.

.. c:macro:: CAHUTE_FILE_TYPE_MAINMEM

    fx main memory archive; see :ref:`file-format-mainmem` for more
    information.

.. c:macro:: CAHUTE_FILE_TYPE_PICTURE_CG

    fx-CG picture; see :ref:`file-format-g3p` for more information.

.. c:macro:: CAHUTE_FILE_TYPE_PICTURE_CP

    fx-CP picture; see :ref:`file-format-c2p` for more information.

Type definitions
----------------

.. c:struct:: cahute_file

    Opened file for reading or writing.

    This type is opaque, and such resources must be created using
    :c:func:`cahute_open_file`.

Function declarations
---------------------

.. c:function:: int cahute_open_file_for_reading(cahute_file **filep, \
    void const *path, int path_type)

    Open a file from a path, in order to read it.

    :param filep: Pointer to the file object to create.
    :param path: Path to the file to open, with the file type.
    :param path_type: Type of the path to the file to open.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_open_file_for_export(cahute_file **filep, \
    size_t size, void const *path, int path_type)

    Open a file from a path, in order to write its content.

    :param filep: Pointer to the file object to create.
    :param size: Size of the file to create or open.
    :param path: Path to the file to open, with the file type.
    :param path_type: Type of the path to the file to create or open.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_open_stdout(cahute_file **filep)

    Open standard output as a file, in order to write to it.

    .. warning::

        A file obtained through this method **must be closed** using
        :c:func:`cahute_close_file`, just like files opened using
        :c:func:`cahute_open_file`. It will always return a new
        file instance.

    :param filep: Pointer to the file object to create.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_get_file_size(cahute_file *file, \
    unsigned long *sizep)

    Get the total size of the file.

    :param file: File object.
    :param sizep: Pointer to the integer to set with the file size.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_read_from_file(cahute_file *file, \
    unsigned long off, void *buf, size_t size)

    Read from the file starting at a given offset.

    :param file: File object.
    :param off: Offset at which to read.
    :param buf: Buffer in which to write the resulting data.
    :param size: Size of the data to read.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_write_to_file(cahute_file *file, \
    unsigned long off, void const *data, size_t size)

    Write to the file starting at a given offset.

    :param file: File object.
    :param off: Offset at which to read.
    :param buf: Buffer in which to write the resulting data.
    :param size: Size of the data to read.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_guess_file_type(cahute_file *file, int *typep)

    Get the type of a file, in order to read it.

    :param file: File object.
    :param typep: Value to define with the determined type for the file.
    :return: Error, or 0 if the operation was successful.

.. c:function:: void cahute_close_file(cahute_file *file)

    Close a file.

    :param file: File object to close.
