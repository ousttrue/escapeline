#include "el_pty.h"
#include <Windows.h>
#include <assert.h>
#include <iostream>

namespace el {

struct PtyImpl {
  HPCON Console = 0;
  HANDLE ReadPipe = 0;
  HANDLE WritePipe = 0;

  bool Create(const COORD &size) {

    HANDLE inPipeRead{INVALID_HANDLE_VALUE};
    if (!CreatePipe(&inPipeRead, &WritePipe, NULL, 0)) {
      std::cerr << "CreatePipe" << std::endl;
      return {};
    }

    HANDLE outPipeWrite{INVALID_HANDLE_VALUE};
    if (!CreatePipe(&this->ReadPipe, &outPipeWrite, NULL, 0)) {
      CloseHandle(inPipeRead);
      CloseHandle(this->WritePipe);
      std::cerr << "CreatePipe" << std::endl;
      return {};
    }

    // Create the Pseudo Console of the required size, attached to the PTY-end
    // of the pipes
    auto hr =
        CreatePseudoConsole(size, inPipeRead, outPipeWrite, 0, &this->Console);
    // Note: We can close the handles to the PTY-end of the pipes here
    // because the handles are dup'ed into the ConHost and will be released
    // when the ConPTY is destroyed.
    if (INVALID_HANDLE_VALUE != outPipeWrite) {
      CloseHandle(outPipeWrite);
    }
    if (INVALID_HANDLE_VALUE != inPipeRead) {
      CloseHandle(inPipeRead);
    }
    if (FAILED(hr)) {
      std::cerr << "CreatePseudoConsole" << std::endl;
      CloseHandle(this->WritePipe);
      CloseHandle(this->ReadPipe);
      return {};
    }

    assert(this->Console);
    return true;
  }
};

Pty::Pty() : m_impl(new PtyImpl) {}

Pty::~Pty() { delete m_impl; }

std::shared_ptr<Pty> Pty::Create(const RowCol &size) {
  // pty
  auto ptr = std::shared_ptr<Pty>(new Pty);
  if (!ptr->m_impl->Create({
          .X = size.Col,
          .Y = size.Row,
      })) {
    return {};
  }

  return ptr;
}

bool Pty::IsAlive() const { return false; }

} // namespace el
