def link_prep_1(
    trace_filename: str,
    destination: str,
    working_dir: str = None,
    memssa_check_limit: int = None,
) -> None: ...
def link_prep_2(
    trace_filename: str,
    destination: str,
    working_dir: str = None,
    memssa_check_limit: int = None,
) -> None: ...
def clean(
    trace_filename: str,
    destination: str,
    working_dir: str = None,
    memssa_check_limit: int = None,
) -> None: ...
def lift(
    trace_filename: str,
    destination: str,
    working_dir: str = None,
    clean_names: bool = False,
    skip_link: bool = False,
    trace_calls: bool = False,
    memssa_check_limit: int = None,
) -> None: ...
def optimize(
    trace_filename: str,
    destination: str,
    working_dir: str = None,
    memssa_check_limit: int = None,
) -> None: ...
def optimize_better(
    trace_filename: str,
    destination: str,
    working_dir: str = None,
    memssa_check_limit: int = None,
) -> None: ...
def compile_prep(
    trace_filename: str,
    destination: str,
    working_dir: str = None,
    memssa_check_limit: int = None,
) -> None: ...
