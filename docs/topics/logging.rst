.. _logging:

Logging facilities with Cahute
==============================

Cahute has logging facilities for its internal needs, which functions with
logging levels.

Every log is accompanied with a level, among the following:

* ``info`` (:c:macro:`CAHUTE_LOGLEVEL_INFO`): debug messages, e.g. packet
  contents or activated behaviour tweaks that have been encountered.
* ``warning`` (:c:macro:`CAHUTE_LOGLEVEL_WARNING`): messages signifying
  abnormal behaviours that are recoverable and recovered by the library.
* ``error`` (:c:macro:`CAHUTE_LOGLEVEL_ERROR`): messages signifying abnormal
  behaviours that are not recoverable, and may cause link termination, or
  termination of a specific flow.
* ``fatal`` (:c:macro:`CAHUTE_LOGLEVEL_FATAL`): messages signifying program
  termination, e.g. due to catastrophic system-related incidents.

Logging messages are printed on standard error if their level is allowed to
be printed.

.. note::

    There is no way currently to reroute these logs to another output than
    standard error. If the need arises, this may change.

Whether messages for a given logging level are printed or not can be
controlled from the outside, by setting the current "logging level".
Messages for a logging level can be printed if they are at the same or of
higher importance than the currently configured logging level for the
library, i.e. if they are on the same line or below in the list presented
above.

By that rationale:

* If the currently configured logging level is ``info``, then messages will
  be displayed for all logging levels, since ``info`` has the lowest
  importance.
* If the currently configured logging level is ``warning``, then messages
  will be displayed for the ``warning``, ``error`` and ``fatal`` logging
  level only.
* If the currently configured logging level is ``error``, then messages
  will be displayed for the ``error`` and ``fatal`` logging levels only.
* If the currently configured logging level is ``fatal``, then messages
  will be displayed for the ``fatal`` logging level only.

There is a special logging level ``none`` (:c:macro:`CAHUTE_LOGLEVEL_NONE`)
that, if configured, will not let any log be printed.

As a library user, the current logging level can be gathered using
:c:func:`cahute_get_log_level`, and can be set using
:c:func:`cahute_set_log_level`.

As a command-line user, some command-line utilities support an ``-l`` and/or
``--log`` option to set the current logging level for the library.
