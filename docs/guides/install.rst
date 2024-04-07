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

Once Homebrew is installed, you can install the `cahute formula
<cahute homebrew formula_>`_::

    brew install cahute

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

|linux| Other Linux distributions
---------------------------------

.. note::

    This guide may not be exhaustive, and a package may exist for your
    distribution. Please check with your distribution's package registry
    and/or wiki before proceeding!

If no package exists for your distribution, or you are to package Cahute for
your distribution, you can build the command-line utilities yourself.

First, you need to install the build and runtime dependencies for Cahute:

* cmake_ >= 3.16;
* `GNU Make`_, `pkg-config`_, and other C compilation and linking utilities;
* SDL_ >= 2.0 (for ``p7screen``);
* libusb_.

For getting the source, you have the following options:

* You can download the latest source package at
  https://ftp.cahuteproject.org/releases\ :

  .. parsed-literal::

      curl -o cahute-|version|.tar.gz https\://ftp.cahuteproject.org/releases/cahute-|version|.tar.gz
      tar xvaf cahute-|version|.tar.gz

* You can clone the repository and checkout the tag corresponding to the
  release:

  .. parsed-literal::

      git clone https\://gitlab.com/cahuteproject/cahute.git cahute-|version|
      (cd cahute-|version| && git checkout -f |version|)

The project is present in the "cahute-|version|" directory.
In the parent directory, we are to create the ``build`` directory aside
it, and install from it, by running the following commands:

.. parsed-literal::

    cmake -B build -S cahute-|version| -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build
    sudo cmake --install build

.. warning::

    For communicating with calculators over USB and serial, Cahute library
    and command-line utilities require access to such devices.

    For serial devices, this is traditionally represented by being a member
    of the ``uucp`` group, defined as the group owner on ``/dev/ttyS*``
    devices; you can check this by running ``ls -l /dev/ttyS*``.
    However, by default, USB devices don't have such rules.

    CMake automatically installs the udev rules, which means you need to
    do the following:

    * Reload the udev daemon reload to apply the newly installed rules
      on the running system without a reboot, with this command **as root**::

          udevadm control --reload

    * Adding your user to the ``uucp`` group, then restarting your session::

          usermod -a -G uucp <your-username>

That's it! You should be able to run the following command::

    p7 --version

.. note::

    Since you are not using a packaged version of Cahute, the project won't
    be automatically updated when updating the rest of the system, which
    means you need to do it manually, especially if a security update is
    made.

    You can subscribe to releases by creating a Gitlab.com account, and
    following the steps in `Get notified when a release is created`_.
    You can check your notification settings at any time in Notifications_.

.. _Homebrew: https://brew.sh/
.. _cahute homebrew formula: https://formulae.brew.sh/formula/cahute
.. _Archlinux User Repository: https://aur.archlinux.org/
.. _cahute on AUR: https://aur.archlinux.org/packages/cahute
.. _p7 on AUR: https://aur.archlinux.org/packages/p7
.. _p7screen on AUR: https://aur.archlinux.org/packages/p7screen
.. _paru: https://github.com/morganamilo/paru
.. _pikaur: https://github.com/actionless/pikaur
.. _GiteaPC: https://git.planet-casio.com/Lephenixnoir/giteapc

.. _cmake: https://cmake.org/
.. _GNU Make: https://www.gnu.org/software/make/
.. _pkg-config: https://git.sr.ht/~kaniini/pkgconf
.. _SDL: https://www.libsdl.org/
.. _libusb: https://libusb.info/

.. _Get notified when a release is created:
    https://docs.gitlab.com/ee/user/project/releases/
    #get-notified-when-a-release-is-created
.. _Notifications: https://gitlab.com/-/profile/notifications

.. |apple| image:: apple.svg
.. |homebrew| image:: homebrew.svg
.. |archlinux| image:: arch.svg
.. |manjaro| image:: manjaro.svg
.. |lephe| image:: lephe.png
.. |linux| image:: linux.svg
