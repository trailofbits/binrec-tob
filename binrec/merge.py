import logging
import os
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import List

from . import project
from .env import BINREC_BIN, llvm_command
from .errors import BinRecError
from .lift import prep_bitcode_for_linkage

logger = logging.getLogger("binrec.merge")


def _link_bitcode(base: Path, source: Path, destination: Path):
    """
    Link two LLVM bitcode captures.

    :param base: bitcode file to use as the basis for the link (passed to llvm-link as
        ``-override`` parameter)
    :param source: input bitcode file to link
    :param destination: output bitcode file
    """
    logger.info("linking prepared bitcode: %s", source)
    try:
        subprocess.check_call(
            [
                llvm_command("llvm-link"),
                "-print-after-all",
                "-v",
                "-o",
                str(destination),
                f"-override={base}",
                str(source),
            ]
        )
    except subprocess.CalledProcessError:
        raise BinRecError(f"llvm-link failed on captured bitcode: {source}")


def _merge_trace_info(trace_info_files: List[Path], destination: Path) -> None:
    """
    Merge binrec trace information files, ``traceInfo.json``.

    :param trace_info_files: list of input trace info files to merge
    :param destination: output file path
    """

    binrec_tracemerge = str(BINREC_BIN / "binrec_tracemerge")
    args = [str(filename) for filename in trace_info_files] + [str(destination)]
    try:
        subprocess.check_call([binrec_tracemerge] + args)
    except subprocess.CalledProcessError:
        raise BinRecError(f"binrec_tracemerge failed on trace info: {destination}")


def merge_bitcode(capture_dirs: List[Path], destination: Path) -> None:
    """
    Perform the actual merging of multiple trace captures or merged captures into a
    single LLVM bitcode and disassembly. This method performs the following:

    - Recursively delete and then recreate the ``destination``
    - Prepares each captured bitcode, ``captured.bc``, for linkage
    - Links all the preparsed captured bitcode into a single bitcode file,
      ``{destination}/captured.bc``
    - Disassembles the liked capture bitcode to LLVM assembly code,
      ``{destination}/captured.ll``
    - Merges all the trace information JSON files to a single trace info,
      ``{destination}/traceInfo.json``

    :param capture_dirs: list of trace capture directories or merged captures
    :param outdidestindestinationationr: output directory
    """
    logger.debug("merging captures %s to %s", capture_dirs, destination)
    SOURCE_BITCODE = Path("captured.bc")
    DESTINATION_BITCODE = Path("captured-link-ready.bc")

    # Verify that the output directory (destination) is empty.
    if destination.exists():
        shutil.rmtree(destination)

    destination.mkdir(exist_ok=True)

    # prepare each captured bitcode (captured.bc) for linking
    for capture in capture_dirs:
        prep_bitcode_for_linkage(capture, SOURCE_BITCODE, DESTINATION_BITCODE)

    # copy the first captured-link-ready.bc to {destination}/captured.bc
    outfile = destination / SOURCE_BITCODE
    shutil.copy(capture_dirs[0] / DESTINATION_BITCODE, outfile)

    fd, tmp = tempfile.mkstemp(suffix=".bc")
    os.close(fd)

    for capture_dir in capture_dirs[1:]:
        # Link each captured-link-ready.bc to the {destination}/captured.bc
        _link_bitcode(
            base=outfile,
            source=capture_dir / DESTINATION_BITCODE,
            destination=Path(tmp),
        )
        shutil.move(tmp, outfile)

    logger.debug("disassembling linked bitcode: %s", outfile)

    try:
        subprocess.check_call([llvm_command("llvm-dis"), str(outfile)])
    except subprocess.CalledProcessError:
        raise BinRecError(f"llvm-dis failed on linked bitcode: {outfile}")

    trace_info_files = [capture / "traceInfo.json" for capture in capture_dirs]
    _merge_trace_info(trace_info_files, destination / "traceInfo.json")


def merge_traces(project_name: str) -> None:
    """
    Merge multiple traces into a single trace.

    **Input:** BINREC_PROJECTS / "{project_name}" / "s2e-out-{trace_num}"

    **Output:** BINREC_PROJECTS / "{project_name}" / "s2e-out"

    This was originally named "merge_all_inputs" bash function.

    :param project_name: the name of the project to merge
    """
    trace_dirs = project.get_trace_dirs(project_name)
    outdir = project.merged_trace_dir(project_name)

    if not trace_dirs:
        raise BinRecError(
            f"nothing to merge: no captures found for binary: {project_name}"
        )

    merge_bitcode(trace_dirs, outdir)
    shutil.copy2(trace_dirs[0] / "binary", outdir / "binary")


def main() -> None:
    import argparse
    import sys

    from .core import init_binrec

    init_binrec()

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-v", "--verbose", action="count", help="enable verbose logging"
    )
    parser.add_argument("project_name", help="Name of analysis project")

    args = parser.parse_args()

    if args.verbose:
        logging.getLogger("binrec").setLevel(logging.DEBUG)
        if args.verbose > 1:
            from .audit import enable_python_audit_log

            enable_python_audit_log()

    merge_traces(args.project_name)

    sys.exit(0)


if __name__ == "__main__":  # pragma: no cover
    main()
