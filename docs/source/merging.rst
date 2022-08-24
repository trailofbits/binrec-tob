BinRec Merging
--------------

Merging is the process of combining multiple captures and traces of a binary,
that exercise different functionality and branches, into a single trace.
Merging is performed by the ``binrec.merge`` module, which can be used
programmatically or as a standalone script. For example,

**Programmatically**

.. code-block:: python

    >>> from binrec.merge import recursive_merge_traces, merge_trace_captures
    >>> from binrec.core import init_binrec
    >>> # Initialize the binrec library
    >>> init_binrec()
    >>> # Merge captures within a trace
    >>> merge_trace_captures("hello-1")
    >>> # Recursively merge all captures and traces for a binary
    >>> recursive_merge_traces("hello")


**Standlone Script**

.. code-block:: bash

    $ # Merge captures within a trace
    $ python -m binrec.merge --trace-id hello-1

    $ # Recursively merge all captures and traces for a binary
    $ python -m binrec.merge --binary-name hello


binrec.merge Module
^^^^^^^^^^^^^^^^^^^

.. automodule:: binrec.merge
    :members:

