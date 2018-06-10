#include "port_audio_system.h"
#include "ambient_sound.h"
#include "effect_sound.h"
#include "engine_settings.h"
#include "entity_manager.h"
#include "music.h"
#ifdef UnixBuild
#include "portaudio.h"
#elif WindowsBuild
#include "portaudio/portaudio.h"
#endif
#include <cassert>

namespace lib_sound {
int portaudio_callback(const void *input, void *output, unsigned long frames,
                       const PaStreamCallbackTimeInfo *time,
                       PaStreamCallbackFlags status, void *user_data) {
  auto stream_info = static_cast<PortAudioSystem::StreamInfo *>(user_data);

  PortAudioSystem::SoundInfo info;
  while (stream_info->add_sound_queue_.try_pop(info))
    stream_info->playing_sounds_[info.type][info.ent] = info;

  std::pair<lib_core::Entity, PortAudioSystem::SoundType> rem_sound;
  while (stream_info->remove_sound_queue_.try_pop(rem_sound))
    stream_info->playing_sounds_[rem_sound.second].erase(rem_sound.first);

  return stream_info->sound_func(output, frames, stream_info);
}

template <typename T>
int sound_callback(void *output, unsigned long frames,
                   PortAudioSystem::StreamInfo *info) {
  float volumes[5];
  volumes[0] = g_settings.MusicVolume();
  volumes[1] = g_settings.SoundEffectVolume();
  volumes[2] = g_settings.AmbientVolume();
  volumes[3] = g_settings.VoiceVolume();
  volumes[4] = g_settings.MasterVolume();

  size_t fi;
  auto *out = static_cast<T *>(output);
  auto &samples = info->sample_storage.get_value<ct::dyn_array<T>>();
  for (int i = 0; i < 4; ++i) {
    std::memset(samples.data(), 0, samples.size() * sizeof(T));
    for (auto &s : info->playing_sounds_[i]) {
      auto sound_data = info->sys->GetSoundData(s.second.sound);
      if (!sound_data) continue;
      auto sound_desc = info->sys->GetSoundDesc(s.second.sound);

      for (fi = 0; fi < frames; ++fi) {
        if (s.second.progress >= sound_data->size()) {
          if (s.second.loop)
            s.second.progress = 0;
          else {
            info->sys->RemoveSoundEntity(s.second.type, s.second.ent);
            break;
          }
        }

        for (int ii = 0; ii < info->channels; ++ii)
          samples[fi * info->channels + ii] +=
              T(*reinterpret_cast<T *>(
                    &sound_data->at(s.second.progress +
                                    sizeof(T) * (ii % sound_desc->channels))) *
                s.second.volume);
        s.second.progress += sound_desc->channels * sizeof(T);
      }
    }

    out = static_cast<T *>(output);
    if (i == 0)
      for (fi = 0; fi < frames; ++fi)
        for (int ii = 0; ii < info->channels; ++ii)
          *out++ = T(samples[fi * info->channels + ii] * volumes[i]);
    else
      for (fi = 0; fi < frames; ++fi)
        for (int ii = 0; ii < info->channels; ++ii)
          *out++ += T(samples[fi * info->channels + ii] * volumes[i]);
  }

  out = static_cast<T *>(output);
  for (fi = 0; fi < frames; ++fi) {
    for (int i = 0; i < info->channels; ++i) {
      *out = T(*out * volumes[4]);
      ++out;
    }
  }

  return paContinue;
}

PortAudioSystem::PortAudioSystem() {
  add_music_cb_ =
      g_ent_mgr.RegisterAddComponentCallback<Music>([&](lib_core::Entity ent) {
        auto music_comp = g_ent_mgr.GetNewCbeR<Music>(ent);

        SoundInfo info;
        info.ent = ent;
        info.loop = music_comp->loop;
        info.progress = 0;
        info.sound = music_comp->audio_id;
        info.type = kMusic;
        info.volume = music_comp->volume;

        stream_info_.add_sound_queue_.push(info);
      });
  add_es_cb_ = g_ent_mgr.RegisterAddComponentCallback<EffectSound>(
      [&](lib_core::Entity ent) {
        auto es_comp = g_ent_mgr.GetNewCbeR<EffectSound>(ent);

        SoundInfo info;
        info.ent = ent;
        info.loop = false;
        info.progress = 0;
        info.sound = es_comp->audio_id;
        info.type = kEffect;
        info.volume = es_comp->volume;

        stream_info_.add_sound_queue_.push(info);
      });
  add_as_cd_ = g_ent_mgr.RegisterAddComponentCallback<AmbientSound>(
      [&](lib_core::Entity ent) {
        auto as_comp = g_ent_mgr.GetNewCbeR<AmbientSound>(ent);

        SoundInfo info;
        info.ent = ent;
        info.loop = as_comp->loop;
        info.progress = 0;
        info.sound = as_comp->audio_id;
        info.type = kAmbient;
        info.volume = as_comp->volume;

        stream_info_.add_sound_queue_.push(info);
      });

  rem_music_cb_ = g_ent_mgr.RegisterRemoveComponentCallback<Music>(
      [&](lib_core::Entity ent) {
        stream_info_.remove_sound_queue_.push({ent, kMusic});
      });
  rem_as_cd_ = g_ent_mgr.RegisterRemoveComponentCallback<AmbientSound>(
      [&](lib_core::Entity ent) {
        stream_info_.remove_sound_queue_.push({ent, kAmbient});
      });
}

PortAudioSystem::~PortAudioSystem() {
  g_ent_mgr.UnregisterAddComponentCallback<Music>(add_music_cb_);
  g_ent_mgr.UnregisterAddComponentCallback<EffectSound>(add_es_cb_);
  g_ent_mgr.UnregisterAddComponentCallback<AmbientSound>(add_as_cd_);
  g_ent_mgr.UnregisterRemoveComponentCallback<Music>(rem_music_cb_);
  g_ent_mgr.UnregisterRemoveComponentCallback<AmbientSound>(rem_as_cd_);
}

void PortAudioSystem::InitSystem() {
  auto err = Pa_Initialize();
  cu::AssertError(err == paNoError,
                  "Port audio initialization error: " + std::to_string(err),
                  __FILE__, __LINE__);

  stream_info_.playing_sounds_.assign(4, {});
  stream_info_.sys = this;
  stream_info_.channels = 2;
  stream_info_.sound_func = &sound_callback<int16_t>;
  stream_info_.sample_storage =
      any_type(ct::dyn_array<int16_t>(256 * stream_info_.channels, 0));

  PaStreamParameters output_params;
  output_params.channelCount = 2;
  output_params.hostApiSpecificStreamInfo = nullptr;
  output_params.sampleFormat = paInt16;
  output_params.suggestedLatency = 0.04;
  output_params.device = Pa_GetDefaultOutputDevice();
  err = Pa_OpenStream(&stream_, nullptr, &output_params, 44100, 256, paNoFlag,
                      portaudio_callback, &stream_info_);
  cu::AssertError(err == paNoError, "Port audio error: " + std::to_string(err),
                  __FILE__, __LINE__);

  err = Pa_StartStream(stream_);
  cu::AssertError(err == paNoError, "Port audio error: " + std::to_string(err),
                  __FILE__, __LINE__);
}

void PortAudioSystem::FinalizeSystem() {
  auto err = Pa_StopStream(stream_);
  cu::AssertError(err == paNoError, "Port audio error: " + std::to_string(err),
                  __FILE__, __LINE__);

  err = Pa_CloseStream(stream_);
  cu::AssertError(err == paNoError, "Port audio error: " + std::to_string(err),
                  __FILE__, __LINE__);

  err = Pa_Terminate();
  cu::AssertError(err == paNoError, "Port audio error: " + std::to_string(err),
                  __FILE__, __LINE__);
}

void PortAudioSystem::LogicUpdate(float dt) { SoundSystem::LogicUpdate(dt); }

void PortAudioSystem::RemoveSoundEntity(SoundType type, lib_core::Entity ent) {
  remove_sound_.push({type, ent});
}
}  // namespace lib_sound
