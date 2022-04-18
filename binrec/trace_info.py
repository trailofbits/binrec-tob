import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Iterator, List, TextIO

#: a transform function that accepts a value and returns a set
SetFunc = Callable[[Any], set]


@dataclass
class TraceInfoDiff:
    """
    A single difference between two trace info files.
    """

    #: the path to the difference in the JSON
    path: str
    #: the side where the item exists ("-" is missing from right side, "+" is missing
    #: from left side)
    side: str
    #: the JSON item that is different
    item: Any

    def __str__(self) -> str:  # pragma: no cover
        return f"{self.side} {self.path} {self.item}"


def _hexify_int_list(items: List[int]) -> List[str]:
    """
    :returns: the list of int items as hex
    """
    return [hex(i) for i in items]


def _hexify_address_tuple_list(items: List[List[int]]) -> List[List[str]]:
    """
    :returns: the list of int items as hex
    """
    return [_hexify_int_list(item) for item in items]


def _hexify_successor_list(items: List[dict]) -> List[dict]:
    """
    :returns: the successor list as hex values
    """
    return [
        {"successor": hex(item["successor"]), "pc": hex(item["pc"])} for item in items
    ]


def _transform_trace_info_item(
    trace_info: dict, path: str, transform: Callable
) -> None:
    """
    Apply a transform to a dictionary nested value.

    .. code-block::

        x = {'a': {'b': [1]}}
        _transform_trace_info_item(x, 'a.b', lambda i: str(i))
        # equivalent to:
        x['a']['b'] = [str(i) for i in x['a']['b']]

    :param trace_info: trace info object
    :param path: the path to the nest key
    :param transform: function that accepts the nested value and returns the
        transformed value
    """
    keys = path.split(".")
    dest = keys.pop()
    target = trace_info
    while keys:
        target = target[keys.pop(0)]

    target[dest] = transform(target[dest])


def _hexify_tbs_list(items: List[list]) -> List[list]:
    """
    :returns: the hex representation of the tbs list
    """
    return [[hex(item[0]), _hexify_int_list(item[1])] for item in items]


def hexify_trace_info(trace_info: dict) -> dict:
    """
    :returns: the hex representation of the trace info object
    """
    ret = dict(trace_info)
    _transform_trace_info_item(
        ret, "functionLog.callerToFollowUp", _hexify_address_tuple_list
    )
    _transform_trace_info_item(
        ret, "functionLog.entryToCaller", _hexify_address_tuple_list
    )
    _transform_trace_info_item(
        ret, "functionLog.entryToReturn", _hexify_address_tuple_list
    )
    _transform_trace_info_item(ret, "functionLog.entries", _hexify_int_list)
    _transform_trace_info_item(ret, "successors", _hexify_successor_list)
    _transform_trace_info_item(ret, "functionLog.entryToTbs", _hexify_tbs_list)

    return ret


def _compare_set(path: str, left: set, right: set) -> Iterator[TraceInfoDiff]:
    """
    Compare two sets, finding items that exist in one and not the other.

    :param path: the item path being compared
    :param left: the left object
    :param right: the right object
    :returns: a generator contains the differences
    """
    for item in left:
        try:
            right.remove(item)
        except KeyError:
            yield TraceInfoDiff(path, "-", item)

    for item in right:
        yield TraceInfoDiff(path, "+", item)


def _get_nested_dict_item(obj: dict, keys: List[str]) -> Any:
    """
    Get a nested item from a dictionary.

    :param obj: source dictionary
    :param keys: list of keys to traverse
    :returns: the nested value or an empty list if the key does not exist
    """
    part, keys = keys[0], keys[1:]
    ret = obj.get(part)
    if ret is None:
        ret = {} if keys else []

    if keys:
        if not isinstance(ret, dict):
            raise ValueError(f"key is not a dictionary: {part}")
        return _get_nested_dict_item(ret, keys)
    return ret


def _compare_trace_info_item(
    path: str, left: dict, right: dict, set_func: SetFunc = set
) -> List[TraceInfoDiff]:
    """
    Compare a portion of two trace info objects.

    :param path: the path in the JSON object to compare (ie. functionLog.entryToReturn)
    :param left: the left trace info
    :param right: the right trace info
    :param set_func: a callable to accepts the left and right list being compared and
        returns a comparable set
    :returns: the list of differences
    """
    parts = path.split(".")
    entry_left = set_func(_get_nested_dict_item(left, parts))
    entry_right = set_func(_get_nested_dict_item(right, parts))
    return list(_compare_set(path, entry_left, entry_right))


