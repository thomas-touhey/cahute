``<cahute/error.h>`` -- Error definitions for Cahute
====================================================

This header declares error values and utilities for Cahute.

Macro definitions
-----------------

.. c:macro:: CAHUTE_OK

    Error returned in case of success.
    This is set to ``0``, so that in order to check if a Cahute function has
    yielded an error, you can do the following:

    .. code-block:: c

        if (cahute_do_thing(1, 2)) {
            fprintf(stderr, "An error has occurred.\n");
            return EXIT_FAILURE;
        }

.. c:macro:: CAHUTE_ERROR_UNKNOWN

    Error raised if the cause of the error is unknown.

    The logs can be investigated for more information.

.. c:macro:: CAHUTE_ERROR_IMPL

    Error raised if a required feature or code path was unimplemented.

.. c:macro:: CAHUTE_ERROR_ALLOC

    Error raised if a memory allocation has failed.

.. c:macro:: CAHUTE_ERROR_PRIV

    Error raised if a system privilege error has been encountered.

.. c:macro:: CAHUTE_ERROR_INT

    Error raised if, for a function taking a callback and calling it
    with every iteration, said callback has returned ``1`` on a given
    iteration, meaning the iteration was *INTerrupted*.

.. c:macro:: CAHUTE_ERROR_SIZE

    Error raised if an incoming message was too big for the corresponding
    internal buffers.

.. c:macro:: CAHUTE_ERROR_NOT_FOUND

    Error code raised if a device could not be found using the provided
    identification (name, path, or bus identification).

.. c:macro:: CAHUTE_ERROR_TOO_MANY

    Error raised if only a single device was expected, but multiple were
    found.

.. c:macro:: CAHUTE_ERROR_INCOMPAT

    Error raised if a device was not suitable to be opened to be used by
    a link.

.. c:macro:: CAHUTE_ERROR_TERMINATED

    Error raised if a device is still present, but has terminated the
    communication.

.. c:macro:: CAHUTE_ERROR_GONE

    Error raised if a device with which communication was previously
    established is no longer accessible.

.. c:macro:: CAHUTE_ERROR_TIMEOUT_START

    Error raised if a read timeout has been encountered on the start of a
    block, e.g. at the start of a packet.

.. c:macro:: CAHUTE_ERROR_TIMEOUT

    Error raised if a read timeout has been encountered within or at the
    end of a block, e.g. within or at the end of a packet.

.. c:macro:: CAHUTE_ERROR_CORRUPT

    Error raised if an incoming packet had invalid format, or an invalid
    checksum.

.. c:macro:: CAHUTE_ERROR_IRRECOV

    Error raised if the link was previously deemed irrecoverable, and as such,
    the current operation could not be executed.

.. c:macro:: CAHUTE_ERROR_NOOW

    Error raised if overwrite was requested and rejected by either us or
    the calculator.

Function declarations
---------------------

.. c:function:: char const *cahute_get_error_name(int code)

    Get the name of the constant corresponding to the given error code,
    in ASCII, using only capital letters, decimal digits and low lines
    (underscores).

    :param code: Code for which to get the error name.
    :return: Name of the error code, in ASCII.
