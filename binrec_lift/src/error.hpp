#ifndef BINREC_LIFT_ERROR_HPP
#define BINREC_LIFT_ERROR_HPP

#define LLVM_ERROR(VARNAME)                                                                        \
    std::string VARNAME;                                                                           \
    llvm::raw_string_ostream _llvm_error_stream{VARNAME};                                          \
    _llvm_error_stream

#endif
