.. _protocol-casiolink:

CASIOLINK protocols -- Serial protocols used by pre fx-9860G calculators
========================================================================

.. warning::

    This page is mainly present for compatibility.
    If you are linking to this page, please update the link to one of those
    below.

All protocols introduced before Protocol 7.00 are considered variants, or
versions, of a single protocol also known as CASIOLINK. They include the
following:

* :ref:`protocol-cas40`;
* :ref:`protocol-cas50`;
* :ref:`protocol-cas100`;
* :ref:`protocol-cas300`.

The most common factor between all variants is the initial handshake,
which consists of the sender sending an ``0x16`` byte / packet, and
the receiver sending a ``0x13`` back to establish the link.

See the protocol descriptions for more information.
