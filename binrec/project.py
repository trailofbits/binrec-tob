import errno
import json
import logging
import os
import re
import shlex
import subprocess
from pathlib import Path
from typing import Iterable, List, Optional, Union

from binrec.env import BINREC_PROJECTS

from .errors import BinRecError

BOOTSTRAP_EXECUTE_COMMAND = 'execute "${TARGET_PATH}"'
BOOTSTRAP_SYM_ARGS_START = "S2E_SYM_ARGS="
BOOTSTRAP_SYM_ARGS_END = (
    'LD_PRELOAD="${S2E_SO}" "${TARGET}" "$@" > /dev/null 2> /dev/null'
)

logger = logging.getLogger("binrec.project")


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


def trace_dir(project_name: str, trace_id: int) -> Path:
    """
    :returns: the path to a single project trace directory
    """
    return project_dir(project_name) / f"s2e-out-{trace_id}"


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


def parse_batch_file(filename: Path) -> List[List[str]]:
    with open(filename, "r") as file:
        # read command line invocations
        invocations = [shlex.split(line.strip()) for line in file]
        if not invocations:
            # no command line arguments specified, run the test once with no arguments
            invocations = [[""]]
    return invocations


def listing() -> List[str]:
    try:
        output = subprocess.check_output(["s2e", "info"])
    except subprocess.CalledProcessError:
        raise BinRecError("s2e run failed to get project list")

    d = json.loads(output)
    return d["projects"].keys()


def _list_merge_dirs(args):
    project = _get_project_name(args)
    print(merged_trace_dir(project))


def _get_project_name(args) -> str:
    project = str(args.name[0])
    if not project:
        raise BinRecError("Please specify project name.")
    return project


def _list() -> None:
    for proj in listing():
        print(proj)


def traces(project: str) -> Iterable[int]:
    projdir = project_dir(project)
    for name in os.listdir(projdir):
        if not name.startswith("s2e-out-"):
            continue
        yield int(name.split("-")[-1])


def _list_traces(args) -> None:
    proj = _get_project_name(args)
    for t in traces(proj):
        print(f"{t}: {trace_dir(proj, t)}")


def _config_file(project: str) -> Path:
    return project_dir(project) / "s2e-config.lua"


def set_project_args(project: str, args: List[str]) -> None:
    """
    Set an existing project's sample command line arguments. This method modifies
    the project's ``bootstrap.sh`` script, changing the how the sample is executed
    within the qemu image.

    :param project: project name
    :param args: list of arguments, the first of which indicates symbolic arguments
    """
    # Check for no arguments provided or empty quotes from batch files
    if not args:
        args = [""]
    elif args[0] in ('""', "''"):
        args = [""] + args[1:]

    bootstrap = project_dir(project) / "bootstrap.sh"
    logger.debug("setting symbolic arguments in %s to %s", bootstrap, args[0])
    logger.debug("setting command line arguments in %s to %s", bootstrap, args[1:])

    with open(bootstrap, "r") as file:
        lines = file.readlines()

    found_cli = None
    found_sym = None

    for i, line in enumerate(lines):
        # Find what lines contain the concrete and symbolic arguments
        stripped = line.lstrip()
        if stripped.startswith(BOOTSTRAP_EXECUTE_COMMAND):
            found_cli = i
        elif stripped.startswith(BOOTSTRAP_SYM_ARGS_START):
            found_sym = i

    if found_cli is None:
        # We didn't find the line, append it to the end of the file. This shouldn't
        # happen under normal circumstances.
        #
        # We make sure that there is a newline separating the original last line and
        # the new execute call.
        lines.extend(["\n", ""])
        found_cli = len(lines) - 1

    if found_sym is None:
        # If we didn't find this line, then there is probably a big problem
        # with the bootsrap script, as this line is the code that actually
        # executes the target.
        raise BinRecError(f"Could not set symbolic args for project: {project}")

    # Change the call to execute the sample with the new concrete arguments
    lines[found_cli] = f"{BOOTSTRAP_EXECUTE_COMMAND} {shlex.join(args[1:])}\n"

    # Change the call to execute the sample with the new symbolic arguments
    lines[
        found_sym
    ] = f'    {BOOTSTRAP_SYM_ARGS_START}"{args[0]}" {BOOTSTRAP_SYM_ARGS_END}\n'

    with open(bootstrap, "w") as file:
        file.writelines(lines)


