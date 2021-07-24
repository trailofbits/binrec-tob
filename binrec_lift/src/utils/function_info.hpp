#ifndef BINREC_FUNCTION_INFO_HPP
#define BINREC_FUNCTION_INFO_HPP

#include "analysis/trace_info_analysis.hpp"
#include <cstdint>
#include <llvm/IR/Module.h>
#include <map>
#include <set>
#include <vector>

namespace binrec {
    struct FunctionInfo {
        std::vector<uint32_t> entry_pc;
        std::unordered_map<uint32_t, llvm::DenseSet<uint32_t>> entry_pc_to_return_pcs;
        std::map<uint32_t, std::set<uint32_t>> entry_pc_to_caller_pcs;
        llvm::DenseMap<uint32_t, uint32_t> caller_pc_to_follow_up_pc;
        std::map<uint32_t, std::set<uint32_t>> entry_pc_to_bb_pcs;
        std::map<uint32_t, std::set<uint32_t>> bb_pc_to_entry_pcs;

        explicit FunctionInfo(const TraceInfo &ti);

        [[nodiscard]] auto get_tbs_by_function_entry(llvm::Module &m) const
            -> std::unordered_map<llvm::Function *, llvm::DenseSet<llvm::Function *>>;
        [[nodiscard]] auto get_ret_bbs_by_merged_function(const llvm::Module &m) const
            -> std::map<llvm::Function *, std::set<llvm::BasicBlock *>>;
        [[nodiscard]] auto get_entrypoint_functions(const llvm::Module &m) const
            -> std::vector<llvm::Function *>;
        [[nodiscard]] auto get_ret_pcs_for_entry_pcs() const
            -> std::unordered_map<uint32_t, llvm::DenseSet<uint32_t>>;
        [[nodiscard]] auto get_caller_pc_to_follow_up_pc() const
            -> llvm::DenseMap<uint32_t, uint32_t>;
    };
} // namespace binrec

#endif
