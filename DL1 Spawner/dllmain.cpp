#include "pch/pch.h"

namespace
{
    struct KeyState
    {
        bool WasDown{};

        bool Pressed(int VirtualKey)
        {
            const bool IsDown = (GetAsyncKeyState(VirtualKey) & 0x8000) != 0;
            const bool Result = IsDown && !WasDown;
            WasDown = IsDown;
            return Result;
        }

        void Synchronize(int VirtualKey)
        {
            WasDown = (GetAsyncKeyState(VirtualKey) & 0x8000) != 0;
        }
    };

    void WaitForKeyRelease(int VirtualKey)
    {
        while (GetAsyncKeyState(VirtualKey) & 0x8000)
            Sleep(10);
    }

    void FlushConsoleInput()
    {
        const HANDLE Input = GetStdHandle(STD_INPUT_HANDLE);

        if (Input != INVALID_HANDLE_VALUE)
            FlushConsoleInputBuffer(Input);
    }
}

DWORD WINAPI Setup(LPVOID)
{
    if (!Console::SetupConsole())
        return 0;

    if (!Offsets::Initialize())
        return 0;

    EngineFuncOriginal = reinterpret_cast<decltype(EngineFuncOriginal)>(Offsets::EngineModuleBase + Offsets::Engine::EngineThreadFunction);
    DropInventoryItemOriginal = reinterpret_cast<decltype(DropInventoryItemOriginal)>(Offsets::GameModuleBase + Offsets::Game::DropInventoryItemFunction);

    DetourStart(EngineFuncOriginal, EngineFunc);
    DetourStart(DropInventoryItemOriginal, DropInventoryItem);

    printf(
        "========================================\n"
        "              ITEM SPAWNER\n"
        "========================================\n"
        "[NUMPAD 1] Input item ID and quantity\n"
        "[NUMPAD 2] Enable/disable item duper\n"
        "========================================\n\n"
    );

    KeyState SpawnKey{};
    KeyState DupeKey{};
    bool DupeEnabled = false;

    SpawnKey.Synchronize(VK_NUMPAD1);
    DupeKey.Synchronize(VK_NUMPAD2);

    while (true)
    {
        if (SpawnKey.Pressed(VK_NUMPAD1))
        {
            WaitForKeyRelease(VK_NUMPAD1);
            FlushConsoleInput();

            uint32_t ItemID{};
            uint32_t Quantity{};

            printf("\n[ITEM SPAWNER]\n");

            if (!Console::ReadUnsignedInteger("Enter item ID: ", ItemID))
            {
                printf("[ERROR] Invalid item ID.\n\n");
            }
            else if (!Console::ReadUnsignedInteger("Enter quantity: ", Quantity))
            {
                printf("[ERROR] Invalid quantity.\n\n");
            }
            else if (!Quantity)
            {
                printf("[ERROR] Quantity must be greater than zero.\n\n");
            }
            else
            {
                PendingRequest.store(Console::EncodeRequest(ItemID, Quantity), std::memory_order_release);
                printf("[QUEUED] Item ID: %u | Quantity: %u\n\n", ItemID, Quantity);
            }

            // Digits entered on the numpad must not become hotkey presses.
            SpawnKey.Synchronize(VK_NUMPAD1);
            DupeKey.Synchronize(VK_NUMPAD2);
        }

        if (DupeKey.Pressed(VK_NUMPAD2))
        {
            DupeEnabled = !DupeEnabled;
            DupeItems.store(DupeEnabled, std::memory_order_release);

            printf("[ITEM DUPER] %s\n", DupeEnabled ? "Enabled" : "Disabled");

            WaitForKeyRelease(VK_NUMPAD2);
            DupeKey.Synchronize(VK_NUMPAD2);
        }

        Sleep(10);
    }
}

BOOL APIENTRY DllMain(HMODULE Module, DWORD Reason, LPVOID)
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(Module);

        if (HANDLE Thread = CreateThread(nullptr, 0, Setup, nullptr, 0, nullptr))
            CloseHandle(Thread);
    }

    return TRUE;
}