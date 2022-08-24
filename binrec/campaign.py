import json
import shlex
from dataclasses import InitVar, asdict, dataclass, field
from enum import Enum
from pathlib import Path
from typing import Any, List, Optional, TextIO, Union

from .env import (
    INPUT_FILES_DIRNAME,
    TRACE_CONFIG_FILENAME,
    campaign_filename,
    input_files_dir,
    project_binary_filename,
    project_dir,
    trace_config_filename,
)

GET_TRACE_INPUT_FILES_FUNCTION = "get_trace_input_files"
SETUP_FUNCTION = "setup_trace"
TEARDOWN_FUNCTION = "teardown_trace"

#: The bash variable name for the trace concrete arguments
TRACE_CONCRETE_ARGS_VAR = "TRACE_ARGS"

#: The bash variable name for the trace symbolic arguments
TRACE_SYMBOLIC_ARGS_VAR = "S2E_SYM_ARGS"

#: The prefix in ``bootstrap.sh`` for executing the sample
BOOTSTRAP_EXECUTE_SAMPLE_PREFIX = (
    f'{TRACE_SYMBOLIC_ARGS_VAR}="" LD_PRELOAD="${{S2E_SO}}" "${{TARGET}}"'
)
#: The prefix in ``bootstrap.sh`` for calling the ``execute`` function
BOOTSTRAP_CALL_EXECUTE_PREFIX = 'execute "${TARGET_PATH}"'

#: Patched marker comment to signify that the bootstrap.sh script has been patched
BOOTSTRAP_PATCH_MARKER = (
    "# ~=~=~=~= This bootstrap.sh script has been patched by binrec. =~=~=~=~ #"
)

#: Patched bash script to get and load the trace config
BOOTSTRAP_PATCH_LOAD_TRACE_CONFIG = f"""
# binrec patch #
${{S2EGET}} {TRACE_CONFIG_FILENAME}
chmod 755 {TRACE_CONFIG_FILENAME}
source ./{TRACE_CONFIG_FILENAME}
{GET_TRACE_INPUT_FILES_FUNCTION}
################
"""

#: Patched bash script to call the ``execute`` function
BOOTSTRAP_PATCH_CALL_EXECUTE = f"""
# binrec patch #
{SETUP_FUNCTION}
execute "${{TARGET_PATH}}"
{TEARDOWN_FUNCTION}
################
"""

#: Patched bash script to execute the sample
BOOTSTRAP_PATCH_EXECUTE_SAMPLE = f"""
    # binrec patch #
    LD_PRELOAD="${{S2E_SO}}" "${{TARGET}}" "${{{TRACE_CONCRETE_ARGS_VAR}[@]}}" > /dev/null 2> /dev/null
    ################
"""  # noqa: E501

#: The default value for symbolic arguments
DEFAULT_SYMBOLIC_ARG_VALUE = "0"


def _validate_file_permission(permissions: str) -> None:
    """
    Validate that the given permission string is a valid 3 digit octal number that is
    compatible with chmod.

    :raises ValueError: the permission string is not valid
    """
    try:
        int(permissions, base=8)
    except ValueError:
        raise ValueError("invalid permissions: must be valid 3 digit octal number")

    if len(permissions) != 3:
        raise ValueError("invalid permissions: must be valid 3 digit octal number")


class CampaignJsonEncoder(json.JSONEncoder):
    """
    Provides JSON encoding for :class:`Campaign` objects.
    """

    def default(self, obj: Any) -> str:
        if isinstance(obj, Enum):
            return obj.value
        if isinstance(obj, Path):
            return str(obj)
        return super().default(obj)


class TraceArgType(Enum):
    """
    A trace argument type: symbolic or concrete.
    """

    symbolic = "symbolic"
    concrete = "concrete"


@dataclass
class TraceArg:
    """
    A single trace argument containing argument type and the value.
    """

    arg_type: TraceArgType = TraceArgType.concrete
    value: str = ""

    @property
    def is_symbolic(self) -> bool:
        """
        :returns: the argument is symbolic
        """
        return self.arg_type is TraceArgType.symbolic

    @property
    def is_concrete(self) -> bool:
        """
        :returns: the argument is concrete
        """
        return self.arg_type is TraceArgType.concrete

    @property
    def concrete_value(self) -> str:
        """
        :returns: the concrete value that will be used in the command line invocation of
            the sample
        """
        if self.is_symbolic:
            return self.value or DEFAULT_SYMBOLIC_ARG_VALUE
        return self.value

    @classmethod
    def load_dict(cls, obj: dict) -> "TraceArg":
        """
        Load the trace argument from a dictionary.

        :param obj: trace argument object
        :returns: the parsed argument
        """
        return cls(
            arg_type=TraceArgType(obj.get("arg_type") or "concrete"),
            value=str(obj.get("value") or ""),
        )


