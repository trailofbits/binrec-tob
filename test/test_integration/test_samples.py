import logging
import os
import subprocess
import shutil
import sys
from typing import List, Optional, Tuple
from pathlib import Path
from unittest.mock import MagicMock, patch

from _pytest.python import Metafunc
import pytest

from binrec.env import BINREC_ROOT
from binrec.merge import merge_traces
from binrec import lift, project
from binrec.batch import BatchTraceParams, TraceParams

TEST_SAMPLE_SOURCES = ("binrec", "coreutils", "debian")
TEST_SAMPLES_DIR = BINREC_ROOT / "test" / "benchmark" / "samples"
TEST_BUILD_DIR = BINREC_ROOT / "test" / "benchmark" / "samples" / "bin" / "x86"

logger = logging.getLogger("binrec.test.test_samples")


def load_sample_test_cases(func: callable) -> callable:
    """
    Load sample test cases from the test input directory.
    """
    test_plans: List[Tuple[Path, Path]] = []

    for source in TEST_SAMPLE_SOURCES:
        input_dir = TEST_SAMPLES_DIR / source / "test_inputs"
        if not input_dir.is_dir():
            continue

        build_dir = TEST_BUILD_DIR / source
        if not build_dir.is_dir():
            continue

        for filename in sorted(os.listdir(input_dir)):
            binary = build_dir / filename
            if binary.suffix.lower() == ".json":
                binary = binary.with_suffix("")

            if not binary.is_file():
                continue

            test_plans.append((binary, input_dir / filename))

    func.test_plans = test_plans
    return func


def pytest_generate_tests(metafunc: Metafunc):
    """
    This method is called by pytest to generate test cases for this module.
    """
    if isinstance(sys.modules["__real_binrec.lib"].binrec_lift, MagicMock):
        pytest.skip(
            "Unable to run integration tests: _binrec_lift module is unavailable."
        )

    if isinstance(sys.modules["__real_binrec.lib"].binrec_link, MagicMock):
        pytest.skip(
            "Unable to run integration tests: _binrec_link module is unavailable."
        )

    test_plans: Optional[List[Tuple[Path, Path]]] = getattr(
        metafunc.function, "test_plans", None
    )
    if test_plans:
        metafunc.parametrize(
            "binary,test_plan_file",
            test_plans,
            ids=["_".join(binary.parts[-2:]) for binary, _ in test_plans],
        )


@pytest.mark.flaky(reruns=1)
@load_sample_test_cases
def test_sample(binary: Path, test_plan_file: Path, real_lib_module):
    plan = BatchTraceParams.load(binary, test_plan_file)
    patch_body = {
        name: getattr(real_lib_module, name) for name in real_lib_module.__all__
    }
    with patch.multiple(lift, **patch_body):
        run_test(plan)


def run_test(plan: BatchTraceParams) -> None:
    """
    Run a test binary, merge results, and lift the results. The binary may be executed
    multiple times depending on how many items are in the ``plan.traces`` parameter.

    :param plan: trace test plan containing list of command line arguments and stdin
        input to use
    """
    # Check for pre-existing project folder, remove if found
    project_dir = project.project_dir(plan.project)
    if os.path.isdir(project_dir):
        shutil.rmtree(project_dir, ignore_errors=True)

    project_dir = project.new_project(plan.project, plan.binary)

    project.run_project_batch_params(plan)

    # Merge traces
    merge_traces(plan.project)

    # Lift
    lift.lift_trace(plan.project)

    logger.info("successfully ran and merged %d traces; verifying recovered binary",
                len(plan.traces))
    project.validate_lift_result_batch_params(plan)
    logger.info("verified recovered binary")
