import logging
import sys
from typing import Iterable

logger = logging.getLogger(__name__)


def enable_python_audit_log(
    event_prefixes: Iterable[str] = ("shutil.", "os.", "subprocess.")
) -> None:
    """
    Turn on Python event auditing to the binrec logger. This adds a new audit hook,
    :meth:`sys.addaudithook`, that logs events. By default, only events from the
    :mod:`shutil`, :mod:`os`, and :mod:`subprocess` modules are logged, which can be
    changed by passing in a custom list of event prefix strings to filter on. Filtering
    is done by checking if the audit event starts with any of the prefix.

    .. code-block:: python

        if event.startswith(event_prefixes):
            # log the event

    :param event_prefixes: list of event prefixes to filter on.
    """
    prefixes = tuple(event_prefixes)

    def binrec_audit_event(event: str, args: tuple) -> None:
        if event.startswith(prefixes):
            logger.debug("%s: %s", event, args)

    sys.addaudithook(binrec_audit_event)
