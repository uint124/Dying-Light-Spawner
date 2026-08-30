#pragma once


class EntityManager
{
private:
    using BuildTransform_t = void(__fastcall*)(Transform*, Quaternion*, Vector3*, Vector3*);
    using InitializeControl_t = void(__fastcall*)(uintptr_t, float);
    using InitializeHumanAI_t = void(__fastcall*)(uintptr_t);
    using ApplyTransform_t = void(__fastcall*)(uintptr_t, Transform*);

public:
    static inline uintptr_t SpawnHumanAI(const char* Name, const Vector3& Position)
    {
        if (!Name || !*Name)
            return 0;

        LevelDI* Level = LevelDI::GetLevel();
        if (!Level)
            return 0;

        const uintptr_t RTTI = CRTTIFinder::FindCustomRTTI(Offsets::GameModuleBase, ".?AV?$CRTTIGameScript@VHumanAI@@@@"); //ResolveHumanAIRTTI(Name);
        if (!RTTI)
            return 0;

        char PresetBuffer[256]{};
        const int PresetLength = snprintf(PresetBuffer,sizeof(PresetBuffer), "Character;%s", Name);

        if (PresetLength <= 0 || PresetLength >= sizeof(PresetBuffer))
            return 0;

        GameString Presets{PresetBuffer, static_cast<uint32_t>(PresetLength), static_cast<uint32_t>(PresetLength)};

        const uintptr_t LevelObject = reinterpret_cast<uintptr_t>(Level) + 0x10;

        IGSObject* Object = IGSObject::CreateObject(LevelObject, RTTI, false, reinterpret_cast<char**>(&Presets));

        if (!Object)
        {
            printf("CreateObject failed: %s\n", Name);
            return 0;
        }

        // CreateObject returns the HumanAI::IGSObject subobject at +0x28.
        const uintptr_t HumanAI = reinterpret_cast<uintptr_t>(Object) - 0x28;

        *reinterpret_cast<uint8_t*>(HumanAI + 0x90D) = 0;  // +2317
        *reinterpret_cast<uint8_t*>(HumanAI + 0x15D5) = 1; // +5589
        *reinterpret_cast<uint16_t*>(HumanAI + 0xE68) = 0; // crowd EntityID
        *reinterpret_cast<uint16_t*>(HumanAI + 0xE6A) = 0; // AISpawn source ID

        Quaternion Rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
        Vector3 Euler{};
        Vector3 SpawnPosition = Position;
        Transform WorldTransform{};

        static const auto BuildTransform = reinterpret_cast<BuildTransform_t>(Offsets::GameModuleBase + 0x1131450);

        BuildTransform(&WorldTransform, &Rotation, &Euler, &SpawnPosition);

        const uintptr_t VTable = *reinterpret_cast<uintptr_t*>(HumanAI);
        const auto ApplyTransform = reinterpret_cast<ApplyTransform_t>(*reinterpret_cast<uintptr_t*>(VTable + 0x2C8));

        ApplyTransform(HumanAI, &WorldTransform);

        IGSObject::InitObject(Object, reinterpret_cast<IGSObject*>(LevelObject));

        static const auto InitializeControl = reinterpret_cast<InitializeControl_t>(Offsets::GameModuleBase + 0x1103880);

        static const auto InitializeHumanAI = reinterpret_cast<InitializeHumanAI_t>(Offsets::GameModuleBase + 0x181E50);

        InitializeControl(HumanAI + 0x18, 1.0f);
        InitializeHumanAI(HumanAI);

        printf("Spawned HumanAI name=%s object=%p humanAI=%p position={%.2f, %.2f, %.2f}\n", Name, Object, reinterpret_cast<void*>(HumanAI), Position.x, Position.y, Position.z);

        return HumanAI;
    }
};