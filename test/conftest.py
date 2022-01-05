import sys
from pathlib import Path
from unittest.mock import patch, MagicMock
from pytest import fixture

from binrec.env import BINREC_LIB


MOCK_LIB_MODULE = MagicMock()

#
# We don't want to actually load any of the compiled C modules in the tests. So, we
# patch sys.modules to include a MagicMock as the "binrec.lib" module. Getting access
# to this module can be done in a test via the "mock_lib_module" fixture.
#
# We do this so that the unit tests will run without building the Binrec and C modules.
# The underlying C modules can still be imported manually.
#
SYS_MODULES = patch.dict(sys.modules, {"binrec.lib": MOCK_LIB_MODULE})
SYS_MODULES.start()

sys.path.append(str(Path(__file__).parent))
sys.path.append(str(BINREC_LIB))


@fixture
def mock_lib_module():
    '''
    Fixture to get the MagicMock patching the binrec.lib module.
    '''
    MOCK_LIB_MODULE.reset_mock(side_effect=True, return_value=True)
    yield MOCK_LIB_MODULE
    MOCK_LIB_MODULE.reset_mock(side_effect=True, return_value=True)


def pytest_sessionfinish(session, exitstatus):
    SYS_MODULES.stop()
