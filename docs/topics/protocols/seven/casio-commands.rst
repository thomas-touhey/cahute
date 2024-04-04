.. _protocol-seven-casio-commands:

Known Protocol 7.00 commands by CASIO
=====================================

These commands have been defined by CASIO through their Protocol 7.00
implementations. Some may no longer be implemented on newer models.

.. _seven-command-00:

``00`` "Restart"
----------------

.. seven-command::
    :code: 00

    Restart or reset the device.

.. _seven-command-01:

``01`` "Get device information"
-------------------------------

.. seven-command::
    :code: 01

    Request an extended ACK with subtype ``02``, and a payload containing
    device information.

    See :ref:`seven-get-device-information` for the related flow, and
    :ref:`seven-ack-packet` for more information regarding the payload format.

.. _seven-command-02:

``02`` "Set link settings"
--------------------------

.. seven-command::
    :code: 02
    :d1: Baud rate in decimal format.
    :d1-example: 19200
    :d2: Parity, among "ODD", "EVEN" or "NONE".
    :d2-example: EVEN
    :d3: Stop bits, "1" or "2".
    :d3-example: 2

    If connected to the passive device using a serial link, this command
    signifies a request to communicate using different serial parameters.

    .. note::

        The serial speeds are limited to the following speeds (or baud rates):
        300, 600, 1200, 2400, 4800, 9600 (*by default*), 19200, 38400, 57600
        and 115200 bauds.

    See :ref:`seven-update-serial-params` for more information.

.. _seven-command-06:

``06`` "Set link timeout"
-------------------------

.. seven-command::
    :code: 06
    :d1: 4-char :ref:`seven-ascii-hex` number of minutes
    :d1-example: 0001

    Set the timeout for the link, after which the connection is terminated.

    .. todo:: Does the calculator send a termination packet?

.. _seven-command-07:

``07`` Verification 1 (OS Update)
---------------------------------

.. todo:: Describe this command.

.. _seven-command-08:

``08`` Verification 2 (OS Update)
---------------------------------

.. todo:: Describe this command.

.. _seven-command-09:

``09`` Verification 3 (OS Update)
---------------------------------

.. todo::

    Describe this command.

    This command with a Graph 75+E just returns a simple ACK.

.. _seven-command-0A:

``0A`` Verification 4 (OS Update)
---------------------------------

.. todo:: Describe this command.

.. _seven-command-20:

``20`` "Create directory" (main memory)
---------------------------------------

.. seven-command::
    :code: 20
    :d1: Name of the directory to create.
    :d1-example: HELLO

    Create a directory on the main memory root.

.. _seven-command-21:

``21`` "Delete directory" (main memory)
---------------------------------------

.. seven-command::
    :code: 21
    :d1: Name of the directory to delete.
    :d1-example: HELLO

    Delete a directory on the main memory root.

.. _seven-command-22:

``22`` "Rename directory" (main memory)
---------------------------------------

.. seven-command::
    :code: 22
    :d1: Name of the directory to rename.
    :d1-example: HELLO
    :d2: New name for the directory.
    :d2-example: WORLD

    Rename a directory on the main memory root.

.. _seven-command-23:

``23`` "Change working directory" (main memory)
-----------------------------------------------

.. seven-command::
    :code: 23
    :d1: Name of the directory (or "" for root).
    :d1-example: HELLO

    Change the working directory on the main memory.

    .. note::

        When provided and not empty, the directory name is always from the
        root, since the main memory doesn't support more depth.

.. _seven-command-24:

``24`` "Request file transfer" (main memory)
--------------------------------------------

.. seven-command::
    :code: 24
    :dt: Main memory data type.
    :dt-example: 01
    :d1: Directory name.
    :d1-example: system
    :d2: File name.
    :d2-example: PLSOUMOI
    :d3: Group name.

    Request a main memory file to be transferred, using command
    :ref:`seven-command-25`.

.. _seven-command-25:

``25`` "Transfer file" (main memory)
------------------------------------

