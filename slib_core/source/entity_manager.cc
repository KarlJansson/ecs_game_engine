#include "entity_manager.h"
#include "system_manager.h"

namespace lib_core {
EntityManager::EntityManager() { add_entity_queue_.push(Entity(0)); }

bool EntityManager::FullyLoaded() {
  return comp_add_funcs_.empty() && comp_remove_funcs_.empty() &&
         remove_entity_queue_.empty();
}

Entity EntityManager::CreateEntity() {
  auto id = ++entity_id_;
  add_entity_queue_.push(Entity(id));
  return Entity(id);
}

size_t EntityManager::CreateScene() {
  auto id = ++scene_id_;
  add_scene_queue_.push(id);
  return id;
}

void EntityManager::LogicUpdate() {
  // Sync up with render thread
  sync_point_.Wait();

  std::swap(old_, new_);
  SyncEntities();
  g_sys_mgr.SyncSystems();

  // Sync up with render thread
  sync_point_.Wait();

  if (!first_update_) {
    elapsed_ = std::chrono::high_resolution_clock::now() - start_point_;
    g_sys_mgr.SyncInputSystems(elapsed_.count());
  } else {
    g_sys_mgr.SyncInputSystems(0.f);
    first_update_ = false;
  }
  start_point_ = std::chrono::high_resolution_clock::now();

  // Sync up with update thread
  sync_point_.Wait();
}

void EntityManager::DrawUpdate() {
  if (first_sync_) {
    sync_point_.Wait();
    first_sync_ = false;
  }

  // Sync up with update thread
  sync_point_.Wait();

  // Sync up with update thread
  sync_point_.Wait();
}

void EntityManager::FrameFinished() {
  // Sync up with update thread
  sync_point_.Wait();
}

void EntityManager::ResetSync() {
  first_sync_ = true;
  first_update_ = true;
}

void EntityManager::SyncEntities() {
  std::pair<size_t, Entity> tmp_remove_component;
  std::pair<size_t, size_t> tmp_remove_callback;
  CompCallbackFunction tmp_callback;
  ComponentFunction tmp_func;

  while (comp_add_callback_.try_pop(tmp_callback))
    comp_add_callbacks_[tmp_callback.hash][tmp_callback.id] = tmp_callback;

  while (comp_remove_callback_.try_pop(tmp_callback))
    comp_remove_callbacks_[tmp_callback.hash][tmp_callback.id] = tmp_callback;

  while (comp_unregister_remove_callback_.try_pop(tmp_remove_callback))
    comp_remove_callbacks_[tmp_remove_callback.first].erase(
        tmp_remove_callback.second);

  while (comp_unregister_add_callback_.try_pop(tmp_remove_callback))
    comp_add_callbacks_[tmp_remove_callback.first].erase(
        tmp_remove_callback.second);

  Entity tmp_entity;
  while (add_entity_queue_.try_pop(tmp_entity))
    entity_comps_[tmp_entity] = ct::hash_map<size_t, size_t>();

  int max = 0;
  const int max_ops = 100;
  while (remove_entity_queue_.try_pop(tmp_entity)) {
    auto it = entity_comps_.find(tmp_entity);
    if (it != entity_comps_.end()) {
      auto comp_map = it->second;
      for (auto& p : comp_map) {
        auto comp_loc = it->second.find(p.first);
        if (comp_loc == it->second.end()) continue;

        auto func_it = rem_comp_funcs_.find(p.first);
        if (func_it != rem_comp_funcs_.end()) {
          func_it->second.func(
              p.first, tmp_entity, components_[0][p.first],
              components_[1][p.first], update_vecs_[0][p.first],
              update_vecs_[1][p.first], entity_vecs_[p.first], entity_comps_);

          auto callback_it = comp_remove_callbacks_.find(p.first);
          if (callback_it != comp_remove_callbacks_.end())
            for (auto& fp : callback_it->second) fp.second.func(tmp_entity);
        }
      }

      entity_comps_.erase(tmp_entity);
    }

    if (max > max_ops) break;
    ++max;
  }

  tmp_entity = lib_core::Entity();
  while (comp_remove_funcs_.try_pop(tmp_remove_component)) {
    auto ent_it = entity_comps_.find(tmp_remove_component.second);
    if (ent_it == entity_comps_.end()) continue;

    auto comp_loc = ent_it->second.find(tmp_remove_component.first);
    if (comp_loc == ent_it->second.end()) continue;

    auto func_it = rem_comp_funcs_.find(tmp_remove_component.first);
    func_it->second.func(
        tmp_remove_component.first, tmp_remove_component.second,
        components_[0][tmp_remove_component.first],
        components_[1][tmp_remove_component.first],
        update_vecs_[0][tmp_remove_component.first],
        update_vecs_[1][tmp_remove_component.first],
        entity_vecs_[tmp_remove_component.first], entity_comps_);

    auto it = comp_remove_callbacks_.find(tmp_remove_component.first);
    if (it != comp_remove_callbacks_.end())
      for (auto& p : it->second) p.second.func(tmp_remove_component.second);

    if (max > max_ops && tmp_remove_component.second != tmp_entity) break;
    ++max;

    tmp_entity = tmp_remove_component.second;
  }

  if (remove_entity_queue_.empty() && comp_remove_funcs_.empty()) {
    tmp_entity = lib_core::Entity();
    while (comp_add_funcs_.try_pop(tmp_func)) {
      auto it_ec = entity_comps_.find(tmp_func.entity);
      if (it_ec == entity_comps_.end()) continue;

      tmp_func.func(
          components_[0][tmp_func.hash], components_[1][tmp_func.hash],
          update_vecs_[0][tmp_func.hash], update_vecs_[1][tmp_func.hash],
          entity_vecs_[tmp_func.hash], it_ec->second);

      auto it = comp_add_callbacks_.find(tmp_func.hash);
      if (it != comp_add_callbacks_.end())
        for (auto& p : it->second) p.second.func(tmp_func.entity);

      if (max > max_ops && tmp_entity != tmp_func.entity) break;
      ++max;

      tmp_entity = tmp_func.entity;
    }
  }
}

void EntityManager::SyncScenes() {}

short EntityManager::GetBufferIndex(BufferId id) {
  switch (id) {
    case kNew:
      return new_;
    case kOld:
      return old_;
    default:
      return -1;
  }
  return -1;
}

short EntityManager::GetNewIndex() { return new_; }

short EntityManager::GetOldIndex() { return old_; }
}  // namespace lib_core
