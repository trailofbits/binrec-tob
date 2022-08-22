import logging
import os
import re
import shutil
import subprocess
import tempfile
from contextlib import suppress
from enum import Enum
from pathlib import Path
from typing import List, Tuple

from . import project
from .env import BINREC_LIB, BINREC_LINK_LD, BINREC_RUNLIB, llvm_command
from .errors import BinRecError
from .lib import binrec_lift, binrec_link, convert_lib_error

logger = logging.getLogger("binrec.lift")

DATA_IMPORT_PATTERN = re.compile(
    r"^\s*\d+:\s+"  # symbol index
    r"([0-9a-fA-F]+)\s+"  # import address (hex)
    r"(\d+)\s+"  # symbol size (bytes)
    r"OBJECT\s+"  # symbol type - we only handle OBJECT
    r"GLOBAL\s+"  # binding - we only handle GLOBAL
    r"[a-zA-Z_]+\s+"  # visibility
    r"\d+\s+"  # section index
    r"([a-zA-F_0-9]+)@",  # symbol name (@ library)
    re.MULTILINE,
)

ELF_SECTION_PATTERN = re.compile(
    r"^\s*\[\s*\d+\]\s+"  # section index
    r"([a-zA-Z0-9._\-]+)\s+"  # section name (capture group 1)
    r"[A-Za-z]+\s+"  # section type
    r"([a-fA-F0-9]+)\s+"  # section start address (capture group 2)
    r"[a-fA-F0-9]+\s+"  # section file offset
    r"([a-fA-F0-9]+)\s+",  # section size (capture group 3)
    re.MULTILINE,
)


def prep_bitcode_for_linkage(
    working_dir: Path, source: Path, destination: Path
) -> None:
    """
    Prepare captured bitcode for linkage. This was originally the content of the
    ``prep_for_linkage.sh`` script.

    :param working_dir: S2E capture directory (typically ``s2e-out-*``)
    :param source: the input bitcode filename
    :param destination: the output bitcode filename
    :raises BinRecError: operation failed
    :raises OSError: I/O error
    """
    logger.debug("preparing capture bitcode for linkage: %s", source)

    fd, tmp = tempfile.mkstemp()
    os.close(fd)

    # tmp.bc is created by the lifting scripts
    tmp_bc = Path(f"{tmp}.bc")

    cwd = str(working_dir)

    if not source.is_absolute():
        source = working_dir / source
    src = str(source)

    if not destination.is_absolute():
        destination = working_dir / destination
    dest = str(destination)

    try:
        binrec_lift.link_prep_1(trace_filename=src, destination=tmp, working_dir=cwd)
    except Exception as err:
        os.remove(tmp)
        with suppress(OSError):
            os.remove(tmp_bc)

        raise convert_lib_error(
            err, f"failed first stage of linkage prep for bitcode: {source}"
        )

    shutil.move(str(tmp_bc), destination)
    try:
        binrec_lift.link_prep_2(trace_filename=dest, destination=tmp, working_dir=cwd)
    except Exception as err:
        with suppress(OSError):
            os.remove(tmp_bc)

        os.remove(tmp)

        raise convert_lib_error(
            err, f"failed second stage of linkage prep for bitcode: {source}"
        )

    shutil.move(str(tmp_bc), destination)
    os.remove(tmp)


def _extract_binary_symbols(trace_dir: Path) -> None:
    """
    Extract the symbols from the original binary.

    **Inputs:** trace_dir / "binary"

    **Outputs:** trace_dir / "symbols"

    :param trace_dir: binrec binary trace directory
    :raises BinRecError: operation failed
    :raises OSError: I/O error
    """
    plt_entry_re = r"^([0-9a-f]+) <(.+)@plt>:$"
    output_file = trace_dir / "symbols"
    logger.debug("extracting symbols from binary: %s", trace_dir.name)
    # TODO(artem): Objdump should be parametrized by target file architecture.
    # e.g.: i686-linux-gnu-objdump. We will need it for multiarch support
    try:
        with open(output_file, "w") as symbols:
            # capture disassembly of file according to objdump
            objdump_proc = subprocess.run(
                ["objdump", "-d", "binary"], cwd=str(trace_dir), stdout=subprocess.PIPE
            )
            objdump_proc.check_returncode()
            objdump_data = objdump_proc.stdout.decode("utf-8")
            # find every line that look like:
            # abc12345 <foo@plt>:
            # and extract out 'abc12345' and 'foo'
            # and then save these into the 'symbols' file
            plt_matches = re.findall(plt_entry_re, objdump_data, re.MULTILINE)
            for plt_match in plt_matches:
                plt_addr, plt_name = plt_match
                symbols.write(f"{plt_addr} {plt_name}\n")

    except subprocess.CalledProcessError:
        raise BinRecError(f"failed to extract symbols from trace: {trace_dir.name}")


