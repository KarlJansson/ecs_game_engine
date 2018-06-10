#pragma once
#include <limits>
#include "axis_aligned_box.h"
#include "camera.h"
#include "entity.h"
#include "light.h"
#include "sort_trees/oc_tree.h"
#include "system.h"
#include "system_manager.h"

namespace lib_core {
class EngineCore;
}

namespace lib_graphics {
class CullingSystem : public lib_core::System {
 public:
  CullingSystem(lib_core::EngineCore *engine);
  ~CullingSystem() override;

  void LogicUpdate(float dt) override;
  void DrawUpdate(lib_graphics::Renderer *renderer,
                  lib_gui::TextSystem *text_renderer) override;

  class MeshOctreeFlag {
   public:
  };

  class LightOctreeFlag {
   public:
  };

  struct MeshPack {
    size_t mesh_id;
    size_t material_id;
    size_t mesh_count;
    size_t start_ind;
  };

  const ct::dyn_array<MeshPack> *GetMeshPacks(lib_core::Entity entity,
                                              bool opeque = true);
  const ct::dyn_array<lib_core::Entity> *GetLightPacks(lib_core::Entity entity);

  ct::dyn_array<lib_core::Vector3> &GetAlbedoVecs(bool opeque = true);
  ct::dyn_array<lib_core::Vector3> &GetRmeVecs(bool opeque = true);
  ct::dyn_array<lib_core::Vector2> &GetTexScaleVecs(bool opeque = true);
  ct::dyn_array<lib_core::Vector2> &GetTexOffsetVecs(bool opeque = true);
  ct::dyn_array<lib_core::Matrix4x4> &GetWorldMatrices(bool opeque = true);
  ct::dyn_array<lib_core::Matrix4x4> &GetWorldInvTransMatrices(
      bool opeque = true);

  ct::dyn_array<float> &GetTransparencyVecs();
  ct::dyn_array<lib_core::Matrix4x4> &GetLightMatrices(lib_core::Entity ent);

  class AddMeshAabbCommand : public lib_core::Command {
   public:
    AddMeshAabbCommand() = default;
    AddMeshAabbCommand(size_t mesh_id, BoundingVolume aabb)
        : mesh_id(mesh_id), aabb(aabb) {}

    size_t mesh_id;
    BoundingVolume aabb;
  };

  class RemoveMeshAabbCommand : public lib_core::Command {
   public:
    RemoveMeshAabbCommand() = default;
    RemoveMeshAabbCommand(size_t mesh_id) : mesh_id(mesh_id) {}

    size_t mesh_id;
  };

 private:
  size_t AabbMeshCheck(const Camera::FrustumPlanes planes,
                       lib_core::Entity target, bool clear_ents = true);
  size_t AabbLightCheck(const Camera::FrustumPlanes planes,
                        lib_core::Entity target, bool clear_ents = true);
  size_t AabbLightCheckTiled(Camera &camera, lib_core::Entity target,
                             bool clear_ents = true);
  void UpdateSearchTrees();

  struct MeshPackData {
    void clear() {
      rme_vec.clear(), albedo_vec.clear();
      tex_scale.clear(), tex_offset.clear();
      world_vec.clear(), world_inv_trans_vec.clear();
      transp_vec.clear();
      closest_dist = std::numeric_limits<float>::infinity();
    }

    float closest_dist;
    ct::dyn_array<float> transp_vec;
    ct::dyn_array<lib_core::Vector3> rme_vec;
    ct::dyn_array<lib_core::Vector3> albedo_vec;
    ct::dyn_array<lib_core::Vector2> tex_scale;
    ct::dyn_array<lib_core::Vector2> tex_offset;
    ct::dyn_array<lib_core::Matrix4x4> world_vec;
    ct::dyn_array<lib_core::Matrix4x4> world_inv_trans_vec;
  };

  MeshPackData opeque_meshes_, translucent_meshes_;

  lib_core::EngineCore *engine_;

  ct::tree_map<std::pair<size_t, size_t>, MeshPackData> opeque_mesh_packs_;
  ct::tree_map<float, ct::tree_map<std::pair<size_t, size_t>, MeshPackData>,
               std::greater<float>>
      translucent_mesh_packs_;

  ct::tree_map<float, ct::dyn_array<decltype(opeque_mesh_packs_)::iterator>>
      sorted_opeque_packs_;

  std::unique_ptr<OcTree> mesh_octree_;
  std::unique_ptr<OcTree> light_octree_;

  ct::hash_map<lib_core::Entity, ct::dyn_array<lib_core::Matrix4x4>>
      light_matrices_;
  ct::hash_map<lib_core::Entity, ct::dyn_array<MeshPack>>
      opeque_mesh_packs_out_, translucent_mesh_packs_out_;
  ct::hash_map<lib_core::Entity, ct::dyn_array<lib_core::Entity>> light_packs_;
  ct::hash_map<lib_core::Entity, ct::dyn_array<lib_core::Entity>>
      draw_entities_;

  ct::hash_map<size_t, BoundingVolume> mesh_aabb_;

  ct::dyn_array<lib_core::Entity> add_mesh_vec_, add_light_vec_;

  size_t rem_mesh_callback_id, rem_light_callback_id_;
  size_t add_mesh_callback_id, add_light_callback_id_;
  size_t shadow_meshes_, mesh_count_, light_count_;
};
}  // namespace lib_graphics
