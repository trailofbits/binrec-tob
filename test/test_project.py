from pathlib import Path
import subprocess
from unittest.mock import patch, mock_open, call, MagicMock

import pytest

from binrec import project
from binrec.env import BINREC_PROJECTS
from binrec.errors import BinRecError
from helpers.mock_path import MockPath


BOOTSTRAP_WITH_EXECUTE_AND_SYM = '''
line1
line2

    S2E_SYM_ARGS="1" LD_PRELOAD="${S2E_SO}" "${TARGET}" "$@" > /dev/null 2> /dev/null

execute "${TARGET_PATH}" existing args
'''

BOOTSTRAP_NO_ARGS = '''
line1
line2

    S2E_SYM_ARGS="" LD_PRELOAD="${S2E_SO}" "${TARGET}" "$@" > /dev/null 2> /dev/null

execute "${TARGET_PATH}"\x20
'''

BOOTSTRAP_WITH_EXECUTE = '''
line1
line2

execute "${TARGET_PATH}" existing args
'''

BOOTSTRAP_NO_EXECUTE = '''
line1
line2

    S2E_SYM_ARGS="1" LD_PRELOAD="${S2E_SO}" "${TARGET}" "$@" > /dev/null 2> /dev/null
'''

BOOTSTRAP_EXPECTED = '''
line1
line2

    S2E_SYM_ARGS="" LD_PRELOAD="${S2E_SO}" "${TARGET}" "$@" > /dev/null 2> /dev/null

execute "${TARGET_PATH}" \'hello world\' goodbye
'''

BATCH_WITH_ARGS = '''Jack Jill
0 1 2
Jack 0 Jill 1 2'''

BATCH_WITHOUT_ARGS = ""


class TestProject:

    @patch.object(project, "Campaign")
    @patch.object(project, "_validate_campaign_trace")
    def test_validate_campaign(self, mock_validate_lift_result, mock_params_cls):
        binary = BINREC_PROJECTS / "asdf" / "s2e-out" / "binary"
        c = mock_params_cls.load_project.return_value = MagicMock(traces=[1, 2, 3], project="asdf")

        project.validate_campaign("asdf")

        mock_params_cls.load_project.assert_called_once_with("asdf")
        calls = [call(c, 1), call(c, 2), call(c, 3)]
        mock_validate_lift_result.assert_has_calls(calls, any_order=False)

    @patch.object(project, "subprocess")
    def test_run_trace_setup(self, mock_subproc):
        trace = MagicMock(setup=['asdf', 'qwer'])
        project._run_trace_setup(MagicMock(), trace, Path("path"))
        mock_subproc.run.assert_called_once_with(["/bin/bash", "--noprofile"], input=b"asdf\nqwer", cwd="path")

    @patch.object(project, "subprocess")
    def test_run_trace_setup_empty(self, mock_subproc):
        trace = MagicMock(setup=[])
        project._run_trace_setup(MagicMock(setup=[]), trace, Path("path"))
        mock_subproc.run.assert_not_called()

    @patch.object(project, "subprocess")
    def test_run_trace_teardown(self, mock_subproc):
        trace = MagicMock(teardown=['asdf', 'qwer'])
        project._run_trace_teardown(MagicMock(), trace, Path("path"))
        mock_subproc.run.assert_called_once_with(["/bin/bash", "--noprofile"], input=b"asdf\nqwer", cwd="path")

    @patch.object(project, "subprocess")
    def test_run_trace_teardown_empty(self, mock_subproc):
        trace = MagicMock(teardown=[])
        project._run_trace_teardown(MagicMock(teardown=[]), trace, Path("path"))
        mock_subproc.run.assert_not_called()

    @patch.object(project.subprocess, "check_call")
    def test_run_campaign_trace(self, mock_check_call):
        c = MagicMock()
        trace = MagicMock()
        project._run_campaign_trace(c, trace)
        trace.setup_input_file_directory.assert_called_once_with(c.project)
        trace.write_config_script.assert_called_once_with(c.project)
        mock_check_call.assert_called_once_with(["s2e", "run", "--no-tui", c.project])

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
