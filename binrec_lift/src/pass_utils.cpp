#include "pass_utils.hpp"
#include <fstream>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IRReader/IRReader.h>

using namespace llvm;

namespace logging {
    static raw_null_ostream dummy;

    static const char *const levelStrings[] = {"ERROR", "WARNING", "INFO", "DEBUG"};

    auto getStream(logging::Level level) -> raw_ostream &
    {
        if (level > logLevel)
            return dummy;

        errs() << "[" << levelStrings[level] << "] ";
        return errs();
    }
} // namespace logging

cl::opt<unsigned> debugVerbosity(
    "debug-verbosity",
    cl::desc("Verbosity of generated code (0: default, 1: basic log, 2: register log"),
    cl::value_desc("level"));

cl::opt<logging::Level> logLevel(
    "loglevel",
    cl::desc("Logging level"),
    cl::values(
        clEnumValN(logging::ERROR, "error", "Only show errors"),
        clEnumValN(logging::WARNING, "warning", "Show warnings"),
        clEnumValN(
            logging::INFO,
            "info",
            "(default) Show some informative messages about what passes are doing"),
        clEnumValN(logging::DEBUG, "debug", "Show extensive debug messages")),
    cl::initializer<logging::Level>(logging::INFO));

cl::opt<fallback::Mode> fallbackMode(
    "fallback-mode",
    cl::desc("How to handle edges that are not recorded in successor lists"),
    cl::values(
        clEnumValN(
            fallback::NONE,
            "none",
            "Make error block unreachable, allows most optimization"
            " (assumes a single code path)"),
        clEnumValN(
            fallback::ERROR1,
            "error1",
            "Print prevPC and PC values when execution diverged."),
        clEnumValN(fallback::ERROR, "error", "Print PC value to which execution diverged."),
        clEnumValN(fallback::BASIC, "basic", "Fallback for any unknown edge"),
        clEnumValN(
            fallback::EXTENDED,
            "extended",
            "(default) Basic fallback + jump table for unknown edges with"
            " target blocks that are in the recovered set"),
        clEnumValN(
            fallback::UNFALLBACK,
            "unfallback",
            "extended + try to return to recovered code")),
    cl::initializer<fallback::Mode>(fallback::NONE));

cl::list<unsigned> breakAt(
    "break-at",
    cl::desc("Basic block addresses to insert debugging breakpoints at"),
    cl::value_desc("address"));

auto checkif(bool condition, const std::string &message) -> bool
{
    if (!condition)
        errs() << "Check failed: " << message << '\n';

    return condition;
}

void failUnless(bool condition, const std::string &message)
{
    if (!condition) {
        errs() << "Check failed: " << message << '\n';
        exit(1);
    }
}

auto s2eRoot() -> std::string
{
    const char *envval = getenv("S2EDIR");
    if (!envval) {
        errs() << "missing S2EDIR environment variable\n";
        exit(1);
    }
    return std::string(envval);
}

auto runlibDir() -> std::string
{
    return s2eRoot() + "/../runlib";
}

auto s2eOutFile(const std::string &basename) -> std::string
{
    return basename;
}

auto fileOpen(std::ifstream &f, const std::string &path, bool failIfMissing) -> bool
{
    f.open(path);

    if (failIfMissing)
        failUnless(f.is_open(), "could not open " + path);

    return f.is_open();
}

auto s2eOpen(std::ifstream &f, const std::string &basename, bool failIfMissing) -> bool
{
    return fileOpen(f, s2eOutFile(basename), failIfMissing);
}

void doInlineAsm(
    IRBuilder<> &b,
    StringRef asmText,
    StringRef constraints,
    bool hasSideEffects,
    ArrayRef<Value *> args)
{
    std::vector<Type *> argTypes(args.size());

    for (unsigned i = 0, iUpperBound = args.size(); i < iUpperBound; ++i)
        argTypes[i] = args[i]->getType();

    b.CreateCall(
        InlineAsm::get(
            FunctionType::get(b.getVoidTy(), argTypes, false),
            asmText,
            constraints,
            hasSideEffects),
        args);
}

auto loadBitcodeFile(StringRef path, LLVMContext &ctx) -> std::unique_ptr<Module>
{
    SMDiagnostic err;
    std::unique_ptr<Module> from = parseIRFile(path, err, ctx);

    if (!from) {
        err.print("could not load bitcode file", errs());
        exit(1);
    }

    return from;
}

void createJumpTableEntry(SwitchInst *jumpTable, BasicBlock *bb)
{
    ConstantInt *addr = ConstantInt::get(Type::getInt32Ty(bb->getContext()), getBlockAddress(bb));
    jumpTable->addCase(addr, bb);
    DBG("called create entry");
}
