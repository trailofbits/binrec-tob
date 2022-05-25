cmake_minimum_required(VERSION 3.12)
project(BinRec)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

find_package(LLVM 14.0 REQUIRED CONFIG)
find_package(Python 3.9 REQUIRED COMPONENTS Interpreter Development)
find_package(Doxygen
             REQUIRED dot
             OPTIONAL_COMPONENTS mscgen dia)

FetchContent_Declare(robin_hood_hashing
    GIT_REPOSITORY https://github.com/martinus/robin-hood-hashing.git
    GIT_TAG 3.11.2
)
FetchContent_MakeAvailable(robin_hood_hashing)

include(GoogleTest)


add_compile_options(-Wall -pedantic -Wno-comment)

add_subdirectory(binrec_lift)
add_subdirectory(binrec_link)
add_subdirectory(binrec_rt)
add_subdirectory(binrec_traceinfo)
add_subdirectory(binrec_tracemerge)

unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)

add_custom_target(runlib ALL $(MAKE) WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/runlib)

add_custom_target(doxygen)
add_dependencies(doxygen
                 binrec_traceinfo_docs
                 binrec_lift_docs
                 binrec_link_docs
                 binrec_rt_docs)
