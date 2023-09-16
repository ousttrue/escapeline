#include "el_term.h"
#include <Windows.h>

namespace el {

RowCol GetTermSize() {

  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

  return {
      .Row = static_cast<short>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1),
      .Col = static_cast<short>(csbi.srWindow.Right - csbi.srWindow.Left + 1),
  };
}

} // namespace el
