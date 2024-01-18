.. _contribution-style:

Contribution style
==================

TL;DR:

* Contributors, especially external contributors, must work on a fork, and
  submit their work via a merge request.
* Contributors can only target ``develop``, except maintainers that can
  target other release branches to backport fixes if need be.
* Contributors must use `pre-commit`_ for autoformatting and linting their
  contribution.
* Contributors must write documentation for their modifications on the
  library.

.. _project-commit-naming:

Commit naming
-------------

Cahute uses `Conventional Commits`_, as enforced using Commitizen_ installed
as a `pre-commit`_ hook.

Your commit messages must be in **imperative** form, i.e. ``do something``.

.. warning::

    Please do **NOT** use past form, e.g. ``done something``.

.. note::

    Scopes in commit messages aren't common in Cahute, but they can be
    accepted if they are relevant.

Coding style
------------

The code included within the contribution must respect Cahute's coding
style; see :ref:`coding-style` for more information.

Documentation
-------------

Every modification on library code must be repercuted on the documentation.
Think of it that way:

* If it is wrong in the code, it is most certainly wrong in the documentation.
* If it is not present in the documentation, then it is missing, and must be
  added into it.
* If it is added to the code, it must be added to the documentation.

Writing documentation is hard, and Diátaxis_' logic has to be learned in
order to be applied efficiently. However, even bad documentation is
documentation, and Cahute needs it to be sustainable, especially since
Cahute implements reverse engineered protocols and file formats for which
no official documentation exists!

.. _pre-commit: https://pre-commit.com/
.. _Commitizen: https://commitizen-tools.github.io/commitizen/
.. _Conventional Commits: https://www.conventionalcommits.org/en/v1.0.0/
.. _Diátaxis: https://diataxis.fr/
