#pragma once
#include "core_utilities.h"

namespace cmd_fract_cooking {
class SoundCooker {
 public:
  SoundCooker() = default;
  ~SoundCooker() = default;

  void LoadSound(ct::string path);
  void SerializeAndSave(ct::string save_path);

 private:
  struct SoundDesc {
    uint16_t channels;
    uint16_t bits_per_sample;
    uint32_t sample_rate;
    size_t data_location;
  };

  struct Sound {
    ct::string path;
    SoundDesc desc;
    ct::dyn_array<uint8_t> data;
  };

  void LoadWaveFile(ct::string& path);

  tbb::concurrent_vector<Sound> sound_vector_;
};
}  // namespace cmd_fract_cooking
