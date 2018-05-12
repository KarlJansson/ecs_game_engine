#pragma once
#include "core_utilities.h"
#include "vector_def.h"

namespace lib_graphics {
class Mesh {
 public:
  Mesh() = default;
  explicit Mesh(size_t mesh, size_t material)
      : mesh(mesh), material(material) {}
  explicit Mesh(size_t mesh, size_t material, lib_core::Vector3 alb,
                lib_core::Vector3 rme)
      : mesh(mesh), material(material), albedo(alb), rme(rme) {}
  ~Mesh() = default;

  size_t mesh = 0;
  size_t material = 0;

  lib_core::Vector3 albedo = {1.f};
  lib_core::Vector3 rme = {.0f};

  lib_core::Vector2 texture_scale = {1.f};
  lib_core::Vector2 texture_offset = {0.f};

  float translucency = 1.f;
  float fade_in = 0.f;

  static Mesh Parse(ct::dyn_array<char> &buffer, size_t &cursor,
                    ct::hash_map<ct::string, ct::string> &val_map) {
    Mesh m;
    if (cu::ScrollCursor(buffer, cursor, '{')) {
      auto type = cu::ParseType(buffer, cursor);
      while (!type.empty()) {
        if (type.compare("Albedo") == 0) {
          m.albedo = cu::ParseVector<lib_core::Vector3, 3>(
              cu::ParseValue(buffer, cursor, val_map));
        } else if (type.compare("RME") == 0) {
          m.rme = cu::ParseVector<lib_core::Vector3, 3>(
              cu::ParseValue(buffer, cursor, val_map));
        } else if (type.compare("TextureScale") == 0) {
          m.texture_scale = cu::ParseVector<lib_core::Vector2, 2>(
              cu::ParseValue(buffer, cursor, val_map));
        } else if (type.compare("TextureOffset") == 0) {
          m.texture_offset = cu::ParseVector<lib_core::Vector2, 2>(
              cu::ParseValue(buffer, cursor, val_map));
        } else if (type.compare("Translucency") == 0) {
          m.translucency =
              cu::Parse<float>(cu::ParseValue(buffer, cursor, val_map));
        }

        type = cu::ParseType(buffer, cursor);
      }
    }
    return m;
  }
};
}  // namespace lib_graphics
