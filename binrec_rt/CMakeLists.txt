add_library(binrec_rt
        include/binrec/rt/cpu_x86.hpp

        src/cpu_x86.cpp
        src/debug.cpp)

target_compile_options(binrec_rt PUBLIC -m32 -fno-rtti -fno-exceptions -fno-pic)
target_include_directories(binrec_rt PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(binrec_rt PRIVATE robin_hood)

# Doxygen
set(DOXYGEN_PROJECT_NAME binrec_rt)
set(DOXYGEN_PROJECT_BRIEF "binrec runtime emulation library")
set(DOXYGEN_GENERATE_TAGFILE ${CMAKE_CURRENT_BINARY_DIR}/binrec_rt.tag)

doxygen_add_docs(binrec_rt_docs)
