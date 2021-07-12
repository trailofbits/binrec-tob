#include "binrec/Utils/HeaderDecls.h"
#include <clang-c/Index.h>
#include <clang/AST/GlobalDecl.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/Version.h>
#include <clang/CodeGen/ModuleBuilder.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <dlfcn.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/PrettyStackTrace.h>

using namespace clang;
using namespace llvm;
using namespace std;

namespace {
#define CC_LOOKUP(CLANG_CC, LLVM_CC)                                                                                   \
    { static_cast<size_t>(CLANG_CC), llvm::CallingConv::LLVM_CC }
std::unordered_map<size_t, llvm::CallingConv::ID> ccLookupTable({
    CC_LOOKUP(CC_C, C),
    CC_LOOKUP(CC_X86StdCall, X86_StdCall),
    CC_LOOKUP(CC_X86FastCall, X86_FastCall),
    CC_LOOKUP(CC_X86ThisCall, X86_ThisCall),
    CC_LOOKUP(CC_X86VectorCall, X86_VectorCall),
    CC_LOOKUP(CC_Win64, Win64),
    CC_LOOKUP(CC_X86_64SysV, X86_64_SysV),
    CC_LOOKUP(CC_AAPCS, ARM_AAPCS),
    CC_LOOKUP(CC_AAPCS_VFP, ARM_AAPCS_VFP),
    CC_LOOKUP(CC_IntelOclBicc, Intel_OCL_BI),
    CC_LOOKUP(CC_SpirFunction, SPIR_FUNC),
    CC_LOOKUP(CC_Swift, Swift),
    CC_LOOKUP(CC_PreserveMost, PreserveMost),
    CC_LOOKUP(CC_PreserveAll, PreserveAll),
});
#undef CC_LOOKUP

template <typename T, size_t N> constexpr size_t countof(const T (&)[N]) { return N; }

llvm::CallingConv::ID lookupCallingConvention(clang::CallingConv cc) {
    size_t index = static_cast<size_t>(cc);
    auto ccFound = ccLookupTable.find(index);
    if (ccFound != ccLookupTable.end()) {
        return ccFound->second;
    } else {
        return llvm::CallingConv::C;
    }
}

string getClangResourcesPath() {
    Dl_info info;
    if (dladdr(reinterpret_cast<void *>(clang_createTranslationUnit), &info) == 0) {
        llvm_unreachable("BinRec isn't linked against libclang?!");
    }

    SmallString<128> parentPath = sys::path::parent_path(info.dli_fname);
    sys::path::append(parentPath, "clang", CLANG_VERSION_STRING);
    return parentPath.str();
}

string getSelfPath() {
    Dl_info info;
    if (dladdr(reinterpret_cast<void *>(getSelfPath), &info) == 0) {
        llvm_unreachable("linker doesn't know where executable itself is?!");
    }
    return info.dli_fname;
}

class FunctionDeclarationFinder : public RecursiveASTVisitor<FunctionDeclarationFinder> {
private:
    unordered_map<string, FunctionDecl *> &knownImports;
    unordered_map<uint64_t, HeaderDeclarations::Export> knownExports;

public:
    FunctionDeclarationFinder(unordered_map<string, FunctionDecl *> &knownImports,
                              unordered_map<uint64_t, HeaderDeclarations::Export> &knownExports)
        : knownImports(knownImports), knownExports(knownExports) {}

    bool shouldVisitImplicitCode() { return true; }

    bool TraverseFunctionDecl(FunctionDecl *fn) {
        // MAGIC
        // errs() << "-------------------------\n";
        // errs() << fn->getName().str() << "\n";
        // for (const auto& param : fn->parameters()) {
        //     errs() << QualType::getAsString(param->getType().split(), PrintingPolicy{{}}) << "\n";
        // }
        // errs() << "-------------------------\n";
        return true;
    }
};

} // namespace

HeaderDeclarations::HeaderDeclarations(llvm::Module &module, std::unique_ptr<clang::ASTUnit> tu,
                                       std::vector<std::string> includedFiles)
    : module(module), tu(move(tu)), includedFiles(move(includedFiles)) {}

Function *HeaderDeclarations::prototypeForDeclaration(FunctionDecl &decl) {
    llvm::FunctionType *functionType = typeLowering->GetFunctionType(GlobalDecl(&decl));

    // Cheating and bringing in CodeGenTypes is fairly cheap and reliable. Unfortunately, CodeGenModules, which is
    // responsible for attribute translation, is a pretty big class with lots of dependencies.
    // That said, while most attributes have a lot of value for compilation, they don't bring that much in for
    // decompilation.
    AttrBuilder attributeBuilder;
    if (decl.isNoReturn()) {
        attributeBuilder.addAttribute(Attribute::NoReturn);
    }
    if (decl.hasAttr<ConstAttr>()) {
        attributeBuilder.addAttribute(Attribute::ReadNone);
        attributeBuilder.addAttribute(Attribute::NoUnwind);
    }
    if (decl.hasAttr<PureAttr>()) {
        attributeBuilder.addAttribute(Attribute::ReadOnly);
        attributeBuilder.addAttribute(Attribute::NoUnwind);
    }
    if (decl.hasAttr<NoAliasAttr>()) {
        attributeBuilder.addAttribute(Attribute::ArgMemOnly);
        attributeBuilder.addAttribute(Attribute::NoUnwind);
    }

    Function *fn = Function::Create(functionType, GlobalValue::ExternalLinkage);
    fn->addAttributes(AttributeList::FunctionIndex, attributeBuilder);
    if (decl.hasAttr<RestrictAttr>()) {
        fn->addAttribute(AttributeList::ReturnIndex, Attribute::NoAlias);
    }

    // If we know the calling convention, apply it here
    auto prototype = decl.getType()->getCanonicalTypeUnqualified().getAs<FunctionProtoType>();
    auto callingConvention = lookupCallingConvention(prototype->getExtInfo().getCC());

    fn->setCallingConv(callingConvention);
    module.getFunctionList().insert(module.getFunctionList().end(), fn);
    return fn;
}

