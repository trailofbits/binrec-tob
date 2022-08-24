BinRec Library Function Signatures
----------------------------------

During :doc:`lifting <lifting>`, BinRec detects calls into library functions
and initially replaces the direct calls with an indirect call using a
trampoline function
(see the ``helper_stub_trampoline`` function in ``custom-helpers.cpp``).
If the function signature is known, then BinRec is able to eventually replace
the indirect trampoline call with a direct call into the library function.
Function signatures are stored within a database file
with the following format::

    func_name returns_float? return_size arg1_size ... argN_size
    # for example:
    atof 1 8 4
    read 0 4 4 4 4

The function signatures are an optimization and BinRec supports calling library
functions if the signature is unknown. Therefore, at this time, BinRec is
optimized for functions within the libc library.

The ``binrec.sigs`` module, and supporting ``binrec._gdb_sigs`` module, uses a
combination of GDB, library debug symbols, and manpages on the qemu analysis
VM to determine the exported functions and their signatures from any given
library. The process is a multi-pass approach:

1. Extract all exported functions from the library.
2. Use GDB and debug symbols to parse the function signature.
3. If the function signature is not available, typically because
   the function is a weak export or an indirect function (ELF ifunc),
   find and parse the function's manpage for the signature.
4. For every unique type, use GDB to determine its storage size, in bytes.

BinRec is only concerned with the storage size for the function return
and arguments so all pointers are treated as ``void*``, which includes
function pointers and array arguments. Internally, the ``binrec.sigs``
module creates a GDB subprocess that loads the ``binrec._gdb_sigs`` module,
which actually does a majority of the work to generate the database.

The ``binrec.sigs`` module can be used programmatically or as a standalone
script to generate the function signature database. For example,

**Programmatically**

.. code-block:: python

    >>> from binrec.sigs import generate_library_signature_database
    >>> from binrec.core import init_binrec
    >>> from pathlib import Path
    >>> # Initialize the binrec library
    >>> init_binrec()
    >>> # Generate the library signature database
    >>> generate_library_signature_database(Path("libc.so.6"), Path("libc-argsizes"))


**Standlone Script**

.. code-block:: bash

    $ # Generate the library signature database
    $ python -m binrec.sigs libc.so.6 libc-argsizes


binrec.sigs Module
^^^^^^^^^^^^^^^^^^

.. automodule:: binrec.sigs
    :members:
