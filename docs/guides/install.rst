.. _guide-install:

Installing Cahute
=================

In order to install Cahute's library and command-line utilities, the
instructions depends on the system you want to install it on.

.. note::

    The guides expect ``p7`` and ``p7screen`` to be directly usable in your
    command-line shell at the end of the installation process, i.e. that:

    * Either both executables are placed in a directory already referenced
      by your ``$PATH``;
    * Or the directory in which both executables are placed is added to
      your ``$PATH``, and your shell has been updated to take the change
      into account.

    If neither of these is the case for your system, you will need to
    tweak the commands to point to the right executable in an absolute or
    relative fashion, e.g. ``/opt/cahute/bin/p7 info``.

|apple| macOS, OS X
-------------------

Cahute and its command-line utilities can be installed using
|homebrew| Homebrew_.

Once Homebrew is installed, **disconnect all calculators from your computer**
and install the `cahute formula <cahute homebrew formula_>`_ with the
following command::

    brew install cahute

.. warning::

    The installation requires that no calculator is currently connected
    to your computer through USB; having one currently in receive mode may
    result in the installation failing.

|archlinux| Archlinux, |manjaro| Manjaro Linux
----------------------------------------------

Cahute and its command-line utilities are present on the
`Archlinux User Repository`_, you can pop up your favourite pacman frontend
and install the `cahute <cahute on AUR_>`_ package:

* Using paru_::

    paru -S cahute

* Using pikaur_::

    pikaur -S cahute

Once installed, it is recommended to add your user to the ``uucp`` group,
for access to serial and USB devices, by running the following command
**as root** then restarting your session::

    usermod -a -G uucp <your-username>

|lephe| GiteaPC
---------------

Cahute and its command-line utilities are installable as a desktop application
through GiteaPC_, by running the following command:

.. parsed-literal::

    giteapc install cake/cahute@\ |version|

.. _build-cahute:

Other systems
-------------

.. note::

    This guide may not be exhaustive, and a package may exist for your
    distribution. Please check with your distribution's package registry
    and/or wiki before proceeding!

If no package exists for your distribution, or you are to package Cahute for
your distribution, you can build the command-line utilities yourself.

See :ref:`guide-build` for more information.

.. _Homebrew: https://brew.sh/
.. _cahute homebrew formula: https://formulae.brew.sh/formula/cahute
.. _Archlinux User Repository: https://aur.archlinux.org/
.. _cahute on AUR: https://aur.archlinux.org/packages/cahute
.. _p7 on AUR: https://aur.archlinux.org/packages/p7
.. _p7screen on AUR: https://aur.archlinux.org/packages/p7screen
.. _paru: https://github.com/morganamilo/paru
.. _pikaur: https://github.com/actionless/pikaur
.. _GiteaPC: https://git.planet-casio.com/Lephenixnoir/giteapc

.. |apple| image:: apple.svg
.. |homebrew| image:: homebrew.svg
.. |archlinux| image:: arch.svg
.. |manjaro| image:: manjaro.svg
.. |lephe| image:: lephe.png
