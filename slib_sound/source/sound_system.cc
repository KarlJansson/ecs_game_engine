#include "sound_system.h"
#include "ambient_sound.h"
#include "core_utilities.h"
#include "effect_sound.h"
#include "entity_manager.h"
#include "music.h"

namespace lib_sound {
SoundSystem::SoundSystem() {
  callback_ids[0] =
      g_ent_mgr.RegisterAddComponentCallback<Music>([&](lib_core::Entity ent) {
        auto comp = g_ent_mgr.GetNewCbeR<Music>(ent);
        add_sound_.insert(comp->audio_id);
      });
  callback_ids[1] = g_ent_mgr.RegisterAddComponentCallback<EffectSound>(
      [&](lib_core::Entity ent) {
        auto comp = g_ent_mgr.GetNewCbeR<EffectSound>(ent);
        add_sound_.insert(comp->audio_id);
      });
  callback_ids[2] = g_ent_mgr.RegisterAddComponentCallback<AmbientSound>(
      [&](lib_core::Entity ent) {
        auto comp = g_ent_mgr.GetNewCbeR<AmbientSound>(ent);
        add_sound_.insert(comp->audio_id);
      });
}

SoundSystem::~SoundSystem() {
  g_ent_mgr.UnregisterAddComponentCallback<Music>(callback_ids[0]);
  g_ent_mgr.UnregisterAddComponentCallback<EffectSound>(callback_ids[1]);
  g_ent_mgr.UnregisterAddComponentCallback<AmbientSound>(callback_ids[2]);
}

void SoundSystem::RegisterSoundBank(const ct::string &path) {
  add_packs_.push(path);
}

void SoundSystem::UnloadSoundBank(const ct::string &path) {
  remove_packs_.push(path);
}

size_t SoundSystem::LoadSound(const ct::string &name) {
  auto sound_hash = std::hash<ct::string>{}(name);
  return sound_hash;
}

void SoundSystem::LogicUpdate(float dt) {
  RegisterQueuedBanks();

  for (auto s : add_sound_)
    if (loaded_sounds_.find(s) == loaded_sounds_.end()) LoadSound(s);
  add_sound_.clear();

  std::pair<SoundType, lib_core::Entity> p;
  while (remove_sound_.try_pop(p)) {
    switch (p.first) {
      case kMusic:
        g_ent_mgr.RemoveComponent<Music>(p.second);
        break;
      case kEffect:
        break;
      case kAmbient:
        break;
      case kVoice:
        break;
    }
  }
}

ct::dyn_array<uint8_t> *SoundSystem::GetSoundData(size_t hash) {
  auto it = sound_bank_map_.find(hash);
  if (it != sound_bank_map_.end()) {
    auto bank_it = sound_banks_.find(it->second);
    if (bank_it != sound_banks_.end()) {
      auto sound_it = bank_it->second.sounds.find(hash);
      if (sound_it != bank_it->second.sounds.end()) {
        if (sound_it->second.data.empty())
          return nullptr;
        else
          return &sound_it->second.data;
      }
    }
  }
  return nullptr;
}

SoundSystem::SoundDesc *SoundSystem::GetSoundDesc(size_t hash) {
  auto it = sound_bank_map_.find(hash);
  if (it != sound_bank_map_.end()) {
    auto bank_it = sound_banks_.find(it->second);
    if (bank_it != sound_banks_.end()) {
      auto sound_it = bank_it->second.sounds.find(hash);
      if (sound_it != bank_it->second.sounds.end())
        return &sound_it->second.desc;
    }
  }
  return nullptr;
}

void SoundSystem::RegisterQueuedBanks() {
  ct::string path;
  while (add_packs_.try_pop(path)) {
    auto bank_hash = std::hash<ct::string>{}(path);
    if (sound_banks_.find(bank_hash) != sound_banks_.end()) return;

    auto &bank = sound_banks_[bank_hash];
    bank.file = std::ifstream(path, std::ios::binary);

    if (!bank.file.fail()) {
      size_t nr_sounds;
      bank.file.read((char *)&nr_sounds, sizeof(nr_sounds));

      size_t path_length;
      ct::string sound_str;
      for (int i = 0; i < nr_sounds; ++i) {
        bank.file.read((char *)&path_length, sizeof(path_length));
        sound_str.assign(path_length, ' ');
        bank.file.read((char *)sound_str.data(), sizeof(uint8_t) * path_length);

        if (sound_str.find_last_of('\\') != ct::string::npos)
          sound_str = sound_str.substr(sound_str.find_last_of('\\') + 1,
                                       sound_str.size());

        auto sound_hash = std::hash<ct::string>{}(sound_str);
        sound_bank_map_[sound_hash] = bank_hash;
        auto &sound = bank.sounds[sound_hash];
        bank.file.read((char *)&sound.desc, sizeof(sound.desc));
      }
    } else
      sound_banks_.erase(bank_hash);
  }

  while (remove_packs_.try_pop(path)) {
    auto bank_hash = std::hash<ct::string>{}(path);
    sound_banks_.erase(bank_hash);
  }
}

void SoundSystem::LoadSound(size_t hash) {
  auto it = sound_bank_map_.find(hash);
  if (it != sound_bank_map_.end()) {
    auto bank_it = sound_banks_.find(it->second);
    if (bank_it != sound_banks_.end()) {
      auto sound_it = bank_it->second.sounds.find(hash);
      if (sound_it != bank_it->second.sounds.end()) {
        if (sound_it->second.data.empty()) {
          bank_it->second.file.seekg(sound_it->second.desc.data_location);

          size_t data_size;
          bank_it->second.file.read((char *)&data_size, sizeof(size_t));
          ct::dyn_array<uint8_t> compressed(data_size);
          bank_it->second.file.read((char *)compressed.data(),
                                    compressed.size());

          cu::DecompressMemory(compressed, sound_it->second.data);

          loaded_sounds_.insert(hash);
        }
      }
    }
  }
}
}  // namespace lib_sound
