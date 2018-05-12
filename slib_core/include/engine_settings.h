#pragma once
#include "core_utilities.h"
#include "any_type.hpp"

namespace lib_core {
class EngineSettings {
 public:
  static EngineSettings& get();

  void SetVSync(bool value);
  void SetBloom(bool value);
  void SetSsao(bool value);
  void SetSmaa(bool value);
  void SetFullscreen(bool value);
  void SetWindowed(bool value);

  bool VSync() const;
  bool Bloom() const;
  bool Ssao() const;
  bool Smaa() const;
  bool Fullscreen() const;
  bool Windowed() const;

  void SetGamma(float value);
  void SetMasterVolume(float value);
  void SetMusicVolume(float value);
  void SetSoundEffectVolume(float value);
  void SetAmbientVolume(float value);
  void SetVoiceVolume(float value);
  void SetSaturation(float value);
  void SetHue(float value);
  void SetBrightness(float value);
  void SetMouseSensitivity(float value);
  void SetMouseSmoothing(float value);
  void SetFramePace(float value);

  float Gamma() const;
  float MasterVolume() const;
  float MusicVolume() const;
  float SoundEffectVolume() const;
  float AmbientVolume() const;
  float VoiceVolume() const;
  float Saturation() const;
  float Hue() const;
  float Brightness() const;
  float MouseSensitivity() const;
  float MouseSmoothing() const;
  float FramePace() const;
  float FpsTarget() const;

  void SetWindowedWidth(int width);
  void SetWindowedHeight(int height);
  void SetFullscreenWidth(int width);
  void SetFullscreenHeight(int height);
  void SetMaxShadowResolution(int res);
  void SetMaxTextureResolution(int res);

  int WindowedWidth() const;
  int WindowedHeight() const;
  int FullscreenWidth() const;
  int FullscreenHeight() const;
  int MaxShadowTexture() const;
  int MaxTextureResolution() const;

  void LoadSettings();
  void SaveSettings();
  void SetDefaults();

 private:
  EngineSettings();
  ~EngineSettings() = default;

  template <typename T>
  void SetSetting(T value, int pos);
  template <typename T>
  T Setting(int pos) const;

  ct::hash_map<size_t, any_type> setting_vecs_;
  ct::hash_map<size_t, ct::hash_map<ct::string, int> > enum_string_map_;
  ct::hash_map<size_t, ct::hash_map<int, ct::string> > enum_string_map_rev_;

  enum IntSettings {
    kWindowedWidth,
    kWindowedHeight,
    kFullscreenWidth,
    kFullscreenHeight,
    kMaxShadowResoulution,
    kMaxTextureResolution
  };
  enum FloatSettings {
    kGamma,
    kMasterVolume,
    kMusicVolume,
    kAmbientVolume,
    kSoundEffectVolume,
    kVoiceVolume,
    kSaturation,
    kBrightness,
    kHue,
    kMouseSensitivity,
    kMouseSmoothing,
    kFramePace
  };
  enum BoolSettings { kVsync, kBloom, kSsao, kSmaa, kFullscreen, kWindowed };
};
}  // namespace lib_core

static auto& g_settings = lib_core::EngineSettings::get();
