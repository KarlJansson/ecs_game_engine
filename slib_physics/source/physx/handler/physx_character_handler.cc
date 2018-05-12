#include "physx_character_handler.h"
#include "actor.h"
#include "camera.h"
#include "character.h"
#include "entity_manager.h"
#include "gui_text.h"

namespace lib_physics {
PhysxCharacterHandler::PhysxCharacterHandler(physx::PxPhysics* phys,
                                             physx::PxScene* scene)
    : physics_(phys) {
  controller_manager_ = PxCreateControllerManager(*scene);
  report_callback = std::make_unique<PhysxReportCallback>();

  add_callback_ = g_ent_mgr.RegisterAddComponentCallback<Character>(
      [&](lib_core::Entity ent) { add_character_.push_back(ent); });
  remove_callback_ = g_ent_mgr.RegisterRemoveComponentCallback<Character>(
      [&](lib_core::Entity ent) { remove_character_.push_back(ent); });
}

PhysxCharacterHandler::~PhysxCharacterHandler() {
  controller_manager_->release();

  g_ent_mgr.UnregisterAddComponentCallback<Character>(add_callback_);
  g_ent_mgr.UnregisterRemoveComponentCallback<Character>(remove_callback_);
}

void PhysxCharacterHandler::UpdateCharacters(float dt) {
  for (auto& e : add_character_) CreateCharacterController(e);
  add_character_.clear();
  for (auto& e : remove_character_) RemoveCharacterController(e);
  remove_character_.clear();

  auto characters = g_ent_mgr.GetNewCbt<Character>();

  if (characters) {
    float gravity_acc = -9.82f * dt;
    auto old_characters = g_ent_mgr.GetOldCbt<Character>();
    auto char_ents = g_ent_mgr.GetEbt<Character>();
    auto char_update = g_ent_mgr.GetNewUbt<Character>();

    for (int i = 0; i < char_update->size(); ++i) {
      if (!(*char_update)[i]) continue;

      g_ent_mgr.MarkForUpdate<lib_graphics::Camera>(char_ents->at(i));

      auto& c = characters->at(i);
      auto& old_c = old_characters->at(i);
      auto character = controllers_[char_ents->at(i)];

      if (c.teleport) {
        character->setPosition(
            physx::PxExtendedVec3(c.port_loc[0], c.port_loc[1], c.port_loc[2]));
        c.teleport = false;
        g_ent_mgr.MarkForUpdate<lib_physics::Character>(char_ents->at(i));
      }

      if (c.resize) {
        character->resize(c.height);
        old_c.height = c.height;
        c.resize = false;
      }

      c.vert_velocity = old_c.vert_velocity;
      c.vert_velocity += gravity_acc * 3.f;
      if (c.vert_velocity < 0.2f) c.vert_velocity += gravity_acc * 2.5f;
      c.vert_velocity += c.disp[1];
      auto add_position = dt * c.vert_velocity;

      auto filter = physx::PxControllerFilters();
      auto flags =
          character->move(physx::PxVec3(c.disp[0], add_position, c.disp[2]),
                          0.001f, dt, filter);
      c.disp.ZeroMem();

      if (flags & physx::PxControllerCollisionFlag::eCOLLISION_UP &&
          c.vert_velocity > 0.f)
        c.vert_velocity = 0.f;

      if (flags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN) {
        c.vert_velocity = 0.f;
        c.grounded = true;
      } else {
        c.grounded = false;
      }

      auto pos = character->getPosition();
      pos += (character->getPosition() - character->getFootPosition()) * 0.9f;
      lib_core::Vector3 pos_vec = {float(pos.x), float(pos.y), float(pos.z)};
      c.pos = pos_vec;
      if (c.pos[0] == old_c.pos[0] && c.pos[1] == old_c.pos[1] &&
          c.pos[2] == old_c.pos[2] && c.vert_velocity == old_c.vert_velocity &&
          c.vert_velocity == 0.f)
        (*char_update)[i] = false;
    }
  }
}

void PhysxCharacterHandler::CreateCharacterController(lib_core::Entity ent) {
  RemoveCharacterController(ent);
  auto ent_ptr = g_ent_mgr.GetNewCbeR<Character>(ent);

  physx::PxCapsuleControllerDesc desc;
  desc.contactOffset = 0.2f;
  desc.density = ent_ptr->density;
  desc.position.x = ent_ptr->pos[0];
  desc.position.y = ent_ptr->pos[1];
  desc.position.z = ent_ptr->pos[2];
  desc.height = ent_ptr->height;
  desc.radius = ent_ptr->radius;
  physx::PxMaterial* material = physics_->createMaterial(1.5, 1.5, 1.5);
  desc.material = material;
  desc.reportCallback = report_callback.get();

  auto controller = controller_manager_->createController(desc);
  controllers_[ent] = controller;

  material->release();
}

void PhysxCharacterHandler::RemoveCharacterController(lib_core::Entity ent) {
  if (controllers_.find(ent) != controllers_.end()) {
    controllers_[ent]->release();
    controllers_.erase(ent);
  }
}

void PhysxCharacterHandler::PhysxReportCallback::onShapeHit(
    const physx::PxControllerShapeHit& hit) {
  auto type = hit.actor->getType();
  if (type == physx::PxActorType::eRIGID_DYNAMIC) {
    auto rigid_dynamic = static_cast<physx::PxRigidDynamic*>(hit.actor);
    if (rigid_dynamic->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC)
      return;
    if (hit.dir.y > -0.5)
      rigid_dynamic->addForce(hit.dir * hit.length * 10,
                              physx::PxForceMode::eIMPULSE);
  }
}

void PhysxCharacterHandler::PhysxReportCallback::onControllerHit(
    const physx::PxControllersHit& hit) {}

void PhysxCharacterHandler::PhysxReportCallback::onObstacleHit(
    const physx::PxControllerObstacleHit& hit) {}
}  // namespace lib_physics