def validate_project(
    project: str,
    args: List[str],
    match_stdout: Union[bool, str] = True,
    match_stderr: Union[bool, str] = True,
    skip_first: bool = False,
    stdin: Union[bool, str] = False,
) -> None:
    """
    Compare the original binary against the lifted binary for a given sample of
    command line arguments. This method runs the original and the lifted binary
    and then compares the process return code, stdout, and stderr content. An
    ``AssertionError`` is raised if any of the comparison criteria does not match
    between the original and lifted sample.

    :param project: project name
    :param args: list of arguments
    :param match_stdout: how to validate stdout content
    :param match_stderr: how to validate stderr content
    :param skip_first: ignore first argument (lets batch files be used for
                       tracing and validation)
    """
    if skip_first:
        args = args[1:]

    logger.info("Validating project: %s", project)

    merged_dir = merged_trace_dir(project)
    lifted = str(merged_dir / "recovered")
    original = str(merged_dir / "binary")
    target = str(merged_dir / "test-target")

    # We link to the binary we are running to make sure argv[0] is the same
    # for the original and the lifted program.
    os.link(original, target)
    logger.debug(">> running original sample with args: %s", args)

    if stdin is False or stdin is None:
        # /dev/null
        stdin_file: Optional[int] = subprocess.DEVNULL
    elif stdin is True:
        # pass through stdin
        stdin_file = None
    else:
        raise NotImplementedError("stdin content is not currently supported")

    original_proc = subprocess.Popen(
        [target] + args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=stdin_file,
    )
    # stdin is not supported yet in the updated s2e integration
    # if trace.stdin:
    #     original_proc.stdin.write(trace.stdin.encode())

    # Only close if stdin is a file / pipe (unsupported at this time)
    # original_proc.stdin.close()  # type: ignore
    original_proc.wait()
    os.remove(target)

    original_stdout = original_proc.stdout.read()  # type: ignore
    original_stderr = original_proc.stderr.read()  # type: ignore

    os.link(lifted, target)
    logger.debug(">> running recovered sample with args: %s", args)
    lifted_proc = subprocess.Popen(
        [target] + args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=stdin_file,
    )
    # stdin is not supported yet in the updated s2e integration
    # if trace.stdin:
    #    lifted_proc.stdin.write(trace.stdin.encode())

    # Only close if stdin is a file / pipe (unsupported at this time)
    # lifted_proc.stdin.close()  # type: ignore
    lifted_proc.wait()
    os.remove(target)

    lifted_stdout = lifted_proc.stdout.read()  # type: ignore
    lifted_stderr = lifted_proc.stderr.read()  # type: ignore

    assert (
        original_proc.returncode == lifted_proc.returncode
    ), "recovered exit code does not match original"

    if match_stdout is True:
        assert (
            original_stdout == lifted_stdout
        ), "recovered stdout content does not match original"
    elif isinstance(match_stdout, str):
        assert (
            re.match(match_stdout, lifted_stdout.decode(errors="replace")) is not None
        ), "regex pattern for stdout content does not match"

    if match_stderr is True:
        assert (
            original_stderr == lifted_stderr
        ), "recovered stderr content does not match original"
    elif isinstance(match_stderr, str):
        assert (
            re.match(match_stderr, lifted_stderr.decode(errors="replace")) is not None
        ), "regex pattern for stderr content does not match"

    logger.info(
        "Output from %s's original and lifted binaries match for args: %s",
        project,
        str(args),
    )


def validate_batch_project(project: str, batch_file: Path, skip_first: bool) -> None:
    """
    Validate a project with a batch of inputs provided in a file.

    :param project: project name
    :param batch_file: file containing one or more set of command line arguments
    """
    invocations = parse_batch_file(batch_file)

    for invocation in invocations:
        validate_project(project, invocation, skip_first=skip_first)


def run_project(project: str, args: List[str] = None) -> None:
    """
    Run a project. The ``args`` parameter is optional and, when specified, changes the
    project's command line arguments prior to running.

    :param project: project name
    :param args: project command line arguments
    """
    logger.info("Running project: %s", project)

    if args is not None:
        set_project_args(project, args)

    try:
        subprocess.check_call(["s2e", "run", "--no-tui", project])
    except subprocess.CalledProcessError:
        raise BinRecError(f"s2e run failed for project: {project}")


def run_batch_project(project: str, batch_file: Path) -> None:
    """
    Run a project with a batch of inputs provided in a file.

    :param project: project name
    :param batch_file: file containing one or more set of command line arguments
    """
    invocations = parse_batch_file(batch_file)

    for invocation in invocations:
        run_project(project, invocation)