def _extract_data_imports(trace_dir: Path) -> None:
    """
    Extract the data imports from the original binary. This creates a text file
    where each line is a data import in the following format::

        <symbol_address_hex>  <symbol_size>  <symbol_name>

    **Inputs:** trace_dir / "binary"

    **Outputs:** trace_dir / "data_imports"

    :param trace_dir: binrec binary trace directory
    :raises BinRecError: operation failed
    :raises OSError: I/O error
    """
    binary = trace_dir / "binary"
    data_imports = trace_dir / "data_imports"

    logger.debug("extracting data imports from binary: %s", trace_dir.name)

    try:
        readelf = subprocess.check_output(["readelf", "--dyn-sym", str(binary)])
    except subprocess.CalledProcessError:
        raise BinRecError(
            f"failed to extract data imports from binary: {trace_dir.name}"
        )

    with open(data_imports, "w") as file:
        for match in DATA_IMPORT_PATTERN.finditer(readelf.decode()):
            print(" ".join(match.groups()), file=file)


def _extract_dependencies(trace_dir: Path) -> None:
    """
    Extract the list of dependency libraries from the original binary and write them
    to a file. This function gets the absolute file path for every direct dependency
    of the binary.

    **Inputs:** trace_dir / "binary"

    **Outputs:** trace_dir / "dependencies"

    :param trace_dir: binrec binary trace directory
    :raises BinRecError: operation failed
    :raises OSError: I/O error
    """
    binary = trace_dir / "binary"

    # first, get the list of direct dependencies. direct_deps will be a list of library
    # _filenames_ (e.g.- libselinux.so.1).
    direct_deps = []
    try:
        objdump = subprocess.check_output(["objdump", "--private-headers", str(binary)])
    except subprocess.CalledProcessError:
        raise BinRecError(
            f"failed to extract direct dependencies from binary: {trace_dir.name}"
        )

    for line in objdump.decode().splitlines(keepends=False):
        parts = line.strip().split(maxsplit=1)
        if len(parts) == 2 and parts[0] == "NEEDED":
            # line: NEEDED <lib_filename>
            lib_name = parts[1]
            direct_deps.append(lib_name)

    # next, get all dependencies, both direct and indirect. all_deps will be a list
    # of tuples: (lib_name, lib_path)
    all_deps: List[Tuple[str, str]] = []
    try:
        ldd = subprocess.check_output(["ldd", str(binary)])
    except subprocess.CalledProcessError:
        raise BinRecError(
            f"failed to extract all dependencies from binary: {trace_dir.name}"
        )

    for line in ldd.decode().splitlines(keepends=False):
        parts = line.strip().split("=>", 1)
        if len(parts) == 2:
            # line: lib_name  =>  lib_path  (hex_address)
            lib_name = parts[0].strip()
            lib_path = parts[1].rpartition("(")[0].strip()
            all_deps.append((lib_name, lib_path))

    # We have the list of direct dependency library names and the full path to every
    # dependency. Resolve each direct dependency to its full path.
    result = []
    for lib_name, lib_path in all_deps:
        if lib_name in direct_deps:
            result.append(lib_path)
            direct_deps.remove(lib_name)

    if direct_deps:
        # We have any remaining direct dependencies that could not be resolved
        # to an absolute file path. This most likely means that the lifted bitcode
        # will not link.
        logger.warn(
            "failed to resolve %d dependencies, linking may fail: %s",
            len(direct_deps),
            ", ".join(direct_deps),
        )

    # Save dependencies to trace_dir/dependencies, one per line
    logger.debug(
        "discovered %d direct dependencies: %s", len(result), ", ".join(result)
    )
    with open(trace_dir / "dependencies", "w") as fp:
        for lib_path in result:
            print(lib_path, file=fp)


def _extract_sections(trace_dir: Path) -> None:
    """
    Extract the sections from the original binary. This creates a text file
    where each line is an ELF section in the following format::

        <section_address_hex>  <section_size_hex>  <section_name>

    **Inputs:** trace_dir / "binary"

    **Outputs:** trace_dir / "sections"

    :param trace_dir: binrec binary trace directory
    :raises BinRecError: operation failed
    :raises OSError: I/O error
    """
    binary = trace_dir / "binary"
    sections = trace_dir / "sections"

    logger.debug("extracting sections from binary: %s", trace_dir.name)

    try:
        readelf = subprocess.check_output(["readelf", "--section-headers", str(binary)])
    except subprocess.CalledProcessError:
        raise BinRecError(f"failed to extract sections from binary: {trace_dir.name}")

    with open(sections, "w") as file:
        for match in ELF_SECTION_PATTERN.finditer(readelf.decode()):
            line = list(match.groups())
            line.append(line.pop(0))  # move the symbol name to the end
            print(" ".join(line), file=file)


