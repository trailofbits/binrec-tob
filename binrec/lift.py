import logging
import os
import shutil
import subprocess
import tempfile
from contextlib import suppress
from pathlib import Path

from .core import AnyPath
from .env import (
    BINREC_BIN,
    BINREC_LIB,
    BINREC_LINK_LD,
    BINREC_ROOT,
    BINREC_RUNLIB,
    BINREC_SCRIPTS,
)
from .errors import BinRecError
from .project import merge_dir

logger = logging.getLogger("binrec.lift")

BINREC_LIFT = str(BINREC_BIN / "binrec_lift")
BINREC_LINK = str(BINREC_BIN / "binrec_link")


def prep_bitcode_for_linkage(
    working_dir: AnyPath, source: AnyPath, destination: AnyPath
) -> None:
    """
    Prepare captured bitcode for linkage. This was originally the content of the
    ``prep_for_linkage.sh`` script.

    :param working_dir: S2E capture directory (typically ``s2e-out-*``)
    :param source: the input bitcode filename
    :param destination: the output bitcode filename
    """
    logger.debug("preparing capture bitcode for linkage: %s", source)

    fd, tmp = tempfile.mkstemp()
    os.close(fd)

    # tmp.bc is created by the lifting scripts
    tmp_bc = Path(f"{tmp}.bc")

    cwd = str(working_dir)
    src = str(source)
    dest = str(destination)

    # We need the absolute path to the destination so we can copy the resulting bitcode
    # to it when we are finished. binrec_lift and llvm-link seem to be hardcoded to
    # need the  and source/destination passed in may be relative to it. However, our
    # Python process's CWD will be different so we need to resolve the destination
    # path against the working_dir.
    dest_abs = Path(destination)
    if not dest_abs.is_absolute():
        dest_abs = Path(working_dir) / destination

    try:
        subprocess.check_call([BINREC_LIFT, "--link-prep-1", "-o", tmp, src], cwd=cwd)
    except subprocess.CalledProcessError:
        os.remove(tmp)
        with suppress(OSError):
            os.remove(tmp_bc)
        raise BinRecError(f"failed first stage of linkage prep for bitcode: {source}")

    # try:
    #     # softfloat appears to be part of qemu and implements floating point math
    #     subprocess.check_call(
    #         [
    #             "llvm-link",
    #             "-o",
    #             dest,
    #             tmp_bc,
    #             str(BINREC_RUNLIB / "softfloat.bc"),
    #         ],
    #         cwd=cwd,
    #     )
    # except subprocess.CalledProcessError:
    #     # The original script ran this in via "eval", which does not honor "set -e".
    #     # So, I'm seeing this llvm-link call fail consistently when run against a
    #     # merged capture (s2e-out-binary-1/merged), but the script continues and
    #     # properly finishes the merging / lifting.
    #     #
    #     # If this call fails then the destination file won't exist and the next call
    #     # to binrec_lift won't be run.
    #     #
    #     # TODO add a check that determine whether llvm-link is needed before calling
    #     # it.
    #     logger.warning(
    #         "failed llvm link prep for bitcode (this may be normal if the "
    #         "bitcode has already been merged from multiple captures): %s",
    #         src,
    #     )

    if tmp_bc.is_file():
        shutil.move(tmp_bc, dest_abs)
    #if dest_abs.is_file():
        # llvm-link worked successfully
        try:
            subprocess.check_call(
                [BINREC_LIFT, "--link-prep-2", "-o", tmp, dest], cwd=cwd
            )
        except subprocess.CalledProcessError:
            os.remove(tmp_bc)
            os.remove(tmp)
            raise BinRecError(
                f"failed second stage of linkage prep for bitcode: " f"{source}"
            )

    # else:
    # TODO (hbrodin): Raise exception here. If any step fails we have little chance of
    # getting any sucess later...

    # Regardless of the llvm-link result, the temp bitcode file will exist and we use
    # it as the final output file.
    shutil.move(f"{tmp}.bc", dest_abs)
    os.remove(tmp)


def _extract_binary_symbols(trace_dir: Path) -> None:
    """
    Extract the symbols from the original binary.

    **Inputs:** trace_dir / "binary"

    **Outputs:** trace_dir / "symbols"

    :param trace_dir: binrec binary trace directory
    """
    makefile = str(BINREC_SCRIPTS / "s2eout_makefile")
    logger.debug("extracting symbols from binary: %s", trace_dir.name)
    try:
        subprocess.check_call(["make", "-f", makefile, "symbols"], cwd=str(trace_dir))
    except subprocess.CalledProcessError:
        raise BinRecError(f"failed to extract symbols from trace: {trace_dir.name}")


