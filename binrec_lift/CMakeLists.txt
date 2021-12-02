add_executable(binrec_lift
        src/analysis/env_alias_analysis.cpp src/analysis/env_alias_analysis.hpp
        src/analysis/trace_info_analysis.cpp src/analysis/trace_info_analysis.hpp

        src/debug/call_tracer.cpp src/debug/call_tracer.hpp

        src/ir/register.hpp
        src/ir/selectors.cpp src/ir/selectors.hpp

        src/lifting/add_debug.cpp src/lifting/add_debug.hpp
        src/lifting/add_mem_array.cpp src/lifting/add_mem_array.hpp
        src/lifting/elf_symbols.cpp src/lifting/elf_symbols.hpp
        src/lifting/extern_plt.cpp src/lifting/extern_plt.hpp
        src/lifting/fix_cfg.cpp src/lifting/fix_cfg.hpp
        src/lifting/fix_overlaps.cpp src/lifting/fix_overlaps.hpp
        src/lifting/global_env_to_alloca.cpp src/lifting/global_env_to_alloca.hpp
        src/lifting/implement_lib_call_stubs.cpp src/lifting/implement_lib_call_stubs.hpp
        src/lifting/implement_lib_calls_new_plt.cpp src/lifting/implement_lib_calls_new_plt.hpp
        src/lifting/inline_lib_call_args.cpp src/lifting/inline_lib_call_args.hpp
        src/lifting/inline_qemu_op_helpers.cpp src/lifting/inline_qemu_op_helpers.hpp
        src/lifting/inline_stubs.cpp src/lifting/inline_stubs.hpp
        src/lifting/insert_calls.cpp src/lifting/insert_calls.hpp
        src/lifting/insert_tramp_for_rec_funcs.cpp src/lifting/insert_tramp_for_rec_funcs.hpp
        src/lifting/internalize_functions.cpp src/lifting/internalize_functions.hpp
        src/lifting/internalize_globals.cpp src/lifting/internalize_globals.hpp
        src/lifting/lib_call_new_plt.cpp src/lifting/lib_call_new_plt.hpp
        src/lifting/pc_jumps.cpp src/lifting/pc_jumps.hpp
        src/lifting/prune_libargs_push.cpp src/lifting/prune_libargs_push.hpp
        src/lifting/prune_null_succs.cpp src/lifting/prune_null_succs.hpp
        src/lifting/prune_retaddr_push.cpp src/lifting/prune_retaddr_push.hpp
        src/lifting/prune_trivially_dead_succs.cpp src/lifting/prune_trivially_dead_succs.hpp
        src/lifting/recov_func_trampolines.cpp src/lifting/recov_func_trampolines.hpp
        src/lifting/recover_functions.cpp src/lifting/recover_functions.hpp
        src/lifting/remove_metadata.cpp src/lifting/remove_metadata.hpp
        src/lifting/remove_opt_none.cpp src/lifting/remove_opt_none.hpp
        src/lifting/remove_s2e_helpers.cpp src/lifting/remove_s2e_helpers.hpp
        src/lifting/replace_dynamic_symbols.cpp src/lifting/replace_dynamic_symbols.hpp
        src/lifting/successor_lists.cpp src/lifting/successor_lists.hpp
        src/lifting/unalign_stack.cpp src/lifting/unalign_stack.hpp

        src/lowering/halt_on_declarations.cpp src/lowering/halt_on_declarations.hpp
        src/lowering/internalize_debug_helpers.cpp src/lowering/internalize_debug_helpers.hpp
        src/lowering/remove_sections.cpp src/lowering/remove_sections.hpp

        src/merging/decompose_env.cpp src/merging/decompose_env.hpp
        src/merging/globalize_env.cpp src/merging/globalize_env.hpp
        src/merging/prune_redundant_basic_blocks.cpp src/merging/prune_redundant_basic_blocks.hpp
        src/merging/externalize_functions.cpp src/merging/externalize_functions.hpp
        src/merging/internalize_imports.cpp src/merging/internalize_imports.hpp
        src/merging/rename_block_funcs.cpp src/merging/rename_block_funcs.hpp
        src/merging/rename_env.cpp src/merging/rename_env.hpp
        src/merging/unflatten_env.cpp src/merging/unflatten_env.hpp
        src/merging/unimplement_custom_helpers.cpp src/merging/unimplement_custom_helpers.hpp

        src/object/function_renaming.cpp src/object/function_renaming.hpp
        src/object/global_address_map.cpp src/object/global_address_map.hpp

        src/utils/entry_points.hpp
        src/utils/function_info.cpp src/utils/function_info.hpp
        src/utils/intrinsic_cleaner.cpp src/utils/intrinsic_cleaner.hpp
        src/utils/name_cleaner.cpp src/utils/name_cleaner.hpp

        src/add_custom_helper_vars.cpp src/add_custom_helper_vars.hpp
        src/binrec_lift.cpp
        src/constant_loads.cpp src/constant_loads.hpp
        src/custom_loop_unroll.cpp src/custom_loop_unroll.hpp
        src/detect_vars.cpp src/detect_vars.hpp
        src/elf_plt_funcs.cpp
        src/env_to_allocas.cpp src/env_to_allocas.hpp
        src/fix_cfg_without_func_data.cpp src/fix_cfg_without_func_data.hpp
        src/inline_wrapper.cpp src/inline_wrapper.hpp
        src/main_utils.cpp src/main_utils.hpp
        src/meta_utils.cpp src/meta_utils.hpp
        src/pass_utils.cpp src/pass_utils.hpp
        src/pc_utils.cpp src/pc_utils.hpp
        src/pe_main.cpp src/pe_main.hpp
        src/pe_reader.cpp src/pe_reader.hpp
        src/pe_sections.cpp src/pe_sections.hpp
        src/remove_calls_to_exception.cpp src/remove_calls_to_exception.hpp
        src/remove_libc_start.cpp src/remove_libc_start.hpp
        src/replace_local_function_pointers.cpp src/replace_local_function_pointers.hpp
        src/section_utils.cpp src/section_utils.hpp
        src/set_data_layout_32.cpp src/set_data_layout_32.hpp
        src/simplify_stack_offsets.cpp src/simplify_stack_offsets.hpp
        src/tag_inst_pc.cpp src/tag_inst_pc.hpp
        src/untangle_interpreter.cpp src/untangle_interpreter.hpp)

target_compile_definitions(binrec_lift PRIVATE ${LLVM_DEFINITIONS})
target_compile_options(binrec_lift PRIVATE -fno-rtti -fno-exceptions)
target_include_directories(binrec_lift PRIVATE ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR}/src)
llvm_map_components_to_libnames(llvm_libs CodeGen Core ipo IRReader Passes ScalarOpts Support TransformUtils)
target_link_options(binrec_lift PRIVATE -fuse-ld=lld)
target_link_libraries(binrec_lift PRIVATE ${llvm_libs} binrec_traceinfo)
