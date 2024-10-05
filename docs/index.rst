Cahute |version|
================

Cahute is a library and set of command-line utilities to handle serial
and USB communication protocols and file formats related to CASIO calculators,
dating from the 1990s to today. It provides the following
features\ [#mutant]_\ :

.. feature-list::

    * - |feat-transfer|
      - File transfer between storages
      - With ``p7``, transfer files from and to storages on
        fx-9860G compatible calculators, over USB and serial links!
    * - |feat-program|
      - Program backup
      - With ``CaS``, extract programs from all CASIO calculators since 1991,
        over USB and serial links!
    * - |feat-text|
      - Text conversions
      - With :c:func:`cahute_convert_text`, convert text between standard and
        CASIO-specific text encodings!
    * - |feat-ohp|
      - Screenstreaming
      - With ``p7screen``, display screen captures and streaming from your
        calculator on your host system!
    * - |feat-flash|
      - ROM flashing
      - With ``p7os``, flash fx-9860G and compatible calculators!

Officially supported systems are the following:

.. system-list::

    * - |system-arch|
      - Archlinux_ and derivatives
    * - |system-apple|
      - macOS_

.. note::

    Support and distribution of other platforms is in progress or is awaiting
    volunteers:

    .. system-list::

        * - |system-debian|
          - Debian_ and derivatives (`#8
            <https://gitlab.com/cahuteproject/cahute/-/issues/8>`_)
        * - |system-win|
          - `Microsoft Windows`_ XP and above (`#10
            <https://gitlab.com/cahuteproject/cahute/-/issues/10>`_)
        * - |system-amigaos|
          - AmigaOS_ 3.2 and above (`#26
            <https://gitlab.com/cahuteproject/cahute/-/issues/26>`_)

The project is being worked on `on Gitlab <Cahute on Gitlab_>`_.
It is maintained by `Thomas Touhey`_. See :ref:`project-forums` for the
topics describing the projects in other communities.

The project's code and documentation contents are licensed under CeCILL_
version 2.1 as distributed by the CEA, CNRS and Inria on
`cecill.info <CeCILL_>`_.

This documentation is organized using `Diátaxis`_' structure.

Acknowledgements
----------------

There have been many projects over the years about reversing and
reimplementing CASIO's shenannigans for using their calculators from
alternative OSes, or simply for fun or out of curiosity. Cahute couldn't
have been made without their research, implementations and in some cases,
documentation. This page is a little tribute to these works.

* Thanks to Tom Wheeley and Tom Lynn for their work on CaS and Caspro_
  and the `Casio Graphical Calculator Encyclopaedia`_.
* Thanks to the (now defunct) Graph100.com wiki, `saved here
  <Graph100.com Wiki_>`_ for historical purposes.
* Thanks to the team behind Casetta_ for their documentation on legacy
  protocols and file formats, which helped me navigate the subtleties more
  easily.
* Thanks to `Simon Lothar`_ and Andreas Bertheussen for their work on
  Protocol 7.00 and derivatives through fxReverse_ and xfer9860, and to
  Teamfx_ for `their additions <Teamfx additions_>`_.
* Thanks to the Cemetech community for their `Prizm Wiki`_, especially
  gbl08ma, BrandonWilson and amazonka.
* Thanks to Nessotrin_ for their work on UsbConnector_, which prompted me
  to work on a better version in the first place.

There are obviously plenty more people working on other connected aspects
(hardware, low-level system stuff), administering or moderating forums and
websites, maintaining communication with CASIO and other partners.
Quoting you all would take a substantial time, and I'd likely miss quite a lot
of you, but thank you all for your efforts!

How-to guides
-------------

These sections provide guides, i.e. recipes, targeted towards various actors.
They guide you through the steps involved in addressing key problems
and use-cases.

.. toctree::
    :maxdepth: 3

    guides
    cli-guides
    developer-guides

Discussion topics
-----------------

These sections discuss key topics and concepts at a fairly high level,
and provide useful background information and explanation.

.. toctree::
    :maxdepth: 3

    data-formats
    communication-protocols
    abstractions
    internals

References
----------

These sections provide technical reference for APIs and other aspects of
Cahute's machinery. They go into detail, and therefore, assume you have a
basic understanding of key concepts.

.. toctree::
    :maxdepth: 3

    cli
    headers

Project management
------------------

These sections describe how the project is managed, and what works it
is based on.

.. toctree::
    :maxdepth: 3

    project

.. [#mutant] Icons used here are from the `Mutant Standard`_,
  licensed under `CC BY-NC-SA 4.0 International`_.
  Copyright © 2017 - 2024 \ `Caius Nocturne`_.

.. |feat-transfer| image:: feat-transfer.svg
.. |feat-program| image:: feat-program.svg
.. |feat-text| image:: feat-text.svg
.. |feat-ohp| image:: feat-ohp.svg
.. |feat-flash| image:: feat-flash.svg
.. |system-arch| image:: guides/arch.svg
.. |system-apple| image:: guides/apple.svg
.. |system-win| image:: guides/win.png
.. |system-debian| image:: guides/debian.svg
.. |system-amigaos| image:: guides/amigaos.png

.. _Archlinux: https://archlinux.org/
.. _macOS: https://www.apple.com/macos/
.. _Microsoft Windows: http://windows.microsoft.com/
.. _Debian: https://www.debian.org/
.. _AmigaOS: https://www.amigaos.net/

.. _Cahute on Gitlab: https://gitlab.com/cahuteproject/cahute
.. _Thomas Touhey: https://thomas.touhey.fr/
.. _CeCILL: http://www.cecill.info/licences.en.html
.. _Diátaxis: https://diataxis.fr/

.. _Simon Lothar:
    https://www.casiopeia.net/forum/memberlist.php?mode=viewprofile&u=10405
.. _Teamfx:
    https://www.casiopeia.net/forum/memberlist.php?mode=viewprofile&u=10504&sid=b1f4fb842b29e6f686d832a7e1117789
.. _Nessotrin:
    https://www.planet-casio.com/Fr/compte/voir_profil.php?membre=nessotrin

.. _Casio Graphical Calculator Encyclopaedia:
    https://serval.mythic-beasts.com/~tom/calcs/calcs/encyc/
.. _Graph100.com Wiki:
    https://bible.planet-casio.com/cakeisalie5/websaves/graph100.com/
.. _fxReverse:
    https://bible.planet-casio.com/simlo/fxreverse/fxReverse2.pdf
.. _Teamfx additions: https://bible.planet-casio.com/teamfx/
.. _Prizm Wiki: https://prizm.cemetech.net/
.. _UsbConnector:
    https://www.planet-casio.com/Fr/forums/topic13656-1-usbconnector
    -remplacement-de-fa124-multi-os.html

.. _Casetta: https://casetta.tuxfamily.org/
.. _Caspro:
    https://web.archive.org/web/20160504230033/
    http://www.spiderpixel.co.uk:80/caspro/

.. _Mutant Standard: https://mutant.tech/
.. _Caius Nocturne: https://nocturne.works/
.. _CC BY-NC-SA 4.0 International:
    http://creativecommons.org/licenses/by-nc-sa/4.0/
