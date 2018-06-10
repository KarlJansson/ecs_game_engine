#include "scripted_system.h"
#include <experimental/filesystem>
#include <fstream>
#include <regex>
#include <utility>
#include "actor.h"
#include "fps_camera_system.h"
#include "graphics_commands.h"
#include "gui_text.h"
#include "joint.h"
#include "light.h"
#include "material_system.h"
#include "mesh.h"
#include "mesh_system.h"
#include "particle_emitter.h"
#include "renderer.h"
#include "text_system.h"
#include "transform.h"
#include "trigger.h"

namespace lib_core {
ScriptedSystem::ScriptedSystem(lib_core::EngineCore *engine,
                               ct::string script_path, lib_core::Vector3 offset)
    : engine_(engine), script_path_(std::move(script_path)), offset_(offset) {
  mod_ = offset_[0] < .0f ? -1.f : 1.f;

  AddType("ForLoop", [&]() { ParseForLoop(); });
  AddType("Material", [&]() { ParseMaterial(); });
  AddType("MeshActor", [&]() { ParseActorMesh(); });
  AddType("ActorJoint", [&]() { ParseActorJoint(); });
  AddType("ParticleEmitter", [&]() { ParseParticleEmitter(); });
}

void ScriptedSystem::InitSystem() {
  if (!script_path_.empty()) {
    auto timestamp =
        std::experimental::filesystem::last_write_time(script_path_);
    time_stamp_ = decltype(timestamp)::clock::to_time_t(timestamp);
    ParseScript();
  }
}

void ScriptedSystem::LogicUpdate(float dt) {
  for (auto &p : units_) p->Update(dt);

  check_timer_ -= dt;
  if (!script_path_.empty() && check_timer_ < 0.f) {
    auto timestamp =
        std::experimental::filesystem::last_write_time(script_path_);
    size_t mod_time = decltype(timestamp)::clock::to_time_t(timestamp);
    if (time_stamp_ != mod_time) {
      PurgeEntities();
      PurgeSystem();
      ParseScript();
      time_stamp_ = mod_time;
    }

    check_timer_ += 1.f;
  }
}

void ScriptedSystem::FinalizeSystem() { PurgeSystem(); }

void ScriptedSystem::AddType(const std::string &name,
                             std::function<void(void)> func) {
  valid_types_[name] = std::move(func);
}

void ScriptedSystem::Activate() {
  if (!active_) {
    Unit::Activate();
    for (auto &p : units_) p->Activate();
  }
}

void ScriptedSystem::Deactivate() {
  if (active_) {
    Unit::Deactivate();
    for (auto &p : units_) p->Deactivate();
  }
}

void ScriptedSystem::PurgeSystem() {
  for (auto &p : material_map_)
    issue_command(lib_graphics::RemoveMaterialCommand(p.second));
  units_.clear();
}

void ScriptedSystem::ParseScript() {
  std::ifstream script(script_path_, std::ios::binary | std::ios::ate);

  if (!script.fail()) {
    size_t size = script.tellg();
    script_buffer_.assign(size, ' ');
    script.seekg(0, std::ios::beg);
    script.read(script_buffer_.data(), size);
    script.close();

    script_cursor_ = 0;
    while (script_cursor_ < script_buffer_.size()) {
      auto type_token = cu::ParseType(script_buffer_, script_cursor_);
      if (!type_token.empty()) {
        auto type_it = valid_types_.find(type_token);
        if (type_it != valid_types_.end()) {
          if (cu::ScrollCursor(script_buffer_, script_cursor_, '{'))
            type_it->second();
        } else if (type_token.compare("Light") == 0) {
          auto ent = CreateScopedEntity();
          auto light = lib_graphics::Light::Parse(
              script_buffer_, script_cursor_, variable_map_);
          light.data_pos[0] *= mod_;
          light.data_pos += offset_;
          g_ent_mgr.AddComponent(
              ent, lib_graphics::Transform(light.data_pos, {0.f}, {1.f}));
          g_ent_mgr.AddComponent(ent, light);
        } else {
          if (type_token[0] == '[' && type_token.back() == ']') {
            variable_map_[type_token] =
                cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
          } else {
            // Skip unknown type
            cu::ScrollCursor(script_buffer_, script_cursor_, '{');
            size_t counter = 1;
            while (counter != 0) {
              if (script_buffer_[script_cursor_] == '{')
                ++counter;
              else if (script_buffer_[script_cursor_] == '}')
                --counter;
              ++script_cursor_;
            }
            ++script_cursor_;
          }
        }
      } else
        break;
    }
  }
}

void ScriptedSystem::ParseForLoop() {
  int range = 0;
  float increment = 1.f;
  float start = 1.f;
  ct::string ops;
  ct::string ind_id = "index";

  ct::string type = cu::ParseType(script_buffer_, script_cursor_);
  while (!type.empty()) {
    if (type.compare("Increment") == 0) {
      increment = cu::Parse<float>(
          cu::ParseValue(script_buffer_, script_cursor_, variable_map_));
    } else if (type.compare("Start") == 0) {
      start = cu::Parse<float>(
          cu::ParseValue(script_buffer_, script_cursor_, variable_map_));
    } else if (type.compare("IndexId") == 0) {
      ind_id = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
    } else if (type.compare("Range") == 0) {
      range = cu::Parse<int>(
          cu::ParseValue(script_buffer_, script_cursor_, variable_map_));
    } else if (type.compare("Operation") == 0) {
      ops = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
    }

    type = cu::ParseType(script_buffer_, script_cursor_);
  }

  if (range > 0) {
    ct::string tmp_op;
    for (int i = 0; i < range; ++i) {
      tmp_op = std::regex_replace(ops, std::regex(ind_id),
                                  std::to_string(start + increment * float(i)));
      script_buffer_.insert(script_buffer_.begin() + script_cursor_,
                            tmp_op.begin(), tmp_op.end());
    }
  }
}

lib_graphics::Material ScriptedSystem::ParseMaterial() {
  lib_graphics::Material material;
  auto material_system = engine_->GetMaterial();

  ct::string id;
  ct::string value;
  ct::string type = cu::ParseType(script_buffer_, script_cursor_);
  while (!type.empty()) {
    if (type.compare("Id") == 0) {
      id = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
    } else if (type.compare("Texture") == 0) {
      value = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);

      int int_val;
      std::stringstream ss;
      ss << value.substr(value.find_last_of('_') + 1, value.size());
      ss >> int_val;
      if (!ss.fail()) {
        value = value.substr(0, value.find_last_of('_'));
        material = material_system->CreateTexturedMaterial(
            value + "_albedo_" + std::to_string(int_val) + ".png",
            value + "_normal.png", value + "_rme.png");
      } else {
        material = material_system->CreateTexturedMaterial(
            value + "_albedo.png", value + "_normal.png", value + "_rme.png");
      }
    }

    type = cu::ParseType(script_buffer_, script_cursor_);
  }

