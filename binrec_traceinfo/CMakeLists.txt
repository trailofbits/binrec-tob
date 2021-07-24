add_library(binrec_traceinfo
        include/binrec/address.hpp
        include/binrec/byte_unit.hpp
        include/binrec/tracing/call_stack.hpp
        include/binrec/tracing/stack_frame.hpp
        include/binrec/tracing/trace_info.hpp

        src/call_stack.cpp
        src/stack_frame.cpp
        src/trace_info.cpp)

target_include_directories(binrec_traceinfo PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_options(binrec_traceinfo PRIVATE -fPIC -fno-rtti -fno-exceptions)
