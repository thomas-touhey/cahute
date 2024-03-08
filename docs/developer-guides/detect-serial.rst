.. _guide-developer-detect-serial:

Listing available serial ports
==============================

In order to list calculators connected by USB, you must use
:c:func:`cahute_detect_serial`, while providing it with a callback to either
add the USB device to your list or display it.

An example program using this function is the following:

.. literalinclude:: detect-serial.c
    :language: c

An example output for this program is the following:

.. code-block:: text

    New entry data:
    - Path: /dev/ttyUSB0
