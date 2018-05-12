#pragma once
#include "core_utilities.h"

namespace cmd_fract_cooking {
class TextureCooker {
 public:
  TextureCooker() = default;
  ~TextureCooker() = default;

  void LoadTexture(ct::string path);
  void SerializeAndSave(ct::string save_path);

 protected:
 private:
  struct Texture {
    ct::string name;
    short channels;
    std::pair<int, int> dims;
    ct::dyn_array<uint8_t> data;
  };

  tbb::concurrent_vector<Texture> textures_;
};
}  // namespace cmd_fract_cooking
