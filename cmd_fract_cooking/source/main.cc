#include <execution>
#include <filesystem>
#include <iostream>
#include <sstream>
#include "core_utilities.h"
#include "model_cooker.h"
#include "sound_cooker.h"
#include "texture_cooker.h"

namespace cmd_fract_cooking {}
using namespace cmd_fract_cooking;

int main(int argc, char** argv) {
  int asset_id = 0;
  ct::string out_path = "./";
  ct::dyn_array<ct::string> assets[6];

  for (int i = 1; i < argc; ++i) {
    if (ct::string(argv[i]).compare("t") == 0)
      asset_id = 0;
    else if (ct::string(argv[i]).compare("td") == 0)
      asset_id = 2;
    else if (ct::string(argv[i]).compare("m") == 0)
      asset_id = 1;
    else if (ct::string(argv[i]).compare("md") == 0)
      asset_id = 3;
    else if (ct::string(argv[i]).compare("s") == 0)
      asset_id = 4;
    else if (ct::string(argv[i]).compare("sd") == 0)
      asset_id = 5;
    else if (ct::string(argv[i]).compare("o") == 0)
      asset_id = 6;
    else if (asset_id == 6)
      out_path = argv[i];
    else
      assets[asset_id].push_back(argv[i]);
  }

  ct::tree_set<ct::string> tex_formats, model_foramts, sound_formats;
  tex_formats.insert(".png");
  tex_formats.insert(".tga");
  tex_formats.insert(".jpg");
  tex_formats.insert(".dds");

  model_foramts.insert(".obj");
  model_foramts.insert(".dae");
  model_foramts.insert(".fbx");

  sound_formats.insert(".wav");

  auto tex_cooker = std::make_unique<TextureCooker>();
  auto mod_cooker = std::make_unique<ModelCooker>();
  auto sound_cooker = std::make_unique<SoundCooker>();
  ct::dyn_array<ct::string> textures, models, sounds;

  for (auto& path : assets[0]) {
    if (tex_formats.find(path.substr(path.find_last_of('.'), path.size())) !=
        tex_formats.end())
      textures.push_back(path);
    else
      std::cout << "Unsupported texture format: " << path << "\n";
  }
  for (auto& path : assets[1]) {
    if (model_foramts.find(path.substr(path.find_last_of('.'), path.size())) !=
        model_foramts.end())
      models.push_back(path);
    else
      std::cout << "Unsupported model format: " << path << "\n";
  }
  for (auto& path : assets[4]) {
    if (sound_formats.find(path.substr(path.find_last_of('.'), path.size())) !=
        sound_formats.end())
      sounds.push_back(path);
    else
      std::cout << "Unsupported sound format: " << path << "\n";
  }
  for (auto& path : assets[2]) {
    for (auto& file : std::filesystem::recursive_directory_iterator(path)) {
      if (std::filesystem::is_regular_file(file)) {
        ct::stringstream path_str_stream;
        path_str_stream << file;
        ct::string path_str = path_str_stream.str();
        while (!path_str.empty() && path_str[0] == '"')
          path_str = path_str.substr(1, path_str.size());
        while (!path_str.empty() && path_str.back() == '"') path_str.pop_back();

        if (tex_formats.find(
                path_str.substr(path_str.find_last_of('.'), path_str.size())) !=
            tex_formats.end())
          textures.push_back(path_str);
        else
          std::cout << "Unsupported texture format: " << path_str << "\n";
      }
    }
  }
  for (auto& path : assets[3]) {
    for (auto& file : std::filesystem::recursive_directory_iterator(path)) {
      if (std::filesystem::is_regular_file(file)) {
        ct::stringstream path_str_stream;
        path_str_stream << file;
        ct::string path_str = path_str_stream.str();
        while (!path_str.empty() && path_str[0] == '"')
          path_str = path_str.substr(1, path_str.size());
        while (!path_str.empty() && path_str.back() == '"') path_str.pop_back();

        if (model_foramts.find(
                path_str.substr(path_str.find_last_of('.'), path_str.size())) !=
            model_foramts.end())
          models.push_back(path_str);
        else
          std::cout << "Unsupported model format: " << path_str << "\n";
      }
    }
  }
  for (auto& path : assets[5]) {
    for (auto& file : std::filesystem::recursive_directory_iterator(path)) {
      if (std::filesystem::is_regular_file(file)) {
        ct::stringstream path_str_stream;
        path_str_stream << file;
        ct::string path_str = path_str_stream.str();
        while (!path_str.empty() && path_str[0] == '"')
          path_str = path_str.substr(1, path_str.size());
        while (!path_str.empty() && path_str.back() == '"') path_str.pop_back();

        if (sound_formats.find(
                path_str.substr(path_str.find_last_of('.'), path_str.size())) !=
            sound_formats.end())
          sounds.push_back(path_str);
        else
          std::cout << "Unsupported sound format: " << path_str << "\n";
      }
    }
  }

  auto tex_thread = [&](auto& tex) { tex_cooker->LoadTexture(tex); };
  if (!textures.empty())
    std::for_each(std::execution::par_unseq, std::begin(textures),
                  std::end(textures), tex_thread);

  auto model_thread = [&](auto& model) { mod_cooker->LoadModel(model); };
  if (!models.empty())
    std::for_each(std::execution::par_unseq, std::begin(models),
                  std::end(models), model_thread);

  // for (auto& model : models) mod_cooker->LoadModel(model);

  auto sound_thread = [&](auto& sound) { sound_cooker->LoadSound(sound); };
  if (!sounds.empty())
    std::for_each(std::execution::par_unseq, std::begin(sounds),
                  std::end(sounds), sound_thread);

  if (!assets[0].empty() || !assets[2].empty())
    tex_cooker->SerializeAndSave(out_path + "_texpack");
  if (!assets[1].empty() || !assets[3].empty())
    mod_cooker->SerializeAndSave(out_path + "_modelpack");
  if (!assets[4].empty() || !assets[5].empty())
    sound_cooker->SerializeAndSave(out_path + "_soundpack");

  return 1;
}
