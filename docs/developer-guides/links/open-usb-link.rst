Opening a link to a calculator connected by USB
===============================================

In order to open the link to the calculator, the steps are the following:

1. Call :c:func:`cahute_open_simple_usb_link` with the device address.
2. *Profit!*
3. Call :c:func:`cahute_close_link` to close the link.

.. note::

    If there are multiple calculators connected by USB to your system,
    you can manage multiple or a specific subset of them by:

    * Detecting available USB devices using :c:func:`cahute_detect_usb`.
      See :ref:`guide-developer-detect-usb` for steps to do so;
    * Opening a link to a specific USB device using
      :c:func:`cahute_open_usb_link`.

An example program to do this is the following:

.. literalinclude:: open-usb-link.c
    :language: c
