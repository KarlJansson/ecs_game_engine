#include "system_manager.h"
#include "core_commands.h"
#include "entity_manager.h"

#include <execution>

namespace lib_core {
void SystemManager::DrawUpdate(lib_graphics::Renderer *renderer,
                               lib_gui::TextSystem *text_renderer) {
  for (auto &sys_vec : system_map_)
    for (auto &p : sys_vec.second)
      if (p->IsActive() && p->IsDrawn()) p->DrawUpdate(renderer, text_renderer);
}

void SystemManager::SyncSystems() {
  IssueCommandStruct command_holder;
  while (command_queue_.try_pop(command_holder))
    command_holder.command(command_lists_);

  auto add_system_commands = g_sys_mgr.GetCommands<AddSystemCommand>();
  if (add_system_commands && !add_system_commands->empty()) {
    for (auto &c : *add_system_commands) {
      if (!c.system->Initialized()) {
        c.system->InitSystem();
        c.system->initialized_ = true;
      }

      auto &prio_list = system_map_[c.priority];
      system_id_map_[c.SystemId()] = {c.priority, c.system.get()};
      prio_list.emplace_back(std::move(c.system));
    }
    add_system_commands->clear();
  }

  CleanSystems();
}

void SystemManager::SyncInputSystems(float dt) {
  for (auto &p : system_map_[4000]) p->LogicUpdate(dt);
}

void SystemManager::ClearSystems() {
  for (auto &p : system_map_)
    for (auto &s : p.second) s->FinalizeSystem();

  system_map_.clear();
  system_id_map_.clear();
}

void SystemManager::CleanSystems() {
  auto remove_system_commands = g_sys_mgr.GetCommands<RemoveSystemCommand>();
  if (remove_system_commands && !remove_system_commands->empty()) {
    for (auto &c : *remove_system_commands) {
      auto system_location = system_id_map_.find(c.system_id);
      if (system_location != system_id_map_.end()) {
        auto &vec = system_map_[system_location->second.first];
        int counter = 0;
        for (auto &s : vec) {
          if (s.get() == system_location->second.second) {
            s->FinalizeSystem();
            vec.erase(vec.begin() + counter);
            break;
          }
          ++counter;
        }

        system_id_map_.erase(system_location);
      }
    }
    remove_system_commands->clear();
  }
}

void SystemManager::LogicUpdate(float dt) {
  if (g_ent_mgr.FullyLoaded() && empty_frames_ > 5)
    for (auto &p : system_map_)
      for (auto &s : p.second)
        if (s->Initialized() && !s->Loaded()) s->fully_loaded_ = true;

  if (!g_ent_mgr.FullyLoaded() || empty_frames_ > 5) empty_frames_ = 0;
  if (g_ent_mgr.FullyLoaded()) ++empty_frames_;

  ct::dyn_array<size_t> prio_ids;
  for (auto &p : system_map_)
    if (p.first != 1000 && p.first != 2000 && p.first != 3000 &&
        p.first != 4000)
      prio_ids.push_back(p.first);

  for (auto &s : system_map_[1000]) {
    if (s->IsActive() && s->IsUpdated()) {
      s->LogicUpdate(dt);
      s->temporary_memory_.clear();
    }
  }

  auto &pre_run_sys_vec = system_map_[2000];
  if (!pre_run_sys_vec.empty()) {
    pre_run_sys_vec[0]->LogicUpdate(dt);
    pre_run_sys_vec[0]->temporary_memory_.clear();
    auto pre_run_systems = [&](auto &sys) {
      if (sys->IsActive() && sys->IsUpdated()) {
        sys->LogicUpdate(dt);
        sys->temporary_memory_.clear();
      }
    };
    if (pre_run_sys_vec.size() > 1)
      std::for_each(std::execution::par_unseq, std::begin(pre_run_sys_vec) + 1,
                    std::end(pre_run_sys_vec), pre_run_systems);
  }

  auto update_func = [&](size_t id) {
    auto &sys_vec = system_map_[id];
    for (auto &s : sys_vec) {
      if (s->Loaded()) {
        if (s->IsActive() && s->IsUpdated()) {
          s->LogicUpdate(dt);
          s->temporary_memory_.clear();
        }
      }
    }
  };
  std::for_each(std::execution::par_unseq, std::begin(prio_ids),
                std::end(prio_ids), update_func);

  auto &post_run_sys_vec = system_map_[3000];
  auto post_run_systems = [&](size_t id) {
    if (post_run_sys_vec[id]->IsActive() && post_run_sys_vec[id]->IsUpdated()) {
      post_run_sys_vec[id]->LogicUpdate(dt);
      post_run_sys_vec[id]->temporary_memory_.clear();
    }
  };
  for (size_t i = 0; i < post_run_sys_vec.size(); ++i) post_run_systems(i);

  // auto r = range(0, post_run_sys_vec.size());
  // std::for_each(std::execution::par_unseq, std::begin(r), std::end(r),
  //               post_run_systems);
}
}  // namespace lib_core
