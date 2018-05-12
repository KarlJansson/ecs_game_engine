#pragma once
#include "core_utilities.h"

struct aiScene;
struct aiNode;
struct aiMesh;

namespace cmd_fract_cooking {
class ModelCooker {
 public:
  ModelCooker() = default;
  ~ModelCooker() = default;

  void LoadModel(ct::string path);
  void SerializeAndSave(ct::string save_path);

 protected:
 private:
  struct Vertex {
    float position[3];
    float normal[3];
    float tangent[3];
    float texcoord[2];
  };

  struct Mesh {
    ct::dyn_array<Vertex> vertices;
    ct::dyn_array<uint32_t> indices;
    float center[3], extent[3];
  };

  struct Model {
    ct::dyn_array<Mesh> meshes;
  };

  struct CompressedModel {
    ct::string name;
    size_t mesh_count;
    ct::dyn_array<uint8_t> data;
  };

  void ProcessNode(aiNode* node, const aiScene* scene, Model& model);
  Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

  tbb::concurrent_vector<CompressedModel> models_;
};
}  // namespace cmd_fract_cooking
