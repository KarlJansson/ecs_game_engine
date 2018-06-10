#pragma once
#include <utility>

#include "system_manager.h"

namespace lib_gui {
class LoadFontCommand : public lib_core::Command {
 public:
  LoadFontCommand() = default;
  LoadFontCommand(int size, ct::string path) : size(size), path(std::move(path)) {
    base_id = g_sys_mgr.GenerateResourceIds(1);
  }

  size_t FontId() const { return base_id; }

  int size;
  ct::string path;
};

class UnloadFontCommand : public lib_core::Command {
 public:
  UnloadFontCommand(size_t font_id) : font_id(font_id) {}

  size_t font_id;
};
}  // namespace lib_gui
