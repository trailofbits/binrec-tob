from pathlib import Path
import subprocess
from unittest.mock import patch, mock_open, call

import pytest

from binrec.errors import BinRecError
from binrec import sigs

NM_OUTPUT = b"""
001edf60 B svc_fdset
00136400 T svc_getreq
000887c0 W strsep
000880b0 i strncasecmp
"""

class TestSigs:

    @patch.object(sigs.subprocess, "check_output")
    def test_get_exported_functions(self, mock_check_output):
        mock_check_output.return_value = NM_OUTPUT
        libc = Path("libc.so")
        assert sigs.get_exported_functions(libc) == [
            "svc_getreq",
            "strsep",
            "strncasecmp"
        ]

    @patch.object(sigs.subprocess, "check_output")
    def test_get_exported_functions_error(self, mock_check_output):
        mock_check_output.side_effect = subprocess.CalledProcessError(0, "error")
        libc = Path("libc.so")
        with pytest.raises(BinRecError):
            sigs.get_exported_functions(libc)

    @patch.object(sigs, "get_exported_functions")
    @patch.object(sigs, "get_function_signatures")
    @patch.object(sigs, "open", new_callable=mock_open)
    def test_gen_lib_sig_database(self, mock_file, mock_get_sigs, mock_get_funcs):
        mock_get_funcs.return_value = ["asdf", "qwer"]
        mock_get_sigs.return_value = ["1", "2"]
        libc = Path("libc.so")
        outfile = Path("libc-argsizes")
        sigs.generate_library_signature_database(libc, outfile)

        mock_get_funcs.assert_called_once_with(libc)
        mock_get_sigs.assert_called_once_with(str(libc), ["asdf", "qwer"])
        mock_file.assert_called_once_with(outfile, "w")
        handle = mock_file()
        handle.write.assert_has_calls([call("1"), call("2")], any_order=True)

    @patch("sys.argv", ["sigs", "libc.so", "libc-argsizes"])
    @patch.object(sigs, "generate_library_signature_database")
    def test_main(self, mock_gen):
        sigs.main()
        mock_gen.assert_called_once_with(Path("libc.so"), Path("libc-argsizes"))
