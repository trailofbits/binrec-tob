#
# This module is meant to run within a GDB process, which will use whatever Python
# version was built into GDB during compile time and may not be the same Python version
# that is running binrec. Therefore, this module is intentionally targeting an older
# Python version that binrec actually supports for increased compatibility with GDB
# releases.
#
# The actual gdb module is only available when running within a GDB process. See the GDB
# docs: https://sourceware.org/gdb/onlinedocs/gdb/Basic-Python.html#Basic-Python
#
import os
import re
import subprocess
import sys
from typing import Dict, List, Optional, Set, Tuple

SIG_PATTERN = re.compile(r"^type *= *([a-zA-Z_].*\))$")
SIZEOF_PATTERN = re.compile(r"^\$\d+ *= *(\d+)$")
ALLOWED_VAR_ARGS = ("fcntl", "ulimit")
COMMENT_PATTERN = re.compile(r"/\*.*?\*/")
GUESTFS_ROOT = os.environ["BINREC_GUESTFS_ROOT"]
SIGNATURE_PREFIX = "SIG:"
PTR_SIZE = 4


def log(*msg):  # pragma: no cover
    print("_gdb_sigs.py:", *msg, file=sys.stderr)


def _clean_type_name(type_name: str) -> str:
    """
    Clean the type name, treating all pointers as ``void*``.
    """
    if "*" in type_name:
        # We are only concerned with type sizes. So, if the type is a pointer we know
        # the size will always be sizeof(void*). This works for both function pointers
        # and type pointers.
        return "void*"

    return type_name.strip()


def _parse_gdb_function_signature(sig: str) -> Optional[List[str]]:
    """
    Parse the function return and argument types from the output of the GDB ``whatis``
    command.

    :param sig: GDB ``whatis`` output
    :return: a list of return and argument types (the first item in the list is the
        return type)
    """
    match = SIG_PATTERN.match(sig)
    if not match:
        return None

    sig = match.group(1)

    ret_type, arg_types = _split_function_signature(sig)
    arg_types = [ret_type] + arg_types

    return [_clean_type_name(item) for item in arg_types]


def _split_function_signature(sig: str) -> Tuple[str, List[str]]:
    """
    Given a C function signature, split the return statement and the argument list.

    :param sig: the C function signature
    :return: a tuple containing ``(ret_type: str, arg_types: List[str])``
    """
    #
    # We have to support function pointers as arguments and as a return type. To do
    # that, we need to find where the argument list begins. Everything to the left of
    # the argument list is the return value.
    #
    # First, find the opening paren that starts the argument list.
    #
    paren_count = 0
    for i in range(len(sig) - 1, 0, -1):
        if sig[i] == ")":
            # a closing paren, we expect to see a matching opening paren
            # sig[-1] will always be a closing paren.
            paren_count += 1
        elif sig[i] == "(":
            # an opening paren that completes the last closing paren
            paren_count -= 1

        if paren_count == 0:
            # We've reached the opening paren that starts the argument list
            break

    # i is the index for the opening paren that is the start of the argument list.
    # Everything before i is the return value.
    ret_type = sig[:i].strip()
    # get the argument list, stripping the first and last paren
    arg_str = sig[i + 1 : -1]

    #
    # Next, we need to split the argument list. Again, we support function pointers,
    # so splitting on "," is not sufficient. We only split on "," if we are not inside
    # a paren.
    #
    paren_count = 0
    arg_types = []
    arg_start = 0
    for i in range(len(arg_str)):
        c = arg_str[i]
        if c == "(":
            # the start of a function pointer type
            paren_count += 1
        elif c == ")":
            # the end of a function pointer type
            paren_count -= 1
        elif c == "," and paren_count == 0:
            # the separator between arguments and we are not in a paren sequence
            arg_types.append(arg_str[arg_start:i].strip())
            arg_start = i + 1

    # add the last argument to the list
    last_arg = arg_str[arg_start:].strip()
    if last_arg:
        arg_types.append(last_arg)

    if arg_types == ["void"]:
        arg_types = []

    return ret_type, arg_types


