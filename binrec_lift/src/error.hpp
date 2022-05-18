#ifndef BINREC_LIFT_ERROR_HPP
#define BINREC_LIFT_ERROR_HPP

#include <stdexcept>
#include <string>

#define LLVM_ERROR(VARNAME)                                                                        \
    std::string VARNAME;                                                                           \
    llvm::raw_string_ostream _llvm_error_stream{VARNAME};                                          \
    _llvm_error_stream


#define LIFT_ASSERT(pass, cond)                                                                    \
    if (!(cond)) {                                                                                 \
        throw binrec::lifting_error{pass, #cond};                                                  \
    }

namespace binrec {

    class binrec_error : public std::runtime_error {
    public:
        binrec_error(const std::string &what) : std::runtime_error(what) {}

        binrec_error(const binrec_error &) = default;
        binrec_error &operator=(const binrec_error &) = default;
    };

    class lifting_error : public binrec_error {
    public:
        lifting_error(const std::string &pass, const std::string &what) :
                binrec_error(what),
                pass_(pass)
        {
        }

        lifting_error(const lifting_error &) = default;
        lifting_error &operator=(const lifting_error &) = default;

        const char *pass() const
        {
            return pass_.c_str();
        }

    private:
        std::string pass_;
    };

} // namespace binrec


#endif
