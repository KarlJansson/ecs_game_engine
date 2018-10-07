#pragma once
#include <atomic>
#include <utility>
#include "any_type.hpp"
#include "barrier.hpp"
#include "core_utilities.h"
#include "entity.h"

namespace lib_core {
class EntityManager {
 private:
  EntityManager() = default;

  struct ComponentFunction {
    size_t hash;
    Entity entity;
    std::function<void(any_type&, any_type&, ct::dyn_array<uint8_t>&,
                       ct::dyn_array<uint8_t>&, ct::dyn_array<Entity>&,
                       ct::hash_map<size_t, size_t>&)>
        func;
  };

  struct RemoveComponentFunction {
    std::function<void(size_t, Entity, any_type&, any_type&,
                       ct::dyn_array<uint8_t>&, ct::dyn_array<uint8_t>&,
                       ct::dyn_array<Entity>&,
                       ct::hash_map<Entity, ct::hash_map<size_t, size_t>>&)>
        func;
  };

  struct CompCallbackFunction {
    size_t id;
    size_t hash;
    std::function<void(lib_core::Entity)> func;
  };

 public:
  EntityManager(const EntityManager&) = delete;
  EntityManager(const EntityManager&&) = delete;
  EntityManager& operator=(const EntityManager) = delete;

  static EntityManager& get() {
    static EntityManager instance_;
    return instance_;
  }

  Entity CreateEntity();
  size_t CreateScene();

  template <typename T>
  void AddComponent(Entity entity, T comp) {
    auto hash = typeid(T).hash_code();

    auto add_func = [hash, entity, comp](
                        any_type& new_comps, any_type& old_comps,
                        ct::dyn_array<uint8_t>& new_update,
                        ct::dyn_array<uint8_t>& old_update,
                        ct::dyn_array<Entity>& e_vec,
                        ct::hash_map<size_t, size_t>& e_comps) {
      if (!e_vec.empty()) {
        auto loc_it = e_comps.find(hash);
        if (loc_it != e_comps.end()) {
          new_comps.get_value<ct::dyn_array<T>>()[loc_it->second] = comp;
          old_comps.get_value<ct::dyn_array<T>>()[loc_it->second] = comp;
          new_update[loc_it->second] = old_update[loc_it->second] = true;
        } else {
          e_comps[hash] = new_comps.get_value<ct::dyn_array<T>>().size();
          new_comps.get_value<ct::dyn_array<T>>().push_back(comp);
          old_comps.get_value<ct::dyn_array<T>>().push_back(comp);
          e_vec.push_back(entity);
          new_update.push_back(true), old_update.push_back(true);
        }
        return;
      }

      new_comps = any_type(ct::dyn_array<T>({comp}));
      old_comps = any_type(ct::dyn_array<T>({comp}));
      e_vec.push_back(entity);
      new_update.push_back(true), old_update.push_back(true);
      e_comps[hash] = 0;
    };
    comp_add_funcs_.push({hash, entity, add_func});

    auto it = rem_comp_funcs_.find(hash);
    if (it == rem_comp_funcs_.end()) {
      auto remove_func =
          [](size_t hash, Entity entity, any_type& new_comps,
             any_type& old_comps, ct::dyn_array<uint8_t>& new_update,
             ct::dyn_array<uint8_t>& old_update, ct::dyn_array<Entity>& e_vec,
             ct::hash_map<Entity, ct::hash_map<size_t, size_t>>& e_comps) {
            auto it = e_comps.find(entity);
            if (it != e_comps.end()) {
              auto loc_it = it->second.find(hash);
              if (loc_it != it->second.end()) {
                auto& new_comp_vec = new_comps.get_value<ct::dyn_array<T>>();
                auto& old_comp_vec = old_comps.get_value<ct::dyn_array<T>>();
                if (loc_it->second != new_comp_vec.size() - 1 ||
                    new_comp_vec.size() != 1) {
                  auto& move_ent = e_vec.back();
                  e_comps[move_ent][hash] = loc_it->second;
                  e_vec[loc_it->second] = move_ent;
                  new_comp_vec[loc_it->second] = std::move(new_comp_vec.back());
                  old_comp_vec[loc_it->second] = std::move(old_comp_vec.back());

                  new_update[loc_it->second] = new_update.back();
                  old_update[loc_it->second] = old_update.back();
                }
                new_comp_vec.pop_back(), old_comp_vec.pop_back();
                new_update.pop_back(), old_update.pop_back();

                e_vec.pop_back();

                it->second.erase(hash);
              }
            }
          };

      rem_comp_funcs_[hash] = {remove_func};
    }
  }

