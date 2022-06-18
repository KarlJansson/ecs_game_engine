#include "text_system.h"
#include "entity_manager.h"
#include "gui_text.h"
#include "range_iterator.hpp"

#include <execution>

namespace lib_gui {

TextSystem::TextSystem(lib_core::EngineCore* engine) : engine_(engine) {}

void TextSystem::DrawUpdate(lib_graphics::Renderer* renderer,
                            TextSystem* text_renderer) {
  auto unload_font_commands = g_sys_mgr.GetCommands<UnloadFontCommand>();
  if (unload_font_commands && !unload_font_commands->empty()) {
    for (auto& c : *unload_font_commands) {
      HandleUnloadCommand(c);
    }
    unload_font_commands->clear();
  }

  auto load_font_commands = g_sys_mgr.GetCommands<LoadFontCommand>();
  if (load_font_commands && !load_font_commands->empty()) {
    for (auto& c : *load_font_commands) {
      // Abort if this command has been nullified
      if (missing_removed_.find(c.FontId()) != missing_removed_.end()) {
        missing_removed_.erase(c.FontId());
        continue;
      }

      auto hash = std::hash<ct::string>{}(c.path) + c.size;
      auto font_it = shared_resource_lookup_.find(hash);
      if (font_it != shared_resource_lookup_.end()) {
        font_id_mapping_[c.FontId()] = font_it->second;
        ++fonts_[font_it->second].ref_count;
      } else {
        auto& font_pack = fonts_[c.FontId()];
        font_pack.hash = hash;
        font_pack.ref_count = 1;
        shared_resource_lookup_[hash] = c.FontId();
        font_id_mapping_[c.FontId()] = c.FontId();
        HandleLoadCommand(c);
      }
    }
    load_font_commands->clear();
  }
}

void TextSystem::LogicUpdate(float dt) {
  auto text_comps = g_ent_mgr.GetNewCbt<GuiText>();

  if (text_comps) {
    auto old_comps = g_ent_mgr.GetOldCbt<GuiText>();
    auto text_update = g_ent_mgr.GetNewUbt<GuiText>();

    auto update_func = [&](size_t i) {
      if ((*text_update)[i]) {
        (*text_update)[i] = false;

        auto& old_text = old_comps->at(i);
        auto& new_text = text_comps->at(i);

        new_text = old_text;
      }
    };

    auto r = range(0, text_update->size());
    std::for_each(std::execution::par_unseq, std::begin(r), std::end(r),
                  update_func);
  }
}
}  // namespace lib_gui
