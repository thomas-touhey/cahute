Protocol 7.00 communication flows
=================================

Protocol 7.00 involves an **active side** and a **passive side**.
At the beginning of the communication, the passive side is usually the
calculator, and the active side is the client, e.g. a PC or another
calculator.

When a flow isn't currently in progress, the active side is allowed to send
a command to the passive side. This command may start a flow where a
"role swap" occurs, i.e. the active side becomes the passive side, and vice
versa. At the end of said flow, the role are usually swapped again, so that
the original active side becomes the active side again.

When required, the role swap is always initiated by the active side.

The communication schema is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Original Active Side<br>(PC, ...)
        Participant passive as Original Passive Side<br>(calculator)

        Note over active,passive: Initialize the link

        active->>passive: Command
        alt Erroneous
            passive->>active: Error
        else
            passive->>active: ACK

            alt Data transfer is required
                Note over active,passive: Data transfer
            else
                alt Data transfer must be requested by the active side
                    Note over active,passive: Data transfer request
                end
            end
        end

        Note over active,passive: Terminate the link

The different blocks are described in the following sections.

.. note::

    If any packet is received with an invalid checksum, a specific flow
    takes place for the sender to be able to understand the situation and
    resend said packet.

    This sub-flow is considered part of the "sending a packet" flow, and
    must be treated within this logic.

    See :ref:`seven-report-invalid-checksum` for more information.

.. note::

    In between any of the packet sending steps, if the side expected to send
    a packet takes too long to do so, a sub flow takes place to check if the
    other side is still present.

    See :ref:`seven-check-link` for more information.

.. _seven-report-invalid-checksum:

Reporting invalid checksums
---------------------------

In case a packet has an invalid checksum, it may mean that the packet has
been corrupted by the medium lying under the link. In such cases, a specific
flow takes place so that the sender of the packet can resend the packet.

An example flow where the active side sends the corrupted packet
is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Command with invalid checksum
        passive->>active: Error with '01' subtype
        active->>passive: Same command with valid checksum
        Note over active,passive: ...

An example flow where the passive side sends the corrupted packet is
the following:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Command
        passive->>active: ACK with invalid checksum
        active->>passive: Error with '01' subtype
        passive->>active: ACK with valid checksum
        Note over active,passive: ...

.. _seven-init-link:

Initiating the link
-------------------

When opening a link to a calculator that hasn't received any packets yet,
the PC must initialize the link using this flow, which is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Initial check
        passive->>active: ACK

The initial check packet is a check packet of subtype ``00``; see
:ref:`seven-check-packet` for more information.

.. _seven-terminate-link:

Terminating the link
--------------------

When closing a link to a calculator that we don't plan on sending packets
anymore, we are expected to terminate the link using the following flow:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Terminate
        passive->>active: ACK

See :ref:`seven-terminate-packet` for more information on the terminate packet.

.. _seven-check-link:

Checking up on the link
-----------------------

In order to detect a "deaf" passive side, the active side must run the
following flow if inactive for more than 6 minutes:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Regular check
        passive->>active: ACK

The regular check is a check packet with the ``01`` subtype; see
:ref:`seven-check-packet` for more information.

If this flow is not run and the connection is active for more than 6 minutes,
the passive side terminates the connection.

.. _seven-confirm-overwrite:

Confirming or rejecting overwrite
---------------------------------

Some commands, such as :ref:`seven-command-45`, may require overwrite
confirmation in case the file already exists on the calculator. In such
cases, the flow is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Command with OW set to '00'
        passive->>active: Overwrite confirmation requested
        active->>active: Request user confirmation

        alt User confirms overwrite
            active->>passive: Confirm overwrite
            passive->>active: Acknowledge
        else
            active->>passive: Deny overwrite
            passive->>active: Acknowledge
        end

The packets used in this sequence are the following:

* The overwrite confirmation request is represented as a NAK packet with
  the ``02`` subtype; see :ref:`seven-nak-packet`.
* The overwrite confirmation is represented as an ACK packet with the
  ``01`` subtype; see :ref:`seven-ack-packet`.
* The overwrite denial is represented as a NAK packet with the ``03``
  subtype; see :ref:`seven-nak-packet`.
* Both the overwrite confirmation acknowledgement and overwrite denial
  acknowledgement are represented as an ACK packet with the ``00`` subtype;
  see :ref:`seven-ack-packet`.

