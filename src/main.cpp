#include "manager/storage_manager.hpp"
#include "utils/storage_config.hpp"

int main() {
  const storage::text_type BD_name = "AMOGUS_BD";

  const storage::text_type kVertexMangling = "v_";
  const storage::text_type kEdgeMangling = "e_";

  storage::manager::FieldsConstructor vertex_fields(kVertexMangling);
  storage::manager::FieldsConstructor edge_fields(kEdgeMangling);

  vertex_fields.add_field("name", "std::string");
  edge_fields.add_field("status", "std::string");
  /*TRIG*/
  const storage::manager::GraphInfo kGraphInfo{
      .graph_name = "BIBOBA_GRAPH",
      .key_type = "int",
      .vertex_names = vertex_fields.get_names(),
      .vertex_type = vertex_fields.get_type(),
      .edge_names = edge_fields.get_names(),
      .edge_type = edge_fields.get_type()};

  storage::manager::StorageManager manager(BD_name);
  manager.create_db();
  manager.create_graph(kGraphInfo);
}
