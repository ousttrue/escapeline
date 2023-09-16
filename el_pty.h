#pragma once
#include "el_types.h"
#include <memory>

namespace el {

class Pty {

  struct PtyImpl *m_impl;

  Pty();

public:
  Pty(const Pty &) = delete;
  Pty &operator=(const Pty &) = delete;
  ~Pty();

  static std::shared_ptr<Pty> Create(const RowCol &size);
  bool IsAlive() const;
};

} // namespace el
