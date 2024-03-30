``<cahute/cdefs.h>`` -- Basic definitions for Cahute
====================================================

This header declares basic definitions for Cahute.

Macro definitions
-----------------

.. c:macro:: CAHUTE_PREREQ(MAJOR, MINOR)

    Macro that returns whether the current version of Cahute is compatible
    with the provided version of Cahute.

    For example, ``CAHUTE_PREREQ(2, 4)`` checks if the current version of
    Cahute is compatible with version 2.4.

.. c:macro:: CAHUTE_EXTERN(TYPE)

    Macro to use in Cahute function declarations, surrounding the return type,
    for compatibility. For example::

        CAHUTE_EXTERN(int) my_function(int arg1, char const *arg2);

    This is used to add calling conventions or other platform-specific options
    in some cases, such as, on 16-bit x86 where the calling convention was
    best described explicitely.

    Example outputs of the above are the following::

        int my_function(int arg1, char const *arg2); /* Default. */

        _stdcall int my_function(int arg1, char const *arg2); /* WINAPI x86 */

        int __cdecl my_function(int arg1, char const *arg2); /* GCC x86 */

        extern __declspec(dllexport) int my_function(int arg1, char const *arg2); /* Borland C */

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

.. c:macro:: CAHUTE_DEPRECATED

    Function attribute, placed before the return type and
    :c:macro:`CAHUTE_EXTERN`, that indicates that the compiler should
    warn that the function is deprecated when compiling a user program or
    library.

    For example::

        CAHUTE_DEPRECATED int my_deprecated_function(int a, int b);

.. c:macro:: CAHUTE_WUR

    Function attribute, placed before the return type and
    :c:macro:`CAHUTE_EXTERN`, that indicates that the compiler should
    **w**\ arn in case of **u**\ nused **r**\ esult, i.e. if the caller
    does not store nor process the resulting value from the function.

    For example::

        CAHUTE_WUR resource *create_resource(int a, int b);

.. c:macro:: CAHUTE_NNPTR(NAME)

    Function parameter attribute that indicate that the passed value should
    not be ``NULL``. It must be used in both the function declaration
    and definition. For example::

        int my_function(char const CAHUTE_NNPTR(my_string))

    Calling this with an explicitely NULL pointer will raise a compiler
    warning or error.

    This may resolve as::

        int my_function(char const *my_string); /* Default. */
        int my_function(char const my_string[static 1]); /* C99. */

    For maximum compatibility, this macro must be used with
    :c:macro:`CAHUTE_NONNULL`.

.. c:macro:: CAHUTE_NONNULL(INDEXES)

    Indicate, as an attribute, that one or more of the function arguments
    should not be passed as NULL. For example::

        int my_function(int *a, int *b, int *c) CAHUTE_NONNULL((1, 3));

    Calling ``my_function`` with a NULL pointer for ``a`` or ``c`` will raise
    a compiler warning or error.

    This may resolve as::

        int my_function(int *a, int *b, int *c); /* Default. */
        int my_function(int *a, int *b, int *c) __attribute__((nonnull (1, 3))); /* Pre-C99 GCC. */

    For maximum compatibility, this macro must be used with
    :c:macro:`CAHUTE_NNPTR`.

.. c:macro:: CAHUTE_PRIu8

    printf specifier for displaying :c:type:`cahute_u8` in decimal form,
    e.g. ``hhu``.

.. c:macro:: CAHUTE_PRIu16

    printf specifier for displaying :c:type:`cahute_u16` in decimal form,
    e.g. ``hu``.

.. c:macro:: CAHUTE_PRIu32

    printf specifier for displaying :c:type:`cahute_u32` in decimal form,
    e.g. ``u``.

.. c:macro:: CAHUTE_PRIuSIZE

    printf specifier for displaying ``size_t`` in decimal form, e.g. ``zu``.

.. c:macro:: CAHUTE_PRIx8

    printf specifier for displaying :c:type:`cahute_u8` in lowercase
    hexadecimal form, e.g. ``hhx``.

.. c:macro:: CAHUTE_PRIx16

    printf specifier for displaying :c:type:`cahute_u16` in lowercase
    hexadecimal form, e.g. ``hx``.

.. c:macro:: CAHUTE_PRIx32

    printf specifier for displaying :c:type:`cahute_u32` in lowercase
    hexadecimal form, e.g. ``x``.

.. c:macro:: CAHUTE_PRIxSIZE

    printf specifier for displaying ``size_t`` in lowercase hexadecimal form,
    e.g. ``zx``.

.. c:macro:: CAHUTE_PRIX8

    printf specifier for displaying :c:type:`cahute_u8` in uppercase
    hexadecimal form, e.g. ``hhX``.

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

.. c:type:: cahute_u8

    Unsigned 8-bit integer type.

    Available printf specifiers for this type are :c:macro:`CAHUTE_PRIu8`,
    :c:macro:`CAHUTE_PRIx8` and :c:macro:`CAHUTE_PRIX8`.

.. c:type:: cahute_u16

    Unsigned 16-bit integer type.

    Available printf specifiers for this type are :c:macro:`CAHUTE_PRIu16`,
    :c:macro:`CAHUTE_PRIx16` and :c:macro:`CAHUTE_PRIX16`.

.. c:type:: cahute_u32

    Unsigned 32-bit integer type.

    Available printf specifiers for this type are :c:macro:`CAHUTE_PRIu32`,
    :c:macro:`CAHUTE_PRIx32` and :c:macro:`CAHUTE_PRIX32`.

Function declarations
---------------------

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
