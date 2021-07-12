add_library(binrec-translator-obj OBJECT
        src/Analysis/EnvAliasAnalysis.cpp
        src/Analysis/EnvAliasAnalysis.h
        src/Analysis/TraceInfoAnalysis.cpp
        src/Analysis/TraceInfoAnalysis.h

        src/Debug/CallTracer.cpp
        src/Debug/CallTracer.h
        src/Debug/DotMergedBlocks.cpp
        src/Debug/DotOverlaps.cpp

        src/IR/Register.h
        src/IR/Selectors.cpp
        src/IR/Selectors.h

        src/Lifting/AddDebug.cpp
        src/Lifting/AddMemArray.cpp
        src/Lifting/AddMemArray.h
        src/Lifting/ELFSymbols.cpp
        src/Lifting/ExternPLT.cpp
        src/Lifting/FixCFG.cpp
        src/Lifting/FixOverlaps.cpp
        src/Lifting/GlobalEnvToAlloca.cpp
        src/Lifting/GlobalEnvToAlloca.h
        src/Lifting/ImplementLibCallsNewPLT.cpp
        src/Lifting/ImplementLibCallStubs.cpp
        src/Lifting/InlineLibCallArgs.cpp
        src/Lifting/InlineLibCallArgs.h
        src/Lifting/InlineQemuOpHelpers.cpp
        src/Lifting/InlineStubs.cpp
        src/Lifting/InsertCalls.cpp
        src/Lifting/InsertTrampForRecFuncs.cpp
        src/Lifting/InternalizeFunctions.cpp
        src/Lifting/InternalizeGlobals.cpp
        src/Lifting/LibCallNewPLT.cpp
        src/Lifting/MergeFunctions.cpp
        src/Lifting/PcJumps.cpp
        src/Lifting/PruneLibargsPush.cpp
        src/Lifting/PruneNullSuccs.cpp
        src/Lifting/PruneRetaddrPush.cpp
        src/Lifting/PruneTriviallyDeadSuccs.cpp
        src/Lifting/RecoverFunctions.cpp
        src/Lifting/RecovFuncTrampolines.cpp
        src/Lifting/RemoveMetadata.cpp
        src/Lifting/RemoveOptNone.cpp
        src/Lifting/RemoveOptNone.h
        src/Lifting/RemoveS2EHelpers.cpp
        src/Lifting/RemoveS2EHelpers.h
        src/Lifting/ReplaceDynamicSymbols.cpp
        src/Lifting/SuccessorLists.cpp
        src/Lifting/UnalignStack.cpp

        src/Lowering/HaltOnDeclarations.cpp
        src/Lowering/InternalizeDebugHelpers.cpp
        src/Lowering/RemoveSections.cpp
        src/Lowering/RemoveSections.h

        src/Merging/DecomposeEnv.cpp
        src/Merging/ExternalizeFunctions.cpp
        src/Merging/InternalizeImports.cpp
        src/Merging/RenameBlockFuncs.cpp
        src/Merging/RenameEnv.cpp
        src/Merging/UnflattenEnv.cpp
        src/Merging/UnimplementCustomHelpers.cpp

        src/Object/FunctionRenaming.cpp
        src/Object/FunctionRenaming.h
        src/Object/GlobalAddressMap.cpp
        src/Object/GlobalAddressMap.h

        src/Support/SharedPtrCasting.h

        src/Utils/EntryPoints.h
        src/Utils/FunctionInfo.cpp
        src/Utils/FunctionInfo.h
        #src/Utils/HeaderDecls.cpp
        #src/Utils/HeaderDecls.h

        src/AddCustomHelperVars.cpp
        src/AddCustomHelperVars.h
        src/BinRecTranslator.cpp
        src/ConstantLoads.cpp
        src/ConstantLoads.h
        src/CountBlocks.cpp
        src/CountBlocks.h
        src/CountEdges.cpp
        src/CountEdges.h
        src/CountInsts.cpp
        src/CountInsts.h
        src/CountPcInsts.cpp
        src/CountPcInsts.h
        src/CustomLoopUnroll.cpp
        src/CustomLoopUnroll.h
        src/CyclomaticComplexity.cpp
        src/CyclomaticComplexity.h
        src/DebugUtils.cpp
        src/DebugUtils.h
        src/DetectVars.cpp
        src/DetectVars.h
        src/ELFPLTFuns.cpp
        src/EnvToAllocas.cpp
        src/EnvToAllocas.h
        src/FixCFGWithoutFuncData.cpp
        src/FixCFGWithoutFuncData.h
        src/InlineWrapper.cpp
        src/InlineWrapper.h
        src/InstrumentInterpreter.cpp
        src/InstrumentInterpreter.h
        src/MainUtils.cpp
        src/MainUtils.h
        src/MetaUtils.cpp
        src/MetaUtils.h
        src/MyDotCfg.cpp
        src/MyDotCfg.h
        src/PassUtils.cpp
        src/PassUtils.h
        src/PcUtils.cpp
        src/PcUtils.h
        src/PEMain.cpp
        src/PEMain.h
        src/PEReader.cpp
        src/PEReader.h
        src/PESections.cpp
        src/PESections.h
        src/RegisterPasses.cpp
        src/RegisterPasses.h
        src/RemoveBlocks.cpp
        src/RemoveBlocks.h
        src/RemoveCallsToException.cpp
        src/RemoveCallsToException.h
        src/RemoveLibcStart.cpp
        src/RemoveLibcStart.h
        src/ReplaceLocalFunctionPointers.cpp
        src/ReplaceLocalFunctionPointers.h
        src/ReplaceStackVars.cpp
        src/ReplaceStackVars.h
        src/SectionUtils.cpp
        src/SectionUtils.h
        src/SegfaultDebug.cpp
        src/SegfaultDebug.h
        src/SetDataLayout32.cpp
        src/SetDataLayout32.h
        src/SimplifyStackOffsets.cpp
        src/SimplifyStackOffsets.h
        src/TagInstPc.cpp
        src/TagInstPc.h
        src/UntangleInterpreter.cpp
        src/UntangleInterpreter.h)

target_compile_definitions(binrec-translator-obj PUBLIC ${LLVM_DEFINITIONS})
target_compile_options(binrec-translator-obj PUBLIC -fno-rtti -fno-exceptions)
target_include_directories(binrec-translator-obj PUBLIC ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(binrec-translator-obj PUBLIC binrec-traceinfo)
set_target_properties(binrec-translator-obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(binrec-translator MODULE src/BinRecTranslator.cpp)
target_link_options(binrec-translator PRIVATE -fuse-ld=lld)
target_link_libraries(binrec-translator PRIVATE binrec-translator-obj)
