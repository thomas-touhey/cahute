.. _protocol-cas50:

CAS50 protocol -- Serial protocol used by calculators from 1996 to 2004
=======================================================================

The CAS50 is an evolution of the CAS40 protocol for CASIO calculators
from 1996 to 2004, excluding AFX / Graph 100 and Classpad 300 / 330 (+)
models which have their own protocol variations.

.. note::

    The term was coined by Thomas Touhey, as an evolution of CAS40 with
    50, the size of the headers in the variant, in bytes.

For this protocol, serial settings are always the same, and do not vary
during the transfer:

* Speed: **9600** bauds.
* Parity: **none**.
* Stop bits: **1**.

See the following sections for more details regarding the protocol.

.. toctree::
    :maxdepth: 2

    cas50/packet-format
    cas50/data-types
    cas50/flows
