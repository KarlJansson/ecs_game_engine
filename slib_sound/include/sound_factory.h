#pragma once
#include "sound_system.h"

namespace lib_sound {
class SoundFactory {
 public:
  SoundFactory() = default;
  ~SoundFactory() = default;

  std::unique_ptr<SoundSystem> CreateSoundSystem();
};
}  // namespace lib_sound
