Internal compatibility utilities
================================

There are several sets of compatibility utilities within Cahute, playing
different roles, and offering a different set of possibilities:

* ``<cahute/cdefs.h>`` defines portability utilities that are also used in
  the public headers to the library. They must be usable by any compiler using
  the already compiled library, even if the library has been compiled using
  a different compiler (or the same compiler using different settings).

  Therefore, it **can't** use macros only defined at compile time by CMake,
  e.g. to determine the size of a type, since it may vary later when using a
  different compiler.
* ``<compat.h>`` defines portability utilities that are only used within
  the library or the command-line utilities.

  Since in a given build setup, CMake should only use one compiler, this
  part is allowed to use macros defined at compile time by CMake.
* ``lib/internals.h`` also contain portability utilities, including
  the endianness-related functions, that are only used within the library
  (and **not** within the command-line utilities).

This file references the internal macros and types defined for compatibility
here. For the public ones, see :ref:`header-cdefs`.

Macro definitions
-----------------

.. c:macro:: CAHUTE_LOCAL(TYPE)

    Macro to use in Cahute function definitions, surrounding the return type,
    as opposed to :c:macro:`CAHUTE_EXTERN`. For example::

        CAHUTE_LOCAL(int) my_local_utility(int arg1, char const *arg2);

    For now, this only produces the following output::

        static int my_local_utility(int arg1, char const *arg2);

.. c:macro:: CAHUTE_INLINE(TYPE)

    Macro to use in inlinable local Cahute function definitions, surrounding
    the return type. This macro extends on :c:macro:`CAHUTE_LOCAL`'s meaning,
    by making the function inlinable if the compiler is so inclined.

    For example::

        CAHUTE_INLINE(int) my_tiny_utility(int arg1, char const *arg2);

    This can then use compiler-specific functions, such as GCC's
    ``always_inline`` attribute; see `GCC function attributes`_ for
    more information.

.. c:macro:: CAHUTE_LOCAL_DATA(TYPE)

    Macro to use in local data in Cahute source files, surrounding the
    variable type, for example::

        CAHUTE_LOCAL_DATA(char const *) my_string = "hello, world";

    For now, this only produces the following output::

        static char const * my_string = "hello, world";

.. c:macro:: CAHUTE_SSIZE_MAX

    Maximum value for :c:type:`cahute_ssize`, i.e. portable version of
    ``SSIZE_MAX`` for platforms that do not explicitely define an ``ssize_t``
    type.

.. c:macro:: CAHUTE_UINTPTR_MAX

    Maximum value for :c:type:`cahute_uintptr`, i.e. portable version of
    ``UINTPTR_MAX`` for platforms that do not explicitely define an
    ``uintptr_t`` type.

.. c:macro:: CAHUTE_PRIu16

    printf specifier for displaying :c:type:`cahute_u16` in decimal form,
    e.g. ``hu``.

.. c:macro:: CAHUTE_PRIu32

    printf specifier for displaying :c:type:`cahute_u32` in decimal form,
    e.g. ``u``.

.. c:macro:: CAHUTE_PRIuSIZE

    printf specifier for displaying ``size_t`` in decimal form, e.g. ``zu``.

.. c:macro:: CAHUTE_PRIx16

    printf specifier for displaying :c:type:`cahute_u16` in lowercase
    hexadecimal form, e.g. ``hx``.

.. c:macro:: CAHUTE_PRIx32

    printf specifier for displaying :c:type:`cahute_u32` in lowercase
    hexadecimal form, e.g. ``x``.

.. c:macro:: CAHUTE_PRIxSIZE

    printf specifier for displaying ``size_t`` in lowercase hexadecimal form,
    e.g. ``zx``.

.. c:macro:: CAHUTE_PRIX16

    printf specifier for displaying :c:type:`cahute_u16` in uppercase
    hexadecimal form, e.g. ``hX``.

.. c:macro:: CAHUTE_PRIX32

    printf specifier for displaying :c:type:`cahute_u32` in uppercase
    hexadecimal form, e.g. ``X``.

