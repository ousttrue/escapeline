#include "el.h"
#include "el_term.h"
#include <stdio.h>
#include <string>

#include <lua.hpp>
extern "C" {
#include <lauxlib.h>
}

class Lua {
  lua_State *L;

public:
  Lua() : L(luaL_newstate()) { luaL_openlibs(L); }
  ~Lua() { lua_close(L); }
  Lua(const Lua &) = delete;
  Lua &operator=(const Lua &) = delete;

  void Dofile(const std::string &file) {
    //
    luaL_dofile(L, file.c_str());
  }
};

#define ESC "\033"

namespace el {

struct EscapeLineImpl {
  Lua m_lua;
  RowCol m_current = {0, 0};
  int m_height = 1;
  std::vector<char> m_buf;

  RowCol Initialize(int height, std::string_view lua_file) {
    m_height = height;
    return Update();
  }

  RowCol Update() {
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

  void Draw() {
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

  std::span<char> Input(const char *buf, size_t len) {
    m_buf.assign(buf, buf + len);
    return m_buf;
  }

  std::span<char> Output(const char *buf, size_t len) {
    m_buf.assign(buf, buf + len);
    return m_buf;
  }
};

EscapeLine::EscapeLine() : m_impl(new EscapeLineImpl) {}

EscapeLine::~EscapeLine() { delete m_impl; }

RowCol EscapeLine::Initialize(int height, std::string_view lua_file) {
  return m_impl->Initialize(height, lua_file);
}

RowCol EscapeLine::Update() { return m_impl->Update(); }

void EscapeLine::Draw() { m_impl->Draw(); }

std::span<char> EscapeLine::Input(const char *buf, size_t len) {
  return m_impl->Input(buf, len);
}

std::span<char> EscapeLine::Output(const char *buf, size_t len) {
  return m_impl->Output(buf, len);
}

} // namespace el
