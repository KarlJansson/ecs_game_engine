#pragma once
#include <fstream>
#include "engine_settings.h"
#include "sound_commands.h"
#include "system.h"

namespace lib_sound {
class SoundSystem : public lib_core::System {
 public:
  SoundSystem();
  ~SoundSystem() override;

  void RegisterSoundBank(const ct::string& path);
  void UnloadSoundBank(const ct::string& path);

  size_t LoadSound(const ct::string& name);
  void LogicUpdate(float dt) override;

  enum SoundType { kMusic, kEffect, kAmbient, kVoice };
  struct SoundDesc {
    uint16_t channels;
    uint16_t bits_per_sample;
    uint32_t sample_rate;
    size_t data_location;
  };
  ct::dyn_array<uint8_t>* GetSoundData(size_t hash);
  SoundDesc* GetSoundDesc(size_t hash);

 protected:
  void RegisterQueuedBanks();
  void LoadSound(size_t hash);

  struct Sound {
    SoundDesc desc;
    ct::dyn_array<uint8_t> data;
  };

  struct SoundBank {
    std::ifstream file;
    ct::hash_map<size_t, Sound> sounds;
  };

  tbb::concurrent_queue<ct::string> add_packs_, remove_packs_;
  tbb::concurrent_queue<std::pair<SoundType, lib_core::Entity>> remove_sound_;

  ct::hash_set<size_t> add_sound_;

  ct::hash_set<size_t> loaded_sounds_;
  ct::hash_map<size_t, SoundBank> sound_banks_;
  ct::hash_map<size_t, size_t> sound_bank_map_;

  size_t callback_ids[3];
};
}  // namespace lib_sound
