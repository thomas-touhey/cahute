.. _topic-links:

Links and mediums
=================

All communication implementations are centered around resources called links.
Internally, links are mostly constituted of:

* A medium, which is an interface with a set of resources to communicate with
  the underlying medium opened with the system or hardware.
* A protocol, which is a set of resources and functions that do not constitute
  an interface, since protocols may obey different logics.

Links are opened using one of the following functions:

* :c:func:`cahute_open_simple_usb_link`;
* :c:func:`cahute_open_usb_link`;
* :c:func:`cahute_open_serial_link`.

.. note::

    The internal behaviour of these methods is documented in
    :ref:`internals-link-open`.

.. _topic-links-generic:

Generic links
-------------

While Cahute aims at implementing official and widespread community protocols,
the portability of its link mediums can be appealing to developers implementing
a custom, possibly program-specific protocol between a host and a CASIO
calculator, notably for the host part.

Since Cahute does not expose link mediums publicly, it allows selecting a
special kind of links called "generic links", which allows the following
functions to be used on such links:

* :c:func:`cahute_receive_on_link`;
* :c:func:`cahute_send_on_link`;
* :c:func:`cahute_set_serial_params_to_link`.

The following methods can be used to open a generic link with Cahute:

* Call :c:func:`cahute_open_serial_link` with the
  :c:macro:`CAHUTE_SERIAL_PROTOCOL_NONE` protocol, for opening a generic
  link over a serial medium;
* Call :c:func:`cahute_open_simple_usb_link` or :c:func:`cahute_open_usb_link`
  with the :c:macro:`CAHUTE_USB_NOPROTO` flag, for opening a generic link
  to a USB device.

The following developer guides demonstrate how to open and use generic links
using the aforementioned methods:

* :ref:`guide-developer-use-generic-serial-link`.
