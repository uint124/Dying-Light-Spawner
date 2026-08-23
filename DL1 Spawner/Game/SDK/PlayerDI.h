#pragma once

class PlayerDI
{
public:
    // Player + 0x938 == InventoryContainerDI

    class InventoryContainerDI* GetInventoryContainer()
    {
        return *(InventoryContainerDI**)((DWORD64)this + Offsets::Game::PlayerDI::InventoryContainer);
    }

    Vector3 GetPosition()
    {
        return *(Vector3*)((DWORD64)this + 0x808);
    }
};