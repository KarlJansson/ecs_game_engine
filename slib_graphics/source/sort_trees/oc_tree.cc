#include "oc_tree.h"

namespace lib_graphics {
OcTree::OcTree() {
  root_.center = {50.f};
  root_.extent = {650.f};
  node_map_[1];
}

void OcTree::UpdateEntityPosition(lib_core::Entity entity, BoundingVolume box) {
  uint64_t loc_code = 1;
  auto ent_loc = entity_locations_.find(entity);
  if (ent_loc != entity_locations_.end()) {
    auto node_it = node_map_.find(ent_loc->second);
    assert(node_it != node_map_.end());
    loc_code = node_it->first;
    for (auto it = node_it->second.content.begin();
         it != node_it->second.content.end(); it++) {
      if (it->first == entity) {
        node_it->second.content.erase(it);
        break;
      }
    }
    entity_locations_.erase(entity);

    uint8_t child;
    auto node_box = ComputeNodeVolume(node_it->first);
    while (loc_code > 1 && !AxisAlignedBox(node_box).Contains(box)) {
      child = uint8_t(loc_code & uint64_t(7));
      if (node_it != node_map_.end() && node_it->second.content.empty() &&
          node_it->second.child_mask == 0) {
        node_map_.erase(node_it);
        node_it = node_map_.find(loc_code >> 3);
        if (node_it->second.child_mask & (uint64_t(1) << child))
          node_it->second.child_mask ^= uint64_t(1) << child;
      } else {
        node_it = node_map_.end();
      }
      node_box = ComputeParentVolume(node_box, child);
      loc_code >>= 3;
    }
    AddEntity(entity, box, loc_code > 0 ? loc_code : 1);
  } else
    AddEntity(entity, box, 1);
}

void OcTree::AddEntity(lib_core::Entity entity, BoundingVolume box,
                       uint64_t loc_code) {
  assert(loc_code);
  auto ent_loc = entity_locations_.find(entity);
  if (ent_loc != entity_locations_.end()) {
    UpdateEntityPosition(entity, box);
  } else {
    auto node_depth = ComputeNodeDepth(loc_code);
    auto node_box = ComputeNodeVolume(loc_code);
    if (!AxisAlignedBox(node_box).Overlap(root_)) {
      node_map_[1].content.emplace_back(entity, box);
      entity_locations_[entity] = 1;
    } else {
      bool perc_down = false;
      auto curr_node = node_map_.find(loc_code);
      do {
        perc_down = false;
        if (node_depth < 20) {
          for (auto i = 0; i < 8; ++i) {
            auto child_vol = ComputeChildVolume(node_box, i);
            if (AxisAlignedBox(child_vol).Contains(box)) {
              perc_down = true;
              curr_node->second.child_mask |= uint64_t(1) << i;
              loc_code = (loc_code << 3) | uint64_t(i);
              node_map_[loc_code];
              curr_node = node_map_.find(loc_code);
              node_box = child_vol;
              break;
            }
          }
          ++node_depth;
        }
      } while (perc_down);

      curr_node->second.content.emplace_back(entity, box);
      entity_locations_[entity] = curr_node->first;
    }
  }
}

void OcTree::RemoveEntity(lib_core::Entity entity) {
  auto it = entity_locations_.find(entity);
  if (it != entity_locations_.end()) {
    auto node_it = node_map_.find(it->second);
    assert(node_it != node_map_.end());
    for (auto it = node_it->second.content.begin();
         it != node_it->second.content.end(); it++) {
      if (it->first == entity) {
        node_it->second.content.erase(it);
        break;
      }
    }

    uint8_t child;
    uint64_t parent = node_it->first;
    while (parent > 1 && node_it->second.content.empty() &&
           node_it->second.child_mask == 0) {
      parent = node_it->first >> 3;
      child = uint8_t(node_it->first & uint64_t(7));
      node_map_.erase(node_it);
      node_it = node_map_.find(parent);
      assert(node_it != node_map_.end());
      if (node_it->second.child_mask & (uint64_t(1) << child))
        node_it->second.child_mask ^= uint64_t(1) << child;
    }
    entity_locations_.erase(it);
  }
}

void OcTree::SearchFrustum(const BoundingFrustum frustum,
                           ct::dyn_array<lib_core::Entity>& out) {
  Lookup(frustum, AxisAlignedBox(root_), 1, out);
}

void OcTree::SearchBox(const AxisAlignedBox box,
                       ct::dyn_array<lib_core::Entity>& out) {
  Lookup(box, AxisAlignedBox(root_), 1, out);
}

void OcTree::SearchSphere(const BoundingSphere sphere,
                          ct::dyn_array<lib_core::Entity>& out) {
  Lookup(sphere, AxisAlignedBox(root_), 1, out);
}

void OcTree::SearchFrustum(const BoundingFrustum frustum,
                           ct::tree_set<lib_core::Entity>& out) {
  Lookup(frustum, AxisAlignedBox(root_), 1, out);
}

void OcTree::SearchBox(const AxisAlignedBox box,
                       ct::tree_set<lib_core::Entity>& out) {
  Lookup(box, AxisAlignedBox(root_), 1, out);
}

void OcTree::SearchSphere(const BoundingSphere sphere,
                          ct::tree_set<lib_core::Entity>& out) {
  Lookup(sphere, AxisAlignedBox(root_), 1, out);
}

size_t OcTree::GetNrNodes() { return node_map_.size(); }

template <typename V, typename T, typename C>
void OcTree::Lookup(const V search_vol, T vol, uint64_t loc_code, C& out) {
  if (search_vol.Contains(vol)) {
    Collect(loc_code, out);
  } else if (search_vol.Overlap(vol)) {
    auto node_it = node_map_.find(loc_code);
    assert(node_it != node_map_.end());
    for (auto& e : node_it->second.content) {
      if (search_vol.Overlap(e.second)) out.insert(out.end(), e.first);
    }

    for (auto i = 0; i < 8; ++i) {
      if (node_it->second.child_mask & (uint64_t(1) << i)) {
        auto child_vol = ComputeChildVolume(vol, i);
        Lookup(search_vol, child_vol, (loc_code << 3) | i, out);
      }
    }
  }
}

template <typename T>
void OcTree::Collect(uint64_t loc_code, T& out) {
  auto node_it = node_map_.find(loc_code);
  for (auto& e : node_it->second.content) out.insert(out.end(), e.first);

  for (auto i = 0; i < 8; ++i)
    if (node_it->second.child_mask & (uint64_t(1) << i))
      Collect((loc_code << 3) | i, out);
}

inline BoundingVolume OcTree::ComputeParentVolume(BoundingVolume vol,
                                                  uint8_t child) {
  assert(child < 8);
  vol.center -= vol.extent * octants_[child];
  vol.extent *= 2.f;
  return vol;
}

inline BoundingVolume OcTree::ComputeChildVolume(BoundingVolume vol,
                                                 uint8_t child) {
  assert(child < 8);
  vol.extent *= .5f;
  vol.center += vol.extent * octants_[child];
  return vol;
}

inline AxisAlignedBox OcTree::ComputeChildVolume(AxisAlignedBox vol,
                                                 uint8_t child) {
  return AxisAlignedBox(ComputeChildVolume(vol.data, child));
}

inline BoundingSphere OcTree::ComputeChildVolume(BoundingSphere vol,
                                                 uint8_t child) {
  return BoundingSphere(ComputeChildVolume(vol.data, child));
}

inline BoundingVolume OcTree::ComputeNodeVolume(uint64_t loc_code) {
  BoundingVolume b = root_;
  auto depth = ComputeNodeDepth(loc_code);
  for (auto i = 0; i < depth; ++i) {
    auto shift = (depth - 1 - i) * 3;
    b = ComputeChildVolume(
        b, uint8_t((loc_code & (uint64_t(7) << shift))) >> shift);
  }
  return b;
}

inline size_t OcTree::ComputeNodeDepth(uint64_t loc_code) {
  assert(loc_code);

#ifdef UnixBuild
  size_t count = 0;
  for (auto i = 63; i > 0; --i) {
    if (loc_code & (uint64_t(1) << i))
      break;
    else
      ++count;
  }

  return (64 - count) / 3;
#elif WindowsBuild
  unsigned long msb;
  _BitScanReverse64(&msb, loc_code);
  return msb / 3;
#endif  // WindowsBuild
}
}  // namespace lib_graphics
