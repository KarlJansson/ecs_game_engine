#include "rect_system.h"
#include "cursor_input.h"
#include "entity_manager.h"
#include "gui_button.h"
#include "gui_checkbox.h"
#include "gui_progress_bar.h"
#include "gui_scroll_list.h"
#include "gui_slider.h"
#include "range_iterator.hpp"

#include <execution>

namespace lib_gui {
RectSystem::RectSystem(lib_core::EngineCore* engine) : engine_(engine) {}

void RectSystem::LogicUpdate(float dt) {
  auto rect_comps = g_ent_mgr.GetNewCbt<GuiRect>();

  if (rect_comps) {
    auto old_comps = g_ent_mgr.GetOldCbt<GuiRect>();
    auto cursor_comp = g_ent_mgr.GetNewCbeR<lib_input::CursorInput>();
    auto rect_update = g_ent_mgr.GetNewUbt<GuiRect>();

    auto cursor_check_func = [&](size_t i) {
      auto& rect = (*old_comps)[i];
      if (cursor_comp->pos[0] < rect.position_[0] + rect.half_size_[0] &&
          cursor_comp->pos[0] > rect.position_[0] - rect.half_size_[0] &&
          cursor_comp->pos[1] < rect.position_[1] + rect.half_size_[1] &&
          cursor_comp->pos[1] > rect.position_[1] - rect.half_size_[1]) {
        rect.hover = true;
        (*rect_update)[i] = true;
      } else if (rect.hover) {
        rect.hover = false;
        (*rect_update)[i] = true;
      }
    };

    auto update_func = [&](size_t i) {
      if ((*rect_update)[i]) {
        (*rect_update)[i] = false;
        (*rect_comps)[i] = (*old_comps)[i];
      }
    };

    auto r = range(0, rect_update->size());
    std::for_each(std::execution::par_unseq, std::begin(r), std::end(r),
                  cursor_check_func);
    std::for_each(std::execution::par_unseq, std::begin(r), std::end(r),
                  update_func);
  }
}
}  // namespace lib_gui
