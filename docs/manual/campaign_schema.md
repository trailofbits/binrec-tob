# JSON Schema

BinRec Campaign File Schema

## Properties

- **`setup`** *(array)*: optional list of setup commands to execute before tracing.
  - **Items** *(string)*
- **`teardown`** *(array)*: optional list of teardown commands to execute after tracing.
  - **Items** *(string)*
- **`traces`** *(array)*: List of parameters for the desired traces to be collected.
  - **Items** *(object)*
    - **`name`** *(string)*: name to uniquely identify this trace.
    - **`args`** *(array)*: list of arguments for this trace.
      - **Items** *(object)*
        - **`type`** *(string)*: type of argument: concrete or symbolic.
        - **`value`** *(string)*: value of the concrete argument.
    - **`setup`** *(array)*: optional list of setup commands to execute before
    this trace.
      - **Items** *(string)*
    - **`teardown`** *(array)*: optional list of teardown commands to execute
    after this trace.
      - **Items** *(string)*
    - **`input_files`** *(array)*: List of files to inject into the execution
    environment other than the target binary.
      - **Items** *(object)*
        - **`source`** *(string)*: Location on host system where the input file
        can be found.
        - **`destination`** *(string)*: Location on the guest system where the
        input file should be mapped to for tracing.
        - **`permissions`** *(string)*: Boolean value true to copy the
        permissions from the source file. Can also provide permissions as octal number.
    - **`stdin`** *(string)*: Not yet implemented. Specify input to provide to
    the target binary via stdin.
    - **`match_stdout`** *(string)*: Boolean to match stdout during validation.
    May also be regex to match against.
    - **`match_stderr`** *(string)*: Boolean to match stderr during validation.
    May also be regex to match against.
