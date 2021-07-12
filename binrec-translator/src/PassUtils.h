#ifndef BINREC_PASSUTILS_H
#define BINREC_PASSUTILS_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/CommandLine.h>

namespace logging {
enum Level { ERROR = 0, WARNING, INFO, DEBUG };
auto getStream(Level level) -> llvm::raw_ostream &;
} // namespace logging

namespace fallback {
enum Mode { NONE = 0, ERROR, ERROR1, BASIC, EXTENDED, UNFALLBACK };
} // namespace fallback

// optimization: the if-statement makes this work like llvm's DEBUG macro: if
// the logging level is lower than `level` the `message` argument will not be
// evaluated, saving execution time (since stringification is expensive)
#define LOG(level, message)                                                                                            \
    do {                                                                                                               \
        if (logging::level <= logLevel)                                                                                \
            logging::getStream(logging::level) << message;                                                             \
    } while (false)
#define LOG_LINE(level, message) LOG(level, message << '\n')
#define ERROR(message) LOG_LINE(ERROR, message)
#define WARNING(message) LOG_LINE(WARNING, message)
#define INFO(message) LOG_LINE(INFO, message)
#define DBG(message) LOG_LINE(DEBUG, message)

extern llvm::cl::opt<unsigned> debugVerbosity;
extern llvm::cl::opt<logging::Level> logLevel;
extern llvm::cl::opt<fallback::Mode> fallbackMode;
extern llvm::cl::list<unsigned> breakAt;

static inline auto intToHex(unsigned int num) -> std::string { return llvm::utohexstr(num); }

static inline auto hexToInt(llvm::StringRef hexStr) -> uint32_t { return std::strtoul(hexStr.data(), nullptr, 16); }

static inline auto isRecoveredBlock(llvm::BasicBlock *bb) -> bool {
    return bb->hasName() && bb->getName().startswith("BB_");
}

static inline auto getBlockAddress(llvm::Function *f) -> unsigned { return hexToInt(f->getName().substr(5)); }

static inline auto getBlockAddress(llvm::BasicBlock *bb) -> unsigned { return hexToInt(bb->getName().substr(3)); }

auto getPc(llvm::BasicBlock *bb) -> uint32_t;

void createJumpTableEntry(llvm::SwitchInst *jumpTable, llvm::BasicBlock *bb);

auto checkif(bool condition, const std::string &message) -> bool;

void failUnless(bool condition, const std::string &message = "");

auto s2eRoot() -> std::string;

auto runlibDir() -> std::string;

auto s2eOutFile(const std::string &basename) -> std::string;

auto fileOpen(std::ifstream &f, const std::string &path, bool failIfMissing = true) -> bool;

auto s2eOpen(std::ifstream &f, const std::string &basename, bool failIfMissing = true) -> bool;

void doInlineAsm(llvm::IRBuilder<> &b, llvm::StringRef asmText, llvm::StringRef constraints, bool hasSideEffects,
                 llvm::ArrayRef<llvm::Value *> args);

auto loadBitcodeFile(llvm::StringRef path, llvm::LLVMContext &ctx) -> std::unique_ptr<llvm::Module>;

template <typename T> static auto vectorContains(const std::vector<T> &vec, const T needle) -> bool {
    for (const T val : vec) {
        if (val == needle)
            return true;
    }
    return false;
}

#endif // BINREC_PASSUTILS_H
