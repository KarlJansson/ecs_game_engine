#pragma once
#include "model_cooker.h"

namespace cmd_fract_cooking {
TEST(cmd_fract_cooking, ModelCooker_testcase) {
  auto model_cooker = std::make_unique<ModelCooker>();
  model_cooker->LoadModel("D:/Projects/Models/exported/corridor/monkey.fbx");
  model_cooker->SerializeAndSave("./test_modelpack");
}
}  // namespace cmd_fract_cooking