@dataclass
class TraceInputFile:
    """
    A binrec trace input file used during analysis. This class can be used to copy
    files into the analysis VM, such as configuration files or setup/teardown scripts.
    Input files are copied into the analysis VM to the "./input_files" directory,
    relative to the binary and the CWD.
    """

    #: The source file on the host OS
    source: Path
    #: The destination filename within the analysis VM. The analysis current working
    #: directory is used if not specified.
    destination: Optional[Path] = None
    #: The file's permissions within the analysis VM. This can be either ``True`` to
    #: copy the source file's permissions or a string containing the chmod-compatible
    #: 3 digit octal permissions to set.
    permissions: Union[bool, str] = True

    def check_source(self, resolve_root: Path = None) -> None:
        """
        Verify that the source file can be opened for reading. Optionally, if
        ``resolve_root`` is provided, resolve the source to an absolute path. This
        method should be called with a root directory to resolve relative paths against
        prior to using the input file within a trace.

        :param resolve_root: root path to resolve the relative source path against
        :raises OSError: source file does not exist or cannot be read
        """
        if resolve_root and not self.source.is_absolute():
            self.source = (resolve_root / self.source).absolute()

        with open(self.source, "r") as _:
            # test access, this will raise a proper OSError if we cannot read the file
            pass

    @classmethod
    def load_dict(cls, item: dict) -> "TraceInputFile":
        source = Path(item["source"])
        dest = item.get("destination")
        if dest and dest not in (".", "./"):
            dest_path = Path(dest)
        else:
            dest_path = None

        permissions = item.get("permissions", True)
        return cls(source, dest_path, permissions)

    def get_file_script(
        self, s2e_get_var: str = "${S2EGET}", indent: str = None
    ) -> str:
        """
        Returns a bash script block that copies the file into the analysis VM using
        s2eget. Optionally honors the ``preserve_permissions`` attribute to copy the
        source file's permissions and moves the file to the specified ``destination``.

        :param s2e_get_var: the variable or file path to the ``s2eget`` program
        :param indent: the amount of whitespace to indent each line in the returned
            bash script
        :returns: the bash script block for this input file
        """
        indent = indent or ""
        safe_source = shlex.quote(self.source.name)
        if self.destination:
            safe_dest = shlex.quote(str(self.destination))
        else:
            safe_dest = None

        lines = [f"{indent}{s2e_get_var} {safe_source}"]
        if self.permissions is True:
            perm = self.source.stat().st_mode & 0o777
            mode = f"{perm:o}"
        elif isinstance(self.permissions, str):
            # verify this is a valid 3 digit octal number representing permissions that
            # are compatible with chmod
            _validate_file_permission(self.permissions)
            mode = self.permissions
        else:
            mode = None

        if mode:
            lines.append(f"{indent}chmod {mode} {safe_source}")

        if safe_dest:
            lines.append(f"{indent}mv {safe_source} {safe_dest}")

        return "\n".join(lines)


