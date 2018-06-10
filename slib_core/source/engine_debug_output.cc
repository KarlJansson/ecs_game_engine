#include "engine_debug_output.h"

#include <utility>
#include "entity.h"
#include "entity_manager.h"
#include "gui_rect.h"
#include "gui_text.h"
#include "system_manager.h"
#include "text_system.h"

namespace lib_core {
EngineDebugOutput::EngineDebugOutput() {
  toggles_[0] = toggles_[1] = toggles_[2] = toggles_[3] = false;
  text_scale_[0] = text_scale_[1] = text_scale_[2] = text_scale_[3] = 1.f;
}

EngineDebugOutput::~EngineDebugOutput() {
  for (auto & text_ent : text_ents_)
    for (auto e : text_ent) g_ent_mgr.RemoveEntity(e);
}

void EngineDebugOutput::GenerateFont(int size) {
  if (font_ != -1) issue_command(lib_gui::UnloadFontCommand(font_));
  auto times_font = lib_gui::LoadFontCommand(size, "./content/timess.ttf");
  issue_command(times_font);
  font_ = times_font.FontId();
  for (int i = 0; i < 4; ++i) {
    if (toggles_[i]) {
      ToggleDebugText(i);
      ToggleDebugText(i);
    }
  }
}

void EngineDebugOutput::ToggleDebugOutput() {
  for (int i = 0; i < 4; ++i) ToggleDebugText(i);
}

void EngineDebugOutput::ToggleTopLeftDebugOutput() { ToggleDebugText(0); }
void EngineDebugOutput::ToggleTopRightDebugOutput() { ToggleDebugText(1); }
void EngineDebugOutput::ToggleBottomLeftDebugOutput() { ToggleDebugText(2); }
void EngineDebugOutput::ToggleBottomRightDebugOutput() { ToggleDebugText(3); }

int EngineDebugOutput::AddTopLeftLine(const ct::string& str) {
  auto line = int(text_strings_[0].size());
  text_strings_[0].push_back(str);
  text_ents_[0].push_back(g_ent_mgr.CreateEntity());
  UpdateActiveText(0, line);
  return line;
}

int EngineDebugOutput::AddTopRightLine(const ct::string& str) {
  auto line = int(text_strings_[1].size());
  text_strings_[1].push_back(str);
  text_ents_[1].push_back(g_ent_mgr.CreateEntity());
  UpdateActiveText(1, line);
  return line;
}

int EngineDebugOutput::AddBottomLeftLine(const ct::string& str) {
  auto line = int(text_strings_[2].size());
  text_strings_[2].push_back(str);
  text_ents_[2].push_back(g_ent_mgr.CreateEntity());
  UpdateActiveText(2, line);
  return line;
}

int EngineDebugOutput::AddBottomRightLine(const ct::string& str) {
  auto line = int(text_strings_[3].size());
  text_strings_[3].push_back(str);
  text_ents_[3].push_back(g_ent_mgr.CreateEntity());
  UpdateActiveText(3, line);
  return line;
}

void EngineDebugOutput::UpdateTopLeftLine(int line, ct::string str) {
  while (line >= text_strings_[0].size()) text_strings_[0].push_back("");
  text_strings_[0][line] = std::move(str);
  UpdateActiveText(0, line);
}

void EngineDebugOutput::UpdateTopRightLine(int line, ct::string str) {
  while (line >= text_strings_[1].size()) text_strings_[1].push_back("");
  text_strings_[1][line] = std::move(str);
  UpdateActiveText(1, line);
}

void EngineDebugOutput::UpdateBottomLeftLine(int line, ct::string str) {
  while (line >= text_strings_[2].size()) text_strings_[2].push_back("");
  text_strings_[2][line] = std::move(str);
  UpdateActiveText(2, line);
}

void EngineDebugOutput::UpdateBottomRightLine(int line, ct::string str) {
  while (line >= text_strings_[3].size()) text_strings_[3].push_back("");
  text_strings_[3][line] = std::move(str);
  UpdateActiveText(3, line);
}

void EngineDebugOutput::ToggleDebugText(int i) {
  if (toggles_[i]) {
    for (auto e : text_ents_[i]) g_ent_mgr.RemoveComponent<lib_gui::GuiText>(e);
  } else {
    int line = 0;
    lib_gui::GuiText::HAlignment align =
        i % 2 == 0 ? lib_gui::GuiText::kLeft : lib_gui::GuiText::kRight;

    float x_pos = i % 2 == 0 ? .01f : 1.f - .01f;
    for (auto &str : text_strings_[i]) {
      float y_pos = i < 2 ? float(1.f - (.02f + line * .02f * text_scale_[i]))
                          : float(.02f + line * .02f * text_scale_[i]);
      g_ent_mgr.AddComponent(
          text_ents_[i][line],
          lib_gui::GuiText(str, {x_pos, y_pos}, {.4f, .4f},
                           {1.f, 1.f, 0.f, 1.f}, font_, align));

      ++line;
    }
  }
  toggles_[i] = !toggles_[i];
}

void EngineDebugOutput::UpdateActiveText(int i, int line) {
  while (line >= text_ents_[i].size())
    text_ents_[i].push_back(g_ent_mgr.CreateEntity());

  if (toggles_[i]) {
    lib_gui::GuiText::HAlignment align =
        i % 2 == 0 ? lib_gui::GuiText::kLeft : lib_gui::GuiText::kRight;
    auto text = g_ent_mgr.GetNewCbeW<lib_gui::GuiText>(text_ents_[i][line]);
    if (text) {
      text->text = text_strings_[i][line];
    } else {
      float x_pos = i % 2 == 0 ? .01f : 1.f - .01f;
      float y_pos = i < 2 ? float(1.f - (.02f + line * .02f * text_scale_[i]))
                          : float(.02f + line * .02f * text_scale_[i]);

      g_ent_mgr.AddComponent(
          text_ents_[i][line],
          lib_gui::GuiText(text_strings_[i][line], {x_pos, y_pos}, {.4f, .4f},
                           {1.f, 1.f, 0.f, 1.f}, font_, align));
    }
  }
}
}  // namespace lib_core
