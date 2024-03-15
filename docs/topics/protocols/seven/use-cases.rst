Protocol 7.00 use cases
=======================

This document inventories use cases of Protocol 7.00 that are exploited by
the library and command-line utilities.

.. _seven-send-file-to-storage:

Send a file to storage
----------------------

In order to send a file to storage, the flow is the following:

.. mermaid::

    sequenceDiagram
        Participant user as User
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Command '45'

        alt Overwrite confirmation is required
            passive->>active: Overwrite confirmation requested
            active->>user: Request user confirmation
            user->>active: Confirm or reject

            alt User confirms overwrite
                active->>passive: Confirm overwrite
            else
                active->>passive: Reject overwrite
            end
        end

        alt Error has occurred
            passive->>active: Error
        else
            passive->>active: Acknowledge

            Note over active,passive: Data transfer flow from<br />active side to passive
        end

For more information, consult the following sections:

* Command :ref:`seven-command-45`;
* :ref:`seven-confirm-overwrite`;
* :ref:`seven-transmit-data`.

This flow is used by ``p7 send``; see the :ref:`p7-send` for more information.

.. _seven-request-file-from-storage:

Request a file from storage
---------------------------

In order to request a file to storage, the flow is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Original Active Side
        Participant passive as Original Passive Side

        active->>passive: Command '44'

        alt Error has occurred
            passive->>active: Error
        else
            passive->>active: Acknowledge
            active->>passive: Roleswap

            passive->>active: Command '45'
            active->>passive: Acknowledge

            Note over active,passive: Data transfer flow from<br />original passive side to active

            passive->>active: Roleswap
        end

For more information, consult the following sections:

* Command :ref:`seven-command-44`;
* Command :ref:`seven-command-45`;
* :ref:`seven-request-transfer`;
* :ref:`seven-transmit-data`.

This flow is used by ``p7 get``; see the :ref:`p7-get` for more information.

.. _seven-copy-file-on-storage:

Copy a file to another on storage
---------------------------------

In order to copy a file on storage, the flow is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Command '48'

        alt Error has occurred
            passive->>active: Error
        else
            passive->>active: Acknowledge
        end

For more information, consult the following sections:

* Command :ref:`seven-command-48`.

This flow is used by ``p7 copy``; see the :ref:`p7-copy` for more information.

.. _seven-delete-file-on-storage:

Delete a file from storage
--------------------------

In order to delete a file on storage, the flow is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Command '46'

        alt Error has occurred
            passive->>active: Error
        else
            passive->>active: Acknowledge
        end

For more information, consult the following sections:

* Command :ref:`seven-command-46`.

This flow is used by ``p7 delete``; see the :ref:`p7-delete`
for more information.

.. _seven-list-files-on-storage:

List files on storage
---------------------

In order to list files on storage, the flow is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Original Active Side
        Participant passive as Original Passive Side

        active->>passive: Command '4D'

        alt Error has occurred
            passive->>active: Error
        else
            passive->>active: Acknowledge
            active->>passive: Roleswap

            loop All file information has not been sent
                passive->>active: Command '4E'
                active->>passive: Acknowledge
            end

            passive->>active: Roleswap
        end

For more information, consult the following sections:

* Command :ref:`seven-command-4D`;
* Command :ref:`seven-command-4E`;
* :ref:`seven-request-transfer`.

This flow is used by ``p7 list``; see the :ref:`p7-list` for more information.

.. _seven-reset-storage:

Reset a storage device
----------------------

In order to reset a storage device, the flow is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Command '4A'

        alt Error has occurred
            passive->>active: Error
        else
            passive->>active: Acknowledge
        end

For more information, consult the following sections:

* Command :ref:`seven-command-4A`.

This flow is used by ``p7 reset``; see the :ref:`p7-reset`
for more information.

.. _seven-optimize-storage:

Optimize a storage device
-------------------------

In order to optimize a storage device, the flow is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Command '51'

        alt Error has occurred
            passive->>active: Error
        else
            passive->>active: Acknowledge
        end

