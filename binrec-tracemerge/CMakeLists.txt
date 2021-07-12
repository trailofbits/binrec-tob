add_executable(binrec-tracemerge src/main.cpp)
target_link_libraries(binrec-tracemerge PRIVATE binrec-traceinfo)
