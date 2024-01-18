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

``-z``, ``--zoom``
    Zoom, as an integer from 1 to 16.

    Having a zoom of N means that a single pixel on the calculator
    will be displayed as an NxN full square on the host.

``-l``, ``--log``
    Logging level to set the library as, as any of ``info``, ``warning``,
    ``error``, ``fatal``, ``none``.

    This option may allow contributors to visualize the effects of their
    contribution better, and allows users to produce a full output to join
    to an issue.

    See :ref:`logging` for more information.

Invalid options are ignored by p7screen. If an option is provided several time,
only the latest occurrence will be taken into account.

.. _libp7: https://p7.planet-casio.com/
.. _Thomas Touhey: https://thomas.touhey.fr/
.. _SDL: https://www.libsdl.org/
