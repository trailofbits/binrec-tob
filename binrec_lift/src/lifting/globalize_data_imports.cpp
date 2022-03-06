#include "globalize_data_imports.hpp"
#include "pass_utils.hpp"
#include <fstream>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

using namespace binrec;
using namespace llvm;
using namespace std;


static void globalize_data_import(
    llvm::Module &m,
    uint64_t import_address,
    uint64_t symbol_size,
    const string &symbol)
{
    INFO(
        "creating global variable: " << symbol << " at address 0x" << utohexstr(import_address)
                                     << ", size 0x" << utohexstr(symbol_size));

    // unsigned pointer_size = m.getDataLayout().getPointerSizeInBits(0);
    auto symbol_type = IntegerType::get(m.getContext(), symbol_size * 8);
    auto value = ConstantInt::get(symbol_type, import_address);
    auto global_var =
        new GlobalVariable(m, symbol_type, false, GlobalValue::ExternalLinkage, nullptr, symbol);

    std::vector<llvm::Use *> work_list;
    for (auto &use : value->uses()) {
        work_list.push_back(&use);
    }

    std::vector<llvm::Use *> instruction_uses;

    // Recursively ascend the usage graph until we collect all transitive uses
    // of this constant by instructions.
    while (!work_list.empty()) {
        auto use = work_list.back();
        work_list.pop_back();

        auto user = use->getUser();
        if (llvm::isa<Instruction>(user)) {
            instruction_uses.push_back(use);
        } else if (auto c = llvm::dyn_cast<llvm::Constant>(user)) {
            for (auto &cuse : c->uses()) {
                work_list.push_back(&cuse);
            }
        } else if (llvm::isa<llvm::GlobalVariable>(user)) {
            // This really shouldn't happen because of the way S2E captures bitcode.
        }
    }

    for (auto use : instruction_uses) {
        auto user = use->getUser();
        auto inst = dyn_cast<Instruction>(user);
        auto op_num = use->getOperandNo();
        auto operand = inst->getOperand(op_num);
        auto op_type = operand->getType();

        if (op_type == global_var->getType()) {
            // no conversion necessary, most likely a inttoptr cast
            DBG("replacing pointer cast with global variable: " << *operand << " -> @" << symbol);
            inst->setOperand(op_num, global_var);
        } else if (op_type == symbol_type) {
            // instruction uses the actual address of the symbol, such as "&stdout"
            // Convert the global pointer to a int and update the instruction to use the
            // result.
            auto ptrtoint = new PtrToIntInst(global_var, symbol_type, "", inst);
            DBG("replacing symbol address with global variable: " << *ptrtoint);
            inst->setOperand(op_num, ptrtoint);
        } else {
            WARNING(
                "unrecgonized destination operand type, may produce invalid bitcode: " << *inst);
        }
    }
}


/**
 * This pass creates an external global variable for each data import and updates each
 * reference to the original data import address to the global variable. This pass is
 * required because S2E loses context of the data imports and instead only references
 * the symbol by the import address, which will not be accurate in the recovered
 * binary.
 *
 * In the S2E captured bitcode, imported data symbols are referenced by their import
 * address, as pointed to by the .dynsym and .rel.dyn ELF tables. This pass handles two
 * cases where the imported symbol is referenced:
 *
 *   - Constant expression, such as `inttoptr SYMBOL_ADDRESS`
 *   - Instructions, such as `load SYMBOL_ADDRESS`
 *
 * An example instruction that is loading the address of a symbol:
 *
 *     ; original:
 *     store i32 134529092, i32* %6, align 4
 *     ; replaced:
 *     @stdout = external global i32
 *     %7 = ptrtoint i32* @stdout to i32
 *     store i32 %7, i32* %6, align 4
 *
 * An example instruction that is dereferencing a symbol address:
 *
 *     ; original:
 *     %16 = load i32, i32* inttoptr (i32 134529056 to i32*), align 32
 *     ; replaced:
 *     %16 = load i32, i32* @stdout, align 4
 */
auto GlobalizeDataImportsPass::run(llvm::Module &m, llvm::ModuleAnalysisManager &am)
    -> llvm::PreservedAnalyses
{
    ifstream F;
    uint64_t import_address = 0;
    uint64_t symbol_size = 0;
    string symbol;

    s2eOpen(F, "data_imports");

    while (F >> hex >> import_address >> symbol_size >> symbol) {
        globalize_data_import(m, import_address, symbol_size, symbol);
    }

    return PreservedAnalyses::none();
}
