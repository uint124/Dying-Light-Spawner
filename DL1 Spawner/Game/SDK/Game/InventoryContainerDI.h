#pragma once

class InventoryContainerDI
{
public:
    struct Inventory
    {
        DWORD ID;
        DWORD Ammo;
        void* InventoryItem;
    };

    struct InventoryAmmo
    {
        char pad_0000[0x40];
        Array<Inventory> InventoryList;
    };

    void GiveAmmo()
    {
        auto ammo = GetInventoryAmmo();
        if (!ammo)
            return;

        auto list = ammo->InventoryList;
        for (int i = 0; i < list.Count; i++)
        {
            auto entry = list[i];
            if (!entry.ID || !entry.InventoryItem)
                continue;

            entry.Ammo = 999;
        }
    }

    InventoryAmmo* GetInventoryAmmo()
    {
        return *(InventoryAmmo**)((DWORD64)this + Offsets::Game::InventoryContainerDI::InventoryAmmo);
    }
};