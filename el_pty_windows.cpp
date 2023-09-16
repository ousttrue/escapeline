#include "el_pty.h"
#include <Windows.h>
#include <assert.h>
#include <fcntl.h>
#include <fileapi.h>
#include <io.h>
#include <iostream>
#include <processthreadsapi.h>
#include <string>

HRESULT PrepareStartupInformation(HPCON hpc, STARTUPINFOEXW *psi) {
  // Prepare Startup Information structure
  STARTUPINFOEXW si;
  ZeroMemory(&si, sizeof(si));
  si.StartupInfo.cb = sizeof(si);

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
  int ReadPipeFile = 0;
  HANDLE WritePipe = 0;
  int WritePipeFile = 0;

  // child process
  STARTUPINFOEXW Startup = {};
  std::wstring Cmd = L"C:\\windows\\system32\\cmd.exe";

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

  int GetReadPipeFile() {
    ReadPipeFile = _open_osfhandle((intptr_t)ReadPipe, _O_RDONLY);
    ReadPipe = 0;
    return ReadPipeFile;
  }

  int GetWritePipeFile() {
    WritePipeFile = _open_osfhandle((intptr_t)WritePipe, _O_WRONLY);
    WritePipe = 0;
    return WritePipeFile;
  }

  bool Fork(const std::wstring &cmd) {
    Cmd = cmd;
    auto hr = PrepareStartupInformation(Console, &Startup);
    if (FAILED(hr)) {
      return false;
    }

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessW(NULL, Cmd.data(), NULL, NULL, FALSE,
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

int Pty::ReadFile() const { return m_impl->GetReadPipeFile(); }
int Pty::WriteFile() const { return m_impl->GetWritePipeFile(); }

bool Pty::ForkDefault() {
  // TODO: COMSPEC
  auto cmd = L"C:\\windows\\system32\\cmd.exe";
  return m_impl->Fork(cmd);
}

} // namespace el
