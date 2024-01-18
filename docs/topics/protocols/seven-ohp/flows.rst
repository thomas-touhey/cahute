Protocol 7.00 Screenstreaming communication flows
=================================================

Screenstreaming with Protocol 7.00 involves a **sender** and a **receiver**.
The objective with this protocol is for the sender to be able to
send frames as they are produced to the receiver.

Except for a conditional acknowledgement at the beginning of the flow,
the receiver does not send anything, and **in case of invalid checksum,
it discards such packets**.

The communication schema is the following:

.. mermaid::

    sequenceDiagram
        Participant sender as Sender<br />(Calculator)
        Participant receiver as Receiver<br />(PC, ...)

        alt Sender requires acknowledgement from the Receiver
            sender->>receiver: Check packet
            receiver->>sender: Acknowledgement packet
        end

        loop Screenstreaming is active
            sender->>receiver: Frame packet
        end

.. warning::

    Sometimes, due to the high volume of data, a few bytes can be skipped
    here and there, leading to a desynchronization on the input stream.
    It is recommended, on the receiving function, to add an option to
    find the beginning of the next screenstreaming packet, to fix such
    desynchronized inputs.

.. _seven-ohp-acknowledge:

Acknowledge a Sender
--------------------

On certain conditions, such as when the ``ScreenRecv(XP)`` mode is used on
the fx-CG50, the sender requires acknowledgement from the receiver.
As such, it will continuously send check packets, such as the following one::

    16 43 41 4c 30 30 44 30  .CAL00D0

In such cases, the receiver must answer with the following acknowledgement
packet::

    06 30 32 30 30 31 30 44  .020010D

Once the sender has received acknowledgement, it will start sending
frame packets.

For more information, see :ref:`seven-ohp-check-packet` and
:ref:`seven-ohp-ack-packet`.
