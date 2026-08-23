#pragma once

class ItemSpawner
{
public:
    struct alignas(16) ItemData
    {
        std::byte Data[0xC0]{};
    };

    static void* SpawnItemByID(PlayerDI* Player, int32_t ItemID)
    {
        if (!Player)
            return 0;

        if (!ResolveItemDefinition(&ItemID))
            return 0;

        ItemData Item{};
        ConstructItemData(&Item);
        InitializeItemData(&Item, &ItemID, 0x6A51BC84, 0, nullptr, nullptr, 0, 0);

        void* DroppedItem = SpawnDroppedItem(&Item, Player, 6);
        if (!DroppedItem)
            return 0;

        //GetObjectName((uint64_t*)DroppedItem);

        return DroppedItem;
    }

private:
    static void PrintItemGenerationData(ItemData* Item)
    {
        if (!Item)
            return;

        const uintptr_t Address = reinterpret_cast<uintptr_t>(Item);
        void* Definition = *reinterpret_cast<void**>(Address + 0x30);
        void* BaseContext = *reinterpret_cast<void**>(Address + 0x38);
        void* WeaponModification = *reinterpret_cast<void**>(Address + 0x60);

        printf("Item=%p Definition=%p BaseContext=%p WeaponModification=%p\n", Item, Definition, BaseContext, WeaponModification);
    }

    static ItemData* ConstructItemData(ItemData* Item)
    {
        using Fn = ItemData * (__fastcall*)(ItemData*);
        return reinterpret_cast<Fn>(Offsets::GameModuleBase + Offsets::Game::ConstructItemDataFunction)(Item);
    }

    static void* InitializeItemData(ItemData* Item, int32_t* ItemID, uint32_t GenerationFlags, int32_t Unknown, void* ContextA, void* ContextB, int32_t Tier, int32_t Flags)
    {
        using Fn = void* (__fastcall*)(ItemData*, int32_t*, uint32_t, int32_t, void*, void*, int32_t, int32_t);
        return reinterpret_cast<Fn>(Offsets::GameModuleBase + Offsets::Game::InitializeItemDataFunction)(Item, ItemID, GenerationFlags, Unknown, ContextA, ContextB, Tier, Flags);
    }

    static void* ResolveItemDefinition(int32_t* ItemID)
    {
        using Fn = void* (__fastcall*)(int32_t*, void*);
        return reinterpret_cast<Fn>(Offsets::GameModuleBase + Offsets::Game::ResolveItemDefinitionFunction)(ItemID, nullptr);
    }

    static void* SpawnDroppedItem(ItemData* Item, void* Player, int32_t SpawnContext)
    {
        using Fn = void* (__fastcall*)(ItemData*, void*, int32_t);
        return reinterpret_cast<Fn>(Offsets::GameModuleBase + Offsets::Game::SpawnDroppedItemFunction)(Item, Player, SpawnContext);
    }
};