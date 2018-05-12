#include "model_cooker.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include "../../source_shared/include/serialization_utilities.hpp"
#include "core_utilities.h"

namespace cmd_fract_cooking {
void ModelCooker::LoadModel(ct::string path) {
  if (!std::experimental::filesystem::exists(path)) return;

  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(
      path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals |
                aiProcess_OptimizeMeshes | aiProcess_CalcTangentSpace);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    return;
  }

  Model model;
  ProcessNode(scene->mRootNode, scene, model);

  size_t size = 0;
  for (auto& mesh : model.meshes) {
    for (int ii = 0; ii < 3; ++ii)
      SerializationUtilities::CountSize(mesh.center[ii], size);
    for (int ii = 0; ii < 3; ++ii)
      SerializationUtilities::CountSize(mesh.extent[ii], size);
    SerializationUtilities::CountSize(mesh.vertices, size);
    SerializationUtilities::CountSize(mesh.indices, size);
  }

  ct::dyn_array<uint8_t> buffer(size, 0);
  auto it = buffer.begin();
  for (auto& mesh : model.meshes) {
    for (int ii = 0; ii < 3; ++ii)
      SerializationUtilities::CopyToBuffer(mesh.center[ii], it);
    for (int ii = 0; ii < 3; ++ii)
      SerializationUtilities::CopyToBuffer(mesh.extent[ii], it);
    SerializationUtilities::CopyToBuffer(mesh.vertices, it);
    SerializationUtilities::CopyToBuffer(mesh.indices, it);
  }

  CompressedModel comp_model;
  comp_model.mesh_count = model.meshes.size();
  comp_model.name = path;

  cu::CompressMemory(buffer, comp_model.data);
  models_.push_back(std::move(comp_model));
}

void ModelCooker::SerializeAndSave(ct::string save_path) {
  std::ofstream list_output(save_path + "names.txt");

  size_t size = 0;
  ct::dyn_array<size_t> starting_points;
  SerializationUtilities::CountSize(models_.size(), size);
  for (auto& model : models_) {
    SerializationUtilities::CountSize(model.name, size);
    SerializationUtilities::CountSize(size, size);
    SerializationUtilities::CountSize(size, size);

    list_output << model.name.substr(model.name.find_last_of('\\') + 1,
                                     model.name.size())
                << " " << model.mesh_count << "\n";
  }
  list_output.close();

  for (auto& model : models_) {
    starting_points.push_back(size);
    SerializationUtilities::CountSize(model.data, size);
  }

  int i = 0;
  ct::dyn_array<uint8_t> buffer(size);
  auto it = buffer.begin();
  SerializationUtilities::CopyToBuffer(models_.size(), it);
  for (auto& model : models_) {
    SerializationUtilities::CopyToBuffer(model.name, it);
    SerializationUtilities::CopyToBuffer(model.mesh_count, it);
    SerializationUtilities::CopyToBuffer(starting_points[i++], it);
  }
  for (auto& model : models_)
    SerializationUtilities::CopyToBuffer(model.data, it);

  cu::Save(save_path, buffer);
}

void ModelCooker::ProcessNode(aiNode* node, const aiScene* scene,
                              Model& model) {
  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    model.meshes.push_back(ProcessMesh(mesh, scene));
  }

  for (unsigned int i = 0; i < node->mNumChildren; i++)
    ProcessNode(node->mChildren[i], scene, model);
}

ModelCooker::Mesh ModelCooker::ProcessMesh(aiMesh* mesh, const aiScene* scene) {
  Mesh result;
  result.vertices.reserve(mesh->mNumVertices);
  float max[3], min[3];
  min[0] = min[1] = min[2] = std::numeric_limits<float>::lowest();
  max[0] = max[1] = max[2] = std::numeric_limits<float>::infinity();

  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    Vertex vertex;
    vertex.position[0] = mesh->mVertices[i].x;
    vertex.position[1] = mesh->mVertices[i].y;
    vertex.position[2] = mesh->mVertices[i].z;

    if (i == 0) {
      for (int ii = 0; ii < 3; ++ii) max[ii] = min[ii] = vertex.position[ii];
    } else {
      for (int ii = 0; ii < 3; ++ii) {
        if (max[ii] < vertex.position[ii])
          max[ii] = vertex.position[ii];
        else if (min[ii] > vertex.position[ii])
          min[ii] = vertex.position[ii];
      }
    }

    if (mesh->mNormals) {
      vertex.normal[0] = mesh->mNormals[i].x;
      vertex.normal[1] = mesh->mNormals[i].y;
      vertex.normal[2] = mesh->mNormals[i].z;
    } else {
      vertex.normal[0] = 0;
      vertex.normal[1] = 0;
      vertex.normal[2] = 0;
    }

    if (mesh->mTangents) {
      vertex.tangent[0] = mesh->mTangents[i].x;
      vertex.tangent[1] = mesh->mTangents[i].y;
      vertex.tangent[2] = mesh->mTangents[i].z;
    } else {
      vertex.tangent[0] = 0;
      vertex.tangent[1] = 0;
      vertex.tangent[2] = 0;
    }

    if (mesh->mTextureCoords[0]) {
      vertex.texcoord[0] = mesh->mTextureCoords[0][i].x;
      vertex.texcoord[1] = mesh->mTextureCoords[0][i].y;
    } else {
      vertex.texcoord[0] = 0;
      vertex.texcoord[1] = 0;
    }

    result.vertices.push_back(vertex);
  }

  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++)
      result.indices.push_back(face.mIndices[j]);
  }

  for (int i = 0; i < 3; ++i) result.center[i] = (min[i] + max[i]) * 0.5f;
  for (int i = 0; i < 3; ++i) result.extent[i] = (max[i] - min[i]) * 0.5f;

  return result;
}
}  // namespace cmd_fract_cooking
