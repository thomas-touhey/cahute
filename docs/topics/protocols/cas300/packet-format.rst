.. _cas300-packet-format:

CAS300 packet format
====================

All packets in the CAS100 protocol are introduced by a single byte, which
defines the basic purpose of the packet, and defines the kind of payload that
follows it.

See the following sections for more information.

.. _cas300-packet-01:

``0x01`` -- Command packet
--------------------------

This packet has the following payload:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Packet identifier (*ID*)
      - Identifier of the packet the other party acknowledges.
      - 2-char :ref:`seven-ascii-hex` value.
    * - 2 (0x02)
      - 4 B
      - Payload size (*PZ*)
      - Size of the payload.
      - 4-char :ref:`seven-ascii-hex` value.
    * - 6 (0x06)
      - *PZ* B
      - Payload (*P*)
      - Payload of the command.
      - :ref:`0x5C padded <seven-5c-padding>` content.
    * - 6 + *PZ*
      - 2 B
      - Checksum (*CS*)
      -
      - 2-char :ref:`seven-ascii-hex` value.

The checksum can be obtained or verified by summing all bytes going from
*PZ* to *P*, and adding 1 to its bitwise complement.

See :ref:`cas300-commands` for more information about commands.

.. _cas300-packet-02:

``0x02`` -- Data packet
-----------------------

This packet has the following payload:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Packet identifier (*ID*)
      - Identifier of the packet the other party acknowledges.
      - 2-char :ref:`seven-ascii-hex` value.
    * - 2 (0x02)
      - 4 B
      - Payload size (*PZ*)
      - Size of the payload.
      - 4-char :ref:`seven-ascii-hex` value.
    * - 6 (0x06)
      - *PZ* B
      - Payload (*P*)
      - Payload of the command.
      - :ref:`0x5C padded <seven-5c-padding>` content.
    * - 6 + *PZ*
      - 2 B
      - Checksum (*CS*)
      -
      - 2-char :ref:`seven-ascii-hex` value.

The checksum can be obtained or verified by summing all bytes going from
*PZ* to *P*, and adding 1 to its bitwise complement.

.. _cas300-packet-06:

``0x06`` -- Acknowledge packet
------------------------------

This packet has the following payload:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Packet identifier (*ID*)
      - Identifier of the packet the other party acknowledges.
      - 2-char :ref:`seven-ascii-hex` value.

.. _cas300-packet-18:

``0x18`` -- Terminate packet
----------------------------

This packet has the following payload:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Packet identifier (*ID*)
      -
      - 2-char :ref:`seven-ascii-hex` value, set to ``0x11``.
    * - 2 (0x02)
      - 4 B
      -
      -
      - 4-char :ref:`seven-ascii-hex` value, among the following:

        * ``0x0000``: terminated from the calculator.
        * ``0x0004``: terminated from the host.
