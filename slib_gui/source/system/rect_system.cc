#include "rect_system.h"
#include "cursor_input.h"
#include "entity_manager.h"
#include "gui_button.h"
#include "gui_checkbox.h"
#include "gui_progress_bar.h"
#include "gui_scroll_list.h"
#include "gui_slider.h"

namespace lib_gui {
RectSystem::RectSystem(lib_core::EngineCore* engine) : engine_(engine) {}

void RectSystem::LogicUpdate(float dt) {
  auto rect_comps = g_ent_mgr.GetNewCbt<GuiRect>();

  if (rect_comps) {
    auto win_dim = engine_->GetWindow()->GetWindowDim();
    auto old_comps = g_ent_mgr.GetOldCbt<GuiRect>();
    auto cursor_comp = g_ent_mgr.GetNewCbeR<lib_input::CursorInput>();
    auto rect_update = g_ent_mgr.GetNewUbt<GuiRect>();
    auto cursor_check_func = [&](tbb::blocked_range<size_t>& range) {
      for (size_t i = range.begin(); i != range.end(); ++i) {
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
      }
    };

    tbb::parallel_for(tbb::blocked_range<size_t>(0, rect_update->size()),
                      cursor_check_func);

    auto update_func = [&](tbb::blocked_range<size_t>& range) {
      for (size_t i = range.begin(); i != range.end(); ++i) {
        if (!(*rect_update)[i]) continue;
        (*rect_update)[i] = false;
        (*rect_comps)[i] = (*old_comps)[i];
      }
    };

    tbb::parallel_for(tbb::blocked_range<size_t>(0, rect_update->size()),
                      update_func);
  }
}
}  // namespace lib_gui
