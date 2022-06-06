from unittest.mock import MagicMock
from typing import Union, List


class MockPath:
    """
    A mock Path object, used to emulate a filesystem in unit tests.
    """

    def __init__(
        self,
        path: str = "",
        is_dir: bool = False,
        children: List["MockPath"] = None,
        exists: bool = False,
        **kwargs
    ):
        """
        :param path: the file path or file name
        :param is_dir: the path is a directory
        :children: list of child paths
        :param exists: the file or directory exists
        """
        self.path = path
        if "/" in path:
            self.name = path.rpartition("/")[2]
        else:
            self.name = path

        self._is_dir = is_dir
        self._exists = exists
        self.children = []
        self.is_dir = MagicMock(side_effect=lambda: self._is_dir)
        self.is_file = MagicMock(side_effect=lambda: self._exists and not self._is_dir)
        self.exists = MagicMock(side_effect=lambda: self._exists or self._is_dir)
        self.iterdir = MagicMock(side_effect=lambda: self.children)
        self.symlink_to = MagicMock()
        self.mkdir = MagicMock()

        if children:
            self.build_tree(*children)

        for name, value in kwargs.items():
            object.__setattr__(self, name, value)

    def __getattr__(self, name: str) -> MagicMock:
        attr = MagicMock()
        object.__setattr__(self, name, attr)
        return attr

    def build_tree(self, *paths: List["MockPath"]) -> None:
        """
        Build the directory tree.

        :param paths: list of child files and directories
        """
        for child in paths:
            child.path = f"{self.path}/{child.path}"

        self.children += paths

    def __truediv__(self, other: Union[str, "MockPath"]) -> "MockPath":
        """
        Mirror the "/" operator of the Path object to navigate children.
        """
        if isinstance(other, MockPath):
            other_name = other.name
        else:
            other_name = str(other)

        for child in self.children:
            if child.name == other_name:
                return child

        if isinstance(other, MockPath):
            other.path = f"{self.path}/{other.path}"
            child = other
        else:
            child = MockPath(f"{self.path}/{other}")

        self.children.append(child)
        return child

    def __str__(self) -> str:
        return self.path

    def __repr__(self) -> str:
        return f"MockPath({self.path!r})"

    def __eq__(self, other) -> bool:
        return isinstance(other, MockPath) and other.path == self.path

    def is_absolute(self) -> bool:
        return self.path.startswith(("/", "\\"))

    def __lt__(self, other):
        return isinstance(other, MockPath) and self.path < other.path

    def __gt__(self, other):
        return isinstance(other, MockPath) and self.path > other.path

    def __eq__(self, other):
        return isinstance(other, MockPath) and self.path == other.path

    def __ne__(self, other):
        return isinstance(other, MockPath) and self.path != other.path
