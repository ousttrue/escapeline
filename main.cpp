#include "el_pty.h"
#include "el_term.h"
#include <iostream>

int main(int argc, char **argv) {

  // get termsize
  auto size = el::GetTermSize();
  std::cout << "rows: " << size.Row << ", "
            << "cols: " << size.Col << std::endl;

  // spwawn
  el::Pty pty;
  if (!pty.Spawn(size)) {
    return 1;
  }

  // loop

  return 0;
}
