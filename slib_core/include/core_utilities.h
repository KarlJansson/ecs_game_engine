#pragma once
#include <tbb/tbb.h>
#include <cassert>
#include <chrono>
#include <cmath>
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#define PI 3.14159265359f
#define PI2 PI + PI
#define PI_HALF PI / 2.f

using namespace std::chrono_literals;

// Engine types
namespace ct {
template <typename T>
using dyn_array = std::vector<T>;
template <typename T>
using de_queue = std::deque<T>;

template <typename K, typename V, class comp = std::less<K>>
using tree_map = std::map<K, V, comp>;
template <typename K, typename V>
using hash_map = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>>;
template <typename T, class comp = std::less<T>>
using tree_set = std::set<T, comp>;
template <typename T>
using hash_set = std::unordered_set<T, std::hash<T>, std::equal_to<T>>;
using string = std::string;
using stringstream = std::stringstream;
}  // namespace ct

// Common operations
namespace co {
template <typename T>
static T clamp(const T &min, const T &max, const T &value) {
  if (value < min) return min;
  if (value > max) return max;
  return value;
};

template <typename T>
static T smooth_step_fn(T t) {
  return (t * t * (T(3.) - T(2.) * t));
};

template <typename T>
static T ease_in_sine_fn(T t) {
  return T(1.) - std::cos(t * PI_HALF);
};

template <typename T>
static T sine_fn(T t) {
  return (std::sin(t * PI2 - PI_HALF) + T(1.)) / T(2.);
};

template <typename T>
static T ease_out_sine_fn(T t) {
  return sin(t * PI_HALF);
};

template <typename T>
static T ease_in_out_sine_fn(T t) {
  return T(-0.5) * (std::cos(PI * t) - T(1.));
};

template <typename T>
static T ease_in_back_fn(T t) {
  auto s = T(1.70158);
  return t * t * ((s + T(1.)) * t - s);
};

template <typename T>
static T ease_out_back_fn(T t) {
  T s = T(1.70158);
  --t;
  return (t * t * ((s + T(1.)) * t + s) + T(1.));
};

template <typename T>
static T ease_in_out_back_fn(T t) {
  T s = T(1.70158) * T(1.525);
  t *= T(2.);
  if (t < T(1.)) return T(1.) / T(2.) * (t * t * ((s + T(1)) * t - s));
  T post_fix = t -= T(2.);
  return T(1.) / T(2.) * ((post_fix)*t * ((s + T(1.)) * t + s) + T(2.));
};

template <typename T>
static T lerp(const T &a, const T &b, float factor) {
  auto fact_clamp = clamp(0.f, 1.f, factor);
  return a * (1.f - fact_clamp) + b * fact_clamp;
};
}  // namespace co

class cu {
 public:
  static void LoadAndDecompress(const ct::string &load_path,
                                ct::dyn_array<uint8_t> &out_data);
  static void DecompressMemory(ct::dyn_array<uint8_t> &in_data,
                               ct::dyn_array<uint8_t> &out_data);
  static void CompressAndSave(const ct::string &save_path,
                              ct::dyn_array<uint8_t> &buffer);
  static void Save(const ct::string &save_path, ct::dyn_array<uint8_t> &buffer);
  static void CompressMemory(ct::dyn_array<uint8_t> &in_data,
                             ct::dyn_array<uint8_t> &out_data);

  static bool ScrollCursor(ct::string &buffer, size_t &cursor, char stop_char);
  static ct::string CaptureToken(ct::string &buffer, size_t &cursor,
                                 char stop_char);
  static ct::string ParseType(ct::string &buffer, size_t &cursor);
  static ct::string ParseValue(ct::string &buffer, size_t &cursor,
                               bool scroll = true);
  static ct::dyn_array<ct::string> SplitString(ct::string &str,
                                               ct::dyn_array<char> &&splitters);
  static bool EvalExpr(ct::string &str);

  template <typename T>
  static T Parse(const ct::string &val, T result = T()) {
    std::stringstream ss;
    ss << val, ss >> result;
    return result;
  }

  template <typename T>
  static ct::dyn_array<T> ParseArray(const ct::string &val) {
    ct::dyn_array<T> result;
    size_t start = 0, end = 0;
    while ((end = val.find_first_of(',', start)) != std::string::npos) {
      result.emplace_back(Parse<T>(val.substr(start, end - start)));
      start = end + 1;
    }
    result.emplace_back(Parse<T>(val.substr(start, val.size())));

    return result;
  }

  template <typename T, size_t size>
  static ct::dyn_array<T> ParseVectorArray(ct::string val) {
    ct::dyn_array<T> result;
    while (!val.empty()) result.emplace_back(ParseVector<T, size>(val));
    return result;
  }

