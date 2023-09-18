#include "el.h"
#include "el_pty.h"
#include "el_term.h"
#include <cxxopts.hpp>
#include <iostream>
#include <uv.h>

std::shared_ptr<el::Pty> g_pty;

uv_tty_t g_tty_in;
uv_pipe_t g_ptyin_pipe;

uv_pipe_t g_ptyout_pipe;
uv_tty_t g_tty_out;

uv_signal_t g_signal_resize;

el::EscapeLine g_el;

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

void write_data(uv_stream_t *dest, size_t size, const char *data,
                uv_write_cb cb) {
  write_req_t *req = (write_req_t *)malloc(sizeof(write_req_t));
  req->buf = uv_buf_init((char *)malloc(size), size);
  memcpy(req->buf.base, data, size);
  uv_write((uv_write_t *)req, (uv_stream_t *)dest, &req->buf, 1, cb);
}

int Run(const cxxopts::ParseResult &result) {
  el::SetupTerm();

  std::string lua_file = "~/.config/escapeline/config.lua";
  auto lua_arg = result["lua"].as<std::string>();
  if (lua_arg.size() > 0) {
    lua_file = lua_arg;
  }

  // pty
  g_pty =
      el::Pty::Create(g_el.Initialize(result["height"].as<int>(), lua_file));
  if (!g_pty) {
    return 1;
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

        uv_read_stop((uv_stream_t *)&g_ptyout_pipe);
        uv_read_stop((uv_stream_t *)&g_tty_in);
        uv_signal_stop(&g_signal_resize);
      });

  // stdin ==> ptyin,
  uv_tty_init(uv_default_loop(), &g_tty_in, el::Stdin(), 1);
  uv_tty_set_mode(&g_tty_in, UV_TTY_MODE_RAW);
  uv_pipe_init(uv_default_loop(), &g_ptyin_pipe, 0);
  uv_pipe_open(&g_ptyin_pipe, g_pty->WriteFile());

  uv_read_start(
      (uv_stream_t *)&g_tty_in, &alloc_buffer,
      [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
        if (nread < 0) {
          if (nread == UV_EOF) {
            // end of file
            uv_close((uv_handle_t *)&g_tty_in, NULL);
          }
        } else if (nread > 0) {
          // process key input
          auto data = g_el.Input(buf->base, nread);

          write_data((uv_stream_t *)&g_ptyin_pipe, data.size(), data.data(),
                     [](uv_write_t *req, int status) { free_write_req(req); });
        }

        // OK to free buffer as write_data copies it.
        if (buf->base)
          free(buf->base);
      });

  // stdout <== ptyout
  uv_pipe_init(uv_default_loop(), &g_ptyout_pipe, 0);
  uv_pipe_open(&g_ptyout_pipe, g_pty->ReadFile());
  uv_tty_init(uv_default_loop(), &g_tty_out, el::Stdout(), 0);
  uv_tty_set_mode(&g_tty_out, UV_TTY_MODE_NORMAL);

  uv_read_start(
      (uv_stream_t *)&g_ptyout_pipe, &alloc_buffer,
      [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
        if (nread < 0) {
          if (nread == UV_EOF) {
            // end of file
            uv_close((uv_handle_t *)&g_ptyout_pipe, NULL);
          }
        } else if (nread > 0) {
          auto data = g_el.Output(buf->base, nread);
          write_data((uv_stream_t *)&g_tty_out, data.size(), data.data(),
                     [](uv_write_t *req, int status) { free_write_req(req); });
        }

        // OK to free buffer as write_data copies it.
        if (buf->base)
          free(buf->base);
      });

  uv_signal_init(uv_default_loop(), &g_signal_resize);
  uv_signal_start(
      &g_signal_resize,
      [](uv_signal_t *handle, int signum) {
        // update status line
        auto size = g_el.Update();

        g_pty->SetSize(size);
      },
      SIGWINCH);

  // TODO: signal, timer...
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  uv_loop_close(uv_default_loop());

  return 0;
}

int main(int argc, char **argv) {
  // escapeline --lua rc.lua -- cmd.exe ...
  cxxopts::Options options("EscapeLine", "escape sequence status line");
  options.add_options()
      //
      ("h,height", "status line height",
       cxxopts::value<int>()->default_value("1"))
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
