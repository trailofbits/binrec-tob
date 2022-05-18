import sys
from pathlib import Path
from unittest.mock import patch, MagicMock
from pytest import fixture

#
# The binrec.lib module is mocked as a MagicMock so that any binrec code that imports
# it is not actually calling into the C modules. The mock module can be accessed within
# unit tests using the "mock_lib_module" fixture. Similarly, the real binrec.lib module
# can be accessed with the "real_lib_module" fixture, however the real module may not
# have the actual C modules available if they could not be imported. The real
# binrec.lib module is also in sys.modules, under the name "__real_binrec.lib".
#

try:
    # First try to import the lib module, which also imports the compiled C modules.
    # If this fails then we are most likely running within CI and the modules are
    # unavailable.
    from binrec import lib as lib_module
except ImportError:
    # _binrec_lift and _binrec_link are unavailable, mock them instead
    class MockLiftError(Exception):
        pass

    # first patch the C modules
    with patch.dict(sys.modules, {
        "_binrec_lift": MagicMock(LiftError=MockLiftError),
        "_binrec_link": MagicMock()
    }):
        # import binrec.lib
        from binrec import lib as lib_module


# make sure tests can import conftest if needed
sys.path.append(str(Path(__file__).parent))

# update the patch to sys.modules to make it so binrec.lib is our mock so that unit
# tests don't accidentally call into the C modules. This allows for tests to easily
# mock C modules to ensure calls are being made correctly.
MOCK_LIB_MODULE = MagicMock()

SYS_MODULES = patch.dict(sys.modules, {
    "binrec.lib": MOCK_LIB_MODULE,
    "__real_binrec.lib": lib_module
})
SYS_MODULES.start()


@fixture
def mock_lib_module():
    '''
    Fixture to get the MagicMock patching the binrec.lib module.
    '''
    MOCK_LIB_MODULE.reset_mock(side_effect=True, return_value=True)
    yield MOCK_LIB_MODULE
    MOCK_LIB_MODULE.reset_mock(side_effect=True, return_value=True)


@fixture
def real_lib_module():
    '''
    Fixture to get the real binrec.lib module. The C modules, binrec_lift and
    binrec_link may be MagicMock instances if the modules were unable to be imported.
    '''
    yield lib_module


def pytest_sessionfinish(session, exitstatus):
    SYS_MODULES.stop()
