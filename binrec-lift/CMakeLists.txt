add_executable(binrec-lift src/binrec-lift.cpp)

target_compile_definitions(binrec-lift PRIVATE ${LLVM_DEFINITIONS})
target_compile_options(binrec-lift PRIVATE -fno-rtti -fno-exceptions)
target_include_directories(binrec-lift PRIVATE ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR}/include)
llvm_map_components_to_libnames(llvm_libs CodeGen Core ipo IRReader Passes ScalarOpts Support TransformUtils)
target_link_options(binrec-lift PRIVATE -fuse-ld=lld)
target_link_libraries(binrec-lift PRIVATE ${llvm_libs} binrec-translator-obj)
