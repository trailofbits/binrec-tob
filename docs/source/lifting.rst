BinRec Lifting
--------------

Lifting is the process of taking a captured trace, typically
:doc:`merged <merging>` from multiple captures/traces,
lifting the captured LLVM bitcode to LLVM IR, compiling,
and linking the recovered bitcode to a binary. Lifting is performed by the
``binrec.lift`` module, which can be used programmatically or
as a standalone script. For example,

**Programmatically**

.. code-block:: python

    >>> from binrec.lift import lift_trace
    >>> from binrec.core import init_binrec
    >>> # Initialize the binrec library
    >>> init_binrec()
    >>> # Lift a trace for the "hello" binary
    >>> lift_trace("hello", OptimizationLevel.NORMAL, False)


**Standlone Script**

.. code-block:: bash

    $ # Lift the trace for the "hello" binary
    $ python -m binrec.lift hello


binrec.lift Module
^^^^^^^^^^^^^^^^^^

.. automodule:: binrec.lift
    :members:
