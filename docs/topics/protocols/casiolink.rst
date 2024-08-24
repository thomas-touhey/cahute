.. _protocol-casiolink:

CASIOLINK protocol -- Serial protocols used by pre fx-9860G calculators
=======================================================================

The CASIOLINK protocol is used on graphing calculators from the early 1990s to
the AFX / Graph 100, before the fx-9860G came out in 2004. It is used on serial
links exclusively.

It has three variants with slight variations:

* One for pre-1996 calculators, named "CAS40";
* One for calculators from 1996 to 2004, excluding the AFX, named "CAS50";
* One for the AFX / Graph 100, from 1999 to 2003, named "CAS100";
* One for the Classpad 300 / 330 (+), from 2003 to 2013, named "CAS300".

Variations include packet and header formats, as well as available flows.
They however share the same rationale in handling communication, using a
``0x16`` / ``0x13`` initial handshake and using USB as a bulk-only equivalent
of serial communication, as well as overall rationale compatibility.

.. toctree::
    :maxdepth: 1

    casiolink/packet-format
    casiolink/cas300-commands
    casiolink/flows