def _parse_manpage_function_signature(sig: str) -> List[str]:
    """
    Parse a C function signature from a manpage definition.

    :param sig: the manpage function signature
    :returns: the list of return and argument types
    """
    ret_type, arg_types = _split_function_signature(sig)
    arg_types = [ret_type] + arg_types

    # both ret_type and arg_types contain the name, which we need to remove
    cleaned_types = []
    for item in arg_types:
        if "*" in item or "[" in item:
            # short circut: this is a pointer
            type_name = "void*"
        elif " " in item:
            # ignore the argument name
            type_name, _, _ = item.rpartition(" ")
        else:
            type_name = item

        cleaned_types.append(_clean_type_name(type_name))

    return cleaned_types


def _extract_manpage_function_definition(
    func_name: str, manpage: str
) -> Optional[List[str]]:
    """
    Extract the function definition block from a manpage. This method returns a list of
    lines that define the function signature.

    :param func_name: the function name
    :param manpage: the manpage content
    :returns: the lines within the manpage that define the function signature
    """
    lines = manpage.splitlines()
    start = None
    end = None
    start_marker = "%s(" % func_name

    #
    # Find the start and end of the function definition within the manpage. The start
    # will always be on a line starting with ".B" and contain the marker "{func_name}(".
    # The definition may span multiple lines, in which case, the end line contains the
    # ");" token. There are several ways a function can span multiple lines:
    #
    # 1. The previous line ends with an escape character (\\)
    # 2. The current line starts with a ".B" token
    # 3. An empty line containing only the ".br" command can reside between lines
    #
    for i, line in enumerate(lines):
        if start is None and line.startswith(".B") and start_marker in line:
            start = i
            if ");" in line:
                end = i
                break
        elif start is not None:
            if line == ".br":
                # a line containing just ".br" is ok since it is only whitespace
                # so it doesn't interrupt our parsing
                continue

            if not (line.startswith(".B") or lines[i - 1].endswith("\\")):
                # This is not a valid continuation. The previous line must end with the
                # escape character or our current line must start with ".B"
                break

            if ");" in line:
                # this is a valid continuation and terminates our function signature
                end = i
                break

    if start and end:
        # We have the start and end line indexes that contain the entire function
        # definition. Clean them up before returning.
        block = []
        for line in lines[start : end + 1]:
            if line == ".br":
                # Drop empty line breaks
                continue

            # remove extraneous quote characters
            line = line.replace('"', "")
            if line.startswith("."):
                # remove the leading manpage command (.B)
                _, _, line = line.partition(" ")

            # remove any trailing escape character
            line = line.rstrip("\\").strip()
            block.append(line)

        return block

    return None


def _get_manpage_function_signature(func_name: str) -> Optional[List[str]]:
    """
    Attempt to parse the function signature from the manpage. This method handles both
    section 2 (syscall) and section 3 (library function) manpages within the qemu guest
    image.

    :param func_name: the function name
    :returns: the list of return and argument types for the function
    """
    #
    # Step 1: Find the manpage file, which can either be section 2 (syscall) or
    # 3 (library function).
    #
    for section in (2, 3):
        filename = "%s.%d.gz" % (func_name, section)
        manpage = os.path.join(
            GUESTFS_ROOT, "usr", "share", "man", "man%d" % section, filename
        )
        if os.path.isfile(manpage):
            break
    else:
        log("error: manpage does not exist:", func_name)
        return None

    # Step 2: get the manpage content
    try:
        content = subprocess.check_output(["zcat", manpage])
    except subprocess.CalledProcessError:
        log("error: failed to get manpage content:", func_name)
        return None

    # Step 3: find the line in the manpage that defines the function signature.
    block = _extract_manpage_function_definition(func_name, content.decode())
    if not block:
        log("error: failed to get function block in manpage:", manpage)
        return None

    tokens = []
    for line in block:
        # ignore the ".BI " prefix and remove quote characters
        # line = line.partition(" ")[2].replace('"', '').strip()
        # split all the words (tokens)
        tokens += line.split()

    # join all the tokens and remove the ";" postfix
    sig = " ".join(tokens)[:-1]

    # finally, remove any C++ comments
    sig = COMMENT_PATTERN.sub("", sig)

    return _parse_manpage_function_signature(sig)