.. seven-command::
    :code: 25
    :ow: Overwrite mode.
    :dt: Main memory data type.
    :dt-example: 01
    :fs: File size.
    :fs-example: 00012345
    :d1: Directory name.
    :d1-example: system
    :d2: File name.
    :d2-example: PLSOUMOI
    :d3: Group name.

    Transfer a main memory file.

.. _seven-command-26:

``26`` "Delete file" (main memory)
----------------------------------

.. seven-command::
    :code: 26
    :dt: Main memory data type.
    :dt-example: 01
    :fs: File size.
    :fs-example: 00012345
    :d1: Directory name.
    :d1-example: system
    :d2: File name.
    :d2-example: PLSOUMOI
    :d3: Group name.

    Delete a main memory file.

.. _seven-command-27:

``27`` "Rename file" (main memory)
----------------------------------

.. seven-command::
    :code: 27
    :dt: Main memory data type.
    :dt-example: 01
    :d1: Directory name.
    :d1-example: system
    :d2: File name.
    :d2-example: PLSOUMOI
    :d3: New file name.
    :d3-example: PLUSMOIN

    Rename a main memory file.

.. _seven-command-28:

``28`` "Copy file" (main memory)
--------------------------------

.. seven-command::
    :code: 28
    :dt: Main memory data type.
    :dt-example: 01
    :d1: Directory name.
    :d1-example: system
    :d2: File name.
    :d2-example: PLUSMOIN
    :d3: New directory name.
    :d3-example: system
    :d4: New file name.
    :d4-example: PLMNCOPY

    Copy a main memory file into another on the device.

.. _seven-command-29:

``29`` "Request transfer of all files" (main memory)
----------------------------------------------------

.. seven-command::
    :code: 29

    Request all main memory files to be transferred using command
    :ref:`seven-command-25`.

.. _seven-command-2A:

``2A`` "Reset" (main memory)
----------------------------

.. seven-command::
    :code: 2A

    Reset main memory.

.. _seven-command-2B:

``2B`` "Request available capacity" (main memory)
-------------------------------------------------

.. seven-command::
    :code: 2B

    Request the available capacity on the main memory to be transferred
    using command :ref:`seven-command-2C`.

.. _seven-command-2C:

``2C`` "Transfer available capacity" (main memory)
--------------------------------------------------

.. seven-command::
    :code: 2C
    :fs: Available capacity.
    :fs-example: 00123456

    Transfer the available capacity on the main memory.

.. _seven-command-2D:

``2D`` "Request all file information" (main memory)
---------------------------------------------------

.. seven-command::
    :code: 2D

    Transfer information regarding all main memory files, using
    command :ref:`seven-command-2E`.

.. _seven-command-2E:

``2E`` "Transfer file information" (main memory)
------------------------------------------------

.. seven-command::
    :code: 2E
    :dt: Main memory data type.
    :dt-example: 01
    :fs: File size.
    :fs-example: 00012345
    :d1: Directory name.
    :d1-example: system
    :d2: File name.
    :d2-example: PLUSMOIN
    :d3: Group name.

    Transfer information regarding a main memory file.

.. _seven-command-2F:

``2F`` "Request raw main memory"
--------------------------------

.. seven-command::
    :code: 2F

    Request the raw main memory to be sent using command
    :ref:`seven-command-30`.

.. _seven-command-30:

``30`` "Transfer raw main memory"
---------------------------------

.. seven-command::
    :code: 30

    Transfer the raw main memory.

.. _seven-command-31:

``31`` "Request setup entry"
----------------------------

.. seven-command::
    :code: 31
    :d1: Setup entry name.

    Request a setup entry, to be sent using command :ref:`seven-command-32`.

.. _seven-command-32:

``32`` "Transfer setup entry"
-----------------------------

.. seven-command::
    :code: 32
    :d1: Setup entry name.
    :d1-example: Angle
    :d2: Setup entry value.
    :d2-example: 01

    Send a setup entry.

.. _seven-command-33:

``33`` "Request all setup entries"
----------------------------------

