.. _header-cdefs:

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

.. c:macro:: CAHUTE_PRIx8

    printf specifier for displaying :c:type:`cahute_u8` in lowercase
    hexadecimal form, e.g. ``hhx``.

.. c:macro:: CAHUTE_PRIX8

    printf specifier for displaying :c:type:`cahute_u8` in uppercase
    hexadecimal form, e.g. ``hhX``.

Type definitions
----------------

.. c:type:: cahute_u8

    Unsigned 8-bit integer type.

    Available printf specifiers for this type are :c:macro:`CAHUTE_PRIu8`,
    :c:macro:`CAHUTE_PRIx8` and :c:macro:`CAHUTE_PRIX8`.
