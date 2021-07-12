add_library(binrec-traceinfo
        include/binrec/Address.h
        include/binrec/ByteUnit.h
        include/binrec/tracing/CallStack.h
        include/binrec/tracing/StackFrame.h
        include/binrec/tracing/TraceInfo.h

        src/CallStack.cpp
        src/StackFrame.cpp
        src/TraceInfo.cpp)

target_include_directories(binrec-traceinfo PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_options(binrec-traceinfo PRIVATE -fPIC -fno-rtti -fno-exceptions)
