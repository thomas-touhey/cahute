.. _guide-cli-send-file:

Sending a file to a calculator's storage memory
===============================================

.. warning::

    This guide is currently only available for fx-9860G and compatible
    calculators, excluding the fx-CG line of calculators, which need to
    be used as a normal storage device ("USB Key" mode).

In this guide, we will assume you want to transfer the ``MYADDIN.G1A`` file
from your PC to your calculator.

The instructions vary depending on the way your calculator is connected,
see the correct section for your case.

For more options, see :ref:`p7` and :ref:`p7-send`.

If you are using USB
--------------------

The steps are the following:

1. Connect the calculator to the PC using a mini USB cable.
2. Select the ``DataTrans`` (``TransfDon`` in French) mode when prompted.
3. Run the transfer from the PC using ``p7 send``.

Once steps 1 and 2 are done, run the following command::

    p7 send MYADDIN.G1A

If the file already exists, you will be prompted on whether you want to
overwrite the file on the calculator. In this case, you can either:

* Enter ``y`` and press enter, to confirm the overwrite.
* Enter ``n`` and press enter, to reject the overwrite.

Your file has successfully been transferred to the calculator!

If you are using serial
-----------------------

The steps are the following:

1. Connect the calculator to the PC using a USB to serial cable.
2. Configure the calculator to use serial and data transfer, and place
   the calculator in standby mode.
3. Find out which serial port your calculator is connected on.
4. Run the transfer from the PC using ``p7 send``.

For step 2, go to your link application, and do the following:

* If a ``CABL`` (F4) submenu is available, go into it and press F2
  to select "3-pin" ("3 broches" in French);
* Go to the ``CAPT`` (F6) submenu and press F1 to select "Memory"
  ("Mémoire" in French).
* Place your calculator in receiving mode by pressing F2 to select the
  ``RECV`` submenu.

For step 3, you must run the following command to discover all available
serial ports p7 can use:

.. code-block:: bash

    p7 list-devices

Unfortunately, if there are several ports, there is no easy way to
find out which port is the right one to use; you will need to try
each of them.

For step 4, you can run the transfer by using the following command:

.. code-block:: bash

    p7 --com <your-serial-device> send MYADDIN.G1A

If the file already exists, you will be prompted on whether you want to
overwrite the file on the calculator. In this case, you can either:

* Enter ``y`` and press enter, to confirm the overwrite.
* Enter ``n`` and press enter, to reject the overwrite.

Your file has successfully been transferred to the calculator!
