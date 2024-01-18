"""Configuration file for the Sphinx documentation builder.

For the full list of built-in configuration values, see the documentation:
https://www.sphinx-doc.org/en/master/usage/configuration.html
"""

from __future__ import annotations

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).parent / "_ext"))

project = "Cahute"
version = "0.1"
copyright = "2024, Thomas Touhey"
author = "Thomas Touhey"

extensions = [
    "sphinx.ext.intersphinx",
    "sphinx.ext.todo",
    "sphinxcontrib.mermaid",
    "add_seven_command_directive",
]

templates_path: list[str] = []
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]
primary_domain = "c"

html_theme = "furo"
html_theme_options = {
    "footer_icons": [
        {
            "name": "Planète Casio",
            "url": "https://www.planet-casio.com/Fr/",
            "html": (Path("_static") / "planete_casio.svg").open().read(),
            "class": ""
        },
        {
            "name": "Made by a Human",
            "url": "/project.html",
            "html": (Path("_static") / "made_by_a_human.svg").open().read(),
            "class": "",
        },
    ],
}
html_static_path = ["_static"]
html_title = f"Cahute {version}"
html_favicon = '_static/favicon.png'
html_logo = '_static/cahute.svg'
html_use_index = False
html_copy_source = False
html_show_sourcelink = False
html_domain_indices = False
html_css_files = ["custom.css"]

intersphinx_mapping = {}

todo_include_todos = True

mermaid_output_format = "raw"
mermaid_init_js = """
function isDarkMode() {
    const color = (
        getComputedStyle(document.body)
        .getPropertyValue("--color-foreground-primary")
    );

    if (color == "#ffffffcc")
        return true;

    return false;
}

function initializeMermaid(isStart) {
    mermaid.initialize({
        startOnLoad: isStart,
        theme: isDarkMode() ? "dark" : "base",
        darkMode: isDarkMode(),
        securityLevel: "antiscript"
    });
}

const observer = new MutationObserver(function(mutations) {
    mutations.forEach(function(mutation) {
        if (
            mutation.type != "attributes"
            || mutation.attributeName != "data-theme"
        )
            return

        const nodes = document.querySelectorAll(".mermaid");
        nodes.forEach(node => {
            /* Restore the original code before reprocessing. */
            node.innerHTML = node.getAttribute("data-original-code");

            /* Remove the attribute saying data is processed; it is not! */
            if (node.hasAttribute("data-processed"))
                node.removeAttribute("data-processed");
        });

        initializeMermaid(false);
        mermaid.run({nodes: nodes, querySelector: ".mermaid"});
    });
});

(function (window) {
    /* Store original code for diagrams into an attribute directly, since
       Mermaid actually completely replaces the content and removes the
       original code. */
    document.querySelectorAll(".mermaid").forEach(node => {
        node.setAttribute("data-original-code", node.innerHTML);
    })

    initializeMermaid(true);
    observer.observe(document.body, {attributes: true});
})(window);
"""
