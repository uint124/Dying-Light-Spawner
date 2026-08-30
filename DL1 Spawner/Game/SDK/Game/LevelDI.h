#pragma once

class LevelDI
{
public:
    struct ObjectList
    {
        uintptr_t* Data;
        uint32_t Count;
        uint32_t Capacity;
    };

    struct Extents
    {
        Vector3 Center;
        Vector3 HalfSize;
    };

    static LevelDI* GetLevel()
    {
        const uintptr_t Game = *reinterpret_cast<uintptr_t*>(Offsets::GameModuleBase + 0x1C16348);
        if (!Game)
            return nullptr;

        const uintptr_t Context = *reinterpret_cast<uintptr_t*>(Game + 0x540);
        if (!Context)
            return nullptr;

        return *reinterpret_cast<LevelDI**>(Context + 0xB0);
    }

    ObjectList FindAll(const Vector3& Center, const Vector3& HalfSize = { 20000.0f, 20000.0f, 20000.0f })
    {
        return FindInternal(AllStorage, ARRAYSIZE(AllStorage), Center, HalfSize, 0, false);
    }

    ObjectList FindPlayers(const Vector3& Center, const Vector3& HalfSize = { 20000.0f, 20000.0f, 20000.0f })
    {
        static const uintptr_t RTTI = CRTTIFinder::FindCustomRTTI(Offsets::GameModuleBase, ".?AV?$CRTTIGameScript@VPlayerDI@@@@");
        return RTTI ? FindInternal(PlayerStorage, ARRAYSIZE(PlayerStorage), Center, HalfSize, RTTI, false) : ObjectList{};
    }

    ObjectList FindHumanAI(const Vector3& Center, const Vector3& HalfSize = { 20000.0f, 20000.0f, 20000.0f })
    {
        constexpr uintptr_t HumanAIRTTIRVA = 0x1C5BBC0;
        return FindInternal(HumanAIStorage, ARRAYSIZE(HumanAIStorage), Center, HalfSize, Offsets::GameModuleBase + HumanAIRTTIRVA, false);
    }

    ObjectList FindCharacters(const Vector3& Center, const Vector3& HalfSize = { 20000.0f, 20000.0f, 20000.0f })
    {
        return FindHumanAI(Center, HalfSize);
    }

    ObjectList FindByRTTI(uintptr_t RTTI, const Vector3& Center, const Vector3& HalfSize = { 20000.0f, 20000.0f, 20000.0f }, bool ExactClass = false)
    {
        return RTTI ? FindInternal(GenericStorage, ARRAYSIZE(GenericStorage), Center, HalfSize, RTTI, ExactClass) : ObjectList{};
    }

    ObjectList FindByRTTIName(const char* DecoratedName, const Vector3& Center, const Vector3& HalfSize = { 20000.0f, 20000.0f, 20000.0f }, bool ExactClass = false)
    {
        if (!DecoratedName || !*DecoratedName)
            return {};

        const uintptr_t RTTI = CRTTIFinder::FindCustomRTTI(Offsets::GameModuleBase, DecoratedName);
        return FindByRTTI(RTTI, Center, HalfSize, ExactClass);
    }

private:

    ObjectList FindInternal(uintptr_t* Storage, uint32_t Capacity, const Vector3& Center, const Vector3& HalfSize, uintptr_t RTTI, bool ExactClass)
    {
        if (!Storage || !Capacity)
            return {};

        ObjectList Output{ Storage, 0, Capacity };
        Extents Bounds{ Center, HalfSize };

        if (!FindObjectsInExtents(&Output, &Bounds, RTTI, ExactClass, nullptr))
            return {};

        if (Output.Data != Storage || Output.Count > Capacity)
            return {};

        return { Storage, Output.Count };
    }

    bool FindObjectsInExtents(ObjectList* Output, Extents* Bounds, uintptr_t RTTI, bool ExactClass, uint32_t* Mask)
    {
        using Fn = bool(__fastcall*)(LevelDI* Level, ObjectList* Output, Extents* Bounds, uintptr_t RTTI, bool ExactClass, uint32_t* Mask);
        auto Func = FindExport<Fn>(Offsets::EngineModuleBase, "?FindObjectsInExtents@ILevel@@QEAA_NPEAV?$vector@PEAVIControlObject@@@ttl@@AEBVaabb@@PEBVCRTTI@@_NPEAH@Z");

        if (!Func)
            return false;

        return Func(this, Output, Bounds, RTTI, ExactClass, Mask);
    }

private:
    static inline uintptr_t AllStorage[65536]{};
    static inline uintptr_t PlayerStorage[64]{};
    static inline uintptr_t HumanAIStorage[4096]{};
    static inline uintptr_t GenericStorage[16384]{};
};