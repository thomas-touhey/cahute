# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS ?=
SPHINXBUILD ?= sphinx-build
SPHINXAUTOOPTS ?= --ignore "*.kate-swp" --ignore "**/openapi.json" --watch ../teapots_identity
SPHINXAUTOBUILD ?= sphinx-autobuild
SOURCEDIR = .
BUILDDIR = _build

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

install:
	pip install sphinx sphinxcontrib-mermaid furo

preview:
	@$(SPHINXAUTOBUILD) "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXAUTOOPTS) $(O)

.PHONY: help preview Makefile

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
