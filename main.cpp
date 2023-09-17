#include "el_pty.h"
#include "el_term.h"
#include <cwchar>
#include <cxxopts.hpp>
#include <iostream>
#include <lua.hpp>
#include <memory>
#include <string>
#include <uv.h>

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

std::shared_ptr<el::Pty> g_pty;

uv_tty_t tty_in;
uv_pipe_t ptyin_pipe;

uv_pipe_t ptyout_pipe;
uv_tty_t tty_out;

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size,
                         uv_buf_t *buf) {
  *buf = uv_buf_init((char *)malloc(suggested_size), suggested_size);
}

struct write_req_t {
  uv_write_t req;
  uv_buf_t buf;
};

void free_write_req(uv_write_t *req) {
  write_req_t *wr = (write_req_t *)req;
  free(wr->buf.base);
  free(wr);
}

void write_data(uv_stream_t *dest, size_t size, uv_buf_t buf, uv_write_cb cb) {
  write_req_t *req = (write_req_t *)malloc(sizeof(write_req_t));
  req->buf = uv_buf_init((char *)malloc(size), size);
  memcpy(req->buf.base, buf.base, size);
  uv_write((uv_write_t *)req, (uv_stream_t *)dest, &req->buf, 1, cb);
}

int Run(const cxxopts::ParseResult &result) {
  el::SetupTerm();

  // get termsize
  auto size = el::GetTermSize();
  // std::cout << "rows: " << size.Row << ", "
  //           << "cols: " << size.Col << std::endl;

  // status line !
  size.Row -= 2;

  // pty
  g_pty = el::Pty::Create(size);
  if (!g_pty) {
    return 1;
  }

  Lua lua;
  auto lua_file = result["lua"].as<std::string>();
  if (lua_file.size() > 0) {
    lua.Dofile(lua_file.c_str());
  }

  // spwawn
  if (result.unmatched().size() > 0) {
    if (!g_pty->Fork(result.unmatched())) {
      return 2;
    }
  } else {
    if (!g_pty->ForkDefault()) {
      return 2;
    }
  }

  // status line !
  printf("\033[%d,%dr", 0, size.Row);

  uv_work_t work;
  uv_queue_work(
      uv_default_loop(), &work,
      [](uv_work_t *req) {
        // watch child process
        g_pty->BlockProcess();
      },
      [](uv_work_t *req, int status) {
        // stop loop
        g_pty = nullptr;

        uv_read_stop((uv_stream_t *)&ptyout_pipe);
        // uv_close((uv_handle_t *)&tty_out, NULL);

        uv_read_stop((uv_stream_t *)&tty_in);
        // uv_close((uv_handle_t *)&ptyin_pipe, NULL);
      });

  // stdin ==> ptyin,
  uv_tty_init(uv_default_loop(), &tty_in, el::Stdin(), 1);
  uv_tty_set_mode(&tty_in, UV_TTY_MODE_RAW);
  uv_pipe_init(uv_default_loop(), &ptyin_pipe, 0);
  uv_pipe_open(&ptyin_pipe, g_pty->WriteFile());

  uv_read_start(
      (uv_stream_t *)&tty_in, &alloc_buffer,
      [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
        if (nread < 0) {
          if (nread == UV_EOF) {
            // end of file
            uv_close((uv_handle_t *)&tty_in, NULL);
          }
        } else if (nread > 0) {
          write_data((uv_stream_t *)&ptyin_pipe, nread, *buf,
                     [](uv_write_t *req, int status) { free_write_req(req); });
          // g_pty->Write(buf, nread);
        }

        // OK to free buffer as write_data copies it.
        if (buf->base)
          free(buf->base);
      });

  // stdout <== ptyout
  uv_pipe_init(uv_default_loop(), &ptyout_pipe, 0);
  uv_pipe_open(&ptyout_pipe, g_pty->ReadFile());
  uv_tty_init(uv_default_loop(), &tty_out, el::Stdout(), 0);
  uv_tty_set_mode(&tty_out, UV_TTY_MODE_NORMAL);

  uv_read_start(
      (uv_stream_t *)&ptyout_pipe, &alloc_buffer,
      [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
        if (nread < 0) {
          if (nread == UV_EOF) {
            // end of file
            uv_close((uv_handle_t *)&ptyout_pipe, NULL);
          }
        } else if (nread > 0) {
          write_data((uv_stream_t *)&tty_out, nread, *buf,
                     [](uv_write_t *req, int status) { free_write_req(req); });
          // DWORD size;
          // WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buf->base,
          //               nread, &size, 0);
        }

        // OK to free buffer as write_data copies it.
        if (buf->base)
          free(buf->base);
      });

  // TODO: resize event, signal, timer...
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  uv_loop_close(uv_default_loop());

  return 0;
}

int main(int argc, char **argv) {
  // escapeline --lua rc.lua -- cmd.exe ...
  cxxopts::Options options("EscapeLine", "escape sequence status line");
  options.add_options()
      //
      ("l,lua", "lua script", cxxopts::value<std::string>()->default_value(""))
      // ( "v,verbose", "Verbose output",
      // cxxopts::value<bool>()->default_value("false"))
      ;
  options.allow_unrecognised_options();

  try {
    auto result = options.parse(argc, argv);
    return Run(result);
  } catch (const cxxopts::exceptions::exception &ex) {
    std::cerr << ex.what() << std::endl;
    std::cout << options.help() << std::endl;
    return -1;
  }
}
