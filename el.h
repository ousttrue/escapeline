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

  // <== update TTY stdin scroll region
  // ==> update VT size
  //
  // +--------------------+
  // |                    |
  // | term rows - height |
  // |                    |
  // +--------------------+
  // | status line height |
  // +--------------------+
  //
  RowCol Initialize(int height, std::string_view lua_file);
  RowCol Update();
  void Draw();
  std::span<char> Input(const char *buf, size_t len);
  std::span<char> Output(const char *buf, size_t len);
};

} // namespace el
