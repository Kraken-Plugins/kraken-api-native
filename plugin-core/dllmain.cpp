#include <Windows.h>
#include <iostream>
#include <thread>

// This is the actual plugin logic — runs in its own thread
// so it doesn't block DllMain
void PluginMain() {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);

    std::cout << "==============================\n";
    std::cout << "  plugin-core loaded!         \n";
    std::cout << "  DLL injection is working.   \n";
    std::cout << "==============================\n\n";

    // Keep the thread alive — later this becomes your main loop
    // (reading game state, running plugin tick, etc.)
    int tick = 0;
    while (true) {
        std::cout << "[tick " << tick++ << "] Plugin running...\n";
        Sleep(5000); // print every 5 seconds for now
    }
}

// Windows calls this automatically when the DLL is loaded/unloaded
BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD   reason,
                      LPVOID  /*reserved*/) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        std::thread(PluginMain).detach();
    }
    return TRUE;
}