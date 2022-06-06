Batch Processing
----------------

Multiple traces can be defined in a single JSON file and executed as a batch. The
``binrec.batch`` module provides classes and methods to load batch trace parameters.
When BinRec executes a trace, S2E loads a trace configuration script which exports
symbolic and concrete arguments and input files. This trace configuration script is
generated from the ``TraceParams`` class.

.. automodule:: binrec.batch
    :members:
