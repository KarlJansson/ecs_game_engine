#include "culling_system.h"
#include "actor.h"
#include "engine_core.h"
#include "entity_manager.h"
#include "gl_camera_system.h"
#include "graphics_commands.h"
#include "gui_text.h"
#include "light.h"
#include "light_system.h"
#include "mesh.h"
#include "system_manager.h"
#include "transform.h"

namespace lib_graphics {
CullingSystem::CullingSystem(class lib_core::EngineCore *engine)
    : engine_(engine) {
  mesh_octree_ = std::make_unique<OcTree>();
  light_octree_ = std::make_unique<OcTree>();

  add_mesh_callback_id =
      g_ent_mgr.RegisterAddComponentCallback<lib_graphics::Mesh>(
          [&](lib_core::Entity entity) { add_mesh_vec_.push_back(entity); });
  add_light_callback_id_ =
      g_ent_mgr.RegisterAddComponentCallback<lib_graphics::Light>(
          [&](lib_core::Entity entity) { add_light_vec_.push_back(entity); });

  rem_mesh_callback_id =
      g_ent_mgr.RegisterRemoveComponentCallback<lib_graphics::Mesh>(
          [&](lib_core::Entity entity) {
            g_ent_mgr.RemoveComponent<MeshOctreeFlag>(entity);
            mesh_octree_->RemoveEntity(entity);
          });
  rem_light_callback_id_ =
      g_ent_mgr.RegisterRemoveComponentCallback<lib_graphics::Light>(
          [&](lib_core::Entity entity) {
            g_ent_mgr.RemoveComponent<LightOctreeFlag>(entity);
            light_octree_->RemoveEntity(entity);
            draw_entities_.erase(entity);
            light_matrices_.erase(entity);
            light_packs_.erase(entity);
          });
}

CullingSystem::~CullingSystem() {
  g_ent_mgr.UnregisterRemoveComponentCallback<lib_graphics::Mesh>(
      rem_mesh_callback_id);
  g_ent_mgr.UnregisterRemoveComponentCallback<lib_graphics::Light>(
      rem_light_callback_id_);

  g_ent_mgr.UnregisterAddComponentCallback<lib_graphics::Mesh>(
      add_mesh_callback_id);
  g_ent_mgr.UnregisterAddComponentCallback<lib_graphics::Light>(
      add_light_callback_id_);
}

void CullingSystem::LogicUpdate(float dt) {}

void CullingSystem::DrawUpdate(lib_graphics::Renderer *renderer,
                               lib_gui::TextSystem *text_renderer) {
  auto culling_timer = cu::TimerStart();
  auto cam = g_ent_mgr.GetOldCbt<Camera>();
  auto cam_ents = g_ent_mgr.GetEbt<Camera>();

  auto add_mesh_aabb_command = g_sys_mgr.GetCommands<AddMeshAabbCommand>();
  if (add_mesh_aabb_command && !add_mesh_aabb_command->empty()) {
    for (auto &c : *add_mesh_aabb_command)
      mesh_aabb_[c.mesh_id] = std::move(c.aabb);
    add_mesh_aabb_command->clear();
  }

  auto remove_mesh_aabb_command =
      g_sys_mgr.GetCommands<RemoveMeshAabbCommand>();
  if (remove_mesh_aabb_command && !remove_mesh_aabb_command->empty()) {
    for (auto &c : *remove_mesh_aabb_command) mesh_aabb_.erase(c.mesh_id);
    add_mesh_aabb_command->clear();
  }

  UpdateSearchTrees();
  if (cam) {
    tbb::parallel_invoke(
        [&]() {
          tbb::parallel_for(0, int(cam->size()), 1, [&](int id) {
            AabbMeshCheck(cam->at(id).planes_, cam_ents->at(id));
          });
        },
        [&]() {
          tbb::parallel_for(0, int(cam->size()), 1, [&](int id) {
            AabbLightCheck(cam->at(id).planes_, cam_ents->at(id));
          });
        });
  }

  shadow_meshes_ = 0;
  for (auto &light_p : light_packs_) {
    light_matrices_[light_p.first].clear();

    for (auto light_ent : light_p.second) {
      auto light = g_ent_mgr.GetOldCbeR<Light>(light_ent);

      if (light) {
        if (light->cast_shadows) {
          size_t shadow_max = 1024;
          if (light->type == Light::kDir) {
            auto light_r = g_ent_mgr.GetOldCbeW<Light>(light_ent);
            auto shadow_matrices = LightSystem::GetShadowMatrices(*light_r);
            Camera::FrustumPlanes planes;
            for (int ii = 0; ii < shadow_matrices.size(); ++ii) {
              auto mat_ptr = shadow_matrices[ii].data;
              for (int i = 0; i < 3; ++i) {
                GlCameraSystem::ExtractPlane(planes.planes[2 * i], mat_ptr,
                                             i + 1);
                GlCameraSystem::ExtractPlane(planes.planes[2 * i + 1], mat_ptr,
                                             -(i + 1));
              }

              shadow_max =
                  AabbMeshCheck(planes, light_ent, ii == 0 ? true : false);
            }
          } else if (light->type == Light::kPoint) {
            auto bvol = BoundingVolume(light->data_pos, light->max_radius);
            draw_entities_[light_ent].clear();
            mesh_octree_->SearchBox(bvol, draw_entities_[light_ent]);
            shadow_max = draw_entities_[light_ent].size();
          }
          shadow_meshes_ += shadow_max;
        }

        if (light->type != Light::kDir) {
          lib_core::Matrix4x4 light_world;
          light_world.Identity();
          light_world.Translate(light->data_pos);
          light_world.Scale(lib_core::Vector3(
              light->max_radius, light->max_radius, light->max_radius));
          light_matrices_[light_p.first].emplace_back(std::move(light_world));
        } else {
          auto light_r = g_ent_mgr.GetOldCbeW<Light>(light_ent);
          auto mat = LightSystem::GetShadowMatrices(*light_r);
          for (auto &m : mat) light_matrices_[light_p.first].push_back(m);
        }
      }
    }
  }

  float dist_from_camera;
  opeque_meshes_.clear();
  translucent_meshes_.clear();
  for (auto &draw_ents : draw_entities_) {
    opeque_mesh_packs_out_[draw_ents.first].clear();
    translucent_mesh_packs_out_[draw_ents.first].clear();
    auto camera = g_ent_mgr.GetOldCbeR<lib_graphics::Camera>(draw_ents.first);
    auto light = g_ent_mgr.GetOldCbeR<Light>(draw_ents.first);

    opeque_mesh_packs_.clear();
    translucent_mesh_packs_.clear();

    for (auto e : draw_ents.second) {
      auto mesh = g_ent_mgr.GetOldCbeR<Mesh>(e);
      auto transform = g_ent_mgr.GetOldCbeR<Transform>(e);

      if (mesh) {
        if (light && mesh->translucency < 1.f) continue;

        if (camera) {
          if (transform)
            dist_from_camera =
                (transform->Position() - camera->position_).Length();
          else
            dist_from_camera =
                (lib_core::Vector3(0.f) - camera->position_).Length();
        } else if (light) {
          if (transform)
            dist_from_camera =
                (transform->Position() - light->data_pos).Length();
          else
            dist_from_camera =
                (lib_core::Vector3(0.f) - light->data_pos).Length();
        }

        auto material = mesh->material;
        auto &mesh_pack = mesh->translucency < 1.f && camera
                              ? translucent_mesh_packs_[dist_from_camera]
                                                       [{mesh->mesh, material}]
                              : opeque_mesh_packs_[{mesh->mesh, material}];

        if (mesh->translucency < 1.f && camera)
          mesh_pack.transp_vec.push_back(mesh->translucency);

        if (mesh_pack.closest_dist > dist_from_camera)
          mesh_pack.closest_dist = dist_from_camera;

        mesh_pack.albedo_vec.push_back(mesh->albedo);
        mesh_pack.rme_vec.push_back(mesh->rme);
        mesh_pack.tex_scale.push_back(mesh->texture_scale);
        mesh_pack.tex_offset.push_back(mesh->texture_offset);

        if (transform) {
          mesh_pack.world_vec.push_back(transform->world_);
          mesh_pack.world_inv_trans_vec.push_back(transform->world_);
          mesh_pack.world_inv_trans_vec.back().Inverse();
          mesh_pack.world_inv_trans_vec.back().Transpose();
        } else {
          mesh_pack.world_vec.push_back(lib_core::Matrix4x4());
          mesh_pack.world_vec.back().Identity();
          mesh_pack.world_inv_trans_vec.push_back(mesh_pack.world_vec.back());
        }
      }
    }

    auto opeque_ops = [&]() {
      sorted_opeque_packs_.clear();
      for (auto it = opeque_mesh_packs_.begin(); it != opeque_mesh_packs_.end();
           it++)
        sorted_opeque_packs_[it->second.closest_dist].emplace_back(it);

      for (auto &p : sorted_opeque_packs_) {
        for (auto &it : p.second) {
          MeshPack pack;
          pack.material_id = it->first.second;
          pack.mesh_id = it->first.first;
          pack.mesh_count = int(it->second.world_vec.size());
          pack.start_ind = int(opeque_meshes_.world_vec.size());
          opeque_mesh_packs_out_[draw_ents.first].push_back(pack);
          opeque_meshes_.world_vec.insert(std::end(opeque_meshes_.world_vec),
                                          std::begin(it->second.world_vec),
                                          std::end(it->second.world_vec));

          opeque_meshes_.world_inv_trans_vec.insert(
              std::end(opeque_meshes_.world_inv_trans_vec),
              std::begin(it->second.world_inv_trans_vec),
              std::end(it->second.world_inv_trans_vec));

          opeque_meshes_.albedo_vec.insert(std::end(opeque_meshes_.albedo_vec),
                                           std::begin(it->second.albedo_vec),
                                           std::end(it->second.albedo_vec));

          opeque_meshes_.rme_vec.insert(std::end(opeque_meshes_.rme_vec),
                                        std::begin(it->second.rme_vec),
                                        std::end(it->second.rme_vec));

          opeque_meshes_.tex_scale.insert(std::end(opeque_meshes_.tex_scale),
                                          std::begin(it->second.tex_scale),
                                          std::end(it->second.tex_scale));

          opeque_meshes_.tex_offset.insert(std::end(opeque_meshes_.tex_offset),
                                           std::begin(it->second.tex_offset),
                                           std::end(it->second.tex_offset));
        }
      }
    };

    auto translucent_ops = [&]() {
      for (auto &p : translucent_mesh_packs_) {
        for (auto &tp : p.second) {
          MeshPack pack;
          pack.material_id = tp.first.second;
          pack.mesh_id = tp.first.first;
          pack.mesh_count = int(tp.second.world_vec.size());
          pack.start_ind = int(translucent_meshes_.world_vec.size());
          translucent_mesh_packs_out_[draw_ents.first].push_back(pack);
          translucent_meshes_.world_vec.insert(
              std::end(translucent_meshes_.world_vec),
              std::begin(tp.second.world_vec), std::end(tp.second.world_vec));

          translucent_meshes_.world_inv_trans_vec.insert(
              std::end(translucent_meshes_.world_inv_trans_vec),
              std::begin(tp.second.world_inv_trans_vec),
              std::end(tp.second.world_inv_trans_vec));

          translucent_meshes_.albedo_vec.insert(
              std::end(translucent_meshes_.albedo_vec),
              std::begin(tp.second.albedo_vec), std::end(tp.second.albedo_vec));

          translucent_meshes_.rme_vec.insert(
              std::end(translucent_meshes_.rme_vec),
              std::begin(tp.second.rme_vec), std::end(tp.second.rme_vec));

          translucent_meshes_.tex_scale.insert(
              std::end(translucent_meshes_.tex_scale),
              std::begin(tp.second.tex_scale), std::end(tp.second.tex_scale));

          translucent_meshes_.tex_offset.insert(
              std::end(translucent_meshes_.tex_offset),
              std::begin(tp.second.tex_offset), std::end(tp.second.tex_offset));

          translucent_meshes_.transp_vec.insert(
              std::end(translucent_meshes_.transp_vec),
              std::begin(tp.second.transp_vec), std::end(tp.second.transp_vec));
        }
      }
    };
    tbb::parallel_invoke(opeque_ops, translucent_ops);
  }

  mesh_count_ = 0, light_count_ = 0;
  mesh_count_ +=
      opeque_meshes_.world_vec.size() + translucent_meshes_.world_vec.size();
  for (auto &s : light_matrices_) light_count_ += s.second.size();

  auto dbg_out = engine_->GetDebugOutput();
  dbg_out->UpdateBottomLeftLine(
      0, "Rendered meshes: " + std::to_string(mesh_count_ - shadow_meshes_));
  dbg_out->UpdateBottomLeftLine(
      1, "Rendered shadow meshes: " + std::to_string(shadow_meshes_));
  dbg_out->UpdateBottomLeftLine(
      2, "Rendered lights: " + std::to_string(light_count_));
  dbg_out->UpdateBottomLeftLine(
      3, "Mesh octree nodes: " + std::to_string(mesh_octree_->GetNrNodes()));
  dbg_out->UpdateBottomLeftLine(
      4, "Light octree nodes: " + std::to_string(light_octree_->GetNrNodes()));
  dbg_out->UpdateBottomRightLine(
      1, std::to_string(cu::TimerStop<std::milli>(culling_timer)) +
             " :Culling time");
}

const ct::dyn_array<CullingSystem::MeshPack> *CullingSystem::GetMeshPacks(
    lib_core::Entity entity, bool opeque) {
  if (opeque) return &opeque_mesh_packs_out_[entity];
  return &translucent_mesh_packs_out_[entity];
}

const ct::dyn_array<lib_core::Entity> *CullingSystem::GetLightPacks(
    lib_core::Entity entity) {
  return &light_packs_[entity];
}

ct::dyn_array<lib_core::Vector3> &CullingSystem::GetAlbedoVecs(bool opeque) {
  if (opeque) return opeque_meshes_.albedo_vec;
  return translucent_meshes_.albedo_vec;
}

ct::dyn_array<lib_core::Vector3> &CullingSystem::GetRmeVecs(bool opeque) {
  if (opeque) return opeque_meshes_.rme_vec;
  return translucent_meshes_.rme_vec;
}

ct::dyn_array<lib_core::Vector2> &CullingSystem::GetTexScaleVecs(bool opeque) {
  if (opeque) return opeque_meshes_.tex_scale;
  return translucent_meshes_.tex_scale;
}

ct::dyn_array<lib_core::Vector2> &CullingSystem::GetTexOffsetVecs(bool opeque) {
  if (opeque) return opeque_meshes_.tex_offset;
  return translucent_meshes_.tex_offset;
}

ct::dyn_array<lib_core::Matrix4x4> &CullingSystem::GetWorldMatrices(
    bool opeque) {
  if (opeque) return opeque_meshes_.world_vec;
  return translucent_meshes_.world_vec;
}

ct::dyn_array<lib_core::Matrix4x4> &CullingSystem::GetWorldInvTransMatrices(
    bool opeque) {
  if (opeque) return opeque_meshes_.world_inv_trans_vec;
  return translucent_meshes_.world_inv_trans_vec;
}

ct::dyn_array<float> &CullingSystem::GetTransparencyVecs() {
  return translucent_meshes_.transp_vec;
}

ct::dyn_array<lib_core::Matrix4x4> &CullingSystem::GetLightMatrices(
    lib_core::Entity ent) {
  return light_matrices_[ent];
}

size_t CullingSystem::AabbMeshCheck(const Camera::FrustumPlanes planes,
                                    lib_core::Entity target, bool clear_ents) {
  auto &ents = draw_entities_[target];
  if (clear_ents) ents.clear();
  mesh_octree_->SearchFrustum(planes, ents);
  return ents.size();
}

size_t CullingSystem::AabbLightCheck(const Camera::FrustumPlanes planes,
                                     lib_core::Entity target, bool clear_ents) {
  auto &ents = light_packs_[target];

  if (clear_ents) ents.clear();
  light_octree_->SearchFrustum(planes, ents);
  return ents.size();
}

size_t CullingSystem::AabbLightCheckTiled(Camera &camera,
                                          lib_core::Entity target,
                                          bool clear_ents) {
  size_t count = 0;
  const int x = 16, y = 9;
  auto f_info = Camera::CalculateFrustumInfo(camera);
  auto grid = Camera::CalculateFrustumGrid(f_info, x, y);
  ct::dyn_array<ct::dyn_array<lib_core::Entity>> grid_ents(x * y);
  auto search_thread = [&](int i) {
    light_octree_->SearchFrustum(grid[i], grid_ents[i]);
  };
  tbb::parallel_for(0, x * y, search_thread);
  return count;
}

void CullingSystem::UpdateSearchTrees() {
  auto mesh_update = [&]() {
    for (int i = int(add_mesh_vec_.size()) - 1; i >= 0; --i) {
      auto mesh = g_ent_mgr.GetOldCbeR<Mesh>(add_mesh_vec_[i]);
      if (!mesh) continue;
      auto it = mesh_aabb_.find(mesh->mesh);
      if (it == mesh_aabb_.end()) continue;

      auto aabb = it->second;
      auto trans = g_ent_mgr.GetOldCbeR<Transform>(add_mesh_vec_[i]);
      if (trans) {
        aabb.center.Transform(trans->world_);

        lib_core::Matrix3x3 rot_mat;
        trans->world_.RotationMatrix(rot_mat);
        for (int i = 0; i < 3; ++i)
          for (int ii = 0; ii < 3; ++ii)
            rot_mat.data[i * 3 + ii] = std::abs(rot_mat.data[i * 3 + ii]);

        aabb.extent.Transform(rot_mat);
      }

      g_ent_mgr.AddComponent(add_mesh_vec_[i], MeshOctreeFlag());
      mesh_octree_->AddEntity(add_mesh_vec_[i], aabb);
      add_mesh_vec_.erase(add_mesh_vec_.begin() + i);
    }

    auto mesh_ents = g_ent_mgr.GetEbt<MeshOctreeFlag>();
    if (mesh_ents) {
      auto mesh_update = g_ent_mgr.GetOldUbt<MeshOctreeFlag>();

      for (int i = 0; i < mesh_ents->size(); ++i) {
        if ((*mesh_update)[i]) {
          auto e = mesh_ents->at(i);
          auto mesh = g_ent_mgr.GetNewCbeR<Mesh>(e);
          if (mesh) {
            auto it = mesh_aabb_.find(mesh->mesh);
            if (it == mesh_aabb_.end()) continue;

            auto aabb = it->second;
            auto trans = g_ent_mgr.GetOldCbeR<Transform>(e);
            if (trans) {
              aabb.center.Transform(trans->world_);

              lib_core::Matrix3x3 rot_mat;
              trans->world_.RotationMatrix(rot_mat);
              for (int i = 0; i < 3; ++i)
                for (int ii = 0; ii < 3; ++ii)
                  rot_mat.data[i * 3 + ii] = std::abs(rot_mat.data[i * 3 + ii]);

              aabb.extent.Transform(rot_mat);
            }

            mesh_octree_->UpdateEntityPosition(e, aabb);
          }
          (*mesh_update)[i] = false;
        }
      }
    }
  };

  auto light_update = [&]() {
    BoundingVolume aabb;
    for (int i = int(add_light_vec_.size()) - 1; i >= 0; --i) {
      auto light = g_ent_mgr.GetOldCbeR<Light>(add_light_vec_[i]);
      if (!light) continue;
      if (light->type != Light::kDir) {
        aabb.center = light->data_pos;
        aabb.extent[0] = aabb.extent[1] = aabb.extent[2] = light->max_radius;

        light_octree_->AddEntity(add_light_vec_[i], aabb);
      } else {
        aabb.center.ZeroMem();
        aabb.extent[0] = aabb.extent[1] = aabb.extent[2] = 1000.f;
        light_octree_->AddEntity(add_light_vec_[i], aabb);
      }
      g_ent_mgr.AddComponent(add_light_vec_[i], LightOctreeFlag());
      add_light_vec_.erase(add_light_vec_.begin() + i);
    }

    auto light_ents = g_ent_mgr.GetEbt<LightOctreeFlag>();
    if (light_ents) {
      auto light_update = g_ent_mgr.GetOldUbt<LightOctreeFlag>();

      for (int i = 0; i < light_ents->size(); ++i) {
        if ((*light_update)[i]) {
          auto e = light_ents->at(i);
          auto l = g_ent_mgr.GetOldCbeR<Light>(e);
          if (l) {
            if (l->type != Light::kDir) {
              aabb.center = l->data_pos;
              aabb.extent[0] = aabb.extent[1] = aabb.extent[2] = l->max_radius;

              light_octree_->UpdateEntityPosition(e, aabb);
            } else {
              aabb.center.ZeroMem();
              aabb.extent[0] = aabb.extent[1] = aabb.extent[2] = 1000.f;
              light_octree_->UpdateEntityPosition(e, aabb);
            }
          }
          (*light_update)[i] = false;
        }
      }
    }
  };

  tbb::parallel_invoke(mesh_update, light_update);
}
}  // namespace lib_graphics
