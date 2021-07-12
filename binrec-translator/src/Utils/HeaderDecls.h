#ifndef _HEADER_INDEX_H
#define _HEADER_INDEX_H

#include "EntryPoints.h"
#include "clang/CodeGenTypes.h"
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace clang {
class ASTUnit;
class CodeGenerator;
namespace CodeGen {
// Header for CodeGenTypes pulled from Clang source (nasty!)
class CodeGenTypes;
} // namespace CodeGen
class FunctionDecl;
} // namespace clang

class HeaderDeclarations : public EntryPointProvider {
public:
    struct Export : public SymbolInfo {
        clang::FunctionDecl *decl;
    };

private:
    llvm::Module &module;
    std::unique_ptr<clang::ASTUnit> tu;
    std::unique_ptr<clang::CodeGenerator> codeGenerator;
    std::unique_ptr<clang::CodeGen::CodeGenTypes> typeLowering;

    std::vector<std::string> includedFiles;
    std::unordered_map<std::string, clang::FunctionDecl *> knownImports;
    std::unordered_map<uint64_t, Export> knownExports;

    llvm::Function *prototypeForDeclaration(clang::FunctionDecl &decl);

public:
    static std::unique_ptr<HeaderDeclarations> create(llvm::Module &module, const std::vector<std::string> &searchPath,
                                                      std::vector<std::string> headers, llvm::raw_ostream &errors);

    HeaderDeclarations(llvm::Module &module, std::unique_ptr<clang::ASTUnit> tu,
                       std::vector<std::string> includedFiles);

    const std::vector<std::string> &getIncludedFiles() const { return includedFiles; }
    llvm::Function *prototypeForImportName(const std::string &importName);
    llvm::Function *prototypeForAddress(uint64_t address);

    virtual std::vector<uint64_t> getVisibleEntryPoints() const override;
    virtual const SymbolInfo *getInfo(uint64_t address) const override;

    ~HeaderDeclarations();
};

#endif
