#pragma once
#include "el_types.h"
#include <memory>
#include <string>
#include <vector>

namespace el {

class Pty {

  struct PtyImpl *m_impl;

  Pty();

public:
  Pty(const Pty &) = delete;
  Pty &operator=(const Pty &) = delete;
  ~Pty();

  static std::shared_ptr<Pty> Create(const RowCol &size);
  int ReadFile() const;
  int WriteFile() const;
  bool ForkDefault();
  bool Fork(const std::vector<std::string> &args);
  void BlockProcess();
};

} // namespace el
