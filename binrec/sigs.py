import logging
import re
import subprocess
from pathlib import Path
from typing import List

from ._gdb_sigs import get_function_signatures
from .errors import BinRecError

logger = logging.getLogger("binrec.sigs")

NM_FUNCTION_PATTERN = re.compile(r"[0-9a-f]{8} [tiTW] ")


def get_exported_functions(lib_filename: Path) -> List[str]:
    """
    Get the exported function names for a given library file.

    :param lib_filename: library filename
    :returns: the list of exported function names
    :raises BinRecError: extraction of parsing failed
    """
    logger.info("getting exported functions from library: %s", lib_filename)
    try:
        data = subprocess.check_output(["nm", "-D", str(lib_filename)])
    except subprocess.CalledProcessError:
        raise BinRecError(f"failed to extract functions from library: {lib_filename}")

    functions: List[str] = []
    logger.debug("parsing exported function names")
    for line in data.decode().splitlines(keepends=False):
        # output is similar to: 000f1760 W open
        if NM_FUNCTION_PATTERN.match(line):
            name = line.split(maxsplit=2)[2]
            functions.append(name)

    logger.debug(
        "discoverd %d exported functions for library %s", len(functions), lib_filename
    )
    return functions


def generate_library_signature_database(lib_filename: Path, out_filename: Path) -> None:
    """
    Generate the library signature database file.

    :param lib_filename: library filename
    :param out_filename: output database filename
    """
    funcs = get_exported_functions(lib_filename)

    logger.info("getting function signatures for library %s", lib_filename)
    sigs = get_function_signatures(str(lib_filename), funcs)

    logger.info(
        "discovered %d function signatures for library %s", len(sigs), lib_filename
    )
    with open(out_filename, "w") as fp:
        for line in sigs:
            print(line, file=fp)


def main():
    import argparse

    from .core import init_binrec

    init_binrec()

    parser = argparse.ArgumentParser()
    parser.add_argument("libfile", type=Path, help="input library file")
    parser.add_argument("outfile", type=Path, help="output filename")

    args = parser.parse_args()
    generate_library_signature_database(args.libfile, args.outfile)


if __name__ == "__main__":  # pragma: no cover
    main()
