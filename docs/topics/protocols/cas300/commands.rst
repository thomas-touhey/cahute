.. _cas300-commands:

CAS300 commands
===============

With CAS300, used by Classpad 300 / 330 (+) models, the
:ref:`cas300-packet-01` inner payload (*P* field) is expected to be
at least 4 bytes long, and starts with the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 4 B
      - Command (*CMD*)
      - Identifier of the command to be run.
      - 4-char :ref:`seven-ascii-hex` value.

.. _cas300-command-0000:

``0000`` "Start data transfer"
------------------------------

No payload. Followed by data packets once acknowledged.

.. _cas300-command-0001:

``0001`` "End data transfer"
----------------------------

No payload. Sent after data packets have been sent and acknowledged.

.. _cas300-command-0002:

``0002`` "Send device information"
----------------------------------

Sent by the device when requested using :ref:`cas300-command-0011`.
Payload is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 4 (0x04)
      - 49 B
      - Device information
      -
      - See structure below.

Device information has the following structure:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 0 (0x00)
      - 8 B
      - Model
      -
      - ``0xFF``\ -terminated string, e.g. ``CP430\xFF\xFF\xFF``.
    * - 8 (0x08)
      - 16 B
      - OS version
      -
      - ``0xFF``\ -terminated string, e.g. ``00.00.0(03050000``.
    * - 24 (0x18)
      - 8 B
      - Bootcode version (?)
      -
      - ``0xFF``\ -terminated string, e.g. ``01.01.00``.
    * - 32 (0x20)
      - 8 B
      - Flash ROM size
      -
      - ``0xFF``\ -terminated string, e.g. ``16M\xFF\xFF\xFF\xFF\xFF``.
    * - 40 (0x28)
      - 8 B
      - Size (?)
      -
      - ``0xFF``\ -terminated string, e.g. ``8M\xFF\xFF\xFF\xFF\xFF\xFF``.
    * - 48 (0x30)
      - 1 B
      - ?
      -
      - ``0x83``

.. _cas300-command-0003:

``0003`` "Update link settings"
-------------------------------

.. todo::

    This command has been found to sometimes bear a 2-char
    :ref:`seven-ascii-hex` value, and sometimes a 1-byte value directly.
    On serial links, it seems to set up some kind of link update, that either
    sets up obfuscation, or updates link settings, or both.

    It has not yet been found what the role of this command is exactly.

.. _cas300-command-000C:

``000C`` "Send file"
--------------------

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 4 (0x04)
      - 4 B
      - File size
      -
      - Big endian 32-bit integer.
    * - 8 (0x08)
      - 4 B
      - ?
      -
      - ?, e.g. 0x624e461b if it is a 32-bit big endian value
    * - 12 (0x0C)
      - Variable
      - Path to the file
      -
      - Non-terminated string, e.g. ``main.ACT\eActivity Save.EAC``.

.. _cas300-command-000D:

``000D`` "Request file"
-----------------------

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 4 (0x04)
      - 1 B
      - Type
      -
      - Known values:

        * ``0x0B``: file.
    * - 5 (0x05)
      - Variable
      - Path
      -
      - File to request.

.. _cas300-command-000E:

``000E`` "Request file list"
----------------------------

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 4 (0x04)
      - 1 B
      - Type
      -
      - Known values:

        * ``0x00``: Root.
        * ``0x08``: E-Act or file.
    * - 5 (0x05)
      - Variable
      - Path
      -
      - File path (may be empty)

.. _cas300-command-000F:

``000F`` "Transfer file list"
-----------------------------

Followed by :ref:`cas300-command-0000`, then data packets containing the
listing.

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 4 (0x04)
      - 4 B
      - Listing size.
      -
      - Big endian 32-bit integer, e.g. ``0x000000EB``.

.. _cas300-command-0010:

``0010`` ?
----------

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Field name
      - Description
      - Values
    * - 4 (0x04)
      - 2 B
      - ?
      -
      - ``\0\0`` or ``\1\0``

.. _cas300-command-0011:

``0011`` "Request device information"
-------------------------------------

No payload. Answered by a :ref:`cas300-command-0002`.
