Opening a link to a calculator connected by USB
===============================================

In order to open the link to the calculator, the steps are the following:

1. Either request USB device address from the user, or use USB device
   detection to find out the first available device; see
   :ref:`guide-developer-detect-usb` for more information.
2. Call :c:func:`cahute_open_usb_link` with the device address.
3. *Profit!*
4. Call :c:func:`cahute_close_link` to close the link.

An example program to do this is the following:

.. literalinclude:: open-usb-link.c
    :language: c
