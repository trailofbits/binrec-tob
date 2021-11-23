import os
import subprocess
from typing import List, Optional

from _pytest.python import Metafunc

from binrec.env import BINREC_ROOT, BINREC_SCRIPTS

QEMU_DIR = BINREC_ROOT / "qemu"
TEST_INPUT_DIR = BINREC_ROOT / "test" / "benchmark" / "samples" / "binrec" / "test_inputs"
TEST_BUILD_DIR = BINREC_ROOT / "test" / "benchmark" / "samples" / "bin" / "x86" / "binrec"


if not os.environ.get("S2EDIR"):
    # this is needed by the binrec scripts
    os.environ["S2EDIR"] = str(BINREC_ROOT)


def load_sample_test_cases(func: callable) -> callable:
    """
    Load sample test cases from the test input directory.
    """
    sample_names = []

    for filename in sorted(os.listdir(TEST_INPUT_DIR)):
        binary = TEST_BUILD_DIR / filename
        if not binary.is_file():
            continue

        sample_names.append(filename)

    func.sample_names = sample_names
    return func


def pytest_generate_tests(metafunc: Metafunc):
    """
    This method is called by pytest to generate test cases for this module.
    """
    sample_names: Optional[List[str]] = getattr(metafunc.function, "sample_names", None)
    if sample_names:
        metafunc.parametrize("sample_name", sample_names, ids=sample_names)


@load_sample_test_cases
def test_sample(sample_name: str):
    with open(TEST_INPUT_DIR / sample_name, "r") as file:
        # read command line invocations
        invocations = [line.strip().split() for line in file]
        if not invocations:
            # no command line arguments specified, run the test once with no arguments
            invocations = [[]]

    run_test(sample_name, invocations)


def assert_subprocess(*args, assert_error: str = "", **kwargs) -> None:
    """
    Run a subprocess and assert that is exits with a 0 return code.
    """
    proc = subprocess.run(*args, **kwargs)
    assert proc.returncode == 0, assert_error


def compare_lift(binary: str, invocations: List[List[str]]) -> None:
    """
    Compare the original binary against the lifted binary for each test input. This method runs
    the original and the lifted and then compares the process return code, stdout, and stderr
    content. An ``AssertionError`` is raised if any of the comparison criteria does not match
    between the original and lifted sample.

    :param binary: test binary name
    :param invocations: list of command line arguments to run
    :param log_file: binrec log file
    """
    lifted = str(BINREC_ROOT / f"s2e-out-{binary}" / "recovered")
    original = str(BINREC_ROOT / f"s2e-out-{binary}" / "binary")
    target = str(BINREC_ROOT / f"s2e-out-{binary}" / "test-target")

    for args in invocations:
        # We link to the binary we are running to make sure argv[0] is the same for the original
        # and the lifted program.
        os.link(original, target)
        print(">> running original sample with args:", args)
        original_proc = subprocess.Popen(
            [target] + args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdin=subprocess.DEVNULL,
        )
        original_proc.wait()
        os.remove(target)

        original_stdout = original_proc.stdout.read()
        original_stderr = original_proc.stderr.read()

        os.link(lifted, target)
        print(">> running recovered sample with args:", args)
        lifted_proc = subprocess.Popen(
            [target] + args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdin=subprocess.DEVNULL,
        )
        lifted_proc.wait()
        os.remove(target)

        lifted_stdout = lifted_proc.stdout.read()
        lifted_stderr = lifted_proc.stderr.read()

        assert original_proc.returncode == lifted_proc.returncode
        assert original_stdout == lifted_stdout
        assert original_stderr == lifted_stderr


def run_test(binary: str, invocations: List[List[str]]) -> None:
    """
    Run a test binary, merge results, and lift the results. The binary may be executed multiple
    times depending on how many items are in the ``invocations`` parameter. For example:

    .. code-block:: python

        run_test("ab", [
            ['ab'],
            ['zz']
            []
        ])

    This will run three traces of the ``ab`` test binary:

    1. ``ab ab``
    2. ``ab zz``
    3. ``ab``

    The output of S2E and binrec are recorded to the ``log_file``.

    :param binary: test binary name
    :param invocations: list of command line arguments to run
    :param log_file: binrec log file
    """
    cmd_debian = str(QEMU_DIR / "cmd-debian.sh")

    print(">> running", len(invocations), "traces")

    for i, args in enumerate(invocations, start=1):
        print(">> starting trace", i, "with arguments:", args)
        assert_subprocess(
            [cmd_debian, binary] + args,
            stderr=subprocess.STDOUT,
            assert_error=f"trace failed with arguments: {args}",
        )

    # Merge traces
    print(">> merging traces")
    double_merge = str(BINREC_SCRIPTS / "double_merge.sh")
    assert_subprocess(
        [double_merge, binary], stderr=subprocess.STDOUT, assert_error="merge failed"
    )

    # Lift
    print(">> lifting sample")
    lift2 = str(BINREC_SCRIPTS / "lift2.sh")
    assert_subprocess(
        [lift2],
        cwd=str(BINREC_ROOT / f"s2e-out-{binary}"),
        stderr=subprocess.STDOUT,
        assert_error="lift failed",
    )

    print(
        ">> successfully ran and merged",
        len(invocations),
        "traces, verifying recovered binary",
    )
    compare_lift(binary, invocations)
    print(">> verified recovered binary")
