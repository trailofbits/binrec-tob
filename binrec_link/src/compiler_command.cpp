#include "compiler_command.hpp"
#include "link_error.hpp"
#include <cstdlib>
#include <llvm/Support/Program.h>
#include <llvm/Support/raw_ostream.h>

using namespace binrec;
using namespace llvm;
using namespace std;


static const char *ENV_GCC_LIB = getenv("GCC_LIB");
static const string GCC_LIB = ENV_GCC_LIB ? ENV_GCC_LIB : "";

CompilerCommand::CompilerCommand(CompilerCommandMode mode) : harden{0}, mode{mode} {}

auto CompilerCommand::run() -> error_code
{
    if (!GCC_LIB.length()) {
        errs() << "error: GCC_LIB environment variable not set\n";
        return LinkError::CC_Err;
    }

    vector<string> arg_buffers;
    // TODO this function assumes 32-bit, add support for 64-bit
    string gcc32_path = GCC_LIB + "/32";

    arg_buffers.push_back(compiler_exe);

    arg_buffers.emplace_back("-g");
    arg_buffers.emplace_back("-m32");
    if (mode == CompilerCommandMode::Compile) {
        arg_buffers.emplace_back("-c");
    } else if (mode == CompilerCommandMode::Link) {
        arg_buffers.emplace_back("-no-pie");
        // FPar: we want to link standard libs manually so we can overwrite symbols from those with
        // symbols from the original binary. (Order matters for ld, and usually standard libraries
        // are put before your own objects when using gcc to invoke the linker)
        arg_buffers.emplace_back("-nostdlib");
        arg_buffers.emplace_back("-Wl,--allow-multiple-definition");
    }

    if (!linker_script_path.empty()) {
        string ld_script_arg{"-Wl,-T"};
        ld_script_arg += linker_script_path;
        arg_buffers.push_back(ld_script_arg);
    }

    if (harden) {
        errs() << "DEBUG: Applying ASan, UBsan, and LSan before recompilation.\n";
        arg_buffers.emplace_back("-fsanitize=address,undefined,leak");
    }

    arg_buffers.emplace_back("-o");
    arg_buffers.emplace_back(output_path);

    for (auto path : input_paths) {
        arg_buffers.emplace_back(path);
    }

    if (mode == CompilerCommandMode::Link) {
        arg_buffers.emplace_back("/usr/lib32/crt1.o");
        arg_buffers.emplace_back("/usr/lib32/crti.o");
        arg_buffers.emplace_back(gcc32_path + "/crtbegin.o");
        arg_buffers.emplace_back("-lstdc++");
        arg_buffers.emplace_back("-lm");
        arg_buffers.emplace_back("-lgcc_eh");
        arg_buffers.emplace_back("-lc");
        arg_buffers.emplace_back(gcc32_path + "/crtend.o");
        arg_buffers.emplace_back("/usr/lib32/crtn.o");
    }

    vector<StringRef> args{arg_buffers.begin(), arg_buffers.end()};
    int status = sys::ExecuteAndWait(compiler_exe, args);
    return status != 0 ? LinkError::CC_Err : error_code();
}