def new_project(
    project_name: str,
    binary_filename: Path,
    sym_args: str = None,
    args: List[str] = None,
) -> Path:
    """
    Create a new S2E analysis project.

    :param project_name: the analysis project name
    :param binary_filename: the path to binary being analyzed
    :param sym_args: list of symbolic arguments in the form of ``"X Y Z"``
    :param args: list of inital command line arguments for the binary, which can be
        set later using :func:`set_project_args`
    :returns: the path to the project directory
    """
    project_path = project_dir(project_name)
    if project_path.is_dir():
        raise FileExistsError(
            errno.EEXIST, os.strerror(errno.EEXIST), str(project_path)
        )

    logger.info("Creating project: %s", project_name)
    callargs = ["s2e", "new_project", "--name", project_name]
    if sym_args:
        callargs.extend(["--sym-args", sym_args])

    callargs.append(str(binary_filename))

    if args:
        callargs.extend(args)

    try:
        subprocess.check_call(callargs)
    except subprocess.CalledProcessError:
        raise BinRecError(f"s2e run failed for project: {project_name}")

    with open(_config_file(project_name), "a") as file:
        file.write(
            f"""
add_plugin(\"ELFSelector\")
add_plugin(\"FunctionMonitor\")
add_plugin(\"FunctionLog\")
add_plugin(\"ExportELF\")
pluginsConfig.ExportELF = {{
    baseDirs = {{
        "{project_path}"
    }},
    exportInterval = 1000 -- export every 1000 basic blocks
}}
"""
        )

    return project_path


def main() -> None:
    import argparse

    from .core import init_binrec

    init_binrec()

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-v", "--verbose", action="count", help="enable verbose logging"
    )

    subparsers = parser.add_subparsers(dest="current_parser")

    new_proj = subparsers.add_parser("new")
    new_proj.add_argument("project", help="Name of new analysis project")
    new_proj.add_argument("binary", help="Path to binary used in analysis")
    new_proj.add_argument(
        "sym_args",
        help='Symbolic args to pass to binary. E.g. "1 3" ensures makes arguments one '
        "and three symbolic.",
    )
    new_proj.add_argument(
        "args",
        nargs="*",
        help="Arguments to pass to binary. Use @@ to indicate a file with symbolic "
        "content",
    )

    subparsers.add_parser("list")
    listtracecmd = subparsers.add_parser("list-traces")
    listtracecmd.add_argument(
        "name", nargs=1, type=str, help="Name of project to list traces for"
    )

    listtracecmd = subparsers.add_parser("list-merged")
    listtracecmd.add_argument(
        "name", nargs=1, type=str, help="Name of project to list merged traces for"
    )

    run = subparsers.add_parser("run")
    run.add_argument("project", help="Name of project to run")
    run.add_argument("--args", nargs="*", help="command line arguments", default=None)
    # TODO (hbrodin): Enable passing of additional parameters to s2e run

    run_batch = subparsers.add_parser("run-batch")
    run_batch.add_argument("project", help="project name")
    run_batch.add_argument("batch_file", help="file containing inputs to run")

    set_args = subparsers.add_parser("set-args")
    set_args.add_argument("project", type=str, help="project name")
    set_args.add_argument("args", nargs="*", help="command line arguments")

    validate = subparsers.add_parser("validate")
    validate.add_argument("project", type=str, help="project name")
    validate.add_argument("args", nargs="*", help="command line arguments")

    validate_batch = subparsers.add_parser("validate-batch")
    validate_batch.add_argument("project", type=str, help="project name")
    validate_batch.add_argument(
        "batch_file", type=Path, help="file containing inputs to validate"
    )
    validate_batch.add_argument(
        "--skip_first", help="ignore first argument in invocations", action="store_true"
    )

    args = parser.parse_args()

    if args.verbose:
        logging.getLogger("binrec").setLevel(logging.DEBUG)

    if args.current_parser == "run":
        run_project(args.project, args.args)
    elif args.current_parser == "run-batch":
        run_batch_project(args.project, args.batch_file)
    elif args.current_parser == "new":
        new_project(args.project, args.binary, args=args.args, sym_args=args.sym_args)
    elif args.current_parser == "list":
        _list()
    elif args.current_parser == "list-traces":
        _list_traces(args)
    elif args.current_parser == "list-merged":
        _list_merge_dirs(args)
    elif args.current_parser == "set-args":
        set_project_args(args.project, args.args or [])
    elif args.current_parser == "validate":
        validate_project(args.project, args.args or [])
    elif args.current_parser == "validate-batch":
        validate_batch_project(args.project, args.batch_file, args.skip_first)
    else:
        parser.print_help()


if __name__ == "__main__":  # pragma: no cover
    main()
