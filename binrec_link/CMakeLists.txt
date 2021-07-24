add_executable(binrec_link
        src/binrec_link.cpp
        src/compiler_command.cpp src/compiler_command.hpp
        src/elf_exe_to_obj.cpp src/elf_exe_to_obj.hpp
        src/link_context.hpp
        src/link_error.cpp src/link_error.hpp
        src/section_info.hpp
        src/section_link.cpp src/section_link.hpp
        src/stitch.cpp src/stitch.hpp)

target_compile_definitions(binrec_link PRIVATE ${LLVM_DEFINITIONS})
target_compile_options(binrec_link PRIVATE -fno-rtti -fno-exceptions)
target_include_directories(binrec_link PRIVATE ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR}/include)
llvm_map_components_to_libnames(llvm_libs core object support)
target_link_libraries(binrec_link PRIVATE ${llvm_libs})
