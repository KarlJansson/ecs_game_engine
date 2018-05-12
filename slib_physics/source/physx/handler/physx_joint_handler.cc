#include "physx_joint_handler.h"
#include "actor.h"
#include "entity_manager.h"
#include "joint.h"

namespace lib_physics {
PhysxJointHandler::PhysxJointHandler(physx::PxPhysics *phys,
                                     PhysxActorHandler *actor_handler)
    : physics_(phys), actor_handler_(actor_handler) {
  joint_added_callback = g_ent_mgr.RegisterAddComponentCallback<Joint>(
      [&](lib_core::Entity entity) { add_joint_.push_back(entity); });
  joint_removed_callback = g_ent_mgr.RegisterRemoveComponentCallback<Joint>(
      [&](lib_core::Entity entity) { remove_joint_.push_back(entity); });
}

PhysxJointHandler::~PhysxJointHandler() {
  g_ent_mgr.UnregisterAddComponentCallback<Joint>(joint_added_callback);
  g_ent_mgr.UnregisterRemoveComponentCallback<Joint>(joint_removed_callback);
}

void PhysxJointHandler::Update() {
  for (int i = int(add_joint_.size()) - 1; i >= 0; --i)
    if (AddNewJointData(add_joint_[i]))
      add_joint_.erase(add_joint_.begin() + i);

  for (int i = int(remove_joint_.size()) - 1; i >= 0; --i)
    if (RemoveJointData(remove_joint_[i]))
      remove_joint_.erase(remove_joint_.begin() + i);

  auto joint_comps = g_ent_mgr.GetNewCbt<Joint>();
  if (joint_comps) {
    auto old_joint_comps = g_ent_mgr.GetOldCbt<Joint>();
    auto joint_ents = g_ent_mgr.GetEbt<Joint>();
    auto joint_update = g_ent_mgr.GetNewUbt<Joint>();

    for (int i = 0; i < joint_update->size(); ++i) {
      if (!(*joint_update)[i]) continue;

      auto &joint = joint_comps->at(i);
      auto &old_joint = old_joint_comps->at(i);

      auto joint_it = joints_.find(joint_ents->at(i));
      if (joint_it == joints_.end()) continue;
      (*joint_update)[i] = false;

      physx::PxJoint *joint_ptr = joint_it->second;
      if (joint_ptr->getConstraintFlags() & physx::PxConstraintFlag::eBROKEN) {
        joint.broken = true;
        continue;
      }

      if (joint.set_limits) {
        if (joint_ptr->is<physx::PxRevoluteJoint>()) {
          auto rev_joint = static_cast<physx::PxRevoluteJoint *>(joint_ptr);
          rev_joint->setLimit({joint.limits[0], joint.limits[1]});
        } else if (joint_ptr->is<physx::PxDistanceJoint>()) {
          auto dist_joint = static_cast<physx::PxDistanceJoint *>(joint_ptr);
          dist_joint->setMinDistance(joint.limits[0]);
          dist_joint->setMaxDistance(joint.limits[1]);
        } else if (joint_ptr->is<physx::PxSphericalJoint>()) {
          auto sp_joint = static_cast<physx::PxSphericalJoint *>(joint_ptr);
          if (joint.limits[0] != joint.limits[1]) {
            sp_joint->setLimitCone(physx::PxJointLimitCone(
                joint.limits[0], joint.limits[1], 0.01f));
            sp_joint->setSphericalJointFlag(
                physx::PxSphericalJointFlag::eLIMIT_ENABLED, true);
          }
        } else if (joint_ptr->is<physx::PxPrismaticJoint>()) {
          auto pris_joint = static_cast<physx::PxPrismaticJoint *>(joint_ptr);
          if (joint.limits[0] != joint.limits[1]) {
            pris_joint->setLimit(physx::PxJointLinearLimitPair(
                physx::PxTolerancesScale(), joint.limits[0], joint.limits[1],
                0.01f));
            pris_joint->setPrismaticJointFlag(
                physx::PxPrismaticJointFlag::eLIMIT_ENABLED, true);
          }
        }
      }

      joint = old_joint;
      joint.set_limits = false;
    }
  }
}

bool PhysxJointHandler::AddNewJointData(lib_core::Entity entity) {
  auto joint = g_ent_mgr.GetNewCbeR<Joint>(entity);
  if (!joint) return false;

  bool rigid_found = false;
  physx::PxActorType::Enum a_type;

  physx::PxActor *actor_1_ptr = nullptr, *actor_2_ptr = nullptr;
  auto actor_it = actor_handler_->actors_.find(joint->actor_1);
  if (actor_it == actor_handler_->actors_.end()) return false;

  actor_1_ptr = actor_it->second;

  actor_it = actor_handler_->actors_.find(joint->actor_2);
  if (actor_it != actor_handler_->actors_.end()) actor_2_ptr = actor_it->second;

  a_type = actor_1_ptr->getType();
  if (a_type == physx::PxActorType::eRIGID_DYNAMIC) rigid_found = true;

  if (actor_2_ptr) {
    a_type = actor_2_ptr->getType();
    if (a_type == physx::PxActorType::eRIGID_DYNAMIC) rigid_found = true;
  }

  cu::AssertWarning(rigid_found, "Rigid body not found.", __FILE__, __LINE__);
  if (!rigid_found) return false;

  const float convert_calc = (PI / 180.0f) * 0.5f;
  float t0 = std::cos(joint->rot_1[2] * convert_calc);
  float t1 = std::sin(joint->rot_1[2] * convert_calc);
  float t2 = std::cos(joint->rot_1[0] * convert_calc);
  float t3 = std::sin(joint->rot_1[0] * convert_calc);
  float t4 = std::cos(joint->rot_1[1] * convert_calc);
  float t5 = std::sin(joint->rot_1[1] * convert_calc);

  physx::PxQuat q;
  q.w = t0 * t2 * t4 + t1 * t3 * t5;
  q.x = -(t0 * t3 * t4 - t1 * t2 * t5);
  q.y = -(t0 * t2 * t5 + t1 * t3 * t4);
  q.z = -(t1 * t2 * t4 - t0 * t3 * t5);
  auto frame_1 =
      physx::PxTransform(joint->pos_1[0], joint->pos_1[1], joint->pos_1[2], q);

  t0 = std::cos(joint->rot_2[2] * convert_calc);
  t1 = std::sin(joint->rot_2[2] * convert_calc);
  t2 = std::cos(joint->rot_2[0] * convert_calc);
  t3 = std::sin(joint->rot_2[0] * convert_calc);
  t4 = std::cos(joint->rot_2[1] * convert_calc);
  t5 = std::sin(joint->rot_2[1] * convert_calc);

  q.w = t0 * t2 * t4 + t1 * t3 * t5;
  q.x = -(t0 * t3 * t4 - t1 * t2 * t5);
  q.y = -(t0 * t2 * t5 + t1 * t3 * t4);
  q.z = -(t1 * t2 * t4 - t0 * t3 * t5);
  auto frame_2 =
      physx::PxTransform(joint->pos_2[0], joint->pos_2[1], joint->pos_2[2], q);

  physx::PxJoint *joint_ptr;
  physx::PxRigidActor *rigid_actor_1 = nullptr, *rigid_actor_2 = nullptr;
  if (actor_1_ptr)
    rigid_actor_1 = static_cast<physx::PxRigidActor *>(actor_1_ptr);
  if (actor_2_ptr)
    rigid_actor_2 = static_cast<physx::PxRigidActor *>(actor_2_ptr);

  switch (joint->type) {
    case Joint::kDistance: {
      auto dist_joint = physx::PxDistanceJointCreate(
          *physics_, rigid_actor_1, frame_1, rigid_actor_2, frame_2);
      dist_joint->setMinDistance(joint->limits[0]);
      dist_joint->setMaxDistance(joint->limits[1]);
      dist_joint->setDistanceJointFlag(
          physx::PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, true);
      dist_joint->setDistanceJointFlag(
          physx::PxDistanceJointFlag::eMIN_DISTANCE_ENABLED, true);
      joint_ptr = dist_joint;
    } break;
    case Joint::kRevolute: {
      auto rev_joint = physx::PxRevoluteJointCreate(
          *physics_, rigid_actor_1, frame_1, rigid_actor_2, frame_2);

      if (joint->limits[0] != joint->limits[1]) {
        rev_joint->setLimit(physx::PxJointAngularLimitPair(
            joint->limits[0], joint->limits[1], 0.0001f));
        rev_joint->setRevoluteJointFlag(
            physx::PxRevoluteJointFlag::eLIMIT_ENABLED, true);
      }

      rev_joint->setDriveVelocity(10.0f);
      rev_joint->setRevoluteJointFlag(
          physx::PxRevoluteJointFlag::eDRIVE_ENABLED, false);
      joint_ptr = rev_joint;
    } break;
    case Joint::kFixed: {
      auto fixed_joint = physx::PxFixedJointCreate(
          *physics_, rigid_actor_1, frame_1, rigid_actor_2, frame_2);
      joint_ptr = fixed_joint;
    } break;
    case Joint::kSpherical: {
      auto spherical_joint = physx::PxSphericalJointCreate(
          *physics_, rigid_actor_1, frame_1, rigid_actor_2, frame_2);
      if (joint->limits[0] != joint->limits[1]) {
        spherical_joint->setLimitCone(
            physx::PxJointLimitCone(joint->limits[0], joint->limits[1], 0.01f));
        spherical_joint->setSphericalJointFlag(
            physx::PxSphericalJointFlag::eLIMIT_ENABLED, true);
      }
      joint_ptr = spherical_joint;
    } break;
    case Joint::kPrismatic: {
      auto prismatic_joint = physx::PxPrismaticJointCreate(
          *physics_, rigid_actor_1, frame_1, rigid_actor_2, frame_2);
      if (joint->limits[0] != joint->limits[1]) {
        prismatic_joint->setLimit(physx::PxJointLinearLimitPair(
            physx::PxTolerancesScale(), joint->limits[0], joint->limits[1],
            0.01f));
        prismatic_joint->setPrismaticJointFlag(
            physx::PxPrismaticJointFlag::eLIMIT_ENABLED, true);
      }
      joint_ptr = prismatic_joint;
    } break;
    default:
      cu::AssertWarning(false, "Faulty joint type specified.", __FILE__,
                        __LINE__);
      return false;
  }

  if (joint->break_force_torque[0] > .0f || joint->break_force_torque[1] > .0f)
    joint_ptr->setBreakForce(joint->break_force_torque[0],
                             joint->break_force_torque[1]);

  auto it = joints_.find(entity);
  if (it != joints_.end()) it->second->release();

  joints_[entity] = joint_ptr;
  return true;
}

bool PhysxJointHandler::RemoveJointData(lib_core::Entity entity) {
  auto it = joints_.find(entity);
  if (it != joints_.end()) {
    it->second->release();
    joints_.erase(it);
  }
  return true;
}

}  // namespace lib_physics
