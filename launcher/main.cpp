#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <filesystem>

// Starts the OSRS process and returns its info
PROCESS_INFORMATION LaunchClient(const std::string& clientPath) {
    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    // Extract the directory so the client's working dir is correct
    const std::string dir = std::filesystem::path(clientPath).parent_path().string();

    const bool ok = CreateProcessA(
        clientPath.c_str(),  // path to .exe
        nullptr,             // args
        nullptr, nullptr,
        FALSE,
        0,                   // no special flags — launch normally
        nullptr,
        dir.c_str(),         // working directory
        &si, &pi
    );

    if (!ok) {
        std::cerr << "[!] Failed to launch client. Error: "
                  << GetLastError() << "\n";
        exit(1);
    }

    std::cout << "[+] Client launched. PID: " << pi.dwProcessId << "\n";
    return pi;
}

// Injects a DLL into a running process by path
bool InjectDLL(HANDLE hProcess, const std::string& dllPath) {

    // 1. Allocate memory inside the target process for the DLL path string
    size_t pathLen = dllPath.size() + 1;
    void* remoteMem = VirtualAllocEx(
        hProcess, nullptr, pathLen,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE
    );
    if (!remoteMem) {
        std::cerr << "[!] VirtualAllocEx failed: " << GetLastError() << "\n";
        return false;
    }

    // 2. Write the DLL path string into that remote memory
    WriteProcessMemory(hProcess, remoteMem,
                       dllPath.c_str(), pathLen, nullptr);

    // 3. Get the address of LoadLibraryA (same in every process — it's in kernel32)
    FARPROC loadLib = GetProcAddress(
        GetModuleHandleA("kernel32.dll"), "LoadLibraryA"
    );

    // 4. Tell the target process to call LoadLibraryA(dllPath)
    //    This is what actually loads your DLL into the game
    HANDLE hThread = CreateRemoteThread(
        hProcess,
        nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLib),
        remoteMem,
        0, nullptr
    );

    if (!hThread) {
        std::cerr << "[!] CreateRemoteThread failed: " << GetLastError() << "\n";
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        return false;
    }

    // Wait for LoadLibrary to finish (i.e. your DllMain to run)
    WaitForSingleObject(hThread, 5000);

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);

    std::cout << "[+] DLL injected successfully.\n";
    return true;
}

std::string GetExeDir() {
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path().string();
}

int main() {
    const std::string clientPath = "C:\\Program Files (x86)\\Jagex Launcher\\Games\\Old School RuneScape\\Client\\osclient.exe";
    const std::string dllPath = GetExeDir() + "\\plugin-core.dll";

    PROCESS_INFORMATION pi = LaunchClient(clientPath);

    // Give the client a moment to initialize before we inject
    std::cout << "[*] Waiting for client to initialise...\n";
    Sleep(3000);

    InjectDLL(pi.hProcess, dllPath);

    // Keep launcher alive so we can see its output
    std::cout << "[*] Launcher holding. Press Enter to exit.\n";
    std::cin.get();

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}