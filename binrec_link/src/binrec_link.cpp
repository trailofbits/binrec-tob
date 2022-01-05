#include "binrec_link.hpp"
#include "elf_exe_to_obj.hpp"
#include "section_link.hpp"
#include "stitch.hpp"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

using namespace binrec;
using namespace llvm;
using namespace llvm::sys::fs;
using namespace std;


auto binrec::run_link(LinkContext &ctx) -> Error
{
    SmallString<128> original_object_filename = ctx.work_dir;
    original_object_filename += "/original.o";

    auto sections_or_err = elf_exe_to_obj(original_object_filename, ctx);
    if (error_code ec = sections_or_err.getError()) {
        return errorCodeToError(ec);
    }
    auto &sections = sections_or_err.get();

    SmallString<128> temp_output_path = ctx.work_dir;
    temp_output_path += "/output";

    // Link libgcc statically for division builtins
    if (auto err = link_recovered_binary(
            sections,
            ctx.ld_script_filename,
            temp_output_path,
            {ctx.recovered_filename,
             ctx.librt_filename,
             original_object_filename,
             "-static-libgcc",
             "-lgcc"},
            ctx))
    {
        return err;
    }

    if (error_code ec = stitch(temp_output_path, ctx)) {
        return errorCodeToError(ec);
    }

    if (error_code ec = copy_file(temp_output_path, ctx.output_filename)) {
        return errorCodeToError(ec);
    }

    setPermissions(ctx.output_filename, *getPermissions(temp_output_path));

    return Error::success();
}

void binrec::cleanup_link(LinkContext &ctx)
{
    error_code ec;
    for (directory_iterator temp_dir{ctx.work_dir, ec}, temp_dir_end; temp_dir != temp_dir_end;
         temp_dir.increment(ec))
    {
        if (ec) {
            errs() << ec.message() << '\n';
            break;
        }
        ec = remove(temp_dir->path());
        if (ec) {
            errs() << ec.message() << '\n';
        }
    }

    ec = remove(ctx.work_dir);
    if (ec) {
        errs() << ec.message() << '\n';
    }
}
