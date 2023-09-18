#include "el_pty.h"
#include <Windows.h>
#include <assert.h>
#include <fcntl.h>
#include <fileapi.h>
#include <io.h>
#include <iostream>
#include <processthreadsapi.h>
#include <string>

auto NEWLINE = "\n\r";

HRESULT PrepareStartupInformation(HPCON hpc, STARTUPINFOEXA *psi) {
  // Prepare Startup Information structure
  STARTUPINFOEXA si;
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
  STARTUPINFOEXA Startup = {};
  std::string Cmd;
  PROCESS_INFORMATION Pi;

  PtyImpl() {}

  ~PtyImpl() {
    if (ReadPipe) {
      // std::cout << "CloseHandle ReadPipe" << NEWLINE;
      CloseHandle(ReadPipe);
      ReadPipe = 0;
    }
    if (ReadPipeFile) {
      // std::cout << "_close ReadPipeFile" << NEWLINE;
      _close(ReadPipeFile);
      ReadPipeFile = 0;
    }
    if (WritePipe) {
      // std::cout << "CloseHandle WritePipe" << NEWLINE;
      CloseHandle(WritePipe);
      WritePipe = 0;
    }
    if (WritePipeFile) {
      // std::cout << "_close WritePipeFile" << NEWLINE;
      _close(WritePipeFile);
      WritePipeFile = 0;
    }
    if (Pi.hThread) {
      // std::cout << "CloseHandle Pi.hThread" << NEWLINE;
      CloseHandle(Pi.hThread);
    }
    if (Pi.hProcess) {
      // std::cout << "CloseHandle Pi.hProcess" << NEWLINE;
      CloseHandle(Pi.hProcess);
    }
    // std::cout << "released" << NEWLINE;
  }

  bool Create(const COORD &size) {

    HANDLE inPipeRead{INVALID_HANDLE_VALUE};
    if (!CreatePipe(&inPipeRead, &WritePipe, NULL, 0)) {
      std::cerr << "CreatePipe" << NEWLINE;
      return {};
    }

    HANDLE outPipeWrite{INVALID_HANDLE_VALUE};
    if (!CreatePipe(&this->ReadPipe, &outPipeWrite, NULL, 0)) {
      CloseHandle(inPipeRead);
      CloseHandle(this->WritePipe);
      std::cerr << "CreatePipe" << NEWLINE;
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
      std::cerr << "CreatePseudoConsole" << NEWLINE;
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

  bool Fork(const std::vector<std::string> &args) {
    if (args.empty()) {
      return false;
    }
    Cmd = args[0];
    for (int i = 1; i < args.size(); ++i) {
      Cmd += " ";
      Cmd += args[i];
    }
    std::cout << "launch: " << Cmd << NEWLINE;

    auto hr = PrepareStartupInformation(Console, &Startup);
    if (FAILED(hr)) {
      return false;
    }

    ZeroMemory(&Pi, sizeof(Pi));
    if (!CreateProcessA(NULL, Cmd.data(), NULL, NULL, FALSE,
                        EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
                        &Startup.StartupInfo, &Pi)) {
      return HRESULT_FROM_WIN32(GetLastError());
    }

    return true;
  }

  void BlockProcess() {
    WaitForSingleObject(Pi.hProcess, INFINITE);
    std::cout << NEWLINE << "process: exited" << NEWLINE;
  }

  void SetSize(const RowCol &size) {
    ResizePseudoConsole(Console, {
                                     .X = static_cast<short>(size.Col),
                                     .Y = static_cast<short>(size.Row),
                                 });
  }
};

Pty::Pty() : m_impl(new PtyImpl) {}

Pty::~Pty() { delete m_impl; }

std::shared_ptr<Pty> Pty::Create(const RowCol &size) {
  // pty
  auto ptr = std::shared_ptr<Pty>(new Pty);
  if (!ptr->m_impl->Create({
          .X = static_cast<short>(size.Col),
          .Y = static_cast<short>(size.Row),
      })) {
    return {};
  }

  return ptr;
}

int Pty::ReadFile() const { return m_impl->GetReadPipeFile(); }
int Pty::WriteFile() const { return m_impl->GetWritePipeFile(); }

bool Pty::Fork(const std::vector<std::string> &args) {
  return m_impl->Fork(args);
}

bool Pty::ForkDefault() {
  // TODO: COMSPEC
  std::vector<std::string> args{"C:\\windows\\system32\\cmd.exe"};
  return Fork(args);
}

void Pty::BlockProcess() { m_impl->BlockProcess(); }

void Pty::SetSize(const RowCol &size) { m_impl->SetSize(size); }

} // namespace el
