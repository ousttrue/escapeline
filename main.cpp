#include "el_pty.h"
#include "el_term.h"
#include <iostream>
#include <lua.hpp>
#include <string>

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

int main(int argc, char **argv) {

  // get termsize
  auto size = el::GetTermSize();
  std::cout << "rows: " << size.Row << ", "
            << "cols: " << size.Col << std::endl;

  // spwawn
  auto pty = el::Pty::Create(size);
  if (!pty) {
    return 1;
  }

  if (!pty->ForkDefault()) {
    return 2;
  }

  Lua lua;

  if (argc > 1) {
    lua.Dofile(argv[1]);
  }

  return 0;
}
