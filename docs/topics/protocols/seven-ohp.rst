.. _protocol-seven-ohp:

Protocol 7.00 Screenstreaming -- fx-9860G and fx-CG screenstreaming
===================================================================

Screenstreaming, also named "OverHead Projector" (OHP), "Projector", or
"Screen Receiver"\ [#screenreceiver]_, is a mode in which the calculator
shares its screen contents to another device.

This protocol is analogous to Protocol 7.00, see :ref:`protocol-seven` for
more information. It is used on the same models and during the same period
of time.

However, it has its own packet formats and flows that make it effectively
a completely separate protocol, sharing only a few similarities. Therefore,
it is documented in a completely separated set of sections and documents
from the original protocol, for clarity.

.. toctree::
    :maxdepth: 1

    seven-ohp/packet-format
    seven-ohp/flows

.. [#screenreceiver] `Screen Receiver`_ is the name of the piece of software
   by CASIO to view the calculator's screen from a desktop PC running on
   MacOS, OS X or Microsoft Windows.

.. _Screen Receiver:
    https://www.planet-casio.com/Fr/logiciels/voir_un_logiciel_casio.php
    ?showid=102
