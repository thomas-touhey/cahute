Protocol 7.00 packet format
===========================

All packets with Protocol 7.00 have the following header:

.. list-table::
    :header-rows: 1

    * - Size
      - Field name
      - Description
      - Values
    * - 1 B
      - Type (*T*)
      - Basic purpose of the packet
      - 0x00 to 0x1F
    * - 2 B
      - Subtype (*ST*)
      - Specific function of the packet
      - 2-chars :ref:`seven-ascii-hex` value, ``00`` to ``FF``
    * - 1 B
      - Extended (*EX*)
      - Whether data is attached to the packet
      - 1-char :ref:`seven-ascii-hex` value, ``0`` or ``1``

If the packet is extended, i.e. if *EX* is set to ``1``, the following
extension is appended to the header:

.. list-table::
    :header-rows: 1

    * - Size
      - Field name
      - Description
      - Values
    * - 4 B
      - Data size (*DS*)
      - Size of the *D* field
      - 4-char :ref:`seven-ascii-hex` value, ``0000`` to ``FFFF``
    * - *DS* B
      - Data (*D*)
      - Additional data
      - :ref:`seven-5c-padding` encoded data

All packets have the following footer:

.. list-table::
    :header-rows: 1

    * - Size
      - Field name
      - Description
      - Values
    * - 2 B
      - Checksum (*CS*)
      - Checksum for integrity check
      - 2-char :ref:`seven-ascii-hex` value, ``00`` to ``FF``

.. _seven-checksum:

The checksum can be obtained or verified by summing all bytes going from
*ST* to *D* if present, or *EX* if not, and adding 1 to its bitwise
complement.

The interpretation of the packet depends on the value of *T*.
See the following sections for more information.

.. _seven-command-packet:

``0x01`` -- Command packet
--------------------------

The command packet reflects an order given by the active side to the
passive side. The packet subtype (*ST*) contains the command code.

If the packet is extended, the packet data (*D*) is **usually** formatted
the following way:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 2 B
      - Overwrite (*OW*)
      - Mode of operation if a file exists.
      - 2-char :ref:`seven-ascii-hex` value:

        - ``00``: Request user confirmation.
        - ``01``: Terminate if exists.
        - ``02``: Force overwrite.
    * - 2 (0x02)
      - 2 B
      - Data type (*DT*)
      - Data type.
      - 2-char :ref:`seven-ascii-hex` value, ``00``.
    * - 4 (0x04)
      - 8 B
      - File size (*FS*)
      - Size of the file being transferred.
      - 8-char :ref:`seven-ascii-hex` value.
    * - 12 (0x0C)
      - 2 B
      - Data size 1 (*SD1*)
      - Size of *D1*
      - 2-char :ref:`seven-ascii-hex` value.
    * - 14 (0x0E)
      - 2 B
      - Data size 2 (*SD2*)
      - Size of *D2*
      - 2-char :ref:`seven-ascii-hex` value.
    * - 16 (0x10)
      - 2 B
      - Data size 3 (*SD3*)
      - Size of *D3*
      - 2-char :ref:`seven-ascii-hex` value.
    * - 18 (0x12)
      - 2 B
      - Data size 4 (*SD4*)
      - Size of *D4*
      - 2-char :ref:`seven-ascii-hex` value.
    * - 20 (0x14)
      - 2 B
      - Data size 5 (*SD5*)
      - Size of *D5*
      - 2-char :ref:`seven-ascii-hex` value.
    * - 22 (0x16)
      - 2 B
      - Data size 6 (*SD6*)
      - Size of *D6*
      - 2-char :ref:`seven-ascii-hex` value.
    * - 24 (0x16)
      - *SD1* B
      - Data 1 (*D1*)
      - First argument
      -
    * - 24 (0x16) + *SD1*
      - *SD2* B
      - Data 2 (*D2*)
      - Second argument
      -
    * - 24 (0x16) + *SD1* + *SD2*
      - *SD3* B
      - Data 3 (*D3*)
      - Third argument
      -
    * - 24 (0x16) + *SD1* + *SD2* + *SD3*
      - *SD4* B
      - Data 4 (*D4*)
      - Fourth argument
      -
    * - 24 (0x16) + *SD1* + *SD2* + *SD3* + *SD4*
      - *SD5* B
      - Data 5 (*D5*)
      - Fifth argument
      -
    * - 24 (0x16) + *SD1* + *SD2* + *SD3* + *SD4* + *SD5*
      - *SD6* B
      - Data 6 (*D6*)
      - Sixth argument
      -

