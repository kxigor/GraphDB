#pragma once

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include "ast.hpp"

BOOST_FUSION_ADAPT_STRUCT(ast::Property, identifier, property);

BOOST_FUSION_ADAPT_STRUCT(ast::Vertex, key, properties);

BOOST_FUSION_ADAPT_STRUCT(ast::Match, from, edge, to);

BOOST_FUSION_ADAPT_STRUCT(ast::Graph, name, key_type, vertex_fields, edge_fields);

BOOST_FUSION_ADAPT_STRUCT(ast::ExitQuery);

BOOST_FUSION_ADAPT_STRUCT(ast::NewQuery, graph);
BOOST_FUSION_ADAPT_STRUCT(ast::DeleteQuery, graph);

BOOST_FUSION_ADAPT_STRUCT(ast::AddQuery, graph, vertex);
BOOST_FUSION_ADAPT_STRUCT(ast::MatchQuery, graph, match);

BOOST_FUSION_ADAPT_STRUCT(ast::RemoveQuery, graph);
BOOST_FUSION_ADAPT_STRUCT(ast::UnmatchQuery, graph);

BOOST_FUSION_ADAPT_STRUCT(ast::UpdateQuery, graph, properties);
BOOST_FUSION_ADAPT_STRUCT(ast::SelectQuery, graph, target);

BOOST_FUSION_ADAPT_STRUCT(ast::ReturnQuery, graph);