.. seven-command::
    :code: 33

    Request all setup entries to be sent using command
    :ref:`seven-command-32`.

.. _seven-command-40:

``40`` "Create directory" (storage)
-----------------------------------

.. seven-command::
    :code: 40
    :d1: Directory name.
    :d1-example: MYFOLDER
    :d5: Device name.
    :d5-example: fls0

    Create a directory on the provided storage device.

.. _seven-command-41:

``41`` "Delete directory" (storage)
-----------------------------------

.. seven-command::
    :code: 41
    :d1: Directory name.
    :d1-example: MYFOLDER
    :d5: Device name.
    :d5-example: fls0

    Delete a directory on the provided storage device.

.. _seven-command-42:

``42`` "Rename directory" (storage)
-----------------------------------

.. seven-command::
    :code: 42
    :d1: Directory name.
    :d1-example: MYFOLDER
    :d2: New directory name.
    :d2-example: PRECIOUS
    :d5: Device name.
    :d5-example: fls0

    Rename a directory on the provided storage device.

.. _seven-command-43:

``43`` "Change working directory" (storage)
-------------------------------------------

.. seven-command::
    :code: 43
    :d1: Directory name.
    :d1-example: MYFOLDER
    :d5: Device name.
    :d5-example: fls0

    Update the working directory on the given device.

.. _seven-command-44:

``44`` "Request file" (storage)
-------------------------------

.. seven-command::
    :code: 44
    :d1: Directory name.
    :d1-example: MYFOLDER
    :d2: File name.
    :d2-example: MYADDIN.G1A
    :d5: Device name.
    :d5-example: fls0

    Request for a file to be sent on the given device.

    This command is used in the following use cases:

    * :ref:`seven-request-file-from-storage`.

.. _seven-command-45:

``45`` "Transfer file" (storage)
--------------------------------

.. seven-command::
    :code: 45
    :ow: Overwrite mode.
    :ow-example: 02
    :fs: File size.
    :fs-example: 00123456
    :d1: Directory name.
    :d2: File name.
    :d2-example: MYADDIN.G1A
    :d5: Device name.
    :d5-example: fls0

    Transfer a file to be sent on the given device.

    This command is used in the following use cases:

    * :ref:`seven-send-file-to-storage`;
    * :ref:`seven-request-file-from-storage`.

.. _seven-command-46:

``46`` "Delete file" (storage)
------------------------------

.. seven-command::
    :code: 46
    :d1: Directory name.
    :d2: File name.
    :d2-example: MYADDIN.G1A
    :d5: Device name.
    :d5-example: fls0

    Delete a file on the given device.

    This command is used in the following use cases:

    * :ref:`seven-delete-file-on-storage`.

.. _seven-command-47:

``47`` "Rename file" (storage)
------------------------------

.. seven-command::
    :code: 47
    :d1: Directory name.
    :d2: File name.
    :d2-example: MYADDIN.G1A
    :d3: New file name.
    :d3-example: GRAVDUCK.G1A
    :d5: Device name.
    :d5-example: fls0

    Rename a file on the given device.

.. _seven-command-48:

``48`` "Copy file" (storage)
----------------------------

.. seven-command::
    :code: 48
    :d1: Directory name.
    :d2: File name.
    :d2-example: ORIGNAME.G1A
    :d3: New directory name.
    :d4: New file name.
    :d4-example: NEWNAME.G1A
    :d5: Device name.
    :d5-example: fls0

    Copy a file on the given device.

    This command is used in the following use cases:

    * :ref:`seven-copy-file-on-storage`.

.. _seven-command-49:

``49`` "Request all files" (storage)
------------------------------------

.. seven-command::
    :code: 49
    :d5: Storage device
    :d5-example: fls0

    Request all files to be transmitted using command
    :ref:`seven-command-45`.

.. _seven-command-4A:

``4A`` "Reset" (storage)
------------------------

