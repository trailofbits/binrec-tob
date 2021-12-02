import os
from pathlib import Path

from dotenv import load_dotenv

from .errors import BinRecError

__all__ = (
    "BINREC_ROOT",
    "BINREC_BIN",
    "BINREC_RUNLIB",
    "BINREC_SCRIPTS",
    "BINREC_LINK_LD",
    "BINREC_LIB",
)


def _load_env() -> None:
    """
    Load binrec environment variables from the ``.env`` file. This method raises an
    error if the ``BINREC_ROOT`` and ``S2EDIR`` environment variables are not set by
    the ``.env`` file.

    This method is called automatically on first import.
    """
    load_dotenv(".env", override=True)

    if not os.environ.get("BINREC_ROOT") or not os.environ.get("S2EDIR"):
        raise BinRecError(".env file must specify both BINREC_ROOT and S2EDIR")


_load_env()

#: The absolute path to the BinRec repository root directory
BINREC_ROOT = Path(os.environ["BINREC_ROOT"]).absolute()
#: The absolute path to the BinRec build binary directory
BINREC_BIN = Path(os.environ["BINREC_BIN"]).absolute()
#: The absolute path to the BinRec runlib directory
BINREC_RUNLIB = Path(os.environ["BINREC_RUNLIB"]).absolute()
#: The absolute path to the BinRec scripts directory
BINREC_SCRIPTS = Path(os.environ["BINREC_SCRIPTS"]).absolute()
#: The absolute path to the BinRec link library directory
BINREC_LINK_LD = Path(os.environ["BINREC_LINK_LD"]).absolute()
#: The absolute path to the BinRec build library directory
BINREC_LIB = Path(os.environ["BINREC_LIB"]).absolute()
