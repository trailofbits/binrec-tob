"""
Binrec C API.
"""
import sys

from . import env  # noqa: F401 isort:skip

# This is a little bit of a hack at the moment so that Binrec Python C modules can
# be imported.
sys.path.append(str(env.BINREC_LIB))

import _binrec_lift as binrec_lift  # noqa: F401 E402
import _binrec_link as binrec_link  # noqa: F401 E402