.. warning::

    There is an exception, i.e. a command that uses a different payload format:
    command :ref:`seven-command-56`.

.. _seven-data-packet:

``0x02`` -- Data packet
-----------------------

The data packet is used to transfer "raw" data, as described in
:ref:`seven-transmit-data`.

The packet subtype (*ST*) must be the same as the subtype (*ST*) of the
command that has initiated the data transmission; e.g. if the data flow has
been initiated following command :ref:`seven-command-25`, all data packets
in the corresponding flow must bear the *ST* ``25``.

All data packets are expected to be extended (i.e. *EX* is set to ``1``).
The layout of *D* is the following:

.. list-table::
    :header-rows: 1

    * - Size
      - Field name
      - Description
      - Values
    * - 4 B
      - Total number (*TN*)
      - Total number of data packets in current transmission
      - 4-char :ref:`seven-ascii-hex` value, ``0001`` to ``FFFF``.
    * - 4 B
      - Current number (*CN*)
      - Current data packet number in current transmission
      - 4-char :ref:`seven-ascii-hex` value, ``0001`` to ``FFFF``.
    * - 0-512 B
      - Contents (*DD*)
      - Contents of data packet
      -

Note that since 512 bytes is the maximum transmittable amount, and that
:ref:`seven-5c-padding` can only double the data size, the maximum size
of transmitted data within one data packet is 256 bytes.

``0x03`` -- Roleswap packet
---------------------------

The roleswap packet indicates a roleswap, and is used for transfer requests;
see :ref:`seven-request-transfer` for more information.

Such packets will never be extended, and will never have another subtype
than ``00``.

.. _seven-check-packet:

``0x05`` -- Check packet
------------------------

This covers two different packet types, depending on the subtype (*ST*):

* If *ST* is ``00``, this is an initial check packet used to establish
  communication with the calculator. See :ref:`seven-init-link` for
  more information.
* If *ST* is ``01``, this is a regular check packet used to check if
  the link is still up with the calculator. See :ref:`seven-check-link`
  for more information.

This packet is not expected to be extended, i.e. *EX* should be ``0``.

.. _seven-ack-packet:

``0x06`` -- Acknowledgement (ACK) packet
----------------------------------------

This covers two different packet types, depending on the subtype (*ST*):

* If *ST* is ``00``, this is a basic acknowledgement whose role can be
  interpreted differently depending on the flow. It is not expected to be
  extended, i.e. *EX* should be ``0``.
* If *ST* is ``01``, this is an overwrite confirmation; see
  :ref:`seven-confirm-overwrite` for more details on the related flow.
* If *ST* is ``02``, this is an extended acknowledgement (EACK), providing
  additional information, i.e. the packet is extended and *EX* should be
  set to ``1``.

  **This is only used in the flow to get device information**; see
  :ref:`seven-get-device-information` and :ref:`seven-command-01`
  for more information.
* If *ST* is ``03``, this means the acknowledge packet also marks the end
  of the communication.

The payload for an extended acknowledgement is the following:

If *ST* is set to ``02``, the packet data (*D*) is expected to be 164 (0xA4)
bytes long, and structured the following way:

If *ST* is set to ``02``, the packet data (*D*) is expected to be one of:

* 164 (0xA4) bytes long for fx-9860G and compatible models.
* 188 (0xBC) bytes long for the fx-CG family of models.

