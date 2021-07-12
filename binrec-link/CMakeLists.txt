add_executable(binrec-link
        src/binrec-link.cpp
        src/CompilerCommand.cpp
        src/CompilerCommand.h
        src/ElfExeToObj.cpp
        src/ElfExeToObj.h
        src/LinkContext.h
        src/LinkError.cpp
        src/LinkError.h
        src/SectionInfo.h
        src/SectionLink.cpp
        src/SectionLink.h
        src/Stitch.cpp
        src/Stitch.h)

target_compile_definitions(binrec-link PRIVATE ${LLVM_DEFINITIONS})
target_compile_options(binrec-link PRIVATE -fno-rtti -fno-exceptions)
target_include_directories(binrec-link PRIVATE ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR}/include)
llvm_map_components_to_libnames(llvm_libs core object support)
target_link_libraries(binrec-link PRIVATE ${llvm_libs})
