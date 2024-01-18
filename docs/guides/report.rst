.. _issue-reporting-guide:

Reporting a vulnerability or issue
==================================

If you have identified that Cahute or any of its utilities does not work
properly, or would like to suggest a feature or evolution, there are
different procedures depending on the issue.

.. warning::

    Once your issue is up or sent, **please check on it every few days at
    least, or leave a way for the maintainers to contact you without
    giving up their privacy** (i.e. no phone numbers, social network
    profile or instant messaging address); an e-mail address is fine.

    An issue reported by someone who can't answer once additional details
    are required from them is an issue that gets closed and has wasted
    everyone's time and efforts.

.. warning::

    For any type of issue, due to the fact that Cahute is free software
    maintained by people on their free time, there is no guarantee of any
    delay, or even of a response or that the issue won't be closed due to
    lack of availability on the maintainers' part.

    Note however that this warning is worst case scenario, and hopefully,
    it won't come to that for any correctly reported issue.

Reporting a vulnerability
-------------------------

If your issue has security implications, e.g. if it allows a malicious
device to access the host and/or execute arbitrary code without authorisation,
please send an e-mail to Thomas Touhey, the maintainer of Cahute,
at <security@cahuteproject.org>.

.. note::

    Please only use this e-mail address if there is security implications
    to your demand. If you are not sure if your issue qualifies or not,
    send it anyway; use your best judgment.

.. _report-other-issues:

Reporting any other issue or evolution request
----------------------------------------------

For all other issues, you can create an issue on the `issue tracker at
Gitlab`_.

If you are effectively reporting a bug in your issue, please include the
following elements:

* System and version (e.g. Debian 14, Ubuntu 22.04 LTS, Windows 11 Pro, ...).
* Architecture (x64, ARM, ...).
* Cahute version (e.g. 0.1, 1.2.1, ...).
* If the issue regards a communication protocol: the calculator model and
  OS version, e.g. Graph 90+E with OS 03.60.2202.
* **Steps to reproduce the issue**, on both the host and any device that
  was implicated in the operation. (The order is important!)

.. _Issue tracker at Gitlab: https://gitlab.com/cahuteproject/cahute/-/issues