std::unique_ptr<HeaderDeclarations> HeaderDeclarations::create(llvm::Module &module, const vector<string> &searchPath,
                                                               std::vector<std::string> headers,
                                                               llvm::raw_ostream &errors) {
    if (headers.size() == 0) {
        return unique_ptr<HeaderDeclarations>(new HeaderDeclarations(module, nullptr, std::move(headers)));
    }

    string includeContent;
    raw_string_ostream includer(includeContent);
    for (const auto &header : headers) {
        includer << "#include \"" << header << "\"\n";
    }
    includer.flush();

    if (auto includeBuffer = MemoryBuffer::getMemBuffer(includeContent, "<binrec>")) {
        auto diagOpts = std::make_unique<DiagnosticOptions>();
        diagOpts->TabStop = 4;
        auto diagPrinter = new TextDiagnosticPrinter(errors, diagOpts.get());
        auto diags = CompilerInstance::createDiagnostics(diagOpts.release(), diagPrinter);

        shared_ptr<CompilerInvocation> clang;
        {
            vector<const char *> invocationArgs = {getSelfPath().c_str(), "-x", "c", "<binrec>"};
            auto argsArrayRef = makeArrayRef(&*invocationArgs.begin(), &*invocationArgs.end());
            clang = createInvocationFromCommandLine(argsArrayRef, diags);
        }

        if (clang) {
            clang->getLangOpts()->SpellChecking = false;
            clang->getTargetOpts().Triple = module.getTargetTriple();

            auto &searchOpts = clang->getHeaderSearchOpts();
            searchOpts.ResourceDir = getClangResourcesPath();

            for (const auto &includeDir : searchPath) {
                searchOpts.AddPath(includeDir, frontend::System, false, true);
            }

            searchOpts.AddPath(".", frontend::System, false, true);

            auto &preprocessorOpts = clang->getPreprocessorOpts();
            preprocessorOpts.addRemappedFile("<binrec>", includeBuffer.release());

            auto pch = std::make_shared<PCHContainerOperations>();
            auto tu =
                ASTUnit::LoadFromCompilerInvocation(clang, pch, diags, new FileManager(FileSystemOptions()), true);

            if (tu) {
                unique_ptr<HeaderDeclarations> result(new HeaderDeclarations(module, move(tu), move(headers)));
                if (CodeGenerator *codegen =
                        CreateLLVMCodeGen(*diags, "binrec-headers", clang->getHeaderSearchOpts(), preprocessorOpts,
                                          clang->getCodeGenOpts(), module.getContext())) {
                    codegen->Initialize(result->tu->getASTContext());
                    result->codeGenerator.reset(codegen);
                    result->typeLowering.reset(new CodeGen::CodeGenTypes(codegen->CGM()));
                    FunctionDeclarationFinder visitor(result->knownImports, result->knownExports);
                    visitor.TraverseDecl(result->tu->getASTContext().getTranslationUnitDecl());
                    errors << "Success!\n";
                    return result;
                } else {
                    errors << "Failure!\n";
                }
            } else {
                errors << "Failed to create translation unit.\n";
            }
        } else {
            errors << "Could not create compiler instance.\n";
        }
    } else {
        errors << "Could not create MemBuffer.\n";
    }
    return nullptr;
}

Function *HeaderDeclarations::prototypeForImportName(const string &importName) {
    if (Function *fn = module.getFunction(importName)) {
        return fn;
    }

    auto iter = knownImports.find(importName);
    if (iter == knownImports.end()) {
        return nullptr;
    }

    return prototypeForDeclaration(*iter->second);
}

Function *HeaderDeclarations::prototypeForAddress(uint64_t address) {
    auto iter = knownExports.find(address);
    if (iter == knownExports.end()) {
        return nullptr;
    }

    return prototypeForDeclaration(*iter->second.decl);
}

vector<uint64_t> HeaderDeclarations::getVisibleEntryPoints() const {
    vector<uint64_t> entryPoints;
    for (const auto &pair : knownExports) {
        entryPoints.push_back(pair.first);
    }
    std::sort(entryPoints.begin(), entryPoints.end());
    return entryPoints;
}

const SymbolInfo *HeaderDeclarations::getInfo(uint64_t address) const {
    auto iter = knownExports.find(address);
    return iter == knownExports.end() ? nullptr : &iter->second;
}

HeaderDeclarations::~HeaderDeclarations() {}
