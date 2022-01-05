import subprocess
from unittest.mock import patch

import pytest

from binrec import env
from binrec.errors import BinRecError


class TestEnv:

    @patch.object(env, "os")
    @patch.object(env, "load_dotenv")
    @patch.object(env.subprocess, "check_output")
    def test_load_env(self, mock_check_output, mock_load, mock_os):
        mock_os.environ = {"BINREC_ROOT": "asdf", "S2EDIR": "asdf"}
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
    def test_gcc_lib(self, mock_check_output, mock_load, mock_os):
        mock_check_output.return_value = b"asdf"
        mock_os.environ = {"BINREC_ROOT": "asdf", "S2EDIR": "asdf"}
        env._load_env()
        assert mock_os.environ["GCC_LIB"] == "/usr/lib/gcc/x86_64-linux-gnu/asdf"

    @patch.object(env, "os")
    @patch.object(env, "load_dotenv")
    @patch.object(env.subprocess, "check_output")
    def test_gcc_lib_error(self, mock_check_output, mock_load, mock_os):
        mock_check_output.side_effect = subprocess.CalledProcessError(1, "asdf")
        mock_os.environ = {"BINREC_ROOT": "asdf", "S2EDIR": "asdf"}
        env._load_env()
        assert "GCC_LIB" not in mock_os.environ
