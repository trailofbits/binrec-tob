from unittest import mock
from unittest.mock import patch, MagicMock, mock_open, call
from subprocess import CalledProcessError
import logging
import sys

import pytest

from binrec import lift
from binrec.env import BINREC_ROOT, llvm_command
from binrec.errors import BinRecError
from binrec import audit

from helpers.mock_path import MockPath


READELF_OUTPUT = """
Symbol table '.dynsym' contains 9 entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
     0: 00000000     0 NOTYPE  LOCAL  DEFAULT  UND
     1: 00000000     0 FUNC    GLOBAL DEFAULT  UND printf@GLIBC_2.0 (2)
     2: 00000000     0 FUNC    GLOBAL DEFAULT  UND fwrite@GLIBC_2.0 (2)
     3: 00000000     0 NOTYPE  WEAK   DEFAULT  UND __gmon_start__
     4: 00000000     0 FUNC    GLOBAL DEFAULT  UND __libc_start_main@GLIBC_2.0 (2)
     5: 0804c044     4 OBJECT  GLOBAL DEFAULT   26 stdout@GLIBC_2.0 (2)
     6: 0804c020     4 OBJECT  GLOBAL DEFAULT   26 stderr@GLIBC_2.0 (2)
     7: 0804a004     4 OBJECT  GLOBAL DEFAULT   17 _IO_stdin_used
"""


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

    @patch.object(lift.subprocess, 'check_output')
    @patch.object(lift, 'open', new_callable=mock_open)
    def test_extract_data_imports(self, mock_file, mock_check_output):
        trace_dir = MagicMock(name="asdf")
        mock_check_output.return_value = READELF_OUTPUT.encode()
        lift._extract_data_imports(trace_dir)

        mock_check_output.assert_called_once_with(["readelf", "--dyn-sym",
                                                   str(trace_dir / "binary")])
        mock_file.assert_called_once_with(trace_dir / "data_imports", "w")
        handle = mock_file()
        assert handle.write.call_args_list == [
            call("0804c044 4 stdout"),
            call('\n'),
            call("0804c020 4 stderr"),
            call('\n')
        ]

    @patch.object(lift.subprocess, 'check_output')
    def test_extract_data_imports_error(self, mock_check_output):
        mock_check_output.side_effect = CalledProcessError(0, 'asdf')
        with pytest.raises(BinRecError):
            lift._extract_data_imports(MagicMock(name='asdf'))

    def test_clean_bitcode(self, mock_lib_module):
        trace_dir = MockPath("asdf")

        lift._clean_bitcode(trace_dir)

        mock_lib_module.binrec_lift.clean.assert_called_once_with(
            trace_filename="captured.bc",
            destination="cleaned",
            working_dir=str(trace_dir)
        )

    def test_clean_bitcode_error(self, mock_lib_module):
        mock_lib_module.binrec_lift.clean.side_effect = OSError()
        with pytest.raises(BinRecError):
            lift._clean_bitcode(MockPath("asdf"))

    @patch.object(lift.subprocess, 'check_call')
    def test_apply_fixups(self, mock_check_call):
        trace_dir = MagicMock(name="asdf")

        lift._apply_fixups(trace_dir)

        mock_check_call.assert_called_once_with([
                llvm_command("llvm-link"),
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

    def test_lift_bitcode(self, mock_lib_module):
        trace_dir = MockPath("asdf")

        lift._lift_bitcode(trace_dir)

        mock_lib_module.binrec_lift.lift.assert_called_once_with(
            trace_filename="linked.bc",
            destination="lifted",
            clean_names=True,
            working_dir=str(trace_dir)
        )

    def test_lift_bitcode_error(self, mock_lib_module):
        mock_lib_module.binrec_lift.lift.side_effect = OSError()
        with pytest.raises(BinRecError):
            lift._lift_bitcode(MockPath("asdf"))

    def test_optimize_bitcode(self, mock_lib_module):
        trace_dir = MockPath("asdf")

        lift._optimize_bitcode(trace_dir)

        mock_lib_module.binrec_lift.optimize(
            trace_filename="lifted.bc",
            destination="optimized",
            memssa_check_limit=100000,
            working_dir=str(trace_dir)
        )

    def test_optimize_bitcode_error(self, mock_lib_module):
        mock_lib_module.binrec_lift.optimize.side_effect = OSError()
        with pytest.raises(BinRecError):
            lift._optimize_bitcode(MockPath("asdf"))

    @patch.object(lift.subprocess, 'check_call')
    def test_disassemble_bitcode(self, mock_check_call):
        trace_dir = MagicMock(name="asdf")

        lift._disassemble_bitcode(trace_dir)

        mock_check_call.assert_called_once_with([llvm_command("llvm-dis"), "optimized.bc"], cwd=str(trace_dir))

    @patch.object(lift.subprocess, 'check_call')
    def test_disassemble_bitcode_error(self, mock_check_call):
        mock_check_call.side_effect = CalledProcessError(0, 'asdf')
        with pytest.raises(BinRecError):
            lift._disassemble_bitcode(MagicMock(name="asdf"))

    def test_recover_bitcode(self, mock_lib_module):
        trace_dir = MockPath("asdf")

        lift._recover_bitcode(trace_dir)

        mock_lib_module.binrec_lift.compile_prep.assert_called_once_with(
            trace_filename="optimized.bc",
            destination="recovered",
            working_dir=str(trace_dir)
        )

    def test_recover_bitcode_error(self, mock_lib_module):
        mock_lib_module.binrec_lift.compile_prep.side_effect = OSError()
        with pytest.raises(BinRecError):
            lift._recover_bitcode(MockPath("asdf"))

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

    def test_link_recovered_binary(self, mock_lib_module):
        trace_dir = MockPath("asdf")
        i386_ld = str(BINREC_ROOT / "binrec_link" / "ld" / "i386.ld")
        libbinrec_rt = str(BINREC_ROOT / "build" / "lib" / "libbinrec_rt.a")

        lift._link_recovered_binary(trace_dir)

        mock_lib_module.binrec_link.link.assert_called_once_with(
            binary_filename=str(trace_dir / "binary"),
            recovered_filename=str(trace_dir / "recovered.o"),
            runtime_library=libbinrec_rt,
            linker_script=i386_ld,
            destination=str(trace_dir / "recovered")
        )

    def test_link_recovered_binary_error(self, mock_lib_module):
        mock_lib_module.binrec_link.link.side_effect = CalledProcessError(0, 'asdf')
        with pytest.raises(BinRecError):
            lift._link_recovered_binary(MagicMock(name="asdf"))

    @patch.object(lift, '_extract_binary_symbols')
    @patch.object(lift, '_extract_data_imports')
    @patch.object(lift, '_clean_bitcode')
    @patch.object(lift, '_apply_fixups')
    @patch.object(lift, '_lift_bitcode')
    @patch.object(lift, '_optimize_bitcode')
    @patch.object(lift, '_disassemble_bitcode')
    @patch.object(lift, '_recover_bitcode')
    @patch.object(lift, '_compile_bitcode')
    @patch.object(lift, '_link_recovered_binary')
    @patch.object(lift, 'project')
    def test_lift_trace(self, mock_project, mock_link, mock_compile, mock_recover, mock_disasm, mock_optimize,
                        mock_lift, mock_apply, mock_clean, mock_data_imports, mock_extract):
        mock_project.merged_trace_dir.return_value = trace_dir = MockPath("s2e-out", is_dir=True)
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
        mock_data_imports.assert_called_once_with(trace_dir)

    @patch.object(lift, '_extract_binary_symbols')
    @patch.object(lift, '_clean_bitcode')
    @patch.object(lift, '_apply_fixups')
    @patch.object(lift, '_lift_bitcode')
    @patch.object(lift, '_optimize_bitcode')
    @patch.object(lift, '_disassemble_bitcode')
    @patch.object(lift, '_recover_bitcode')
    @patch.object(lift, '_compile_bitcode')
    @patch.object(lift, '_link_recovered_binary')
    @patch.object(lift, 'project')
    def test_lift_trace_error(self, mock_project, mock_link, mock_compile, mock_recover, mock_disasm, mock_optimize,
                        mock_lift, mock_apply, mock_clean, mock_extract):
        mock_project.merged_trace_dir.return_value = MockPath("s2e-out", exists=False)
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
