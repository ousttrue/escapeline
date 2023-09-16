#pragma once
#include "el_types.h"

namespace el {

class Pty {
public:
  bool Spawn(const RowCol &size);
};

} // namespace el
