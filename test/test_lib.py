import sys

from binrec.errors import BinRecError, BinRecLiftingError
from binrec.env import BINREC_LIB


class TestLib:

    def test_imports(self):
        assert str(BINREC_LIB) in sys.path

    def test_convert_lib_error(self, real_lib_module):
        err = real_lib_module.convert_lib_error('asdf', Exception('qwer'))
        assert str(err) == 'qwer: asdf'
        assert isinstance(err, BinRecError)
        assert not isinstance(err, BinRecLiftingError)

    def test_convert_lib_error_lifting(self, real_lib_module):
        err = real_lib_module.convert_lib_error(real_lib_module.binrec_lift.LiftError('pass_1', 'qwer'), 'asdf')
        assert str(err) == 'asdf: [pass_1] qwer'
        assert isinstance(err, BinRecLiftingError)