@dataclass
class TraceParams:
    """
    The parameters for running a single trace within a campaign. Each trace has a set of
    arguments, input files, setup steps, teardown steps, and information on how to
    validate the lifted result against the original.
    """

    args: List[TraceArg] = field(default_factory=list)
    stdin: Union[bool, str] = False
    match_stdout: Union[bool, str] = True
    match_stderr: Union[bool, str] = True
    input_files: List[TraceInputFile] = field(default_factory=list)
    setup: List[str] = field(default_factory=list)
    teardown: List[str] = field(default_factory=list)
    name: Optional[str] = None

    @property
    def symbolic_indexes(self) -> List[int]:
        """
        :returns: the list of argument indexes that are symbolic
        """
        return [i for i, arg in enumerate(self.args, start=1) if arg.is_symbolic]

    @property
    def command_line_args(self) -> List[str]:
        """
        :returns: the command line arguments (symbolic and concrete)
        """
        return [arg.concrete_value for arg in self.args]

    def write_config_script(self, project: str) -> None:
        """
        Write the trace params to the binrec trace config script. The config script
        relies on a patched ``bootstrap.sh`` script (see :func:`patch_s2e_project`).

        :param project: project name
        """
        filename = trace_config_filename(project)

        with open(filename, "w") as fp:
            print("#!/bin/bash", file=fp)
            self._write_variables(fp)
            self._write_get_input_files_function(fp)
            self._write_setup_function(fp)
            self._write_teardown_function(fp)

    def _write_variables(self, file: TextIO) -> None:
        """
        Write the bash config variables used by binrec and S2E when executing the
        sample.
        """
        symbolic_indexes = shlex.quote(
            " ".join([str(i) for i in self.symbolic_indexes])
        )
        args = " ".join(shlex.quote(arg) for arg in self.command_line_args)
        print(f"export {TRACE_SYMBOLIC_ARGS_VAR}={symbolic_indexes}", file=file)
        print(f"export {TRACE_CONCRETE_ARGS_VAR}=({args})", file=file)
        print(file=file)

    def _write_get_input_files_function(self, file: TextIO) -> None:
        """
        Write the bash function to download all input files into the analysis VM and
        store them in the ``./input_files`` directory.
        """
        print(f"function {GET_TRACE_INPUT_FILES_FUNCTION}() {{", file=file)
        print(f"  mkdir ./{INPUT_FILES_DIRNAME}", file=file)
        print(f"  cd ./{INPUT_FILES_DIRNAME}", file=file)
        for file_input in self.input_files:
            print(
                file_input.get_file_script(s2e_get_var="../${S2EGET}", indent="  "),
                file=file,
            )

        print("  cd ..", file=file)
        print("}", file=file)
        print(file=file)

    def _write_setup_function(self, file: TextIO) -> None:
        """
        Write the bash function that executes every setup action.
        """
        print(f"function {SETUP_FUNCTION}() {{", file=file)
        for action in self.setup:
            print(f"  {action}", file=file)

        # This function could be empty if no setup actions are being performed. Add a
        # return statement to make sure bash is happy
        print("  return", file=file)
        print("}", file=file)
        print(file=file)

    def _write_teardown_function(self, file: TextIO) -> None:
        """
        Write the bash function that executes every teardown action.
        """
        print(f"function {TEARDOWN_FUNCTION}() {{", file=file)
        for action in self.teardown:
            print(f"  {action}", file=file)

        # This function could be empty if no setup actions are being performed. Add a
        # return statement to make sure bash is happy
        print("  return", file=file)
        print("}", file=file)
        print(file=file)

    @classmethod
    def load_dict(cls, item: dict) -> "TraceParams":
        """
        Load the trace parameters from a dictionary.

        :param item: trace parameters object
        :returns: the parsed trace parameters
        :raises ValueError: the trace parameters object is invalid and could not be
            parsed
        """
        args = item.get("args") or []
        stdin = item.get("stdin") or False
        match_stdout = item.get("match_stdout", True)
        match_stderr = item.get("match_stderr", True)
        files = item.get("input_files") or []
        setup = item.get("setup") or []
        teardown = item.get("teardown") or []

        if not isinstance(files, list):
            raise ValueError("the 'files' entry must be an array")

        input_files = []
        for fn in files:
            if isinstance(fn, str):
                input_files.append(TraceInputFile(Path(fn)))
            elif isinstance(fn, dict):
                input_files.append(TraceInputFile.load_dict(fn))
            else:
                raise ValueError("items in 'files' array must be a string or object")

        trace_args = []
        for arg in args:
            if isinstance(arg, str):
                trace_arg = TraceArg(TraceArgType.concrete, arg)
            else:
                trace_arg = TraceArg.load_dict(arg)

            trace_args.append(trace_arg)

        return cls(
            args=trace_args,
            stdin=stdin,
            match_stdout=match_stdout,
            match_stderr=match_stderr,
            input_files=input_files,
            setup=setup,
            teardown=teardown,
            name=item.get("name"),
        )

    def setup_input_file_directory(self, project: str, cleanup: bool = True) -> None:
        """
        Setup the project's trace file input directory. This method will link all
        ``input_files`` to the provided project's file input directory. The ``cleanup``
        parameter, when specified, will first delete all symlinks within the directory
        that may be present from a previous trace.

        :param project: the project name
        :param cleanup: remove all symlinks from the file input directory prior to
            linking this trace's file inputs
        """
        dirname = input_files_dir(project)
        if not dirname.is_dir():
            dirname.mkdir()

        if cleanup:
            for child in dirname.iterdir():
                if child.is_symlink() and child.is_file():
                    child.unlink()

        for item in self.input_files:
            if item.source.is_file():
                link = dirname / item.source.name
                link.symlink_to(item.source)

    @classmethod
    def create_trace_args(
        cls, args: List[str], symbolic_indexes: List[int]
    ) -> List[TraceArg]:
        """
        Create a list of trace arguments from a list of argument values and a list of
        symbolic argument indexes. For example:

        .. code-block:: python

            TraceParams.create_trace_args(["first", "second"], [2]) == [
                TraceArg(TraceArgType.concrete, "first),
                TraceArg(TraceArgType.symbolic, "second")
            ]

        :param args: concrete argument values
        :param symbolic_indexes: symbolic argument indexes (1-based)
        :returns: the list of ``TraceArg`` objects
        """
        trace_args = []
        for i, value in enumerate(args, start=1):
            arg_type = (
                TraceArgType.symbolic
                if i in symbolic_indexes
                else TraceArgType.concrete
            )
            trace_args.append(TraceArg(arg_type, value))

        for i in symbolic_indexes:
            if i > len(trace_args) or i < 0:
                raise IndexError(f"symbolic argument out of bounds: {i}")

        return trace_args


