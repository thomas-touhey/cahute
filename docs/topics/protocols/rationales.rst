Rationales behind the communication protocols
=============================================

In order to use the communication protocols that come with CASIO calculators,
it is best to know what they're used for, and how the Cahute interface
represents those rationales.

.. _receive-protocol-rationale:

Data or screen reception rationale
----------------------------------

All CASIO calculators have the possibility to transmit data directly:

* In COMM or LINK, by selecting TRANSMIT.
* By using the "F<->D" key on older models, to transmit a screen capture.
* By using the screenstreaming protocols on newer models, to continually
  transmit the screen contents.

As a host device, we can receive such data by:

* On serial links, using the CASIOLINK protocol as a receiver.
* On USB links, using Protocol 7.00 OHP or UMS with Protocol 7.00 OHP above
  as a receiver.
* On USB or serial links, using either the CASIOLINK protocol with CAS100
  or Protocol 7.00 as a passive side, even though this only one usage of many
  for the control rationale; see :ref:`control-protocol-rationale`.

In this rationale, the calculator initiates the connection, so the host
actually does not need to know much about the protocol beforehand, only
the medium and related parameters.

In order to open the link for such a rationale:

* On serial links, you must use :c:func:`cahute_open_serial_link` with
  :c:macro:`CAHUTE_SERIAL_RECEIVER`, and can either use automatic protocol
  and CASIOLINK variant detection, or fix some of it:

  .. code-block:: c

        /* Full detection and automatic mode.
         * By default, this will use 9600 bauds, no parity, 1 stop bit. */
        cahute_open_serial_link(
            &link,
            CAHUTE_SERIAL_RECEIVER,
            "/dev/ttyUSB0",
            0
        );

        /* Fix the protocol to Protocol 7.00.
         * By default, this will use 9600 bauds, no parity, 2 stop bits. */
        cahute_open_serial_link(
            &link,
            CAHUTE_SERIAL_PROTOCOL_SEVEN | CAHUTE_SERIAL_RECEIVER,
            "/dev/ttyUSB0",
            0
        );

        /* Fix the protocol to CASIOLINK with CAS40 data format.
         * By default, this will use 4800 bauds, no parity, 1 stop bit,
         * and will force interpretation of the headers as CAS40 headers. */
         cahute_open_serial_link(
            &link,
            CAHUTE_SERIAL_PROTOCOL_CASIOLINK
            | CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS40
            | CAHUTE_SERIAL_RECEIVER,
            0
         );

* On USB links, you must use :c:func:`cahute_open_usb_link` or
  :c:func:`cahute_open_simple_usb_link` with :c:macro:`CAHUTE_USB_RECEIVER`,
  and optionally :c:macro:`CAHUTE_USB_OHP` for screen streaming protocols:

  .. code-block:: c

      /* Use data protocols. */
      cahute_open_simple_usb_link(&link, CAHUTE_USB_RECEIVER);

      /* Use screenstreaming protocols. */
      cahute_open_simple_usb_link(&link, CAHUTE_USB_OHP | CAHUTE_USB_RECEIVER);

.. todo::

    From here, the idea would be to either call ``cahute_receive_data`` or
    ``cahute_receive_screen``, but the first does not exist, and maybe we
    don't want to keep the same "callback" logic for the second?

.. _transmit-protocol-rationale:

Data or screen sending rationale
--------------------------------

All CASIO calculators excluding the fx-CG have a "Receive mode", in which
they can receive data from another party using the rationale above.

.. note::

    Except calculators only supporting the CASIOLINK protocol with CAS40
    data formats, all "receive modes" are actually passive modes in the
    control rationale; see :ref:`control-protocol-rationale`.

In order to open the link for such a rationale:

* On serial links, you must use :c:func:`cahute_open_serial_link` with the
  protocol and data format to use:

  .. code-block:: c

      /* Use CASIOLINK with CAS50 data headers. */
      cahute_open_serial_link(
          &link,
          CAHUTE_SERIAL_PROTOCOL_CASIOLINK
          | CAHUTE_SERIAL_CASIOLINK_VARIANT_CAS50,
          "/dev/ttyUSB0",
          0
      );

      /* Try instanciating multiple protocols using different check packets
       * and timeouts. */
      cahute_open_serial_link(
          &link,
          0,
          "/dev/ttyUSB0",
          0
      );

.. todo::

    From here, the idea would be to call ``cahute_send_data`` or
    ``cahute_send_screen``, but both do not exist as is for now.
    Also, in this case, should "storage file" be a type of data you can send?

.. _control-protocol-rationale:

Device control rationale
------------------------

Starting from the CASIOLINK protocol with CAS50 data formats, calculators
allow for a control-oriented rationale, with more commands available than
just "send".

The link is opened the same way as for the transmit protocol rationale;
see :ref:`transmit-protocol-rationale`. However, the functions to use are
more diverse and depend on the protocol and data format.

.. note::

    Getting data from the calculator differs in logic from the
    :ref:`receive-protocol-rationale`, since we no longer "receive" data that
    the device has chosen for us, but actively "request" data we chose.
