#pragma once

#include <fmt/core.h>

#include "utils/storage_config.hpp"

namespace storage::details {
text_type to_string(const auto& text) { return fmt::format("{}", text); }
}  // namespace storage::details