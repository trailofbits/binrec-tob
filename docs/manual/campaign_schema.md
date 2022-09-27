# JSON Schema

BinRec Campaign File Schema

## Properties

- **`setup`** *(array)*: optional list of setup commands to execute before tracing.
  - **Items** *(string)*
- **`teardown`** *(array)*: optional list of teardown commands to execute after tracing.
  - **Items** *(string)*
- **`traces`** *(array)*: List of parameters for the desired traces to be collected.
  - **Items** *(object)*
    - **`name`** *(['string', 'null'])*: name to uniquely identify this trace.
    - **`args`** *(array)*: list of arguments for this trace.
      - **Items**
    - **`setup`** *(array)*: optional list of setup commands to execute before
     this trace.
      - **Items** *(string)*
    - **`teardown`** *(array)*: optional list of teardown commands to execute
     after this trace.
      - **Items** *(string)*
    - **`input_files`** *(array)*: List of files to inject into the execution
    environment other than the target binary.
      - **Items**
    - **`stdin`** *(['string', 'null'])*: Specify input to provide to the
    target binary via stdin.
    - **`match_stdout`**
    - **`match_stderr`**
