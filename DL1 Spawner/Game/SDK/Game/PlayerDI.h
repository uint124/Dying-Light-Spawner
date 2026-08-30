#pragma once

class PlayerDI : public IControlObject
{
public:
    // Player + 0x938 == InventoryContainerDI

    class InventoryContainerDI* GetInventoryContainer()
    {
        return *(InventoryContainerDI**)((DWORD64)this + Offsets::Game::PlayerDI::InventoryContainer);
    }

    const char* GetName()
    {
        uint64_t Data = *(uint64_t*)((DWORD64)this + 0x7E8);
        if (!Data)
            return 0;

        return *(const char**)((DWORD64)Data + 0x10);
    }
};