  template <typename T>
  void RemoveComponent(Entity entity) {
    auto hash = typeid(T).hash_code();
    comp_remove_funcs_.push({hash, entity});
  }

  void RemoveEntity(Entity entity) { remove_entity_queue_.push(entity); }

  enum BufferId { kNew, kOld };

  template <typename T>
  ct::dyn_array<T>* const GetNewCbt() {
    return GetComponentsByType<T>(kNew);
  }

  template <typename T>
  ct::dyn_array<T>* const GetOldCbt() {
    return GetComponentsByType<T>(kOld);
  }

  template <typename T>
  ct::dyn_array<Entity> const* const GetEbt() const {
    return GetEntitiesByType<T>();
  }

  template <typename T>
  ct::dyn_array<uint8_t>* const GetNewUbt() {
    return GetUpdateByType<T>(kNew);
  }

  template <typename T>
  ct::dyn_array<uint8_t>* const GetOldUbt() {
    return GetUpdateByType<T>(kOld);
  }

  template <typename T>
  T* const GetNewCbeW(Entity entity) {
    return GetComponentByEntity<T>(entity, kNew, true);
  }

  template <typename T>
  T* const GetOldCbeW(Entity entity) {
    return GetComponentByEntity<T>(entity, kOld, true);
  }

  template <typename T>
  T const* const GetNewCbeR(Entity entity) {
    return GetComponentByEntity<T>(entity, kNew, false);
  }

  template <typename T>
  T const* const GetOldCbeR(Entity entity) {
    return GetComponentByEntity<T>(entity, kOld, false);
  }

  template <typename T>
  void MarkForUpdate(Entity entity) {
    auto pos = GetPbe<T>(entity);
    if (pos == -1) return;

    auto hash = typeid(T).hash_code();
    update_vecs_[0][hash][pos] = update_vecs_[1][hash][pos] = true;
  }

  template <typename T>
  int GetPbe(Entity entity) {
    return GetPositionByEntity<T>(entity);
  }

  void LogicUpdate();
  void DrawUpdate();
  void FrameFinished();
  void ResetSync();

  short GetNewIndex();
  short GetOldIndex();

  template <typename T>
  size_t RegisterAddComponentCallback(
      std::function<void(lib_core::Entity)> callback) {
    auto hash = typeid(T).hash_code();
    auto id = add_callback_id_++;
    comp_add_callback_.push({id, hash, std::move(callback)});
    return id;
  }

  template <typename T>
  void UnregisterAddComponentCallback(size_t id) {
    auto hash = typeid(T).hash_code();
    comp_unregister_add_callback_.push({hash, id});
  }

  template <typename T>
  size_t RegisterRemoveComponentCallback(
      std::function<void(lib_core::Entity)> callback) {
    auto hash = typeid(T).hash_code();
    auto id = remove_callback_id_++;
    comp_remove_callback_.push({id, hash, std::move(callback)});
    return id;
  }

  template <typename T>
  void UnregisterRemoveComponentCallback(size_t id) {
    auto hash = typeid(T).hash_code();
    comp_unregister_remove_callback_.push({hash, id});
  }

  bool FullyLoaded();

 private:
  template <typename T>
  T* const GetComponentByEntity(Entity entity, BufferId id,
                                bool write = false) {
    auto hash = typeid(T).hash_code();
    short buffer = GetBufferIndex(id);

    auto it = entity_comps_.find(entity);
    if (it == entity_comps_.end()) return nullptr;

    auto it_ent = it->second.find(hash);
    if (it_ent != it->second.end()) {
      if (write) MarkForUpdate<T>(entity);
      return static_cast<T*>(
          &components_[buffer][hash]
               .get_value<ct::dyn_array<T>>()[it_ent->second]);
    }

    return nullptr;
  }

