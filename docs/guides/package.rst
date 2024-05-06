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

Preparing the dependencies
--------------------------

Cahute depends on the following, build-only dependencies:

* cmake_ >= 3.16;
* Python_ >= 3.8;
* `toml module for Python <python-toml_>`_, either installed through pip
  or as a native package such as ``python-toml`` or ``python3-toml``;
* `GNU Make`_, `pkg-config`_, and other C compilation and linking utilities.

It also depends on the following build and runtime dependencies:

* libusb_;
* SDL_ >= 2.0 (for ``p7screen``).

Producing the distribution directory
------------------------------------

You first need to retrieve the source directory, named "cahute-|version|",
using one of the following methods:

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

Now that you have the source directory, you can build it using the following
commands:

.. parsed-literal::

    cmake -B build -S cahute-|version| -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
    cmake --build build

Once this is done, you can produce the distribution directory ``dist/``
using the following command::

    DESTDIR=./dist cmake --install build --strip

Your distribution directory is ready; from there, the instructions are
specific to your distribution.

.. warning::

    If your packaging system provides stripping out of the box, it is
    recommended to disable it, as it may strip the symbols out of the
    static library, rendering it unusable.

    For example, in the ``PKGBUILD`` file for Archlinux_ and
    derivatives, the ``strip`` option must be explicitely disabled,
    i.e. the PKGBUILD should have the following line::

        options=(!strip)

.. note::

    Cahute provides udev rules by default, which make the following
    assumptions:

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
.. _cmake: https://cmake.org/
.. _Python: https://www.python.org/
.. _python-toml: https://pypi.org/project/toml/
.. _GNU Make: https://www.gnu.org/software/make/
.. _pkg-config: https://git.sr.ht/~kaniini/pkgconf
.. _SDL: https://www.libsdl.org/
.. _libusb: https://libusb.info/
.. _Archlinux: https://archlinux.org/
.. _udev: https://wiki.archlinux.org/title/Udev
.. _reload the udev rules:
    https://wiki.archlinux.org/title/Udev#Loading_new_rules
.. _Linux Standard Base Core Specification section 23.2:
    https://refspecs.linuxfoundation.org/LSB_5.0.0/LSB-Core-generic/
    LSB-Core-generic/usernames.html
.. _Releases: https://gitlab.com/cahuteproject/cahute/-/releases
.. _Get notified when a release is created:
    https://docs.gitlab.com/ee/user/project/releases/
    #get-notified-when-a-release-is-created
.. _Notifications: https://gitlab.com/-/profile/notifications