For more information, consult the following sections:

* Command :ref:`seven-command-51`.

This flow is used by ``p7 optimize``; see the :ref:`p7-optimize` for more
information.

.. _seven-backup-rom:

Back up the system
------------------

In order to back up the flash, the flow is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Original Active Side<br />(PC, ...)
        Participant passive as Original Passive Side<br />(calculator)

        active->>passive: Command '4F'

        alt Error has occurred
            passive->>active: Error
        else
            passive->>active: Acknowledge
            active->>passive: Roleswap
            passive->>active: Command '50'
            active->>passive: Acknowledge

            Note over active,passive: Data flow from calculator to PC

            passive->>active: Roleswap
        end

This flow is only available in older models, and has been removed since;
see :ref:`seven-devices` for more information.

For more information, consult the following sections:

* Command :ref:`seven-command-4F`;
* Command :ref:`seven-command-50`;
* :ref:`seven-request-transfer`.

This flow is used by ``p7os get``; see the :ref:`p7os-get` for more
information.

.. _seven-upload-and-run:

Upload and run an executable program
------------------------------------

.. warning::

    This is a dangerous flow, and is only documented here for completeness.
    Programs suitable for this command, nicknamed "Update.EXE", are crafted
    in a very specific way, and you should not attempt to make your own.

In order to upload and run an executable program, the flow is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Command '56'

        alt Error has occurred
            passive->>active: Error
        else
            passive->>active: Acknowledge

            Note over active,passive: Data transfer from active to passive
        end

If successful, the last acknowledgement from the data transfer will have
the special subtype ``03``, which means that the communication is terminated
as control of the device is handed over to the program.

This use case is employed for updating the calculator, by uploading and running
special programs whose role is to open a USB or serial link, and receive and
flash the new OS at the command of the host. This has also been employed by
the community to do the same, such as with fxRemote_.

For more information, consult the following sections:

* Command :ref:`seven-command-56`;
* :ref:`seven-transmit-data`.

.. _seven-fxremote-flash:

Flash the calculator using fxRemote
-----------------------------------

.. warning::

    This is a **very** dangerous flow, and is only documented here for
    completeness. It also is specific to the fxRemote's Update.EXE,
    and is unofficial.

In order to flash a new OS on an fx-9860G or compatible, the flow is
the following:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        Note over active,passive: Passive side is the bootloader.

        active->>passive: Command '56'
        passive->>active: Ack
        Note over active,passive: Data flow from active to passive (Update.EXE)

        Note over active,passive: Passive side becomes fxRemote's Update.EXE.

        active->>passive: Initialize connection
        passive->>active: Acknowledge

        active->>passive: Check hardware configuration (command '76')
        passive->>active: Acknowledge
        passive->>active: Identification data

        loop From 0xA0010000 to 0xA0280000 excluded, by increments of 0x10000
            active->>passive: Clear data (command '72')
            passive->>active: Acknowledge
        end

        loop From 0xA0020000 to 0xA0280000 excluded, then 0xA0010000, by increments of 0x10000
            loop Data left to send for sector
                active->>passive: Send sector data in buffer (command '70')
                passive->>active: Acknowledge
            end

            active->>passive: Request sector copy to flash (command '71')
            passive->>active: Acknowledge
        end

        active->>passive: Request for termination (command '78')
        passive->>active: Acknowledge

For serial links between the active side and fxRemote as the passive
side, the link should use 115200 bauds, no parity, 2 stop bits,
XON/XOFF software control and, if supported by the cable, DTR/RTS hardware
control.

For more information, consult the following sections:

* :ref:`seven-upload-and-run`
* :ref:`seven-fxremote-command-70`
* :ref:`seven-fxremote-command-71`
* :ref:`seven-fxremote-command-72`
* :ref:`seven-fxremote-command-76`
* :ref:`seven-fxremote-command-78`

.. _fxRemote: https://tiplanet.org/forum/archives_voir.php?id=4484
