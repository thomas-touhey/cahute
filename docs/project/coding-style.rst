.. _coding-style:

Coding style
============

Cahute is made to be compatible with as much platforms as possible, adopting
the lowest C standard (C89/C90) and protecting platform-specific usage
behind macros as much as possible.

Formatting
----------

In order to focus as much as possible on the work that matters, Cahute relies
heavily on autoformatting and uncompromising sets of rules enforceable through
tooling. This includes:

* For C code, the use the `clang-format`_ with a set of rules described in
  ``.clang-format``, installed as a `pre-commit`_ hook.
* For Python code, the use of Black_.

Otherwise, the rules are not yet clearly defined, and may be as a few
contributions have gone by, and the standard for Cahute shapes itself a
bit more clearly.

Namespace
---------

Cahute reserves both the ``cahute_`` and ``CAHUTE_`` prefixes for all
symbols and macros, i.e. it is recommended a library user does not use
that prefix in any of their work.

Cahute respects reserved namespaces from C and POSIX; see
`Open Group Base Specifications Issue 7, section 2.2.2 The Name Space`_
for more information.

.. note::

    This notably prevents us from using the ``_t`` suffix, since it
    is reserved by POSIX / Single UNIX Specification.

To avoid overlapping with user macros, **function and macro parameters are also
defined within the Cahute namespaces**, more specifically in the ``cahute__``
and ``CAHUTE__`` (*double underscore*) namespaces.

For example, a preprocessed Cahute macro and function declaration in public
headers are the following::

    #define CAHUTE_MY_MACRO(CAHUTE__ARG1, CAHUTE__ARG2) ...

    int cahute_open_usb_link(
        cahute_link *cahute__linkp,
        unsigned long cahute__flags,
        int cahute__bus,
        int cahute__address
    );

.. note::

    The namespace mainly applies to public headers from Cahute.
    Within internal headers and source files, **only exported symbols
    need to be placed within the reserved namespaces**.

    Exported symbols in the Cahute static library can be found using ``nm``:

    .. code-block:: bash

        nm -CgU libcahute.a

Compatibility
-------------

Cahute is designed to be more compatible than not, but to make use of
compiler and platform specific tools for static analysis if possible.
As such, you must use the relevant compatibility macros for your case:

* Both function declarations and definitions must use the following:

  - Either :c:macro:`CAHUTE_EXTERN`, or the function is only to be used by
    other code **in the same file**, :c:macro:`CAHUTE_LOCAL` or
    :c:macro:`CAHUTE_INLINE`.

    Functions internal to Cahute but used cross-file must make use of
    :c:macro:`CAHUTE_EXTERN`, but be declared in ``lib/internals.h`` instead
    of the public headers.
  - :c:macro:`CAHUTE_NNPTR` and :c:macro:`CAHUTE_NONNULL` together, where
    relevant.
* Function declarations only must use the following:

  - :c:macro:`OF`\ ``((int arg1, char const *arg2, ...))`` for parameters.
  - :c:macro:`CAHUTE_DEPRECATED` and :c:macro:`CAHUTE_WUR` where relevant.

**Only utilities available in C89 / C90 must be used.**

For easier implementation, the following out-of-standard general utilities
are available:

* Portable fixed-width integer types, in the form of :c:type:`cahute_u8`,
  :c:type:`cahute_u16` and :c:type:`cahute_u32`.
* Portable printf specifiers for :c:type:`cahute_u8`, :c:type:`cahute_u16`,
  :c:type:`cahute_u32` and ``size_t``.
* Endianness conversion utilities for both 16-bit and 32-bit integers,
  as :c:func:`cahute_be16toh`, :c:func:`cahute_le16toh`,
  :c:func:`cahute_be32toh`, :c:func:`cahute_le32toh`, :c:func:`cahute_htobe16`,
  :c:func:`cahute_htole16`, :c:func:`cahute_htobe32`, :c:func:`cahute_htole32`.

.. _pre-commit: https://pre-commit.com/
.. _clang-format: https://clang.llvm.org/docs/ClangFormat.html
.. _Black: https://github.com/psf/black
.. _`Open Group Base Specifications Issue 7, section 2.2.2 The Name Space`:
    https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html
    #tag_15_02_02
