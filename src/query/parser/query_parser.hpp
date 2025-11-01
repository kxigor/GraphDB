#pragma once

#include <boost/spirit/home/x3.hpp>

#include "ast.hpp"
#include "config.hpp"

namespace parser {
using query_parser_type = x3::rule<class query_parser_, ast::Query>;
BOOST_SPIRIT_DECLARE(query_parser_type);

const char separator = ';';
query_parser_type parser();
}  // namespace parser
