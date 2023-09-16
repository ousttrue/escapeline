#include "el_term.h"
#include <Windows.h>
#include <fcntl.h>
#include <io.h>

namespace el {

RowCol GetTermSize() {

  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

  return {
      .Row = static_cast<short>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1),
      .Col = static_cast<short>(csbi.srWindow.Right - csbi.srWindow.Left + 1),
  };
}

int Stdin() {
  return _open_osfhandle((intptr_t)GetStdHandle(STD_INPUT_HANDLE), _O_RDONLY);
}

int Stdout() {
  return _open_osfhandle((intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), _O_WRONLY);
}

} // namespace el
