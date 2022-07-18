# libc-fsigs

Tools and scripts to generate the function signatures for libc. These tools create a `libc-argsizes` database file that contains the list of libc functions in the following form:

```
<func_name> <returns_float?> <return_size> <arg_1_size> <arg_2_size> ... <arg_n_size>
# example:
fopen 0 4 4 4
```

The `libc-argsizes` database is an optimization and binrec will lift library functions even if they are missing from the database. When a function is in the database, binrec will repalce calls to `helper_stub_trampoline`, which eventually calls the library function, to a direct call to the library function.

To rebuild the database, perform the following steps:

1. Change into this directory and run `make`
   ```bash
   cd ./scripts/libc-figs
   make
   ```

   **Note:** if you receive a compilation error saying that `sys/capability.h` does not exist, install `libcap-dev`.

   ```bash
   sudo apt-get install libcap-dev
   ```

2. After the database is built, copy it to the `runlib` directory:
   ```bash
   cp ./argsizes ../runlib/libc-argsizes
   ```

The database is used during lifting so the new database will be used on the next lift operation without needing to rebuild binrec.
