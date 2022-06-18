#include "transform_system.h"
#include "actor.h"
#include "character.h"
#include "culling_system.h"
#include "light.h"
#include "mesh.h"
#include "range_iterator.hpp"
#include "transform.h"
#include "trigger.h"

#include <execution>

namespace lib_graphics {
void TransformSystem::LogicUpdate(float dt) {
  auto trans_comps = g_ent_mgr.GetNewCbt<Transform>();

  if (trans_comps) {
    auto trans_comps_old = g_ent_mgr.GetOldCbt<Transform>();
    auto trans_update = g_ent_mgr.GetNewUbt<Transform>();
    auto entity_vec = g_ent_mgr.GetEbt<Transform>();

    auto update_func = [&](size_t i) {
      if (!(*trans_update)[i]) return;

      auto actor = g_ent_mgr.GetNewCbeR<lib_physics::Actor>(entity_vec->at(i));
      auto character =
          g_ent_mgr.GetOldCbeR<lib_physics::Character>(entity_vec->at(i));
      UpdateTransform(trans_comps->at(i), trans_comps_old->at(i), actor,
                      character);
      g_ent_mgr.MarkForUpdate<lib_graphics::Light>(entity_vec->at(i));
      g_ent_mgr.MarkForUpdate<lib_graphics::CullingSystem::LightOctreeFlag>(
          entity_vec->at(i));
      g_ent_mgr.MarkForUpdate<lib_graphics::CullingSystem::MeshOctreeFlag>(
          entity_vec->at(i));
      g_ent_mgr.MarkForUpdate<lib_physics::Trigger>(entity_vec->at(i));

      (*trans_update)[i] = false;
    };

    auto r = range(0, trans_comps->size());
    std::for_each(std::execution::par_unseq, std::begin(r), std::end(r),
                  update_func);
  }
}

void TransformSystem::UpdateTransform(Transform& trans, Transform& old,
                                      const lib_physics::Actor* actor,
                                      const lib_physics::Character* character) {
  lib_core::Quaternion q, orb_q;
  trans.scale_ = old.scale_;

  if (!actor && !character) {
    trans.position_ = old.position_;
    trans.rotation_ = old.rotation_;
    trans.orbit_offset_ = old.orbit_offset_;
    trans.orbit_rotation_ = old.orbit_rotation_;

    float pi2 = PI * 2;
    for (int i = 0; i < 3; ++i) {
      if (trans.rotation_[i] > pi2)
        trans.rotation_[i] -= pi2;
      else if (trans.rotation_[i] < -pi2)
        trans.rotation_[i] += pi2;

      if (trans.orbit_rotation_[i] > pi2)
        trans.orbit_rotation_[i] -= pi2;
      else if (trans.orbit_rotation_[i] < -pi2)
        trans.orbit_rotation_[i] += pi2;
    }

    if (!trans.orbit_rotation_.Zero()) orb_q.FromAngle(trans.orbit_rotation_);
    q.FromAngle(trans.rotation_);
  } else {
    if (actor) {
      trans.position_ = actor->pos;
      q = actor->rot;
      q.GetAngles(trans.rotation_);
    } else if (character) {
      trans.position_ = character->pos;
    }
  }

  trans.world_.Identity();

  auto orb_translate = trans.orbit_offset_;
  if (!actor && !trans.orbit_rotation_.Zero())
    orb_q.RotateVector(orb_translate);

  trans.world_.Translate(trans.position_);
  if (!actor && !orb_translate.Zero()) trans.world_.Translate(orb_translate);
  trans.world_ *= q.RotationMatrix();
  trans.world_.Scale(trans.scale_);

  trans.world_.Left(trans.left_);
  trans.world_.Up(trans.up_);
  trans.world_.Forward(trans.forward_);

  trans.left_.Normalize();
  trans.up_.Normalize();
  trans.forward_.Normalize();
}
}  // namespace lib_graphics