  if (!id.empty()) {
    auto cmd = lib_graphics::AddMaterialCommand(material);
    issue_command(cmd);
    material_map_[std::hash<ct::string>{}(id)] = cmd.MaterialId();
  }

  return material;
}

void ScriptedSystem::ParseActorMesh() {
  ct::string model_str;
  size_t mesh_id = 0;
  size_t material_id = lib_core::EngineCore::stock_material_untextured;
  bool pickupable = false;
  bool pullable = false;
  bool ccd = false;
  bool add_light = false;
  float density = 1.f;
  float stat_friction = 1.f;
  float dyn_friction = 1.f;
  float restitution = .1f;
  lib_physics::Actor::ActorType actor_type = lib_physics::Actor::kStatic;
  lib_graphics::Transform transform;
  lib_graphics::Light light;
  lib_graphics::Mesh mesh;
  ct::string name_id;
  ct::string type = cu::ParseType(script_buffer_, script_cursor_);
  while (!type.empty()) {
    if (type.compare("MaterialId") == 0) {
      material_id = MaterialId();
    } else if (type.compare("NameId") == 0) {
      name_id = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
    } else if (type.compare("Transform") == 0) {
      transform = lib_graphics::Transform::Parse(script_buffer_, script_cursor_,
                                                 variable_map_);
    } else if (type.compare("Light") == 0) {
      add_light = true;
      light = lib_graphics::Light::Parse(script_buffer_, script_cursor_,
                                         variable_map_);
    } else if (type.compare("Mesh") == 0) {
      mesh = lib_graphics::Mesh::Parse(script_buffer_, script_cursor_,
                                       variable_map_);
    } else if (type.compare("Interact") == 0) {
      auto val = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
      if (val.compare("Pickup") == 0)
        pickupable = true;
      else if (val.compare("Pull") == 0)
        pullable = true;
    } else if (type.compare("Model") == 0) {
      model_str = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
    } else if (type.compare("MeshId") == 0) {
      mesh_id = cu::Parse<size_t>(
          cu::ParseValue(script_buffer_, script_cursor_, variable_map_));
    } else if (type.compare("ActorType") == 0) {
      auto val = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
      if (val.compare("kDynamic") == 0) {
        actor_type = lib_physics::Actor::kDynamic;
        ccd = true;
      } else if (val.compare("kStatic") == 0)
        actor_type = lib_physics::Actor::kStatic;
      else if (val.compare("kKinematic") == 0)
        actor_type = lib_physics::Actor::kKinematic;
      else if (val.compare("kPlane") == 0)
        actor_type = lib_physics::Actor::kPlane;
    } else if (type.compare("Density") == 0) {
      density = cu::Parse<float>(
          cu::ParseValue(script_buffer_, script_cursor_, variable_map_));
    } else if (type.compare("DynamicFriction") == 0) {
      dyn_friction = cu::Parse<float>(
          cu::ParseValue(script_buffer_, script_cursor_, variable_map_));
    } else if (type.compare("StaticFriction") == 0) {
      stat_friction = cu::Parse<float>(
          cu::ParseValue(script_buffer_, script_cursor_, variable_map_));
    } else if (type.compare("Restitution") == 0) {
      restitution = cu::Parse<float>(
          cu::ParseValue(script_buffer_, script_cursor_, variable_map_));
    }
    type = cu::ParseType(script_buffer_, script_cursor_);
  }

  if (!model_str.empty()) {
    auto mesh_system = engine_->GetMesh();
    auto model = mesh_system->GetModel(model_str);
    if (model && mesh_id < model->meshes.size()) {
      transform.position_[0] *= mod_;
      transform.position_ += offset_;
      transform.rotation_[1] *= mod_;
      transform.rotation_[2] *= mod_;
      transform = lib_graphics::Transform(
          transform.position_, transform.rotation_ * 180 / PI, transform.scale_,
          transform.orbit_offset_, transform.orbit_rotation_ * 180 / PI);

      auto ent = CreateScopedEntity();
      g_ent_mgr.AddComponent(ent, transform);

      mesh.mesh = model->meshes[mesh_id];
      mesh.material = material_id;
      g_ent_mgr.AddComponent(ent, mesh);

      if (add_light) {
        light.data_pos = transform.position_;
        g_ent_mgr.AddComponent(ent, light);
      }

      auto actor = lib_physics::Actor(model->meshes[mesh_id], actor_type, ccd);
      actor.density = density;
      actor.static_friction = stat_friction;
      actor.dynamic_friction = dyn_friction;
      actor.restitution = restitution;
      g_ent_mgr.AddComponent(ent, actor);

      if (pickupable)
        g_ent_mgr.AddComponent(ent,
                               lib_graphics::FpsCameraSystem::Pickupable());
      else if (pullable)
        g_ent_mgr.AddComponent(ent, lib_graphics::FpsCameraSystem::Pullable());

      if (!name_id.empty()) actor_map_[name_id] = ent;
    }
  }
}

