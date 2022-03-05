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
    auto address_type = IntegerType::get(m.getContext(), 32);
    auto global_var =
        new GlobalVariable(m, address_type, false, GlobalValue::ExternalLinkage, nullptr, symbol);

    INFO(
        "creating global variable: " << symbol << " at address 0x" << utohexstr(import_address)
                                     << ", size 0x" << utohexstr(symbol_size));
    auto value = ConstantInt::get(address_type, import_address);
    vector<ConstantExpr *> const_exprs;
    vector<pair<Instruction *, unsigned>> instructions;

    for (Use &use : value->uses()) {
        DBG("Found reference to global symbol: " << *use);
        if (auto *inst = dyn_cast<Instruction>(use.getUser())) {
            // An instructions is referencing the symbol value. All globals are
            // pointers, so we need to cast the pointer to an appropriate integer type,
            // address_type.
            instructions.emplace_back(inst, use.getOperandNo());
        } else if (auto *expr = dyn_cast<ConstantExpr>(use.getUser())) {
            // A constant expression using the symbol value, which will typically be a
            // inttoptr expression. This will replace the expression with a dereference
            // to the global value.
            const_exprs.push_back(expr);
        }
    }

    for (auto expr : const_exprs) {
        DBG("replacing constexpr: " << *expr << " with global variable " << symbol);
        expr->replaceAllUsesWith(global_var);
    }

    for (auto item : instructions) {
        auto ptrtoint = new PtrToIntInst(global_var, address_type, "", item.first);
        DBG("replacing instruction: " << *item.first << " with global variable: " << symbol);
        item.first->setOperand(item.second, ptrtoint);
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
