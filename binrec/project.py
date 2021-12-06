import logging
import json
import os
import subprocess

from .errors import BinRecError

logger = logging.getLogger("binrec.project")

# TODO (hbrodin): Annotate with types
# TODO (hbrodin): Add comments


def _project_dir(project):
    try:
        output = subprocess.check_output(
            [
                "s2e",
                "info"
            ]
        )
    except subprocess.CalledProcessError:
        raise BinRecError(f"s2e run failed for project: {project}")
    
    d = json.loads(output)
    return d['projects'][project]['project_dir']


def _config_file(project):
    return os.path.join(_project_dir(project), "s2e-config.lua")


def _run(args):
    project = str(args.name[0])
    if not project:
      raise BinRecError("Don't know what project to run. Please specify name.")

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
    project = str(args.name[0])
    if not project:
      raise BinRecError("Please specify a non-empty name for project")

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
  

    projdir = _project_dir(project)
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


    run = subparsers.add_parser("run")
    run.add_argument("name", nargs=1, type=str, help="Name of project to run")
    # TODO (hbrodin): Enable passing of additional parameters to s2e run

    args = parser.parse_args()

    if args.current_parser == "run":
      _run(args)
    elif args.current_parser == "new":
      _new(args)
    else:
      parser.print_help()


if __name__ == "__main__":  # pragma: no cover
    main()