@dataclass
class Campaign:
    """
    Parameters for executing multiple traces of a single analysis sample. This class
    can created programmatically or loaded from a JSON file.

    Each campaign targets a single sample binary, ``binary``, and a list of trace
    parameters, ``traces``.
    """

    binary: Path
    traces: List[TraceParams] = field(default_factory=list)
    project: InitVar[str] = None
    setup: List[str] = field(default_factory=list)
    teardown: List[str] = field(default_factory=list)

    def __post_init__(self, project: str = None):
        self.project = project or (self.binary.name if self.binary else "")

    def save(self, file: Union[str, Path, TextIO] = None) -> None:
        """
        Save the campaign to disk. The ``file`` argument can be one of:

        1. ``str | Path`` - save the campaign to the provided path
        2. ``TextIO`` - save the campaign to the provide open file
        3. ``None`` (default) - save the campaign to the default campaign path for the
           project (see :func:`~binrec.env.campaign_filename`)

        :param file: the destination file path or file object
        """
        if file is None:
            file = campaign_filename(self.project)

        if isinstance(file, (str, Path)):
            fp: TextIO = open(file, "w")
        else:
            fp = file

        body = asdict(self)
        # remove "binary" from the JSON since we want the campaign to be reusable by
        # being decoupled from the S2E project
        body.pop("binary", None)
        fp.write(json.dumps(body, indent=2, cls=CampaignJsonEncoder))

    def remove_trace(self, name_or_id: Union[str, int]) -> None:
        """
        Remove a trace by name or id. The trace id is the index of the trace within the
        ``traces`` list. The trace id can be negative, such as ``-1`` to remove the last
        trace in the list.

        :param name_or_id: the trace name (``str``) or id (``int``) to remove
        :raises KeyError: the trace name does not exist within the campaign
        :raises IndexError: the trace id does not exist within the campaign
        """
        if isinstance(name_or_id, int):
            if abs(name_or_id) >= len(self.traces):
                raise IndexError(f"invalid trace index: {name_or_id}")
            self.traces.pop(name_or_id)
            return

        for i, trace in enumerate(self.traces):
            if trace.name == name_or_id:
                found = i
                break
        else:
            raise KeyError(f"trace does not exist: {name_or_id}")

        self.traces.pop(found)

    def get_trace(self, name_or_id: Union[str, int]) -> TraceParams:
        """
        Get a trace by name or id. The trace id is the index of the trace within the
        ``traces`` list. The trace id can be negative, such as ``-1`` to get the last
        trace in the list.

        :param name_or_id: the trace name (``str``) or id (``int``) to get
        :raises KeyError: the trace name does not exist within the campaign
        :raises IndexError: the trace id does not exist within the campaign
        """
        if isinstance(name_or_id, int):
            if abs(name_or_id) >= len(self.traces):
                raise IndexError(f"invalid trace index: {name_or_id}")
            return self.traces[name_or_id]

        for trace in self.traces:
            if trace.name == name_or_id:
                return trace

        raise KeyError(f"trace does not exist: {name_or_id}")

    @classmethod
    def load_project(cls, project_name: str, **kwargs) -> "Campaign":
        """
        Load the campaign for a project.

        :param project_name: the project
        :param kwargs: additional arguments to pass to :meth:`load_json`
        :returns: the parsed campaign object
        """
        return cls.load_json(
            project_binary_filename(project_name),
            campaign_filename(project_name),
            **kwargs,
        )

    @classmethod
    def load_json(
        cls, binary: Path, filename: Path, resolve_input_files: bool = True, **kwargs
    ) -> "Campaign":
        """
        Load the campaign from a JSON file.

        :param binary: the sample binary
        :param filename: JSON filename
        :param resolve_input_files: for each loaded :class:`TraceInputFile` resolve
            relative filenames against the parent directory of the source JSON file,
            ``filename`` (e.g.- ``input_file.check_source(filename.parent))``). See
            :meth:`TraceInputFile.check_source` for more information.
        """
        with open(filename, "r") as file:
            body = json.loads(file.read().strip())

        campaign = Campaign(
            binary,
            [TraceParams.load_dict(item) for item in body["traces"]],
            setup=body.get("setup") or [],
            teardown=body.get("teardown") or [],
            **kwargs,
        )

        if resolve_input_files:  # TODO unit test this
            for trace in campaign.traces:
                for input_file in trace.input_files:
                    input_file.check_source(filename.parent)

        return campaign