void ScriptedSystem::ParseActorJoint() {
  lib_core::Entity actor_1, actor_2;
  lib_physics::Joint joint;
  ct::string type = cu::ParseType(script_buffer_, script_cursor_);
  while (!type.empty()) {
    if (type.compare("Joint") == 0) {
      joint = lib_physics::Joint::Parse(script_buffer_, script_cursor_,
                                        variable_map_);
    } else if (type.compare("Actor1") == 0) {
      auto val = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
      auto it = actor_map_.find(val);
      if (it != actor_map_.end()) actor_1 = it->second;
    } else if (type.compare("Actor2") == 0) {
      auto val = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
      auto it = actor_map_.find(val);
      if (it != actor_map_.end()) actor_2 = it->second;
    }
    type = cu::ParseType(script_buffer_, script_cursor_);
  }

  joint.actor_1 = actor_1;
  joint.actor_2 = actor_2;
  g_ent_mgr.AddComponent(CreateScopedEntity(), joint);
}

void ScriptedSystem::ParseParticleEmitter() {
  lib_core::Entity attach_point;
  size_t particle_tex_id = lib_core::EngineCore::stock_texture;
  lib_graphics::ParticleEmitter emitter(100, particle_tex_id);
  lib_graphics::Transform transform;
  bool transform_set = false;
  ct::string type = cu::ParseType(script_buffer_, script_cursor_);
  while (!type.empty()) {
    if (type.compare("ParticleTexture") == 0) {
      auto val = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
      particle_tex_id = engine_->GetMaterial()->AddTexture2D(val);
    } else if (type.compare("ParticleDesc") == 0) {
      emitter = lib_graphics::ParticleEmitter::Parse(
          script_buffer_, script_cursor_, variable_map_);
    } else if (type.compare("Transform") == 0) {
      transform = lib_graphics::Transform::Parse(script_buffer_, script_cursor_,
                                                 variable_map_);
      transform_set = true;
    } else if (type.compare("AttachPoint") == 0) {
      auto val = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
      auto it = actor_map_.find(val);
      if (it != actor_map_.end()) attach_point = it->second;
    }

    type = cu::ParseType(script_buffer_, script_cursor_);
  }

  if (attach_point == lib_core::Entity()) attach_point = CreateScopedEntity();
  emitter.particle_texture = particle_tex_id;
  g_ent_mgr.AddComponent(attach_point, emitter);
  if (transform_set) {
    transform.position_[0] *= mod_;
    transform.position_ += offset_;
    transform.rotation_[1] *= mod_;
    transform.rotation_[2] *= mod_;
    transform = lib_graphics::Transform(
        transform.position_, transform.rotation_ * 180 / PI, transform.scale_,
        transform.orbit_offset_, transform.orbit_rotation_ * 180 / PI);
    g_ent_mgr.AddComponent(attach_point, transform);
  }
}

size_t ScriptedSystem::MaterialId() {
  size_t material_id = lib_core::EngineCore::stock_material_untextured;
  auto val_str = cu::ParseValue(script_buffer_, script_cursor_, variable_map_);
  auto it = material_map_.find(std::hash<ct::string>{}(val_str));
  if (it != material_map_.end()) material_id = it->second;
  return material_id;
}
}  // namespace lib_core
