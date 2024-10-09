CAS50 flows
===========

The CAS50 protocol involves a **sender** and a **receiver**.
Both sides are defined in advance, and do not exchange roles during transfer.

.. _cas50-init:

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

See :ref:`cas50-packet-format` for more information.

.. _cas50-send:

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

    on CAS50 headers, with a few exceptions, you can generally rely on the
    3-letter format instead (with exceptions), which is easier to support
    as it has fewer values.

Every data type can either be final, i.e. be the last one to be transmitted
on a given communication before the sender considers the communication as
completed, or not.

When only non-final data types are used, the sender can end the communication
manually by sending a sentinel header, in the form of a
:ref:`cas50-header-end` data type.

.. _cas50-request:

Request data using the CAS50 variant
------------------------------------

.. todo:: Write this!
