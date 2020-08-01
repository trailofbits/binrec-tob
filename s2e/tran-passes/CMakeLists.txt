cmake_minimum_required(VERSION 2.8)
find_package(LLVM)

add_definitions(-DPACKAGE)
add_definitions(-DPACKAGE_VERSION)

# Add glib2 include dirs
#find_package(PkgConfig REQUIRED)
#pkg_check_modules(GLIB_PKG glib-2.0)
#include_directories(${GLIB_PKG_INCLUDE_DIRS})

# If we didn't use -DLLVM_DIR flag for configure command in Makefile,
# we would need the following I think.
#list ( APPEND CMAKE_PREFIX_PATH
#"${ LLVM_ROOT }/ share / llvm / cmake ")


# Define add_llvm_* macro's.
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)
#include(~/binrec-3.8/multicompiler/share/llvm/cmake/AddLLVM.cmake)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

#include_directories(
#    "../qemu"
#    "../qemu/include"
#    "../klee/include"
#    "../../build/qemu-debug"
#    "../../build/qemu-debug/i386-s2e-softmmu"
#    )

add_subdirectory(translator)
