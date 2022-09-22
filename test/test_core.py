from unittest.mock import patch

from binrec import core


class TestCore:
    @patch.object(core, "logging")
    @patch.object(core, "os")
    def test_init_binrec(self, mock_os, mock_logging):
        core.init_binrec()
        mock_logging.basicConfig.assert_called_once()

    @patch.object(core, "logging")
    @patch.object(core, "os")
    @patch.object(core, "enable_binrec_debug_mode")
    def test_init_binrec_debug(self, mock_debug, mock_os, mock_logging):
        mock_os.environ = {"BINREC_DEBUG": "1"}
        core.init_binrec()
        mock_logging.basicConfig.assert_called_once()
        mock_debug.assert_called_once()

    @patch.object(core, "logging")
    @patch.object(core, "os")
    def test_enable_binrec_debug_mode(self, mock_os, mock_logging):
        mock_os.environ = {}
        core.enable_binrec_debug_mode()
        mock_logging.getLogger.assert_called_once_with("binrec")
        mock_logging.getLogger.return_value.setLevel.assert_called_once_with(mock_logging.DEBUG)
        assert mock_os.environ == {"BINREC_DEBUG": "1"}
