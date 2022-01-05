from inspect import trace
import os
from pathlib import Path
from subprocess import CalledProcessError
from unittest.mock import call, patch

import pytest

from binrec import lift
from binrec.env import BINREC_ROOT
from binrec.errors import BinRecError
from test.helpers.mock_path import MockPath

binrec_lift = str(BINREC_ROOT / "build" / "bin" / "binrec_lift")


class TestLiftingPrepBitcode:
    @patch.object(lift.subprocess, "check_call")
    @patch.object(lift.shutil, "move")
    @patch.object(lift, "os")
    @patch.object(lift.tempfile, "mkstemp")
    def test_prep_bitcode_for_linkage(
        self, mock_mkstemp, mock_os, mock_move, mock_check_call, mock_lib_module
    ):
        mock_mkstemp.return_value = (100, "tempfile")

        trace_dir = MockPath("asdf")
        source = MockPath("/source")
        dest = MockPath("/dest", exists=True)
        lift.prep_bitcode_for_linkage(trace_dir, source, dest)

        mock_mkstemp.assert_called_once()
        mock_os.close.assert_called_once_with(100)
        mock_os.remove.assert_called_once_with("tempfile")
        mock_check_call.assert_called_once_with([
            "llvm-link",
            "-o",
            str(dest),
            "tempfile.bc",
            str(BINREC_ROOT / "runlib" / "softfloat.bc")],
            cwd=str(trace_dir),
        )

        mock_move.assert_called_once_with("tempfile.bc", str(dest))
        mock_lib_module.binrec_lift.link_prep_1.assert_called_once_with(
            trace_filename=str(source),
            destination="tempfile",
            working_dir=str(trace_dir)
        )
        mock_lib_module.binrec_lift.link_prep_2.assert_called_once_with(
            trace_filename=str(dest),
            destination="tempfile",
            working_dir=str(trace_dir)
        )

    @patch.object(lift.subprocess, "check_call")
    @patch.object(lift.shutil, "move")
    @patch.object(lift, "os")
    @patch.object(lift.tempfile, "mkstemp")
    def test_prep_bitcode_for_linkage_rel_destination(
        self, mock_mkstemp, mock_os, mock_move, mock_check_call
    ):
        mock_mkstemp.return_value = (100, "tempfile")
        dest_rel = MockPath("dest")
        trace_dir = MockPath("/trace_dir")

        lift.prep_bitcode_for_linkage(trace_dir, MockPath("/source"), dest_rel)
        mock_move.assert_called_once_with("tempfile.bc", str(trace_dir / "dest"))

    @patch.object(lift.subprocess, "check_call")
    @patch.object(lift.shutil, "move")
    @patch.object(lift, "os")
    @patch.object(lift.tempfile, "mkstemp")
    def test_prep_bitcode_for_linkage_lift_error_stage1(
        self, mock_mkstemp, mock_os, mock_move, mock_check_call, mock_lib_module
    ):
        mock_mkstemp.return_value = (100, "tempfile")
        mock_lib_module.binrec_lift.link_prep_1.side_effect = CalledProcessError(0, "asdf")

        with pytest.raises(BinRecError):
            lift.prep_bitcode_for_linkage(MockPath("/"), MockPath("source"), MockPath("dest"))

        assert mock_os.remove.call_args_list == [call("tempfile"), call("tempfile.bc")]

    @patch.object(lift.subprocess, "check_call")
    @patch.object(lift.shutil, "move")
    @patch.object(lift, "os")
    @patch.object(lift.tempfile, "mkstemp")
    def test_prep_bitcode_for_linkage_lift_llvm_error(
        self, mock_mkstemp, mock_os, mock_move, mock_check_call, mock_lib_module
    ):
        mock_mkstemp.return_value = (100, "tempfile")
        mock_check_call.side_effect = CalledProcessError(0, "asdf")

        lift.prep_bitcode_for_linkage(MockPath("/"), MockPath("source"), MockPath("does not exist"))
        mock_lib_module.binrec_lift.link_prep_2.assert_not_called()

    @patch.object(lift.subprocess, "check_call")
    @patch.object(lift.shutil, "move")
    @patch.object(lift, "os")
    @patch.object(lift.tempfile, "mkstemp")
    def test_prep_bitcode_for_linkage_lift_stage2_error(
        self, mock_mkstemp, mock_os, mock_move, mock_check_call, mock_lib_module
    ):
        mock_mkstemp.return_value = (100, "tempfile")
        mock_lib_module.binrec_lift.link_prep_2.side_effect = CalledProcessError(0, 'asdf')

        with pytest.raises(BinRecError):
            lift.prep_bitcode_for_linkage(MockPath("/"), MockPath("source"), MockPath("/dest", exists=True))

        assert mock_os.remove.call_args_list == [call("tempfile.bc"), call("tempfile")]