.. _seven-transmit-data:

Transferring data
-----------------

Say that an active and a passive side have agreed on a data exchange through
commands, and have already swapped if necessary. In order to transfer data,
the flow is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        loop Data left to transmit
            active->>passive: Data packet
            passive->>active: ACK
        end

Data packets are described in :ref:`seven-data-packet`.

.. note::

    Note that the number of data packets must be known in advance, since all
    data packets contain both the sequence number and the total sequence count,
    e.g. "packet 51/128".

    In order to know the number of packets an original buffer can take, due
    to escaping concerns, it is highly recommended to consider that all packets
    contain up to 256 bytes, except the last one that may contain less.
    For example:

    * 500 bytes will be represented as 2 data packets (one of 256 bytes,
      one of 244 bytes).
    * 512 bytes will be represented as 2 data packets (both of 256 bytes).
    * 1055 bytes will be represented as 5 data packets (four of 256 bytes,
      one of 31 bytes).

Packet shifting
~~~~~~~~~~~~~~~

Packet shifting is a technique discovered in 2017 that makes data transfers
faster. It consists in sending the next data packet before the acknowledgement
for the previous one is received.

The flow becomes the following:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: First data packet

        loop Data left to transmit
            active->>passive: Send data packet #N
            passive->>active: ACK for data packet #N - 1
        end

        passive->>active: ACK for last data packet

.. warning::

    This technique comes with its risks, especially the fact that it renders
    the link non-recoverable in case of bad packet checksum while it is
    in effect, since the packet correction flow assumes that no "normal"
    packet is sent after the problematic packet (while we have already sent
    the packet that comes after in the sequence).

    In order to mitigate such risks while still employing the technique,
    Cahute disables packet shifting on serial links, i.e. only enables it
    on USB and UAS (SCSI) links.

.. _seven-request-transfer:

Requesting data transfer(s)
---------------------------

When requesting a file or some data generally from the calculator,
the usual flow is the following:

.. mermaid::

    sequenceDiagram
        Participant active as Original Active Side
        Participant passive as Original Passive Side

        active->>passive: Command (transfer request)
        passive->>active: ACK
        active->>passive: Roleswap

        Note over active,passive: Active side becomes passive, and vice versa

        loop Transfer required
            passive->>active: Command (transfer)
            active->>passive: ACK
            Note over active,passive: Data flow from calculator to PC
        end

        passive->>active: Roleswap

        Note over active,passive: Original active side becomes active again

This applies to several use cases, but an example one is the PC requesting
a file from the calculator's flash memory. The transfer request command for
such a case is :ref:`seven-command-24`, and the transfer command emitted
by the calculator after the roleswap is :ref:`seven-command-25`.

.. note::

    The transfer request can lead to multiple commands from the calculator,
    e.g. with commands such as :ref:`seven-command-29` that will spawn
    one command and associated data transmission by file in the main memory.

.. _seven-get-device-information:

Requesting device information
-----------------------------

While in the "requesting transfer" rationale, the flow to get the device
information using command :ref:`seven-command-01` is different from the
usual solution applied for this case:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Command '01'
        passive->>active: EACK with device information

.. note::

    This is likely to avoid a more complex flow with a roleswap, as described
    in :ref:`seven-request-transfer`, or because the aforementioned flow did
    not exist when this command was conceived.

.. _seven-update-serial-params:

Updating serial parameters
--------------------------

If the link is established on a serial stream, it is possible to negotiate
different serial parameters with the calculator dynamically using
:ref:`seven-command-02`, using the following flow:

.. mermaid::

    sequenceDiagram
        Participant active as Active Side
        Participant passive as Passive Side

        active->>passive: Command '02'

        alt Error has occurred
            passive->>active: Error
        else
            passive->>active: Acknowledge

            Note over active,passive: Both sides update their<br />serial parameters before<br />their next packet exchanges.
        end

The only elements that can be updated are the serial speed in bauds, the
parity, and the number of stop bits (1 or 2).

The serial speeds are limited to the following speeds (or baud rates):
300, 600, 1200, 2400, 4800, 9600 (*by default*), 19200, 38400, 57600 and
115200 bauds.

.. warning::

    Depending on the quality of the serial cable (or USB to serial cable
    / converter), higher speeds may cause more corruption to occur, causing
    a lot of resends and hence, being less efficient than lower baud rates.
