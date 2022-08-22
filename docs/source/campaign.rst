Campaigns
---------

Multiple traces can be defined in a single JSON file and executed as a batch. The
``binrec.campaign`` module provides classes and methods to load Campaigns which define
multiple S2E analysis traces. When BinRec executes a trace, S2E loads a trace
configuration script which exports symbolic and concrete arguments and input files. This
trace configuration script is generated from the ``TraceParams`` class.

.. automodule:: binrec.campaign
    :members:
