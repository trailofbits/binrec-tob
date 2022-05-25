import os
import subprocess
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
    "BINREC_PROJECTS",
    "llvm_command",
)

LLVM_MAJOR_VERSION = 14


def llvm_command(command_name: str) -> str:
    """
    A stub function to resolve an unversioned LLVM command to the correct command for
    the system. For now, this function simply appends the major version of the
    supported LLVM platform. In the future, this may resolve the LLVM command to a path
    or support multiple LLVM versions.
    """
    #return f"{command_name}-{LLVM_MAJOR_VERSION}"
    return f"{command_name}"


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

    # binrec_link needs the GCC_LIB environment variable to work properly (see
    # compiler_command.cpp). Attempt to set it if it isn't present.
    if not os.environ.get("GCC_LIB"):
        try:
            version = subprocess.check_output(["gcc", "-dumpversion"]).decode().strip()
        except subprocess.CalledProcessError:
            version = ""

        if version:
            os.environ["GCC_LIB"] = f"/usr/lib/gcc/x86_64-linux-gnu/{version}"


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
#: The aboslute path to the BinRec S2E projects directory
BINREC_PROJECTS = Path(os.environ["BINREC_PROJECTS"]).absolute()
