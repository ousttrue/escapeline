#include "el.h"
#include "el_term.h"

#define ESC "\033"

namespace el {

RowCol EscapeLine::Initialize(int height) {
  m_height = height;
  return Update();
}

RowCol EscapeLine::Update() {
  auto size = GetTermSize();
  m_current = size;

  // DECSTBM
  printf(ESC "[%d,%dr", 1, m_current.Row - m_height);

  size.Row -= m_height;
  return size;
}

void EscapeLine::Draw() {
  // save
  printf(ESC "[s");

  // write
  int row = m_current.Row - m_height + 1; // 1 origin
  printf(ESC "[%d;%dH", row, 1);

  printf(ESC "[0m"
             "hello status line !");

  // restore
  printf(ESC "[u");
}

std::span<char> EscapeLine::Input(const char *buf, size_t len) {
  m_buf.assign(buf, buf + len);
  return m_buf;
}

void EscapeLine::Output(const char *buf, size_t len) {
  //
  std::vector<char> tmp;
  tmp.assign(buf, buf+len);
  auto a = 0;
}

} // namespace el
