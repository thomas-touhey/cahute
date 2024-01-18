Contributing to Cahute
======================

You have something to contribute to Cahute, whether it's a new feature, a tweak
on an existing protocol, an unhandled file format, some more background on
a feature or problematic, or a fix to a typo in the documentation?

First of all, thank you so much! This guide will help you set up for efficient
contribution, and save some time to all involved.

.. note::

    If you are interested in the rationale of what is indicated here,
    you can consult the :ref:`contribution-style` for more information.

Preparing the repository
------------------------

You must fork Cahute's repository, by:

* Creating an account on `Gitlab.com`_.
* Going on the Cahute repository and selecting "Fork".
* Cloning the fork on your PC, by running a command resembling
  ``git clone https://gitlab.com/cahuteproject/cahute.git``.
* Changing the current branch to a work branch, by e.g.
  running ``git switch -c feat/your-goal-here``.

Installing and setting up the required tools
--------------------------------------------

You will need the following dependencies and tools:

* Build dependencies for the project; see :ref:`build-cahute`.
* Python >= 3.11, with dependencies listed in ``docs/requirements.txt``,
  which you can either install externally, with a virtualenv or directly,
  using ``pip install -r docs/requirements.txt``.
* `pre-commit`_.

In order to set up pre-commit, you need to run the following command::

    pre-commit install

If you are modifying the code, even just when fixing a typo, you need to
build the project from the modified source; see :ref:`build-cahute` for
more information.

Testing your changes
--------------------

If you have only updated the documentation, you can run
``make -C docs preview``, which will run a webserver on
``http://localhost:8000`` to allow you to preview your changes.

If you have updated the code, you must build the project, then run
either ``./p7 <your command args here...>``,
``./p7screen <your command args here...>``, or any other custom
executable you link with the built ``./libcahute.a`` to test it.

.. note::

    It is recommended to describe the tests that you have done in the
    description of the merge request, so that the maintainers can have
    an idea of what you could have missed!

Pushing your changes
--------------------

Once you are done, you must commit and push your changes.

To create your commit, you most certainly want to do something like
``git add .``, then ``git commit -m "your message here"``.
Note that the message must be formatted a certain way:

* If you are adding a feature: ``feat: your message here``, e.g.
  ``feat: add hypertriangulation to protocol delta``.
* If you are fixing a bug in the code: ``fix: your message here``, e.g.
  ``fix: fix bad buffer usage by TYPKZ OHP packets``.
* If you are adding something to or fixing something in the docs:
  ``docs: your message here``, e.g.
  ``docs: add missing section for hypertriangulation flow``.

You can now push to your repository using
``git push -u origin feat/your-goal-here`` (*replace the branch name*).
Gitlab will provide you the link to create the merge request.

.. _Gitlab.com: https://gitlab.com/
.. _pre-commit: https://pre-commit.com/
.. _Sphinx: https://www.sphinx-doc.org/en/master/
