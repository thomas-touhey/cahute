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

    cmake -B build -S cahute-|version| -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build

Once this is done, you can produce the distribution directory ``dist/``
using the following command::

    DESTDIR=./dist cmake --install build

Your distribution directory is ready; from there, the instructions are
specific to your distribution.

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
.. _GNU Make: https://www.gnu.org/software/make/
.. _pkg-config: https://git.sr.ht/~kaniini/pkgconf
.. _SDL: https://www.libsdl.org/
.. _libusb: https://libusb.info/
.. _Releases: https://gitlab.com/cahuteproject/cahute/-/releases
.. _Get notified when a release is created:
    https://docs.gitlab.com/ee/user/project/releases/
    #get-notified-when-a-release-is-created
.. _Notifications: https://gitlab.com/-/profile/notifications
