#include "compiler_command.hpp"
#include "link_error.hpp"
#include <llvm/Support/Program.h>

using namespace binrec;
using namespace llvm;
using namespace std;

CompilerCommand::CompilerCommand(CompilerCommandMode mode) : mode{mode} {}

auto CompilerCommand::run() -> error_code
{
    vector<string> arg_buffers;
    arg_buffers.push_back(compiler_exe.str());

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
        arg_buffers.push_back(move(ld_script_arg));
    }

    arg_buffers.emplace_back("-o");
    arg_buffers.emplace_back(output_path.str());

    for (StringRef path : input_paths) {
        arg_buffers.emplace_back(path.str());
    }

    if (mode == CompilerCommandMode::Link) {
        arg_buffers.emplace_back("/usr/lib32/crt1.o");
        arg_buffers.emplace_back("/usr/lib32/crti.o");
        arg_buffers.emplace_back("/usr/lib/gcc/x86_64-linux-gnu/9/32/crtbegin.o");
        arg_buffers.emplace_back("-lstdc++");
        arg_buffers.emplace_back("-lm");
        arg_buffers.emplace_back("-lgcc_eh");
        arg_buffers.emplace_back("-lc");
        arg_buffers.emplace_back("/usr/lib/gcc/x86_64-linux-gnu/9/32/crtend.o");
        arg_buffers.emplace_back("/usr/lib32/crtn.o");
    }

    vector<StringRef> args{arg_buffers.begin(), arg_buffers.end()};
    int status = sys::ExecuteAndWait(compiler_exe, args);
    return status != 0 ? LinkError::CC_Err : error_code();
}
