BinRec C Libraries
------------------

.. module:: binrec.lib

There are multiple BinRec C modules that perform the actual lifting and merging. These
modules are wrapped in the ``binrec.lib`` module, which imports them and provides
access to the public API. For example, the ``_binrec_link`` C module exports a ``link``
method that is exposed in ``binrec.lib`` as ``binrec_link.link``.

The ``binrec.lib`` module is used as wrapper, instead of accessing the C modules
directly, because each Binrec tool will have a corresponding C module and there may be
necessary setup in Python for the module to work properly. The ``binrec.lib`` updates
the :data:`sys.path` so that these modules can be imported.

``binrec_link`` Module
^^^^^^^^^^^^^^^^^^^^^^^

Code level documentation for the underlying C++ module can be found here: `binrec_link <../../../build/binrec_link/html/index.html>`_.

.. autofunction:: binrec.lib.binrec_link.link


``binrec_lift`` Module
^^^^^^^^^^^^^^^^^^^^^^^

Code level documentation for the underlying C++ module can be found here `binrec_lift <../../../build/binrec_lift/html/index.html>`_.

.. autofunction:: binrec.lib.binrec_lift.link_prep_1

.. autofunction:: binrec.lib.binrec_lift.link_prep_2

.. autofunction:: binrec.lib.binrec_lift.clean

.. autofunction:: binrec.lib.binrec_lift.lift

.. autofunction:: binrec.lib.binrec_lift.optimize

.. autofunction:: binrec.lib.binrec_lift.optimize_better

.. autofunction:: binrec.lib.binrec_lift.compile_prep
