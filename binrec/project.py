import errno
import json
import logging
import os
import re
import subprocess
from pathlib import Path
from typing import Iterable, List, Optional

from binrec.batch import BatchTraceParams, TraceParams, patch_s2e_project

from .env import (
    INPUT_FILES_DIRNAME,
    input_files_dir,
    merged_trace_dir,
    project_dir,
    trace_dir,
)
from .errors import BinRecError

logger = logging.getLogger("binrec.project")


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

    logger.debug("setting symbolic arguments in %s to %s", project, args[0])
    logger.debug("setting command line arguments in %s to %s", project, args[1:])

    TraceParams(args).write_config_script(project)


def _link_lifted_input_files(project_name: str) -> None:
    """
    Link the project's input files directory to the final lifted directory so that
    comarpison between the original and the lifted binary can be performed.
    """
    source = input_files_dir(project_name)
    dest = merged_trace_dir(project_name) / INPUT_FILES_DIRNAME

    if source.is_dir() and not dest.is_dir():
        dest.symlink_to(source)


def validate_lift_result(project: str, trace: TraceParams) -> None:
    """
    Compare the original binary against the lifted binary for a given sample of
    command line arguments. This method runs the original and the lifted binary
    and then compares the process return code, stdout, and stderr content. An
    ``AssertionError`` is raised if any of the comparison criteria does not match
    between the original and lifted sample.

    :param project: project name
    :param trace: Set of trace parameters
    """
    logger.info(
        "Validating project %s with arguments: %s", project, trace.concrete_args
    )

    merged_dir = merged_trace_dir(project)
    lifted = str(merged_dir / "recovered")
    original = str(merged_dir / "binary")
    target = str(merged_dir / "test-target")

    # We link to the binary we are running to make sure argv[0] is the same
    # for the original and the lifted program.
    os.link(original, target)
    logger.debug(">> running original sample with args: %s", trace.concrete_args)

    trace.setup_input_file_directory(project)
    _link_lifted_input_files(project)

    if trace.stdin is False or trace.stdin is None:
        # /dev/null
        stdin_file: Optional[int] = subprocess.DEVNULL
    elif trace.stdin is True:
        # pass through stdin
        stdin_file = None
    else:
        raise NotImplementedError("stdin content is not currently supported")

    original_proc = subprocess.Popen(
        [target] + trace.concrete_args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=stdin_file,
        cwd=str(merged_dir),
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
    logger.debug(">> running recovered sample with args: %s", trace.concrete_args)
    lifted_proc = subprocess.Popen(
        [target] + trace.concrete_args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=stdin_file,
        cwd=str(merged_dir),
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

    if trace.match_stdout is True:
        assert (
            original_stdout == lifted_stdout
        ), "recovered stdout content does not match original"
    elif isinstance(trace.match_stdout, str):
        assert (
            re.match(trace.match_stdout, lifted_stdout.decode(errors="replace"))
            is not None
        ), "regex pattern for stdout content does not match"

    if trace.match_stderr is True:
        assert (
            original_stderr == lifted_stderr
        ), "recovered stderr content does not match original"
    elif isinstance(trace.match_stderr, str):
        assert (
            re.match(trace.match_stderr, lifted_stderr.decode(errors="replace"))
            is not None
        ), "regex pattern for stderr content does not match"

    logger.info(
        "Output from %s's original and lifted binaries match for args: %s",
        project,
        str(trace.concrete_args),
    )


def validate_lift_result_batch_params(params: BatchTraceParams) -> None:
    """
    Validate a project with batch trace parameters.

    :param params: trace parameters
    """
    logger.info(
        "validating batch results for %s with %d traces",
        params.project,
        len(params.traces),
    )
    for trace in params.traces:
        validate_lift_result(params.project, trace)


def validate_lift_result_batch_file(project: str, batch_file: Path) -> None:
    """
    Validate a project with a batch of inputs provided in a file.

    :param project: project name
    :param batch_file: file containing one or more set of command line arguments
    """
    binary = merged_trace_dir(project) / "binary"
    params = BatchTraceParams.load(binary, batch_file, project=project)
    validate_lift_result_batch_params(params)


def run_project(project: str, campaign_json: Optional[str]) -> None:
    """
    Run a project. The ``args`` parameter is optional and, when specified, changes the
    project's command line arguments prior to running.

    :param project: project name
    :param args: project command line arguments
    """
    logger.info("Running project %s with arguments: %s", project, json)

    if campaign_json is not None:
        body = json.loads(campaign_json)
        args: List[str] = [body["symbolic"]] + [
            str(concrete) for concrete in body["concrete"]
        ]
        set_project_args(project, args)

    try:
        subprocess.check_call(["s2e", "run", "--no-tui", project])
    except subprocess.CalledProcessError:
        raise BinRecError(f"s2e run failed for project: {project}")


def run_project_batch_params(params: BatchTraceParams) -> None:
    """
    Run a project with a batch trace parameters.

    :param params: batch trace parameters
    """
    logger.info("running %s with %d traces", params.project, len(params.traces))
    for i, trace in enumerate(params.traces, start=1):
        trace.setup_input_file_directory(params.project)
        trace.write_config_script(params.project)
        run_project(params.project, None)  # FIXME(jl)


def run_project_batch_file(project: str, batch_file: Path) -> None:
    """
    Run a project with a batch of inputs provided in a file.

    :param project: project name
    :param batch_file: file containing one or more set of command line arguments
    """
    binary = project_dir(project) / "binary"
    params = BatchTraceParams.load(binary, batch_file, project=project)
    run_project_batch_params(params)


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
    callargs = ["s2e", "new_project", "--name", project_name, str(binary_filename)]

    try:
        subprocess.check_call(callargs)
    except subprocess.CalledProcessError:
        raise BinRecError(f"s2e run failed for project: {project_name}")

    # create the input files directory
    input_files = input_files_dir(project_name)
    input_files.mkdir()

    # link to the sample binary so we can easily reference it later on
    binary_target = project_path / binary_filename.name
    binary = project_path / "binary"
    binary.symlink_to(binary_target)

    # Update the configuration file to load our plugins and map in the input files
    # directory to the analysis VM
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

table.insert(pluginsConfig.HostFiles.baseDirs, "{input_files}")
"""
        )

    project_args = [sym_args or ""]
    if args:
        project_args.extend(args)

    patch_s2e_project(project_name)
    set_project_args(project_name, project_args)

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
    run.add_argument("json", help="no")
    # TODO (hbrodin): Enable passing of additional parameters to s2e run

    run_batch = subparsers.add_parser("run-batch")
    run_batch.add_argument("project", help="project name")
    run_batch.add_argument("batch_file", help="file containing inputs to run")

    run_fuzzer = subparsers.add_parser("run-fuzzer")
    run_fuzzer.add_argument("project", help="project name")
    run_fuzzer.add_argument(
        "campaign_file", type=Path, help="file containing JSON fuzzing campaign"
    )
    run_fuzzer.add_argument(
        "--dump_batch",
        help="dump a batch file of the campaign fuzzing realization",
        action="store_true",
    )

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

    args = parser.parse_args()

    if args.verbose:
        logging.getLogger("binrec").setLevel(logging.DEBUG)

    if args.current_parser == "run":
        run_project(args.project, open(args.json, "r"))
    elif args.current_parser == "run-batch":
        run_project_batch_file(args.project, Path(args.batch_file))
    elif args.current_parser == "new":
        new_project(args.project, Path(args.binary), args=None, sym_args=None)
    elif args.current_parser == "list":
        _list()
    elif args.current_parser == "list-traces":
        _list_traces(args)
    elif args.current_parser == "list-merged":
        _list_merge_dirs(args)
    elif args.current_parser == "set-args":
        set_project_args(args.project, args.args or [])
    elif args.current_parser == "validate":
        validate_lift_result(args.project, TraceParams(args.args or []))
    elif args.current_parser == "validate-batch":
        validate_lift_result_batch_file(args.project, Path(args.batch_file))
    else:
        parser.print_help()


if __name__ == "__main__":  # pragma: no cover
    main()
