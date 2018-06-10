#pragma once
#include "sound_system.h"

namespace lib_sound {
class PortAudioSystem : public SoundSystem {
 public:
  PortAudioSystem();
  ~PortAudioSystem() override;

  void InitSystem() override;
  void FinalizeSystem() override;
  void LogicUpdate(float dt) override;

  struct SoundInfo {
    lib_core::Entity ent;
    SoundType type;
    size_t sound;
    float volume;
    size_t progress = 0;
    bool loop = true;
  };

  struct StreamInfo {
    std::function<int(void*, unsigned long, StreamInfo*)> sound_func;

    int channels;
    PortAudioSystem* sys;
    any_type sample_storage;

    ct::dyn_array<ct::hash_map<lib_core::Entity, SoundInfo>> playing_sounds_;

    tbb::concurrent_queue<SoundInfo> add_sound_queue_;
    tbb::concurrent_queue<std::pair<lib_core::Entity, SoundType>>
        remove_sound_queue_;
  };

  void RemoveSoundEntity(SoundType type, lib_core::Entity ent);

 private:
  void* stream_;
  StreamInfo stream_info_;

  size_t add_music_cb_, add_es_cb_, add_as_cd_;
  size_t rem_music_cb_, rem_as_cd_;
};
}  // namespace lib_sound
