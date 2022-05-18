"""
Binrec C API.
"""
import sys

from . import env  # isort:skip
from . import errors  # isort:skip

# This is a little bit of a hack at the moment so that Binrec Python C modules can
# be imported.
sys.path.append(str(env.BINREC_LIB))

import _binrec_lift as binrec_lift  # noqa: E402
import _binrec_link as binrec_link  # noqa: F401 E402

__all__ = ["binrec_lift", "binrec_link", "convert_lib_error"]


def convert_lib_error(error: Exception, message: str) -> errors.BinRecError:
    """
    Convert an exception that occurred within a C module to a more user-friendly
    :class:`~binrec.errors.BinRecError`, or one of its subclasses.

    :param error: exception raised by a binrec C module
    :param message: error message to include in the new exception
    :returns: the user-friendly ``BinRecError`` exception that can be raised
    """
    if isinstance(error, binrec_lift.LiftError):
        # a lifting error has two arguments:
        # 0. The pass where the error occurred
        # 1. The error or assertion that failed
        exc: errors.BinRecError = errors.BinRecLiftingError(
            message, error.args[0], error.args[1]
        )
    else:
        exc = errors.BinRecError(f"{message}: {error}")

    return exc