  template <typename T>
  static ct::dyn_array<T> ParsePairArray(const ct::string &val) {
    ct::dyn_array<T> result;

    T pair;
    size_t start = 0, end = 0;
    while (start < val.size()) {
      end = val.find_first_of(',', start);
      if (end == ct::string::npos) end = val.size();

      pair.first = Parse<int>(val.substr(start, end - start));
      start = end + 1;

      end = val.find_first_of(',', start);
      if (end == ct::string::npos) end = val.size();

      pair.second = Parse<int>(val.substr(start, end - start));
      start = end + 1;

      result.push_back(pair);
    }

    return result;
  }

  template <typename T, size_t size>
  static T ParseVector(ct::string val) {
    T result;
    if (val.find(',') == std::string::npos)
      result = Parse<float>(val);
    else {
      size_t start = 0, end = 0;
      for (int i = 0; i < size; ++i) {
        end = val.find_first_of(',', start);
        if (end == ct::string::npos) end = val.size();

        result[i] = Parse<float>(val.substr(start, end - start));
        start = end + 1;
      }
      if (start < val.size())
        val = val.substr(start, val.size());
      else
        val = "";
    }

    return result;
  }

  static void PrintLogFile(const ct::string &dest_path);
  static void PrintProfiling(const ct::string &dest_path);

  class TerminatingException : public std::exception {
   public:
    explicit TerminatingException(std::string reason)
        : reason_(std::move(reason)) {}
    [[nodiscard]] const char *what() const noexcept override {
      return reason_.c_str();
    }

   private:
    std::string reason_;
  };

  static inline void AssertError(bool condition, const ct::string &cause,
                                 const ct::string &file = "", int line = -1) {
    if (!condition) {
      ct::string desc_str = file;
#ifdef WindowsBuild
      auto pos = file.find_last_of('\\');
#else
      auto pos = file.find_last_of('/');
#endif
      if (pos != std::string::npos)
        desc_str = file.substr(pos + 1, file.size());
      if (line != -1) desc_str += "(" + std::to_string(line) + ") ";

      errors.push_back(desc_str + cause);
      assert(condition);
      PrintLogFile("./logfile.txt");
      try {
        throw TerminatingException(desc_str + cause);
      } catch (...) {
        std::terminate();
      }
    }
  }

  static inline void AssertWarning(bool condition, const ct::string &cause,
                                   const ct::string &file = "", int line = -1) {
    if (!condition) {
      ct::string desc_str;
      if (!file.empty())
        desc_str += file.substr(file.find_last_of('\\') + 1, file.size());
      if (line != -1) desc_str += "(" + std::to_string(line) + ") ";

      std::cout << desc_str + cause << "\n";
      assert(condition);

      warnings.push_back(desc_str + cause);
    }
  }

  static inline void Log(const ct::string &message, const ct::string &file = "",
                         int line = -1) {
    ct::string desc_str;
    if (!file.empty())
      desc_str += file.substr(file.find_last_of('\\') + 1, file.size());
    if (line != -1) desc_str += "(" + std::to_string(line) + ") ";
    logs.push_back(desc_str + message);
  }

  static inline void ProfileStart(const ct::string &name) {
    auto hash = std::hash<ct::string>{}(name);
    str_map.insert({hash, name});
    profile_starts.insert({hash, std::chrono::high_resolution_clock::now()});
  }
  static inline void ProfileStop(const ct::string &name) {
    profile_ends.insert({std::hash<ct::string>{}(name),
                         std::chrono::high_resolution_clock::now()});
  }

  template <class T>
  static inline float ProfileTime(const ct::string &name) {
    auto hash = std::hash<ct::string>{}(name);
    decltype(profile_starts)::accessor it1, it2;
    if (profile_starts.find(it1, hash) && profile_ends.find(it2, hash)) {
      std::chrono::duration<float, T> dur;
      dur = it2->second - it1->second;
      return dur.count();
    }
    return 0.f;
  }

  static inline std::chrono::time_point<std::chrono::high_resolution_clock>
  TimerStart() {
    return std::chrono::high_resolution_clock::now();
  }
  template <class T>
  static inline float TimerStop(
      std::chrono::time_point<std::chrono::high_resolution_clock> &time) {
    std::chrono::duration<float, T> dur;
    dur = std::chrono::high_resolution_clock::now() - time;
    return dur.count();
  }

 private:
  static tbb::concurrent_vector<ct::string> errors, warnings, logs;
  static tbb::concurrent_hash_map<
      size_t, std::chrono::time_point<std::chrono::high_resolution_clock>>
      profile_starts, profile_ends;
  static tbb::concurrent_hash_map<size_t, ct::string> str_map;
  static bool EvalDiv(ct::string &str);
  static bool EvalMul(ct::string &str);
  static bool EvalAdd(ct::string &str);
  static bool EvalSub(ct::string &str);

  static bool EvalAny(ct::string &str, ct::dyn_array<ct::string> &parts,
                      const std::function<float(float, float)> &ops);

  cu() = default;
  ~cu() = default;
};
