from unittest.mock import patch

from binrec import core


class TestCore:
    @patch.object(core, "logging")
    @patch.object(core, "os")
    def test_init_binrec(self, mock_os, mock_logging):
        core.init_binrec()
        mock_logging.basicConfig.assert_called_once()
