#pragma once
#include "sound_cooker.h"

namespace cmd_fract_cooking {
TEST(cmd_fract_cooking, SoundCooker_testcase) {
  auto sound_cooker = std::make_unique<SoundCooker>();
  sound_cooker->LoadSound("D:/Projects/Sound/sound_bank/test_audio.wav");
  sound_cooker->SerializeAndSave("./test_soundpack");
}
}  // namespace cmd_fract_cooking
