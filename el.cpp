#include "el.h"
#include "el_term.h"

namespace el {

RowCol EscapeLine::Initialize(int height) {
  m_height = height;
  return Update();
}

RowCol EscapeLine::Update() {
  auto size = GetTermSize();
  size.Row -= m_height;

  // scroll region
  printf("\033[%d,%dr", 0, size.Row);

  return size;
}

void EscapeLine::Draw() {
  // save

  // write

  // restore
}

std::span<char> EscapeLine::Input(const char *buf, size_t len) {
  m_buf.assign(buf, buf + len);
  return m_buf;
}

} // namespace el
