#include "texture_cooker.h"
#include <experimental/filesystem>
#include <fstream>
#include "../../source_shared/include/serialization_utilities.hpp"
#include "core_utilities.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../source_shared/include/stb_image.hpp"

namespace cmd_fract_cooking {
void TextureCooker::LoadTexture(ct::string path) {
  if (!std::experimental::filesystem::exists(path)) return;

  Texture tex;
  tex.name = path.substr(path.find_last_of('/') + 1, path.size());
  int width, height, channels = 0;

  auto image = stbi_load(path.c_str(), &width, &height, &channels, 0);

  tex.channels = channels;
  tex.dims = {width, height};
  ct::dyn_array<uint8_t> tex_source(width * height * channels);
  std::copy(image, image + sizeof(uint8_t) * width * height * channels,
            tex_source.begin());

  stbi_image_free(image);

  cu::CompressMemory(tex_source, tex.data);
  textures_.push_back(tex);
}

void TextureCooker::SerializeAndSave(ct::string save_path) {
  std::ofstream list_output(save_path + "_names.txt");

  size_t size = 0;
  ct::dyn_array<size_t> starting_points;
  SerializationUtilities::CountSize(textures_.size(), size);
  for (auto &tex : textures_) {
    SerializationUtilities::CountSize(tex.name, size);
    SerializationUtilities::CountSize(tex.channels, size);
    SerializationUtilities::CountSize(tex.dims, size);
    SerializationUtilities::CountSize(size, size);

    if (tex.name.find("albedo") != ct::string::npos)
      list_output << tex.name.substr(tex.name.find_last_of('\\') + 1,
                                     tex.name.size())
                  << "\n";
  }
  list_output.close();

  for (auto &tex : textures_) {
    starting_points.push_back(size);
    SerializationUtilities::CountSize(tex.data, size);
  }

  int i = 0;
  ct::dyn_array<uint8_t> buffer(size);
  auto it = buffer.begin();
  SerializationUtilities::CopyToBuffer(textures_.size(), it);
  for (auto &tex : textures_) {
    SerializationUtilities::CopyToBuffer(tex.name, it);
    SerializationUtilities::CopyToBuffer(tex.channels, it);
    SerializationUtilities::CopyToBuffer(tex.dims, it);
    SerializationUtilities::CopyToBuffer(starting_points[i++], it);
  }
  for (auto &tex : textures_)
    SerializationUtilities::CopyToBuffer(tex.data, it);

  cu::Save(save_path, buffer);
}
}  // namespace cmd_fract_cooking
