"""
S2E output directories:

- ``s2e-out-{binary}-{#}/`` - a trace of a single input for a binary.

  - ``./{#}/`` - a single capture within a trace, where a trace can contain multiple
    captures
  - ``./merged/`` - a merge of all captures within the trace

- ``s2e-out-{binary}/`` - the final merged trace for all traces
"""

import logging
import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import List

from .env import BINREC_BIN, BINREC_ROOT
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
                "llvm-link",
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
    SOURCE_BITCODE = "captured.bc"
    DESTINATION_BITCODE = "captured-link-ready.bc"

    # Verify that the output directory (destination) is empty.
    if destination.exists():
        shutil.rmtree(destination)

    destination.mkdir(exist_ok=True)

    # prepare each captured bitcode (captured.bc) for linking
    for capture in capture_dirs:
        prep_bitcode_for_linkage(capture, SOURCE_BITCODE, DESTINATION_BITCODE)

    outfile = destination / SOURCE_BITCODE
    shutil.copy(capture_dirs[0] / DESTINATION_BITCODE, outfile)

    fd, tmp = tempfile.mkstemp(suffix=".bc")
    os.close(fd)

    for capture_dir in capture_dirs:
        _link_bitcode(
            base=outfile,
            source=capture_dir / DESTINATION_BITCODE,
            destination=Path(tmp),
        )
        shutil.move(tmp, outfile)

    logger.debug("disassembling linked bitcode: %s", outfile)

    try:
        subprocess.check_call(["llvm-dis", str(outfile)])
    except subprocess.CalledProcessError:
        raise BinRecError(f"llvm-dis failed on linked bitcode: {outfile}")

    trace_info_files = [capture / "traceInfo.json" for capture in capture_dirs]
    _merge_trace_info(trace_info_files, destination / "traceInfo.json")


def merge_traces(binary_name: str) -> None:
    """
    Merge multiple traces into a single trace.

    **Input:** BINREC_ROOT / "s2e-out-{binary_name}-{trace_num}"

    **Output:** BINREC_ROOT / "s2e-out-{binary_name}"

    This was originally named "merge_all_inputs" bash function.

    :param binary_name: the name of the binary to merge
    """
    outdir = BINREC_ROOT / f"s2e-out-{binary_name}"
    pattern = re.compile(r"s2e-out-%s-[0-9]$" % binary_name)

    capture_dirs: List[Path] = []
    for trace_dir in BINREC_ROOT.iterdir():
        if trace_dir.is_dir() and pattern.match(trace_dir.name):
            capture = trace_dir / "merged"
            if capture.is_dir():
                capture_dirs.append(capture)

    if not capture_dirs:
        raise BinRecError(
            f"nothing to merge: no merged captures found for binary: {binary_name}"
        )

    merge_bitcode(capture_dirs, outdir)
    shutil.copy2(capture_dirs[0] / "binary", outdir / "binary")
    shutil.copy2(capture_dirs[0] / "Makefile", outdir / "Makefile")


def merge_trace_captures(trace_id: str) -> None:
    """
    Merge multiple captures within a single trace.

    **Input:** BINREC_ROOT / "s2e-out-{trace_id}" / "{capture_id}"

    **Output:** BINREC_ROOT / "s2e-out-{trace_id}" / "merged"

    Originally this was the ``merge_one_input`` function in the
    ``merge_captured_dirs_multi.sh`` script.

    :param trace_id: S2E trace id in the format of ``{binary}-{id}``.
    """
    logger.debug("merging single trace: %s", trace_id)
    capture_dirs: List[Path] = []
    source = BINREC_ROOT / f"s2e-out-{trace_id}"
    outdir = source / "merged"

    for capture in source.iterdir():
        # find capture directories, which are numbered (0, 1, 2, 3, etc.)
        if capture.is_dir() and capture.name.isdigit() and len(capture.name) == 1:
            capture_dirs.append(capture)

    if not capture_dirs:
        raise BinRecError(f"nothing to merge: no captures found for trace: {source}")

    merge_bitcode(capture_dirs, outdir)

    # Copy the binary and makefile from the first capture directory, 0, into the output
    # directory.
    shutil.copy2(source / "0" / "binary", outdir / "binary")
    shutil.copy2(source / "Makefile", outdir / "Makefile")


def recursive_merge_traces(binary_name: str) -> None:
    """
    Merge all captures and all traces for a binary.

    **Input:** BINREC_ROOT / "s2e-out-{binary}-{trace_num}"

    **Output:** BINREC_ROOT / "s2e-out-{binary}"

    Originally this was the content of the ``double_merge.sh`` script.

    :param binary_name: the binary name
    """
    logger.info("performing double merge on binary: %s", binary_name)

    pattern = re.compile(r"s2e-out-(%s-[0-9])$" % binary_name)
    trace_ids: List[str] = []

    # identify trace directories for the binary
    for trace_dir in BINREC_ROOT.iterdir():
        match = pattern.match(trace_dir.name)
        if match and trace_dir.is_dir():
            # add the trace id ({binary_name}-{iteration})
            trace_ids.append(match.group(1))

    if not trace_ids:
        raise BinRecError(
            f"nothing to merge: no trace directory exists: s2e-out-{binary_name}-*"
        )

    # Merge each trace captures
    for trace_id in trace_ids:
        merge_trace_captures(trace_id)

    # merge the traces
    merge_traces(binary_name)


def main() -> None:
    import argparse
    import sys

    from .core import init_binrec

    init_binrec()

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-v", "--verbose", action="count", help="enable verbose logging"
    )
    parser.add_argument("-t", "--trace-id", action="store", help="merge single trace")
    parser.add_argument(
        "-b",
        "--binary-name",
        action="store",
        help="recursively merge all traces for binary",
    )

    args = parser.parse_args()
    if args.trace_id and args.binary_name:
        parser.error("cannot specify both -t/--trace-id and -b/--binary-name")

    if not args.trace_id and not args.binary_name:
        parser.error("must specify either -t/--trace-id or -b/--binary-name")

    if args.verbose:
        logging.getLogger("binrec").setLevel(logging.DEBUG)
        if args.verbose > 1:
            from .audit import enable_python_audit_log

            enable_python_audit_log()

    if args.trace_id:
        merge_trace_captures(args.trace_id)
    elif args.binary_name:
        recursive_merge_traces(args.binary_name)

    sys.exit(0)


if __name__ == "__main__":  # pragma: no cover
    main()
