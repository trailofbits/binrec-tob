import sys
from pathlib import Path
from unittest.mock import MagicMock, patch, call
import pytest
from binrec import trace_info


class TestTraceInfo:
    def test_hexify_int_list(self):
        assert trace_info._hexify_int_list([1, 20, 64]) == ["0x1", "0x14", "0x40"]

    def test_hexify_address_tuple_list(self):
        assert trace_info._hexify_address_tuple_list([[10, 20], [40, 60]]) == [
            ["0xa", "0x14"],
            ["0x28", "0x3c"],
        ]

    def test_hexify_successor_list(self):
        assert trace_info._hexify_successor_list(
            [{"successor": 16, "pc": 32}, {"successor": 64, "pc": 80}]
        ) == [{"successor": "0x10", "pc": "0x20"}, {"successor": "0x40", "pc": "0x50"}]

    def test_transform_trace_info_item(self):
        transform = MagicMock()
        obj = {"a": {"b": [1]}}
        trace_info._transform_trace_info_item(obj, "a.b", transform)

        transform.assert_called_once_with([1])
        assert obj["a"]["b"] is transform.return_value

    def test_hexify_tbs_list(self):
        assert trace_info._hexify_tbs_list([[1, [2, 3]], [4, [5]]]) == [
            ["0x1", ["0x2", "0x3"]],
            ["0x4", ["0x5"]],
        ]

    @patch.object(trace_info, "_transform_trace_info_item")
    def test_hexify_trace_info(self, mock_transform):
        obj = {"a": {"b": [1]}}
        check = trace_info.hexify_trace_info(obj)
        assert obj == check
        assert obj is not check
        assert mock_transform.call_args_list == [
            call(
                check,
                "functionLog.callerToFollowUp",
                trace_info._hexify_address_tuple_list,
            ),
            call(
                check,
                "functionLog.entryToCaller",
                trace_info._hexify_address_tuple_list,
            ),
            call(
                check,
                "functionLog.entryToReturn",
                trace_info._hexify_address_tuple_list,
            ),
            call(check, "functionLog.entries", trace_info._hexify_int_list),
            call(check, "successors", trace_info._hexify_successor_list),
            call(check, "functionLog.entryToTbs", trace_info._hexify_tbs_list),
        ]

    def test_compare_set(self):
        left = set([1, 2, 3])
        right = set([1, 3, 5])
        assert list(trace_info._compare_set("asdf", left, right)) == [
            trace_info.TraceInfoDiff("asdf", "-", 2),
            trace_info.TraceInfoDiff("asdf", "+", 5),
        ]

    def test_get_nested_dict_item(self):
        obj = {"a": {"b": {"c": [1]}}}
        assert (
            trace_info._get_nested_dict_item(obj, ["a", "b", "c"]) is obj["a"]["b"]["c"]
        )

    def test_get_nested_dict_item_default(self):
        obj = {"a": {}}
        assert trace_info._get_nested_dict_item(obj, ["a", "b", "c"]) == []

    def test_get_nested_dict_item_error(self):
        with pytest.raises(ValueError):
            trace_info._get_nested_dict_item({"a": 1}, ["a", "b"])

    @patch.object(trace_info, "_compare_set")
    @patch.object(trace_info, "_get_nested_dict_item")
    def test_compare_trace_info_item(self, mock_get_item, mock_compare_set):
        mock_compare_set.return_value = [1, 2, 3]
        mock_set = MagicMock()
        mock_set.side_effect = [3, 4]
        left = object()
        right = object()
        mock_get_item.side_effect = [1, 2]
        assert (
            trace_info._compare_trace_info_item("a.b.c", left, right, mock_set)
            == mock_compare_set.return_value
        )
        assert mock_get_item.call_args_list == [
            call(left, ["a", "b", "c"]),
            call(right, ["a", "b", "c"]),
        ]
        assert mock_set.call_args_list == [call(1), call(2)]
        mock_compare_set.assert_called_once_with("a.b.c", 3, 4)

    def test_address_list_to_set(self):
        assert trace_info._address_list_to_set([[1, 2], [3, 4]]) == set(
            [(1, 2), (3, 4)]
        )

    def test_successor_list_to_set(self):
        assert trace_info._successor_list_to_set(
            [{"successor": 1, "pc": 2}, {"successor": 4, "pc": 5}]
        ) == set([("successor:1", "pc:2"), ("successor:4", "pc:5")])

    def test_tbs_list_to_set(self):
        assert trace_info._tbs_list_to_set([[1, [5, 2]], [3, [6]]]) == set(
            [(1, (2, 5)), (3, (6,))]
        )

    @patch.object(trace_info, "_compare_trace_info_item")
    def test_compare_trace_info(self, mock_compare):
        left = object()
        right = object()
        mock_compare.return_value = [1]
        assert trace_info.compare_trace_info(left, right) == [1, 1, 1, 1, 1, 1]
        assert mock_compare.call_args_list == [
            call(
                "functionLog.callerToFollowUp",
                left,
                right,
                trace_info._address_list_to_set,
            ),
            call(
                "functionLog.entryToCaller",
                left,
                right,
                trace_info._address_list_to_set,
            ),
            call(
                "functionLog.entryToReturn",
                left,
                right,
                trace_info._address_list_to_set,
            ),
            call("functionLog.entries", left, right, set),
            call("successors", left, right, trace_info._successor_list_to_set),
            call("functionLog.entryToTbs", left, right, trace_info._tbs_list_to_set),
        ]

    @patch.object(trace_info, "json")
    @patch.object(trace_info, "print")
    def test_pretty_print_trace_info(self, mock_print, mock_json):
        mock_path = MagicMock()
        mock_file = MagicMock()
        trace_info.pretty_print_trace_info(mock_path, file=mock_file)
        mock_print.assert_called_once_with(mock_json.dumps.return_value, file=mock_file)
        mock_json.dumps.assert_called_once_with(mock_json.loads.return_value, indent=2)
        mock_json.loads.assert_called_once_with(
            mock_path.read_text.return_value.strip.return_value
        )

    @patch.object(trace_info, "json")
    @patch.object(trace_info, "print")
    @patch.object(trace_info, "hexify_trace_info")
    def test_pretty_print_trace_info_hexify(self, mock_hexify, mock_print, mock_json):
        mock_path = MagicMock()
        mock_file = MagicMock()
        trace_info.pretty_print_trace_info(mock_path, hexify=True, file=mock_file)
        mock_hexify.assert_called_once_with(mock_json.loads.return_value)
        mock_print.assert_called_once_with(mock_json.dumps.return_value, file=mock_file)
        mock_json.dumps.assert_called_once_with(mock_hexify.return_value, indent=2)
        mock_json.loads.assert_called_once_with(
            mock_path.read_text.return_value.strip.return_value
        )

    @patch.object(trace_info, "print")
    @patch.object(trace_info, "compare_trace_info")
    @patch.object(trace_info, "json")
    def test_diff_trace_info_files(self, mock_json, mock_compare, mock_print):
        mock_left = MagicMock()
        mock_right = MagicMock()
        left = object()
        right = object()
        mock_compare.return_value = [object(), object(), object()]
        mock_json.loads.side_effect = [left, right]
        assert trace_info._diff_trace_info_files(mock_left, mock_right) == 3
        assert mock_json.loads.call_args_list == [
            call(mock_left.read_text.return_value.strip.return_value),
            call(mock_right.read_text.return_value.strip.return_value),
        ]
        mock_compare.assert_called_once_with(left, right)
        for obj in mock_compare.return_value:
            assert call(obj) in mock_print.call_args_list

    @patch.object(trace_info, "print")
    @patch.object(trace_info, "compare_trace_info")
    @patch.object(trace_info, "json")
    @patch.object(trace_info, "hexify_trace_info")
    def test_diff_trace_info_files_hexify(
        self, mock_hexify, mock_json, mock_compare, mock_print
    ):
        mock_left = MagicMock()
        mock_right = MagicMock()
        left = object()
        right = object()
        mock_hexify.side_effect = [left, right]
        mock_compare.return_value = [object(), object(), object()]
        mock_json.loads.side_effect = loads = [object(), object()]
        assert (
            trace_info._diff_trace_info_files(mock_left, mock_right, hexify=True) == 3
        )
        assert mock_json.loads.call_args_list == [
            call(mock_left.read_text.return_value.strip.return_value),
            call(mock_right.read_text.return_value.strip.return_value),
        ]
        assert mock_hexify.call_args_list == [call(loads[0]), call(loads[1])]
        mock_compare.assert_called_once_with(left, right)
        for obj in mock_compare.return_value:
            assert call(obj) in mock_print.call_args_list

    @patch.object(trace_info, "print")
    @patch.object(trace_info, "compare_trace_info")
    @patch.object(trace_info, "json")
    def test_diff_trace_info_files_eq(self, mock_json, mock_compare, mock_print):
        mock_left = MagicMock()
        mock_right = MagicMock()
        left = object()
        right = object()
        mock_compare.return_value = []
        mock_json.loads.side_effect = [left, right]
        assert trace_info._diff_trace_info_files(mock_left, mock_right) == 0

    @patch("sys.argv", ["trace_info", "pretty", "traceInfo.json"])
    @patch.object(trace_info, "pretty_print_trace_info")
    def test_main_pretty_print(self, mock_pretty):
        assert trace_info.main() == 0
        mock_pretty.assert_called_once_with(Path("traceInfo.json"), hexify=False)

    @patch("sys.argv", ["trace_info", "pretty", "-x", "traceInfo.json"])
    @patch.object(trace_info, "pretty_print_trace_info")
    def test_main_pretty_print_hexify(self, mock_pretty):
        assert trace_info.main() == 0
        mock_pretty.assert_called_once_with(Path("traceInfo.json"), hexify=True)

    @patch("sys.argv", ["trace_info", "diff", "traceInfo-1.json", "traceInfo-2.json"])
    @patch.object(trace_info, "_diff_trace_info_files")
    def test_main_diff(self, mock_diff):
        mock_diff.return_value = 0
        assert trace_info.main() == 0
        mock_diff.assert_called_once_with(
            Path("traceInfo-1.json"), Path("traceInfo-2.json"), hexify=False
        )

    @patch(
        "sys.argv", ["trace_info", "diff", "-x", "traceInfo-1.json", "traceInfo-2.json"]
    )
    @patch.object(trace_info, "_diff_trace_info_files")
    def test_main_diff_hexify(self, mock_diff):
        mock_diff.return_value = 10
        assert trace_info.main() == 10
        mock_diff.assert_called_once_with(
            Path("traceInfo-1.json"), Path("traceInfo-2.json"), hexify=True
        )
