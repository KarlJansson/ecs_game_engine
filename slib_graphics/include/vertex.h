#pragma once
#include "vector_def.h"

namespace lib_graphics {
struct Vertex {
  lib_core::Vector3 position;
  lib_core::Vector3 normal;
  lib_core::Vector3 tangent;
  float texcoord[2];
};

struct PosVertex {
  lib_core::Vector3 position;
};
}  // namespace lib_graphics
