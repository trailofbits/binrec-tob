import subprocess
from unittest.mock import patch, mock_open, call

import pytest

from binrec import project
from binrec.env import BINREC_PROJECTS
from binrec.errors import BinRecError
from helpers.mock_path import MockPath


BOOTSTRAP_WITH_EXECUTE = '''
line1
line2

execute "${TARGET_PATH}" existing args
'''

BOOTSTRAP_NO_EXECUTE = '''
line1
line2'''

BATCH_WITH_ARGS = '''Jack Jill
0 1 2
Jack 0 Jill 1 2'''

BATCH_WITHOUT_ARGS = ""


class TestProject:

    def test_project_dir(self):
        assert project.project_dir("asdf") == BINREC_PROJECTS / "asdf"

    def test_merged_trace_dir(self):
        assert project.merged_trace_dir("asdf") == BINREC_PROJECTS / "asdf" / "s2e-out"

    def test_trace_dir(self):
        assert project.trace_dir("asdf", 100) == BINREC_PROJECTS / "asdf" / "s2e-out-100"

    @patch.object(project, "project_dir")
    def test_get_trace_dirs(self, mock_project_dir):
        root = mock_project_dir.return_value = MockPath("asdf")
        file = root / MockPath("s2e-out-1", exists=True)
        not_trace_dir = root / MockPath("asdf", is_dir=True)
        trace_dir_1 = root / MockPath("s2e-out-2", is_dir=True)
        trace_dir_2 = root / MockPath("s2e-out-3", is_dir=True)

        assert project.get_trace_dirs("asdf") == [trace_dir_1, trace_dir_2]
        mock_project_dir.assert_called_once_with("asdf")

    @patch.object(project, "open", new_callable=mock_open,
                  read_data=BATCH_WITH_ARGS)
    def test_parse_batch_file(self, mock_file):
        invocations = project.parse_batch_file("myfile.txt")

        assert mock_file.call_args_list == [ call("myfile.txt", "r")]
        assert invocations == [ ["Jack", "Jill"], ["0", "1", "2"],
                                ["Jack", "0", "Jill", "1", "2"] ]

    @patch.object(project, "open", new_callable=mock_open,
                  read_data=BATCH_WITHOUT_ARGS)
    def test_parse_batch_file_no_args(self, mock_file):
        invocations = project.parse_batch_file("myfile.txt")
        assert mock_file.call_args_list == [ call("myfile.txt", "r")]
        assert invocations == [[]]

    @patch.object(project, "open", new_callable=mock_open,
                  read_data=BOOTSTRAP_WITH_EXECUTE)
    def test_set_project_args(self, mock_file):
        bootstrap = BINREC_PROJECTS / "my_project" / "bootstrap.sh"
        project.set_project_args("my_project", ["hello world", "goodbye"])
        assert mock_file.call_args_list == [
            call(bootstrap, "r"),
            call(bootstrap, "w")
        ]

        handle = mock_file()
        handle.readlines.assert_called_once()
        handle.writelines.assert_called_once_with(
            BOOTSTRAP_WITH_EXECUTE.splitlines(keepends=True)[:-1] + [
                'execute "${TARGET_PATH}" \'hello world\' goodbye\n'
            ]
        )

    @patch.object(project, "open", new_callable=mock_open,
                  read_data=BOOTSTRAP_NO_EXECUTE)
    def test_set_project_args_no_existing_command(self, mock_file):
        bootstrap = BINREC_PROJECTS / "my_project" / "bootstrap.sh"
        project.set_project_args("my_project", ["hello world", "goodbye"])
        assert mock_file.call_args_list == [
            call(bootstrap, "r"),
            call(bootstrap, "w")
        ]

        handle = mock_file()
        handle.readlines.assert_called_once()
        handle.writelines.assert_called_once_with(
            BOOTSTRAP_NO_EXECUTE.splitlines(keepends=True) + [
                "\n",
                'execute "${TARGET_PATH}" \'hello world\' goodbye\n'
            ]
        )

    # Note this test is rather shallow since validation is really test code itself
    @patch.object(project.os, "remove")
    @patch.object(project.os, "link")
    @patch.object(project.subprocess, "Popen")
    def test_validate_project(self, mock_popen, mock_link, mock_remove):
        args = ["Jack", "Jill"]
        project.validate_project("asdf", args)
        merged_tgt = str(BINREC_PROJECTS / "asdf" / "s2e-out" / "test-target")
        mock_popen.assert_called_with([merged_tgt] + args,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    stdin=subprocess.PIPE)

    @patch.object(project, "parse_batch_file", return_value=[line.strip().split() for line in BATCH_WITH_ARGS.split("\n")])
    @patch.object(project, "validate_project")
    def test_validate_batch_project(self, mock_validate_project, mock_parse_batch_file):
        project.validate_batch_project("asdf", "./myfile.txt")
        mock_parse_batch_file.assert_called_once_with("./myfile.txt")
        calls = [call("asdf", ["Jack", "Jill"]), call("asdf", ["0", "1", "2"]), call("asdf", ["Jack", "0", "Jill", "1", "2"])]
        mock_validate_project.assert_has_calls(calls, any_order=False)

    @patch.object(project.subprocess, "check_call")
    def test_run_project(self, mock_check_call):
        project.run_project("asdf")
        mock_check_call.assert_called_once_with(["s2e", "run", "--no-tui", "asdf"])

    @patch.object(project.subprocess, "check_call")
    @patch.object(project, "set_project_args")
    def test_run_project_args(self, mock_set_args, mock_check_call):
        project.run_project("asdf", ["qwer"])
        mock_set_args.assert_called_once_with("asdf", ["qwer"])
        mock_check_call.assert_called_once_with(["s2e", "run", "--no-tui", "asdf"])

    @patch.object(project.subprocess, "check_call")
    def test_run_project_error(self, mock_check_call):
        mock_check_call.side_effect = subprocess.CalledProcessError(1, "zxcv")
        with pytest.raises(BinRecError):
            project.run_project("asdf")

    @patch.object(project, "parse_batch_file", return_value=[line.strip().split() for line in BATCH_WITH_ARGS.split("\n")])
    @patch.object(project, "run_project")
    def test_run_batch_project(self, mock_run_project, mock_parse_batch_file):
        project.run_batch_project("asdf", "./myfile.txt")
        mock_parse_batch_file.assert_called_once_with("./myfile.txt")
        calls = [call("asdf", ["Jack", "Jill"]), call("asdf", ["0", "1", "2"]), call("asdf", ["Jack", "0", "Jill", "1", "2"])]
        mock_run_project.assert_has_calls(calls, any_order=False)

    @patch.object(project, "project_dir")
    @patch.object(project.subprocess, "check_call")
    @patch.object(project, "open", new_callable=mock_open)
    def test_new_project(self, mock_file, mock_check_call, mock_project_dir):
        project_dir = mock_project_dir.return_value = MockPath("/asdf", exists=False)
        assert project.new_project("asdf", "/binary") is project_dir
        mock_check_call.assert_called_once_with(["s2e", "new_project", "--name", "asdf", "/binary"])
        mock_file.assert_called_once_with(project_dir / "s2e-config.lua", "a")
        handle = mock_file()
        handle.write.assert_called_once_with(f"""
add_plugin(\"ELFSelector\")
add_plugin(\"FunctionMonitor\")
add_plugin(\"FunctionLog\")
pluginsConfig.FunctionLog = {{
    logFile = 'function-log.txt'
}}
add_plugin(\"ExportELF\")
pluginsConfig.ExportELF = {{
    baseDirs = {{
        "{project_dir}"
    }},
    exportInterval = 1000 -- export every 1000 basic blocks
}}
"""
        )

    @patch.object(project, "project_dir")
    @patch.object(project.subprocess, "check_call")
    @patch.object(project, "open", new_callable=mock_open)
    def test_new_project_args(self, mock_file, mock_check_call, mock_project_dir):
        mock_project_dir.return_value = MockPath("/asdf", exists=False)
        project.new_project("asdf", "/binary", "1 2", ["one", "two"])
        mock_check_call.assert_called_once_with(["s2e", "new_project", "--name", "asdf", "--sym-args", "1 2", "/binary", "one", "two"])

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
