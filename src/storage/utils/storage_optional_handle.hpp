#include <utility>

namespace storage::details {
template <typename Exception, typename OptionalType, typename... Args>
auto unwrap_optional_or_throw(OptionalType&& opt, Args&&... args) {
  if (!opt.has_value()) {
    throw Exception(std::forward<Args>(args)...);
  }
  return std::forward<OptionalType>(opt).value();
}
};  // namespace storage::details