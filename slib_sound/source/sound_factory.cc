#include "sound_factory.h"
#include "port_audio_system.h"

namespace lib_sound {
std::unique_ptr<SoundSystem> SoundFactory::CreateSoundSystem() {
  return std::make_unique<PortAudioSystem>();
}
}  // namespace lib_sound
