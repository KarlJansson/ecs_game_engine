#include "core_utilities.h"
#include <fstream>
#include <regex>
#ifdef UnixBuild
#include <zlib.h>
#elif WindowsBuild
#include <zlib/zlib.h>
#endif

tbb::concurrent_hash_map<size_t, ct::string> cu::str_map;
tbb::concurrent_vector<ct::string> cu::errors, cu::warnings, cu::logs;
tbb::concurrent_hash_map<
    size_t, std::chrono::time_point<std::chrono::high_resolution_clock>>
    cu::profile_starts, cu::profile_ends;

ct::string cu::ReadFile(const ct::string &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  size_t size = file.tellg();
  file.seekg(0, std::ios::beg);

  ct::string content(size, ' ');
  file.read(content.data(), size);
  file.close();

  return content;
}

void cu::LoadAndDecompress(const ct::string &load_path,
                           ct::dyn_array<uint8_t> &out_data) {
  std::ifstream open(load_path, std::ios::binary | std::ios::ate);
  if (open.fail()) return;

  std::streamsize size = open.tellg();
  open.seekg(0, std::ios::beg);
  ct::dyn_array<uint8_t> compressed(size);
  open.read(reinterpret_cast<char *>(compressed.data()), size);
  open.close();
  DecompressMemory(compressed, out_data);
}

void cu::DecompressMemory(ct::dyn_array<uint8_t> &in_data,
                          ct::dyn_array<uint8_t> &out_data) {
  ct::dyn_array<uint8_t> buffer;
  buffer.reserve(in_data.size() * 2);
  const size_t BUFFSIZE = 128 * 1024;
  ct::dyn_array<uint8_t> temp_buffer(BUFFSIZE);

  z_stream strm;
  strm.opaque = nullptr;
  strm.zalloc = nullptr;
  strm.zfree = nullptr;
  strm.next_in = in_data.data();
  strm.avail_in = uInt(in_data.size());
  strm.next_out = temp_buffer.data();
  strm.avail_out = BUFFSIZE;

  inflateInit(&strm);

  int res = Z_OK;
  while (res != Z_STREAM_END) {
    res = inflate(&strm, Z_NO_FLUSH);
    if (strm.avail_out == 0) {
      buffer.insert(buffer.end(), temp_buffer.begin(), temp_buffer.end());
      strm.next_out = temp_buffer.data();
      strm.avail_out = BUFFSIZE;
    }
    cu::AssertError(res > -1, "zlib inflation error: " + std::to_string(res),
                    __FILE__, __LINE__);
  }

  buffer.insert(buffer.end(), temp_buffer.begin(),
                temp_buffer.end() - strm.avail_out);
  inflateEnd(&strm);

  buffer.shrink_to_fit();
  out_data.swap(buffer);
}

void cu::CompressAndSave(const ct::string &save_path,
                         ct::dyn_array<uint8_t> &buffer) {
  ct::dyn_array<uint8_t> compressed;
  CompressMemory(buffer, compressed);

  std::ofstream open(save_path, std::ios::binary);
  open.write(reinterpret_cast<char *>(compressed.data()), compressed.size());
  open.close();
}

void cu::Save(const ct::string &save_path, ct::dyn_array<uint8_t> &buffer) {
  std::ofstream open(save_path, std::ios::binary);
  open.write(reinterpret_cast<char *>(buffer.data()), buffer.size());
  open.close();
}

void cu::CompressMemory(ct::dyn_array<uint8_t> &in_data,
                        ct::dyn_array<uint8_t> &out_data) {
  ct::dyn_array<uint8_t> buffer;
  buffer.reserve(in_data.size());
  const size_t BUFFSIZE = 128 * 1024;
  ct::dyn_array<uint8_t> temp_buffer(BUFFSIZE);

  z_stream strm;
  strm.zalloc = nullptr;
  strm.zfree = nullptr;
  strm.next_in = in_data.data();
  strm.avail_in = uInt(in_data.size());
  strm.next_out = temp_buffer.data();
  strm.avail_out = BUFFSIZE;

  deflateInit(&strm, Z_BEST_COMPRESSION);

  while (strm.avail_in != 0) {
    deflate(&strm, Z_NO_FLUSH);
    if (strm.avail_out == 0) {
      buffer.insert(buffer.end(), temp_buffer.begin(), temp_buffer.end());
      strm.next_out = temp_buffer.data();
      strm.avail_out = BUFFSIZE;
    }
  }

  int deflate_res = Z_OK;
  while (deflate_res == Z_OK) {
    if (strm.avail_out == 0) {
      buffer.insert(buffer.end(), temp_buffer.begin(), temp_buffer.end());
      strm.next_out = temp_buffer.data();
      strm.avail_out = BUFFSIZE;
    }
    deflate_res = deflate(&strm, Z_FINISH);
  }

  buffer.insert(buffer.end(), temp_buffer.begin(),
                temp_buffer.end() - strm.avail_out);
  deflateEnd(&strm);

  buffer.shrink_to_fit();
  out_data.swap(buffer);
}

bool cu::ScrollCursor(ct::string &buffer, size_t &cursor, char stop_char) {
  while (cursor < buffer.size() && buffer[cursor] != '}' &&
         buffer[cursor++] != stop_char) {
  }
  if (cursor < buffer.size() && buffer[cursor] != '}') return true;
  ++cursor;
  return false;
}

ct::string cu::CaptureToken(ct::string &buffer, size_t &cursor,
                            char stop_char) {
  int scopes = 0;
  ct::string type_token;
  while (cursor < buffer.size() &&
         (buffer[cursor] != stop_char || scopes > 0)) {
    if (buffer[cursor] == '{')
      ++scopes;
    else if (buffer[cursor] == '}')
      --scopes;
    type_token += buffer[cursor++];
  }
  ++cursor;
  return type_token;
}

