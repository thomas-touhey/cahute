.. _protocol-seven:

Protocol 7.00 -- Serial and USB protocol used by post fx-9860G calculators
==========================================================================

This protocol is used by calculators starting from the fx-9860G, published
in 2004, up to the current day.

.. note::

    fx-CG calculators and derivatives still use a derivative from Protocol
    7.00, although not directly, but hidden behind proprietary SCSI commands.
    See :ref:`protocol-ums` for more information.

For this protocol, serial settings are always the same at the beginning of
the communication:

* Speed: **9600** bauds;
* Parity: **none**;
* Stop bits: **2**.

The active side can request from the passive side that the speed, parity
and/or stop bits be changed at any point, using command
:ref:`seven-command-02`, as described in :ref:`seven-update-serial-params`.

.. note::

    When setting the calculator as the initial active side using the
    ``TRAN`` (*Transmit*) menu in the ``LINK`` application, it sets the
    serial settings after initialization to the following settings:

    * Speed: **115200** bauds;
    * Parity: **even**;
    * Stop bits: **1**.

See the following sections for more details regarding the protocol.

.. toctree::
    :maxdepth: 1

    seven/specific-encodings
    seven/packet-format
    seven/flows
    seven/casio-commands
    seven/fxremote-commands
    seven/use-cases
    seven/hardware-identifiers
