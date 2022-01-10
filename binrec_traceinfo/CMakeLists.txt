
set(source_files
        include/binrec/address.hpp
        include/binrec/byte_unit.hpp
        include/binrec/tracing/call_stack.hpp
        include/binrec/tracing/stack_frame.hpp
        include/binrec/tracing/trace_info.hpp

        src/call_stack.cpp
        src/stack_frame.cpp
        src/trace_info.cpp)

add_library(binrec_traceinfo ${source_files})

target_include_directories(binrec_traceinfo PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_options(binrec_traceinfo PRIVATE -fPIC -fno-rtti -fno-exceptions)


# Google Tests
add_executable(binrec_traceinfo_test
               test/trace_info_json.cpp
               test/trace_info_merge.cpp)
target_link_libraries(binrec_traceinfo_test gmock_main binrec_traceinfo)
gtest_discover_tests(binrec_traceinfo_test)


# Doxygen
set(DOXYGEN_PROJECT_NAME binrec_traceinfo)
set(DOXYGEN_PROJECT_BRIEF "binrec trace information library to track call stacks, frames, and memory accesses")
set(DOXYGEN_GENERATE_TAGFILE ${CMAKE_CURRENT_BINARY_DIR}/binrec_traceinfo.tag)

doxygen_add_docs(binrec_traceinfo_docs)
