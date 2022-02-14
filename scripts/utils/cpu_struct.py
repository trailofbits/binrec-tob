#!/usr/bin/env python

# This script is meant to work with S2E. You need to compile the LLVM
# helpers file (op_helpers.bc) with debug support (-g) to make clang
# include the neccessary annotations in the file. Then you can extract
# information about the QEMU CPUState structure with this script.

def get_type_information(type_description):
    return {
        "name": type_description.operands[2].string,
        "size": type_description.operands[5].z_ext_value, #in bits
        "offset": type_description.operands[7].z_ext_value, #in bits
        "line": type_description.operands[4].z_ext_value}

def get_cpu_struct_information(module):
    cu = module.get_named_metadata("llvm.dbg.cu")
    assert(cu and cu.operand_count == 1)

    global_variable_descriptions = cu.operands[0].operands[13].operands[0]
    for global_variable_description in global_variable_descriptions.operands:
        if global_variable_description.operand_count < 4 or global_variable_description.operands[3].string != "env":
            continue

        #This is the pointer descriptor
        env_description = global_variable_description


        cpustate_description = env_description.operands[8].operands[9]
        cpustate_typeinfo = get_type_information(cpustate_description)
        print("CPU name = %s, size = %d" %(cpustate_typeinfo["name"], cpustate_typeinfo["size"] / 8))
        for member_description in cpustate_description.operands[10].operands:
            member_typeinfo = get_type_information(member_description)
            print("Member offset = %d, name = %s, size = %d" %(member_typeinfo["offset"] / 8, member_typeinfo["name"], member_typeinfo["size"] / 8))


if __name__ == "__main__":
    import sys
    import llvm
    import llvm.core
    with open(sys.argv[1], 'rb') as file:
        data = file.read()

    mod = llvm.core.Module.from_bitcode(data)
    get_cpu_struct_information(mod)
