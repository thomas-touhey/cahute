.. _guide-developer-detect-usb:

As a developer, I want to list calculators connected by USB
===========================================================

In order to list calculators connected by USB, you must use
:c:func:`cahute_detect_usb`, while providing it with a callback to either
add the USB device to your list or display it.

An example program using this function is the following:

.. literalinclude:: detect-usb.c
    :language: c

An example output for this program is the following:

.. code-block:: text

    New entry data:
    - Address: 003:042
    - Type: fx-9860G or compatible
    New entry data:
    - Address: 003:043
    - Type: fx-CG or compatible

.. note::

    On Linux, you can compare this output to the output of ``lsusb``, for
    example in the case above:

    .. code-block:: text

        $ lsusb
        ...
        Bus 003 Device 042: ID 07cf:6101 Casio Computer Co., Ltd fx-9750gII
        Bus 003 Device 043: ID 07cf:6102 Casio Computer Co., Ltd fx-CP400
        ...
