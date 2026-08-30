#include "pch/pch.h"


void Setup()
{
    if (!Offsets::Initialize())
        return;

    EngineFuncOriginal = reinterpret_cast<decltype(EngineFuncOriginal)>(Offsets::EngineModuleBase + Offsets::Engine::EngineThreadFunction);
    DropInventoryItemOriginal = reinterpret_cast<decltype(DropInventoryItemOriginal)>(Offsets::GameModuleBase + Offsets::Game::DropInventoryItemFunction);
    GetFireIntervalOriginal = reinterpret_cast<decltype(GetFireIntervalOriginal)>(Offsets::GameModuleBase + 0xD9F770);

    DetourStart(EngineFuncOriginal, EngineFunc);
    DetourStart(DropInventoryItemOriginal, DropInventoryItem);
    DetourStart(GetFireIntervalOriginal, GetFireInterval);

    if (!WebGUI::Start())
        printf("Failed to start web GUI\n");
}

BOOL APIENTRY DllMain(HMODULE Module, DWORD Reason, LPVOID)
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);

        if (HANDLE Thread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Setup), nullptr, 0, nullptr))
            CloseHandle(Thread);
    }
    else if (Reason == DLL_PROCESS_DETACH)
    {
        WebGUI::Stop();
        DetourRemove(EngineFuncOriginal, EngineFunc);
        DetourRemove(DropInventoryItemOriginal, DropInventoryItem);
        DetourRemove(GetFireIntervalOriginal, GetFireInterval);
		FreeConsole();
    }

    return TRUE;
}   