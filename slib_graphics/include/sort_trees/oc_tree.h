#pragma once
#include "component/axis_aligned_box.h"
#include "component/camera.h"
#include "entity.h"
#include "entity_manager.h"

namespace lib_graphics {
class OcTree {
 public:
  OcTree();
  ~OcTree() = default;

  void UpdateEntityPosition(lib_core::Entity entity, BoundingVolume box);
  void AddEntity(lib_core::Entity entity, BoundingVolume box,
                 uint64_t loc_code = 1);
  void RemoveEntity(lib_core::Entity entity);

  void SearchFrustum(const BoundingFrustum frustum,
                     ct::dyn_array<lib_core::Entity>& out);
  void SearchBox(const AxisAlignedBox box,
                 ct::dyn_array<lib_core::Entity>& out);
  void SearchSphere(const BoundingSphere sphere,
                    ct::dyn_array<lib_core::Entity>& out);
  void SearchFrustum(const BoundingFrustum frustum,
                     ct::tree_set<lib_core::Entity>& out);
  void SearchBox(const AxisAlignedBox box, ct::tree_set<lib_core::Entity>& out);
  void SearchSphere(const BoundingSphere sphere,
                    ct::tree_set<lib_core::Entity>& out);

  size_t GetNrNodes();

 private:
  struct OccNode {
    uint8_t child_mask = 0;
    ct::dyn_array<std::pair<lib_core::Entity, BoundingVolume>> content;
  };

  template <typename V, typename T, typename C>
  void Lookup(const V search_vol, T vol, uint64_t loc_code, C& out);
  template <typename T>
  void Collect(uint64_t loc_code, T& out);

  inline BoundingVolume ComputeChildVolume(BoundingVolume vol, uint8_t child);
  inline AxisAlignedBox ComputeChildVolume(AxisAlignedBox vol, uint8_t child);
  inline BoundingSphere ComputeChildVolume(BoundingSphere vol, uint8_t child);

  inline BoundingVolume ComputeParentVolume(BoundingVolume vol, uint8_t child);
  inline BoundingVolume ComputeNodeVolume(uint64_t loc_code);
  inline size_t ComputeNodeDepth(uint64_t loc_code);

  const std::array<lib_core::Vector3, 8> octants_ = {
      lib_core::Vector3(1.f, 1.f, 1.f),   lib_core::Vector3(-1.f, 1.f, 1.f),
      lib_core::Vector3(1.f, 1.f, -1.f),  lib_core::Vector3(-1.f, 1.f, -1.f),
      lib_core::Vector3(1.f, -1.f, 1.f),  lib_core::Vector3(-1.f, -1.f, 1.f),
      lib_core::Vector3(1.f, -1.f, -1.f), lib_core::Vector3(-1.f, -1.f, -1.f),
  };
  BoundingVolume root_;
  ct::hash_map<uint64_t, OccNode> node_map_;
  ct::hash_map<lib_core::Entity, uint64_t> entity_locations_;
};
}  // namespace lib_graphics
