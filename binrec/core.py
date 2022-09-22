import logging
import os
from typing import Union

#: Type for any acceptable path
AnyPath = Union[str, os.PathLike]

logger = logging.getLogger(__name__)


def init_binrec() -> None:
    """
    Initialize the BinRec module. This module **must** be called prior to calling any
    BinRec function. This method initializes the BinRec logger and environment
    variables.
    """
    # make sure environment variables are setup
    from . import env

    logging.basicConfig(
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
        datefmt="%H:%M:%S",
        level=logging.INFO,
    )

    if env.BINREC_DEBUG:
        enable_binrec_debug_mode()


def enable_binrec_debug_mode() -> None:
    """
    Run binrec with debug logging enabled.
    """
    from . import env

    logging.getLogger("binrec").setLevel(logging.DEBUG)
    os.environ[env.BINREC_DEBUG_ENV] = "1"
