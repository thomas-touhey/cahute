CAS100 flows
============

The CAS100 protocol involves a **sender** and a **receiver**.
Both sides are defined in advance, and do not exchange roles during transfer.

.. _cas100-init:

Initiate the connection
-----------------------

The CAS100 initiation flow is more complete than the CAS40 and CAS50 variants:

.. mermaid::

    sequenceDiagram
        Participant sender as Sender
        Participant receiver as Receiver

        sender->>receiver: Send a 0x16 (START)
        receiver->>sender: Send a 0x13 (ESTABLISHED)

        sender->>receiver: Send an MDL1 header (0x3A)
        receiver->>sender: Answer with an MDL1 header (0x3A)
        sender->>receiver: Acknowledge (0x06)
        receiver->>sender: Acknowledge (0x06)

.. note::

    On cross-variant CASIOLINK reception, since the MDL1 header is received
    in the place any other data would be received in the CAS40 and CAS50
    variants, the MDL1 header and acknowledgement reactions must be
    managed in the data reception utilities rather than in the
    communication initialization.

    However, when the CAS100 variant is selected explicitely by the user,
    the MDL1 header can and should be managed in the communication
    initialization directly, so that device information can be exploited.

See the following for more information:

* :ref:`cas100-packet-format`;
* :ref:`cas100-header-mdl1`.

.. _cas100-send:

Send data
---------

.. todo:: Write this!
