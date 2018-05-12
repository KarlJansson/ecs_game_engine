#pragma once
#include <random>
#include "camera.h"
#include "graphics_commands.h"
#include "particle_emitter.h"
#include "particle_system.h"

namespace lib_graphics {
class GlParticleSystem : public ParticleSystem {
 public:
  GlParticleSystem(const lib_core::EngineCore* engine);

  void DrawUpdate(lib_graphics::Renderer* renderer,
                  lib_gui::TextSystem* text_renderer) override;
  void FinalizeSystem() override;

  void DrawParticleEmitter(lib_core::Entity entity,
                           const lib_graphics::Camera& camera,
                           const TextureDesc& depth_desc) override;

  void PurgeGpuResources() override;
  void RebuildGpuResources() override;

 private:
  struct ParticleVertex {
    lib_core::Vector4 random;
    lib_core::Vector3 position;
    lib_core::Vector3 velocity;
    lib_core::Vector2 corners;
    float start_time;
  };

  void AddParticleEmitter(lib_core::Entity ent, const ParticleEmitter& emitter);

  void AddParticlePoint(ParticleVertex* buffer, const ParticleEmitter* emitter,
                        const Transform* transform);
  void AddParticleCube(ParticleVertex* buffer, const ParticleEmitter* emitter,
                       const Transform* transform);
  void AddParticleSphere(ParticleVertex* buffer, const ParticleEmitter* emitter,
                         const Transform* transform);
  void AddParticleCircle(ParticleVertex* buffer, const ParticleEmitter* emitter,
                         const Transform* transform);
  void AddParticleSquare(ParticleVertex* buffer, const ParticleEmitter* emitter,
                         const Transform* transform);

  bool get_uniform_locations_ = true;
  Material particle_material_;
  int shader_uniforms_[14];

  std::default_random_engine rng_engine_;

  struct EmitterGpuData {
    unsigned vao;
    unsigned vbo;
    unsigned ebo;

    float delta_rest = .0f;
    float last_time = 0.f;
    size_t particle_emitted = 0;
    size_t head = 0;
    size_t particle_count = 0;
    size_t buffer_size;
    ParticleVertex* mapped_ptr = nullptr;
    void* sync_obj = nullptr;
    ct::dyn_array<ParticleVertex> saved_particles;
  };

  ct::string frag_shader_, vert_shader_;
  size_t shader_id_;
  size_t add_callback_, remove_callback_;
  ct::hash_set<lib_core::Entity> add_emitter_, remove_emitter_;
  ct::hash_map<lib_core::Entity, EmitterGpuData> emitter_data_;
};
}  // namespace lib_graphics