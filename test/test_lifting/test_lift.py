from unittest.mock import patch, MagicMock
from subprocess import CalledProcessError
import logging
import sys

import pytest

from binrec import lift
from binrec.env import BINREC_ROOT
from binrec.errors import BinRecError
from binrec import audit

from helpers.mock_path import MockPath


class TestLifting:

    @patch.object(lift.subprocess, 'check_call')
    def test_extract_symbols(self, mock_check_call):
        trace_dir = MagicMock(name="asdf")
        makefile = str(BINREC_ROOT / "scripts" / "s2eout_makefile")

        lift._extract_binary_symbols(trace_dir)

        mock_check_call.assert_called_once_with(["make", "-f", makefile, "symbols"], cwd=str(trace_dir))

    @patch.object(lift.subprocess, 'check_call')
    def test_extract_symbols_error(self, mock_check_call):
        mock_check_call.side_effect = CalledProcessError(0, 'asdf')
        with pytest.raises(BinRecError):
            lift._extract_binary_symbols(MagicMock(name="asdf"))

    @patch.object(lift.subprocess, 'check_call')
    def test_clean_bitcode(self, mock_check_call):
        trace_dir = MagicMock(name="asdf")

        lift._clean_bitcode(trace_dir)

        mock_check_call.assert_called_once_with([lift.BINREC_LIFT, "--clean", "-o", "cleaned", "captured.bc"], cwd=str(trace_dir))

    @patch.object(lift.subprocess, 'check_call')
    def test_clean_bitcode_error(self, mock_check_call):
        mock_check_call.side_effect = CalledProcessError(0, 'asdf')
        with pytest.raises(BinRecError):
            lift._clean_bitcode(MagicMock(name="asdf"))

    @patch.object(lift.subprocess, 'check_call')
    def test_apply_fixups(self, mock_check_call):
        trace_dir = MagicMock(name="asdf")

        lift._apply_fixups(trace_dir)

        mock_check_call.assert_called_once_with([
                "llvm-link",
                "-o",
                "linked.bc",
                "cleaned.bc",
                str(BINREC_ROOT / "runlib" / "custom-helpers.bc"),
            ],
            cwd=str(trace_dir))

    @patch.object(lift.subprocess, 'check_call')
    def test_apply_fixups_error(self, mock_check_call):
        mock_check_call.side_effect = CalledProcessError(0, 'asdf')
        with pytest.raises(BinRecError):
            lift._apply_fixups(MagicMock(name="asdf"))

    @patch.object(lift.subprocess, 'check_call')
    def test_lift_bitcode(self, mock_check_call):
        trace_dir = MagicMock(name="asdf")

        lift._lift_bitcode(trace_dir)

        mock_check_call.assert_called_once_with([lift.BINREC_LIFT, "--lift", "-o", "lifted", "linked.bc", "--clean-names"],
            cwd=str(trace_dir),)

    @patch.object(lift.subprocess, 'check_call')
    def test_lift_bitcode_error(self, mock_check_call):
        mock_check_call.side_effect = CalledProcessError(0, 'asdf')
        with pytest.raises(BinRecError):
            lift._lift_bitcode(MagicMock(name="asdf"))

    @patch.object(lift.subprocess, 'check_call')
    def test_optimize_bitcode(self, mock_check_call):
        trace_dir = MagicMock(name="asdf")

        lift._optimize_bitcode(trace_dir)

        mock_check_call.assert_called_once_with([
                lift.BINREC_LIFT,
                "--optimize",
                "-o",
                "optimized",
                "lifted.bc",
                "--memssa-check-limit=100000",
            ],
            cwd=str(trace_dir),)

    @patch.object(lift.subprocess, 'check_call')
    def test_optimize_bitcode_error(self, mock_check_call):
        mock_check_call.side_effect = CalledProcessError(0, 'asdf')
        with pytest.raises(BinRecError):
            lift._optimize_bitcode(MagicMock(name="asdf"))

    @patch.object(lift.subprocess, 'check_call')
    def test_disassemble_bitcode(self, mock_check_call):
        trace_dir = MagicMock(name="asdf")

        lift._disassemble_bitcode(trace_dir)

        mock_check_call.assert_called_once_with(["llvm-dis", "optimized.bc"], cwd=str(trace_dir))

    @patch.object(lift.subprocess, 'check_call')
    def test_disassemble_bitcode_error(self, mock_check_call):
        mock_check_call.side_effect = CalledProcessError(0, 'asdf')
        with pytest.raises(BinRecError):
            lift._disassemble_bitcode(MagicMock(name="asdf"))

    @patch.object(lift.subprocess, 'check_call')
    def test_recover_bitcode(self, mock_check_call):
        trace_dir = MagicMock(name="asdf")

        lift._recover_bitcode(trace_dir)

        mock_check_call.assert_called_once_with([lift.BINREC_LIFT, "--compile", "-o", "recovered", "optimized.bc"], cwd=str(trace_dir))

    @patch.object(lift.subprocess, 'check_call')
    def test_recover_bitcode_error(self, mock_check_call):
        mock_check_call.side_effect = CalledProcessError(0, 'asdf')
        with pytest.raises(BinRecError):
            lift._recover_bitcode(MagicMock(name="asdf"))

    @patch.object(lift.subprocess, 'check_call')
    def test_compile_bitcode(self, mock_check_call):
        trace_dir = MagicMock(name="asdf")
        makefile = str(BINREC_ROOT / "scripts" / "s2eout_makefile")

        lift._compile_bitcode(trace_dir)

        mock_check_call.assert_called_once_with(["make", "-f", makefile, "-sr", "recovered.o"], cwd=str(trace_dir))

    @patch.object(lift.subprocess, 'check_call')
    def test_compile_bitcode_error(self, mock_check_call):
        mock_check_call.side_effect = CalledProcessError(0, 'asdf')
        with pytest.raises(BinRecError):
            lift._compile_bitcode(MagicMock(name="asdf"))

    @patch.object(lift.subprocess, 'check_call')
    def test_link_recovered_binary(self, mock_check_call):
        trace_dir = MagicMock(name="asdf")
        i386_ld = str(BINREC_ROOT / "binrec_link" / "ld" / "i386.ld")
        libbinrec_rt = str(BINREC_ROOT / "build" / "lib" / "libbinrec_rt.a")

        lift._link_recovered_binary(trace_dir)

        mock_check_call.assert_called_once_with([
                lift.BINREC_LINK,
                "-b",
                "binary",
                "-r",
                "recovered.o",
                "-l",
                libbinrec_rt,
                "-o",
                "recovered",
                "-t",
                i386_ld,
            ],
            cwd=str(trace_dir),)

    @patch.object(lift.subprocess, 'check_call')
    def test_link_recovered_binary_error(self, mock_check_call):
        mock_check_call.side_effect = CalledProcessError(0, 'asdf')
        with pytest.raises(BinRecError):
            lift._link_recovered_binary(MagicMock(name="asdf"))

    @patch.object(lift, '_extract_binary_symbols')
    @patch.object(lift, '_clean_bitcode')
    @patch.object(lift, '_apply_fixups')
    @patch.object(lift, '_lift_bitcode')
    @patch.object(lift, '_optimize_bitcode')
    @patch.object(lift, '_disassemble_bitcode')
    @patch.object(lift, '_recover_bitcode')
    @patch.object(lift, '_compile_bitcode')
    @patch.object(lift, '_link_recovered_binary')
    @patch.object(lift, 'BINREC_ROOT', new_callable=MockPath)
    def test_lift_trace(self, mock_root, mock_link, mock_compile, mock_recover, mock_disasm, mock_optimize,
                        mock_lift, mock_apply, mock_clean, mock_extract):
        trace_dir = mock_root / MockPath("s2e-out-hello", is_dir=True)
        lift.lift_trace("hello")
        mock_extract.assert_called_once_with(trace_dir)
        mock_clean.assert_called_once_with(trace_dir)
        mock_apply.assert_called_once_with(trace_dir)
        mock_lift.assert_called_once_with(trace_dir)
        mock_optimize.assert_called_once_with(trace_dir)
        mock_disasm.assert_called_once_with(trace_dir)
        mock_recover.assert_called_once_with(trace_dir)
        mock_compile.assert_called_once_with(trace_dir)
        mock_link.assert_called_once_with(trace_dir)

    @patch.object(lift, '_extract_binary_symbols')
    @patch.object(lift, '_clean_bitcode')
    @patch.object(lift, '_apply_fixups')
    @patch.object(lift, '_lift_bitcode')
    @patch.object(lift, '_optimize_bitcode')
    @patch.object(lift, '_disassemble_bitcode')
    @patch.object(lift, '_recover_bitcode')
    @patch.object(lift, '_compile_bitcode')
    @patch.object(lift, '_link_recovered_binary')
    @patch.object(lift, 'BINREC_ROOT', new_callable=MockPath)
    def test_lift_trace_error(self, mock_root, mock_link, mock_compile, mock_recover, mock_disasm, mock_optimize,
                        mock_lift, mock_apply, mock_clean, mock_extract):
        trace_dir = mock_root / MockPath("s2e-out-hello", exists=False)
        with pytest.raises(BinRecError):
            lift.lift_trace("hello")

        mock_extract.assert_not_called()
        mock_clean.assert_not_called()
        mock_apply.assert_not_called()
        mock_lift.assert_not_called()
        mock_optimize.assert_not_called()
        mock_disasm.assert_not_called()
        mock_recover.assert_not_called()
        mock_compile.assert_not_called()
        mock_link.assert_not_called()

    @patch("sys.argv", ["merge", "hello"])
    @patch.object(sys, "exit")
    @patch.object(lift, "lift_trace")
    def test_main(self, mock_lift, mock_exit):
        lift.main()
        mock_lift.assert_called_once_with("hello")
        mock_exit.assert_called_once_with(0)

    @patch("sys.argv", ["lift"])
    def test_main_usage_error(self):
        with pytest.raises(SystemExit) as err:
            lift.main()

        assert err.value.code == 2

    @patch("sys.argv", ["lift", "-v", "hello"])
    @patch.object(sys, "exit")
    @patch.object(lift, "lift_trace")
    @patch.object(lift, "logging")
    @patch.object(audit, "enable_python_audit_log")
    def test_main_verbose(self, mock_audit, mock_logging, mock_lift, mock_exit):
        mock_logging.DEBUG = logging.DEBUG
        lift.main()
        mock_logging.getLogger.return_value.setLevel.assert_called_once_with(
            logging.DEBUG
        )
        mock_audit.assert_not_called()

    @patch("sys.argv", ["lift", "-vv", "hello"])
    @patch.object(sys, "exit")
    @patch.object(lift, "lift_trace")
    @patch.object(lift, "logging")
    @patch.object(audit, "enable_python_audit_log")
    def test_main_audit(self, mock_audit, mock_logging, mock_lift, mock_exit):
        mock_logging.DEBUG = logging.DEBUG
        lift.main()
        mock_logging.getLogger.return_value.setLevel.assert_called_once_with(
            logging.DEBUG
        )
        mock_audit.assert_called_once()
