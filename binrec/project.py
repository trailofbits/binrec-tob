import errno
import json
import logging
import os
import re
import shlex
import subprocess
from pathlib import Path
from typing import Iterable, List

from binrec.env import BINREC_PROJECTS

from .errors import BinRecError

BOOTSTRAP_EXECUTE_COMMAND = 'execute "${TARGET_PATH}"'

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
    :param args: list of arguments
    """
    bootstrap = project_dir(project) / "bootstrap.sh"
    logger.debug("setting command line arguments in %s to %s", bootstrap, args)

    with open(bootstrap, "r") as file:
        lines = file.readlines()

    for i, line in enumerate(lines):
        # Find what line contains the command line arguments
        if not line.lstrip().startswith(BOOTSTRAP_EXECUTE_COMMAND):
            continue

        found = i
        break
    else:
        # We didn't find the line, append it to the end of the file. This shouldn't
        # happen under normal circumstances.
        #
        # We make sure that there is a newline separating the original last line and
        # the new execute call.
        lines.extend(["\n", ""])
        found = len(lines) - 1

    # Change the actual call to the sample with the new command line arguments.
    lines[found] = f"{BOOTSTRAP_EXECUTE_COMMAND} {shlex.join(args)}\n"
    with open(bootstrap, "w") as file:
        file.writelines(lines)


def validate_project(project: str, args: List[str]) -> None:
    """
    Compare the original binary against the lifted binary for a given sample of
    command line arguments. This method runs the original and the lifted binary
    and then compares the process return code, stdout, and stderr content. An
    ``AssertionError`` is raised if any of the comparison criteria does not match
    between the original and lifted sample.

    :param project: project name
    :param args: list of arguments
    """

    logger.info("Validating project: %s", project)

    merged_dir = merged_trace_dir(project)
    lifted = str(merged_dir / "recovered")
    original = str(merged_dir / "binary")
    target = str(merged_dir / "test-target")

    # We link to the binary we are running to make sure argv[0] is the same
    # for the original and the lifted program.
    os.link(original, target)
    logger.debug(">> running original sample with args:", args)
    original_proc = subprocess.Popen(
        [target] + args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE,
    )
    # stdin is not supported yet in the updated s2e integration
    # if trace.stdin:
    #     original_proc.stdin.write(trace.stdin.encode())

    original_proc.stdin.close()  # type: ignore
    original_proc.wait()
    os.remove(target)

    original_stdout = original_proc.stdout.read()  # type: ignore
    original_stderr = original_proc.stderr.read()  # type: ignore

    os.link(lifted, target)
    logger.debug(">> running recovered sample with args:", args)
    lifted_proc = subprocess.Popen(
        [target] + args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE,
    )
    # stdin is not supported yet in the updated s2e integration
    # if trace.stdin:
    #    lifted_proc.stdin.write(trace.stdin.encode())

    lifted_proc.stdin.close()  # type: ignore
    lifted_proc.wait()
    os.remove(target)

    lifted_stdout = lifted_proc.stdout.read()  # type: ignore
    lifted_stderr = lifted_proc.stderr.read()  # type: ignore

    assert original_proc.returncode == lifted_proc.returncode
    assert original_stdout == lifted_stdout
    assert original_stderr == lifted_stderr

    logger.info(
        "Output from %s's original and lifted binaries match for args: %s",
        project,
        str(args),
    )


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
pluginsConfig.FunctionLog = {{
    logFile = 'function-log.txt'
}}
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


def _debug_trace_info(trace_info: Path, doprint=True):
    with open(trace_info, "r") as f:
        ti = json.load(f)

        # TODO (hbrodin): There is probably a more clever way of doing this...
        def remap(a1, a2):
            ctfup = ti[a1][a2]
            ti[a1][a2] = list(map(lambda x: [hex(x[0]), hex(x[1])], ctfup))

        remap("functionLog", "callerToFollowUp")
        remap("functionLog", "entryToCaller")
        remap("functionLog", "entryToReturn")
        # remap("functionLog", "entryToTbs")

        ti["functionLog"]["entryToTbs"] = list(
            map(
                lambda x: [hex(x[0]), list(map(hex, x[1]))],
                ti["functionLog"]["entryToTbs"],
            )
        )

        ti["successors"] = list(
            map(
                lambda x: {"pc": hex(x["pc"]), "successor": hex(x["successor"])},
                ti["successors"],
            )
        )

        ti["functionLog"]["entries"] = list(map(hex, ti["functionLog"]["entries"]))

        if doprint:
            print(json.dumps(ti, indent=4))
        return ti


def _diff_trace_info(paths: List[Path], show_common=False):
    def _do_compare(etype, a, b):
        if a == b:
            print(f"\n{etype} are equal")
            return
        diff = ea.difference(eb)
        if diff:
            print(f"\n{etype} only in a: {diff}")
        diff = eb.difference(ea)
        if diff:
            print(f"\n{etype} only in b: {diff}")
        if show_common:
            intersection = ea.intersection(eb)
            if intersection:
                print(f"\n{etype} common to both a and b {intersection}")

    ta = _debug_trace_info(paths[0], False)
    tb = _debug_trace_info(paths[1], False)
    print(f"Show com{show_common}")
    # Compare entries (entrypoints)
    ea = set(ta["functionLog"]["entries"])
    eb = set(tb["functionLog"]["entries"])
    _do_compare("Entries", ea, eb)

    # Compare successors
    dict2tuple = lambda x: (x["pc"], x["successor"])  # noqa: E731
    ea = set(map(dict2tuple, ta["successors"]))
    eb = set(map(dict2tuple, tb["successors"]))
    _do_compare("Successors", ea, eb)

    # Compare callerToFollowup
    ea = set(map(tuple, ta["functionLog"]["callerToFollowUp"]))
    eb = set(map(tuple, tb["functionLog"]["callerToFollowUp"]))
    _do_compare("callerToFollowUp", ea, eb)

    # Compare entryToCaller
    ea = set(map(tuple, ta["functionLog"]["entryToCaller"]))
    eb = set(map(tuple, tb["functionLog"]["entryToCaller"]))
    _do_compare("entryToCaller", ea, eb)

    # Compare entryToReturn
    ea = set(map(tuple, ta["functionLog"]["entryToReturn"]))
    eb = set(map(tuple, tb["functionLog"]["entryToReturn"]))
    _do_compare("entryToReturn", ea, eb)

    # compare entryToTbs
    etbs2tuple = lambda x: (x[0], tuple(sorted(x[1])))  # noqa: E731
    ea = set(map(etbs2tuple, ta["functionLog"]["entryToTbs"]))
    eb = set(map(etbs2tuple, tb["functionLog"]["entryToTbs"]))
    _do_compare("entryToTbs", ea, eb)


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

    dbg = subparsers.add_parser("debug-traceinfo")
    dbg.add_argument("path", nargs=1, type=Path, help="Path to traceinfo to debug")

    dbg = subparsers.add_parser("diff-traceinfo")
    dbg.add_argument(
        "paths", nargs=2, type=Path, help="Diff of two traceInfo.json files"
    )
    dbg.add_argument(
        "--show_common", help="Show entries common to both files", action="store_true"
    )

    set_args = subparsers.add_parser("set-args")
    set_args.add_argument("project", help="project name")
    set_args.add_argument("args", nargs="*", help="command line arguments")

    validate = subparsers.add_parser("validate")
    validate.add_argument("project", help="project name")
    validate.add_argument("args", nargs="*", help="command line arguments")

    args = parser.parse_args()

    if args.verbose:
        logging.getLogger("binrec").setLevel(logging.DEBUG)

    if args.current_parser == "run":
        run_project(args.project, args.args)
    elif args.current_parser == "new":
        new_project(args.project, args.binary, args=args.args, sym_args=args.sym_args)
    elif args.current_parser == "list":
        _list()
    elif args.current_parser == "list-traces":
        _list_traces(args)
    elif args.current_parser == "list-merged":
        _list_merge_dirs(args)
    elif args.current_parser == "debug-traceinfo":
        _debug_trace_info(args.path[0])
    elif args.current_parser == "diff-traceinfo":
        _diff_trace_info(args.paths, args.show_common)
    elif args.current_parser == "set-args":
        set_project_args(args.project, args.args or [])
    elif args.current_parser == "validate":
        validate_project(args.project, args.args or [])
    else:
        parser.print_help()


if __name__ == "__main__":  # pragma: no cover
    main()
