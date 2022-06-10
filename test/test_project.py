from re import M
import subprocess
import json
from unittest.mock import patch, mock_open, call, MagicMock

import pytest

from binrec import project
from binrec.env import BINREC_PROJECTS
from binrec.errors import BinRecError
from helpers.mock_path import MockPath


BOOTSTRAP_WITH_EXECUTE_AND_SYM = """
line1
line2

    S2E_SYM_ARGS="1" LD_PRELOAD="${S2E_SO}" "${TARGET}" "$@" > /dev/null 2> /dev/null

execute "${TARGET_PATH}" existing args
"""

BOOTSTRAP_NO_ARGS = """
line1
line2

    S2E_SYM_ARGS="" LD_PRELOAD="${S2E_SO}" "${TARGET}" "$@" > /dev/null 2> /dev/null

execute "${TARGET_PATH}"\x20
"""

BOOTSTRAP_WITH_EXECUTE = """
line1
line2

execute "${TARGET_PATH}" existing args
"""

BOOTSTRAP_NO_EXECUTE = """
line1
line2

    S2E_SYM_ARGS="1" LD_PRELOAD="${S2E_SO}" "${TARGET}" "$@" > /dev/null 2> /dev/null
"""

BOOTSTRAP_EXPECTED = """
line1
line2

    S2E_SYM_ARGS="" LD_PRELOAD="${S2E_SO}" "${TARGET}" "$@" > /dev/null 2> /dev/null

execute "${TARGET_PATH}" \'hello world\' goodbye
"""

BATCH_WITH_ARGS = """Jack Jill
0 1 2
Jack 0 Jill 1 2"""

BATCH_WITHOUT_ARGS = ""