The format of the 164 bytes long payload is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 8 B
      - Hardware Identifier (*HWID*)
      - Hardware identifier for the calculator.
      - See :ref:`seven-hardware-identifiers` for known hardware identifiers.
    * - 8 (0x08)
      - 16 B
      - Processor Identifier (*CPUID*)
      - String describing the processor used by the calculator.
        Is known to be incorrect, it is recommended not to use this field.
      - ``RENESAS SH735501``
    * - 24 (0x18)
      - 8 B
      - Preprogrammed ROM capacity
      - Capacity of the preprogrammed ROM, in KB
      - 8-char **ASCII-DEC** value, e.g. ``00000000``.
    * - 32 (0x20)
      - 8 B
      - Flash ROM capacity
      - Capacity of the flash ROM, in KB
      - 8-char **ASCII-DEC** value, e.g. ``00004096``.
    * - 40 (0x28)
      - 8 B
      - RAM capacity
      - Capacity of the RAM, in KB
      - 8-char **ASCII-DEC** value, e.g. ``00000512``.
    * - 48 (0x30)
      - 16 B
      - Preprogrammed ROM version
      - Version of the preprogrammed ROM, as an OS version
      - ``xx.xx.xxxx`` + ``\xFF\xFF\xFF\xFF\xFF\xFF``
    * - 64 (0x40)
      - 16 B
      - Boot code version
      - Version of the bootcode, as an OS version
      - ``xx.xx.xxxx`` + ``\xFF\xFF\xFF\xFF\xFF\xFF``
    * - 80 (0x50)
      - 8 B
      - Boot code offset
      - Offset of the boot code on the ROM, as a 32-bit address.
      - 8-char :ref:`seven-ascii-hex` value.
    * - 88 (0x58)
      - 8 B
      - Boot code size
      - Size of the boot code, in KB.
      - 8-char :ref:`seven-ascii-hex` value.
    * - 96 (0x60)
      - 16 B
      - OS code version
      - Version of the OS, as an OS version
      - ``xx.xx.xxxx`` + ``\xFF\xFF\xFF\xFF\xFF\xFF``
    * - 112 (0x70)
      - 8 B
      - OS code offset
      - Offset of the OS on the ROM, as a 32-bit address.
      - 8-char :ref:`seven-ascii-hex` value.
    * - 120 (0x78)
      - 8 B
      - OS code size
      - Size of the OS on the ROM, in KB.
      - 8-char :ref:`seven-ascii-hex` value.
    * - 128 (0x80)
      - 4 B
      - Protocol version
      - Version of the protocol in use.
      - ``7.00``
    * - 132 (0x84)
      - 16 B
      - Product Identifier
      - Identifier of the calculator.
      - 8-char alphabetical (upper and lower case) identifier.
    * - 148 (0x94)
      - 16 B
      - User Name
      - Name set by the user in SYSTEM
      -

The format of the 164 bytes long payload is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 148 B
      - ...
      - Equivalent to the 164 bytes long payload up to the Product Identifier,
        included.
      - See above.
    * - 148 (0x94)
      - 20 B
      - User Name
      - Set by the user in SYSTEM.
      -
    * - 168 (0xA8)
      - 20 B
      - Organisation Name
      - Set by the user in SYSTEM.
      -

.. _seven-nak-packet:

``0x15`` -- Negative acknowledgement (NAK) packet
-------------------------------------------------

This covers multiple packet types, depending on the subtype (*ST*):

* If *ST* is ``00``, a default error is returned.
* If *ST* is ``01``, the previously received packet is invalid and requires
  resending from the other side. See :ref:`seven-report-invalid-checksum`
  for more information on the usage of this error.
* If *ST* is ``02``, the operation requires an overwrite confirmation;
  see :ref:`seven-confirm-overwrite` for more details on the related flow.
* If *ST* is ``03``, the active side is signalling an overwrite rejection from
  the user to the passive side; see :ref:`seven-confirm-overwrite` for more
  details on the related flow.
* If *ST* is ``04``, the error is a generic error that terminates the link.
* If *ST* is ``05``, the memory is full and the link is also terminated.

.. _seven-terminate-packet:

``0x18`` -- Terminate packet
----------------------------

This data packet is used by the active side to signal termination of
the link usage. The subtype (*ST*) represents the reason for termination:

* *ST* being set to ``00`` is the default case.
* If *ST* is set to ``01``, it means that the user has requested
  termination from either the active or passive side.
* If *ST* is set to ``02``, it means that the termination is due to
  timeouts, likely due to checking flows having failed; see
  :ref:`seven-check-link` for more information.
* If *ST* is set to ``03``, it means that the termination is due to
  an overwrite request having been denied by the user on the active side.

All termination packets should not be extended, i.e. *EX* should be ``0``.

See :ref:`seven-terminate-link` for more information on the link
termination flow.
