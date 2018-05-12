#include "particle_system.h"
#include "particle_emitter.h"

namespace lib_graphics {
ParticleSystem::ParticleSystem(const lib_core::EngineCore *engine)
    : engine_(engine) {}

void ParticleSystem::LogicUpdate(float dt) {
  auto update_emitter = g_ent_mgr.GetNewUbt<ParticleEmitter>();
  if (update_emitter) {
    auto emitters = g_ent_mgr.GetNewCbt<ParticleEmitter>();
    auto old_emitter = g_ent_mgr.GetOldCbt<ParticleEmitter>();
    for (size_t i = 0; i < update_emitter->size(); ++i) {
      if ((*update_emitter)[i]) {
        (*emitters)[i].emitter_time = (*old_emitter)[i].emitter_time + dt;
        (*update_emitter)[i] = false;
      }
    }
  }
}
}  // namespace lib_graphics
