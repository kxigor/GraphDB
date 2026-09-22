#pragma once

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/vector.hpp>

namespace ba = boost::archive;

namespace ast {

using cpp_type = std::string;

struct Property {
 public:
  std::string identifier;
  std::string property;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & identifier;
    ar & property;
  }
};

using PropertyList = std::vector<Property>;

struct Vertex {
 public:
  boost::optional<std::string> key;
  PropertyList properties;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & key;
    ar & properties;
  }
};

using Edge = PropertyList;

struct Match {
 public:
  Vertex from;
  Edge edge;
  Vertex to;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & from;
    ar & to;
    ar & edge;
  }
};

struct Graph {
 public:
  std::string name;
  cpp_type key_type;

  std::vector<std::pair<cpp_type, std::string>> vertex_fields;
  std::vector<std::pair<cpp_type, std::string>> edge_fields;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & name;
    ar & key_type;
    ar & vertex_fields;
    ar & edge_fields;
  }
};

struct ExitQuery {
 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &, const unsigned) {}
};

struct NewQuery {
 public:
  Graph graph;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & graph;
  }
};

struct DeleteQuery {
 public:
  std::string graph;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & graph;
  }
};

struct AddQuery {
 public:
  std::string graph;
  Vertex vertex;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & graph;
    ar & vertex;
  }
};

struct MatchQuery {
 public:
  std::string graph;
  Match match;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & graph;
    ar & match;
  }
};

struct RemoveQuery {
 public:
  std::string graph;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & graph;
  }
};

struct UnmatchQuery {
 public:
  std::string graph;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & graph;
  }
};

struct UpdateQuery {
 public:
  std::string graph;
  PropertyList properties;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & graph;
    ar & properties;
  }
};

struct SelectQuery {
 public:
  std::string graph;
  boost::variant<Match, Vertex> target;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & graph;
    ar & target;
  }
};

struct ReturnQuery {
 public:
  std::string graph;

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned) {
    ar & graph;
  }
};

using Query = boost::variant<ExitQuery, ReturnQuery, NewQuery, DeleteQuery,
                             AddQuery, MatchQuery, RemoveQuery, UnmatchQuery,
                             UpdateQuery, SelectQuery>;
}  // namespace ast