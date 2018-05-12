#pragma once

namespace lib_sound {
class EffectSound {
 public:
  EffectSound(size_t audio, float vol) : audio_id(audio), volume(vol) {}

  size_t audio_id;
  float volume = 1.f;
};
}  // namespace lib_sound