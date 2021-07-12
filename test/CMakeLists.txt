include(ExternalProject)

function(add_c_test_programs prefix)
    foreach(target IN LISTS ARGN)
        add_executable(${prefix}-${target} ${target}.c)
        target_compile_options(${prefix}-${target} PRIVATE -m32 -g -std=c99 -fno-stack-protector -march=pentium)
        set_target_properties(${prefix}-${target} PROPERTIES OUTPUT_NAME ${target} LINK_FLAGS "-m32 -no-pie")
    endforeach(target)
endfunction(add_c_test_programs)

function(add_cpp_test_programs prefix)
    foreach(target IN LISTS ARGN)
        add_executable(${prefix}-${target} ${target}.cpp)
        target_compile_options(${prefix}-${target} PRIVATE -m32 -g -fno-stack-protector)
        set_target_properties(${prefix}-${target} PROPERTIES OUTPUT_NAME ${target} LINK_FLAGS "-m32 -no-pie")
    endforeach(target)
endfunction(add_cpp_test_programs)

add_c_test_programs(test
    ab
    ab2
    abs
    add-vm
    aorb
    args
    atoftest
    bf
    consecutive_calls
    eq
    eq-vm
    eq-vm-2
    eq2
    fib
    fibit
    fibit-vm
    fileptr
    floattest
    fptr
    heap2
    hello
    longjmp
    matmul
    matmul-vm
    pwcheck
    pwcheck-vm
    qsort
    round
    sha2
    sha2-float
    siglongjmp
    simple-c
    strtoftest
)

add_cpp_test_programs(test
    client
    fibpp
    globaloob
    heapover
    server
    simple_class
    stackoob
    uaf
    virtual
)

add_subdirectory(aiethel)

ExternalProject_Add(test-bzip2
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/bzip2
        GIT_REPOSITORY git://sourceware.org/git/bzip2.git
        GIT_TAG bzip2-1.0.8
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make bzip2 "CFLAGS=-m32 -g -fno-stack-protector -march=pentium" LDFLAGS=-no-pie
        BUILD_IN_SOURCE ON
        INSTALL_COMMAND "")
