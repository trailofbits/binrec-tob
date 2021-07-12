add_c_test_programs(test-aiethel
    all_data_array
    all_globals
    all_stringpool
    all_switch
    argv
    array
    bubblesort
    calc
    complex_double
    complex_float
    complex_long_double
    complex_numbers
    dot_product
    fibonacci
    global_array
    globals_and_io
    helloworld
    local-array
    matrix_vector_mult
    open_close_dir
    pointers
    qsort
    qsort_int_array
    rand_and_strtol
    readdir
    sigabort
    sigalarm
    signal_handler
    sigsegv
    sigsegv_handler
    sigsegv_intx80
    sigsegv_vdso
    simple_array
    simple_exit
    simple_for_loop
    simple_main
    struct
    x86_bts
)

target_link_libraries(test-aiethel-complex_double PRIVATE m)
target_link_libraries(test-aiethel-complex_float PRIVATE m)
target_link_libraries(test-aiethel-complex_long_double PRIVATE m)
target_link_libraries(test-aiethel-complex_numbers PRIVATE m)

add_cpp_test_programs(test-aiethel
    fmodf
    global_var
    inheritance
    iostream_basics
    operator_new
    printf_floats
    pthread
    qsort_function_ptrs
    struct_func_ptr
    virtual
    virtual_simpler
)

target_compile_options(test-aiethel-pthread PRIVATE -pthread)
set_property(TARGET test-aiethel-pthread APPEND_STRING PROPERTY LINK_FLAGS " -pthread")
