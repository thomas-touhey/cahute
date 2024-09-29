# *****************************************************************************
# Copyright (C) 2024 Thomas Touhey <thomas@touhey.fr>
#
# This software is governed by the CeCILL 2.1 license under French law and
# abiding by the rules of distribution of free software. You can use, modify
# and/or redistribute the software under the terms of the CeCILL 2.1 license
# as circulated by CEA, CNRS and INRIA at the following
# URL: https://cecill.info
#
# As a counterpart to the access to the source code and rights to copy, modify
# and redistribute granted by the license, users are provided only with a
# limited warranty and the software's author, the holder of the economic
# rights, and the successive licensors have only limited liability.
#
# In this respect, the user's attention is drawn to the risks associated with
# loading, using, modifying and/or developing or reproducing the software by
# the user in light of its specific status of free software, that may mean
# that it is complicated to manipulate, and that also therefore means that it
# is reserved for developers and experienced professionals having in-depth
# computer knowledge. Users are therefore encouraged to load and test the
# software's suitability as regards their requirements in conditions enabling
# the security of their systems and/or data to be ensured and, more generally,
# to use and operate it in the same conditions as regards security.
#
# The fact that you are presently reading this means that you have had
# knowledge of the CeCILL 2.1 license and that you accept its terms.
# *****************************************************************************
"""Sphinx plugin to modify the HTML rendering."""

from __future__ import annotations

from typing import TYPE_CHECKING

from docutils import nodes
from sphinx.writers.html import HTMLTranslator

if TYPE_CHECKING:
    from sphinx.application import Sphinx


class MyHTMLTranslator(HTMLTranslator):
    """Custom"""

    def visit_title(self, node):
        is_title = (
            not isinstance(
                node.parent,
                (
                    nodes.topic,
                    nodes.sidebar,
                    nodes.Admonition,
                    nodes.table,
                    nodes.document,
                ),
            )
            and self.section_level == 1
        )
        self.context.append(is_title)

        if is_title:
            self.body.append(self.starttag(node, "div", "", CLASS="title-container"))
            self.body.append(self.starttag(node, "div", "", CLASS="title-contrast"))
            self.body.append(self.starttag(node, "hr", ""))
            self.body.append("</hr></div>")

        super().visit_title(node)

    def depart_title(self, node):
        super().depart_title(node)

        if self.context.pop():
            self.body.append("</div>")


def setup(app: Sphinx, /) -> None:
    """Set up the extension.

    :param app: The Sphinx application to set up the extension for.
    """
    app.set_translator("html", MyHTMLTranslator)
