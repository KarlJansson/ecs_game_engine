#include "rect_system.h"
#include "entity_manager.h"
#include "gui_button.h"
#include "gui_checkbox.h"
#include "gui_progress_bar.h"
#include "gui_scroll_list.h"
#include "gui_slider.h"

namespace lib_gui {
void RectSystem::LogicUpdate(float dt) {
  auto rect_comps = g_ent_mgr.GetNewCbt<GuiRect>();

  if (rect_comps) {
    auto old_comps = g_ent_mgr.GetOldCbt<GuiRect>();
    auto rect_update = g_ent_mgr.GetNewUbt<GuiRect>();

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
