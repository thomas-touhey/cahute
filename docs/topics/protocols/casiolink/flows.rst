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

Send or receive data using the CAS40 or CAS50 variant
-----------------------------------------------------

For the :ref:`receive-protocol-rationale` or :ref:`transmit-protocol-rationale`
using the CAS40 or CAS50 variants, the flow is the following:

.. mermaid::

    sequenceDiagram
        Participant sender as Sender
        Participant receiver as Receiver

        sender->>receiver: Send header (0x3A)

        alt An error has occurred
            receiver->>sender: Send an error
        else
            receiver->>sender: Acknowledge (0x06)

            loop A data part is expected
                sender->>receiver: Send a data part (0x3A)
                receiver->>sender: Acknowledge (0x06)
            end
        end

The number of data parts, and the size of each of them, is determined using
on the header, and is **not** communicated using the protocol itself.

.. note::

    This is the opposite approach from Protocol 7.00, where the interpretation
    can be done entirely after the file has been sent, even for unknown
    data types. Here, you need to make at least a partial interpretation
    of the header.

    On CAS40 headers, the interpretation must be done per data type, whereas
    on CAS50 headers, with a few exceptions, you can generally rely on the
    3-letter format instead (with exceptions), which is easier to support
    as it has fewer values.

Every data type can either be final, i.e. be the last one to be transmitted
on a given communication before the sender considers the communication as
completed, or not.

When only non-final data types are used, the sender can end the communication
manually by sending a sentinel header, in the form of a
:ref:`casiolink-cas40-end` or :ref:`casiolink-cas50-end` data type.

.. _casiolink-cas40-al-mode:

CAS40 AL Mode
~~~~~~~~~~~~~

When a sender using CAS40 wants to transmit all of its data, it can use an
AL mode, which causes the following flow:

.. mermaid::

    sequenceDiagram
        Participant sender as Sender
        Participant receiver as Receiver

        sender->>receiver: Send AL header (0x3A, "AL" data type)
        receiver->>sender: Acknowledge (0x06)

        loop Data is to be sent
            Note over sender,receiver: Sender sends header and data parts
        end

        sender->>receiver: Send AL End header (0x3A, "\x17\x17" data type)
        receiver->>sender: Acknowledge (0x06)

        Note over sender,receiver: Communication ends

In this mode, **all data types that are normally final become non-final**.
This includes :ref:`casiolink-cas40-end`, which does not end the communication
anymore, as once this mode is enabled, only :ref:`casiolink-cas40-al-end`
is able to do this.

See the following for more information:

* :ref:`casiolink-cas40-al`
* :ref:`casiolink-cas40-al-end`

Request data using the CAS50 variant
------------------------------------

.. todo:: Write this!

Send or receive data using the CAS100 variant
---------------------------------------------

.. todo:: Write this!
