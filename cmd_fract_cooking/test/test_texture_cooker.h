#pragma once
#include "texture_cooker.h"

namespace cmd_fract_cooking {
TEST(cmd_fract_cooking, TextureCooker_testcase) {
  auto tex_cooker = std::make_unique<TextureCooker>();
  tex_cooker->LoadTexture("D:/Projects/Textures/particles/poly_particle.png");
  tex_cooker->SerializeAndSave("./test_texpack");
}
}  // namespace cmd_fract_cooking
