#ifndef BINREC_SHAREDPTRCASTING_H
#define BINREC_SHAREDPTRCASTING_H

#include <llvm/Support/Casting.h>
#include <memory>

namespace llvm {
template <typename To, typename From> struct isa_impl_cl<To, const std::shared_ptr<From>> {
    static inline auto doit(const std::shared_ptr<From> &val) -> bool {
        assert(val && "isa<> used on a null pointer");
        return isa_impl_cl<To, From>::doit(*val);
    }
};

template <class To, class From> struct cast_retty_impl<To, std::shared_ptr<From>> {
private:
    using PointerType = typename cast_retty_impl<To, From *>::ret_type;
    using ResultType = std::remove_pointer_t<PointerType>;

public:
    using ret_type = std::shared_ptr<ResultType>;
};

template <class X, class Y>
inline auto cast(std::shared_ptr<Y> val) -> typename cast_retty<X, std::shared_ptr<Y>>::ret_type {
    assert(isa<X>(val) && "cast<Ty>() argument of incompatible type!");
    using ret_type = typename cast_retty<X, std::shared_ptr<Y>>::ret_type;
    return ret_type{move(val), cast_convert_val<X, Y *, typename simplify_type<Y *>::SimpleType>::doit(val.get())};
}

template <class X, class Y>
[[nodiscard]] inline auto cast_or_null(std::shared_ptr<Y> val) -> // NOLINT
    typename cast_retty<X, std::shared_ptr<Y>>::ret_type {
    if (!val)
        return nullptr;
    return cast<X>(move(val));
}

template <class X, class Y>
[[nodiscard]] inline auto dyn_cast(std::shared_ptr<Y> val) -> decltype(cast<X>(val)) { // NOLINT
    if (!isa<X>(val))
        return nullptr;
    return cast<X>(move(val));
}

template <class X, class Y>
[[nodiscard]] inline auto dyn_cast_or_null(std::shared_ptr<Y> val) -> decltype(cast<X>(val)) { // NOLINT
    if (!val)
        return nullptr;
    return dyn_cast<X>(move(val));
}
} // namespace llvm

#endif // BINREC_SHAREDPTRCASTING_H
