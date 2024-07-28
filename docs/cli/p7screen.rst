.. _p7screen:

``p7screen`` command line reference
===================================

p7screen is a utility originally provided with libp7_ back in
September of 2016 by `Thomas Touhey`_. It is used for receiving the
screen from any USB calculator in "Projector", "Screen Capture" or
"Screen Receiver" mode, and displaying it to a basic window using SDL_.

.. warning::

    This interface is provided by compatibility with libp7 / libcasio, and
    must not be changed to bring new features or change the syntax of existing
    ones.

For concrete steps on using p7screen, see :ref:`guide-cli-display-screen`.

Available options are the following:

``-l``, ``--log``
    Logging level to set the library as, as any of ``info``, ``warning``,
    ``error``, ``fatal``, ``none``.

    This option may allow contributors to visualize the effects of their
    contribution better, and allows users to produce a full output to join
    to an issue.

    See :ref:`logging` for more information.

``--com <device>``
    Path or name of the serial device with which to communicate,
    e.g. ``/dev/ttyUSB0``.

    If this option is not provided, p7screen will look for a calculator
    connected through USB.

``--use <settings>``
    Serial settings to instantiate the serial link with.

    This parameter is of the following format:

    .. code-block:: text

        <Speed in bauds><Parity><Stop bits>

    Where:

    * The speed in bauds is expressed as an integer, e.g. ``19200``.
    * The parity is either ``O`` for odd parity, ``E`` for even parity,
      and ``N`` if parity checks are disabled altogether.
    * The stop bits is either ``1`` or ``2``.

    By default, the serial link is instanciated with a speed of 9600 bauds,
    no parity and 2 stop bits (``9600N2``).

    This option is ignored if the path or name of a serial device is not
    provided using the ``--com`` option.

``-z``, ``--zoom``
    Zoom, as an integer from 1 to 16.

    Having a zoom of N means that a single pixel on the calculator
    will be displayed as an NxN full square on the host.

Invalid options are ignored by p7screen. If an option is provided several time,
only the latest occurrence will be taken into account.

.. _libp7: https://web.archive.org/web/20230401210038/https://p7.planet-casio.com/en.html
.. _Thomas Touhey: https://thomas.touhey.fr/
.. _SDL: https://www.libsdl.org/