def patch_s2e_project(project: str, existing_patch_ok: bool = False) -> bool:
    """
    Patch the s2e ``bootstrap.sh`` script so that it loads the binrec trace config,
    which sets the symbolic and concrete arguments. Once patched, the
    :class:`TraceParams` can be used to change how the next trace will execute.

    :param project: s2e project name
    :param existing_patch_ok: do not raise an exception if the ``boostrap.sh`` has
        already been patched
    :returns: ``True`` if the file was patched, ``False`` if not. The return value is
        only meaningful when ``existing_patch_ok=True``.
    :raises ValueError: the ``bootstrap.sh`` script has already been patched or cannot
        be patched
    """
    # read the entire bootstrap script
    bootstrap_path = project_dir(project) / "bootstrap.sh"
    with open(bootstrap_path, "r") as file:
        lines = file.readlines()

    #
    # we need to find 2 locations in the script to patch:
    #
    #   1. Where the sample is actually executed (BOOTSTRAP_EXECUTE_SAMPLE_PREFIX)
    #   2. Where the "execute" function is called (BOOTSTRAP_CALL_EXECUTE_PREFIX)
    #
    execute_sample_index = -1
    execute_call_index = -1
    for i, line in enumerate(lines):
        check = line.lstrip()
        if check.startswith(BOOTSTRAP_EXECUTE_SAMPLE_PREFIX):
            execute_sample_index = i
        elif check.startswith(BOOTSTRAP_CALL_EXECUTE_PREFIX):
            execute_call_index = i
        elif check.startswith(BOOTSTRAP_PATCH_MARKER):
            if existing_patch_ok:
                return False

            raise ValueError(
                f"project bootstrap script has already been patched: "
                f"{bootstrap_path}"
            )

    # verify that we found the couple locations we need
    if execute_sample_index < 0:
        raise ValueError("project bootstrap does not contain call to execute sample")
    elif execute_call_index < 0:
        raise ValueError("project bootstrap does not contain call to execute function")

    # patch the bootstrap script
    with open(bootstrap_path, "w") as file:
        for i, line in enumerate(lines):
            if i == execute_sample_index:
                # Update the sample execution to remove the symbolic argumnets, since
                # this is now stored in the environment variable
                file.write(BOOTSTRAP_PATCH_EXECUTE_SAMPLE)
            elif i == execute_call_index:
                # Update the call to the "execute" function to load the binrec trace
                # config first and then call the function without any arguments, since
                # these are now stored in environment variables
                file.write(BOOTSTRAP_PATCH_LOAD_TRACE_CONFIG)
                file.write(BOOTSTRAP_PATCH_CALL_EXECUTE)
            else:
                # write the line as-is
                file.write(line)

        file.write(f"\n{BOOTSTRAP_PATCH_MARKER}\n")

    return True
