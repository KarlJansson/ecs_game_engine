#pragma once
#include "engine_core.h"

namespace slib_core {
/*ct::dyn_array<bool> bit_array(100'000'000, false);
ct::dyn_array<uint8_t> byte_array(100'000'000, false);
ct::dyn_array<int> int_array(100'000'000, 0);

TEST(slib_core, EngineCore_byteiteraterange) {
  byte_array[90'000'000] = true;
  int count = 0;
  for (auto b : byte_array)
    if (b) ++count;

  printf("Count: %i\n", count);
}

TEST(slib_core, EngineCore_byteiterate) {
  byte_array[90'000'000] = true;
  int count = 0;
  for (size_t i = 0; i < 100'000'000; ++i)
    if (byte_array[i]) ++count;

  printf("Count: %i\n", count);
}

TEST(slib_core, EngineCore_bytefind) {
  std::find(byte_array.begin(), byte_array.end(), true);
}

TEST(slib_core, EngineCore_bititerate) {
  bit_array[90'000'000] = true;
  int count = 0;
  for (auto b : bit_array)
    if (b) ++count;

  printf("Count: %i\n", count);
}

TEST(slib_core, EngineCore_bitfind) {
  std::find(bit_array.begin(), bit_array.end(), true);
}

TEST(slib_core, EngineCore_intiterate) {
  int_array[90'000'000] = 1;
  int count = 0;
  for (auto b : bit_array)
    if (b) ++count;

  printf("Count: %i\n", count);
}

TEST(slib_core, EngineCore_intfind) {
  std::find(int_array.begin(), int_array.end(), 1);
}*/
}  // namespace slib_core
