cmake_minimum_required(VERSION 3.12)
project(BinRec)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(LLVM 12.0 REQUIRED CONFIG)

add_compile_options(-Wall -pedantic -Wno-comment)

add_subdirectory(binrec-lift)
add_subdirectory(binrec-link)
add_subdirectory(binrec-rt)
add_subdirectory(binrec-traceinfo)
add_subdirectory(binrec-tracemerge)
add_subdirectory(binrec-translator)
add_subdirectory(s2e)

unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
add_subdirectory(test)

add_custom_target(runlib ALL $(MAKE) WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/runlib)
add_dependencies(runlib qemu)
