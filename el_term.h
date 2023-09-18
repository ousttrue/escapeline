#pragma once
#include "el_types.h"
#include <string>

namespace el {

RowCol GetTermSize();
int Stdout();
int Stdin();
void SetupTerm();
std::string GetHome();

} // namespace el
