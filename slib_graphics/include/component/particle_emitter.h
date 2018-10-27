#pragma once
#include "core_utilities.h"
#include "vector_def.h"

namespace lib_graphics {
class ParticleEmitter {
 public:
  ParticleEmitter() = default;
  ParticleEmitter(size_t max_particles, size_t particle_texture_id)
      : max_particles(max_particles), particle_texture(particle_texture_id) {}

  enum EmitterType { kPoint, kCircle, kSquare, kSphere, kCube };

  EmitterType emitt_type{kPoint};
  float emitter_time{0.f};
  bool loop{true};

  size_t max_particles{500};
  size_t particle_texture;

  float particle_life{2.f};
  float emitting_speed{1.f};
  float end_velocity{5.f};
  float center_velocity{0.01f};
  lib_core::Vector4 type_data{0.f};
  lib_core::Vector4 random_scale{.1f};
  lib_core::Vector3 start_velocity{.0f};

  lib_core::Vector4 start_color{1.f};
  lib_core::Vector4 end_color{0.f};

  lib_core::Vector3 gravity{0.f};

  lib_core::Vector2 start_size{.5f, 1.f};
  lib_core::Vector2 end_size{1.f, 2.f};

  lib_core::Vector2 rotate_speed{.1f, .5f};

  static ParticleEmitter Parse(ct::string &buffer, size_t &cursor) {
    ParticleEmitter e;
    if (cu::ScrollCursor(buffer, cursor, '{')) {
      auto type = cu::ParseType(buffer, cursor);
      while (!type.empty()) {
        if (type.compare("StartVelocity") == 0)
          e.start_velocity = cu::ParseVector<lib_core::Vector3, 3>(
              cu::ParseValue(buffer, cursor));
        else if (type.compare("StartColor") == 0)
          e.start_color = cu::ParseVector<lib_core::Vector4, 4>(
              cu::ParseValue(buffer, cursor));
        else if (type.compare("EndColor") == 0)
          e.end_color = cu::ParseVector<lib_core::Vector4, 4>(
              cu::ParseValue(buffer, cursor));
        else if (type.compare("TypeData") == 0)
          e.type_data = cu::ParseVector<lib_core::Vector4, 4>(
              cu::ParseValue(buffer, cursor));
        else if (type.compare("RandomScale") == 0)
          e.random_scale = cu::ParseVector<lib_core::Vector4, 4>(
              cu::ParseValue(buffer, cursor));
        else if (type.compare("Gravity") == 0)
          e.gravity = cu::ParseVector<lib_core::Vector3, 3>(
              cu::ParseValue(buffer, cursor));
        else if (type.compare("StartSize") == 0)
          e.start_size = cu::ParseVector<lib_core::Vector2, 2>(
              cu::ParseValue(buffer, cursor));
        else if (type.compare("EndSize") == 0)
          e.end_size = cu::ParseVector<lib_core::Vector2, 2>(
              cu::ParseValue(buffer, cursor));
        else if (type.compare("RotateSpeed") == 0)
          e.rotate_speed = cu::ParseVector<lib_core::Vector2, 2>(
              cu::ParseValue(buffer, cursor));
        else if (type.compare("MaxParticles") == 0)
          e.max_particles =
              cu::Parse<size_t>(cu::ParseValue(buffer, cursor));
        else if (type.compare("EmittingSpeed") == 0)
          e.emitting_speed =
              cu::Parse<float>(cu::ParseValue(buffer, cursor));
        else if (type.compare("EndVelocity") == 0)
          e.end_velocity =
              cu::Parse<float>(cu::ParseValue(buffer, cursor));
        else if (type.compare("ParticleLife") == 0)
          e.particle_life =
              cu::Parse<float>(cu::ParseValue(buffer, cursor));
        else if (type.compare("CenterVelocity") == 0)
          e.center_velocity =
              cu::Parse<float>(cu::ParseValue(buffer, cursor));
        else if (type.compare("Loop") == 0)
          e.loop = cu::Parse<bool>(cu::ParseValue(buffer, cursor));
        else if (type.compare("EmittType") == 0) {
          auto val = cu::ParseValue(buffer, cursor);
          if (val.compare("kPoint") == 0)
            e.emitt_type = kPoint;
          else if (val.compare("kCircle") == 0)
            e.emitt_type = kCircle;
          else if (val.compare("kSquare") == 0)
            e.emitt_type = kSquare;
          else if (val.compare("kSphere") == 0)
            e.emitt_type = kSphere;
          else if (val.compare("kCube") == 0)
            e.emitt_type = kCube;
        }

        type = cu::ParseType(buffer, cursor);
      }
    }
    return e;
  }
};
}  // namespace lib_graphics