def _run_internal():
    import gdb

    if not GUESTFS_ROOT:
        raise KeyError("BINREC_GUESTFS_ROOT environment variable must be defined")

    type_names: Set[str] = set()
    type_sizes: Dict[str, int] = {}
    functions: Dict[str, List[str]] = {}
    retry: List[str] = []

    # Step 1: Load the shared library
    lib_filename = sys.stdin.readline().strip()
    log("loading library file:", lib_filename)
    gdb.execute("file %s" % lib_filename)

    # Step 2: Get all function signatures and register discovered argument/return type
    # names
    for func_name in sys.stdin:
        func_name = func_name.strip()
        sig = gdb.execute("whatis %s" % func_name, to_string=True)

        arg_types = _parse_gdb_function_signature(sig)
        if arg_types:
            functions[func_name] = arg_types
            type_names.update(arg_types)
        else:
            log(
                "error: cannot parse weak or indirect function signature, will retry "
                "using the manpage:",
                func_name,
            )
            retry.append(func_name)

    if retry:
        for func_name in retry:
            log("getting function signature from manpage:", func_name)
            arg_types = _get_manpage_function_signature(func_name)
            if arg_types is not None:
                functions[func_name] = arg_types
                type_names.update(arg_types)

    # Step 3: Get the size for each type
    for type_name in type_names:
        if type_name == "...":
            continue

        if type_name == "void":
            # for some reason gdb thinks "void" is a 1 byte type. We hardcode the type
            # size as 0 here to bypass gdb's weird behavior
            type_sizes["void"] = 0
            continue

        log("getting type info:", type_name)
        try:
            size = gdb.execute("p sizeof(%s)" % type_name, to_string=True)
        except gdb.error:
            size = ""

        match = SIZEOF_PATTERN.match(size)
        if match:
            type_sizes[type_name] = int(match.group(1))
        else:
            log("warning: unknown type size, assuming", PTR_SIZE, "bytes:", type_name)
            type_sizes[type_name] = PTR_SIZE

    # Step 4: Combine the function signatures and type sizes to write the database
    for func_name in sorted(functions.keys()):
        arg_types = functions[func_name]
        if "..." in arg_types:
            log("ignoring variadic argument function:", func_name)
            continue

        sizes = [type_sizes[type_name] for type_name in arg_types]

        ret_tokens = arg_types[0].split()
        is_float = "float" in ret_tokens or "double" in ret_tokens
        if sizes[0] > 0 and sizes[0] < PTR_SIZE:
            # coerce i8 and i16 to i32 on 32-bit systems
            # see issue #204 for more information:
            # https://github.com/trailofbits/binrec-prerelease/issues/204
            sizes[0] = PTR_SIZE

        print(SIGNATURE_PREFIX + func_name, int(is_float), *sizes)


def get_function_signatures(lib_filename: str, function_names: List[str]) -> List[str]:
    """
    Using a GDB subprocess, attempt to extract and parse all the function signatures.
    The output is a list containing the function signatures in the format of::

        func_name is_fp_return ret_size arg1_size .... argN_size
        # example for chown: int chmod(const char *pathname, mode_t mode);
        chmod 0 4 4 4

    :param lib_filename: the library filename
    :param function_names: the function names to extract
    :returns: the list of function signatures
    """
    proc = subprocess.Popen(
        ["gdb", "-q", "-x", __file__], stdout=subprocess.PIPE, stdin=subprocess.PIPE
    )
    stdout, _ = proc.communicate(
        lib_filename.encode() + b"\n" + "\n".join(function_names).encode()
    )

    if proc.returncode != 0:
        return []

    # the output of each function signature is {SIGNATURE_PREFIX}func ....
    # we do this because GDB can have output intermixed in stdout, which we want to
    # ignore and we want to only filter to actual signatures
    skip = len(SIGNATURE_PREFIX)
    return [
        item[skip:]
        for item in stdout.decode().split("\n")
        if item.startswith(SIGNATURE_PREFIX)
    ]


if __name__ == "__main__":  # pragma: no cover
    # We are running within GDB
    _run_internal()
