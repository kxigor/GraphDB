#include "query_parser_def.hpp"

namespace parser {
BOOST_SPIRIT_INSTANTIATE(query_parser_type, iterator_type, context_type);
}  // namespace parser