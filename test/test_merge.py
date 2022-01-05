import subprocess
import sys
from unittest.mock import patch, call
from pathlib import Path
import logging

import pytest

from binrec import merge
from binrec.env import BINREC_ROOT
from binrec.errors import BinRecError
from binrec import audit

from helpers.mock_path import MockPath


class TestMerge:
    @patch.object(merge.subprocess, "check_call")
    def test_link_bitcode(self, mock_check_call):
        merge._link_bitcode("base", "source", "dest")
        mock_check_call.assert_called_once_with(
            [
                "llvm-link",
                "-print-after-all",
                "-v",
                "-o",
                "dest",
                "-override=base",
                "source",
            ],
        )

    @patch.object(merge.subprocess, "check_call")
    def test_link_bitcode_exc(self, mock_check_call):
        mock_check_call.side_effect = subprocess.CalledProcessError(0, "asdf")
        with pytest.raises(BinRecError):
            merge._link_bitcode("base", "source", "dest")

    @patch.object(merge.subprocess, "check_call")
    def test_merge_trace_info(self, mock_check_call):
        binrec_tracemerge = str(BINREC_ROOT / "build" / "bin" / "binrec_tracemerge")
        merge._merge_trace_info([1, 2, 3], "dest")
        mock_check_call.assert_called_once_with(
            [binrec_tracemerge, "1", "2", "3", "dest"]
        )

    @patch.object(merge.subprocess, "check_call")
    def test_merge_trace_info_exc(self, mock_check_call):
        mock_check_call.side_effect = subprocess.CalledProcessError(0, "asdf")
        with pytest.raises(BinRecError):
            merge._merge_trace_info(["asdf"], "qwer")

    @patch.object(merge, "shutil")
    @patch.object(merge, "prep_bitcode_for_linkage")
    @patch.object(merge.tempfile, "mkstemp")
    @patch.object(merge, "os")
    @patch.object(merge, "_link_bitcode")
    @patch.object(merge.subprocess, "check_call")
    @patch.object(merge, "_merge_trace_info")
    def test_merge_bitcode(
        self,
        mock_merge,
        mock_check_call,
        mock_link,
        mock_os,
        mock_mkstemp,
        mock_prep_bitcode,
        mock_shutil,
    ):
        dest = MockPath() / "does" / "not" / "exist"
        dest.exists.return_value = False
        mock_mkstemp.return_value = (100, "temp")
        outfile = dest / "captured.bc"
        capture_dirs = [
            Path("/") / "i" / "don't" / "exist",
            Path("/") / "I" / "don't" / "either",
        ]

        merge.merge_bitcode(capture_dirs, dest)
        dest.exists.assert_called_once()
        dest.mkdir.assert_called_once_with(exist_ok=True)
        mock_shutil.rmtree.assert_not_called()

        assert mock_prep_bitcode.call_args_list == [
            call(capture_dirs[0], Path("captured.bc"), Path("captured-link-ready.bc")),
            call(capture_dirs[1], Path("captured.bc"), Path("captured-link-ready.bc")),
        ]

        mock_shutil.copy.assert_called_once_with(
            capture_dirs[0] / "captured-link-ready.bc", outfile
        )

        mock_mkstemp.assert_called_once_with(suffix=".bc")
        mock_os.close.assert_called_once_with(100)

        assert mock_link.call_args_list == [
            call(
                base=outfile,
                source=capture_dirs[0] / "captured-link-ready.bc",
                destination=Path("temp"),
            ),
            call(
                base=outfile,
                source=capture_dirs[1] / "captured-link-ready.bc",
                destination=Path("temp"),
            ),
        ]
        assert mock_shutil.move.call_args_list == [
            call("temp", outfile),
            call("temp", outfile),
        ]

        mock_check_call.assert_called_once_with(["llvm-dis", str(outfile)])
        mock_merge.assert_called_once_with(
            [capture_dirs[0] / "traceInfo.json", capture_dirs[1] / "traceInfo.json"],
            dest / "traceInfo.json",
        )

    @patch.object(merge, "shutil")
    @patch.object(merge, "prep_bitcode_for_linkage")
    @patch.object(merge.tempfile, "mkstemp")
    @patch.object(merge, "os")
    @patch.object(merge, "_link_bitcode")
    @patch.object(merge.subprocess, "check_call")
    @patch.object(merge, "_merge_trace_info")
    def test_merge_bitcode_rmtree(
        self,
        mock_merge,
        mock_check_call,
        mock_link,
        mock_os,
        mock_mkstemp,
        mock_prep_bitcode,
        mock_shutil,
    ):
        dest = MockPath("/") / "does" / "not" / "exist"
        dest._exists = True
        mock_mkstemp.return_value = (100, "temp")
        capture_dirs = [
            Path("/") / "i" / "don't" / "exist",
            Path("/") / "I" / "don't" / "either",
        ]

        merge.merge_bitcode(capture_dirs, dest)
        mock_shutil.rmtree.assert_called_once_with(dest)

    @patch.object(merge, "shutil")
    @patch.object(merge, "prep_bitcode_for_linkage")
    @patch.object(merge.tempfile, "mkstemp")
    @patch.object(merge, "os")
    @patch.object(merge, "_link_bitcode")
    @patch.object(merge.subprocess, "check_call")
    @patch.object(merge, "_merge_trace_info")
    def test_merge_bitcode_llvm_error(
        self,
        mock_merge,
        mock_check_call,
        mock_link,
        mock_os,
        mock_mkstemp,
        mock_prep_bitcode,
        mock_shutil,
    ):
        dest = MockPath("/") / "does" / "not" / "exist"
        dest.exists.return_value = True
        mock_mkstemp.return_value = (100, "temp")
        capture_dirs = [
            Path("/") / "i" / "don't" / "exist",
            Path("/") / "I" / "don't" / "either",
        ]
        mock_check_call.side_effect = subprocess.CalledProcessError(0, "asdf")

        with pytest.raises(BinRecError):
            merge.merge_bitcode(capture_dirs, dest)

    @patch.object(merge, "BINREC_ROOT", new_callable=MockPath)
    @patch.object(merge, "shutil")
    @patch.object(merge, "merge_bitcode")
    def test_merge_traces(self, mock_merge_bc, mock_shutil, mock_root):
        normal_file = MockPath("norm", is_dir=False)
        dir_no_merged = MockPath("s2e-out-hello-1", is_dir=True)
        dir_with_merged = MockPath("s2e-out-hello-2", is_dir=True)
        dir_no_match = MockPath("dir3", is_dir=True)
        merged_dir = MockPath("merged", is_dir=True)
        mock_root.build_tree(normal_file, dir_no_merged, dir_with_merged, dir_no_match)
        dir_with_merged.build_tree(merged_dir)

        outdir = mock_root / "s2e-out-hello"

        merge.merge_traces("hello")

        mock_merge_bc.assert_called_once_with([merged_dir], outdir)

        assert mock_shutil.copy2.call_args_list == [
            call(merged_dir / "binary", outdir / "binary"),
            call(merged_dir / "Makefile", outdir / "Makefile"),
        ]

    @patch.object(merge, "BINREC_ROOT", new_callable=MockPath)
    @patch.object(merge, "shutil")
    @patch.object(merge, "merge_bitcode")
    def test_merge_traces_no_dirs(self, mock_merge_bc, mock_shutil, mock_root):
        mock_root.build_tree(
            MockPath("s2e-out-hello-1", is_dir=False),
            MockPath("s2e-out-hello-2", is_dir=True),
            MockPath("asdf", is_dir=True),
        )

        with pytest.raises(BinRecError):
            merge.merge_traces("hello")

    @patch.object(merge, "BINREC_ROOT", new_callable=MockPath)
    @patch.object(merge, "shutil")
    @patch.object(merge, "merge_bitcode")
    def test_merge_trace_captures(self, mock_merge_bc, mock_shutil, mock_root):
        source = mock_root / "s2e-out-hello-1"
        outdir = source / "merged"

        trace0 = source / MockPath("0", is_dir=True)
        source.build_tree(
            MockPath("1"), MockPath("100", is_dir=True), MockPath("asdf", is_dir=True)
        )

        merge.merge_trace_captures("hello-1")

        mock_merge_bc.assert_called_once_with([trace0], outdir)
        assert mock_shutil.copy2.call_args_list == [
            call(trace0 / "binary", outdir / "binary"),
            call(source / "Makefile", outdir / "Makefile"),
        ]

    @patch.object(merge, "BINREC_ROOT", new_callable=MockPath)
    def test_merge_trace_captures_error(self, mock_root):
        source = mock_root / "s2e-out-hello-1"

        source.build_tree(
            MockPath("1"), MockPath("100", is_dir=True), MockPath("asdf", is_dir=True)
        )

        with pytest.raises(BinRecError):
            merge.merge_trace_captures("hello-1")

    @patch.object(merge, "BINREC_ROOT", new_callable=MockPath)
    @patch.object(merge, "merge_trace_captures")
    @patch.object(merge, "merge_traces")
    def test_recursive_merge_traces(
        self, mock_merge_traces, mock_merge_captures, mock_root
    ):
        mock_root.build_tree(
            MockPath("s2e-out-hello-1", is_dir=True),
            MockPath("s2e-out-hello2-1", is_dir=True),
            MockPath("asdf"),
            MockPath("s2e-out-hello-2"),
        )

        merge.recursive_merge_traces("hello")

        mock_merge_captures.assert_called_once_with("hello-1")
        mock_merge_traces.assert_called_once_with("hello")

    @patch.object(merge, "BINREC_ROOT", new_callable=MockPath)
    def test_recursive_merge_traces_error(self, mock_root):
        mock_root.build_tree(
            MockPath("s2e-out-hello2-1", is_dir=True),
            MockPath("asdf"),
            MockPath("s2e-out-hello-2"),
        )

        with pytest.raises(BinRecError):
            merge.recursive_merge_traces("hello")

    @patch("sys.argv", ["merge", "-t", "hello-1"])
    @patch.object(sys, "exit")
    @patch.object(merge, "merge_trace_captures")
    def test_main_captures(self, mock_merge_captures, mock_exit):
        merge.main()
        mock_merge_captures.assert_called_once_with("hello-1")
        mock_exit.assert_called_once_with(0)

    @patch("sys.argv", ["merge", "-b", "hello"])
    @patch.object(sys, "exit")
    @patch.object(merge, "recursive_merge_traces")
    def test_main_traces(self, mock_merge_traces, mock_exit):
        merge.main()
        mock_merge_traces.assert_called_once_with("hello")
        mock_exit.assert_called_once_with(0)

    @patch("sys.argv", ["merge", "-b", "hello", "-t", "hello-1"])
    def test_main_usage_error1(self):
        with pytest.raises(SystemExit) as err:
            merge.main()

        assert err.value.code == 2

    @patch("sys.argv", ["merge"])
    def test_main_usage_error2(self):
        with pytest.raises(SystemExit) as err:
            merge.main()

        assert err.value.code == 2

    @patch("sys.argv", ["merge", "-v", "-b", "hello"])
    @patch.object(sys, "exit")
    @patch.object(merge, "recursive_merge_traces")
    @patch.object(merge, "logging")
    def test_main_verbose(self, mock_logging, mock_merge, mock_exit):
        merge.logging.DEBUG = logging.DEBUG
        merge.main()
        mock_logging.getLogger.assert_called_once_with("binrec")
        print(mock_logging.getLogger.return_value.setLevel.call_args_list)
        mock_logging.getLogger.return_value.setLevel.assert_called_once_with(
            logging.DEBUG
        )

    @patch("sys.argv", ["merge", "-vv", "-b", "hello"])
    @patch.object(sys, "exit")
    @patch.object(merge, "recursive_merge_traces")
    @patch.object(merge, "logging")
    @patch.object(audit, "enable_python_audit_log")
    def test_main_audit(self, mock_audit, mock_logging, mock_merge, mock_exit):
        mock_logging.DEBUG = logging.DEBUG
        merge.main()
        mock_logging.getLogger.return_value.setLevel.assert_called_once_with(
            logging.DEBUG
        )
        mock_audit.assert_called_once()
