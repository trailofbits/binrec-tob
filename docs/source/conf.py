# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).absolute().parents[2]))

try:
    from binrec import lib
except ImportError as err:
    # We require lib when we are building documentation, not when we are only
    # doing a spell check (-b spelling).
    try:
        target_flag = sys.argv.index("-b")
        bail = sys.argv[target_flag:target_flag+2] != ["-b", "spelling"]
    except ValueError:
        bail = True

    if bail:
        raise

    # Mock the _binrec_lift and _binrec_link extensions so that Sphinx is
    # still happy within CI.
    print("Mocking _binrec_lift and _binrec_link extensions")

    import types
    sys.modules["_binrec_lift"] = types.ModuleType("_binrec_lift")
    sys.modules["_binrec_link"] = types.ModuleType("_binrec_link")

    from binrec import lib


# -- Project information -----------------------------------------------------

project = 'BinRec'
copyright = '2022, Trail of Bits'
author = 'Trail of Bits, University of California, Irvine and Vrije Universiteit Amsterdam'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
    "sphinxcontrib.spelling"
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'alabaster'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = []  # ['_static']

intersphinx_mapping = {'python': ('https://docs.python.org/3', None)}