def _clean_bitcode(trace_dir: Path) -> None:
    """
    Clean the trace for linking with custom helpers.

    **Inputs:** trace_dir / captured.bc

    **Outputs:**

    - trace_dir / "cleaned.bc"
    - trace_dir / "cleaned.ll"
    - trace_dir / "cleaned-memssa.ll"

    :param trace_dir: binrec binary trace directory
    :raises BinRecError: operation failed
    """
    logger.debug("cleaning captured bitcode: %s", trace_dir.name)
    try:
        binrec_lift.clean(
            trace_filename="captured.bc",
            destination="cleaned",
            working_dir=str(trace_dir),
        )
    except Exception as err:
        raise convert_lib_error(
            err, f"failed to clean captured LLVM bitcode: {trace_dir.name}"
        )


def _apply_fixups(trace_dir: Path) -> None:
    """
    Apply binrec fixups to the cleaned bitcode.

    **Inputs:** trace_dir / "cleaned.bc"

    **Outputs:**
      - trace_dir / "linked.bc"
      - trace_dir / "linked.ll"

    :param trace_dir: binrec binary trace directory
    :raises BinRecError: operation failed
    """
    logger.debug("applying fixups to captured bitcode: %s", trace_dir.name)
    try:
        subprocess.check_call(
            [
                llvm_command("llvm-link"),
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

    try:
        subprocess.check_call(
            [llvm_command("llvm-dis"), "linked.bc"], cwd=str(trace_dir)
        )
    except subprocess.CalledProcessError:  # pragma: no cover
        # we don't really care if this fail since we really only want the
        # human-readable for debugging purposes.
        pass


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
    :raises BinRecError: operation failed
    """
    logger.debug(
        "performing initial lifting of captured LLVM bitcode: %s", trace_dir.name
    )
    try:
        binrec_lift.lift(
            trace_filename="linked.bc",
            destination="lifted",
            working_dir=str(trace_dir),
            clean_names=True,
        )
    except Exception as err:
        raise convert_lib_error(
            err, f"failed to perform initial lifting of LLVM bitcode: {trace_dir.name}"
        )


class OptimizationLevel(Enum):
    NORMAL = 1
    HIGH = 2


def _optimize_bitcode(trace_dir: Path, opt_level: OptimizationLevel) -> None:
    """
    Optimize the lifted LLVM module.

    **Inputs:** trace_dir / "lifted.bc"

    **Outputs:**
        - trace_dir / "optimized.bc"
        - trace_dir / "optimized.ll"
        - trace_dir / "optimized-memssa.ll"

    :param trace_dir: binrec binary trace directory
    :raises BinRecError: operation failed
    """
    optimizers = {
        OptimizationLevel.NORMAL: binrec_lift.optimize,
        OptimizationLevel.HIGH: binrec_lift.optimize_better,
    }

    logger.debug("optimizing lifted bitcode: %s", trace_dir.name)

    if opt_level not in optimizers:
        raise BinRecError(f"Unknown optimization level: {opt_level}")

    optimizer = optimizers[opt_level]

    try:
        optimizer(
            trace_filename="lifted.bc",
            destination="optimized",
            memssa_check_limit=100000,
            working_dir=str(trace_dir),
        )
    except Exception as err:
        raise convert_lib_error(
            err, f"failed to optimized lifted LLVM bitcode: {trace_dir.name}"
        )


def _disassemble_bitcode(trace_dir: Path) -> None:
    """
    Disassemble optimized bitcode.

    **Inputs:** trace_dir / "optimized.bc"

    **Outputs:** trace_dir / "optimized.ll"

    :param trace_dir: binrec binary trace directory
    :raises BinRecError: operation failed
    """
    logger.debug("disassembling optimized bitcode: %s", trace_dir.name)
    try:
        subprocess.check_call(
            [llvm_command("llvm-dis"), "optimized.bc"], cwd=str(trace_dir)
        )
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
    :raises BinRecError: operation failed
    """
    logger.debug("lowering optimized LLVM bitcode for compilation: %s", trace_dir.name)
    try:
        binrec_lift.compile_prep(
            trace_filename="optimized.bc",
            destination="recovered",
            working_dir=str(trace_dir),
        )
    except Exception as err:
        raise convert_lib_error(
            err, f"failed to lower optimized LLVM bitcode: {trace_dir.name}"
        )


def _compile_bitcode(trace_dir: Path) -> None:
    """
    Compile the recovered bitcode.

    **Inputs:** trace_dir / "recovered.bc"

    **Outputs:** trace_dir / "recovered.o"

    :param trace_dir: binrec binary trace directory
    :raises BinRecError: operation failed
    """
    logger.debug("compiling recovered LLVM bitcode: %s", trace_dir.name)
    try:
        subprocess.check_call(
            [
                llvm_command("llc"),
                "-filetype",
                "obj",
                "-o",
                "recovered.o",
                "recovered.bc",
            ],
            cwd=str(trace_dir),
        )
    except subprocess.CalledProcessError:
        raise BinRecError(f"failed to compile recovered LLVM bitcode: {trace_dir.name}")


def _link_recovered_binary(trace_dir: Path) -> None:
    """
    Linked the recovered binary.

    **Inputs:** trace_dir / "recovered.o"

    **Outputs:** trace_dir / "recovered"

    :param trace_dir: binrec binary trace directory
    :raises BinRecError: operation failed
    """
    i386_ld = str(BINREC_LINK_LD / "i386.ld")
    libbinrec_rt = str(BINREC_LIB / "libbinrec_rt.a")

    logger.debug("linking recovered binary: %s", trace_dir.name)
    try:
        binrec_link.link(
            binary_filename=str(trace_dir / "binary"),
            recovered_filename=str(trace_dir / "recovered.o"),
            runtime_library=libbinrec_rt,
            linker_script=i386_ld,
            destination=str(trace_dir / "recovered"),
            dependencies_filename=str(trace_dir / "dependencies"),
        )
    except Exception as err:
        raise convert_lib_error(err, "failed to link recovered binary")


def lift_trace(project_name: str, opt_level=OptimizationLevel.NORMAL) -> None:
    """
    Lift and recover a binary from a binrec trace. This lifts, compiles, and links
    capture bitcode to a recovered binary. This method works on the binrec trace
    directory, ``s2e-out-<binary>``.

    :param project_name: name of the s2e project to operate on
    :param opt_level: How much effort to put into optimizing lifted bitcode
    """
    merged_trace_dir = project.merged_trace_dir(project_name)
    if not merged_trace_dir.is_dir():
        raise BinRecError(
            f"nothing to lift: directory does not exist: {merged_trace_dir}"
        )

    # Step 1: extract symbols, data imports, sections, and dependencies
    _extract_binary_symbols(merged_trace_dir)
    _extract_data_imports(merged_trace_dir)
    _extract_sections(merged_trace_dir)
    _extract_dependencies(merged_trace_dir)

    # Step 2: clean captured LLVM bitcode
    _clean_bitcode(merged_trace_dir)

    # Step 3: apply fixups to captured bitcode
    _apply_fixups(merged_trace_dir)

    # Step 4: initial lifting
    _lift_bitcode(merged_trace_dir)

    # Step 5: optimize the lifted bitcode
    _optimize_bitcode(merged_trace_dir, opt_level)

    # Step 6: disassemble optimized bitcode
    _disassemble_bitcode(merged_trace_dir)

    # Step 7: recover the optimized bitcode
    _recover_bitcode(merged_trace_dir)

    # Step 8: compile recovered bitcode
    _compile_bitcode(merged_trace_dir)

    # Step 9: Link the recovered binary
    _link_recovered_binary(merged_trace_dir)

    logger.info(
        "successfully lifted and recovered binary for project %s: %s",
        project_name,
        merged_trace_dir / "recovered",
    )


def main() -> None:
    import argparse
    import sys

    from .core import init_binrec

    init_binrec()

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-v", "--verbose", action="count", help="enable verbose logging"
    )
    parser.add_argument(
        "-o",
        "--optimize",
        action="store_true",
        help="Enable extra lifted bitcode optimizations",
    )
    parser.add_argument("project_name", help="lift and compile the binary trace")

    args = parser.parse_args()
    if args.verbose:
        logging.getLogger("binrec").setLevel(logging.DEBUG)
        if args.verbose > 1:
            from .audit import enable_python_audit_log

            enable_python_audit_log()

    opt_level = OptimizationLevel.NORMAL
    if args.optimize:
        logger.debug("Enabling optimized bitcode emission")
        opt_level = OptimizationLevel.HIGH

    lift_trace(args.project_name, opt_level)
    sys.exit(0)


if __name__ == "__main__":  # pragma: no cover
    main()
