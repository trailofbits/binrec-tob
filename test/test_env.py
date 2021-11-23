from unittest.mock import patch

import pytest

from binrec import env
from binrec.errors import BinRecError


class TestEnv:

    @patch.object(env, "os")
    @patch.object(env, "load_dotenv")
    def test_load_env(self, mock_load, mock_os):
        mock_os.environ = {"BINREC_ROOT": "asdf", "S2EDIR": "asdf"}
        env._load_env()
        mock_load.assert_called_once_with(".env", override=True)

    @patch.object(env, "os")
    @patch.object(env, "load_dotenv")
    def test_load_env_error(self, mock_load, mock_os):
        mock_os.environ = {}
        with pytest.raises(BinRecError):
            env._load_env()
