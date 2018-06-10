#pragma once
#include "command.h"
#include "system.h"

namespace lib_core {
class SystemManager {
 public:
  SystemManager(const SystemManager&) = delete;
  SystemManager(const SystemManager&&) = delete;
  SystemManager& operator=(const SystemManager) = delete;

  static SystemManager& get() {
    static SystemManager instance_;
    return instance_;
  }

  void LogicUpdate(float dt);
  void DrawUpdate(lib_graphics::Renderer* renderer,
                  lib_gui::TextSystem* text_renderer);

  void SyncSystems();
  void SyncInputSystems(float dt);

  void ClearSystems();
  void CleanSystems();

  template <typename T>
  void IssueCommand(T&& command) {
    command_queue_.push({[command{std::move(command)}]
          (ct::hash_map<size_t, any_type>& command_lists) {
          auto hash = typeid(T).hash_code();
          auto it = command_lists.find(hash);
          if (it == command_lists.end()) {
            command_lists[hash] = any_type(ct::de_queue<T>());
            it = command_lists.find(hash);
          }
          it->second.get_value<ct::de_queue<T>>().emplace_back(
              std::move(command));
        }});
  }

  template <typename T>
  ct::de_queue<T>* GetCommands() const {
    auto hash = typeid(T).hash_code();

    auto it = command_lists_.find(hash);
    if (it == command_lists_.end()) return nullptr;

    return &it->second.get_value<ct::de_queue<T>>();
  }

  inline size_t GenerateResourceIds(size_t count) {
    return resource_id_.fetch_add(count);
  }

 private:
  SystemManager()= default;

  struct IssueCommandStruct {
    std::function<void(ct::hash_map<size_t, any_type>&)> command;
  };

  std::atomic<size_t> resource_id_{0};
  ct::hash_map<size_t, any_type> command_lists_;
  tbb::concurrent_queue<IssueCommandStruct> command_queue_;

  ct::hash_map<size_t, ct::dyn_array<std::shared_ptr<System>>> system_map_;
  ct::hash_map<size_t, std::pair<size_t, System*>> system_id_map_;
  int empty_frames_ = 0;
};
}  // namespace lib_core

static auto& g_sys_mgr = lib_core::SystemManager::get();

template <typename T>
void issue_command(T&& command) {
  g_sys_mgr.IssueCommand(std::move(command));
}
