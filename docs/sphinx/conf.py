# Configuration file for the Sphinx documentation builder.
#
# For all the built-in options, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

import os
import sys

sys.path.insert(0, os.path.abspath("."))

# -- Project information -----------------------------------------------------

project = "rampg"
copyright = "2026, rampg Contributors"
author = "rampg Contributors"

release = "0.1.0"

# -- General configuration ---------------------------------------------------

extensions = [
    "breathe",
    "sphinx.ext.mathjax",
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
]

templates_path = ["_templates"]

exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

source_suffix = ".rst"

master_doc = "index"

language = "en"

# -- Options for HTML output -------------------------------------------------

html_theme = "alabaster"
html_theme_options = {
    "description": "A lightweight linear ramp generator for embedded control loops",
}

html_static_path = ["_static"]

# -- Options for Breathe -----------------------------------------------------

breathe_default_project = "rampg"
breathe_default_domain = "c"

# -- Options for intersphinx -------------------------------------------------

intersphinx_mapping = {
    "python": ("https://docs.python.org/3", None),
}
