include(ExternalProject)

ExternalProject_Add(llvm-3.2
        GIT_REPOSITORY https://github.com/FPar/llvm-project.git
        GIT_TAG release/3.2.x
        GIT_PROGRESS ON
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ln -s -f <SOURCE_DIR>/clang <SOURCE_DIR>/llvm/tools/clang && <SOURCE_DIR>/llvm/configure --enable-cxx11 --enable-optimized
        BUILD_COMMAND env REQUIRES_RTTI=1 $(MAKE)
        INSTALL_COMMAND "")
ExternalProject_Get_Property(llvm-3.2 SOURCE_DIR)
set(LLVM32_SOURCE_DIR ${SOURCE_DIR})
ExternalProject_Get_Property(llvm-3.2 BINARY_DIR)
set(LLVM32_BINARY_DIR ${BINARY_DIR})

ExternalProject_Add(stp
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/stp
        CONFIGURE_COMMAND cp -r <SOURCE_DIR>/. <BINARY_DIR> && <BINARY_DIR>/scripts/configure --with-fpic
        BUILD_COMMAND $(MAKE)
        INSTALL_COMMAND "")
ExternalProject_Get_Property(stp BINARY_DIR)
set(STP_BINARY_DIR ${BINARY_DIR})

ExternalProject_Add(klee
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/klee
        CONFIGURE_COMMAND env ENABLE_OPTIMIZED=1 <SOURCE_DIR>/configure
        --with-llvmsrc=${LLVM32_SOURCE_DIR}/llvm
        --target=x86_64 --enable-exceptions
        --with-stp=${STP_BINARY_DIR}
        --with-llvmobj=${LLVM32_BINARY_DIR}
        BUILD_COMMAND $(MAKE)
        INSTALL_COMMAND ""
        DEPENDS llvm-3.2 stp)
ExternalProject_Get_Property(klee SOURCE_DIR)
set(KLEE_SOURCE_DIR ${SOURCE_DIR})
ExternalProject_Get_Property(klee BINARY_DIR)
set(KLEE_BINARY_DIR ${BINARY_DIR})

ExternalProject_Add(qemu
        SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/qemu
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/qemu
        CONFIGURE_COMMAND <SOURCE_DIR>/configure
        --disable-werror
        --target-list=i386-s2e-softmmu,i386-softmmu
        --enable-llvm
        --enable-s2e
        --enable-binrec
        --binrec-source=${CMAKE_SOURCE_DIR}
        --binrec-build=${CMAKE_BINARY_DIR}
        --with-pkgversion=S2E
        --disable-virtfs
        --disable-fdt
        --with-llvm=${LLVM32_BINARY_DIR}/Release+Asserts
        --with-stp=${STP_BINARY_DIR}
        --with-klee=${KLEE_BINARY_DIR}/Release+Asserts
        --extra-cflags=-mno-sse3
        "--extra-cxxflags=-mno-sse3 -std=gnu++17"
        BUILD_COMMAND $(MAKE)
        BUILD_ALWAYS ON
        INSTALL_COMMAND ""
        DEPENDS llvm-3.2 stp klee binrec_traceinfo)