.. seven-command::
    :code: 4A
    :d5: Storage device
    :d5-example: fls0

    Reset a storage device.

    This command is used in the following use cases:

    * :ref:`seven-reset-storage`.

.. _seven-command-4B:

``4B`` "Request available capacity" (storage)
---------------------------------------------

.. seven-command::
    :code: 4B
    :d5: Storage device
    :d5-example: fls0

    Request the available capacity for a given storage device.

.. _seven-command-4C:

``4C`` "Transfer available capacity" (storage)
----------------------------------------------

.. seven-command::
    :code: 4C
    :fs: Available capacity
    :d5: Storage device
    :d5-example: fls0

    Provide the available capacity for a given storage device.

.. _seven-command-4D:

``4D`` "Request all file information" (storage)
-----------------------------------------------

.. seven-command::
    :code: 4D
    :d5: Storage device
    :d5-example: fls0

    Request information from all files on the given storage device
    to be transferred.

    This command is used in the following use cases:

    * :ref:`seven-list-files-on-storage`.

.. _seven-command-4E:

``4E`` "Transfer file information" (storage)
--------------------------------------------

.. seven-command::
    :code: 4E
    :fs: File size
    :d1: Directory name
    :d2: File name
    :d2-example: MYADDIN.G1A
    :d5: Storage device
    :d5-example: fls0

    Transfer information regarding a file.

    This command is used in the following use cases:

    * :ref:`seven-list-files-on-storage`.

.. _seven-command-4F:

``4F`` "Request flash image"
----------------------------

.. todo:: Describe this command.

.. _seven-command-50:

``50`` "Transfer flash image"
-----------------------------

.. todo:: Describe this command.

.. _seven-command-51:

``51`` "Optimize filesystem" (storage)
--------------------------------------

.. seven-command::
    :code: 51
    :d5: Storage device
    :d5-example: fls0

    Transfer information regarding a file.

    This command is used in the following use cases:

    * :ref:`seven-optimize-storage`.

.. _seven-command-52:

``52`` "Request CASIOWIN entry transfer"
----------------------------------------

.. seven-command::
    :code: 52

    Request the CASIOWIN entry to be transferred using command
    :ref:`seven-command-53`.

.. _seven-command-53:

``53`` "Transfer CASIOWIN entry"
--------------------------------

.. seven-command::
    :code: 53

    Transfer the CASIOWIN entry.

.. _seven-command-54:

``54`` "Request bootcode transfer"
----------------------------------

.. seven-command::
    :code: 54

    Request the bootcode to be transferred using command
    :ref:`seven-command-55`.

.. _seven-command-55:

``55`` "Transfer bootcode"
--------------------------

.. seven-command::
    :code: 55

    Transfer the bootcode.

.. _seven-command-56:

``56`` "Upload and run"
-----------------------

Upload and run a specially crafted program.

.. warning::

    Please do not use this command lightly, as uploading badly formatted
    programs or the incorrect program for your calculator **can brick said
    calculator**!

This command does not use the typical command payload, but a **custom 24-bytes
payload** for which the format is the following:

.. list-table::
    :header-rows: 1

    * - Offset
      - Size
      - Name
      - Description
      - Values
    * - 0 (0x00)
      - 8 B
      - Upload size
      - Size of the payload to upload.
      - 8-char :ref:`seven-ascii-hex` number.
    * - 8 (0x08)
      - 8 B
      - Load address
      - RAM address at which to load the provided payload.
      - 8-char :ref:`seven-ascii-hex` number, usually set to
        ``88030000``.
    * - 16 (0x10)
      - 8 B
      - Start address
      - Address which to jump to after successful upload.
      - 8-char :ref:`seven-ascii-hex` number, usually set to
        ``88030000``.

This command is used in the following use cases:

* :ref:`seven-upload-and-run`.

``59`` "Backup main memory" (fx-CG)
-----------------------------------

.. todo:: Describe this command, known as "save Backup.g3m".

``5A`` "Restore main memory" (fx-CG)
------------------------------------

.. todo:: Describe this command, known as "restore Backup.g3m".
