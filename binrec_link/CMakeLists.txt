# Static library for binrec_link, used both by the executable and Python bindings
add_library(binrec_link_static STATIC
        src/compiler_command.cpp src/compiler_command.hpp
        src/elf_exe_to_obj.cpp src/elf_exe_to_obj.hpp
        src/link_context.hpp
        src/link_error.cpp src/link_error.hpp
        src/section_info.hpp
        src/section_link.cpp src/section_link.hpp
        src/stitch.cpp src/stitch.hpp
        src/binrec_link.cpp src/binrec_link.hpp)

target_compile_definitions(binrec_link_static PUBLIC ${LLVM_DEFINITIONS})
target_compile_options(binrec_link_static PUBLIC -fno-rtti -fpic)
target_include_directories(binrec_link_static PUBLIC ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR}/include)
llvm_map_components_to_libnames(llvm_libs core object support)
target_link_libraries(binrec_link_static PUBLIC ${llvm_libs})
set_property(TARGET binrec_link_static PROPERTY POSITION_INDEPENDENT_CODE ON)

# binrec_link executable
add_executable(binrec_link
        src/main.cpp)

target_link_libraries(binrec_link binrec_link_static)

# Python binrec_link module
add_library(pybinrec_link MODULE src/py_binrec_link.cpp)
target_link_libraries(pybinrec_link ${Python_LIBRARIES} binrec_link_static)
target_include_directories(pybinrec_link PRIVATE ${Python_INCLUDE_DIRS})
target_link_options(pybinrec_link PRIVATE ${Python_LINK_OPTIONS})

set_target_properties(
    pybinrec_link
    PROPERTIES
        PREFIX ""
        OUTPUT_NAME "_binrec_link"
        LINKER_LANGUAGE C)

# Doxygen
set(DOXYGEN_PROJECT_NAME binrec_link)
set(DOXYGEN_PROJECT_BRIEF "binrec recovered binary linking")
set(DOXYGEN_GENERATE_TAGFILE ${CMAKE_CURRENT_BINARY_DIR}/binrec_link.tag)

doxygen_add_docs(binrec_link_docs)
