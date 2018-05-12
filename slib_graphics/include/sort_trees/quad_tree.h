#pragma once
#include "core_utilities.h"
#include "entity.h"

namespace lib_graphics {
class QuadTree {
 public:
  QuadTree();
  ~QuadTree() = default;

  struct QuadNode {
    float center[2];
    float extent[2];
    ct::dyn_array<QuadNode> child = ct::dyn_array<QuadNode>(4);
    ct::tree_set<lib_core::Entity> content;
  };

 private:
};
}  // namespace lib_graphics
