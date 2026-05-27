#pragma once

#include "query_parser_rules.hpp"

// NOLINTBEGIN
namespace parser {
    const auto identifier_def     = x3::lexeme[+(x3::alnum | x3::char_('_'))];
    const auto quotted_string_def = x3::lexeme['"' >> *~x3::char_('"') >> '"'];

    const auto location           = x3::lit("at") >> identifier;

    const auto property_def       = identifier >> '=' >> quotted_string;
    const auto property_list_def  = '{' >> -(property % ',') >> '}';

    const auto vertex_def         = '[' >> -quotted_string >> ']' >> property_list;

    const auto edge_def           = property_list;
    const auto match_def          = vertex >> x3::lit("--") >> edge >> x3::lit("->") >> vertex;

    const auto graph_def          = identifier >> '{' >>
                                        base_type >> x3::lit("vertex") >> '{' >>
                                            -((base_type >> identifier) % ',') >>
                                        '}' >>
                                        x3::lit("edge") >> '{' >>
                                            -((base_type >> identifier) % ',') >>
                                        '}' >>
                                    '}';

    const auto exit_query_def     = x3::lit("exit") >> x3::attr(ast::ExitQuery{});

    const auto new_query_def      = x3::lit("new")    >> graph;
    const auto delete_query_def   = x3::lit("delete") >> identifier;

    const auto add_query_def      = x3::lit("add")   >> location >> vertex;
    const auto match_query_def    = x3::lit("match") >> location >> match;

    const auto remove_query_def   = x3::lit("remove")  >> location;
    const auto unmatch_query_def  = x3::lit("unmatch") >> location;

    const auto return_query_def   = x3::lit("return") >> location;
    const auto update_query_def   = x3::lit("update") >> location >> property_list; // TODO add update for edges

    const auto select_query_def   = x3::lit("select") >> location >> (match | vertex); // TODO more complex select(long mathces, bfs, etc.)

    const auto query_parser_def   = exit_query   | return_query
                                  | new_query    | delete_query
                                  | add_query    | match_query
                                  | remove_query | unmatch_query
                                  | update_query | select_query;
}  // namespace parser

namespace parser {
    BOOST_SPIRIT_DEFINE(identifier, quotted_string,
                        property, property_list,
                        vertex, edge, match, graph,
                        exit_query, return_query,
                        new_query, delete_query,
                        add_query, match_query,
                        remove_query, unmatch_query,
                        update_query, select_query,
                        query_parser);

    query_parser_type parser() {
        return query_parser;
    }
}  // namespace parser
// NOLINTEND