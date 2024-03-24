Git and release versioning
==========================

This document covers how the Git repository is organized, as well as how
release numbers are organized.

Releases
--------

Cahute's main source package distribution, also known as "tarballs", is
on `ftp.cahuteproject.org/releases
<https://ftp.cahuteproject.org/releases>`_.

Cahute uses `Semantic Versioning` with only major and minor versions, not
patches. "Breaking changes" cover both the library and utilities, i.e.:

* If a valid usage of the library becomes invalid or produces an unexpected
  result (e.g. requests more privileges), breaking changes have occurred.
* If a valid usage of any of the command-line utilities become invalid or
  produce an unexpected result, breaking changes have occurred.

Note however that **0.x versions don't require backwards compatible changes
between minor versions**, only 1.0 and above, due to API incremental design
and stabilization concerns.

.. _release-process:

Release process
~~~~~~~~~~~~~~~

Only the BDFL can release a new version, by pushing a new commit
``feat: release version x.y`` on ``develop`` that:

* Updates the project version in the ``project()`` function call in
  ``CMakeLists.txt``.
* Updates the project version with the ``version`` attribute of the
  ``docs/conf.py`` file.

The BDFL then pushes a tag containing the version name, e.g. ``x.y`` in this
case, and the CI, on detecting a new tag:

* Creates a tarball of the Git repository, and pushes it to the
  tarball distribution point with an associated SHA256 checksum file.
* Builds the documentation and updates the main website using the newly
  generated files.

Once this is done, the BDFL creates a new release on the Releases_ tab of
the Gitlab repository, linked to the newly created tag, with a human-readable
changelog.

.. note::

    Creating a new entry in Releases_ will notify package maintainers and
    other manual users.

Git repository structure
------------------------

Cahute's main repository is at `gitlab.com/cahuteproject/cahute
<https://gitlab.com/cahuteproject/cahute>`_.
**Only maintainers are allowed to push to it**; other contributors must work
on a fork, then submit the remote branch as a merge request.

The main branch is ``develop``, which may have breaking changes at any point.
At every release, a tag is created, pointing on the commit that has
incremented the version; see :ref:`release-process` for more information.

If contributions, under the form of merge requests, are accepted, they are
to be merged using fast-forward with no merge commit.

Maintainers are allowed to push their work on the repository under branches
prefixed by one of the following:

* ``fix/``: If a fix is required for an issue, e.g.
  ``fix/fix-bad-check-packet-type-on-special-mode``.
* ``feat/``: For adding features such as adding a protocol, a file format,
  an option, and so on.
* ``refactor/``: For refactoring part of the code or documentation.

Git commit messages must be named using the commit naming convention;
see :ref:`project-commit-naming`.

.. note::

    This structure, while simple, gives flexibility for release branches,
    hotfix branches, and so on, to be introduced later.

.. _Semantic Versioning: https://semver.org/
.. _Releases: https://gitlab.com/cahuteproject/cahute/-/releases
