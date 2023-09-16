#include "el_pty.h"
#include "el_term.h"
#include <iostream>

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

  // loop
  while (pty->IsAlive()) {
    //

  }

  return 0;
}
