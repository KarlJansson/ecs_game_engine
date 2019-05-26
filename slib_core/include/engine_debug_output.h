#pragma once
#include "core_utilities.h"

namespace lib_core {
class EngineDebugOutput {
 public:
  EngineDebugOutput();
  ~EngineDebugOutput();

  void GenerateFont(int size);

  void ToggleDebugOutput();
  void ToggleTopLeftDebugOutput();
  void ToggleTopRightDebugOutput();
  void ToggleBottomLeftDebugOutput();
  void ToggleBottomRightDebugOutput();

  int AddTopLeftLine(const ct::string& str = "");
  int AddTopRightLine(const ct::string& str = "");
  int AddBottomLeftLine(const ct::string& str = "");
  int AddBottomRightLine(const ct::string& str = "");

  void UpdateTopLeftLine(int line, ct::string str = "");
  void UpdateTopRightLine(int line, ct::string str = "");
  void UpdateBottomLeftLine(int line, ct::string str = "");
  void UpdateBottomRightLine(int line, ct::string str = "");

 private:
  void ToggleDebugText(int i);
  void UpdateActiveText(int i, int line);

  std::array<ct::dyn_array<ct::string>, 4> text_strings_;
  std::array<ct::dyn_array<class Entity>, 4> text_ents_;
  std::array<bool, 4> toggles_;
  std::array<float, 4> text_scale_;
  size_t font_ = -1;
};
}  // namespace lib_core
