#pragma once
#include "el_types.h"
#include <span>
#include <stdint.h>
#include <vector>

namespace el {

class EscapeLine {
  RowCol m_current = {0, 0};
  int m_height = 1;
  std::vector<char> m_buf;

public:
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
  RowCol Initialize(int height);
  RowCol Update();
  void Draw();
  std::span<char> Input(const char *buf, size_t len);
  std::span<char> Output(const char *buf, size_t len);
};

} // namespace el
