#include "el.h"
#include "el_term.h"
#include <stdio.h>

#define ESC "\033"

namespace el {

RowCol EscapeLine::Initialize(int height) {
  m_height = height;
  return Update();
}

RowCol EscapeLine::Update() {
  auto size = GetTermSize();
  m_current = size;

  {
    // uim-fep
    printf(ESC "[s");

    // DECSTBM
    printf(ESC "[%d;%dr", 1, m_current.Row - m_height);

    printf(ESC "[u");
  }

  size.Row -= m_height;
  return size;
}

void EscapeLine::Draw() {
  // save
  printf(ESC "[s");
  printf(ESC "[?25l");

  // DECSTBM
  // printf(ESC "[r");

  // write
  int row = m_current.Row - m_height + 1; // 1 origin
  printf(ESC "[%d;%dH", row, 1);
  printf(ESC "[0m"
             "hello status line !");

  // DECSTBM
  // printf(ESC "[%d;%dr", 1, m_current.Row - m_height);

  // restore
  printf(ESC "[?25h");
  printf(ESC "[u");
}

std::span<char> EscapeLine::Input(const char *buf, size_t len) {
  m_buf.assign(buf, buf + len);
  return m_buf;
}

std::span<char> EscapeLine::Output(const char *buf, size_t len) {
  m_buf.assign(buf, buf + len);
  // if (m_buf.size() == 1 && m_buf[0] == '\r') {
  //   m_buf[0] = '\n';
  // }
  return m_buf;
}

} // namespace el
