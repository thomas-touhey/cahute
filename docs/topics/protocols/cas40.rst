.. _protocol-cas40:

CAS40 protocol -- Serial protocol used by pre-1996 calculators
==============================================================

The CAS40 variant of the CASIOLINK protocol is the earliest protocol used
by CASIO calculators over serial links.

.. note::

    The term was coined by Thomas Touhey, combining "CAS", the
    start of "CASIOLINK" and also the name of the community tool
    from that time (see :ref:`cas`), and 40, the size in bytes of
    the headers used in the protocol.

For this protocol, serial settings are selected manually on both the sender
and the receiver, and stay the same until the communication ends.
Available serial settings on the reference implementations of this protocol
are the following:

* Speed: **1200**, **2400**, **4800** or **9600** bauds.
* Parity: **even**, **odd** or **none**.
* Stop bits: **1**.

See the following sections for more details regarding the protocol.

.. toctree::
    :maxdepth: 2

    cas40/packet-format
    cas40/data-types
    cas40/flows
