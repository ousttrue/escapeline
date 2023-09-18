#include "el_pty.h"
#include <fcntl.h>
#include <fstream>
#include <pty.h>
#include <stdlib.h>
#include <utmp.h>
#include <wait.h>

namespace el {

struct PtyImpl {
  int Master = -1;
  int Slave = -1;
  struct winsize WinSize;
  struct termios OrigTermios;
  char SlaveName[20];

  pid_t Child = {};

  PtyImpl() {}
  ~PtyImpl() {
    if (Master != -1) {
      close(Master);
      Master = -1;
    }
    if (Slave != -1) {
      close(Slave);
      Slave = -1;
    }
  }

  bool Create(const RowCol &size) {

    WinSize.ws_row = static_cast<unsigned short>(size.Row);
    WinSize.ws_col = static_cast<unsigned short>(size.Col);

    if (-1 == tcgetattr(STDIN_FILENO, &OrigTermios)) {
      perror("tcgetattr()");
      // exit(1);
      return false;
    }

    if (openpty(&Master, &Slave, SlaveName, nullptr, &WinSize) != 0) {
      return false;
    }

    return true;
  }

  void SetSize(const RowCol &size) {
    struct winsize ws = {
        .ws_row = static_cast<unsigned short>(size.Row),
        .ws_col = static_cast<unsigned short>(size.Col),
    };
    ioctl(Master, TIOCSWINSZ, &ws);
  }

  bool Fork(const std::vector<std::string> &args) {
    std::vector<const char *> argv;
    for (size_t i = 1; i < args.size(); ++i) {
      argv.push_back(args[i].c_str());
    }
    argv.push_back(NULL);

    Child = fork();
    if (Child == -1) {
      std::ofstream w("tmp.log");
      w << "Child -1: " << std::endl;
      return false;
    }

    if (Child == 0) {
      // in child process
      if (auto err = login_tty(Slave) != 0) {
        exit(1);
      }
      close(Master);

      // setsid();
      // setenv("TERM", "xterm", 1);
      // signal(SIGCHLD, SIG_DFL);
      if (auto err = execvp(args[0].c_str(), (char *const *)argv.data()) != 0) {
        exit(2);
      }
    }

    close(Slave);
    Slave = -1;

    // struct termios attr;
    // tcgetattr(Master, &attr);
    // // attr.c_lflag &= ~ICANON;
    // attr.c_lflag &= ~ECHO;
    // tcsetattr(Master, TCSANOW, &attr);

    return true;
  }

  void WaitChild() {
    int stat;
    waitpid(Child, &stat, 0);
  }
};

Pty::Pty() : m_impl(new PtyImpl) {}

Pty::~Pty() { delete m_impl; }

std::shared_ptr<Pty> Pty::Create(const RowCol &size) {
  auto ptr = std::shared_ptr<Pty>(new Pty);
  if (!ptr->m_impl->Create(size)) {
    return {};
  }
  return ptr;
}

int Pty::ReadFile() const { return m_impl->Master; }

int Pty::WriteFile() const { return m_impl->Master; }

bool Pty::ForkDefault() { return Fork({"/bin/bash"}); }

bool Pty::Fork(const std::vector<std::string> &args) {
  return m_impl->Fork(args);
}

void Pty::BlockProcess() { m_impl->WaitChild(); }

void Pty::SetSize(const RowCol &size) { m_impl->SetSize(size); }

} // namespace el
