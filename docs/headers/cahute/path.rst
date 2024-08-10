.. _header-cahute-path:

``<cahute/path.h>`` -- Path related utilities for Cahute
========================================================

Macro definitions
-----------------

``CAHUTE_PATH_TYPE_*`` are constants representing the type of the path
representation.

.. c:macro:: CAHUTE_PATH_TYPE_CLI

    Alias for the path type assumed to be passed via command-line.
    This is portable for all officially supported platforms.

.. c:macro:: CAHUTE_PATH_TYPE_POSIX

    POSIX path, as used by standard POSIX file APIs.

    This path representation has the following properties:

    * Is terminated by a single NUL byte (``\0``);
    * Path components are separated by a single-byte ``/`` (U+002F) separator
      regardless of the locale;
    * Uses an ASCII, UTF-8 or other multibyte character encoding based on the
      user's locale;
    * There is no device at the start of the path.

    Example paths using this type are the following:

    ``/usr/bin/p7``
        Absolute path using ASCII / UTF-8.

    ``mydir/myfile.ext``
        Relative path using ASCII / UTF-8.

.. c:macro:: CAHUTE_PATH_TYPE_DOS

    DOS path, as used by MS-DOS or ROM-DOS (used by the AFX / Graph 100).

    This path representation has the following properties:

    * Is terminated by a single NUL byte (``\0``);
    * Path components are separated by a single-byte ``\`` (U+005C) separator
      regardless of the locale;
    * Path components must observe the 8:3 rule, i.e. respect the following
      regular expression: ``[^.]{1,8}(\.[^.]{1,3})``;
    * Uses an ASCII or ISO-8859-* encoding based on the user's locale;
    * Path may be prefixed by a single-character device name followed by
      a single-byte ``:`` (U+003A), being an uppercase latin letter from ``A``
      to ``Z``.

    Example paths using this type are the following:

    ``\mydir\myfile.ext``
        Absolute path on the current device.

    ``A:\mydir\myfile.ext``
        Absolute path on device ``A:``.

    ``mydir\myfile.ext``
        Relative path on the current device.

    ``A:mydir\myfile.ext``
        Path relative to the working directory on device ``A:``.

    .. warning::

        For security, when validating, legacy devices, i.e. special names
        with any extension, must be forbidden. As described in
        `Naming Files, Paths and Namespaces`_, the list is the following:

        .. code-block:: text

            CON, PRN, AUX, NUL, COM0, COM1, COM2, COM3, COM4, COM5, COM6, COM7,
            COM8, COM9, COM¹, COM², COM³, LPT0, LPT1, LPT2, LPT3, LPT4, LPT5,
            LPT6, LPT7, LPT8, LPT9, LPT¹, LPT², and LPT³

        Note that 8-bit superscript digits ``¹``, ``²``, and ``³`` are
        treated as digits and, thus, can identify legacy DOS devices.

.. c:macro:: CAHUTE_PATH_TYPE_WIN32_ANSI

    Win32 / Windows NT path, including UNC paths, as used natively under
    Windows 2000 or above.

    If using UNC, i.e. if the path starts with ``\\`` (two U+005C bytes),
    this path representation has the following properties:

    * Is terminated by a single NUL byte (``\0``);
    * Path components are separated by a single-byte ``\`` (U+005C) or
      ``/`` (U+002F) separator, used interchangeably;
    * Uses an ASCII, UTF-8 or other multibyte character encoding based on the
      user's locale;
    * The path starts with ``\\([^\]+)\([^\]+)``, in which the capturing
      groups represent both the

    Otherwise, this path representation has the same properties as for
    :c:macro:`CAHUTE_PATH_TYPE_DOS`, except that it does not have the
    8:3 path component limitation.

    Example paths using this type are the following:

    ``\mydir\myfile.ext``
        Absolute path on the current device.

    ``A:\mydir\myfile.ext``
        Absolute path on device ``A:``.

    ``mydir\myfile.ext``
        Relative path on the current device.

    ``A:mydir\myfile.ext``
        Path relative to the working directory on device ``A:``.

    ``\\myhostname\myshare\mydir\myfile.ext``
        Absolute path on share ``myshare`` of server ``myhostname``.

    ``\\.\A:\mydir\myfile.ext``
        Absolute path on device ``A:``.

    ``\\.\Volume{b75e2c83-0000-0000-0000-602f00000000}\mydir\myfile.ext``
        Absolute path on device
        ``Volume{b75e2c83-0000-0000-0000-602f00000000}``.

    ``\\?\A:\mydir\myfile.ext``
        Absolute path on device ``A:``.

    .. warning::

        When normalizing, DOS devices must be detected and forbidden.
        This includes their DOS form, e.g. ``mydir\COM1.txt\myfile.ext``,
        or their UNC form, e.g. ``\\.\COM1`` or ``\\?\COM1``.

