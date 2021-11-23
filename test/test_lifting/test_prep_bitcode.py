import os
from pathlib import Path
from subprocess import CalledProcessError
from unittest.mock import call, patch

import pytest

from binrec import lift
from binrec.env import BINREC_ROOT
from binrec.errors import BinRecError

binrec_lift = str(BINREC_ROOT / "build" / "bin" / "binrec_lift")


class TestLiftingPrepBitcode:
    @patch.object(lift.subprocess, "check_call")
    @patch.object(lift.shutil, "move")
    @patch.object(lift, "os")
    @patch.object(lift.tempfile, "mkstemp")
    def test_prep_bitcode_for_linkage(
        self, mock_mkstemp, mock_os, mock_move, mock_check_call
    ):
        dest = __file__
        mock_mkstemp.return_value = (100, "tempfile")

        lift.prep_bitcode_for_linkage("/", "source", dest)

        mock_mkstemp.assert_called_once()
        mock_os.close.assert_called_once_with(100)
        mock_os.remove.assert_called_once_with("tempfile")
        assert mock_check_call.call_args_list == [
            call([binrec_lift, "--link-prep-1", "-o", "tempfile", "source"], cwd="/"),
            call(
                [
                    "llvm-link",
                    "-o",
                    dest,
                    "tempfile.bc",
                    str(BINREC_ROOT / "runlib" / "softfloat.bc"),
                ],
                cwd="/",
            ),
            call([binrec_lift, "--link-prep-2", "-o", "tempfile", dest], cwd="/"),
        ]

        mock_move.assert_called_once_with("tempfile.bc", Path(dest))

    @patch.object(lift.subprocess, "check_call")
    @patch.object(lift.shutil, "move")
    @patch.object(lift, "os")
    @patch.object(lift.tempfile, "mkstemp")
    def test_prep_bitcode_for_linkage_rel_destination(
        self, mock_mkstemp, mock_os, mock_move, mock_check_call
    ):
        cwd = os.getcwd()
        mock_mkstemp.return_value = (100, "tempfile")
        dest_rel = Path(__file__).relative_to(os.getcwd())

        lift.prep_bitcode_for_linkage(cwd, "source", dest_rel)
        mock_move.assert_called_once_with("tempfile.bc", Path(cwd) / dest_rel)

    @patch.object(lift.subprocess, "check_call")
    @patch.object(lift.shutil, "move")
    @patch.object(lift, "os")
    @patch.object(lift.tempfile, "mkstemp")
    def test_prep_bitcode_for_linkage_lift_error_stage1(
        self, mock_mkstemp, mock_os, mock_move, mock_check_call
    ):
        mock_mkstemp.return_value = (100, "tempfile")
        mock_check_call.side_effect = CalledProcessError(0, "asdf")

        with pytest.raises(BinRecError):
            lift.prep_bitcode_for_linkage("/", "source", __file__)

        assert mock_os.remove.call_args_list == [call("tempfile"), call("tempfile.bc")]

    @patch.object(lift.subprocess, "check_call")
    @patch.object(lift.shutil, "move")
    @patch.object(lift, "os")
    @patch.object(lift.tempfile, "mkstemp")
    def test_prep_bitcode_for_linkage_lift_llvm_error(
        self, mock_mkstemp, mock_os, mock_move, mock_check_call
    ):
        mock_mkstemp.return_value = (100, "tempfile")
        mock_check_call.side_effect = [None, CalledProcessError(0, "asdf")]

        lift.prep_bitcode_for_linkage("/", "source", "does not exist")
        assert mock_check_call.call_count == 2

    @patch.object(lift.subprocess, "check_call")
    @patch.object(lift.shutil, "move")
    @patch.object(lift, "os")
    @patch.object(lift.tempfile, "mkstemp")
    def test_prep_bitcode_for_linkage_lift_stage2_error(
        self, mock_mkstemp, mock_os, mock_move, mock_check_call
    ):
        mock_mkstemp.return_value = (100, "tempfile")
        mock_check_call.side_effect = [None, None, CalledProcessError(0, "asdf")]

        with pytest.raises(BinRecError):
            lift.prep_bitcode_for_linkage("/", "source", __file__)

        assert mock_os.remove.call_args_list == [call("tempfile.bc"), call("tempfile")]
