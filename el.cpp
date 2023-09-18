#include "el.h"
#include "el_term.h"
#include <assert.h>
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

  void Dofile(std::string_view src) {
    if (src.empty()) {
      return;
    }

    std::string file;
    if (src[0] == '~') {
      // expand: ~
      file += el::GetHome();
      src = src.substr(1);
    }
    file += src;

    // set global table el
    auto top = lua_gettop(L);
    lua_newtable(L);

    // default: el.line_height=1
    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "line_height");

    lua_setglobal(L, "el");
    assert(lua_gettop(L) == top);

    luaL_dofile(L, file.c_str());
  }

  // set el[key] = value
  void set(const char *key, int value) {
    auto top = lua_gettop(L);
    lua_getglobal(L, "el");
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key);
    lua_pop(L, 1);
    assert(lua_gettop(L) == top);
  }

  int get_int(const char *key) {
    auto top = lua_gettop(L);
    lua_getglobal(L, "el");
    lua_getfield(L, -1, key);
    lua_remove(L, -2);
    auto value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    assert(lua_gettop(L) == top);
    return value;
  }

  // call: el[func]()
  void call(const char *func) {
    auto top = lua_gettop(L);
    lua_getglobal(L, "el");
    lua_getfield(L, -1, func);
    lua_remove(L, -2);
    lua_call(L, 0, 0);
    auto end = lua_gettop(L);
    assert(end == top);
  }

  const char *call(const char *func, const char *buf, size_t len) {
    auto top = lua_gettop(L);
    lua_getglobal(L, "el");
    auto field_type = lua_getfield(L, -1, func);
    lua_remove(L, -2);
    lua_pushlstring(L, buf, len);
    if (field_type == LUA_TFUNCTION) {
      lua_call(L, 1, 1);
    }
    auto value = lua_tostring(L, -1);
    lua_pop(L, 1);
    assert(lua_gettop(L) == top);
    return value;
  }
};

namespace el {

struct EscapeLineImpl {
  Lua m_lua;
  std::vector<char> m_buf;

  RowCol Initialize(int height, std::string_view lua_file) {
    m_lua.Dofile(lua_file);

    m_lua.set("line_height", height);

    return Update();
  }

  RowCol Update() {
    auto size = GetTermSize();
    m_lua.set("rows", size.Row);
    m_lua.set("cols", size.Col);

    m_lua.call("update");

    size.Row -= m_lua.get_int("line_height");
    return size;
  }

  std::span<char> Input(const char *buf, size_t len) {
    auto result = m_lua.call("input", buf, len);
    m_buf.assign(result, result + strlen(result));
    return m_buf;
  }

  std::span<char> Output(const char *buf, size_t len) {
    auto result = m_lua.call("output", buf, len);
    m_buf.assign(result, result + strlen(result));
    return m_buf;
  }
};

EscapeLine::EscapeLine() : m_impl(new EscapeLineImpl) {}

EscapeLine::~EscapeLine() { delete m_impl; }

RowCol EscapeLine::Initialize(int height, std::string_view lua_file) {
  return m_impl->Initialize(height, lua_file);
}

RowCol EscapeLine::Update() { return m_impl->Update(); }

std::span<char> EscapeLine::Input(const char *buf, size_t len) {
  return m_impl->Input(buf, len);
}

std::span<char> EscapeLine::Output(const char *buf, size_t len) {
  return m_impl->Output(buf, len);
}

} // namespace el
