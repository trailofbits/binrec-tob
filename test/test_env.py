import subprocess
from unittest.mock import patch, MagicMock

import pytest

from helpers.mock_path import MockPath

from binrec import env
from binrec.errors import BinRecError


class TestEnv:

    @patch.object(env, "os")
    @patch.object(env, "load_dotenv")
    @patch.object(env.subprocess, "check_output")
    def test_load_env(self, mock_check_output, mock_load, mock_os):
        mock_os.environ = {"BINREC_ROOT": "asdf", "S2EDIR": "asdf", "S2E_BIN":"asdf", "PATH":"ghjk"}
        mock_check_output.return_value = b""
        env._load_env()
        mock_load.assert_called_once_with(".env", override=True)

    @patch.object(env, "os")
    @patch.object(env, "load_dotenv")
    @patch.object(env.subprocess, "check_output")
    def test_load_env_error(self, mock_check_output, mock_load, mock_os):
        mock_os.environ = {}
        mock_check_output.return_value = b""
        with pytest.raises(BinRecError):
            env._load_env()

    @patch.object(env, "os")
    @patch.object(env, "load_dotenv")
    @patch.object(env.subprocess, "check_output")
    def test_load_env_s2e_bins_error(self, mock_check_output, mock_load, mock_os):
        mock_os.environ = {"BINREC_ROOT": "asdf", "S2EDIR": "asdf"}
        mock_check_output.return_value = b""
        with pytest.raises(BinRecError):
            env._load_env()

    @patch.object(env, "os")
    @patch.object(env, "load_dotenv")
    @patch.object(env.subprocess, "check_output")
    def test_gcc_lib(self, mock_check_output, mock_load, mock_os):
        mock_check_output.return_value = b"asdf"
        mock_os.environ = {"BINREC_ROOT": "asdf", "S2EDIR": "asdf", "S2E_BIN":"asdf", "PATH":"ghjk"}
        env._load_env()
        assert mock_os.environ["GCC_LIB"] == "/usr/lib/gcc/x86_64-linux-gnu/asdf"

    @patch.object(env, "os")
    @patch.object(env, "load_dotenv")
    @patch.object(env.subprocess, "check_output")
    def test_gcc_lib_error(self, mock_check_output, mock_load, mock_os):
        mock_check_output.side_effect = subprocess.CalledProcessError(1, "asdf")
        mock_os.environ = {"BINREC_ROOT": "asdf", "S2EDIR": "asdf", "S2E_BIN":"asdf", "PATH":"ghjk"}
        env._load_env()
        assert "GCC_LIB" not in mock_os.environ

    @patch.object(env, "os")
    @patch.object(env, "load_dotenv")
    @patch.object(env.subprocess, "check_output")
    def test_path_update(self, mock_check_output, mock_load, mock_os):
        mock_check_output.return_value = b"asdf"
        mock_os.environ = {"BINREC_ROOT": "asdf", "S2EDIR": "asdf", "S2E_BIN":"asdf", "PATH": "ghjk"}
        env._load_env()
        assert mock_os.environ["PATH"] == "asdf:ghjk"

    def test_project_dir(self):
        assert env.project_dir("asdf") == env.BINREC_PROJECTS / "asdf"

    def test_merged_trace_dir(self):
        assert env.merged_trace_dir("asdf") == env.BINREC_PROJECTS / "asdf" / "s2e-out"

    def test_recovered_binary_filename(self):
        assert env.recovered_binary_filename("asdf") == env.BINREC_PROJECTS / "asdf" / "s2e-out" / "recovered"

    def test_trace_dir(self):
        assert env.trace_dir("asdf", 100) == env.BINREC_PROJECTS / "asdf" / "s2e-out-100"

    @patch.object(env, "project_dir")
    def test_get_trace_dirs(self, mock_project_dir):
        root = mock_project_dir.return_value = MockPath("asdf")
        file = root / MockPath("s2e-out-1", exists=True)
        not_trace_dir = root / MockPath("asdf", is_dir=True)
        trace_dir_1 = root / MockPath("s2e-out-2", is_dir=True)
        trace_dir_2 = root / MockPath("s2e-out-3", is_dir=True)

        assert env.get_trace_dirs("asdf") == [trace_dir_1, trace_dir_2]
        mock_project_dir.assert_called_once_with("asdf")

    def test_input_files_dirs(self):
        assert env.input_files_dir("asdf") == env.BINREC_PROJECTS / "asdf" / "input_files"

    @patch.object(env, "project_dir")
    def test_campaign_filename(self, mock_proj):
        mock_proj.return_value = MockPath("/project")
        assert env.campaign_filename("asdf") == mock_proj.return_value / "campaign.json"
        mock_proj.assert_called_once_with("asdf")

    @patch.object(env, "project_dir")
    def test_s2e_config_file(self, mock_proj):
        mock_proj.return_value = MockPath("/project")
        assert env.s2e_config_filename("asdf") == mock_proj.return_value / "s2e-config.lua"
        mock_proj.assert_called_once_with("asdf")

    @patch.object(env, "project_dir")
    def test_project_binary_filename(self, mock_proj):
        mock_proj.return_value = MockPath("/project")
        binary = mock_proj.return_value / "binary"
        binary.readlink = MagicMock()
        assert env.project_binary_filename("asdf") == binary.readlink.return_value
        mock_proj.assert_called_once_with("asdf")
        binary.readlink.assert_called_once()
