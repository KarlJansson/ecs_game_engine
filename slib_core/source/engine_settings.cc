#include "engine_settings.h"
#include <fstream>

namespace lib_core {
EngineSettings::EngineSettings() {
  auto bool_hash = typeid(bool).hash_code();
  auto float_hash = typeid(float).hash_code();
  auto int_hash = typeid(int).hash_code();

  enum_string_map_[bool_hash]["kVsync"] = kVsync;
  enum_string_map_[bool_hash]["kBloom"] = kBloom;
  enum_string_map_[bool_hash]["kSsao"] = kSsao;
  enum_string_map_[bool_hash]["kSmaa"] = kSmaa;
  enum_string_map_[bool_hash]["kFullscreen"] = kFullscreen;
  enum_string_map_[bool_hash]["kWindowed"] = kWindowed;

  enum_string_map_[float_hash]["kGamma"] = kGamma;
  enum_string_map_[float_hash]["kMasterVolume"] = kMasterVolume;
  enum_string_map_[float_hash]["kMusicVolume"] = kMusicVolume;
  enum_string_map_[float_hash]["kAmbientVolume"] = kAmbientVolume;
  enum_string_map_[float_hash]["kSoundEffectVolume"] = kSoundEffectVolume;
  enum_string_map_[float_hash]["kVoiceVolume"] = kVoiceVolume;
  enum_string_map_[float_hash]["kSaturation"] = kSaturation;
  enum_string_map_[float_hash]["kBrightness"] = kBrightness;
  enum_string_map_[float_hash]["kHue"] = kHue;
  enum_string_map_[float_hash]["kMouseSensitivity"] = kMouseSensitivity;
  enum_string_map_[float_hash]["kMouseSmoothing"] = kMouseSmoothing;
  enum_string_map_[float_hash]["kFramePace"] = kFramePace;

  enum_string_map_[int_hash]["kWindowedWidth"] = kWindowedWidth;
  enum_string_map_[int_hash]["kWindowedHeight"] = kWindowedHeight;
  enum_string_map_[int_hash]["kFullscreenWidth"] = kFullscreenWidth;
  enum_string_map_[int_hash]["kFullscreenHeight"] = kFullscreenHeight;
  enum_string_map_[int_hash]["kMaxShadowResoulution"] = kMaxShadowResoulution;
  enum_string_map_[int_hash]["kMaxTextureResolution"] = kMaxTextureResolution;

  enum_string_map_rev_[bool_hash][kVsync] = "kVsync";
  enum_string_map_rev_[bool_hash][kBloom] = "kBloom";
  enum_string_map_rev_[bool_hash][kSsao] = "kSsao";
  enum_string_map_rev_[bool_hash][kFullscreen] = "kFullscreen";
  enum_string_map_rev_[bool_hash][kWindowed] = "kWindowed";
  enum_string_map_rev_[bool_hash][kSmaa] = "kSmaa";
  enum_string_map_rev_[float_hash][kGamma] = "kGamma";
  enum_string_map_rev_[float_hash][kMasterVolume] = "kMasterVolume";
  enum_string_map_rev_[float_hash][kMusicVolume] = "kMusicVolume";
  enum_string_map_rev_[float_hash][kAmbientVolume] = "kAmbientVolume";
  enum_string_map_rev_[float_hash][kSoundEffectVolume] = "kSoundEffectVolume";
  enum_string_map_rev_[float_hash][kVoiceVolume] = "kVoiceVolume";
  enum_string_map_rev_[float_hash][kSaturation] = "kSaturation";
  enum_string_map_rev_[float_hash][kBrightness] = "kBrightness";
  enum_string_map_rev_[float_hash][kHue] = "kHue";
  enum_string_map_rev_[float_hash][kMouseSensitivity] = "kMouseSensitivity";
  enum_string_map_rev_[float_hash][kMouseSmoothing] = "kMouseSmoothing";
  enum_string_map_rev_[float_hash][kFramePace] = "kFramePace";
  enum_string_map_rev_[int_hash][kWindowedWidth] = "kWindowedWidth";
  enum_string_map_rev_[int_hash][kWindowedHeight] = "kWindowedHeight";
  enum_string_map_rev_[int_hash][kFullscreenWidth] = "kFullscreenWidth";
  enum_string_map_rev_[int_hash][kFullscreenHeight] = "kFullscreenHeight";
  enum_string_map_rev_[int_hash][kMaxShadowResoulution] =
      "kMaxShadowResoulution";
  enum_string_map_rev_[int_hash][kMaxTextureResolution] =
      "kMaxTextureResolution";

  SetDefaults();
  LoadSettings();
  SaveSettings();
}

EngineSettings &EngineSettings::get() {
  static EngineSettings instance;
  return instance;
}

void EngineSettings::SetVSync(bool value) { SetSetting<bool>(value, kVsync); }

void EngineSettings::SetBloom(bool value) { SetSetting<bool>(value, kBloom); }

void EngineSettings::SetSsao(bool value) { SetSetting<bool>(value, kSsao); }

void EngineSettings::SetSmaa(bool value) { SetSetting<bool>(value, kSmaa); }

void EngineSettings::SetFullscreen(bool value) {
  SetSetting<bool>(value, kFullscreen);
}

void EngineSettings::SetWindowed(bool value) {
  SetSetting<bool>(value, kWindowed);
}

bool EngineSettings::VSync() const { return Setting<bool>(kVsync); }

bool EngineSettings::Bloom() const { return Setting<bool>(kBloom); }

bool EngineSettings::Ssao() const { return Setting<bool>(kSsao); }

bool EngineSettings::Smaa() const { return Setting<bool>(kSmaa); }

bool EngineSettings::Fullscreen() const { return Setting<bool>(kFullscreen); }

bool EngineSettings::Windowed() const { return Setting<bool>(kWindowed); }

void EngineSettings::SetGamma(float value) { SetSetting<float>(value, kGamma); }

void EngineSettings::SetMasterVolume(float value) {
  SetSetting<float>(value, kMasterVolume);
}

void EngineSettings::SetMusicVolume(float value) {
  SetSetting<float>(value, kMusicVolume);
}

void EngineSettings::SetSoundEffectVolume(float value) {
  SetSetting<float>(value, kSoundEffectVolume);
}

void EngineSettings::SetAmbientVolume(float value) {
  SetSetting<float>(value, kAmbientVolume);
}

void EngineSettings::SetVoiceVolume(float value) {
  SetSetting<float>(value, kVoiceVolume);
}

void EngineSettings::SetSaturation(float value) {
  SetSetting<float>(value, kSaturation);
}

void EngineSettings::SetHue(float value) { SetSetting<float>(value, kHue); }

void EngineSettings::SetBrightness(float value) {
  SetSetting<float>(value, kBrightness);
}

void EngineSettings::SetMouseSensitivity(float value) {
  SetSetting<float>(value, kMouseSensitivity);
}

void EngineSettings::SetMouseSmoothing(float value) {
  SetSetting<float>(value, kMouseSmoothing);
}

void EngineSettings::SetFramePace(float value) {
  SetSetting<float>(value, kFramePace);
}

float EngineSettings::Gamma() const { return Setting<float>(kGamma); }

float EngineSettings::MasterVolume() const {
  return Setting<float>(kMasterVolume);
}

float EngineSettings::MusicVolume() const {
  return Setting<float>(kMusicVolume);
}

float EngineSettings::SoundEffectVolume() const {
  return Setting<float>(kSoundEffectVolume);
}

float EngineSettings::AmbientVolume() const {
  return Setting<float>(kAmbientVolume);
}

float EngineSettings::VoiceVolume() const {
  return Setting<float>(kVoiceVolume);
}

float EngineSettings::Saturation() const { return Setting<float>(kSaturation); }

float EngineSettings::Hue() const { return Setting<float>(kHue); }

float EngineSettings::Brightness() const { return Setting<float>(kBrightness); }

float EngineSettings::MouseSensitivity() const {
  return Setting<float>(kMouseSensitivity);
}

float EngineSettings::MouseSmoothing() const {
  return Setting<float>(kMouseSmoothing);
}

float EngineSettings::FramePace() const {
  return 1.f / Setting<float>(kFramePace);
}

float EngineSettings::FpsTarget() const { return Setting<float>(kFramePace); }

void EngineSettings::LoadSettings() {
  std::ifstream input("./config.ini");
  if (input.fail()) return;

  auto bool_map = &enum_string_map_[typeid(bool).hash_code()];
  auto float_map = &enum_string_map_[typeid(float).hash_code()];
  auto int_map = &enum_string_map_[typeid(int).hash_code()];

  ct::string line, lhs, rhs;
  while (!input.eof()) {
    std::getline(input, line);
    lhs = line.substr(0, line.find_first_of('='));
    rhs = line.substr(line.find_first_of('=') + 1, line.size());
    while (!lhs.empty() && lhs[0] == ' ') lhs = lhs.substr(1, lhs.size());
    while (!rhs.empty() && rhs[0] == ' ') rhs = rhs.substr(1, rhs.size());
    while (!lhs.empty() && lhs.back() == ' ') lhs.pop_back();
    while (!rhs.empty() && rhs.back() == ' ') rhs.pop_back();

    if (lhs.empty() || rhs.empty()) continue;

    auto it = bool_map->find(lhs);
    if (it != bool_map->end()) {
      bool flag;
      ct::stringstream ss;
      ss << rhs, ss >> flag;
      if (!ss.fail()) SetSetting(flag, it->second);
    }

    it = float_map->find(lhs);
    if (it != float_map->end()) {
      float value;
      ct::stringstream ss;
      ss << rhs, ss >> value;
      if (!ss.fail()) SetSetting(value, it->second);
    }

    it = int_map->find(lhs);
    if (it != int_map->end()) {
      int value;
      ct::stringstream ss;
      ss << rhs, ss >> value;
      if (!ss.fail()) SetSetting(value, it->second);
    }
  }
}

void EngineSettings::SaveSettings() {
  auto bool_map = &enum_string_map_rev_[typeid(bool).hash_code()];
  auto float_map = &enum_string_map_rev_[typeid(float).hash_code()];
  auto int_map = &enum_string_map_rev_[typeid(int).hash_code()];

  std::ofstream output("./config.ini");
  auto it = setting_vecs_.find(typeid(bool).hash_code());
  if (it != setting_vecs_.end()) {
    output << "[Bool settings]\n";
    auto bool_settings = &it->second.get_value<ct::dyn_array<bool>>();
    for (int i = 0; i < bool_settings->size(); ++i) {
      auto it = bool_map->find(i);
      if (it != bool_map->end())
        output << it->second << " = " << bool_settings->at(i) << "\n";
    }
  }

  it = setting_vecs_.find(typeid(float).hash_code());
  if (it != setting_vecs_.end()) {
    output << "\n[Float settings]\n";
    auto float_settings = &it->second.get_value<ct::dyn_array<float>>();
    for (int i = 0; i < float_settings->size(); ++i) {
      auto it = float_map->find(i);
      if (it != float_map->end())
        output << it->second << " = " << float_settings->at(i) << "\n";
    }
  }

  it = setting_vecs_.find(typeid(int).hash_code());
  if (it != setting_vecs_.end()) {
    output << "\n[Int settings]\n";
    auto int_settings = &it->second.get_value<ct::dyn_array<int>>();
    for (int i = 0; i < int_settings->size(); ++i) {
      auto it = int_map->find(i);
      if (it != int_map->end())
        output << it->second << " = " << int_settings->at(i) << "\n";
    }
  }
}

void EngineSettings::SetDefaults() {
  SetVSync(true);
  SetBloom(true);
  SetSsao(true);
  SetSmaa(true);
  SetFullscreen(false);
  SetWindowed(true);

  SetGamma(2.2f);
  SetMasterVolume(1.f);
  SetMusicVolume(.5f);
  SetSoundEffectVolume(1.f);
  SetAmbientVolume(1.f);
  SetVoiceVolume(1.f);
  SetSaturation(1.2f);
  SetHue(1.f);
  SetBrightness(1.f);
  SetMouseSensitivity(1.f);
  SetMouseSmoothing(1.f);
  SetFramePace(150.f);

  SetWindowedWidth(1920);
  SetWindowedHeight(1080);
  SetFullscreenWidth(1920);
  SetFullscreenHeight(1080);
  SetMaxShadowResolution(2048);
  SetMaxTextureResolution(2048);
}

template <typename T>
void EngineSettings::SetSetting(T value, int pos) {
  auto hash = typeid(T).hash_code();
  auto it = setting_vecs_.find(hash);
  if (it == setting_vecs_.end()) {
    setting_vecs_[hash] = any_type(ct::dyn_array<T>());
    it = setting_vecs_.find(hash);
  }

  auto &vec = it->second.get_value<ct::dyn_array<T>>();
  while (vec.size() <= pos) vec.push_back(T());
  vec[pos] = value;
}

template <typename T>
T EngineSettings::Setting(int pos) const {
  auto hash = typeid(T).hash_code();
  auto it = setting_vecs_.find(hash);
  if (it == setting_vecs_.end()) return T();

  return it->second.get_value<ct::dyn_array<T>>()[pos];
}

void EngineSettings::SetWindowedWidth(int width) {
  SetSetting<int>(width, kWindowedWidth);
}

void EngineSettings::SetWindowedHeight(int height) {
  SetSetting<int>(height, kWindowedHeight);
}

void EngineSettings::SetFullscreenWidth(int width) {
  SetSetting<int>(width, kFullscreenWidth);
}

void EngineSettings::SetFullscreenHeight(int height) {
  SetSetting<int>(height, kFullscreenHeight);
}

void EngineSettings::SetMaxShadowResolution(int res) {
  SetSetting<int>(res, kMaxShadowResoulution);
}

void EngineSettings::SetMaxTextureResolution(int res) {
  SetSetting<int>(res, kMaxTextureResolution);
}

int EngineSettings::WindowedWidth() const {
  return Setting<int>(kWindowedWidth);
}

int EngineSettings::WindowedHeight() const {
  return Setting<int>(kWindowedHeight);
}

int EngineSettings::FullscreenWidth() const {
  return Setting<int>(kFullscreenWidth);
}

int EngineSettings::FullscreenHeight() const {
  return Setting<int>(kFullscreenHeight);
}

int EngineSettings::MaxShadowTexture() const {
  return Setting<int>(kMaxShadowResoulution);
}

int EngineSettings::MaxTextureResolution() const {
  return Setting<int>(kMaxTextureResolution);
}
}  // namespace lib_core
