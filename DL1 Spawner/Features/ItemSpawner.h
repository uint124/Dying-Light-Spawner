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
        if (!Player || !ResolveItemDefinition(&ItemID))
            return nullptr;

        ItemData Item{};
        ConstructItemData(&Item);
        InitializeItemData(&Item, &ItemID, GetBestDamageContext(), nullptr, nullptr, nullptr, 0, 0);
        return SpawnDroppedItem(&Item, Player, 6);
    }

    static void* SpawnItemWithCraftParts(PlayerDI* Player, int32_t WeaponID, const int32_t* CraftPartIDs, uint32_t CraftPartCount)
    {
        if (!Player || !ResolveItemDefinition(&WeaponID))
            return nullptr;

        uintptr_t CraftParts[16]{};
        uint32_t ValidCount = 0;

        for (uint32_t Index = 0; Index < CraftPartCount && ValidCount < 16; ++Index)
        {
            int32_t CraftPartID = CraftPartIDs[Index];
            void* CraftPart = ResolveItemDefinition(&CraftPartID);

            if (!CraftPart)
                continue;

            const uintptr_t VTable = *reinterpret_cast<uintptr_t*>(CraftPart);
            if (VTable != Offsets::GameModuleBase + 0x1551AE0)
            {
                printf("Ignoring non-craft-part ID=%d VTableRVA=%llX\n", CraftPartID, VTable - Offsets::GameModuleBase);
                continue;
            }

            CraftParts[ValidCount++] = reinterpret_cast<uintptr_t>(CraftPart);
        }

        Array ModifierSources{ CraftParts, ValidCount };

        ItemData Item{};
        ConstructItemData(&Item);
        InitializeItemData(&Item, &WeaponID, GetBestDamageContext(), nullptr, nullptr, ValidCount ? &ModifierSources : nullptr, 0, 0);

        printf("Context=%08X Tier=%u Rarity=%u Sources=%u Modification=%p\n",
            *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(&Item) + 0x28),
            GetTier(&Item),
            GetRarity(&Item),
            *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(&Item) + 0x50),
            *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(&Item) + 0x60));

        return SpawnDroppedItem(&Item, Player, 6);
    }

private:
    static uint32_t AttributeHash(const char* Name)
    {
        uint32_t Hash = 0;

        while (*Name)
            Hash = static_cast<uint8_t>(*Name++) + 97 * Hash;

        return Hash;
    }

    static uint32_t GetBestDamageContext()
    {
        static uint32_t Context = 0;

        if (Context)
            return Context;

        using GetManagerFn = uintptr_t(__fastcall*)();
        using GetRollFn = float(__fastcall*)(uintptr_t, uint32_t, int32_t);

        const auto GetManager = reinterpret_cast<GetManagerFn>(Offsets::GameModuleBase + 0x6F4A20);
        const auto GetRoll = reinterpret_cast<GetRollFn>(Offsets::GameModuleBase + 0x6F9C10);
        const uintptr_t Manager = GetManager();
        const uint32_t DamageHash = AttributeHash("Damage");

        constexpr uint32_t MaximumTier = 127;
        constexpr uint32_t MaximumRarity = 5;

        uint32_t BestSeed = 0;
        float BestRoll = -FLT_MAX;

        for (uint32_t Seed = 0; Seed <= 0x1FFFFF; ++Seed)
        {
            const float Roll = GetRoll(Manager, MaximumRarity, static_cast<int32_t>(DamageHash + Seed));

            if (Roll > BestRoll)
            {
                BestRoll = Roll;
                BestSeed = Seed;
            }
        }

        Context = (BestSeed << 10) | (MaximumTier << 3) | MaximumRarity;
        printf("BestDamageContext=%08X Seed=%06X Roll=%f\n", Context, BestSeed, BestRoll);
        return Context;
    }

    static uint32_t GetTier(ItemData* Item)
    {
        const uint32_t Context = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(Item) + 0x28);
        return (Context >> 3) & 0x7F;
    }

    static uint32_t GetRarity(ItemData* Item)
    {
        const uint32_t Context = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(Item) + 0x28);
        return (Context & 7) % 6;
    }

    static ItemData* ConstructItemData(ItemData* Item)
    {
        using Fn = ItemData * (__fastcall*)(ItemData*);
        return reinterpret_cast<Fn>(Offsets::GameModuleBase + Offsets::Game::ConstructItemDataFunction)(Item);
    }

    static void* InitializeItemData(ItemData* Item, int32_t* ItemID, uint32_t GenerationContext, void* BaseContext, void* AdditionalContext, Array<uint64_t>* ModifierSources, int32_t Tier, int32_t Flags)
    {
        using Fn = void* (__fastcall*)(ItemData*, int32_t*, uint32_t, void*, void*, Array<uint64_t>*, int32_t, int32_t);
        return reinterpret_cast<Fn>(Offsets::GameModuleBase + Offsets::Game::InitializeItemDataFunction)(Item, ItemID, GenerationContext, BaseContext, AdditionalContext, ModifierSources, Tier, Flags);
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