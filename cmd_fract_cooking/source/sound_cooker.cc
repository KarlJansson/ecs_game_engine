#include "sound_cooker.h"
#include <fstream>
#include <iostream>
#include "../../source_shared/include/serialization_utilities.hpp"
#include "core_utilities.h"

namespace cmd_fract_cooking {
void SoundCooker::LoadSound(ct::string path) {
  if (path.find(".wav") != ct::string::npos) LoadWaveFile(path);
}

void SoundCooker::SerializeAndSave(ct::string save_path) {
  std::ofstream list_output(save_path + "_names.txt");

  size_t size = 0;
  SerializationUtilities::CountSize(sound_vector_.size(), size);
  for (auto &sound : sound_vector_) {
    SerializationUtilities::CountSize(sound.path, size);
    SerializationUtilities::CountSize(sound.desc, size);
    list_output << sound.path.substr(sound.path.find_last_of('\\') + 1,
                                     sound.path.size())
                << "\n";
  }
  list_output.close();

  for (auto &sound : sound_vector_) {
    sound.desc.data_location = size;
    SerializationUtilities::CountSize(sound.data, size);
  }

  ct::dyn_array<uint8_t> buffer(size);
  auto it = buffer.begin();
  SerializationUtilities::CopyToBuffer(sound_vector_.size(), it);
  for (auto &sound : sound_vector_) {
    SerializationUtilities::CopyToBuffer(sound.path, it);
    SerializationUtilities::CopyToBuffer(sound.desc, it);
  }
  for (auto &sound : sound_vector_)
    SerializationUtilities::CopyToBuffer(sound.data, it);

  cu::Save(save_path, buffer);
}

void SoundCooker::LoadWaveFile(ct::string &path) {
  std::ifstream open(path, std::ios::binary);

  Sound sound;
  sound.path = path;
  ct::dyn_array<uint8_t> uncompressed_data;
  if (!open.fail()) {
    ct::string id("    ");

    uint32_t size;
    uint32_t data_size;
    uint16_t format_tag, block_align;
    uint32_t format_length, avg_bytes_sec;

    open.read((char *)id.data(), 4);
    if (id.find("RIFF") != ct::string::npos) {
      open.read((char *)&size, sizeof(size));

      open.read((char *)id.data(), 4);
      if (id.find("WAVE") != ct::string::npos) {
        open.read((char *)id.data(), 4);

        open.read((char *)&format_length, sizeof(format_length));
        open.read((char *)&format_tag, sizeof(format_tag));

        open.read((char *)&sound.desc.channels, sizeof(sound.desc.channels));

        open.read((char *)&sound.desc.sample_rate,
                  sizeof(sound.desc.sample_rate));
        open.read((char *)&avg_bytes_sec, sizeof(avg_bytes_sec));

        open.read((char *)&block_align, sizeof(block_align));
        open.read((char *)&sound.desc.bits_per_sample,
                  sizeof(sound.desc.bits_per_sample));

        open.read((char *)id.data(), 4);
        open.read((char *)&data_size, sizeof(data_size));
        uncompressed_data.assign(data_size, 0);

        open.read((char *)uncompressed_data.data(), data_size);

        cu::CompressMemory(uncompressed_data, sound.data);
      } else
        std::cout << "Error: RIFF file but not a wave file\n";
    } else
      std::cout << "Error: not a RIFF file\n";
  }

  sound_vector_.push_back(std::move(sound));
}
}  // namespace cmd_fract_cooking
