import os
import re
import subprocess
from pathlib import Path
from typing import List

from dotenv import load_dotenv

from .errors import BinRecError

__all__ = (
    "BINREC_DEBUG",
    "BINREC_ROOT",
    "BINREC_BIN",
    "BINREC_RUNLIB",
    "BINREC_SCRIPTS",
    "BINREC_LINK_LD",
    "BINREC_LIB",
    "BINREC_PROJECTS",
    "llvm_command",
    "project_dir",
    "merged_trace_dir",
    "trace_dir",
    "input_files_dir",
    "trace_config_filename",
    "campaign_filename",
    "s2e_config_filename",
    "project_binary_filename",
    "get_trace_dirs",
)

LLVM_MAJOR_VERSION = 14


def llvm_command(command_name: str) -> str:
    """
    A stub function to resolve an unversioned LLVM command to the correct command for
    the system. For now, this function simply appends the major version of the
    supported LLVM platform. In the future, this may resolve the LLVM command to a path
    or support multiple LLVM versions.
    """
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

    # Add Clang / LLVM binaries (and other dependencies) from S2E install to PATH
    if not os.environ.get("S2E_BIN"):
        raise BinRecError(".env file must specify S2E_BIN")
    os.environ["PATH"] = os.environ["S2E_BIN"] + ":" + os.environ["PATH"]


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
#: The absolute path to the BinRec S2E projects directory
BINREC_PROJECTS = Path(os.environ["BINREC_PROJECTS"]).absolute()
#: The absolute path to the libc module used during analysis
BINREC_LIBC_MODULE = Path(os.environ["BINREC_LIBC_MODULE"]).absolute()
#: The absolute path to the qemu guest filesystem root
BINREC_GUESTFS_ROOT = Path(os.environ["BINREC_GUESTFS_ROOT"]).absolute()
#: The environment variable name for the BINREC_DEBUG value
BINREC_DEBUG_ENV = "BINREC_DEBUG"
#: Binrec is running in debug mode
BINREC_DEBUG = os.environ.get(BINREC_DEBUG_ENV) == "1"

#: The default filename for the binrec trace config script
TRACE_CONFIG_FILENAME = "binrec_trace_config.sh"

#: The default input files directory name
INPUT_FILES_DIRNAME = "input_files"


def project_dir(project_name: str) -> Path:
    """
    :returns: the path to the S2E project directory
    """
    return BINREC_PROJECTS / project_name


def merged_trace_dir(project_name: str) -> Path:
    """
    :returns: the path to the merged trace directory for the project. Contains
        merged traces, IRs, and binaries, etc.
    """
    return project_dir(project_name) / "s2e-out"


def recovered_binary_filename(project_name: str) -> Path:
    """
    :returns: the path to the recovered binary filename for the project.
    """
    return merged_trace_dir(project_name) / "recovered"


def trace_dir(project_name: str, trace_id: int) -> Path:
    """
    :returns: the path to a single project trace directory
    """
    return project_dir(project_name) / f"s2e-out-{trace_id}"


def input_files_dir(project_name: str) -> Path:
    """
    :returns the path to the project input files directory
    """
    return project_dir(project_name) / INPUT_FILES_DIRNAME


def trace_config_filename(project_name: str) -> Path:
    """
    :returns: the path to the project trace config filename
    """
    return project_dir(project_name) / TRACE_CONFIG_FILENAME


def campaign_filename(project_name: str) -> Path:
    """
    :returns: the filename of the JSON campaign file for the project
    """
    return project_dir(project_name) / "campaign.json"


def s2e_config_filename(project_name: str) -> Path:
    """
    :returns: the filename of the S2E configuration file for the project
    """
    return project_dir(project_name) / "s2e-config.lua"


def project_binary_filename(project_name: str) -> Path:
    """
    :returns: the filename of the project analysis binary
    """
    binary = project_dir(project_name) / "binary"
    return binary.readlink()


def get_trace_dirs(project_name: str) -> List[Path]:
    """
    :returns: the list of project completed trace directories, excluding the merged
        trace directory
    """
    pattern = re.compile(r"s2e-out-[0-9]$")

    trace_dirs: List[Path] = []
    for trace_dir in project_dir(project_name).iterdir():
        if trace_dir.is_dir() and pattern.match(trace_dir.name):
            trace_dirs.append(trace_dir)

    return sorted(trace_dirs)
