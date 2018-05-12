#pragma once

namespace lib_sound {
class Music {
 public:
  size_t audio_id;
  bool loop = true;
  float volume = 1.f;
};
}  // namespace lib_sound