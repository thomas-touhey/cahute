.. _cas100-packet-format:

CAS100 packet format
====================

All packets in the CAS100 protocol are introduced by a single byte, which
defines the basic purpose of the packet, and defines the kind of payload that
follows it.

See the following sections for more information.

.. _cas100-packet-06:

``0x06`` -- Acknowledge packet
------------------------------

This packet is sent by the receiver to the sender to acknowledge reception
of a header or data packet.
It is a single-byte packet, i.e. it has no payload.

See :ref:`cas100-send` for more information.

.. _cas100-packet-13:

``0x13`` -- Established packet
------------------------------

This packet is sent by the receiver to the sender on initiation, in response
to a :ref:`cas100-packet-16`.
It is a single-byte packet, i.e. it has no payload.

See :ref:`cas100-init` for more information.

.. _cas100-packet-16:

``0x16`` -- Start packet
------------------------

This packet is sent by the sender to the receiver on initiation.
It is a single-byte packet, i.e. it has no payload.

See :ref:`cas100-init` for more information.

.. _cas100-packet-3A:

``0x3A`` -- Header and data packet
----------------------------------

This header format covers headers used with the AlgebraFX / Graph 100.
Such headers are 39-bytes long (excluding the basic purpose byte), but
transferred at higher speeds.

See :ref:`cas100-data-types` for more information.

``0x3E`` -- Segment packet
--------------------------

.. todo::

    This is used when requesting a 1024 byte sector, i.e. ``FRxx`` data type.
    Document this when encountered.