  template <typename T>
  ct::dyn_array<T>* const GetComponentsByType(BufferId id) {
    auto hash = typeid(T).hash_code();
    short buffer = GetBufferIndex(id);

    auto it = components_[buffer].find(hash);
    if (it == components_[buffer].end()) return nullptr;

    auto& vec = it->second.get_value<ct::dyn_array<T>>();
    if (vec.empty()) return nullptr;
    return &vec;
  }

  template <typename T>
  ct::dyn_array<Entity> const* const GetEntitiesByType() const {
    auto hash = typeid(T).hash_code();

    auto it = entity_vecs_.find(hash);
    if (it == entity_vecs_.end()) return nullptr;

    if (it->second.empty()) return nullptr;
    return &it->second;
  }

  template <typename T>
  ct::dyn_array<uint8_t>* const GetUpdateByType(BufferId id) {
    auto hash = typeid(T).hash_code();
    short buffer = GetBufferIndex(id);

    auto it = update_vecs_[buffer].find(hash);
    if (it == update_vecs_[buffer].end()) return nullptr;

    return &it->second;
  }

  template <typename T>
  int GetPositionByEntity(Entity entity) {
    auto hash = typeid(T).hash_code();

    auto it = entity_comps_.find(entity);
    if (it == entity_comps_.end()) return -1;

    auto it_ent = it->second.find(hash);
    if (it_ent != it->second.end()) return int(it_ent->second);

    return -1;
  }

  void SyncEntities();
  void SyncScenes();

  short GetBufferIndex(BufferId id);

  struct Scene {
    ct::hash_map<size_t, any_type> components_[2];
    ct::hash_map<size_t, ct::dyn_array<uint8_t>> update_vecs_[2];
    ct::hash_map<size_t, ct::dyn_array<Entity>> entity_vecs_;
    ct::hash_map<Entity, ct::hash_map<size_t, size_t>> entity_comps_;
  };

  ct::hash_map<size_t, Scene> scenes_;

  short old_ = 0, new_ = 1;

  tbb::concurrent_unordered_map<size_t, RemoveComponentFunction>
      rem_comp_funcs_;
  tbb::concurrent_queue<std::pair<size_t, Entity>> comp_remove_funcs_;
  tbb::concurrent_queue<ComponentFunction> comp_add_funcs_;

  tbb::concurrent_queue<std::pair<size_t, size_t>>
      comp_unregister_remove_callback_;
  tbb::concurrent_queue<std::pair<size_t, size_t>>
      comp_unregister_add_callback_;

  tbb::concurrent_queue<Entity> add_entity_queue_;
  tbb::concurrent_queue<Entity> remove_entity_queue_;

  tbb::concurrent_queue<size_t> add_scene_queue_;
  tbb::concurrent_queue<size_t> remove_scene_queue_;

  tbb::concurrent_queue<CompCallbackFunction> comp_remove_callback_;
  tbb::concurrent_queue<CompCallbackFunction> comp_add_callback_;

  std::atomic<size_t> add_callback_id_ = {0}, remove_callback_id_ = {0};
  ct::hash_map<size_t, ct::tree_map<size_t, CompCallbackFunction>>
      comp_remove_callbacks_;
  ct::hash_map<size_t, ct::tree_map<size_t, CompCallbackFunction>>
      comp_add_callbacks_;

  std::atomic<size_t> entity_id_ = {0}, scene_id_ = {0};
  ct::hash_map<size_t, any_type> components_[2];
  ct::hash_map<size_t, ct::dyn_array<uint8_t>> update_vecs_[2];
  ct::hash_map<size_t, ct::dyn_array<Entity>> entity_vecs_;
  ct::hash_map<Entity, ct::hash_map<size_t, size_t>> entity_comps_;

  Barrier sync_point_ = Barrier(2);
  bool first_sync_ = true;
  bool first_update_ = true;

  std::chrono::duration<float> elapsed_;
  std::chrono::time_point<std::chrono::high_resolution_clock> start_point_;
};
}  // namespace lib_core

static auto& g_ent_mgr = lib_core::EntityManager::get();