.. c:macro:: CAHUTE_PATH_TYPE_WIN32_UNICODE

    Win32 / Windows NT path, including UNC paths, as used natively under
    Windows 2000 or above.

    This is equivalent to :c:macro:`CAHUTE_PATH_TYPE_WIN32_UNICODE`, except
    the character encoding is fixed to UTF-16 using the system's endianness.

.. c:macro:: CAHUTE_PATH_TYPE_CASIOWIN

    CASIOWIN path using variable-size encoding, as used by CASIOWIN on
    fx-9860G, fx-CP, fx-CG and derivatives.

    This path representation has the following properties:

    * Is terminated by a single NUL byte (``\0``);
    * Path components are separated by a single-byte ``\`` (U+005C) separator;
    * Path components must observe the 8:3 rule, i.e. respect the following
      regular expression: ``[^.]{1,8}(\.[^.]{1,3})``;
    * Uses :ref:`text-encoding-fc8` for both the device and path components;
    * The path may start with ``\\([^\]+)``, which represents the device on
      which the file is located, e.g. ``\\fls0``.

    Example paths of this type are the following:

    ``mydir\myfile.ext``
        Relative path.

    ``\mydir\myfile.ext``
        Absolute path on current device.

    ``\\fls0\mydir\myfile.ext``
        Absolute path on device ``fls0``.

.. c:macro:: CAHUTE_PATH_TYPE_CASIOWIN_16

    CASIOWIN path using fixed-width encoding, as used by CASIOWIN's SDK on
    fx-9860G, fx-CP, fx-CG and derivatives.

    This is equivalent to :c:macro:`CAHUTE_PATH_TYPE_CASIOWIN`, except the
    character encoding is fixed to :ref:`text-encoding-fc16` using the
    system's endianness instead of :ref:`text-encoding-fc8`.

Function declarations
---------------------

.. c:function:: int cahute_find_path_extension(char *buf, \
    size_t buf_size, void const *path, int path_type)

    Find the extension, in ASCII lowercase, of the file designated by the
    provided path.

    Note that in the case of extensions known as "modifiers", e.g. ``.bz2``
    or ``.gz``, the function will attempt at including the extension found
    before, in order to obtain e.g. ``tar.gz`` or ``g1m.bz2``.

    An example usage of this function is the following:

    .. code-block:: c

        char buf[10];

        cahute_find_path_extension(
            buf,
            sizeof(buf),
            "/home/david/ARCHIVE.G1M",
            CAHUTE_PATH_TYPE_POSIX
        );

        printf("Extension: %s\n", buf); /* "Extension: g1m" */

    :param buf: Buffer in which to place the extension, in ASCII lowercase.
    :param buf_size: Capacity / size of the buffer to write into, including
        the NUL terminator.
    :param path: Path to find the extension in.
    :param path_type: Type of the path.
    :return: Error, or 0 if successful.

.. _File path formats on Windows systems:
    https://learn.microsoft.com/en-us/dotnet/standard/io/file-path-formats
.. _Naming Files, Paths and Namespaces:
    https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file
