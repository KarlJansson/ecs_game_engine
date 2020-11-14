#pragma once
#include "engine_core.h"
#include "gui_commands.h"
#include "system.h"

namespace lib_gui {
class TextSystem : public lib_core::System {
 public:
  TextSystem(lib_core::EngineCore* engine);
  ~TextSystem() override = default;

  void DrawUpdate(lib_graphics::Renderer* renderer,
                  TextSystem* text_renderer) override;
  void LogicUpdate(float dt) override;
  virtual void PurgeGpuResources() = 0;
  virtual void RenderText(class GuiText text, lib_core::Vector2 screen_dim) = 0;

 protected:
  struct Character {
    unsigned int texture_id;
    std::pair<int, int> size;
    std::pair<int, int> bearing;
    unsigned int advance;
  };

  struct FontPack {
    size_t hash;
    size_t ref_count;
    size_t font_size;
    ct::dyn_array<Character> glyphs;
  };

  ct::hash_map<size_t, size_t> font_id_mapping_;
  ct::hash_map<size_t, FontPack> fonts_;
  ct::hash_map<size_t, size_t> shared_resource_lookup_;

  ct::hash_map<size_t, LoadFontCommand> loaded_fonts_;
  ct::hash_set<size_t> missing_removed_;

 protected:
  lib_core::EngineCore* engine_;

 private:
  virtual void HandleUnloadCommand(UnloadFontCommand& command) = 0;
  virtual void HandleLoadCommand(LoadFontCommand& command) = 0;
};
}  // namespace lib_gui
