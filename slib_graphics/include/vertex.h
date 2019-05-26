#pragma once
#include "vector_def.h"

namespace lib_graphics {
struct Vertex {
  lib_core::Vector3 position;
  lib_core::Vector3 normal;
  lib_core::Vector3 tangent;
  std::array<float, 2> texcoord;
};

struct PosVertex {
  lib_core::Vector3 position;
};
}  // namespace lib_graphics