ct::string cu::ParseType(ct::string &buffer, size_t &cursor) {
  if (ScrollCursor(buffer, cursor, '('))
    return CaptureToken(buffer, cursor, ')');
  return "";
}

ct::string cu::ParseValue(ct::string &buffer, size_t &cursor, bool scroll) {
  auto parse = scroll ? ScrollCursor(buffer, cursor, '{') : true;
  if (parse) {
    auto val_token = CaptureToken(buffer, cursor, '}');
    auto valid_expr = true;
    auto token_parts = SplitString(val_token, {','});
    for (auto &str : token_parts)
      if (!EvalExpr(str)) valid_expr = false;
    if (valid_expr) {
      val_token = token_parts[0];
      token_parts.erase(token_parts.begin());
      for (auto &str : token_parts) val_token += "," + str;
    }
    return val_token;
  }
  return "";
}

ct::dyn_array<ct::string> cu::SplitString(ct::string &str,
                                          ct::dyn_array<char> &&splitters) {
  ct::dyn_array<ct::string> splits = {""};
  for (auto c : str) {
    if (std::find(std::begin(splitters), std::end(splitters), c) !=
        std::end(splitters))
      splits.push_back("");
    else
      splits.back().push_back(c);
  }

  return splits;
}

bool cu::EvalExpr(ct::string &str) {
  size_t pos = 0;
  while ((pos = str.find_first_of('(', pos)) != ct::string::npos) {
    size_t matching = 0;
    size_t pos2 = pos + 1;
    while (str.size() > pos2 && (str[pos2] != ')' || matching > 0)) {
      if (str[pos2] == '(')
        ++matching;
      else if (str[pos2] == ')')
        --matching;
      ++pos2;
    }

    ct::string sub_expr = str.substr(pos + 1, (pos2 - pos) - 1);
    ct::string check = sub_expr;
    if (!EvalExpr(sub_expr)) return false;

    str = str.erase(pos, pos2 - pos + 1);
    str.insert(pos, sub_expr);
    pos = pos2 + (sub_expr.size() - check.size());
  }

  if (!EvalAdd(str)) return false;
  float check;
  std::stringstream sscheck;
  sscheck << str, sscheck >> check;
  return sscheck.fail() ? false : true;
}

bool cu::EvalDiv(ct::string &str) {
  str = std::regex_replace(str, std::regex("//"), "");
  auto token_parts = SplitString(str, {'/'});
  for (auto &s : token_parts)
    if (!EvalMul(s)) return false;
  if (token_parts.size() > 1) {
    if (!EvalAny(str, token_parts,
                 [](float lhs, float rhs) -> float { return lhs / rhs; }))
      return false;
  } else
    str = std::move(token_parts.back());
  return true;
}

bool cu::EvalMul(ct::string &str) {
  auto token_parts = SplitString(str, {'*'});
  if (token_parts.size() > 1) {
    if (!EvalAny(str, token_parts,
                 [](float lhs, float rhs) -> float { return lhs * rhs; }))
      return false;
  } else
    str = std::move(token_parts.back());
  return true;
}

bool cu::EvalAdd(ct::string &str) {
  auto token_parts = SplitString(str, {'+'});
  for (auto &s : token_parts)
    if (!EvalSub(s)) return false;
  if (token_parts.size() > 1) {
    if (!EvalAny(str, token_parts,
                 [](float lhs, float rhs) -> float { return lhs + rhs; }))
      return false;
  } else
    str = std::move(token_parts.back());
  return true;
}

bool cu::EvalSub(ct::string &str) {
  auto token_parts = SplitString(str, {'-'});
  bool add_minus = false;
  if (token_parts.size() > 1) {
    for (auto &s : token_parts) {
      if (!s.empty() && add_minus) s.insert(0, "-");
      add_minus = s.empty() ? true : false;
    }
    for (size_t i = token_parts.size(); i > 0; --i)
      if (token_parts[i - 1].empty())
        token_parts.erase(token_parts.begin() + (i - 1));
  }

  for (auto &s : token_parts)
    if (!EvalDiv(s)) return false;
  if (token_parts.size() > 1) {
    if (!EvalAny(str, token_parts,
                 [](float lhs, float rhs) -> float { return lhs - rhs; }))
      return false;
  } else
    str = std::move(token_parts.back());
  return true;
}

bool cu::EvalAny(ct::string &str, ct::dyn_array<ct::string> &parts,
                 const std::function<float(float, float)> &ops) {
  str = parts[0];
  parts.erase(parts.begin());
  for (auto &s : parts) {
    float lhs, rhs;
    std::stringstream sslhs, ssrhs;
    sslhs << str, sslhs >> lhs;
    if (sslhs.fail()) return false;
    ssrhs << s, ssrhs >> rhs;
    if (ssrhs.fail()) return false;
    str = std::to_string(ops(lhs, rhs));
  }
  return true;
}

void cu::PrintLogFile(const ct::string &dest_path) {
  std::ofstream out(dest_path);
  out << "-----------------------Logs---------------------------\n";
  for (auto &p : logs) out << p << "\n";
  out << "-----------------------Errors-------------------------\n";
  for (auto &p : errors) out << p << "\n";
  out << "-----------------------Warnings-----------------------\n";
  for (auto &s : warnings) out << s << "\n";
  out.close();
}

void cu::PrintProfiling(const ct::string &dest_path) {
  std::ofstream out(dest_path);
  out << "-----------------------Profiling----------------------\n";
  for (auto &p : profile_starts) {
    decltype(str_map)::accessor a;
    if (str_map.find(a, p.first))
      out << a->second << ": "
          << std::to_string(ProfileTime<std::milli>(a->second)) << "\n";
  }
  out.close();
}
