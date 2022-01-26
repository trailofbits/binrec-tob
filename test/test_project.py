
from unittest.mock import patch, mock_open, call

from binrec import project
from binrec.env import BINREC_PROJECTS
from helpers.mock_path import MockPath


BOOTSTRAP_WITH_EXECUTE = '''
line1
line2

execute "${TARGET_PATH}" existing args
'''

BOOTSTRAP_NO_EXECUTE = '''
line1
line2'''


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