def _clean_bitcode(trace_dir: Path) -> None:
    """
    Clean the trace for linking with custom helpers.

    **Inputs:** trace_dir / captured.bc

    **Outputs:**

    - trace_dir / "cleaned.bc"
    - trace_dir / "cleaned.ll"
    - trace_dir / "cleaned-memssa.ll"

    :param trace_dir: binrec binary trace directory
    """
    logger.debug("cleaning captured bitcode: %s", trace_dir.name)
    try:
        subprocess.check_call(
            [BINREC_LIFT, "--clean", "-o", "cleaned", "captured.bc"], cwd=str(trace_dir)
        )
    except subprocess.CalledProcessError:
        raise BinRecError(f"failed to clean captured LLVM bitcode: {trace_dir.name}")


def _apply_fixups(trace_dir: Path) -> None:
    """
    Apply binrec fixups to the cleaned bitcode.

    **Inputs:** trace_dir / "cleaned.bc"

    **Outputs:** trace_dir / "linked.bc"

    :param trace_dir: binrec binary trace directory
    """
    logger.debug("applying fixups to captured bitcode: %s", trace_dir.name)
    try:
        subprocess.check_call(
            [
                "llvm-link",
                "-o",
                "linked.bc",
                "cleaned.bc",
                str(BINREC_RUNLIB / "custom-helpers.bc"),
            ],
            cwd=str(trace_dir),
        )
    except subprocess.CalledProcessError:
        raise BinRecError(
            f"failed to apply fixups to captured LLVM bitcode: {trace_dir.name}"
        )


def _lift_bitcode(trace_dir: Path) -> None:
    """
    Lift trace into correct LLVM module.

    **Inputs:** trace_dir / "linked.bc"

    **Outputs:**
      - trace_dir / "lifted.bc"
      - trace_dir / "lifted.ll"
      - trace_dir / "lifted-memssa.ll"
      - trace_dir / "rfuncs"

    :param trace_dir: binrec binary trace directory
    """
    logger.debug(
        "performing initially lifting of captured LLVM bitcode: %s", trace_dir.name
    )
    try:
        subprocess.check_call(
            [BINREC_LIFT, "--lift", "-o", "lifted", "linked.bc", "--clean-names"],
            cwd=str(trace_dir),
        )
    except subprocess.CalledProcessError:
        raise BinRecError(
            f"failed to perform initial lifting of LLVM bitcode: {trace_dir.name}"
        )


def _optimize_bitcode(trace_dir: Path) -> None:
    """
    Optimize the lifted LLVM module.

    **Inputs:** trace_dir / "lifted.bc"

    **Outputs:**
        - trace_dir / "optimized.bc"
        - trace_dir / "optimized.ll"
        - trace_dir / "optimized-memssa.ll"

    :param trace_dir: binrec binary trace directory
    """
    logger.debug("optimizing lifted bitcode: %s", trace_dir.name)
    try:
        subprocess.check_call(
            [
                BINREC_LIFT,
                "--optimize",
                "-o",
                "optimized",
                "lifted.bc",
                "--memssa-check-limit=100000",
            ],
            cwd=str(trace_dir),
        )
    except subprocess.CalledProcessError:
        raise BinRecError(f"failed to optimized lifted LLVM bitcode: {trace_dir.name}")


def _disassemble_bitcode(trace_dir: Path) -> None:
    """
    Disassemble optimized bitcode.

    **Inputs:** trace_dir / "optimized.bc"

    **Outputs:** trace_dir / "optimized.ll"

    :param trace_dir: binrec binary trace directory
    """
    logger.debug("disassembling optimized bitcode: %s", trace_dir.name)
    try:
        subprocess.check_call(["llvm-dis", "optimized.bc"], cwd=str(trace_dir))
    except subprocess.CalledProcessError:
        raise BinRecError(
            f"failed to disassemble captured LLVM bitcode: {trace_dir.name}"
        )


