Opening a generic link to a calculator connected by serial
==========================================================

Opening a generic link can be useful if you want to implement your own
custom protocol for your CASIO calculator, and do not want to deal with
system-specific complexities Cahute has already implemented.

In order to open a link to a calculator plugged in over serial in order to
use the link medium access functions, the steps are the following:

1. Call :c:func:`cahute_open_serial_link` with the
   :c:macro:`CAHUTE_SERIAL_PROTOCOL_NONE` flag.
2. *Profit!*
3. Call :c:func:`cahute_close_link` to close the link.

The functions you can use with generic links are described
in :ref:`header-cahute-link-medium`.

An example program that uses generic links to read two characters, then write
two characters, is the following:

.. literalinclude:: use-generic-serial-link.c
    :language: c

In order to test this, you will need a USB/serial cable and an add-in such
as `Serial monitor`_. Once the cable plugged into both the computer and
the calculator\ [#device_path]_, if you run the program:

1. You will see the two characters ``AB`` appear on your calculator.
2. You can type in two other characters, e.g. ``CD``, on your calculator.
3. The two characters you typed on the calculator will appear on the host.

An example output of the program is the following:

.. code-block:: text

    [2024-09-01 01:42:12    cahute info] set_serial_params_to_link_medium: Setting serial parameters to 9600N1.
    [2024-09-01 01:42:12    cahute info] open_link_from_medium: Using Generic (serial) over Serial (POSIX).
    [2024-09-01 01:42:12    cahute info] open_link_from_medium: Playing the role of sender / active side.
    [2024-09-01 01:42:16    cahute info] receive_on_link_medium: Read 2 bytes in 383ms (after waiting 2746ms).
    Received characters are the following: CD
    [2024-09-01 01:42:16    cahute info] close_link: Closing the link.

.. [#device_path] You may need to adapt the path from the program to match
   the one present on your host system, such as ``/dev/ttyUSB1`` or
   ``/dev/cu.something``.

.. _Serial monitor:
    https://www.planet-casio.com/Fr/programmes/
    programme2161-5-serial-monitor-ziqumu-utilitaires-add-ins.html
