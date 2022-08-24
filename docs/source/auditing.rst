Auditing
--------

BinRec uses multiple subprocesses, temporary files, and temporary directories,
which can make troubleshooting difficult. The ``binrec.audit`` module provides
a method to log these, and other events, to the BinRec debug log to aid in
troubleshooting or if an audit trail is needed.

.. autofunction:: binrec.audit.enable_python_audit_log

