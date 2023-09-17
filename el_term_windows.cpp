#include "el_term.h"
#include <Windows.h>
#include <corecrt_wstdio.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>

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

void SetupTerm() {
  // SetConsoleCP(CP_UTF8);

  {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn == INVALID_HANDLE_VALUE) {
      return;
    }
    DWORD mode;
    if (!GetConsoleMode(hIn, &mode)) {
      return;
    }
    if (!SetConsoleMode(hIn, mode | ENABLE_VIRTUAL_TERMINAL_INPUT)) {
      return;
    }
  }

  {
    // _setmode(1, _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
      return;
    }
    DWORD mode;
    if (!GetConsoleMode(hOut, &mode)) {
      return;
    }
    mode |= ENABLE_PROCESSED_OUTPUT;
    // mode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
    mode |= ENABLE_WRAP_AT_EOL_OUTPUT;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    mode |= DISABLE_NEWLINE_AUTO_RETURN;
    if (!SetConsoleMode(hOut, mode)) {
      return;
    }
  }
}

} // namespace el
