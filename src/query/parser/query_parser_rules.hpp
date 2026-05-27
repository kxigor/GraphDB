#pragma once

#include <boost/spirit/home/x3.hpp>

#include "ast_adapted.hpp"
#include "query_parser.hpp"

// NOLINTBEGIN
namespace parser {
    struct BaseType: x3::symbols<ast::cpp_type> {
        BaseType() {
            add
            ("int"   , "int")
            ("bool"  , "bool")
            ("string", "std::string");
        }
    } base_type;

    x3::rule<class identifier, std::string> identifier;
    x3::rule<class quotted_string, std::string> quotted_string;

    x3::rule<class property, ast::Property> property;
    x3::rule<class property_list, ast::PropertyList> property_list;

    x3::rule<class vertex, ast::Vertex> vertex;
    x3::rule<class edge, ast::Edge> edge;
    x3::rule<class match, ast::Match> match;

    x3::rule<class graph, ast::Graph> graph;

    x3::rule<class exit_query, ast::ExitQuery> exit_query;

    x3::rule<class new_query, ast::NewQuery> new_query;
    x3::rule<class delete_query, ast::DeleteQuery> delete_query;

    x3::rule<class add_query, ast::AddQuery> add_query;
    x3::rule<class match_query, ast::MatchQuery> match_query;

    x3::rule<class remove_query, ast::RemoveQuery> remove_query;
    x3::rule<class unmatch_query, ast::UnmatchQuery> unmatch_query;

    x3::rule<class return_query, ast::ReturnQuery> return_query;
    x3::rule<class update_query, ast::UpdateQuery> update_query;

    x3::rule<class select_query, ast::SelectQuery> select_query;

    query_parser_type query_parser;
} // namespace parser
// NOLINTEND