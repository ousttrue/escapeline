#include "el_term.h"
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace el {

RowCol GetTermSize() {
  struct winsize win;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
  return {
      .Row = win.ws_row,
      .Col = win.ws_col,
  };
}

int Stdout() { return STDOUT_FILENO; }

int Stdin() { return STDIN_FILENO; }

void SetupTerm() {
  // RawMode

  struct termios oldattr, newattr;
  tcgetattr(STDIN_FILENO, &oldattr);

  newattr = oldattr;
  newattr.c_lflag &= ~ICANON;
  newattr.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
}

} // namespace el