class TestProject:
    @patch.object(project, "TraceParams")
    def test_set_project_args(self, mock_params_cls):
        pass

    @patch.object(project, "TraceParams")
    def test_set_project_args_no_args(self, mock_params_cls):
        pass

    @patch.object(project, "TraceParams")
    def test_set_project_args_empty_quote_args0(self, mock_params_cls):
        pass

    @patch.object(project, "BatchTraceParams")
    @patch.object(project, "validate_lift_result")
    def test_validate_batch_project(self, mock_validate_lift_result, mock_params_cls):
        binary = BINREC_PROJECTS / "asdf" / "s2e-out" / "binary"
        mock_params_cls.load.return_value = MagicMock(traces=[1, 2, 3], project="asdf")

        project.validate_lift_result_batch_file("asdf", "./myfile.txt")

        mock_params_cls.load.assert_called_once_with(
            binary, "./myfile.txt", project="asdf"
        )
        calls = [call("asdf", 1), call("asdf", 2), call("asdf", 3)]
        mock_validate_lift_result.assert_has_calls(calls, any_order=False)

    @patch.object(project.subprocess, "check_call")
    def test_run_project(self, mock_check_call):
        project.run_project("asdf", None)
        mock_check_call.assert_called_once_with(["s2e", "run", "--no-tui", "asdf"])

    @patch.object(project.subprocess, "check_call")
    @patch.object(project, "set_project_args")
    def test_run_project_args(self, mock_set_args, mock_check_call):
        args = json.dumps({"symbolic": "qwer", "concrete": []})
        project.run_project("asdf", args)
        mock_set_args.assert_called_once_with("asdf", ["qwer"])
        mock_check_call.assert_called_once_with(["s2e", "run", "--no-tui", "asdf"])

    @patch.object(project.subprocess, "check_call")
    def test_run_project_error(self, mock_check_call):
        mock_check_call.side_effect = subprocess.CalledProcessError(1, "zxcv")
        with pytest.raises(BinRecError):
            project.run_project("asdf", None)

    @patch.object(project, "BatchTraceParams")
    @patch.object(project, "run_project")
    def test_run_batch_project(self, mock_run_project, mock_params_cls):
        binary = BINREC_PROJECTS / "asdf" / "binary"
        invocations = [line.strip().split() for line in BATCH_WITH_ARGS.split("\n")]
        params = mock_params_cls.load.return_value = MagicMock(
            traces=[MagicMock(args=args) for args in invocations], project="asdf"
        )
        project.run_project_batch_file("asdf", "./myfile.txt")
        mock_params_cls.load.assert_called_once_with(
            binary, "./myfile.txt", project="asdf"
        )

        for trace in params.traces:
            trace.setup_input_file_directory.assert_called_once_with("asdf")
            trace.write_config_script.assert_called_once_with("asdf")

        calls = [call("asdf", None), call("asdf", None), call("asdf", None)]
        mock_run_project.assert_has_calls(calls, any_order=False)

    @patch.object(project, "project_dir")
    @patch.object(project, "input_files_dir")
    @patch.object(project.subprocess, "check_call")
    @patch.object(project, "open", new_callable=mock_open)
    @patch.object(project, "set_project_args")
    @patch.object(project, "patch_s2e_project")
    def test_new_project(
        self,
        mock_patch,
        mock_set_args,
        mock_file,
        mock_check_call,
        mock_input_files_dir,
        mock_project_dir,
    ):
        project_dir = mock_project_dir.return_value = MockPath("/asdf", exists=False)
        input_files_dir = mock_input_files_dir.return_value = MockPath(
            "/asdf/input_files", exists=False
        )
        assert project.new_project("asdf", MockPath("/binary")) is project_dir
        mock_check_call.assert_called_once_with(
            ["s2e", "new_project", "--name", "asdf", "/binary"]
        )
        mock_file.assert_called_once_with(project_dir / "s2e-config.lua", "a")
        handle = mock_file()
        handle.write.assert_called_once_with(
            f"""
add_plugin(\"ELFSelector\")
add_plugin(\"FunctionMonitor\")
add_plugin(\"FunctionLog\")
add_plugin(\"ExportELF\")
pluginsConfig.ExportELF = {{
    baseDirs = {{
        "{project_dir}"
    }},
    exportInterval = 1000 -- export every 1000 basic blocks
}}

table.insert(pluginsConfig.HostFiles.baseDirs, "{input_files_dir}")
"""
        )
        mock_patch.assert_called_once_with("asdf")
        mock_set_args.assert_called_once_with("asdf", [""])
        mock_input_files_dir.assert_called_once_with("asdf")
        input_files_dir.mkdir.assert_called_once()
        (project_dir / "binary").symlink_to.assert_called_once_with(
            project_dir / "binary"
        )

    @patch.object(project, "project_dir")
    @patch.object(project, "input_files_dir")
    @patch.object(project.subprocess, "check_call")
    @patch.object(project, "open", new_callable=mock_open)
    @patch.object(project, "set_project_args")
    @patch.object(project, "patch_s2e_project")
    def test_new_project_args(
        self,
        mock_patch,
        mock_set_args,
        mock_file,
        mock_check_call,
        mock_input_files_dir,
        mock_project_dir,
    ):
        mock_project_dir.return_value = MockPath("/asdf", exists=False)
        mock_input_files_dir.return_value = MockPath("/asdf/input_files", exists=False)
        project.new_project("asdf", MockPath("/binary"), "1 2", ["one", "two"])
        mock_check_call.assert_called_once_with(
            ["s2e", "new_project", "--name", "asdf", "/binary"]
        )
        mock_patch.assert_called_once_with("asdf")
        mock_set_args.assert_called_once_with("asdf", ["1 2", "one", "two"])

    @patch.object(project, "project_dir")
    @patch.object(project.subprocess, "check_call")
    def test_new_project_error(self, mock_check_call, mock_project_dir):
        mock_project_dir.return_value = MockPath("/asdf", exists=False)
        mock_check_call.side_effect = subprocess.CalledProcessError(1, "asdf")
        with pytest.raises(BinRecError):
            project.new_project("asdf", "/binary")

    @patch.object(project, "project_dir")
    def test_new_project_exists(self, mock_project_dir):
        mock_project_dir.return_value = MockPath("/asdf", is_dir=True)
        with pytest.raises(FileExistsError):
            project.new_project("asdf", "/binary")