def _address_list_to_set(items: List[List[int]]) -> set:
    """
    Convert a list of addresses to a set.
    """
    return set(tuple(item) for item in items)


def _successor_list_to_set(items: List[dict]) -> set:
    """
    Convert the successor list to a set.
    """
    return set((f"successor:{item['successor']}", f"pc:{item['pc']}") for item in items)


def _tbs_list_to_set(items: List[list]) -> set:
    """
    Convert the TBs list to a set.
    """
    return set((item[0], tuple(sorted(item[1]))) for item in items)


def compare_trace_info(left: dict, right: dict) -> List[TraceInfoDiff]:
    """
    Compare two trace info objects.

    :param left: the left trace info
    :param right: the right trace info
    :returns: the list of differences
    """
    result = []
    result += _compare_trace_info_item(
        "functionLog.callerToFollowUp", left, right, _address_list_to_set
    )
    result += _compare_trace_info_item(
        "functionLog.entryToCaller", left, right, _address_list_to_set
    )
    result += _compare_trace_info_item(
        "functionLog.entryToReturn", left, right, _address_list_to_set
    )
    result += _compare_trace_info_item("functionLog.entries", left, right, set)
    result += _compare_trace_info_item(
        "successors", left, right, _successor_list_to_set
    )
    result += _compare_trace_info_item(
        "functionLog.entryToTbs", left, right, _tbs_list_to_set
    )

    return result


def pretty_print_trace_info(
    trace_info_filename: Path, hexify: bool = False, file: TextIO = None
) -> None:
    """
    Pretty print a trace info object.

    :param trace_info_filename: path to the trace info file
    :param hexify: convert the trace info to hex
    :param file: output file stream (default is stdout)
    """
    trace_info = json.loads(trace_info_filename.read_text().strip())
    if hexify:
        trace_info = hexify_trace_info(trace_info)

    print(json.dumps(trace_info, indent=2), file=file)


def _diff_trace_info_files(
    left_filename: Path, right_filename: Path, hexify: bool = False
) -> int:
    """
    Perform a comparison of two trace info files.

    :param left_filename: the left trace info filename
    :param right_filename: the right trace info filename
    :param hexify: convert the trace info objects to hex
    :returns: the number of differences between the two files
    """
    left = json.loads(left_filename.read_text().strip())
    right = json.loads(right_filename.read_text().strip())

    if hexify:
        left = hexify_trace_info(left)
        right = hexify_trace_info(right)

    print("comparing", left_filename, "(-) against", right_filename, "(+)")

    result = compare_trace_info(left, right)
    if not result:
        print("trace info files are equal")
        return 0

    print("found", len(result), "differences:")
    for diff in result:
        print(diff)

    return len(result)


def main() -> int:
    from .core import init_binrec

    init_binrec()

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-v", "--verbose", action="count", help="enable verbose logging"
    )

    subcmd = parser.add_subparsers(dest="subcmd")

    pretty = subcmd.add_parser("pretty")
    pretty.add_argument(
        "-x", "--hex", action="store_true", help="convert addresses to hex"
    )
    pretty.add_argument(
        "trace_info_filename", action="store", type=Path, help="trace info filename"
    )

    diff = subcmd.add_parser("diff")
    diff.add_argument(
        "-x", "--hex", action="store_true", help="convert addresses to hex"
    )
    diff.add_argument(
        "left_filename", action="store", type=Path, help="left trace info filename"
    )
    diff.add_argument(
        "right_filename", action="store", type=Path, help="left trace info filename"
    )

    args = parser.parse_args()
    if args.subcmd == "pretty":
        pretty_print_trace_info(args.trace_info_filename, hexify=args.hex)
        rc = 0
    elif args.subcmd == "diff":
        rc = _diff_trace_info_files(
            args.left_filename, args.right_filename, hexify=args.hex
        )

    return rc


if __name__ == "__main__":  # pragma: no cover
    rc = main()
    sys.exit(rc)
