add_library(binrec-rt
        include/binrec/rt/CpuX86.h
        include/binrec/rt/Print.h
        include/binrec/rt/StackTracer.h
        include/binrec/rt/Syscall.h

        src/Debug.cpp
        src/Print.cpp
        src/StackTracer.cpp
        src/Syscall.cpp)  

target_compile_options(binrec-rt PUBLIC -m32 -fno-rtti -fno-exceptions -fno-pic -fno-builtin -O1)
target_include_directories(binrec-rt PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_options(binrec-rt PUBLIC -static-libgcc -lgcc)
