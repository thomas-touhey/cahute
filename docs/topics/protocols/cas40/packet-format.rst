.. _cas40-packet-format:

CAS40 packet format
===================

All packets in the CAS40 protocol are introduced by a single byte, which
defines the basic purpose of the packet, and defines the kind of payload that
follows it.

See the following sections for more information.

.. _cas40-packet-06:

``0x06`` -- Acknowledge packet
------------------------------

This packet is sent by the receiver to the sender to acknowledge reception
of a header or data packet.
It is a single-byte packet, i.e. it has no payload.

See :ref:`cas40-send` for more information.

.. _cas40-packet-13:

``0x13`` -- Established packet
------------------------------

This packet is sent by the receiver to the sender on initiation, in response
to a :ref:`cas40-packet-16`.
It is a single-byte packet, i.e. it has no payload.

See :ref:`cas40-init` for more information.

.. _cas40-packet-16:

``0x16`` -- Start packet
------------------------

This packet is sent by the sender to the receiver on initiation.
It is a single-byte packet, i.e. it has no payload.

See :ref:`cas40-init` for more information.

.. _cas40-packet-3A:

``0x3A`` -- Header and data packet
----------------------------------

Whether this is a header or data packet depends on the context in the
:ref:`cas40-send` flow.

See :ref:`cas40-data-types` for more information.
