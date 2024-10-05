Contributing to Cahute
======================

Cahute is a project made by and for the community, and as such, contributions
of many kinds are welcome. This guide references some of the most common ways
you can contribute too.

Using Cahute and providing feedback
-----------------------------------

The most important way you can contribute to Cahute is by using it!
The more Cahute is used on diverse systems, with diverse calculators,
cables and files, the more it stabilizes and adapts to a broader and diverse
audience.

As described in :ref:`project-feedback`, the project does not collect
statistics regarding its usage, therefore you are invited to:

* Report the issues you encounter, see :ref:`issue-reporting-guide` to discover
  how;
* Provide feedback and suggest features you are lacking in either the
  `repository issues`_, or one of the topics or instant messaging platforms
  listed in :ref:`project-forums`.

.. note::

    If you have accounts to these platforms, you can also star the
    `Gitlab repository`_ and/or the `Github repository`_.

Testing Cahute in more exotic configurations
--------------------------------------------

If you're particularly enthusiastic about Cahute, you can go out of your way
to test Cahute with more exotic CASIO calculators or systems, e.g. by buying
rare CASIO calculators to interact with or installing systems that are less
tested or not mentioned in the documentation and using Cahute with them.

In this case, please report any bugs you may encounter using the
:ref:`issue-reporting-guide` guide!

Testing the Cahute documentation
--------------------------------

Cahute's documentation aims at being complete and well organized, which
requires a lot of feedback from many people with different point of views
to ensure all of them find what they need.

Documentation bugs are considered the same as other bugs, and as such,
can be the object of issues produced in :ref:`issue-reporting-guide`.
They include, but are not limited to:

* Lack of how-to guides for a provided use case, or set of use cases;
* Lack of documentation regarding a supported data format or protocol;
* Lack of documentation regarding an abstraction or internal;
* Lack of a policy regarding the project management;
* Incorrect or out-of-date reference data, e.g. incorrect default value;
* Bad wording, lack of clarity or cluttering regarding a specific
  topic. Usage of "simply" or equivalent words as described on
  `justsimply.dev`_ fits here;
* Use of non-inclusive or offensive wording or vocabulary, e.g. use of
  masculine pronouns when not dealing with a specific person, misgendering,
  using "master/slave" or other such vocabulary, and so on.

Localizing and advertising Cahute out of its channels
-----------------------------------------------------

Cahute is a young project, and as such, many people who could use it are
not aware of its existence or uses. As such, the project can benefit
from you:

* Talking about Cahute around you;
* Creating topics on your communities' forum, e.g. in other languages than
  English, on how to use Cahute for common use cases to your communities;
* Creating video tutorials or infographics on how to do specific tasks with
  Cahute, or even talking about how Cahute has impacted you;
* And so on, you know best how to spread the word about Cahute around you!

Packaging Cahute for your system
--------------------------------

If you're using a system for which there is no install instructions in
:ref:`guide-install`, you can probably help packaging Cahute for it and
contributing to your distribution in order to make the package official!

You can find generic packaging instructions in :ref:`packaging-guide`, to
provide you with elements that will help you get the gist of Cahute
packaging.

Once you have packaged Cahute for your system and made these installation
instructions official for your system, please provide these instructions
to the project through an `issue <Repository issues_>`_; we can definitely
add a section in the installation guide for it, regardless of how small that
system is!

Contributing code and documentation directly
--------------------------------------------

Finally, there is the more commonly found option to contribute code and
documentation directly, through `merge requests`_. In this case, it is
highly recommended you read the following sections before doing so:

* :ref:`guide-build`;
* :ref:`contribution-style` and :ref:`coding-style`.

.. warning::

    Since the `Github repository`_ is only a mirror, pull requests on it
    will either be rejected or ignored; you need to make a merge request
    on the `Gitlab repository`_ specifically.

.. note::

    Unless you are a maintainer, you will be required to fork the project
    first. This also means you will need to run pipelines on your side,
    since successful pipelines are a prerequisite to merging.

    See `Gitlab CI/CD pipelines`_ for more information.

.. _Github repository: https://github.com/thomas-touhey/cahute
.. _Gitlab repository: https://gitlab.com/cahuteproject/cahute
.. _Repository issues: https://gitlab.com/cahuteproject/cahute/-/issues
.. _Merge requests: https://gitlab.com/cahuteproject/cahute/-/merge_requests
.. _justsimply.dev: https://justsimply.dev/
.. _`Gitlab CI/CD pipelines`: https://docs.gitlab.com/ee/ci/pipelines/
