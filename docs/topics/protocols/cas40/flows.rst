.. _cas40-flows:

CAS40 flows
===========

The CAS40 protocol involves a **sender** and a **receiver**.
Both sides are defined in advance, and do not exchange roles during transfer.

.. _cas40-init:

Initiate the connection
-----------------------

In order to initiate the connection, the communication schema is the
following:

.. mermaid::

    sequenceDiagram
        Participant sender as Sender
        Participant receiver as Receiver

        sender->>receiver: Send a 0x16 (START)
        receiver->>sender: Send a 0x13 (ESTABLISHED)

See :ref:`cas40-packet-format` for more information.

.. _cas40-send:

Send or receive data
--------------------

For the :ref:`receive-protocol-rationale` or
:ref:`transmit-protocol-rationale`, the flow is the following:

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

    On CAS40 headers, the interpretation must be done per data type.

Every data type can either be final, i.e. be the last one to be transmitted
on a given communication before the sender considers the communication as
completed, or not.

When only non-final data types are used, the sender can end the communication
manually by sending a sentinel header, in the form of a
:ref:`cas40-header-end` data type.

.. _cas40-al-mode:

AL Mode
~~~~~~~

When a sender wants to transmit all of its data, it can use an AL mode,
which causes the following flow:

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
This includes :ref:`cas40-header-end`, which does not end the communication
anymore, as once this mode is enabled, only :ref:`cas40-header-al-end`
is able to do this.

See the following for more information:

* :ref:`cas40-header-al`
* :ref:`cas40-header-al-end`
