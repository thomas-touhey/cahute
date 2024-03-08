Building with the Cahute library
================================

.. warning::

    This guide assumes that you have already installed the Cahute library.
    If you have not, see :ref:`guide-install`.

The instructions to build your project using the Cahute library depend on
your build system; please read the correct section for yours!

Using |cmake| CMake
-------------------

If your project is built using CMake, you can add the following to your
``CMakeLists.txt`` file, before defining your targets, to include and start
building with Cahute:

.. code-block:: cmake

    pkg_check_modules(cahute REQUIRED cahute IMPORTED_TARGET)
    link_libraries(PkgConfig::cahute)

Using pkg-config
----------------

Cahute defines the ``cahute`` pkg-config configuration. Therefore:

* You can obtain the compilation flags by running the following command:

  .. code-block:: bash

      pkg-config cahute --cflags

* You can obtain the linking flags by running the following command:

  .. code-block:: bash

      pkg-config cahute --libs

For example, if you want to compile a simple project using Cahute, you can
use the following command:

.. code-block:: bash

    cc main.c -o ./my_util `pkg-config cahute --cflags --libs`

.. |cmake| image:: cmake.svg

.. _CMake: https://cmake.org/
