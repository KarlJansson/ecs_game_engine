#pragma once
#include "engine_settings.h"
#include "vector_def.h"

namespace lib_graphics {
class Light {
 public:
  enum LightType { kDir, kPoint, kSpot, kEnd };

  Light() {
    auto max_tex_size = g_settings.MaxShadowTexture();
    shadow_resolutions[0] = max_tex_size;
    shadow_resolutions[1] = max_tex_size >> 1;
    shadow_resolutions[2] = max_tex_size >> 2;
    cast_shadows = false;
    update_cast_shadow = false;
  }

  Light(lib_core::Vector3 pos, lib_core::Vector3 col, float radius,
        bool shadow = false) {
    data_pos = pos;
    color = col;
    delta_pos.ZeroMem();

    max_radius = radius;
    cast_shadows = shadow;
    update_cast_shadow = false;
    type = kPoint;

    auto max_tex_size = g_settings.MaxShadowTexture();
    shadow_resolutions[0] = max_tex_size;
  }

  Light(lib_core::Vector3 pos, lib_core::Vector3 dir, lib_core::Vector3 col,
        bool shadow = false) {
    data_pos = pos;
    data_dir = dir;
    color = col;
    delta_pos.ZeroMem();

    cast_shadows = shadow;
    update_cast_shadow = false;
    type = kDir;

    auto max_tex_size = g_settings.MaxShadowTexture();
    shadow_resolutions[0] = max_tex_size;
    shadow_resolutions[1] = max_tex_size >> 1;
    shadow_resolutions[2] = max_tex_size >> 2;
  }

  Light(lib_core::Vector3 pos, lib_core::Vector3 dir, lib_core::Vector3 col,
        lib_core::Vector2 cutoffs, bool shadow = false) {
    data_pos = pos;
    data_dir = dir;
    data_cutoffs = cutoffs;
    color = col;
    delta_pos.ZeroMem();

    cast_shadows = shadow;
    update_cast_shadow = false;
    type = kSpot;

    auto max_tex_size = g_settings.MaxShadowTexture();
    shadow_resolutions[0] = max_tex_size;
    shadow_resolutions[1] = max_tex_size >> 1;
    shadow_resolutions[2] = max_tex_size >> 2;
  }

  ~Light() = default;

  void SetCastShadows(bool cast_shadows) {
    new_cast_shadows = cast_shadows;
    update_cast_shadow = true;
  }

  LightType type = kPoint;
  bool cast_shadows = false;
  bool update_cast_shadow;
  bool new_cast_shadows;

  lib_core::Vector3 data_pos = {0.f, -1000.f, 0.f};
  lib_core::Vector3 data_dir = {1.f, 0.f, 0.f};
  lib_core::Vector3 color = {1.f};

  lib_core::Vector3 delta_pos;
  lib_core::Vector2 data_cutoffs;

  float constant = 1.0f;
  float linear = 0.14f;
  float quadratic = 0.07f;
  float max_radius = 10.f;

  float view_depth[3];
  int shadow_resolutions[3];

  static Light Parse(ct::string &buffer, size_t &cursor) {
    Light l;
    if (cu::ScrollCursor(buffer, cursor, '{')) {
      auto type = cu::ParseType(buffer, cursor);
      while (!type.empty()) {
        if (type.compare("Position") == 0)
          l.data_pos = cu::ParseVector<lib_core::Vector3, 3>(
              cu::ParseValue(buffer, cursor));
        else if (type.compare("Direction") == 0)
          l.data_dir = cu::ParseVector<lib_core::Vector3, 3>(
              cu::ParseValue(buffer, cursor));
        else if (type.compare("Shadows") == 0) {
          auto val = cu::ParseValue(buffer, cursor);
          if (!val.empty()) {
            l.shadow_resolutions[0] = cu::Parse<int>(val);
          }
          l.cast_shadows = true;
        } else if (type.compare("Radius") == 0) {
          l.max_radius =
              cu::Parse<float>(cu::ParseValue(buffer, cursor));
        } else if (type.compare("Type") == 0) {
          auto val = cu::ParseValue(buffer, cursor);
          if (val.compare("kPoint") == 0)
            l.type = kPoint;
          else if (val.compare("kSpot") == 0)
            l.type = kSpot;
          else if (val.compare("kDir") == 0)
            l.type = kDir;
        } else if (type.compare("Color") == 0) {
          l.color = cu::ParseVector<lib_core::Vector3, 3>(
              cu::ParseValue(buffer, cursor));
        } else if (type.compare("Cutoffs") == 0) {
          l.data_cutoffs = cu::ParseVector<lib_core::Vector2, 2>(
              cu::ParseValue(buffer, cursor));
        } else if (type.compare("Linear") == 0) {
          l.linear = cu::Parse<float>(cu::ParseValue(buffer, cursor));
        } else if (type.compare("Constant") == 0) {
          l.constant =
              cu::Parse<float>(cu::ParseValue(buffer, cursor));
        } else if (type.compare("Quadratic") == 0) {
          l.quadratic =
              cu::Parse<float>(cu::ParseValue(buffer, cursor));
        }

        type = cu::ParseType(buffer, cursor);
      }
    }
    return l;
  }
};
}  // namespace lib_graphics
