import logging
import json
import os
import subprocess
from pathlib import Path
from typing import Iterable, List

from .errors import BinRecError

logger = logging.getLogger("binrec.project")

# TODO (hbrodin): Annotate with types
# TODO (hbrodin): Add comments


def project_dir(project : str) -> Path:
    try:
        output = subprocess.check_output(
            [
                "s2e",
                "info"
            ]
        )
    except subprocess.CalledProcessError:
        raise BinRecError(f"s2e failed to get info for: {project}")
    
    d = json.loads(output)
    return d['projects'][project]['project_dir']

def listing() ->List[str]:
    try:
        output = subprocess.check_output(
            [
                "s2e",
                "info"
            ]
        )
    except subprocess.CalledProcessError:
        raise BinRecError(f"s2e run failed to get project list")
    
    d = json.loads(output)
    return d['projects'].keys()

def _get_project_name(args) -> str:
    project = str(args.name[0])
    if not project:
        raise BinRecError("Please specify project name.")
    return project

def _list() -> None:
  for proj in listing():
    print(proj)

def traces(project: str) -> Iterable[Path]:
    projdir = project_dir(project)
    return filter(os.path.isdir,
            map(lambda x: os.path.join(projdir, x), 
              filter(lambda x: x.startswith("s2e-out-"), os.listdir(projdir))))

def _list_traces(args):
    proj = _get_project_name(args)
    for t in traces(proj):
      print(t)


def _config_file(project : str) -> Path:
    return os.path.join(project_dir(project), "s2e-config.lua")


def _run(args):
    project = _get_project_name(args)

    logger.info("Running project: %s", project)
    try:
        subprocess.check_call(
            [
                "s2e",
                "run",
                "--no-tui",
                project
            ]
        )
    except subprocess.CalledProcessError:
        raise BinRecError(f"s2e run failed for project: {project}")
    
def _new(args):
    project = _get_project_name(args)

    logger.info("Creating project: %s", project)
    callargs = [
                "s2e",
                "new_project",
                "--name",
                project
            ]
    if args.sym_args:
      callargs.extend(["--sym-args", args.sym_args])
    callargs.append(args.bin)
    callargs.extend(args.args)

    try:
        subprocess.check_call(callargs)
    except subprocess.CalledProcessError:
        raise BinRecError(f"s2e run failed for project: {project}")
  

    projdir = project_dir(project)
    cfgfile = _config_file(project)

    with open(cfgfile, 'a') as f:
      f.write(f"""
add_plugin(\"ELFSelector\")
add_plugin(\"FunctionMonitor\")
add_plugin(\"FunctionLog\")
pluginsConfig.FunctionLog = {{
    logFile = 'function-log.txt'
}}
add_plugin(\"ExportELF\")
pluginsConfig.ExportELF = {{
    baseDirs = {{
        "{projdir}"
    }},
    exportInterval = 1000 -- export every 1000 basic blocks
}}
""")

def main() -> None:
    import argparse
    import sys

    from .core import init_binrec

    init_binrec()

    parser = argparse.ArgumentParser()
    subparsers  = parser.add_subparsers(dest="current_parser")

    new_proj = subparsers.add_parser("new")
    new_proj.add_argument("name", nargs=1, help='Name of new analysis project')
    new_proj.add_argument("--bin", type=str, required=True, help='Path to binary used in analysis')
    new_proj.add_argument("--sym-args", type=str, help='Symbolic args to pass to binary. E.g. "1 3" ensures makes arguments one and three symbolic.')
    new_proj.add_argument("args", nargs='*', type=str, help='Arguments to pass to binary. Use @@ to indicate a file with symbolic content')

    listcmd = subparsers.add_parser("list")
    listtracecmd = subparsers.add_parser("list-traces")
    listtracecmd.add_argument("name", nargs=1, type=str, help="Name of project to run")

    run = subparsers.add_parser("run")
    run.add_argument("name", nargs=1, type=str, help="Name of project to run")
    # TODO (hbrodin): Enable passing of additional parameters to s2e run

    args = parser.parse_args()

    if args.current_parser == "run":
      _run(args)
    elif args.current_parser == "new":
      _new(args)
    elif args.current_parser == "list":
      _list()
    elif args.current_parser == "list-traces":
      _list_traces(args)
    else:
      parser.print_help()


if __name__ == "__main__":  # pragma: no cover
    main()
