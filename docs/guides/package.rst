Packaging Cahute
================

Cahute is constituted of a static library, a set of utilities, and a
set of udev rules. Due to the last element having to be installed for both,
only the following package organisations are available:

* Having one package for both the static library and utilities.
* Having one package for the static library, either a package for all utilities
  or a package for each utility, and a package for installing the udev rules,
  of which all previous package depend.

For more about the rationale behind this document, consult the
`Archlinux CMake packaging guidelines`_.

Preparing the installer or distribution
---------------------------------------

The build dependencies and method for Cahute, as well as the way to obtain
a distribution directory or installer, are described in detail
in :ref:`guide-build`. It is recommended to use the tarballs rather than
the git repository, as they run on a simpler and less prone to fail service.

If need be, you can take inspiration from the `cahute PKGBUILD for Archlinux`_
or the `Cahute formula for Homebrew`_.

.. warning::

    If your packaging system provides stripping out of the box, it is
    recommended to disable it, as it may strip the symbols out of the
    static library, rendering it unusable.

    For example, in the ``PKGBUILD`` file for Archlinux_ and
    derivatives, the ``strip`` option must be explicitely disabled,
    i.e. the PKGBUILD should have the following line::

        options=(!strip)

.. note::

    For Linux distributions, Cahute provides udev rules by default, which make
    the following assumptions:

    * Your distribution uses udev_;
    * Your distribution provides a ``uucp`` group, as per
      `Linux Standard Base Core Specification section 23.2`_;
    * Your distribution uses said group as the owner for serial devices
      (``/dev/ttyS*``, ``/dev/ttyUSB*``).

    If any of these assumptions is incorrect in the case of your distribution,
    it is recommended to:

    * Disable the udev rules installation, by adding the following to the
      initial command:

      .. parsed-literal::

          cmake -B build -S cahute-|version| ... -DENABLE_UDEV=OFF

    * Take the appropriate measures to simplify user access to the devices.

    Otherwise, if all of the assumptions are correct, it is recommended to:

    * `Reload the udev rules`_ in post-installation, by running the
      following command::

          udevadm control --reload

    * Prompt to the user to add their username to the ``uucp`` group,
      e.g. using the ``usermod -a -G uucp <username>`` command.

Testing your package
--------------------

As usually recommended for packagers, it is recommended to test your package
or packages by installing them on a fresh, minimal system, and ensure that
they installs the correct runtime repositories.

A few tests that you can do regarding the command-line utilities are the
following:

* Ensure that both ``p7 --help`` and ``p7screen --help`` both display help
  messages. This can help identifying issues regarding missing shared
  libraries;
* If your system supports USB, connect an fx-9860G calculator or compatible,
  place it in reception mode, run ``p7 info`` and ensure that it displays the
  correct information regarding your calculator. This ensures that:

  - USB detection works correctly;
  - User has access to the USB calculator;
  - USB endpoints are reported correctly and are usable to run a full
    communication.
* If your system supports serial or USB-to-serial devices, and you have both
  an fx-9860G calculator or compatible and the appropriate cable:

  - Connect your serial or USB-to-serial cable to both the computer running
    the system you're testing, and your calculator.
  - Place your calculator in receive mode over 3-pin specifically.
  - Run ``p7 list-devices`` and ensure that it displays the serial adapter.
  - Run ``p7 info --com <serial_device>``, where ``<serial_device>`` is
    the name of the serial device reported above. This ensures that serial
    communications work correctly.

A few tests that you can do regarding the library are the following:

* Build and run the guides you have the hardware and system support for,
  e.g. :ref:`guide-developer-detect-usb`. See :ref:`developer-guide-build`
  for more information. This can help identify issues with overzealous
  binary stripping, or missing dependencies using ``pkg-config``.
* If you have nm_ or equivalent, run ``nm -CgU <path/to/libcahute.a>`` to
  ensure that all exported symbols for your system start with ``cahute_``.

Getting informed when a new version is released
-----------------------------------------------

Maintaining a package is a long-term process, and you need to stay informed
when Cahute is updated so that you can update your package. Fortunately, there
is a way to do just this!

As described in :ref:`release-process`, when a new release is created, a new
entry is created on the Releases_ page of the Gitlab repository.
You can subscribe to such events by creating a Gitlab.com account, and
following the steps in `Get notified when a release is created`_.

.. note::

    You can check your notification settings at any time in Notifications_.

.. _Archlinux CMake packaging guidelines:
    https://wiki.archlinux.org/title/CMake_package_guidelines
.. _Cahute PKGBUILD for Archlinux:
    https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=cahute
.. _Cahute formula for Homebrew:
    https://github.com/Homebrew/homebrew-core/blob/master/Formula/c/cahute.rb
.. _Archlinux: https://archlinux.org/
.. _udev: https://wiki.archlinux.org/title/Udev
.. _reload the udev rules:
    https://wiki.archlinux.org/title/Udev#Loading_new_rules
.. _Linux Standard Base Core Specification section 23.2:
    https://refspecs.linuxfoundation.org/LSB_5.0.0/LSB-Core-generic/
    LSB-Core-generic/usernames.html

.. _nm: https://sourceware.org/binutils/docs/binutils/nm.html

.. _Releases: https://gitlab.com/cahuteproject/cahute/-/releases
.. _Get notified when a release is created:
    https://docs.gitlab.com/ee/user/project/releases/
    #get-notified-when-a-release-is-created
.. _Notifications: https://gitlab.com/-/profile/notifications
