#pragma once
#include "el_types.h"
#include <span>
#include <stdint.h>
#include <string>
#include <vector>

namespace el {

class EscapeLine {

  struct EscapeLineImpl *m_impl;

public:
  EscapeLine();
  ~EscapeLine();
  EscapeLine(const EscapeLine &) = delete;
  EscapeLine &operator=(const EscapeLine &) = delete;

  RowCol Initialize(int height, std::string_view lua_file);
  RowCol Update();
  int Timer();
  std::span<char> Input(const char *buf, size_t len);
  std::span<char> Output(const char *buf, size_t len);
};

} // namespace el
