#include "el_pty.h"
#include <Windows.h>
#include <assert.h>
#include <iostream>
#include <processthreadsapi.h>
#include <string>

HRESULT PrepareStartupInformation(HPCON hpc, STARTUPINFOEX *psi) {
  // Prepare Startup Information structure
  STARTUPINFOEX si;
  ZeroMemory(&si, sizeof(si));
  si.StartupInfo.cb = sizeof(STARTUPINFOEX);

  // Discover the size required for the list
  size_t bytesRequired;
  InitializeProcThreadAttributeList(NULL, 1, 0, &bytesRequired);

  // Allocate memory to represent the list
  si.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(),
                                                              0, bytesRequired);
  if (!si.lpAttributeList) {
    return E_OUTOFMEMORY;
  }

  // Initialize the list memory location
  if (!InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0,
                                         &bytesRequired)) {
    HeapFree(GetProcessHeap(), 0, si.lpAttributeList);
    return HRESULT_FROM_WIN32(GetLastError());
  }

  // Set the pseudoconsole information into the list
  if (!UpdateProcThreadAttribute(si.lpAttributeList, 0,
                                 PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hpc,
                                 sizeof(hpc), NULL, NULL)) {
    HeapFree(GetProcessHeap(), 0, si.lpAttributeList);
    return HRESULT_FROM_WIN32(GetLastError());
  }

  *psi = si;

  return S_OK;
}

namespace el {

struct PtyImpl {
  // pty
  HPCON Console = 0;
  HANDLE ReadPipe = 0;
  HANDLE WritePipe = 0;

  // child process
  STARTUPINFOEXA Startup = {};
  std::string Cmd = "C:\\windows\\system32\\cmd.exe";

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

  bool Fork(const std::string &cmd) {
    Cmd = cmd;
    auto hr = PrepareStartupInformation(Console, &Startup);
    if (FAILED(hr)) {
      return false;
    }

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessA(NULL, Cmd.data(), NULL, NULL, FALSE,
                        EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
                        &Startup.StartupInfo, &pi)) {
      return HRESULT_FROM_WIN32(GetLastError());
    }
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

bool Pty::ForkDefault() {
  // TODO: COMSPEC
  auto cmd = "C:\\windows\\system32\\cmd.exe";
  return m_impl->Fork(cmd);
}

bool Pty::IsAlive() const { return false; }

} // namespace el
