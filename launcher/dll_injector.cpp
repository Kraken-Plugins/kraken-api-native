#include "dll_injector.hpp"

#include "win32_handle.hpp"

#include <TlHelp32.h>

#include <cstdint>
#include <cwchar>
#include <string>

namespace kraken::launcher {
namespace {

class RemoteAllocation {
public:
    RemoteAllocation(HANDLE process, SIZE_T size)
        : process_(process),
          address_(VirtualAllocEx(process,
                                  nullptr,
                                  size,
                                  MEM_COMMIT | MEM_RESERVE,
                                  PAGE_READWRITE)) {
    }

    ~RemoteAllocation() {
        Reset();
    }

    RemoteAllocation(const RemoteAllocation&) = delete;
    RemoteAllocation& operator=(const RemoteAllocation&) = delete;

    [[nodiscard]] void* Get() const {
        return address_;
    }

    [[nodiscard]] explicit operator bool() const {
        return address_ != nullptr;
    }

    void ReleaseWithoutFreeing() {
        address_ = nullptr;
    }

private:
    void Reset() {
        if (address_ != nullptr) {
            VirtualFreeEx(process_, address_, 0, MEM_RELEASE);
            address_ = nullptr;
        }
    }

    HANDLE process_ = nullptr;
    void* address_ = nullptr;
};

std::string LastErrorMessage(const char* action) {
    return std::string(action) + " failed with Win32 error " +
           std::to_string(GetLastError());
}

void SetError(std::string* errorMessage, std::string message) {
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

void SetLastErrorMessage(std::string* errorMessage, const char* action) {
    SetError(errorMessage, LastErrorMessage(action));
}

UniqueHandle CreateModuleSnapshot(DWORD processId) {
    for (int attempt = 0; attempt < 5; ++attempt) {
        UniqueHandle snapshot(CreateToolhelp32Snapshot(
            TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
            processId));

        if (snapshot) {
            return snapshot;
        }

        if (GetLastError() != ERROR_BAD_LENGTH) {
            return snapshot;
        }

        Sleep(10);
    }

    return UniqueHandle(CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
        processId));
}

void* ResolveRemoteLoadLibraryW(DWORD processId, std::string* errorMessage) {
    HMODULE localKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (localKernel32 == nullptr) {
        SetLastErrorMessage(errorMessage, "GetModuleHandleW(kernel32.dll)");
        return nullptr;
    }

    FARPROC localLoadLibrary = GetProcAddress(localKernel32, "LoadLibraryW");
    if (localLoadLibrary == nullptr) {
        SetLastErrorMessage(errorMessage, "GetProcAddress(LoadLibraryW)");
        return nullptr;
    }

    const auto loadLibraryOffset =
        reinterpret_cast<std::uintptr_t>(localLoadLibrary) -
        reinterpret_cast<std::uintptr_t>(localKernel32);

    UniqueHandle snapshot = CreateModuleSnapshot(processId);
    if (!snapshot) {
        SetLastErrorMessage(errorMessage, "CreateToolhelp32Snapshot");
        return nullptr;
    }

    MODULEENTRY32W moduleEntry{};
    moduleEntry.dwSize = sizeof(moduleEntry);

    for (BOOL hasModule = Module32FirstW(snapshot.Get(), &moduleEntry);
         hasModule;
         hasModule = Module32NextW(snapshot.Get(), &moduleEntry)) {
        if (_wcsicmp(moduleEntry.szModule, L"kernel32.dll") == 0) {
            return moduleEntry.modBaseAddr + loadLibraryOffset;
        }
    }

    SetError(errorMessage,
             "Could not find kernel32.dll in target process modules.");
    return nullptr;
}

} // namespace

bool DllInjector::Inject(HANDLE process,
                         DWORD processId,
                         const std::filesystem::path& dllPath,
                         std::chrono::milliseconds timeout,
                         std::string* errorMessage) {
    if (!std::filesystem::exists(dllPath)) {
        SetError(errorMessage, "DLL does not exist: " + dllPath.string());
        return false;
    }

    const std::wstring remotePath = dllPath.wstring();
    const SIZE_T remotePathBytes =
        (remotePath.size() + 1) * sizeof(wchar_t);

    RemoteAllocation remoteMemory(process, remotePathBytes);
    if (!remoteMemory) {
        SetLastErrorMessage(errorMessage, "VirtualAllocEx");
        return false;
    }

    SIZE_T bytesWritten = 0;
    const BOOL wroteMemory = WriteProcessMemory(
        process,
        remoteMemory.Get(),
        remotePath.c_str(),
        remotePathBytes,
        &bytesWritten);

    if (!wroteMemory || bytesWritten != remotePathBytes) {
        SetLastErrorMessage(errorMessage, "WriteProcessMemory");
        return false;
    }

    void* loadLibrary = ResolveRemoteLoadLibraryW(processId, errorMessage);
    if (loadLibrary == nullptr) {
        return false;
    }

    UniqueHandle remoteThread(CreateRemoteThread(
        process,
        nullptr,
        0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibrary),
        remoteMemory.Get(),
        0,
        nullptr));

    if (!remoteThread) {
        SetLastErrorMessage(errorMessage, "CreateRemoteThread");
        return false;
    }

    const DWORD waitResult = WaitForSingleObject(
        remoteThread.Get(),
        static_cast<DWORD>(timeout.count()));

    if (waitResult == WAIT_TIMEOUT) {
        SetError(errorMessage,
                 "Timed out waiting for LoadLibraryW to finish. Remote "
                 "memory was intentionally left allocated.");
        remoteMemory.ReleaseWithoutFreeing();
        return false;
    }

    if (waitResult != WAIT_OBJECT_0) {
        SetLastErrorMessage(errorMessage, "WaitForSingleObject");
        return false;
    }

    DWORD exitCode = 0;
    if (!GetExitCodeThread(remoteThread.Get(), &exitCode)) {
        SetLastErrorMessage(errorMessage, "GetExitCodeThread");
        return false;
    }

    if (exitCode == 0) {
        SetError(errorMessage, "LoadLibraryW returned null inside the client.");
        return false;
    }

    return true;
}

} // namespace kraken::launcher
