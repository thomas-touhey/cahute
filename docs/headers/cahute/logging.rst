``<cahute/logging.h>`` -- Logging control for Cahute
====================================================

Macro definitions
-----------------

.. c:macro:: CAHUTE_LOGLEVEL_INFO

    Constant representing the ``info`` logging level; see :ref:`logging`
    for more information.

.. c:macro:: CAHUTE_LOGLEVEL_WARNING

    Constant representing the ``warning`` logging level; see :ref:`logging`
    for more information.

.. c:macro:: CAHUTE_LOGLEVEL_ERROR

    Constant representing the ``error`` logging level; see :ref:`logging`
    for more information.

.. c:macro:: CAHUTE_LOGLEVEL_FATAL

    Constant representing the ``fatal`` logging level; see :ref:`logging`
    for more information.

.. c:macro:: CAHUTE_LOGLEVEL_NONE

    Constant representing the special ``none`` logging level; see
    :ref:`logging` for more information.

Function declarations
---------------------

.. c:function:: int cahute_get_log_level()

    :return: The current logging level.

.. c:function:: void cahute_set_log_level(int level)

    :param level: The logging level to set as the current one.
