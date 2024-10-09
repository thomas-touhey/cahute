.. _cas50-packet-format:

CAS50 packet format
===================

All packets in the CAS50 protocol are introduced by a single byte, which
defines the basic purpose of the packet, and defines the kind of payload that
follows it.

See the following sections for more information.

.. _cas50-packet-13:

``0x13`` -- Established packet
------------------------------

This packet is sent by the receiver to the sender on initiation, in response
to a :ref:`cas50-packet-16`.
It is a single-byte packet, i.e. it has no payload.

See :ref:`cas50-init` for more information.

.. _cas50-packet-15:

``0x15`` -- Abort
-----------------

This packet is sent by the sender to the receiver if it wants to abort the
transfer, following an overwrite confirmation request.
It is a single-byte packet, i.e. it has no payload.

.. _cas50-packet-16:

``0x16`` -- Start packet
------------------------

This packet is sent by the sender to the receiver on initiation.
It is a single-byte packet, i.e. it has no payload.

See :ref:`cas50-init` for more information.

.. _cas50-packet-21:

``0x21`` -- Overwrite confirmation request
------------------------------------------

This packet is sent by the receiver to the sender if receiving a header for a
data that is already in memory.
It is a single-byte packet, i.e. it has no payload.

.. _cas50-packet-24:

``0x24`` -- Invalid data type
-----------------------------

This packet is sent by the receiver to the sender if the header of the provided
data references an unknown data type.
It is a single-byte packet, i.e. it has no payload.

.. _cas50-packet-2B:

``0x2B`` -- Corrupted header
----------------------------

The packet is sent by the receiver to the sender if the header of the provided
data has an invalid checksum.
It is a single-byte packet, i.e. it has no payload.

.. _cas50-packet-3A:

``0x3A`` -- Header and data packet
----------------------------------

Whether this is a header or data packet depends on the context in the
:ref:`cas50-send` flow.

See :ref:`cas50-data-types` for more information.

.. _cas50-packet-51:

``0x51`` -- Invalid header
--------------------------

The packet is sent by the receiver to the sender if the header of the provided
data references have errors.
It is a single-byte packet, i.e. it has no payload.
