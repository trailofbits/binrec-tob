cmake_minimum_required(VERSION 2.8)
project(BinRec)

set(CMAKE_CXX_STANDARD 11)
set(S2EDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(COMPOSE_PROJECT_NAME $ENV{USER})
set(VNCPORT 8900)
set(LLVM_DIR ${S2EDIR}/build/llvm-cmake/share/llvm/cmake)

add_subdirectory(s2e/tran-passes)