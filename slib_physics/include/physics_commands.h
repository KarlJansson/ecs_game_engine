#pragma once
#include "system_manager.h"
#include "vector_def.h"

namespace lib_physics {
struct PhysicsInit {
  ct::dyn_array<lib_core::Vector3> verts;
  ct::dyn_array<uint32_t> inds;
};

class AddMeshSourceCommmand : public lib_core::Command {
 public:
  AddMeshSourceCommmand() = default;
  AddMeshSourceCommmand(size_t mesh_id, PhysicsInit pinit)
      : mesh_id(mesh_id), physic_data(std::move(pinit)) {}

  size_t mesh_id;
  PhysicsInit physic_data;
};

class RemoveMeshSourceCommmand : public lib_core::Command {
 public:
  RemoveMeshSourceCommmand() = default;
  RemoveMeshSourceCommmand(size_t mesh_id) : mesh_id(mesh_id) {}
  size_t mesh_id;
};
}  // namespace lib_physics