.. c:macro:: CAHUTE_PRIXSIZE

    printf specifier for displaying ``size_t`` in uppercase hexadecimal form,
    e.g. ``zX``.

Type definitions
----------------

.. c:type:: cahute_uintptr

    Portable definition of ``uintptr_t``.

    This type is required since ``uintptr_t`` may not be defined on all
    platforms, e.g. on Windows where ``UINT_PTR`` is defined in a specific
    header.

    Due to namespace constraints, the name of this type cannot include a ``_t``
    suffix; see :ref:`coding-style-namespace` for more information.

.. c:type:: cahute_ssize

    Portable definition of ``ssize_t``.

    This type is required since ``ssize_t`` may not be defined on all
    platforms, e.g. on Windows where ``SSIZE_T`` is defined in a specific
    header.

    Due to namespace constraints, the name of this type cannot include a ``_t``
    suffix; see :ref:`coding-style-namespace` for more information.

.. c:type:: cahute_i8

    Signed 8-bit integer type.

.. c:type:: cahute_u16

    Unsigned 16-bit integer type.

    Available printf specifiers for this type are :c:macro:`CAHUTE_PRIu16`,
    :c:macro:`CAHUTE_PRIx16` and :c:macro:`CAHUTE_PRIX16`.

.. c:type:: cahute_i16

    Signed 16-bit integer type.

.. c:type:: cahute_u32

    Unsigned 32-bit integer type.

    Available printf specifiers for this type are :c:macro:`CAHUTE_PRIu32`,
    :c:macro:`CAHUTE_PRIx32` and :c:macro:`CAHUTE_PRIX32`.

.. c:type:: cahute_i32

    Signed 32-bit integer type.

Function declarations
---------------------

.. warning::

    These utilities are **only available to the library**, and not to the
    command-line utilities.

.. c:function:: cahute_u16 cahute_be16toh(cahute_u16 value)

    Convert a 16-bit unsigned integer from big endian to host endianness.

    :param value: 16-bit unsigned integer in big endian.
    :return: 16-bit unsigned integer in host endianness.

.. c:function:: cahute_u16 cahute_le16toh(cahute_u16 value)

    Convert a 16-bit unsigned integer from little endian to host endianness.

    :param value: 16-bit unsigned integer in little endian.
    :return: 16-bit unsigned integer in host endianness.

.. c:function:: cahute_u32 cahute_be32toh(cahute_u32 value)

    Convert a 32-bit unsigned integer from big endian to host endianness.

    :param value: 32-bit unsigned integer in big endian.
    :return: 32-bit unsigned integer in host endianness.

.. c:function:: cahute_u32 cahute_le32toh(cahute_u32 value)

    Convert a 32-bit unsigned integer from little endian to host endianness.

    :param value: 32-bit unsigned integer in little endian.
    :return: 32-bit unsigned integer in host endianness.

.. c:function:: cahute_u16 cahute_htobe16(cahute_u16 value)

    Convert a 16-bit unsigned integer from host endianness to big endian.

    :param value: 16-bit unsigned integer in host endianness.
    :return: 16-bit unsigned integer in big endian.

.. c:function:: cahute_u16 cahute_htole16(cahute_u16 value)

    Convert a 16-bit unsigned integer from host endianness to little endian.

    :param value: 16-bit unsigned integer in host endianness.
    :return: 16-bit unsigned integer in little endian.

.. c:function:: cahute_u32 cahute_htobe32(cahute_u32 value)

    Convert a 32-bit unsigned integer from host endianness to big endian.

    :param value: 32-bit unsigned integer in host endianness.
    :return: 32-bit unsigned integer in big endian.

.. c:function:: cahute_u32 cahute_htole32(cahute_u32 value)

    Convert a 32-bit unsigned integer from host endianness to little endian.

    :param value: 32-bit unsigned integer in host endianness.
    :return: 32-bit unsigned integer in little endian.

.. _GCC function attributes:
    https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
