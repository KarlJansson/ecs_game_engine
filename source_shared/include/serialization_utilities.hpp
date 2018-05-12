#pragma once
#include "core_utilities.h"

class SerializationUtilities {
 public:
  template <typename T>
  static inline void CopyToBuffer(T val, ct::dyn_array<uint8_t>::iterator& it) {
    std::copy(reinterpret_cast<uint8_t*>(&val),
              reinterpret_cast<uint8_t*>(&val) + sizeof(val), it);
    it += sizeof(val);
  }

  template <typename T>
  static inline void CopyToBuffer(ct::dyn_array<T>& vec,
                                  ct::dyn_array<uint8_t>::iterator& it) {
    CopyToBuffer(vec.size(), it);
    std::copy(reinterpret_cast<uint8_t*>(vec.data()),
              reinterpret_cast<uint8_t*>(vec.data()) + sizeof(T) * vec.size(),
              it);
    it += sizeof(T) * vec.size();
  }

  static inline void CopyToBuffer(std::string& str,
                                  ct::dyn_array<uint8_t>::iterator& it) {
    CopyToBuffer(str.size(), it);
    std::copy(str.begin(), str.end(), it);
    it += sizeof(str[0]) * str.size();
  }

  template <typename T>
  static inline void ReadFromBuffer(ct::dyn_array<uint8_t>::iterator& it,
                                    T& out) {
    std::copy(it, it + sizeof(T), reinterpret_cast<uint8_t*>(&out));
    it += sizeof(T);
  }

  template <typename T>
  static inline void ReadFromBuffer(ct::dyn_array<uint8_t>::iterator& it,
                                    ct::dyn_array<T>& out) {
    size_t size;
    ReadFromBuffer(it, size);
    out.assign(size, T());
    std::copy(it, it + sizeof(T) * size,
              reinterpret_cast<uint8_t*>(out.data()));
    it += sizeof(T) * size;
  }

  static inline void ReadFromBuffer(ct::dyn_array<uint8_t>::iterator& it,
                                    std::string& out) {
    size_t size;
    ReadFromBuffer(it, size);
    out.assign(size, 0);
    std::copy(it, it + sizeof(out[0]) * size,
              reinterpret_cast<uint8_t*>(&out[0]));
    it += sizeof(out[0]) * size;
  }

  template <typename T>
  static inline void CountSize(T val, size_t& size) {
    size += sizeof(val);
  }

  template <typename T>
  static inline void CountSize(ct::dyn_array<T>& vec, size_t& size) {
    CountSize(vec.size(), size);
    size += sizeof(T) * vec.size();
  }

  static inline void CountSize(std::string& str, size_t& size) {
    CountSize(str.size(), size);
    size += sizeof(str[0]) * str.size();
  }

 protected:
 private:
  SerializationUtilities() = default;
  ~SerializationUtilities() = default;
};
