from unittest.mock import patch

from binrec import audit


class TestAduit:
    @patch.object(audit, "sys")
    def test_enable_python_audit_log(self, mock_sys):
        audit.enable_python_audit_log(["HELLO", "WORLD"])
        mock_sys.addaudithook.assert_called_once()
        check = mock_sys.addaudithook.call_args_list[0]
        assert check.args[0].__name__ == "binrec_audit_event"

    @patch.object(audit, "sys")
    @patch.object(audit, "logger")
    def test_enable_python_audit_log_filter_no_match(self, mock_logger, mock_sys):
        audit.enable_python_audit_log(["HELLO", "WORLD"])
        func = mock_sys.addaudithook.call_args_list[0].args[0]
        func("ferp", [])
        assert mock_logger.method_calls == []

    @patch.object(audit, "sys")
    @patch.object(audit, "logger")
    def test_enable_python_audit_log_match(self, mock_logger, mock_sys):
        audit.enable_python_audit_log(["HELLO", "WORLD"])
        func = mock_sys.addaudithook.call_args_list[0].args[0]
        func("HELLO.asdf", ["qwer"])
        mock_logger.debug.assert_called_once_with("%s: %s", "HELLO.asdf", ["qwer"])
