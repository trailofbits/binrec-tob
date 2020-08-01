#include <llvm/IR/Module.h>
#include "SetDataLayout32.h"

using namespace llvm;

char SetDataLayout32::ID = 0;
static RegisterPass<SetDataLayout32> x("set-data-layout-32",
        "S2E Set a 32-bit data layout and target triple", false, false);

bool SetDataLayout32::runOnModule(Module &m) {
    m.setDataLayout(
        // see http://llvm.org/docs/LangRef.html#data-layout
        "e"             // little-endian
        "-p:32:32:32"   // pointer size/abi/prefalign   (changed)
        "-S128"         // stack align
        "-i1:8:8"       // |
        "-i8:8:8"       // |
        "-i16:16:16"    // |
        "-i32:32:32"    // |
        "-i64:32:64"    // | data type sizes/alignments (changed)
        "-f16:16:16"    // |
        "-f32:32:32"    // |
        "-f64:32:64"    // |                            (changed)
        "-f80:32:32"    // |                            (changed)
        "-v64:64:64"    // |
        "-v128:128:128" // |
        "-a0:0:64"      // 64-bit aggregate align
        "-n8:16:32"     // CPU integer widths           (changed)
    );
    m.setTargetTriple("i386-unknown-linux-gnu");
    return true;
}

