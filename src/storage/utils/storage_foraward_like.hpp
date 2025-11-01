#include <utility>

namespace storage::details {
// NOLINTBEGIN
template <class T, class U>
constexpr auto&& forward_like(U&& x) noexcept {
  constexpr bool kIsAddingConst = std::is_const_v<std::remove_reference_t<T>>;
  if constexpr (std::is_lvalue_reference_v<T&&>) {
    if constexpr (kIsAddingConst) {
      return std::as_const(x);
    } else {
      return static_cast<U&>(x);
    }
  } else {
    if constexpr (kIsAddingConst) {
      return std::move(std::as_const(x));
    } else {
      return std::move(x);
    }
  }
}
// NOLINTEND
}  // namespace storage::details