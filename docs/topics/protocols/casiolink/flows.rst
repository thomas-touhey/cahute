.. _casiolink-flows:

Packet flows for the CASIOLINK protocol
=======================================

The CASIOLINK protocol involves a **sender** and a **receiver**.
Both sides are defined in advance, and do not exchange roles during transfer.
In no cases can the receiver request a resource from the sender, as it
must only respond to requests and receive what the sender chooses to send.

Initiate the connection
-----------------------

In order to initiate the connection, the communication schema is the following:

.. mermaid::

    sequenceDiagram
        Participant sender as Sender
        Participant receiver as Receiver

        sender->>receiver: Send a 0x16 (START)
        receiver->>sender: Send a 0x13 (ESTABLISHED)

.. todo:: Determine the role of all of that.