def _recover_bitcode(trace_dir: Path) -> None:
    """
    Compile the trace to an object file. This method recovers the optimized bitcode.

    **Inputs:** trace_dir / "optimized.bc"

    **Outputs:**
        - trace_dir / "recovered.bc"
        - trace_dir / "recovered.ll"
        - trace_dir / "recovered-memssa.ll"

    :param trace_dir: binrec binary trace directory
    """
    logger.debug("lowering optimized LLVM bitcode for compilation: %s", trace_dir.name)
    try:
        subprocess.check_call(
            [BINREC_LIFT, "--compile", "-o", "recovered", "optimized.bc"],
            cwd=str(trace_dir),
        )
    except subprocess.CalledProcessError:
        raise BinRecError(f"failed to lower optimized LLVM bitcode: {trace_dir.name}")


def _compile_bitcode(trace_dir: Path) -> None:
    """
    Compile the recovered bitcode.

    **Inputs:** trace_dir / "recovered.bc"

    **Outputs:** trace_dir / "recovered.o"

    :param trace_dir: binrec binary trace directory
    """
    makefile = str(BINREC_SCRIPTS / "s2eout_makefile")
    logger.debug("compiling recovered LLVM bitcode: %s", trace_dir.name)
    try:
        subprocess.check_call(
            ["make", "-f", makefile, "-sr", "recovered.o"], cwd=str(trace_dir)
        )
    except subprocess.CalledProcessError:
        raise BinRecError(f"failed to compile recovered LLVM bitcode: {trace_dir.name}")


def _link_recovered_binary(trace_dir: Path) -> None:
    """
    Linked the recovered binary.

    **Inputs:** trace_dir / "recovered.o"

    **Outputs:** trace_dir / "recovered"

    :param trace_dir: binrec binary trace directory
    """
    i386_ld = str(BINREC_LINK_LD / "i386.ld")
    libbinrec_rt = str(BINREC_LIB / "libbinrec_rt.a")

    logger.debug("linking recovered binary: %s", trace_dir.name)
    try:
        subprocess.check_call(
            [
                BINREC_LINK,
                "-b",
                "binary",
                "-r",
                "recovered.o",
                "-l",
                libbinrec_rt,
                "-o",
                "recovered",
                "-t",
                i386_ld,
            ],
            cwd=str(trace_dir),
        )
    except subprocess.CalledProcessError:
        raise BinRecError(f"failed to link recovered binary: {trace_dir.name}")


def lift_trace(project_name: str, index : int) -> None:
    """
    Lift and recover a binary from a binrec trace. This lifts, compiles, and links
    capture bitcode to a recovered binary. This method works on the binrec trace
    directory, ``s2e-out-<binary>``.

    :param binary: binary name
    """
    trace_dir = merge_dir(project_name, index)
    if not trace_dir.is_dir():
        raise BinRecError(
            f"nothing to lift: directory does not exist: s2e-out-{binary}"
        )

    # Step 1: extract symbols
    _extract_binary_symbols(trace_dir)

    # Step 2: clean captured LLVM bitcode
    _clean_bitcode(trace_dir)

    # Step 3: apply fixups to captured bitcode
    _apply_fixups(trace_dir)

    # Step 4: initial lifting
    _lift_bitcode(trace_dir)

    # Step 5: optimize the lifted bitcode
    _optimize_bitcode(trace_dir)

    # Step 6: disassemble optimized bitcode
    _disassemble_bitcode(trace_dir)

    # Step 7: recover the optimized bitcode
    _recover_bitcode(trace_dir)

    # Step 8: compile recovered bitcode
    _compile_bitcode(trace_dir)

    # Step 9: Link the recovered binary
    _link_recovered_binary(trace_dir)

    logger.info(f"successfully lifted and recovered binary in project: {project_name}, available as {trace_dir}/recovered")


def main() -> None:
    import argparse
    import sys

    from .core import init_binrec

    init_binrec()

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-v", "--verbose", action="count", help="enable verbose logging"
    )
    parser.add_argument("project_name", help="lift and compile the binary trace")
    parser.add_argument("index", type=int, nargs='?', help="index of merge to lift and compile", default=0)

    args = parser.parse_args()
    if args.verbose:
        logging.getLogger("binrec").setLevel(logging.DEBUG)
        if args.verbose > 1:
            from .audit import enable_python_audit_log

            enable_python_audit_log()

    lift_trace(args.project_name, args.index)
    sys.exit(0)


if __name__ == "__main__":  # pragma: no cover
    main()
