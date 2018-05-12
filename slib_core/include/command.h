#pragma once
#include <cstddef>

namespace lib_core {
class Command {
public:
  Command() = default;
  ~Command() = default;

  size_t base_id = 0;
};
} // namespace lib_core
